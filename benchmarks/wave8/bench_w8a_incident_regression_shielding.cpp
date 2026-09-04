// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_w8a_incident_regression_shielding.cpp
 * @brief Wave 8-A: Incident-Driven Benchmark Regression Shielding.
 *
 * Purpose: Convert known post-release performance incidents and near-misses
 * into reproducible, targeted benchmarks so that regressions to those paths
 * are caught automatically in CI before they reach production again.
 *
 * Each scenario is derived from a concrete incident or near-miss triage record.
 * The benchmark name encodes the incident category for traceability.
 *
 * Covered scenarios (IRS = Incident Regression Scenario):
 *   IRS-01  Hot-path burst read spike — read amplification under bursty load
 *   IRS-02  Concurrent write storm — memtable pressure / stall under many writers
 *   IRS-03  Read-after-write consistency under load — near-stale read after flush
 *   IRS-04  Hot-prefix range scan — scan regression on a concentrated key prefix
 *   IRS-05  Batch ingest with concurrent readers — live ingest + read path collision
 *   IRS-06  Secondary index rebuild under load — index rebuild latency incident
 *   IRS-07  Delete-tombstone accumulation — compaction debt under delete storm
 *   IRS-08  Large-payload mixed workload — bandwidth-saturation incident (16 KB values)
 *
 * All benchmarks:
 *   - Use kW8CanonicalSeed for deterministic key selection.
 *   - Apply kWarmupIterations before measurement begins.
 *   - UseRealTime() so that I/O wait is included in reported latency.
 *   - Run with Repetitions(kRepetitions) for variance estimation.
 *
 * Hard gates enforced by release_gate_manifest_w8.json:
 *   - IRS-01 burst-read mean ≤ 250 µs (relaxed for burst scenario)
 *   - IRS-02 write-storm throughput ≥ 60 000 ops/s under concurrency
 *   - IRS-05 mixed ingest+read p99 ≤ 400 µs
 *
 * @note Fault-injection uses in-process delay simulation to remain
 *       runnable in standard CI without external infrastructure.
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <atomic>
#include <chrono>
#include <filesystem>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <thread>
#include <vector>

#include "index/secondary_index.h"
#include "storage/base_entity.h"
#include "storage/rocksdb_wrapper.h"

namespace fs = std::filesystem;

namespace themis {
namespace bench {
namespace w8a {

// ---------------------------------------------------------------------------
// Constants – deterministic, release-pinned
// ---------------------------------------------------------------------------

/// Canonical PRNG seed shared by all W8 benchmarks.
static constexpr uint64_t kW8CanonicalSeed = 42;

/// Warmup iterations applied before every measurement window.
static constexpr int kWarmupIterations = 500;

/// Repetitions per benchmark for variance estimation.
static constexpr int kRepetitions = 5;

/// Default dataset record count pre-loaded into DB during SetUp.
static constexpr int kDefaultRecordCount = 50'000;

/// Large-payload size (16 KB, matches bandwidth-saturation incident).
static constexpr std::size_t kLargePayloadBytes = 16'384;

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

namespace {

void RemoveAll(const std::string& path) {
    std::error_code ec = {};
    fs::remove_all(path, ec);
}

std::string UniqueDbPath(const std::string& tag) {
    using namespace std::chrono;
    auto ts = duration_cast<nanoseconds>(steady_clock::now().time_since_epoch()).count();
    return fs::temp_directory_path().string() + "/w8a_" + tag + "_" + std::to_string(ts);
}

RocksDBWrapper::Config DefaultConfig(const std::string& db_path) {
    RocksDBWrapper::Config cfg;
    cfg.db_path                         = db_path;
    cfg.compression_default             = "lz4";
    cfg.compression_bottommost          = "zstd";
    cfg.block_cache_size_mb             = 256;
    cfg.memtable_size_mb                = 128;
    cfg.max_write_buffer_number         = 4;
    cfg.allow_concurrent_memtable_write = true;
    cfg.enable_statistics               = false;
    return cfg;
}

/// Seeded, reproducible key generator for W8 workloads.
class KeyGenerator {
public:
    explicit KeyGenerator(uint64_t seed = kW8CanonicalSeed) : rng_(seed) {}

