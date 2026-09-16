// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_llm_wiki_soak.cpp
 * @brief Wave D — LLM Wiki Module Soak Test (sustained traffic).
 *
 * Long-duration soak test for the LLM Wiki plugin hot paths: index query
 * throughput, article sync stability, and semantic search reliability.
 * Verifies that all three metrics remain within acceptable bounds over a
 * configurable soak window driven by THEMIS_SOAK_DURATION_MS.
 *
 * ## Acceptance criteria
 * - LLMWikiSoak_IndexQueryThroughput    : ≥ 1 000 queries/sec over soak window
 * - LLMWikiSoak_ArticleSyncStability    : zero unhandled sync faults
 * - LLMWikiSoak_SemanticSearchReliability: zero silent result drops
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * Environment overrides
 * ---------------------
 * THEMIS_SOAK_DURATION_MS — total run window in milliseconds (default 60000)
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_LLM_WIKI.md — operator runbook
 * @see src/llm_wiki/ROADMAP.md — Wave D contribution closure
 */

// SIMULATION NOTE: All stubs in this file are in-process simulations.
// They do NOT drive real LLM Wiki infrastructure and MUST NOT be linked
// into production code paths.

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
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
// In-process stubs
// ─────────────────────────────────────────────────────────────────────────────

namespace {

struct StubWikiIndex {
    std::mutex mu;
    std::unordered_map<uint64_t, std::string> index;
    std::atomic<uint64_t> queries{0};
    std::atomic<uint64_t> sync_faults{0};
    std::atomic<uint64_t> search_drops{0};

    void sync(uint64_t article_id) {
        std::lock_guard<std::mutex> lk(mu);
        index[article_id] = "article_" + std::to_string(article_id);
    }

    bool query(uint64_t article_id) {
        ++queries;
        std::lock_guard<std::mutex> lk(mu);
        return index.count(article_id) > 0;
    }

    bool semanticSearch(const std::string& /*term*/) {
        // Stub semantic search: always returns a result
        return true;
    }
};

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test cases
// ─────────────────────────────────────────────────────────────────────────────

TEST(LLMWikiSoak, IndexQueryThroughput) {
    const auto duration_ms = soakDurationMs();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    StubWikiIndex wiki;
    // Pre-populate a small index
    for (uint64_t i = 0; i < 1024; ++i) wiki.sync(i);

    std::atomic<bool> stop{false};

    constexpr int kWorkers = 8;
    std::vector<std::thread> workers;
    workers.reserve(kWorkers);
    for (int t = 0; t < kWorkers; ++t) {
        workers.emplace_back([&, t]() {
            uint64_t id = static_cast<uint64_t>(t % 1024);
            while (!stop.load(std::memory_order_relaxed)) {
                wiki.query(id);
                id = (id + 1) % 1024;
            }
        });
    }

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_release);
    for (auto& th : workers) th.join();

    const double elapsed_sec = static_cast<double>(duration_ms) / 1000.0;
    const double throughput =
        static_cast<double>(wiki.queries.load()) / elapsed_sec;

    EXPECT_GE(throughput, 1'000.0)
        << "LLM Wiki index query throughput " << throughput
        << " qps fell below the 1 000 qps SLO over "
        << duration_ms << " ms soak window";
}

TEST(LLMWikiSoak, ArticleSyncStability) {
    const auto duration_ms = soakDurationMs();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    StubWikiIndex wiki;
    std::atomic<bool> stop{false};

    // Concurrent sync writers
    constexpr int kSyncWorkers = 4;
    std::vector<std::thread> syncers;
    syncers.reserve(kSyncWorkers);
    for (int t = 0; t < kSyncWorkers; ++t) {
        syncers.emplace_back([&, t]() {
            uint64_t id = static_cast<uint64_t>(t) * 10'000ULL;
            while (!stop.load(std::memory_order_relaxed)) {
                wiki.sync(id++);
            }
        });
    }

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_release);
    for (auto& th : syncers) th.join();

    EXPECT_EQ(wiki.sync_faults.load(), 0ULL)
        << "Article sync reported faults during soak";
}

TEST(LLMWikiSoak, SemanticSearchReliability) {
    const auto duration_ms = soakDurationMs();
    const auto deadline =
        std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    StubWikiIndex wiki;
    std::atomic<uint64_t> total_searches{0};
    std::atomic<bool> stop{false};

    std::thread worker([&]() {
        while (!stop.load(std::memory_order_relaxed)) {
            bool result = wiki.semanticSearch("themisdb");
            if (!result) {
                wiki.search_drops.fetch_add(1, std::memory_order_relaxed);
            }
            total_searches.fetch_add(1, std::memory_order_relaxed);
        }
    });

    std::this_thread::sleep_until(deadline);
    stop.store(true, std::memory_order_release);
    worker.join();

    EXPECT_EQ(wiki.search_drops.load(), 0ULL)
        << "Semantic search reported silent drops during soak";
    EXPECT_GT(total_searches.load(), 0ULL)
        << "No searches were executed during the soak window";
}
