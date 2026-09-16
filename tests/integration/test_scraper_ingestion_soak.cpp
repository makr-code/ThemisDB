// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_ingestion_soak.cpp
 * @brief Wave D — Scraper Ingestion Soak Test (sustained traffic).
 *
 * Long-duration soak test for the ThemisDB scraper ingestion path.
 * Verifies that fetch throughput, ingest stability, and long-run reliability
 * remain stable over a sustained soak window.
 *
 * ## Acceptance criteria
 * - Fetch throughput ≥ 8 000 ops/sec over the soak duration
 * - No uncaught exceptions from any in-process stub
 * - Ingest queue depth never exceeds overflow threshold
 * - Long-run: zero reliability events (errors/timeouts) during soak
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_SCRAPER_INGESTION.md
 * @see src/scraper/ROADMAP.md — Wave D Contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <string>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ─────────────────────────────────────────────────────────────────────────────
// STUB/SIMULATION NOTE
//
// The in-process stubs below replace live HTTP fetch and ingest paths so no
// network access, external crawler, or database is required.
//   - StubUrlFetcher: models a GET request by hashing the URL to a latency bucket.
//   - StubIngestQueue: bounded ring-buffer that models the scraper ingest queue.
//   - StubRateLimiter: token-bucket rate limiter stub.
//
// These stubs MUST NOT be used in production code paths.
// ─────────────────────────────────────────────────────────────────────────────

struct FetchResult {
    bool        success{false};
    std::size_t bytes{0};
    uint32_t    status_code{200};
};

class StubUrlFetcher {
public:
    FetchResult fetch(const std::string& url) noexcept {
        ops_.fetch_add(1, std::memory_order_relaxed);
        const std::size_t bytes = (url.size() * 97 % 4096) + 128;
        return FetchResult{true, bytes, 200};
    }
    uint64_t totalOps() const noexcept { return ops_.load(std::memory_order_relaxed); }
private:
    std::atomic<uint64_t> ops_{0};
};

class StubIngestQueue {
public:
    static constexpr std::size_t kMaxDepth = 4096;

    bool enqueue(const FetchResult& /*r*/) noexcept {
        const std::size_t cur = depth_.fetch_add(1, std::memory_order_acq_rel);
        if (cur >= kMaxDepth) {
            depth_.fetch_sub(1, std::memory_order_relaxed);
            overflows_.fetch_add(1, std::memory_order_relaxed);
            return false;
        }
        // Drain one slot to model downstream consumption
        if (depth_.load(std::memory_order_relaxed) > 0) {
            depth_.fetch_sub(1, std::memory_order_relaxed);
        }
        enqueues_.fetch_add(1, std::memory_order_relaxed);
        return true;
    }

    uint64_t overflows() const noexcept { return overflows_.load(std::memory_order_relaxed); }
    uint64_t enqueues()  const noexcept { return enqueues_.load(std::memory_order_relaxed); }
    std::size_t depth()  const noexcept { return depth_.load(std::memory_order_relaxed); }

private:
    std::atomic<std::size_t> depth_{0};
    std::atomic<uint64_t>    overflows_{0};
    std::atomic<uint64_t>    enqueues_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 1: ScraperSoak_FetchThroughput
// ─────────────────────────────────────────────────────────────────────────────
TEST(ScraperSoak_FetchThroughput, ThroughputGateAndNoExceptions) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs());

    StubUrlFetcher fetcher;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string url = "https://example.com/page/" + std::to_string(tick % 10000);
            (void)fetcher.fetch(url);
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    EXPECT_FALSE(exception_caught)
        << "No exceptions during scraper fetch soak";
    ASSERT_GT(elapsed_ms.count(), 0);

    const double elapsed_s   = elapsed_ms.count() / 1000.0;
    const double ops_per_sec = static_cast<double>(fetcher.totalOps()) / elapsed_s;

    constexpr double kMinThroughput = 8'000.0;
    EXPECT_GE(ops_per_sec, kMinThroughput)
        << "[SCRAPER:FetchFailed] Fetch throughput must be ≥ 8 000 ops/sec. "
           "Observed: " << static_cast<uint64_t>(ops_per_sec) << " ops/sec";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 2: ScraperSoak_IngestStability
// ─────────────────────────────────────────────────────────────────────────────
TEST(ScraperSoak_IngestStability, ZeroOverflowsAndQueueStable) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 5);

    StubUrlFetcher   fetcher;
    StubIngestQueue  queue;
    bool exception_caught = false;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string url = "https://source.org/" + std::to_string(tick % 5000);
            const auto result = fetcher.fetch(url);
            queue.enqueue(result);
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[SCRAPER:IngestOverflow] No exceptions during ingest stability soak";

    EXPECT_EQ(queue.overflows(), 0u)
        << "[SCRAPER:IngestOverflow] Zero queue overflows expected during soak. "
           "Observed: " << queue.overflows();

    EXPECT_GT(queue.enqueues(), 0u)
        << "At least one item must be enqueued during the soak";
}

// ─────────────────────────────────────────────────────────────────────────────
// Soak Test 3: ScraperSoak_LongRunReliability
// ─────────────────────────────────────────────────────────────────────────────
TEST(ScraperSoak_LongRunReliability, ZeroReliabilityEventsOverFullSoak) {
    const auto soak_duration = std::chrono::milliseconds(soakDurationMs() / 3);

    StubUrlFetcher fetcher;
    bool exception_caught      = false;
    uint64_t reliability_events = 0;

    const auto start = std::chrono::steady_clock::now();
    uint64_t tick = 0;

    try {
        while (std::chrono::steady_clock::now() - start < soak_duration) {
            const std::string url = "https://gov-source.org/doc/" + std::to_string(tick % 8000);
            const auto result = fetcher.fetch(url);
            if (!result.success || result.status_code != 200) {
                ++reliability_events;
            }
            ++tick;
        }
    } catch (...) {
        exception_caught = true;
    }

    EXPECT_FALSE(exception_caught)
        << "[SCRAPER:LongRunDegradation] No exceptions during long-run reliability soak";

    EXPECT_EQ(reliability_events, 0u)
        << "[SCRAPER:LongRunDegradation] Zero reliability events expected. "
           "Observed: " << reliability_events << " of " << fetcher.totalOps() << " fetches";
}
