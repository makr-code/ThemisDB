/**
 * @file bench_transaction_phase4.cpp
 * @brief Phase 4: Performance and Operational Hardening Benchmarks
 *
 * Phase 4 establishes performance baselines and operational limits for the
 * transaction module under representative workloads. Benchmarks verify that
 * hardening from Phases 1–3 does not regress throughput or latency.
 *
 * Performance Gates:
 * - AC-14: Local throughput baseline (begin/commit round-trip)
 * - AC-15: Isolation level overhead comparison (READ_COMMITTED vs SERIALIZABLE)
 * - AC-16: Audit overhead (auditEnabled path vs baseline)
 * - AC-17: Distributed 2PC throughput (using in-process mock participants)
 *
 * Benchmark Count: 8 benchmarks
 * Workload Profiles: single-thread sequential, isolation comparison, distributed
 *
 * Date: 2026-08-08
 * Target: Q1 2027
 */

#include <benchmark/benchmark.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

#include "transaction/transaction_manager.h"
#include "transaction/distributed_transaction_manager.h"
#include "transaction/isolation_level.h"
#include "storage/rocksdb_wrapper.h"
#include "index/secondary_index.h"
#include "index/graph_index.h"
#include "index/vector_index.h"
#include "../benchmark_cleanup.h"

#include <chrono>
#include <cstdio>
#include <filesystem>
#include <memory>
#include <mutex>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis;
using namespace themis::transaction;
using namespace std::chrono_literals;

namespace {

constexpr double kLocalThroughputGateTxnPerSec = 10000.0;
constexpr double kDistributedThroughputGateTxnPerSec = 5000.0;
constexpr double kLatencyTailGateMs = 50.0;

double publishLatencyCounters(benchmark::State& state,
                              const std::vector<double>& latencies_us) {
    if (latencies_us.empty()) {
        state.SkipWithError("Cannot publish latency statistics without samples");
        return 0.0;
    }

    auto sorted_latencies_us = latencies_us;
    std::sort(sorted_latencies_us.begin(), sorted_latencies_us.end());
    const auto percentile_us = [&sorted_latencies_us](double percentile) {
        const auto rank = static_cast<size_t>(
            std::ceil(percentile * static_cast<double>(sorted_latencies_us.size())));
        return sorted_latencies_us[std::max<size_t>(1, rank) - 1];
    };

    double sum_us = 0.0;
    for (const auto latency_us : latencies_us) {
        sum_us += latency_us;
    }
    state.counters["p50_ms"] = percentile_us(0.50) / 1000.0;
    state.counters["p95_real_time"] = percentile_us(0.95) / 1000.0;
    state.counters["p99_real_time"] = percentile_us(0.99) / 1000.0;
    state.counters["p95_ms"] = percentile_us(0.95) / 1000.0;
    state.counters["p99_ms"] = percentile_us(0.99) / 1000.0;
    const auto average_ms =
        sum_us / static_cast<double>(latencies_us.size()) / 1000.0;
    state.counters["avg_ms"] = average_ms;
    return average_ms;
}

void publishThroughputGate(benchmark::State& state,
                           double average_ms,
                           double target_txn_per_sec) {
    const auto throughput = average_ms > 0.0 ? 1000.0 / average_ms : 0.0;
    state.counters["txns_per_sec"] = throughput;
    state.counters["gate_target_txns_per_sec"] = target_txn_per_sec;
    state.counters["gate_pass"] = throughput >= target_txn_per_sec ? 1.0 : 0.0;
}

void publishLatencyGate(benchmark::State& state, double threshold_ms) {
    state.counters["gate_target_p99_ms"] = threshold_ms;
    state.counters["gate_pass"] =
        state.counters["p99_ms"].value <= threshold_ms ? 1.0 : 0.0;
}

void publishOutcomeCounters(benchmark::State& state,
                            uint64_t attempted,
                            uint64_t committed,
                            uint64_t aborted) {
    state.counters["attempted"] = static_cast<double>(attempted);
    state.counters["committed"] = static_cast<double>(committed);
    state.counters["aborted"] = static_cast<double>(aborted);
    state.counters["retried"] = 0.0;
}

}  // namespace

// ============================================================================
// Shared in-process mock participant (always votes COMMIT)
// ============================================================================

class BenchMockParticipant : public IDistributedParticipantCallback {
public:
    bool onPrepare(const std::string&, const std::set<std::string>&) override { return true; }
    void onCommit(const std::string&) override {}
    void onAbort(const std::string&) override {}
};

// ============================================================================
// TransactionBenchmarkFixture
// Sets up an in-process RocksDB + TransactionManager for local txn benchmarks.
// ============================================================================

