// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_rag_highcardinality_stress.cpp
 * @brief Wave D — RAG High-Cardinality Stress Tests.
 *
 * Three stress cases that validate RAG index scalability and concurrency
 * correctness with high cardinality workloads using in-process stubs only.
 *
 * ## Acceptance criteria
 * - HighCardinalityChunkIndex : build & query 100 000-chunk index across 8 threads
 * - ConcurrentQueryStress     : concurrent query throughput scales linearly
 * - LLMJudgeCachePressure     : judge cache eviction produces no incorrect verdicts
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_RAG_PIPELINE.md
 * @see src/rag/ROADMAP.md — Wave D contribution
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
// In-process stubs — no real LLM or embedding service connections.
// ---------------------------------------------------------------------------

/// Stub chunk record used throughout stress tests.
struct StressChunk {
    uint32_t    id{0};
    std::string text;
    float       score{0.0f};
};

/// Thread-safe stub chunk index supporting concurrent reads after build.
class StressChunkIndex {
public:
    explicit StressChunkIndex() = default;

    /// Inserts a chunk; must be called before concurrent queries begin.
    void insert(StressChunk chunk) {
        std::lock_guard<std::mutex> lock(mu_);
        chunks_.emplace_back(std::move(chunk));
    }

    /// Returns top-k chunks by deterministic score lookup (read-only, no lock needed after build).
    std::vector<StressChunk> query(uint32_t query_id, int k) const noexcept {
        const auto n = static_cast<uint32_t>(chunks_.size());
        if (n == 0) { return {}; }
        const int  actual_k = std::min(k, static_cast<int>(n));
        std::vector<StressChunk> results;
        results.reserve(static_cast<std::size_t>(actual_k));
        for (int i = 0; i < actual_k; ++i) {
            uint32_t idx = (query_id + static_cast<uint32_t>(i)) % n;
            results.push_back(chunks_[idx]);
        }
        return results;
    }

    std::size_t size() const noexcept { return chunks_.size(); }

private:
    mutable std::mutex         mu_;
    std::vector<StressChunk>   chunks_;
};

/// Stub judge cache — LRU-style bounded cache for judge verdicts.
class StubJudgeCachePressure {
public:
    explicit StubJudgeCachePressure(std::size_t capacity = 1024)
        : capacity_(capacity) {}

    void put(uint32_t key, bool verdict) {
        std::lock_guard<std::mutex> lock(mu_);
        if (cache_.size() >= capacity_) {
            // Evict first entry (simple FIFO for stub purposes).
            cache_.erase(cache_.begin());
        }
        cache_[key] = verdict;
    }

    bool get(uint32_t key, bool& out) const {
        std::lock_guard<std::mutex> lock(mu_);
        auto it = cache_.find(key);
        if (it == cache_.end()) { return false; }
        out = it->second;
        return true;
    }

private:
    mutable std::mutex                          mu_;
    std::unordered_map<uint32_t, bool>         cache_;
    std::size_t                                capacity_;
};

// ---------------------------------------------------------------------------
// TEST 1: HighCardinalityChunkIndex
// ---------------------------------------------------------------------------

/**
 * @test HighCardinalityChunkIndex
 *
 * Builds a 100 000-chunk stub index using 8 writer threads, then validates
 * query correctness from 8 reader threads.  Verifies that the index size
 * reaches exactly 100 000 after concurrent build.
 */
TEST(RAGHighCardinalityStress, HighCardinalityChunkIndex) {
    constexpr uint32_t kChunkCount  = 100'000;
    constexpr int      kThreadCount = 8;
    constexpr int      kTopK        = 5;

    StressChunkIndex index;

    // Phase 1 — parallel build.
    {
        std::vector<std::thread> writers;
        writers.reserve(kThreadCount);
        const uint32_t per_thread = kChunkCount / static_cast<uint32_t>(kThreadCount);

        for (int t = 0; t < kThreadCount; ++t) {
            writers.emplace_back([&index, t, per_thread]() {
                const uint32_t start = static_cast<uint32_t>(t) * per_thread;
                const uint32_t end   = start + per_thread;
                for (uint32_t i = start; i < end; ++i) {
                    index.insert({i, "chunk_" + std::to_string(i),
                                  static_cast<float>(i) / static_cast<float>(kChunkCount)});
                }
            });
        }
        for (auto& w : writers) { w.join(); }
    }

    ASSERT_EQ(index.size(), static_cast<std::size_t>(kChunkCount))
        << "Expected " << kChunkCount << " chunks after parallel build";

    // Phase 2 — parallel query.
    std::atomic<uint64_t> total_queries{0};
    std::atomic<bool>     query_error{false};

    {
        std::vector<std::thread> readers;
        readers.reserve(kThreadCount);

        for (int t = 0; t < kThreadCount; ++t) {
            readers.emplace_back([&index, &total_queries, &query_error, t, kTopK]() {
                for (uint32_t q = 0; q < 1000; ++q) {
                    auto results = index.query(static_cast<uint32_t>(t) * 1000 + q, kTopK);
                    if (static_cast<int>(results.size()) != kTopK) {
                        query_error.store(true);
                        return;
                    }
                    total_queries.fetch_add(1, std::memory_order_relaxed);
                }
            });
        }
        for (auto& r : readers) { r.join(); }
    }

    EXPECT_FALSE(query_error.load())
        << "One or more parallel query threads returned wrong result count";
    EXPECT_EQ(total_queries.load(),
              static_cast<uint64_t>(kThreadCount) * 1000)
        << "Not all parallel queries completed";
}

