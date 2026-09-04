// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w2b_concurrency.cpp
 * @brief Wave 2 / B2-B: Concurrency and contention performance benchmarks.
 *
 * Measures throughput, tail latency, and scalability under concurrent load
 * across the storage and index layers.  Each scenario isolates a specific
 * contention pattern:
 *
 *   Scenario 1  – Concurrent writes, disjoint key spaces (no hotspot)
 *   Scenario 2  – Concurrent writes, overlapping key domain (write contention)
 *   Scenario 3  – Concurrent mixed read/write (read-write contention)
 *   Scenario 4  – Concurrent vector-index inserts + kNN queries
 *   Scenario 5  – Reader-writer fan-out: N reader threads, 1 writer thread
 *
 * Methodology
 * -----------
 * - Each benchmark registers variants with --threads=1/2/4/8 so that
 *   Google Benchmark's thread-scaling infrastructure aggregates the counters.
 * - Setup populates a warm dataset outside the timed loop.
 * - `state.SetItemsProcessed` and `benchmark::Counter::kIsRate` expose
 *   throughput in items/s directly comparable across scenarios.
 *
 * Run (Release build):
 * @code
 *   ./bench_w2b_concurrency --benchmark_filter=W2B
 *   ./bench_w2b_concurrency --benchmark_format=json --benchmark_out=w2b.json
 * @endcode
 *
 * Interpretation
 * --------------
 * - `ops_per_sec`: per-thread operations per second (uses `kAvgThreads`).
 *   To derive the total aggregate, multiply by the thread count.
 * - Linear scaling in `ops_per_sec` vs. thread count indicates low contention.
 * - Sub-linear scaling indicates lock contention; compare disjoint vs.
 *   overlapping scenarios to isolate the hotspot.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <memory>
#include <mutex>
#include <random>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

#include "index/vector_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;
using namespace themis;

// ============================================================================
// Shared helpers
// ============================================================================

namespace {

std::string uniqueDbPath(const char* tag) {
    auto ns = static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
    return std::string("./data/w2b_") + tag + "_" + std::to_string(ns);
}

std::unique_ptr<RocksDBWrapper> openBenchDb(const std::string& path) {
    fs::remove_all(path);
    fs::create_directories(path);

    RocksDBWrapper::Config cfg;
    cfg.db_path                   = path;
    cfg.enable_wal                = false;
    cfg.disable_wal_for_benchmark = true;
    cfg.memtable_size_mb          = 64;
    cfg.block_cache_size_mb       = 128;
    cfg.allow_concurrent_memtable_write = true;
    auto db = std::make_unique<RocksDBWrapper>(cfg);
    if (!db->open()) {
        throw std::runtime_error("W2B: failed to open RocksDB at " + path);
    }
    return db;
}

std::vector<float> randomVec(int dim, std::mt19937& rng) {
    std::uniform_real_distribution<float> dis(-1.0f, 1.0f);
    std::vector<float> v(static_cast<std::size_t>(dim));
    for (auto& x : v) {
      x = dis(rng);
    }
    float sq = 0.f;
    for (float x : v) {
      sq += x * x;
    }
    float inv = 1.f / std::sqrt(std::max(sq, 1e-12f));
    for (auto& x : v) {
      x *= inv;
    }
    return v;
}

} // namespace

// ============================================================================
// Base KV-Write Fixture
// ============================================================================

/**
 * @brief Shared fixture for Scenario 1 and 2.
 *
 * Opens a single RocksDB instance shared by all benchmark threads.
 * Pre-populates @p kWarmupKeys so the DB is in a realistic warm state.
 */
class W2B_KvWriteFixture : public benchmark::Fixture {
public:
    static constexpr int kWarmupKeys = 5'000;

    void SetUp(::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        dbPath_ = uniqueDbPath("kv");
        db_     = openBenchDb(dbPath_);
        // Pre-populate warm data
        for (int i = 0; i < kWarmupKeys; ++i) {
            db_->put("warm_" + std::to_string(i), std::string(64, 'w'));
        }
        nextId_.store(kWarmupKeys, std::memory_order_relaxed);
    }

    void TearDown(::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        db_->close();
        db_.reset();
        fs::remove_all(dbPath_);
    }

protected:
    std::string                     dbPath_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::atomic<int>                nextId_{0};
};

// ============================================================================
// Scenario 1 – Concurrent writes, disjoint key spaces
// ============================================================================

/**
 * @brief B2-B Scenario 1: Each thread writes to its own disjoint key space.
 *
 * Baseline for write throughput with zero key-level contention.
 * Compare with Scenario 2 to quantify hotspot overhead.
 */
