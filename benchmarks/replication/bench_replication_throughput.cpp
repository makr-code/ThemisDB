/// @file bench_replication_throughput.cpp
/// @brief Performance benchmarks for the Replication module.
///
/// Covers the following process lines:
///   - WALManager::append()             – WAL write throughput
///   - WALManager::readFrom()           – sequential read throughput
///   - WALEntry::serialize()            – binary serialisation cost
///   - WALEntry::deserialize()          – binary deserialisation cost
///   - ReplicationManager::initialize() – start-up cost
///   - ReplicationManager::promoteToLeader() – leader promotion/failover path
///   - LastWriteWinsResolver::resolve() – HLC conflict detection/resolution
///   - CRDTMergeResolver::resolve()     – CRDT merge performance
///
/// Performance targets (src/replication/ROADMAP.md):
///   - WAL append:        > 50 000 entries/s
///   - WAL readFrom 1000: < 5 ms
///   - WALEntry serialize/deserialize: < 2 µs each

#include <benchmark/benchmark.h>
#include "replication/replication_manager.h"
#include "replication/multi_master_replication.h"
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
    cfg.enable_leader_lease          = false;
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

static MMWriteEntry makeMMEntry(uint64_t physical,
                                uint32_t logical,
                                const std::string& node_id,
                                const std::string& write_id) {
    MMWriteEntry e;
    e.write_id = write_id;
    e.origin_node = node_id;
    e.collection = "bench_col";
    e.document_id = "doc_hlc";
    e.operation = "UPDATE";
    e.data = R"({"field":"value"})";
    e.hlc = HybridLogicalClock::Timestamp{physical, logical, node_id};
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
    std::string                  wal_dir_ = {};
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
    uint64_t logical_bytes = 0;

    // Pre-populate WAL
    for (int i = 0; i < n; ++i) {
        auto entry = makeEntry(static_cast<uint64_t>(i + 1), "INSERT", "c");
        logical_bytes += static_cast<uint64_t>(entry.serialize().size());
        wal_->append(entry);
    }

    for (auto _ : state) {
        auto entries = wal_->readFrom(1, static_cast<uint32_t>(n));
        benchmark::DoNotOptimize(entries);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetBytesProcessed(static_cast<int64_t>(state.iterations()) * static_cast<int64_t>(logical_bytes));
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
// HLC conflict detection / resolution micro-benchmark (R-4 direct metric)
// ============================================================================

static void BM_HLCConflictDetection(benchmark::State& state) {
    const uint64_t skew_ms = static_cast<uint64_t>(state.range(0));
    const uint64_t base_physical = 1'000'000;

    LastWriteWinsResolver resolver;
    std::vector<MMWriteEntry> writes;
    writes.reserve(2);
    writes.push_back(makeMMEntry(base_physical, 1, "node-a", "w-a"));
    writes.push_back(makeMMEntry(base_physical + skew_ms, 2, "node-b", "w-b"));

    for (auto _ : state) {
        const auto winner = resolver.resolve("doc_hlc", writes);
        benchmark::DoNotOptimize(winner);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("skew_ms=" + std::to_string(skew_ms));
}

BENCHMARK(BM_HLCConflictDetection)
    ->Arg(0)
    ->Arg(50)
    ->Arg(200)
    ->Unit(benchmark::kNanosecond);

// ============================================================================
// CRDT merge micro-benchmark (R-5 direct metric)
// ============================================================================

static void BM_CRDTMerge(benchmark::State& state) {
    const int write_count = static_cast<int>(state.range(0));
    const uint64_t base_physical = 2'000'000;

    CRDTMergeResolver resolver(CRDTMergeResolver::CRDTType::LWW_REGISTER);
    std::vector<MMWriteEntry> writes;
    writes.reserve(static_cast<std::size_t>(write_count));

    for (int i = 0; i < write_count; ++i) {
        auto entry = makeMMEntry(base_physical + static_cast<uint64_t>(i),
                                 static_cast<uint32_t>(i % 4),
                                 "node-" + std::to_string(i),
                                 "w-" + std::to_string(i));
        entry.data = "{\"value\":" + std::to_string(i) + "}";
        writes.push_back(std::move(entry));
    }

    for (auto _ : state) {
        const auto merged = resolver.resolve("doc_crdt", writes);
        benchmark::DoNotOptimize(merged);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("writes=" + std::to_string(write_count));
}

BENCHMARK(BM_CRDTMerge)
    ->Arg(2)
    ->Arg(8)
    ->Arg(32)
    ->Unit(benchmark::kNanosecond);

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

// ============================================================================
// ReplicationManager::promoteToLeader direct metric (R-3)
// ============================================================================

static void BM_ReplicationManager_PromoteToLeader(benchmark::State& state) {
    const std::string wal_dir = "./data/bench_repl_mgr_failover_tmp";
    std::size_t success_count = 0;

    for (auto _ : state) {
        state.PauseTiming();
        if (std::filesystem::exists(wal_dir)) {
            std::filesystem::remove_all(wal_dir);
        }
        std::filesystem::create_directories(wal_dir);

        ReplicationManager mgr(makeConfig(wal_dir));
        bool init_ok = mgr.initialize();

        state.ResumeTiming();

        bool promote_ok = false;
        if (init_ok) {
            promote_ok = mgr.promoteToLeader();
        }
        benchmark::DoNotOptimize(promote_ok);
        if (promote_ok) {
            ++success_count;
        }

        state.PauseTiming();
        mgr.shutdown();
        state.ResumeTiming();
    }

    if (std::filesystem::exists(wal_dir)) {
        std::filesystem::remove_all(wal_dir);
    }

    const double success_rate = state.iterations() == 0
        ? 0.0
        : (100.0 * static_cast<double>(success_count) /
           static_cast<double>(state.iterations()));
    state.counters["leader_promotion_success_rate_pct"] = success_rate;
}

BENCHMARK(BM_ReplicationManager_PromoteToLeader)
    ->Unit(benchmark::kMillisecond)
    ->Iterations(30);

BENCHMARK_MAIN();
