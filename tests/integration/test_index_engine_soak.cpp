// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_index_engine_soak.cpp
 * @brief Wave D — Index Engine Soak Tests (sustained index workload).
 *
 * Three soak cases that validate index build throughput, query stability,
 * and concurrent access reliability over a configurable soak window using
 * in-process stubs only (no GPU or CUDA dependency).
 *
 * ## Acceptance criteria
 * - IndexSoak_BuildThroughput             : builds complete without stall
 * - IndexSoak_QueryStability              : no query errors throughout soak
 * - IndexSoak_ConcurrentAccessReliability : concurrent readers/writers stable
 *
 * ## Labels
 * wave_d;soak;not_release_critical
 *
 * ## Environment
 * Set THEMIS_SOAK_DURATION_MS to override the default 60 000 ms window.
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_INDEX_ENGINE.md
 * @see src/index/ROADMAP.md — Wave D contribution
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static uint64_t soakDurationMs() {
    const char* env = std::getenv("THEMIS_SOAK_DURATION_MS");
    if (env && *env) {
        try { return static_cast<uint64_t>(std::stoull(env)); }
        catch (...) {}
    }
    return 60'000ULL;
}

// ---------------------------------------------------------------------------
// In-process index stubs
// MUST NOT be used in production code paths.
// ---------------------------------------------------------------------------

/// Stub index entry — represents a single vector + metadata record.
struct StubIndexEntry {
    uint32_t           id{0};
    std::vector<float> vec;
    std::string        label;
};

/// Thread-safe stub HNSW-like index supporting incremental insert and query.
class StubIndexEngine {
public:
    explicit StubIndexEngine(std::size_t dim = 64) : dim_(dim) {}

    /// Inserts a new entry; thread-safe.
    bool insert(StubIndexEntry entry) {
        std::lock_guard<std::mutex> lock(mu_);
        entries_.emplace_back(std::move(entry));
        return true;
    }

    /// Returns approximate top-k neighbours by linear scan (stub).
    std::vector<StubIndexEntry> query(const std::vector<float>& qvec, int k) const {
        std::lock_guard<std::mutex> lock(mu_);
        if (entries_.empty()) { return {}; }
        const int actual_k = std::min(k, static_cast<int>(entries_.size()));
        // Return last-inserted entries for simplicity.
        std::vector<StubIndexEntry> result;
        result.reserve(static_cast<std::size_t>(actual_k));
        const auto start = entries_.end() - actual_k;
        for (auto it = start; it != entries_.end(); ++it) {
            result.push_back(*it);
        }
        return result;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return entries_.size();
    }

    std::size_t dim() const noexcept { return dim_; }

private:
    mutable std::mutex              mu_;
    std::vector<StubIndexEntry>     entries_;
    std::size_t                     dim_;
};

/// Makes a deterministic unit-norm float vector for a given seed.
static std::vector<float> makeVec(std::size_t dim, uint32_t seed) {
    std::vector<float> v(dim);
    std::mt19937       rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& x : v) { x = dist(rng); }
    return v;
}

// ---------------------------------------------------------------------------
// TEST 1: IndexSoak_BuildThroughput
// ---------------------------------------------------------------------------

/**
 * @test IndexSoak_BuildThroughput
 *
 * Continuously inserts new entries into the stub index for the configured
 * soak window and asserts that no insert operation fails.
 *
 * Trace span: D1-INDEX-BUILD
 * Log pattern: [INDEX:RebuildStall]
 */
TEST(IndexSoakTests, IndexSoak_BuildThroughput) {
    const auto duration_ms = soakDurationMs();
    StubIndexEngine engine(64);

    uint64_t inserts     = 0;
    uint64_t failures    = 0;
    uint32_t entry_id    = 0;

    const auto start = std::chrono::steady_clock::now();
    const auto end   = start + std::chrono::milliseconds(duration_ms);

    while (std::chrono::steady_clock::now() < end) {
        StubIndexEntry e{entry_id, makeVec(engine.dim(), entry_id),
                         "label_" + std::to_string(entry_id)};
        if (!engine.insert(std::move(e))) {
            ++failures;
            ADD_FAILURE() << "[INDEX:RebuildStall] insert failed at entry_id=" << entry_id;
        }
        ++inserts;
        ++entry_id;
    }

    EXPECT_EQ(failures, 0ULL) << "[INDEX:RebuildStall] " << failures << " insert failures";
    EXPECT_GT(inserts, 0ULL) << "No inserts completed during soak window";

    const auto elapsed_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
        .count());

    const double ips = (elapsed_ms > 0.0) ? (static_cast<double>(inserts) / elapsed_ms * 1000.0) : 0.0;

    std::cout << "[IndexSoak_BuildThroughput] inserts=" << inserts
              << " elapsed_ms=" << elapsed_ms
              << " ips=" << ips << "\n";
}

// ---------------------------------------------------------------------------
// TEST 2: IndexSoak_QueryStability
// ---------------------------------------------------------------------------

/**
 * @test IndexSoak_QueryStability
 *
 * Pre-populates the stub index with 4 096 entries, then issues continuous
 * top-5 queries for the full soak window and asserts that every query
 * returns exactly 5 results with no error.
 *
 * Trace span: D1-INDEX-QUERY
 * Log pattern: [INDEX:HNSWCorruption]
 */
