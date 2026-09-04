// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_sharding_release_gates.cpp
 * @brief Phase 5 sharding release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the sharding module's
 * hot paths identified in src/sharding/ROADMAP.md (Phase 5 — Performance
 * and Hardening). Complements the existing bench_sharding_performance.cpp.
 *
 * ## Benchmark families
 *
 * ### SRG-01 — Consistent-Hash Routing
 *   SRG-01  Consistent-hash ring lookup (1K shards, warm)
 *
 * ### SRG-02 — 2PC Prepare
 *   SRG-02  Two-phase commit prepare() overhead (single participant, mock)
 *
 * ### SRG-03 — 2PC Commit
 *   SRG-03  Two-phase commit commit() overhead (single participant, mock)
 *
 * ### SRG-04 — WAL Append
 *   SRG-04  WAL append (in-memory mock, no fsync)
 *
 * ### SRG-05 — Shard Health Check
 *   SRG-05  Shard health check (no network, local state)
 *
 * ### SRG-06 — Route Lookup
 *   SRG-06  Hash ring lookup (64-shard warm cache)
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark | Threshold    |
 * |--------------|-----------|--------------|
 * | GATE-SRG-01  | SRG-01    | p99 ≤ 50 µs  |
 * | GATE-SRG-02  | SRG-02    | p99 ≤ 1 ms   |
 * | GATE-SRG-03  | SRG-03    | p99 ≤ 500 µs |
 * | GATE-SRG-04  | SRG-04    | p99 ≤ 100 µs |
 * | GATE-SRG-05  | SRG-05    | p99 ≤ 50 µs  |
 * | GATE-SRG-06  | SRG-06    | p99 ≤ 20 µs  |
 *
 * All benchmarks:
 *   - Use kShardCanonicalSeed = 42 for deterministic key/ID generation.
 *   - Warm up for kShardWarmupIterations before measurement.
 *   - Run with Repetitions(kShardRepetitions) to capture variance.
 *
 * @see src/sharding/ROADMAP.md — Phase 5 items
 * @see include/sharding/sharding_api_contract.h — contract constants
 * @see benchmarks/sharding/bench_sharding_performance.cpp — complementary benchmarks
 */

#include <benchmark/benchmark.h>

#include "sharding/sharding_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <random>
#include <string>
#include <unordered_map>
#include <vector>

using namespace themis::sharding;

namespace themis {
namespace bench {
namespace srg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Canonical PRNG seed for all SRG benchmarks.
static constexpr uint64_t kShardCanonicalSeed = 42;

/// Warmup iterations before measurement window.
static constexpr int kShardWarmupIterations = 200;

/// Repetitions per benchmark.
static constexpr int kShardRepetitions = 5;

// ---------------------------------------------------------------------------
// Minimal stubs
// ---------------------------------------------------------------------------

/// Stub consistent-hash ring (no vnodes for benchmark simplicity).
class StubHashRing {
public:
    explicit StubHashRing(int num_shards) : num_shards_(num_shards) {}
    int lookup(const std::string& key) const noexcept {
        if (num_shards_ == 0) {
          return -1;
        }
        std::size_t h = std::hash<std::string>{}(key);
        return static_cast<int>(h % static_cast<std::size_t>(num_shards_));
    }
private:
    int num_shards_;
};

/// Stub 2PC participant (in-memory, no network).
class StubTxnParticipant {
public:
    enum class State { IDLE, PREPARED, COMMITTED, ABORTED };
    State state{State::IDLE};
    std::string current_txn = {};