BENCHMARK_DEFINE_F(W2B_KvWriteFixture, ConcurrentWrites_Disjoint)(benchmark::State& state) {
    const int threadIdx = state.thread_index();

    for (auto _ : state) {
        int id = nextId_.fetch_add(1, std::memory_order_relaxed);
        // Key space per thread: threadIdx * 1e6 + sequential id
        std::string key = "t" + std::to_string(threadIdx) +
                          "_k" + std::to_string(id);
        bool ok = db_->put(key, std::string(64, 'v'));
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kAvgThreads);
}

BENCHMARK_REGISTER_F(W2B_KvWriteFixture, ConcurrentWrites_Disjoint)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Scenario 2 – Concurrent writes, overlapping (hot) key domain
// ============================================================================

/**
 * @brief B2-B Scenario 2: All threads write to a small shared key domain.
 *
 * Forces maximum write contention.  The fractional contention overhead is
 * `1 − (overlapping_throughput / disjoint_throughput)`.
 */
BENCHMARK_DEFINE_F(W2B_KvWriteFixture, ConcurrentWrites_Overlapping)(benchmark::State& state) {
    std::mt19937 rng(static_cast<uint32_t>(state.thread_index() + 1));
    std::uniform_int_distribution<int> keyDist(0, 31); // very small domain = high contention

    for (auto _ : state) {
        std::string key = "hot_" + std::to_string(keyDist(rng));
        bool ok = db_->put(key, std::string(64, 'x'));
        benchmark::DoNotOptimize(ok);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kAvgThreads);
}

BENCHMARK_REGISTER_F(W2B_KvWriteFixture, ConcurrentWrites_Overlapping)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Scenario 3 – Concurrent mixed read/write (read-write contention)
// ============================================================================

/**
 * @brief B2-B Scenario 3: Half the threads read, half write (read-write contention).
 *
 * Each thread determines its role by `thread_index() % 2`.  Writers do
 * sequential puts; readers do random gets.  Measures how reads degrade under
 * concurrent writes and vice versa.
 */
BENCHMARK_DEFINE_F(W2B_KvWriteFixture, ConcurrentMixed_ReadWrite)(benchmark::State& state) {
    const bool isWriter = (state.thread_index() % 2 == 0);
    std::mt19937 rng(static_cast<uint32_t>(state.thread_index() + 100));
    std::uniform_int_distribution<int> readKeyDist(0, kWarmupKeys - 1);

    for (auto _ : state) {
        if (isWriter) {
            int id = nextId_.fetch_add(1, std::memory_order_relaxed);
            bool ok = db_->put("rw_" + std::to_string(id), std::string(64, 'r'));
            benchmark::DoNotOptimize(ok);
        } else {
            std::string key = "warm_" + std::to_string(readKeyDist(rng));
            std::string out;
            bool found = db_->get(key, out);
            benchmark::DoNotOptimize(found);
            benchmark::DoNotOptimize(out);
        }
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kAvgThreads);
}

BENCHMARK_REGISTER_F(W2B_KvWriteFixture, ConcurrentMixed_ReadWrite)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Scenario 4 – Concurrent vector-index inserts + kNN queries
// ============================================================================

/**
 * @brief Fixture for vector-index concurrency scenarios.
 *
 * Initialises a shared VectorIndexManager pre-seeded with @p kWarmupVectors.
 */
class W2B_VectorConcurrencyFixture : public benchmark::Fixture {
public:
    static constexpr int kDim           = 128;
    static constexpr int kWarmupVectors = 2'000;
    static constexpr int kTopK          = 10;

    void SetUp(::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        dbPath_ = uniqueDbPath("vec");
        db_     = openBenchDb(dbPath_);
        vix_    = std::make_unique<VectorIndexManager>(*db_);

        auto st = vix_->init("w2b_vec", kDim, VectorIndexManager::Metric::COSINE,
                             /*M=*/16, /*efConstruction=*/100, /*ef=*/64);
        if (!st.ok) {
            throw std::runtime_error("W2B VecFixture init: " + st.message);
        }

        std::mt19937 rng(33);
        for (int i = 0; i < kWarmupVectors; ++i) {
            BaseEntity e("w_" + std::to_string(i));
            e.setField("embedding", Value{randomVec(kDim, rng)});
            vix_->addEntity(e);
        }
        nextId_.store(kWarmupVectors, std::memory_order_relaxed);
    }

    void TearDown(::benchmark::State& state) override {
        if (state.thread_index() != 0) {
          return;
        }
        vix_.reset();
        db_->close();
        db_.reset();
        fs::remove_all(dbPath_);
    }

protected:
    std::string                         dbPath_;
    std::unique_ptr<RocksDBWrapper>     db_;
    std::unique_ptr<VectorIndexManager> vix_;
    std::atomic<int>                    nextId_{0};
};