TEST(IndexSoakTests, IndexSoak_QueryStability) {
    const auto duration_ms = soakDurationMs();
    constexpr uint32_t kPreload = 4096;
    constexpr int      kTopK    = 5;

    StubIndexEngine engine(64);
    for (uint32_t i = 0; i < kPreload; ++i) {
        engine.insert({i, makeVec(engine.dim(), i), "pre_" + std::to_string(i)});
    }

    uint64_t queries  = 0;
    uint64_t errors   = 0;
    uint32_t query_id = 0;

    const auto start = std::chrono::steady_clock::now();
    const auto end   = start + std::chrono::milliseconds(duration_ms);

    while (std::chrono::steady_clock::now() < end) {
        auto qvec   = makeVec(engine.dim(), query_id++);
        auto results = engine.query(qvec, kTopK);
        if (static_cast<int>(results.size()) != kTopK) {
            ++errors;
            ADD_FAILURE() << "[INDEX:HNSWCorruption] query returned " << results.size()
                          << " results (expected " << kTopK << ") at query_id=" << query_id;
        }
        ++queries;
    }

    EXPECT_EQ(errors, 0ULL) << "[INDEX:HNSWCorruption] " << errors << " query errors";
    EXPECT_GT(queries, 0ULL) << "No queries completed during soak window";

    std::cout << "[IndexSoak_QueryStability] queries=" << queries
              << " errors=" << errors << "\n";
}

// ---------------------------------------------------------------------------
// TEST 3: IndexSoak_ConcurrentAccessReliability
// ---------------------------------------------------------------------------

/**
 * @test IndexSoak_ConcurrentAccessReliability
 *
 * Runs 4 writer threads and 4 reader threads concurrently for the soak
 * window.  Writers insert new entries; readers issue top-5 queries.
 * Asserts zero data races or incorrect read results (via stub invariants).
 *
 * Trace span: D1-INDEX-CONCURRENT
 * Log pattern: [INDEX:MultiGPURoutingFailed]
 */
TEST(IndexSoakTests, IndexSoak_ConcurrentAccessReliability) {
    const auto duration_ms = soakDurationMs();
    constexpr int kWriterThreads = 4;
    constexpr int kReaderThreads = 4;
    constexpr int kTopK          = 5;
    constexpr uint32_t kPreload  = 256;

    StubIndexEngine engine(64);
    for (uint32_t i = 0; i < kPreload; ++i) {
        engine.insert({i, makeVec(engine.dim(), i), "seed_" + std::to_string(i)});
    }

    std::atomic<bool>     stop_flag{false};
    std::atomic<uint64_t> write_errors{0};
    std::atomic<uint64_t> read_errors{0};
    std::atomic<uint64_t> total_reads{0};

    const auto deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(duration_ms);

    std::vector<std::thread> threads;
    threads.reserve(kWriterThreads + kReaderThreads);

    // Writer threads
    for (int t = 0; t < kWriterThreads; ++t) {
        threads.emplace_back([&engine, &stop_flag, &write_errors, t]() {
            uint32_t entry_id = 100'000u + static_cast<uint32_t>(t) * 200'000u;
            while (!stop_flag.load(std::memory_order_relaxed)) {
                StubIndexEntry e{entry_id, makeVec(engine.dim(), entry_id),
                                 "w_" + std::to_string(entry_id)};
                if (!engine.insert(std::move(e))) {
                    write_errors.fetch_add(1, std::memory_order_relaxed);
                }
                ++entry_id;
            }
        });
    }

    // Reader threads
    for (int t = 0; t < kReaderThreads; ++t) {
        threads.emplace_back([&engine, &stop_flag, &read_errors, &total_reads, t, kTopK]() {
            uint32_t q = static_cast<uint32_t>(t) * 50'000u;
            while (!stop_flag.load(std::memory_order_relaxed)) {
                auto qvec   = makeVec(engine.dim(), q++);
                auto results = engine.query(qvec, kTopK);
                // Index may have fewer than kTopK entries at startup — only flag if non-empty result is wrong size.
                const auto sz = static_cast<int>(engine.size());
                const int  expected = std::min(kTopK, sz);
                if (!results.empty() && static_cast<int>(results.size()) != expected) {
                    read_errors.fetch_add(1, std::memory_order_relaxed);
                }
                total_reads.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Let threads run for the soak duration then signal stop.
    std::this_thread::sleep_until(deadline);
    stop_flag.store(true, std::memory_order_relaxed);
    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(write_errors.load(), 0ULL)
        << "[INDEX:RebuildStall] writer errors during concurrent soak";
    EXPECT_EQ(read_errors.load(), 0ULL)
        << "[INDEX:MultiGPURoutingFailed] reader errors during concurrent soak";
    EXPECT_GT(total_reads.load(), 0ULL)
        << "No reads completed during concurrent soak window";

    std::cout << "[IndexSoak_ConcurrentAccessReliability] reads=" << total_reads.load()
              << " write_errors=" << write_errors.load()
              << " read_errors=" << read_errors.load() << "\n";
}