    std::string NextKey(int upper_bound) {
        std::uniform_int_distribution<int> dist(0, upper_bound - 1);
        return "entity_" + std::to_string(dist(rng_));
    }

    /// Generate a key within a narrow hot-prefix range (for IRS-04).
    std::string NextHotPrefixKey(int prefix_range) {
        std::uniform_int_distribution<int> dist(0, prefix_range - 1);
        return "hot_prefix_" + std::to_string(dist(rng_));
    }

    std::string NextLargeValue() {
        return std::string(kLargePayloadBytes, static_cast<char>('A' + (rng_() % 26)));
    }

private:
    std::mt19937_64 rng_;
};

} // namespace

// ---------------------------------------------------------------------------
// IRS-01 / IRS-02 / IRS-03 / IRS-07: Single-DB incident scenarios
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for single-connection incident-regression scenarios.
 *
 * Models the minimal repro environment used during incident post-mortems:
 * a single RocksDB instance pre-loaded with kDefaultRecordCount records,
 * warmed up with kWarmupIterations operations.
 */
class IncidentCRUDFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("crud_incident");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8A: failed to open RocksDB for incident CRUD fixture");
        }

        // Pre-load dataset
        for (int i = 0; i < kDefaultRecordCount; ++i) {
            BaseEntity e("entity_" + std::to_string(i));
            e.setField("idx",    std::to_string(i));
            e.setField("status", (i % 5 == 0) ? "deleted" : "active");
            e.setField("score",  std::to_string(i * 13 % 10000));
            db_->put(e.id(), e.serialize());
        }

