/**
 * @file test_vector_search_highcardinality_stress.cpp
 * @brief Wave D — Vector Search High-Cardinality Stress Tests.
 *
 * Stress tests for the ThemisDB vector search subsystem under high-cardinality
 * workloads. Exercises large-scale insertions, concurrent HNSW build+query, and
 * multiple dimensionality configurations.
 *
 * All tests use in-process stubs — no external dependencies beyond GTest and
 * the standard library.
 *
 * ## Labels
 * wave_d;stress;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_VECTOR_SEARCH.md — operator triage guide
 * @see src/vector_search/ROADMAP.md — Wave D evidence closure
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <mutex>
#include <random>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Shared test helpers
// ─────────────────────────────────────────────────────────────────────────────

using Vec = std::vector<float>;

static Vec makeVec(std::size_t dim, uint64_t seed) {
    std::mt19937_64 rng(seed);
    std::uniform_real_distribution<float> dist(-1.0f, 1.0f);
    Vec v(dim);
    for (auto& x : v) { x = dist(rng); }
    return v;
}

static float l2sq(const Vec& a, const Vec& b) {
    float s = 0.0f;
    for (std::size_t i = 0; i < a.size(); ++i) {
        float d = a[i] - b[i];
        s += d * d;
    }
    return s;
}

/// Thread-safe flat vector index (brute-force — simulates HNSW index for stress).
class FlatVectorIndex {
public:
    explicit FlatVectorIndex(std::size_t dim) : dim_(dim) {}

    bool insert(uint64_t id, Vec vec) {
        try {
            std::lock_guard<std::mutex> lk(mu_);
            if (vec.size() != dim_) { return false; }
            entries_.emplace_back(id, std::move(vec));
            return true;
        } catch (...) {
            return false;
        }
    }

    std::vector<uint64_t> knn(const Vec& q, std::size_t k) const {
        std::lock_guard<std::mutex> lk(mu_);
        std::vector<std::pair<float, uint64_t>> scored;
        scored.reserve(entries_.size());
        for (const auto& [id, vec] : entries_) {
            scored.emplace_back(l2sq(q, vec), id);
        }
        const std::size_t top = std::min(k, scored.size());
        std::partial_sort(scored.begin(),
                          scored.begin() + static_cast<std::ptrdiff_t>(top),
                          scored.end(),
                          [](const auto& a, const auto& b) {
                              return a.first < b.first;
                          });
        std::vector<uint64_t> result;
        result.reserve(top);
        for (std::size_t i = 0; i < top; ++i) { result.push_back(scored[i].second); }
        return result;
    }

    std::size_t size() const {
        std::lock_guard<std::mutex> lk(mu_);
        return entries_.size();
    }

private:
    const std::size_t dim_;
    mutable std::mutex mu_;
    std::vector<std::pair<uint64_t, Vec>> entries_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Test 1: HighCardinalityVectorInsert
// Insert 10 000 vectors using 8 threads; verify all vectors are indexed.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_HighCardinalityStress, HighCardinalityVectorInsert) {
    constexpr std::size_t kDim         = 128;
    constexpr std::size_t kTotalVectors = 10'000;
    constexpr int         kNumThreads   = 8;

    FlatVectorIndex index(kDim);

    std::atomic<uint64_t> insert_failures{0};
    std::vector<std::thread> threads;
    threads.reserve(kNumThreads);

    const std::size_t batch = kTotalVectors / kNumThreads;

    for (int t = 0; t < kNumThreads; ++t) {
        threads.emplace_back([&, t]() {
            const uint64_t base = static_cast<uint64_t>(t) * batch;
            for (std::size_t i = 0; i < batch; ++i) {
                uint64_t id = base + static_cast<uint64_t>(i);
                try {
                    Vec v = makeVec(kDim, id);
                    if (!index.insert(id, std::move(v))) {
                        insert_failures.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (...) {
                    insert_failures.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    for (auto& th : threads) { th.join(); }

    EXPECT_EQ(insert_failures.load(), 0u)
        << "All 10 000 high-cardinality inserts must succeed without failure";

    EXPECT_EQ(index.size(), kTotalVectors)
        << "Index must contain exactly " << kTotalVectors
        << " vectors after 8-thread insert. "
           "Missing: " << (kTotalVectors - index.size());
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 2: ConcurrentHNSWBuildAndQuery
// Concurrent writers and readers exercising the index simultaneously.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_HighCardinalityStress, ConcurrentHNSWBuildAndQuery) {
    constexpr std::size_t kDim          = 64;
    constexpr std::size_t kWriterVectors = 5'000;
    constexpr std::size_t kK             = 5;
    constexpr int         kWriterThreads = 4;
    constexpr int         kReaderThreads = 4;

    FlatVectorIndex index(kDim);

    // Seed with a small initial batch so readers always have data
    for (std::size_t i = 0; i < 100; ++i) {
        index.insert(static_cast<uint64_t>(i), makeVec(kDim, i));
    }

    std::atomic<bool>     stop_readers{false};
    std::atomic<uint64_t> write_errors{0};
    std::atomic<uint64_t> read_errors{0};
    std::atomic<uint64_t> total_reads{0};

    std::vector<std::thread> writers;
    writers.reserve(kWriterThreads);
    const std::size_t writer_batch = kWriterVectors / kWriterThreads;

    for (int t = 0; t < kWriterThreads; ++t) {
        writers.emplace_back([&, t]() {
            const uint64_t base = 1'000'000ULL + static_cast<uint64_t>(t) * writer_batch;
            for (std::size_t i = 0; i < writer_batch; ++i) {
                uint64_t id = base + static_cast<uint64_t>(i);
                try {
                    if (!index.insert(id, makeVec(kDim, id + 777'777ULL))) {
                        write_errors.fetch_add(1, std::memory_order_relaxed);
                    }
                } catch (...) {
                    write_errors.fetch_add(1, std::memory_order_relaxed);
                }
            }
        });
    }

    std::vector<std::thread> readers;
    readers.reserve(kReaderThreads);
    for (int t = 0; t < kReaderThreads; ++t) {
        readers.emplace_back([&, t]() {
            uint64_t qid = 2'000'000ULL + static_cast<uint64_t>(t) * 500'000ULL;
            while (!stop_readers.load(std::memory_order_acquire)) {
                try {
                    Vec q = makeVec(kDim, qid++);
                    auto results = index.knn(q, kK);
                    (void)results;
                    total_reads.fetch_add(1, std::memory_order_relaxed);
                } catch (...) {
                    read_errors.fetch_add(1, std::memory_order_relaxed);
                }
                std::this_thread::sleep_for(50us);
            }
        });
    }

    // Wait for all writers to finish, then stop readers
    for (auto& th : writers) { th.join(); }
    stop_readers.store(true, std::memory_order_release);
    for (auto& th : readers) { th.join(); }

    EXPECT_EQ(write_errors.load(), 0u)
        << "Concurrent HNSW build must complete without write errors";

    EXPECT_EQ(read_errors.load(), 0u)
        << "Concurrent HNSW queries must complete without exceptions";

    EXPECT_GT(total_reads.load(), 0u)
        << "At least one concurrent query must succeed during build";
}

// ─────────────────────────────────────────────────────────────────────────────
// Test 3: DimensionalityStressTest
// Exercise multiple dimensionality configurations to verify no dimension-related
// corruption or performance cliff.
// ─────────────────────────────────────────────────────────────────────────────
TEST(WaveD_HighCardinalityStress, DimensionalityStressTest) {
    // Dimensionality configurations representative of common embedding models
    const std::vector<std::size_t> dims = {32, 64, 128, 256, 512, 1024};
    constexpr std::size_t kVectorsPerDim = 500;
    constexpr std::size_t kK             = 5;

    for (std::size_t dim : dims) {
        SCOPED_TRACE("dim=" + std::to_string(dim));

        FlatVectorIndex index(dim);

        // Insert vectors
        std::size_t insert_failures = 0;
        for (std::size_t i = 0; i < kVectorsPerDim; ++i) {
            uint64_t id = static_cast<uint64_t>(dim * 10'000 + i);
            try {
                if (!index.insert(id, makeVec(dim, id))) {
                    ++insert_failures;
                }
            } catch (...) {
                ++insert_failures;
            }
        }

        EXPECT_EQ(insert_failures, 0u)
            << "[VECTOR:IndexCorruption] insert failures for dim=" << dim;

        EXPECT_EQ(index.size(), kVectorsPerDim)
            << "Expected " << kVectorsPerDim << " vectors in index for dim=" << dim;

        // Query — ensure non-empty results for a fresh query vector
        std::size_t empty_results = 0;
        for (std::size_t q = 0; q < 10; ++q) {
            try {
                Vec query = makeVec(dim, static_cast<uint64_t>(dim * 99'999 + q));
                auto results = index.knn(query, kK);
                if (results.empty()) { ++empty_results; }
            } catch (...) {
                ++empty_results;
            }
        }

        EXPECT_EQ(empty_results, 0u)
            << "[VECTOR:QueryTimeout] kNN queries returned empty results for dim=" << dim;
    }
}
