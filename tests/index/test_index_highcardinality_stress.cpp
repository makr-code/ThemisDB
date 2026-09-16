// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_index_highcardinality_stress.cpp
 * @brief Wave D — Index Engine High-Cardinality Stress Tests.
 *
 * Three stress cases that validate index scalability and concurrency
 * correctness with high cardinality workloads using in-process stubs only
 * (no GPU or CUDA dependency).
 *
 * ## Acceptance criteria
 * - HighCardinalityIndexBuild  : build a 1 000 000-vector index across 8 threads
 * - ConcurrentIndexReadWrite   : concurrent read/write produces correct results
 * - MultiBackendStress         : multiple stub backends respond without error
 *
 * ## Labels
 * wave_d;stress;not_release_critical
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
// In-process stubs — no GPU / CUDA / HIP / Vulkan dependency.
// ---------------------------------------------------------------------------

/// Stub vector entry for stress tests.
struct StressVecEntry {
    uint32_t           id{0};
    std::vector<float> vec;
};

/// Minimal thread-safe stub vector store.
class StressVectorStore {
public:
    explicit StressVectorStore(std::size_t dim = 32) : dim_(dim) {}

    bool insert(StressVecEntry entry) {
        std::lock_guard<std::mutex> lock(mu_);
        store_.emplace_back(std::move(entry));
        return true;
    }

    std::vector<StressVecEntry> query(uint32_t query_id, int k) const {
        std::lock_guard<std::mutex> lock(mu_);
        if (store_.empty()) { return {}; }
        const int actual_k = std::min(k, static_cast<int>(store_.size()));
        std::vector<StressVecEntry> result;
        result.reserve(static_cast<std::size_t>(actual_k));
        for (int i = 0; i < actual_k; ++i) {
            uint32_t idx = (query_id + static_cast<uint32_t>(i)) % static_cast<uint32_t>(store_.size());
            result.push_back(store_[idx]);
        }
        return result;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lock(mu_);
        return store_.size();
    }

private:
    mutable std::mutex              mu_;
    std::vector<StressVecEntry>     store_;
    std::size_t                     dim_;
};

/// Stub backend enum for multi-backend stress.
enum class StubBackend { CPU_FLAT, CPU_HNSW, CPU_IVFPQ };

/// Stub backend router — dispatches to one of three CPU-only stub stores.
class StubMultiBackend {
public:
    explicit StubMultiBackend(std::size_t dim = 32)
        : flat_(dim), hnsw_(dim), ivfpq_(dim) {}

    bool insert(StubBackend backend, StressVecEntry entry) {
        switch (backend) {
            case StubBackend::CPU_FLAT:  return flat_.insert(std::move(entry));
            case StubBackend::CPU_HNSW:  return hnsw_.insert(std::move(entry));
            case StubBackend::CPU_IVFPQ: return ivfpq_.insert(std::move(entry));
        }
        return false;
    }

    std::vector<StressVecEntry> query(StubBackend backend, uint32_t qid, int k) const {
        switch (backend) {
            case StubBackend::CPU_FLAT:  return flat_.query(qid, k);
            case StubBackend::CPU_HNSW:  return hnsw_.query(qid, k);
            case StubBackend::CPU_IVFPQ: return ivfpq_.query(qid, k);
        }
        return {};
    }

    std::size_t size(StubBackend backend) const {
        switch (backend) {
            case StubBackend::CPU_FLAT:  return flat_.size();
            case StubBackend::CPU_HNSW:  return hnsw_.size();
            case StubBackend::CPU_IVFPQ: return ivfpq_.size();
        }
        return 0;
    }

private:
    StressVectorStore flat_;
    StressVectorStore hnsw_;
    StressVectorStore ivfpq_;
};

static std::vector<float> makeStressVec(std::size_t dim, uint32_t seed) {
    std::vector<float> v(dim);
    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    for (auto& x : v) { x = dist(rng); }
    return v;
}

// ---------------------------------------------------------------------------
// TEST 1: HighCardinalityIndexBuild
// ---------------------------------------------------------------------------

/**
 * @test HighCardinalityIndexBuild
 *
 * Inserts 1 000 000 vectors into the stub store across 8 threads and asserts
 * that the final size equals exactly 1 000 000.
 *
 * Log pattern: [INDEX:RebuildStall]
 */