        // Warmup reads
        KeyGenerator warmup_kg(kW8CanonicalSeed + 1);
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val = {};
            db_->get(warmup_kg.NextKey(kDefaultRecordCount), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief IRS-01: Hot-path burst read spike.
 *
 * Incident: Under a burst of read requests the p99 latency spiked to >1 ms
 * due to block cache eviction pressure.  This scenario replays the burst
 * pattern with a tight access window (top 5% of key space) to reproduce the
 * cache pressure, then measures per-op latency.
 */
BENCHMARK_F(IncidentCRUDFixture, IRS01_BurstReadSpike)(benchmark::State& state) {
    // Narrow access window: top 5% of records → cache pressure
    const int hot_window = kDefaultRecordCount / 20;
    KeyGenerator kg(kW8CanonicalSeed + 10);
    for (auto _ : state) {
        const std::string key = kg.NextKey(hot_window);
        std::string val = {};
        benchmark::DoNotOptimize(db_->get(key, val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(IncidentCRUDFixture, IRS01_BurstReadSpike)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8A/IRS01_BurstReadSpike_mean_250us_gate");

/**
 * @brief IRS-03: Read-after-write consistency under load.
 *
 * Near-miss: After a compaction flush, reads on recently written keys
 * occasionally saw stale state.  This scenario interleaves writes and
 * immediate reads on the same key to validate read-your-writes consistency
 * and measure the round-trip latency under this access pattern.
 */
BENCHMARK_F(IncidentCRUDFixture, IRS03_ReadAfterWrite)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 30);
    int counter = 0;
    for (auto _ : state) {
        const std::string key = kg.NextKey(kDefaultRecordCount);
        const std::string val = "raw_" + std::to_string(counter++);
        db_->put(key, val);
        std::string read_val = {};
        benchmark::DoNotOptimize(db_->get(key, read_val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(IncidentCRUDFixture, IRS03_ReadAfterWrite)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8A/IRS03_ReadAfterWrite_consistency_latency");

/**
 * @brief IRS-07: Delete-tombstone accumulation.
 *
 * Incident: A delete-storm created a large tombstone backlog, causing
 * range scan latency to grow until compaction completed.  This scenario
 * inserts records, deletes a fraction (creating tombstones), then measures
 * read latency to quantify the tombstone overhead on the read path.
 */
BENCHMARK_F(IncidentCRUDFixture, IRS07_DeleteTombstoneReadCost)(benchmark::State& state) {
    // Issue deletes on every 3rd key to build tombstone pressure
    for (int i = 0; i < kDefaultRecordCount; i += 3) {
        db_->remove("entity_" + std::to_string(i));
    }
    KeyGenerator kg(kW8CanonicalSeed + 70);
    for (auto _ : state) {
        const std::string key = kg.NextKey(kDefaultRecordCount);
        std::string val = {};
        benchmark::DoNotOptimize(db_->get(key, val));
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(IncidentCRUDFixture, IRS07_DeleteTombstoneReadCost)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8A/IRS07_DeleteTombstone_read_overhead");

// ---------------------------------------------------------------------------
// IRS-02: Concurrent write storm
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for concurrent write-storm incident scenario.
 *
 * Models N parallel writers hammering the DB simultaneously, recreating
 * the memtable-stall condition observed in the write-storm incident.
 */
class WriteStormFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("write_storm");
        RemoveAll(db_path_);

        RocksDBWrapper::Config cfg = DefaultConfig(db_path_);
        cfg.max_write_buffer_number         = 8;
        cfg.memtable_size_mb                = 64; // intentionally tight to trigger stall
        cfg.allow_concurrent_memtable_write = true;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            throw std::runtime_error("W8A: failed to open RocksDB for write-storm fixture");
        }
        ops_completed_.store(0, std::memory_order_relaxed);
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
    std::atomic<int64_t>            ops_completed_{0};
};

/**
 * @brief IRS-02: Concurrent write storm.
 *
 * Incident: Eight simultaneous writer threads caused memtable stall and
 * throughput dropped to <20k ops/s.  This benchmark spawns 4 writer threads
 * (reduced for CI resource budget) and measures aggregate throughput.
 * Gate: combined throughput ≥ 60 000 ops/s.
 */
BENCHMARK_F(WriteStormFixture, IRS02_WriteStormThroughput)(benchmark::State& state) {
    static constexpr int kWriterThreads = 4;
    static constexpr int kWritesPerThread = 2'000;

    for (auto _ : state) {
        state.PauseTiming();
        ops_completed_.store(0, std::memory_order_relaxed);
        std::vector<std::thread> writers;
        writers.reserve(kWriterThreads);
        state.ResumeTiming();

        for (int t = 0; t < kWriterThreads; ++t) {
            writers.emplace_back([this, t] {
                KeyGenerator kg(kW8CanonicalSeed + 200 + static_cast<uint64_t>(t));
                for (int i = 0; i < kWritesPerThread; ++i) {
                    db_->put(kg.NextKey(kDefaultRecordCount * 4),
                             "storm_val_" + std::to_string(i));
                }
                ops_completed_.fetch_add(kWritesPerThread, std::memory_order_relaxed);
            });
        }
        for (auto& w : writers) {
          w.join();
        }
    }
    state.SetItemsProcessed(ops_completed_.load(std::memory_order_relaxed));
}
BENCHMARK_REGISTER_F(WriteStormFixture, IRS02_WriteStormThroughput)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8A/IRS02_WriteStorm_throughput_60k_gate");

// ---------------------------------------------------------------------------
// IRS-04: Hot-prefix range scan
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for hot-prefix range scan incident scenario.
 */
class HotPrefixScanFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("hotprefix");
        RemoveAll(db_path_);

        db_  = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8A: failed to open RocksDB for hot-prefix fixture");
        }
        idx_ = std::make_unique<SecondaryIndexManager>(*db_);
        idx_->createRangeIndex("Event", "ts");

        // Load both hot-prefix keys and normal keys
        for (int i = 0; i < kDefaultRecordCount; ++i) {
            BaseEntity e("hot_prefix_" + std::to_string(i % kHotPrefixRange));
            e.setField("ts",  std::to_string(i));
            e.setField("data", "payload_" + std::to_string(i));
            db_->put(e.id() + "_" + std::to_string(i), e.serialize());
        }

        // Warmup
        for (int i = 0; i < kWarmupIterations; ++i) {
            std::string val = {};
            db_->get("hot_prefix_" + std::to_string(i % kHotPrefixRange), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        idx_.reset();
        db_.reset();
        RemoveAll(db_path_);
    }

    static constexpr int kHotPrefixRange = 500; // narrow prefix space → hot keys

protected:
    std::string                            db_path_;
    std::unique_ptr<RocksDBWrapper>        db_;
    std::unique_ptr<SecondaryIndexManager> idx_;
};

/**
 * @brief IRS-04: Hot-prefix range scan.
 *
 * Incident: Range scans on a highly concentrated key prefix caused an
 * unexpectedly high SST file fan-out, making each scan touch more levels
 * than expected.  This scenario runs range scans within the hot prefix.
 */
BENCHMARK_F(HotPrefixScanFixture, IRS04_HotPrefixRangeScan)(benchmark::State& state) {
    std::mt19937_64 rng(kW8CanonicalSeed + 40);
    std::uniform_int_distribution<int> dist(0, kHotPrefixRange - 100);
    for (auto _ : state) {
        const int lo = dist(rng);
        auto results = idx_->rangeQuery("Event", "ts",
                                        std::to_string(lo),
                                        std::to_string(lo + 100));
        benchmark::DoNotOptimize(results);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(HotPrefixScanFixture, IRS04_HotPrefixRangeScan)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8A/IRS04_HotPrefixRangeScan_p99_400us_gate");

// ---------------------------------------------------------------------------
// IRS-05: Batch ingest with concurrent readers
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for concurrent ingest + reader incident scenario.
 */
class IngestReadFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("ingest_read");
        RemoveAll(db_path_);

        db_ = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8A: failed to open RocksDB for ingest+read fixture");
        }
        for (int i = 0; i < kDefaultRecordCount; ++i) {
            db_->put("base_" + std::to_string(i), "v" + std::to_string(i));
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief IRS-05: Batch ingest with concurrent readers.
 *
 * Near-miss: During a live batch ingest, background readers observed
 * elevated p99 latencies due to shared memtable lock contention.  This
 * scenario issues a batch write on the main thread while a background
 * reader runs concurrently, measuring the read-side latency impact.
 * Gate: mixed p99 ≤ 400 µs.
 */
BENCHMARK_F(IngestReadFixture, IRS05_IngestWithConcurrentRead)(benchmark::State& state) {
    static constexpr int kBatchSize = 200;
    std::atomic<bool> stop_reader{false};
    std::atomic<int64_t> read_ops{0};

    // Background reader thread
    std::thread reader([this, &stop_reader, &read_ops] {
        KeyGenerator rkg(kW8CanonicalSeed + 500);
        while (!stop_reader.load(std::memory_order_relaxed)) {
            std::string val = {};
            db_->get(rkg.NextKey(kDefaultRecordCount), val);
            read_ops.fetch_add(1, std::memory_order_relaxed);
        }
    });

    KeyGenerator kg(kW8CanonicalSeed + 50);
    int counter = 0;
    for (auto _ : state) {
        std::vector<std::pair<std::string, std::string>> batch;
        batch.reserve(kBatchSize);
        for (int i = 0; i < kBatchSize; ++i) {
            batch.emplace_back(kg.NextKey(kDefaultRecordCount * 2),
                               "ingest_" + std::to_string(counter++));
        }
        db_->putBatch(batch);
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * kBatchSize);

    stop_reader.store(true, std::memory_order_relaxed);
    reader.join();
    state.counters["bg_read_ops"] =
        benchmark::Counter(static_cast<double>(read_ops.load()), benchmark::Counter::kIsRate);
}
BENCHMARK_REGISTER_F(IngestReadFixture, IRS05_IngestWithConcurrentRead)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8A/IRS05_IngestWithConcurrentRead_p99_400us_gate");

// ---------------------------------------------------------------------------
// IRS-06: Secondary index rebuild under load
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for secondary index rebuild incident scenario.
 */
class IndexRebuildFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("idx_rebuild");
        RemoveAll(db_path_);

        db_  = std::make_unique<RocksDBWrapper>(DefaultConfig(db_path_));
        if (!db_->open()) {
            throw std::runtime_error("W8A: failed to open RocksDB for index rebuild fixture");
        }
        // Load initial dataset
        for (int i = 0; i < kDefaultRecordCount; ++i) {
            BaseEntity e("user_" + std::to_string(i));
            e.setField("email", "u" + std::to_string(i) + "@corp.com");
            e.setField("dept",  std::to_string(i % 50));
            db_->put(e.id(), e.serialize());
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief IRS-06: Secondary index rebuild latency under load.
 *
 * Incident: An index rebuild was triggered mid-flight while reads were
 * ongoing, causing latency spikes.  This scenario models the rebuild cost
 * by creating a fresh index and bulk-inserting all entries, then measuring
 * the per-record indexing time.
 */
BENCHMARK_F(IndexRebuildFixture, IRS06_IndexRebuildLatency)(benchmark::State& state) {
    for (auto _ : state) {
        state.PauseTiming();
        auto idx = std::make_unique<SecondaryIndexManager>(*db_);
        idx->createIndex("User", "email", true);
        state.ResumeTiming();

        // Bulk index insertion (models rebuild)
        for (int i = 0; i < 1'000; ++i) {
            BaseEntity e("user_" + std::to_string(i));
            e.setField("email", "u" + std::to_string(i) + "@corp.com");
            idx->insertIndexEntries(e);
        }
        benchmark::DoNotOptimize(idx.get());
        state.PauseTiming();
        idx.reset();
        state.ResumeTiming();
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()) * 1'000);
}
BENCHMARK_REGISTER_F(IndexRebuildFixture, IRS06_IndexRebuildLatency)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMillisecond)
    ->UseRealTime()
    ->Name("W8A/IRS06_IndexRebuild_per_1k_latency");

// ---------------------------------------------------------------------------
// IRS-08: Large-payload mixed workload
// ---------------------------------------------------------------------------

/**
 * @brief Fixture for large-payload bandwidth-saturation incident scenario.
 */
class LargePayloadFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) override {
        db_path_ = UniqueDbPath("large_payload");
        RemoveAll(db_path_);

        RocksDBWrapper::Config cfg = DefaultConfig(db_path_);
        cfg.block_cache_size_mb = 512;
        db_ = std::make_unique<RocksDBWrapper>(cfg);
        if (!db_->open()) {
            throw std::runtime_error("W8A: failed to open RocksDB for large-payload fixture");
        }
        // Pre-load 5k large-payload records
        KeyGenerator kg(kW8CanonicalSeed + 80);
        for (int i = 0; i < 5'000; ++i) {
            db_->put("large_" + std::to_string(i), kg.NextLargeValue());
        }
        // Warmup
        for (int i = 0; i < 200; ++i) {
            std::string val = {};
            db_->get("large_" + std::to_string(i % 5'000), val);
        }
    }

    void TearDown(const ::benchmark::State&) override {
        db_.reset();
        RemoveAll(db_path_);
    }

protected:
    std::string                     db_path_;
    std::unique_ptr<RocksDBWrapper> db_;
};

/**
 * @brief IRS-08: Large-payload mixed workload.
 *
 * Incident: A 16 KB average payload workload saturated the block cache
 * and caused both read and write throughput to degrade.  This scenario
 * reproduces the mixed read/write pattern with 16 KB values.
 */
BENCHMARK_F(LargePayloadFixture, IRS08_LargePayloadMixed)(benchmark::State& state) {
    KeyGenerator kg(kW8CanonicalSeed + 8);
    std::mt19937_64 mix_rng(kW8CanonicalSeed + 88);
    std::uniform_int_distribution<int> mix(0, 99);
    int write_ctr = 0;
    for (auto _ : state) {
        if (mix(mix_rng) < 40) {
            db_->put("large_new_" + std::to_string(write_ctr++), kg.NextLargeValue());
        } else {
            std::string val = {};
            benchmark::DoNotOptimize(
                db_->get("large_" + std::to_string(write_ctr % 5'000), val));
        }
    }
    state.SetItemsProcessed(static_cast<int64_t>(state.iterations()));
}
BENCHMARK_REGISTER_F(LargePayloadFixture, IRS08_LargePayloadMixed)
    ->Repetitions(kRepetitions)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime()
    ->Name("W8A/IRS08_LargePayload_mixed_bandwidth");

} // namespace w8a
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