/**
 * @brief B2-B Scenario 4: Concurrent vector inserts with interleaved kNN queries.
 *
 * Each thread alternates insert and query.  Measures HNSW throughput under
 * multi-threaded index mutation and search concurrency.
 */
BENCHMARK_DEFINE_F(W2B_VectorConcurrencyFixture, ConcurrentInsertQuery)(benchmark::State& state) {
    std::mt19937 rng(static_cast<uint32_t>(state.thread_index() + 200));

    for (auto _ : state) {
        int id = nextId_.fetch_add(1, std::memory_order_relaxed);
        auto vec = randomVec(kDim, rng);

        // Insert
        BaseEntity e("c_" + std::to_string(id));
        e.setField("embedding", Value{vec});
        benchmark::DoNotOptimize(vix_->addEntity(e).ok);

        // Query
        auto query = randomVec(kDim, rng);
        auto [qst, results] = vix_->searchKnn(query, kTopK);
        benchmark::DoNotOptimize(qst.ok);
        benchmark::DoNotOptimize(results);
    }

    state.SetItemsProcessed(state.iterations());
    state.counters["ops_per_sec"] = benchmark::Counter(
        static_cast<double>(state.iterations()),
        benchmark::Counter::kIsRate | benchmark::Counter::kAvgThreads);
}

BENCHMARK_REGISTER_F(W2B_VectorConcurrencyFixture, ConcurrentInsertQuery)
    ->Threads(1)
    ->Threads(2)
    ->Threads(4)
    ->Threads(8)
    ->Unit(benchmark::kMicrosecond);

// ============================================================================
// Scenario 5 – Reader-writer fan-out: N reader threads, 1 writer thread
// ============================================================================

/**
 * @brief B2-B Scenario 5: Many readers vs. single writer.
 *
 * Implemented as a free function benchmark using manual thread control so that
 * the 1:N reader/writer ratio can be explicitly set independently of
 * Google Benchmark's `--benchmark_min_threads`.
 *
 * @param state range(0) = number of dedicated reader threads (1/2/4/8)
 */
static void BM_W2B_ReaderWriterFanout(benchmark::State& state) {
    const int numReaders = static_cast<int>(state.range(0));
    const int kWarmup    = 2'000;
    const int kTopK      = 10;
    const int kDim       = 128;

    const std::string dbPath = uniqueDbPath("rwfanout");
    auto db = openBenchDb(dbPath);

    // Pre-populate warm data
    for (int i = 0; i < kWarmup; ++i) {
        db->put("warm_" + std::to_string(i), std::string(64, 'w'));
    }

    std::atomic<int> nextWriteId{kWarmup};
    std::atomic<bool> stop{false};
    std::atomic<int64_t> readOps{0};
    std::atomic<int64_t> writeOps{0};

    // Launch dedicated reader threads
    std::vector<std::thread> readers;
    readers.reserve(static_cast<std::size_t>(numReaders));
    std::mt19937 seedRng(11);
    for (int r = 0; r < numReaders; ++r) {
        uint32_t seed = seedRng();
        readers.emplace_back([&, seed]() {
            std::mt19937 rng(seed);
            std::uniform_int_distribution<int> keyDist(0, kWarmup - 1);
            while (!stop.load(std::memory_order_acquire)) {
                std::string key = "warm_" + std::to_string(keyDist(rng));
                std::string out;
                bool found = db->get(key, out);
                benchmark::DoNotOptimize(found);
                readOps.fetch_add(1, std::memory_order_relaxed);
            }
        });
    }

    // Main thread acts as the single writer inside the timed loop
    for (auto _ : state) {
        int id = nextWriteId.fetch_add(1, std::memory_order_relaxed);
        bool ok = db->put("wr_" + std::to_string(id), std::string(64, 'v'));
        benchmark::DoNotOptimize(ok);
        writeOps.fetch_add(1, std::memory_order_relaxed);
    }

    stop.store(true, std::memory_order_release);
    for (auto& t : readers) {
      t.join();
    }

    state.SetItemsProcessed(writeOps.load());
    state.counters["write_ops_per_sec"] = benchmark::Counter(
        static_cast<double>(writeOps.load()), benchmark::Counter::kIsRate);
    state.counters["read_ops"]          = static_cast<double>(readOps.load());
    state.counters["num_readers"]       = static_cast<double>(numReaders);

    db->close();
    db.reset();
    fs::remove_all(dbPath);
}

BENCHMARK(BM_W2B_ReaderWriterFanout)
    ->Arg(1)
    ->Arg(2)
    ->Arg(4)
    ->Arg(8)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