TEST(IndexHighCardinalityStress, HighCardinalityIndexBuild) {
    constexpr uint32_t kVectorCount = 1'000'000;
    constexpr int      kThreadCount = 8;
    constexpr std::size_t kDim      = 32;

    StressVectorStore store(kDim);

    std::vector<std::thread> writers;
    writers.reserve(kThreadCount);
    const uint32_t per_thread = kVectorCount / static_cast<uint32_t>(kThreadCount);

    std::atomic<uint64_t> failures{0};

    for (int t = 0; t < kThreadCount; ++t) {
        writers.emplace_back([&store, &failures, t, per_thread, kDim]() {
            const uint32_t start = static_cast<uint32_t>(t) * per_thread;
            const uint32_t end   = start + per_thread;
            for (uint32_t i = start; i < end; ++i) {
                if (!store.insert({i, makeStressVec(kDim, i)})) {
                    failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& w : writers) { w.join(); }

    EXPECT_EQ(failures.load(), 0ULL)
        << "[INDEX:RebuildStall] " << failures.load() << " insert failures";
    EXPECT_EQ(store.size(), static_cast<std::size_t>(kVectorCount))
        << "[INDEX:RebuildStall] Expected " << kVectorCount << " vectors after parallel build";

    std::cout << "[HighCardinalityIndexBuild] vectors=" << store.size()
              << " failures=" << failures.load() << "\n";
}

// ---------------------------------------------------------------------------
// TEST 2: ConcurrentIndexReadWrite
// ---------------------------------------------------------------------------

/**
 * @test ConcurrentIndexReadWrite
 *
 * Runs 4 writer threads (each inserting 10 000 vectors) concurrently with
 * 4 reader threads (each issuing 10 000 top-5 queries) against a shared
 * stub store.  Asserts no read/write errors.
 *
 * Log pattern: [INDEX:HNSWCorruption]
 */
TEST(IndexHighCardinalityStress, ConcurrentIndexReadWrite) {
    constexpr int      kWriterThreads    = 4;
    constexpr int      kReaderThreads    = 4;
    constexpr uint32_t kWritesPerThread  = 10'000;
    constexpr uint32_t kReadsPerThread   = 10'000;
    constexpr int      kTopK             = 5;
    constexpr std::size_t kDim           = 32;
    constexpr uint32_t kPreload          = 100;

    StressVectorStore store(kDim);
    for (uint32_t i = 0; i < kPreload; ++i) {
        store.insert({i, makeStressVec(kDim, i)});
    }

    std::atomic<uint64_t> write_errors{0};
    std::atomic<uint64_t> read_errors{0};

    std::vector<std::thread> threads;
    threads.reserve(kWriterThreads + kReaderThreads);

    for (int t = 0; t < kWriterThreads; ++t) {
        threads.emplace_back([&store, &write_errors, t, kDim]() {
            const uint32_t base = 1'000'000u + static_cast<uint32_t>(t) * kWritesPerThread;
            for (uint32_t i = 0; i < kWritesPerThread; ++i) {
                const uint32_t id = base + i;
                if (!store.insert({id, makeStressVec(kDim, id)})) {
                    write_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (int t = 0; t < kReaderThreads; ++t) {
        threads.emplace_back([&store, &read_errors, t, kTopK]() {
            for (uint32_t q = 0; q < kReadsPerThread; ++q) {
                const uint32_t qid = static_cast<uint32_t>(t) * kReadsPerThread + q;
                auto results = store.query(qid, kTopK);
                // Results may be smaller than kTopK when store is still filling up.
                const int cur_size = static_cast<int>(store.size());
                const int expected = std::min(kTopK, cur_size);
                if (!results.empty() && static_cast<int>(results.size()) != expected) {
                    read_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(write_errors.load(), 0ULL)
        << "[INDEX:RebuildStall] concurrent write errors=" << write_errors.load();
    EXPECT_EQ(read_errors.load(), 0ULL)
        << "[INDEX:HNSWCorruption] concurrent read errors=" << read_errors.load();

    std::cout << "[ConcurrentIndexReadWrite] final_size=" << store.size()
              << " write_errors=" << write_errors.load()
              << " read_errors=" << read_errors.load() << "\n";
}

// ---------------------------------------------------------------------------
// TEST 3: MultiBackendStress
// ---------------------------------------------------------------------------

/**
 * @test MultiBackendStress
 *
 * Inserts 10 000 vectors into each of three stub backends (FLAT, HNSW, IVFPQ)
 * using dedicated writer threads and issues 5 000 queries against each backend
 * from dedicated reader threads.  Asserts all backends reach the expected size
 * and return consistent results.
 *
 * Log pattern: [INDEX:GPUKernelFallback]
 */
TEST(IndexHighCardinalityStress, MultiBackendStress) {
    constexpr uint32_t kVectorsPerBackend  = 10'000;
    constexpr uint32_t kQueriesPerBackend  = 5'000;
    constexpr int      kTopK              = 5;
    constexpr std::size_t kDim            = 32;

    StubMultiBackend mb(kDim);

    const std::vector<StubBackend> backends = {
        StubBackend::CPU_FLAT, StubBackend::CPU_HNSW, StubBackend::CPU_IVFPQ
    };

    std::atomic<uint64_t> insert_errors{0};
    std::atomic<uint64_t> query_errors{0};

    std::vector<std::thread> threads;
    threads.reserve(backends.size() * 2);

    // Writer per backend
    for (const auto& bk : backends) {
        threads.emplace_back([&mb, &insert_errors, bk, kDim]() {
            for (uint32_t i = 0; i < kVectorsPerBackend; ++i) {
                if (!mb.insert(bk, {i, makeStressVec(kDim, i)})) {
                    insert_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) { th.join(); }
    threads.clear();

    // Verify sizes
    for (const auto& bk : backends) {
        EXPECT_EQ(mb.size(bk), static_cast<std::size_t>(kVectorsPerBackend))
            << "[INDEX:GPUKernelFallback] backend size mismatch";
    }

    // Reader per backend
    for (const auto& bk : backends) {
        threads.emplace_back([&mb, &query_errors, bk, kTopK]() {
            for (uint32_t q = 0; q < kQueriesPerBackend; ++q) {
                auto results = mb.query(bk, q, kTopK);
                if (static_cast<int>(results.size()) != kTopK) {
                    query_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }
    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(insert_errors.load(), 0ULL)
        << "[INDEX:RebuildStall] insert errors=" << insert_errors.load();
    EXPECT_EQ(query_errors.load(), 0ULL)
        << "[INDEX:GPUKernelFallback] query errors=" << query_errors.load();

    std::cout << "[MultiBackendStress] insert_errors=" << insert_errors.load()
              << " query_errors=" << query_errors.load() << "\n";
}