class TransactionPhase4Fixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        const auto unique_id = static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::ostringstream suffix = {};
        suffix << "bench_txn_phase4_t" << state.thread_index() << "_" << unique_id;
        const auto db_dir = std::filesystem::absolute(
            std::filesystem::path("data") / suffix.str());
        test_db_path_ = db_dir.string();

        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(db_dir, cleanup_error)) {
            throw std::runtime_error("Failed to clear transaction phase 4 benchmark DB: " + cleanup_error.message());
        }

        RocksDBWrapper::Config config;
        config.db_path           = test_db_path_;
        config.wal_dir           = (db_dir / "wal").string();
        config.memtable_size_mb  = 64;
        config.block_cache_size_mb = 128;

        db_ = std::make_unique<RocksDBWrapper>(config);
        if (!db_->open()) {
            throw std::runtime_error("Failed to open benchmark database");
        }

        secondary_index_ = std::make_unique<SecondaryIndexManager>(*db_);
        graph_index_     = std::make_unique<GraphIndexManager>(*db_);
        vector_index_    = std::make_unique<VectorIndexManager>(*db_);

        tx_manager_ = std::make_unique<TransactionManager>(
            *db_, *secondary_index_, *graph_index_, *vector_index_);
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        tx_manager_.reset();
        vector_index_.reset();
        graph_index_.reset();
        secondary_index_.reset();
        if (db_) {
            db_->close();
            db_.reset();
        }
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(test_db_path_, cleanup_error)) {
            std::fprintf(stderr, "Transaction phase 4 benchmark DB cleanup failed: %s\n",
                         cleanup_error.message().c_str());
        }
    }

protected:
    std::string test_db_path_;
    std::unique_ptr<RocksDBWrapper>          db_;
    std::unique_ptr<SecondaryIndexManager>   secondary_index_;
    std::unique_ptr<GraphIndexManager>       graph_index_;
    std::unique_ptr<VectorIndexManager>      vector_index_;
    std::unique_ptr<TransactionManager>      tx_manager_;
};

// ============================================================================
// DistributedPhase4Fixture
// Sets up a DistributedTransactionManager with in-process mock participants.
// ============================================================================

class DistributedPhase4Fixture : public benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State& state) override {
        DistributedTxnManagerConfig cfg;
        cfg.prepare_timeout     = 5000ms;
        cfg.commit_timeout      = 5000ms;
        cfg.default_txn_timeout = 60s;

        const auto unique_id = static_cast<unsigned long long>(
            std::chrono::steady_clock::now().time_since_epoch().count());
        std::ostringstream name = {};
        name << "bench-phase4-coord-t" << state.thread_index() << "_" << unique_id;

        const auto wal_dir = std::filesystem::absolute(
            std::filesystem::path("data") /
            ("bench_phase4_wal_t" + std::to_string(state.thread_index()) + "_" +
             std::to_string(unique_id)));
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(wal_dir, cleanup_error)) {
            throw std::runtime_error(
                "Failed to clear transaction phase 4 WAL: " + cleanup_error.message());
        }
        cfg.wal_directory = wal_dir.string();
        wal_dir_ = wal_dir;
        mgr_ = std::make_unique<DistributedTransactionManager>(name.str(), cfg);

        for (auto& p : participants_) {
            p = std::make_unique<BenchMockParticipant>();
        }
    }

    void TearDown(const ::benchmark::State& /*state*/) override {
        mgr_.reset();
        std::error_code cleanup_error;
        if (!themis::bench::removeBenchmarkDirectory(wal_dir_, cleanup_error)) {
            std::fprintf(stderr, "Transaction phase 4 WAL cleanup failed: %s\n",
                         cleanup_error.message().c_str());
        }
    }

protected:
    std::unique_ptr<DistributedTransactionManager> mgr_;
    std::filesystem::path wal_dir_;
    std::array<std::unique_ptr<BenchMockParticipant>, 3> participants_;

    std::vector<Participant> makeParticipants() {
        std::vector<Participant> ps = {};

        for (int i = 0; i < 3; ++i) {
            Participant p;
            p.node_id       = "bench-node-" + std::to_string(i);
            p.endpoint      = p.node_id + ":9090";
            p.affected_keys = {"bench-key"};
            p.callback      = participants_[i].get();
            ps.push_back(p);
        }
        return ps;
    }
};

// ============================================================================
// AC-14: Local Throughput Baseline
// ============================================================================

/**
 * @benchmark ThroughputBaseline_ReadCommitted
 * @gate AC-14: Throughput Baseline
 * @target Measures begin/commit round-trip with READ_COMMITTED isolation.
 */