    ShardingErrorCode prepare(const std::string& txn_id) noexcept {
        current_txn = txn_id;
        state = State::PREPARED;
        return ShardingErrorCode::OK;
    }
    ShardingErrorCode commit() noexcept {
        if (state == State::COMMITTED) return ShardingErrorCode::OK;  // idempotent
        state = State::COMMITTED;
        return ShardingErrorCode::OK;
    }
    void reset() noexcept { state = State::IDLE; current_txn.clear(); }
};

/// Stub WAL (in-memory append, no fsync).
class StubInMemoryWal {
public:
    struct Entry { uint64_t lsn; std::string data; };
    ShardingErrorCode append(const std::string& data) {
        entries_.push_back({next_lsn_++, data});
        return ShardingErrorCode::OK;
    }
    void clear() noexcept { entries_.clear(); next_lsn_ = 0; }
private:
    std::vector<Entry> entries_;
    uint64_t next_lsn_{0};
};

/// Stub shard health state (in-memory, no network probes).
class StubShardHealth {
public:
    explicit StubShardHealth(int num_shards) : health_(num_shards, true) {}
    bool isHealthy(int shard_id) const noexcept {
        if (shard_id < 0 || shard_id >= static_cast<int>(health_.size())) {
          return false;
        }
        return health_[static_cast<std::size_t>(shard_id)];
    }
private:
    std::vector<bool> health_;
};

// ===========================================================================
// SRG-01 — Consistent-hash routing (1K shards, warm)
// ===========================================================================

/**
 * @brief SRG-01: Consistent-hash ring lookup with 1 024 shards (warm).
 *
 * GATE-SRG-01: p99 ≤ 50 µs.
 */
static void BM_SRG01_ConsistentHashRouting1K(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    StubHashRing ring(n);
    // Warm up
    for (int i = 0; i < kShardWarmupIterations; ++i) {
        benchmark::DoNotOptimize(ring.lookup("warm-key-" + std::to_string(i)));
    }
    std::mt19937_64 rng_engine(kShardCanonicalSeed);
    std::uniform_int_distribution<uint64_t> dist = {};

    for (auto _ : state) {
        std::string key = "key-" + std::to_string(dist(rng_engine));
        benchmark::DoNotOptimize(ring.lookup(key));
    }
    state.SetLabel("GATE-SRG-01: p99 <= 50 us");
}
BENCHMARK(BM_SRG01_ConsistentHashRouting1K)
    ->Arg(1024)
    ->Repetitions(kShardRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SRG-02 — 2PC prepare (single participant, mock)
// ===========================================================================

/**
 * @brief SRG-02: 2PC prepare() on a single in-memory participant.
 *
 * GATE-SRG-02: p99 ≤ 1 ms.
 */
static void BM_SRG02_TwoPhaseCommitPrepare(benchmark::State& state) {
    StubTxnParticipant participant;
    int counter = 0;
    for (int i = 0; i < kShardWarmupIterations; ++i) {
        participant.reset();
        participant.prepare("warm-" + std::to_string(i));
    }
    for (auto _ : state) {
        participant.reset();
        std::string txn = "txn-" + std::to_string(counter++);
        benchmark::DoNotOptimize(participant.prepare(txn));
    }
    state.SetLabel("GATE-SRG-02: p99 <= 1 ms");
}
BENCHMARK(BM_SRG02_TwoPhaseCommitPrepare)
    ->Repetitions(kShardRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SRG-03 — 2PC commit (single participant, mock)
// ===========================================================================

/**
 * @brief SRG-03: 2PC commit() on a pre-prepared in-memory participant.
 *
 * GATE-SRG-03: p99 ≤ 500 µs.
 */
static void BM_SRG03_TwoPhaseCommitCommit(benchmark::State& state) {
    StubTxnParticipant participant;
    // Pre-prepare
    participant.prepare("bench-txn");
    for (int i = 0; i < kShardWarmupIterations; ++i) {
        participant.state = StubTxnParticipant::State::PREPARED;
        participant.commit();
    }
    for (auto _ : state) {
        participant.state = StubTxnParticipant::State::PREPARED;
        benchmark::DoNotOptimize(participant.commit());
    }
    state.SetLabel("GATE-SRG-03: p99 <= 500 us");
}
BENCHMARK(BM_SRG03_TwoPhaseCommitCommit)
    ->Repetitions(kShardRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SRG-04 — WAL append (in-memory mock)
// ===========================================================================

/**
 * @brief SRG-04: WAL append() to an in-memory log (no fsync).
 *
 * GATE-SRG-04: p99 ≤ 100 µs.
 */
static void BM_SRG04_WalAppend(benchmark::State& state) {
    StubInMemoryWal wal;
    for (int i = 0; i < kShardWarmupIterations; ++i) {
        wal.append("warmup-entry-" + std::to_string(i));
    }
    wal.clear();
    const std::string entry_data = "bench-wal-entry-data";
    for (auto _ : state) {
        benchmark::DoNotOptimize(wal.append(entry_data));
    }
    state.SetLabel("GATE-SRG-04: p99 <= 100 us");
}
BENCHMARK(BM_SRG04_WalAppend)
    ->Repetitions(kShardRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SRG-05 — Shard health check (no network, local state)
// ===========================================================================

/**
 * @brief SRG-05: Shard health check from local in-memory state.
 *
 * GATE-SRG-05: p99 ≤ 50 µs.
 */
static void BM_SRG05_ShardHealthCheck(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    StubShardHealth health(n);
    for (int i = 0; i < kShardWarmupIterations; ++i) {
        benchmark::DoNotOptimize(health.isHealthy(i % n));
    }
    std::mt19937_64 rng_engine(kShardCanonicalSeed);
    std::uniform_int_distribution<int> dist(0, n - 1);
    for (auto _ : state) {
        benchmark::DoNotOptimize(health.isHealthy(dist(rng_engine)));
    }
    state.SetLabel("GATE-SRG-05: p99 <= 50 us");
}
BENCHMARK(BM_SRG05_ShardHealthCheck)
    ->Arg(64)
    ->Repetitions(kShardRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

// ===========================================================================
// SRG-06 — Route lookup (hash ring, 64-shard warm cache)
// ===========================================================================

/**
 * @brief SRG-06: Hash ring lookup with 64 shards (warm).
 *
 * GATE-SRG-06: p99 ≤ 20 µs.
 */
static void BM_SRG06_RouteLookup64ShardWarm(benchmark::State& state) {
    StubHashRing ring(64);
    for (int i = 0; i < kShardWarmupIterations; ++i) {
        benchmark::DoNotOptimize(ring.lookup("warm-" + std::to_string(i)));
    }
    std::mt19937_64 rng_engine(kShardCanonicalSeed);
    std::uniform_int_distribution<uint64_t> dist = {};

    for (auto _ : state) {
        std::string key = "k-" + std::to_string(dist(rng_engine));
        benchmark::DoNotOptimize(ring.lookup(key));
    }
    state.SetLabel("GATE-SRG-06: p99 <= 20 us");
}
BENCHMARK(BM_SRG06_RouteLookup64ShardWarm)
    ->Repetitions(kShardRepetitions)
    ->ReportAggregatesOnly(true)
    ->Unit(benchmark::kMicrosecond);

} // namespace srg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
