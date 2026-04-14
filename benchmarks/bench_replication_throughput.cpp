/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            bench_replication_throughput.cpp                   ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-04-14 06:48:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     213                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3c11df78a1  2026-03-03  feat(benchmarks): add 6 missing benchmark suites for acce... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/// @file bench_replication_throughput.cpp
/// @brief Performance benchmarks for the Replication module.
///
/// Covers the following process lines:
///   - WALManager::append()             – WAL write throughput
///   - WALManager::readFrom()           – sequential read throughput
///   - WALEntry::serialize()            – binary serialisation cost
///   - WALEntry::deserialize()          – binary deserialisation cost
///   - ReplicationManager::initialize() – start-up cost
///
/// Performance targets (src/replication/ROADMAP.md):
///   - WAL append:        > 50 000 entries/s
///   - WAL readFrom 1000: < 5 ms
///   - WALEntry serialize/deserialize: < 2 µs each

#include <benchmark/benchmark.h>
#include "replication/replication_manager.h"
#include <filesystem>
#include <string>
#include <chrono>
#include <atomic>

using namespace themisdb::replication;

// ============================================================================
// Helpers
// ============================================================================

namespace {

static ReplicationConfig makeConfig(const std::string& wal_dir) {
    ReplicationConfig cfg;
    cfg.enabled                      = true;
    cfg.mode                         = ReplicationMode::ASYNC;
    cfg.heartbeat_interval_ms        = 100;
    cfg.election_timeout_min_ms      = 150;
    cfg.election_timeout_max_ms      = 300;
    cfg.batch_size                   = 500;
    cfg.batch_timeout_ms             = 50;
    cfg.wal_directory                = wal_dir;
    cfg.failure_detection_timeout_ms = 2000;
    cfg.min_sync_replicas            = 0;
    cfg.wal_sync_on_commit           = false;
    return cfg;
}

static WALEntry makeEntry(uint64_t seq, const std::string& op,
                          const std::string& collection) {
    WALEntry e;
    e.sequence_number = seq;
    e.term            = 1;
    e.timestamp       = std::chrono::system_clock::now();
    e.operation       = op;
    e.collection      = collection;
    e.document_id     = "doc_" + std::to_string(seq);
    e.data            = R"({"field":"value","seq":)" + std::to_string(seq) + "}";
    return e;
}

} // anonymous namespace

// ============================================================================
// WALManager::append throughput
// ============================================================================

class WalBenchFixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& /*state*/) override {
        wal_dir_ = "./data/bench_replication_wal_tmp";
        if (std::filesystem::exists(wal_dir_)) {
            std::filesystem::remove_all(wal_dir_);
        }
        std::filesystem::create_directories(wal_dir_);
        cfg_ = makeConfig(wal_dir_);
        wal_ = std::make_unique<WALManager>(cfg_);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        wal_.reset();
        if (std::filesystem::exists(wal_dir_)) {
            std::filesystem::remove_all(wal_dir_);
        }
    }

protected:
    std::string                  wal_dir_;
    ReplicationConfig            cfg_;
    std::unique_ptr<WALManager>  wal_;
};

BENCHMARK_DEFINE_F(WalBenchFixture, Append)(benchmark::State& state) {
    std::atomic<uint64_t> seq{1};
    for (auto _ : state) {
        uint64_t s = seq.fetch_add(1, std::memory_order_relaxed);
        auto entry = makeEntry(s, "INSERT", "bench_col");
        uint64_t written_seq = wal_->append(entry);
        benchmark::DoNotOptimize(written_seq);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK_REGISTER_F(WalBenchFixture, Append)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(10000);

// ============================================================================
// WALManager::readFrom throughput (sequential scan)
// ============================================================================

BENCHMARK_DEFINE_F(WalBenchFixture, ReadFrom)(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));

    // Pre-populate WAL
    for (int i = 0; i < n; ++i) {
        wal_->append(makeEntry(static_cast<uint64_t>(i + 1), "INSERT", "c"));
    }

    for (auto _ : state) {
        auto entries = wal_->readFrom(1, static_cast<uint32_t>(n));
        benchmark::DoNotOptimize(entries);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("n=" + std::to_string(n));
}

BENCHMARK_REGISTER_F(WalBenchFixture, ReadFrom)
    ->Arg(100)
    ->Arg(500)
    ->Arg(1000)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(200);

// ============================================================================
// WALEntry::serialize / deserialize
// ============================================================================

static void BM_WALEntry_Serialize(benchmark::State& state) {
    auto entry = makeEntry(42, "UPDATE", "orders");
    for (auto _ : state) {
        auto bytes = entry.serialize();
        benchmark::DoNotOptimize(bytes);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_WALEntry_Serialize)->Unit(benchmark::kNanosecond);

static void BM_WALEntry_Deserialize(benchmark::State& state) {
    auto entry = makeEntry(42, "UPDATE", "orders");
    auto bytes = entry.serialize();

    for (auto _ : state) {
        auto result = WALEntry::deserialize(bytes);
        benchmark::DoNotOptimize(result);
    }
    state.SetItemsProcessed(state.iterations());
}

BENCHMARK(BM_WALEntry_Deserialize)->Unit(benchmark::kNanosecond);

// ============================================================================
// ReplicationManager::initialize cost
// ============================================================================

static void BM_ReplicationManager_Initialize(benchmark::State& state) {
    const std::string wal_dir = "./data/bench_repl_mgr_init_tmp";

    for (auto _ : state) {
        state.PauseTiming();
        if (std::filesystem::exists(wal_dir)) {
            std::filesystem::remove_all(wal_dir);
        }
        std::filesystem::create_directories(wal_dir);
        state.ResumeTiming();

        ReplicationManager mgr(makeConfig(wal_dir));
        bool ok = mgr.initialize();
        benchmark::DoNotOptimize(ok);
    }

    if (std::filesystem::exists(wal_dir)) {
        std::filesystem::remove_all(wal_dir);
    }
}

BENCHMARK(BM_ReplicationManager_Initialize)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(50);

BENCHMARK_MAIN();