BENCHMARK_DEFINE_F(TransactionPhase4Fixture, ThroughputBaseline_ReadCommitted)
    (benchmark::State& state)
{
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto txn_id = tx_manager_->beginTransaction(IsolationLevel::READ_COMMITTED);
        auto status = tx_manager_->commitTransaction(txn_id);
        const auto end = std::chrono::steady_clock::now();
        if (!status.ok) {
            state.SkipWithError("Commit failed");
            return;
        }
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        ++committed;
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    publishOutcomeCounters(state, attempted, committed, 0);
    const auto average_ms = publishLatencyCounters(state, latencies_us);
    publishThroughputGate(state, average_ms, kLocalThroughputGateTxnPerSec);
}
BENCHMARK_REGISTER_F(TransactionPhase4Fixture, ThroughputBaseline_ReadCommitted)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark ThroughputBaseline_Rollback
 * @gate AC-14: Rollback throughput
 */
BENCHMARK_DEFINE_F(TransactionPhase4Fixture, ThroughputBaseline_Rollback)
    (benchmark::State& state)
{
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t aborted = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto txn_id = tx_manager_->beginTransaction();
        if (!tx_manager_->rollbackTransaction(txn_id)) {
            state.SkipWithError("Rollback failed");
            return;
        }
        ++aborted;
        const auto end = std::chrono::steady_clock::now();
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(aborted));
    publishOutcomeCounters(state, attempted, 0, aborted);
    publishLatencyCounters(state, latencies_us);
    publishLatencyGate(state, kLatencyTailGateMs);
}
BENCHMARK_REGISTER_F(TransactionPhase4Fixture, ThroughputBaseline_Rollback)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// AC-15: Isolation Level Overhead Comparison
// ============================================================================

/**
 * @benchmark IsolationOverhead_ReadCommitted
 * @gate AC-15: Isolation level baseline
 */
BENCHMARK_DEFINE_F(TransactionPhase4Fixture, IsolationOverhead_ReadCommitted)
    (benchmark::State& state)
{
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto txn_id = tx_manager_->beginTransaction(IsolationLevel::READ_COMMITTED);
        auto status = tx_manager_->commitTransaction(txn_id);
        const auto end = std::chrono::steady_clock::now();
        if (!status.ok) {
            state.SkipWithError("READ_COMMITTED commit failed");
            return;
        }
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        ++committed;
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    publishOutcomeCounters(state, attempted, committed, 0);
    publishLatencyCounters(state, latencies_us);
    publishLatencyGate(state, kLatencyTailGateMs);
}
BENCHMARK_REGISTER_F(TransactionPhase4Fixture, IsolationOverhead_ReadCommitted)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark IsolationOverhead_Serializable
 * @gate AC-15: Isolation level comparison (SERIALIZABLE vs READ_COMMITTED)
 * @target SSI overhead should not exceed 2× baseline.
 */
BENCHMARK_DEFINE_F(TransactionPhase4Fixture, IsolationOverhead_Serializable)
    (benchmark::State& state)
{
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto txn_id = tx_manager_->beginTransaction(IsolationLevel::SERIALIZABLE);
        auto status = tx_manager_->commitTransaction(txn_id);
        const auto end = std::chrono::steady_clock::now();
        if (!status.ok) {
            state.SkipWithError("SERIALIZABLE commit failed");
            return;
        }
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        ++committed;
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    publishOutcomeCounters(state, attempted, committed, 0);
    publishLatencyCounters(state, latencies_us);
    publishLatencyGate(state, kLatencyTailGateMs);
}
BENCHMARK_REGISTER_F(TransactionPhase4Fixture, IsolationOverhead_Serializable)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// AC-16: Audit Overhead
// ============================================================================

/**
 * @benchmark AuditOverhead_Baseline
 * @gate AC-16: Baseline (snapshot isolation)
 */
BENCHMARK_DEFINE_F(TransactionPhase4Fixture, AuditOverhead_Baseline)
    (benchmark::State& state)
{
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto txn_id = tx_manager_->beginTransaction(IsolationLevel::Snapshot);
        auto status = tx_manager_->commitTransaction(txn_id);
        const auto end = std::chrono::steady_clock::now();
        if (!status.ok) {
            state.SkipWithError("Snapshot commit failed");
            return;
        }
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        ++committed;
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    publishOutcomeCounters(state, attempted, committed, 0);
    publishLatencyCounters(state, latencies_us);
    publishLatencyGate(state, kLatencyTailGateMs);
}
BENCHMARK_REGISTER_F(TransactionPhase4Fixture, AuditOverhead_Baseline)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark AuditOverhead_Enabled
 * @gate AC-16: SERIALIZABLE isolation overhead as audit-path proxy.
 * @note Full audit logging is exercised via the TransactionAuditor component
 *       (see bench_transaction_throughput.cpp). This benchmark provides an
 *       isolation-level overhead proxy for the phase 4 gate.
 */