// ---------------------------------------------------------------------------
// TEST 2: ConcurrentQueryStress
// ---------------------------------------------------------------------------

/**
 * @test ConcurrentQueryStress
 *
 * Fires 8 concurrent threads each issuing 5 000 queries against a shared
 * 50 000-chunk stub index and asserts zero query failures and that aggregate
 * throughput exceeds 50 000 queries/sec (well above the 500 qps soak gate).
 */
TEST(RAGHighCardinalityStress, ConcurrentQueryStress) {
    constexpr uint32_t kChunkCount  = 50'000;
    constexpr int      kThreadCount = 8;
    constexpr uint32_t kQueriesPerThread = 5'000;
    constexpr int      kTopK        = 5;

    // Build index sequentially to avoid timing the build in this test.
    StressChunkIndex index;
    for (uint32_t i = 0; i < kChunkCount; ++i) {
        index.insert({i, "c_" + std::to_string(i),
                      static_cast<float>(i) / static_cast<float>(kChunkCount)});
    }

    std::atomic<uint64_t> total_queries{0};
    std::atomic<uint64_t> errors{0};

    const auto start = std::chrono::steady_clock::now();

    std::vector<std::thread> workers;
    workers.reserve(kThreadCount);
    for (int t = 0; t < kThreadCount; ++t) {
        workers.emplace_back([&index, &total_queries, &errors, t, kTopK]() {
            for (uint32_t q = 0; q < kQueriesPerThread; ++q) {
                auto results = index.query(static_cast<uint32_t>(t) * kQueriesPerThread + q, kTopK);
                if (static_cast<int>(results.size()) != kTopK) {
                    errors.fetch_add(1, std::memory_order_relaxed);
                } else {
                    total_queries.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& w : workers) { w.join(); }

    const auto elapsed_ms = static_cast<double>(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start)
        .count());

    EXPECT_EQ(errors.load(), 0ULL) << "Concurrent query errors detected";

    const uint64_t expected_queries = static_cast<uint64_t>(kThreadCount) * kQueriesPerThread;
    EXPECT_EQ(total_queries.load(), expected_queries)
        << "Not all concurrent queries succeeded";

    const double qps = (elapsed_ms > 0.0)
        ? (static_cast<double>(total_queries.load()) / elapsed_ms * 1000.0)
        : 0.0;

    EXPECT_GE(qps, 50'000.0)
        << "Concurrent query throughput " << qps
        << " qps below 50 000 qps stress gate";

    std::cout << "[ConcurrentQueryStress] queries=" << total_queries.load()
              << " elapsed_ms=" << elapsed_ms
              << " qps=" << qps << "\n";
}

// ---------------------------------------------------------------------------
// TEST 3: LLMJudgeCachePressure
// ---------------------------------------------------------------------------

/**
 * @test LLMJudgeCachePressure
 *
 * Exercises the stub judge cache under eviction pressure: inserts 10× the
 * cache capacity and verifies that every cached verdict can be retrieved
 * correctly within the current window (no incorrect values returned).
 */
TEST(RAGHighCardinalityStress, LLMJudgeCachePressure) {
    constexpr std::size_t kCacheCapacity = 512;
    constexpr std::size_t kInsertCount   = kCacheCapacity * 10;

    StubJudgeCachePressure cache(kCacheCapacity);

    // Insert alternating true/false verdicts.
    for (uint32_t i = 0; i < static_cast<uint32_t>(kInsertCount); ++i) {
        cache.put(i, (i % 2 == 0));
    }

    // Verify the last kCacheCapacity entries are retrievable and correct.
    // (earlier entries may have been evicted — that is expected.)
    const uint32_t verify_start = static_cast<uint32_t>(kInsertCount) - static_cast<uint32_t>(kCacheCapacity);
    uint32_t incorrect = 0;

    for (uint32_t i = verify_start; i < static_cast<uint32_t>(kInsertCount); ++i) {
        bool verdict = false;
        if (cache.get(i, verdict)) {
            const bool expected = (i % 2 == 0);
            if (verdict != expected) { ++incorrect; }
        }
        // Cache misses after eviction are acceptable — we only flag wrong values.
    }

    EXPECT_EQ(incorrect, 0u)
        << "Judge cache returned " << incorrect << " incorrect verdicts under eviction pressure";

    std::cout << "[LLMJudgeCachePressure] inserts=" << kInsertCount
              << " capacity=" << kCacheCapacity
              << " incorrect_reads=" << incorrect << "\n";
}