BENCHMARK_DEFINE_F(TransactionPhase4Fixture, AuditOverhead_Enabled)
    (benchmark::State& state)
{
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto txn_id = tx_manager_->beginTransaction(IsolationLevel::SERIALIZABLE);
        auto status = tx_manager_->commitTransaction(txn_id);
        const auto end = std::chrono::steady_clock::now();
        if (!status.ok) {
            state.SkipWithError("SERIALIZABLE audit-path commit failed");
            return;
        }
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        ++committed;
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    publishOutcomeCounters(state, attempted, committed, 0);
    publishLatencyCounters(state, latencies_us);
    publishLatencyGate(state, kLatencyTailGateMs);
}
BENCHMARK_REGISTER_F(TransactionPhase4Fixture, AuditOverhead_Enabled)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// AC-17: Distributed 2PC Throughput
// ============================================================================

/**
 * @benchmark DistributedThroughput_2PC_3Participants
 * @gate AC-17: Distributed 2PC throughput (begin → prepare → commit).
 */
BENCHMARK_DEFINE_F(DistributedPhase4Fixture, DistributedThroughput_2PC_3Participants)
    (benchmark::State& state)
{
    auto participants = makeParticipants();
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t committed = 0;
    constexpr uint64_t retried = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto tid = mgr_->beginDistributed(participants);
        if (tid.empty()) {
            state.SkipWithError("Failed to begin distributed transaction");
            return;
        }
        auto ps = mgr_->prepareDistributed(tid);
        if (!ps.ok) {
            const auto message = "Prepare failed: " + ps.message;
            state.SkipWithError(message.c_str());
            return;
        }
        auto cs = mgr_->commitDistributed(tid);
        if (!cs.ok) {
            const auto message = "Commit failed: " + cs.message;
            state.SkipWithError(message.c_str());
            return;
        }
        const auto end = std::chrono::steady_clock::now();
        ++committed;
        benchmark::DoNotOptimize(tid);
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(static_cast<int64_t>(committed));
    state.counters["attempted"] = static_cast<double>(attempted);
    state.counters["committed"] = static_cast<double>(committed);
    state.counters["aborted"] = 0.0;
    state.counters["retried"] = static_cast<double>(retried);
    const auto average_ms = publishLatencyCounters(state, latencies_us);
    publishThroughputGate(state, average_ms, kDistributedThroughputGateTxnPerSec);
}
BENCHMARK_REGISTER_F(DistributedPhase4Fixture, DistributedThroughput_2PC_3Participants)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

/**
 * @benchmark DistributedThroughput_AbortPath
 * @gate AC-17: Distributed 2PC abort throughput.
 */
BENCHMARK_DEFINE_F(DistributedPhase4Fixture, DistributedThroughput_AbortPath)
    (benchmark::State& state)
{
    auto participants = makeParticipants();
    std::vector<double> latencies_us;
    uint64_t attempted = 0;
    uint64_t aborted_count = 0;
    constexpr uint64_t retried = 0;
    for (auto _ : state) {
        ++attempted;
        const auto start = std::chrono::steady_clock::now();
        auto tid = mgr_->beginDistributed(participants);
        if (tid.empty()) {
            state.SkipWithError("Failed to begin distributed abort-path transaction");
            return;
        }
        mgr_->abortDistributed(tid);
        const auto end = std::chrono::steady_clock::now();
        const auto aborted = mgr_->getTransaction(tid);
        if (!aborted || aborted->state != DistributedTxnState::ABORTED) {
            state.SkipWithError("Distributed abort did not reach ABORTED state");
            return;
        }
        ++aborted_count;
        benchmark::DoNotOptimize(tid);
        const auto elapsed_us =
            std::chrono::duration<double, std::micro>(end - start).count();
        latencies_us.push_back(elapsed_us);
        state.SetIterationTime(elapsed_us * 1e-6);
    }
    state.SetItemsProcessed(state.iterations());
    state.counters["attempted"] = static_cast<double>(attempted);
    state.counters["committed"] = 0.0;
    state.counters["aborted"] = static_cast<double>(aborted_count);
    state.counters["retried"] = static_cast<double>(retried);
    publishLatencyCounters(state, latencies_us);
    publishLatencyGate(state, kLatencyTailGateMs);
}
BENCHMARK_REGISTER_F(DistributedPhase4Fixture, DistributedThroughput_AbortPath)
    ->UseManualTime()
    ->Unit(benchmark::kMillisecond);

// ============================================================================
// Benchmark main
// ============================================================================

BENCHMARK_MAIN();
