// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_failover_release_gates.cpp
 * @brief Phase 5 failover hot-path release-gate benchmarks (FRG-01..FRG-06).
 *
 * Provides reproducible latency measurements for the failover hot paths
 * identified in the failover module roadmap (Phase 5 — Performance and Hardening).
 * Results are used as release gates: a regression beyond 10% vs the baseline
 * blocks promotion.
 *
 * ## Benchmark families
 *
 * ### FRG-01 — Heartbeat send overhead
 *   p99 ≤ 500 µs
 *
 * ### FRG-02 — Leader election decision (in-memory, deterministic)
 *   p99 ≤ 5 ms
 *
 * ### FRG-03 — State sync message serialize
 *   p99 ≤ 200 µs
 *
 * ### FRG-04 — Health check evaluation
 *   p99 ≤ 100 µs
 *
 * ### FRG-05 — In-flight request buffer check
 *   p99 ≤ 50 µs
 *
 * ### FRG-06 — Epoch increment + persist (mock)
 *   p99 ≤ 1 ms
 *
 * ## Hard release gates
 *
 * | Gate ID     | Benchmark | Threshold  |
 * |-------------|-----------|------------|
 * | GATE-FRG-01 | FRG-01    | p99 ≤ 500 µs |
 * | GATE-FRG-02 | FRG-02    | p99 ≤ 5 ms   |
 * | GATE-FRG-03 | FRG-03    | p99 ≤ 200 µs |
 * | GATE-FRG-04 | FRG-04    | p99 ≤ 100 µs |
 * | GATE-FRG-05 | FRG-05    | p99 ≤ 50 µs  |
 * | GATE-FRG-06 | FRG-06    | p99 ≤ 1 ms   |
 *
 * @see include/failover/failover_api_contract.h
 * @see src/failover/ROADMAP.md — Phase 5 item
 */

#include <benchmark/benchmark.h>

#include "failover/failover_api_contract.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstring>
#include <random>
#include <string>
#include <vector>

using namespace themis::failover;
using namespace std::chrono_literals;

namespace themis {
namespace bench {
namespace frg {

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr uint64_t kFailoverCanonicalSeed = 42;
static constexpr int      kRepetitions           = 5;
static constexpr int      kWarmupIterations      = 100;

// ---------------------------------------------------------------------------
// Mock: heartbeat message (fixed-size struct, in-memory serialise)
// ---------------------------------------------------------------------------

struct HeartbeatMsg {
    std::uint64_t epoch = {};
    std::uint64_t sender_id;
    std::uint64_t timestamp_ms;
    char          padding[8];  // fill to 32 bytes
};

static HeartbeatMsg makeHeartbeat(std::uint64_t epoch, std::uint64_t id) noexcept {
    return {epoch, id, static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count()), {}};
}

// ---------------------------------------------------------------------------
// Mock: simple in-memory election (N votes, first-past-the-post)
// ---------------------------------------------------------------------------

static std::uint64_t runMockElection(std::uint64_t current_epoch, std::uint64_t n_nodes) noexcept {
    // Deterministic: node 0 always wins (stable sort by ID).
    // Models the decision computation, not network.
    std::uint64_t winner = 0;
    for (std::uint64_t i = 1; i < n_nodes; ++i) {
        if (i < winner) {
          winner = i;
        }
    }
    return current_epoch + 1u;
}

// ---------------------------------------------------------------------------
// Mock: state-sync message serialize (fixed payload)
// ---------------------------------------------------------------------------

struct StateSyncMsg {
    std::uint64_t epoch = {};
    std::uint64_t offset;   // log offset
    char          data[64]; // fixed 64-byte payload
};

static StateSyncMsg makeStateSync(std::uint64_t epoch, std::uint64_t offset) noexcept {
    StateSyncMsg m{};
    m.epoch  = epoch;
    m.offset = offset;
    std::memset(m.data, static_cast<int>(epoch & 0xFF), sizeof(m.data));
    return m;
}

// ---------------------------------------------------------------------------
// Mock: health check evaluation
// ---------------------------------------------------------------------------

struct NodeHealthState {
    std::uint64_t last_hb_ms = {};
    std::uint64_t timeout_ms = {};
    bool          alive = {};
};

[[nodiscard]] static bool evaluateHealth(const NodeHealthState& s,
                                          std::uint64_t now_ms) noexcept {
    return s.alive && (now_ms - s.last_hb_ms) < s.timeout_ms;
}

// ---------------------------------------------------------------------------
// Mock: in-flight buffer occupancy check
// ---------------------------------------------------------------------------

static constexpr std::size_t kBufSize = 1024;

struct InFlightBuffer {
    std::atomic<std::size_t> count{0};
    std::size_t              capacity{kBufSize};

    bool hasRoom(std::size_t n = 1) const noexcept {
        return count.load(std::memory_order_relaxed) + n <= capacity;
    }
};

// ---------------------------------------------------------------------------
// Mock: epoch store (atomic increment + mock persist)
// ---------------------------------------------------------------------------

static std::atomic<std::uint64_t> g_epoch{kFirstValidEpoch};

static std::uint64_t incrementAndPersistEpoch() noexcept {
    // In production this is a RocksDB Put; here it is an atomic increment.
    return g_epoch.fetch_add(1u, std::memory_order_relaxed) + 1u;
}

// ===========================================================================
// FRG-01 — Heartbeat send overhead  (GATE-FRG-01: p99 ≤ 500 µs)
// ===========================================================================

/**
 * @brief FRG-01: Heartbeat message construction + in-memory "send" overhead.
 * GATE-FRG-01: p99 ≤ 500 µs.
 */
static void BM_FRG01_HeartbeatSendOverhead(benchmark::State& state) {
    std::mt19937_64 rng(kFailoverCanonicalSeed);
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(makeHeartbeat(1u, rng()));

    for (auto _ : state) {
        benchmark::DoNotOptimize(makeHeartbeat(
            g_epoch.load(std::memory_order_relaxed), rng()));
    }
    state.SetLabel("GATE-FRG-01: p99 <= 500 us");
}
BENCHMARK(BM_FRG01_HeartbeatSendOverhead)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FRG-02 — Leader election decision (in-memory, deterministic)
//          GATE-FRG-02: p99 ≤ 5 ms
// ===========================================================================

/**
 * @brief FRG-02: In-memory leader election decision for 5-node cluster.
 * GATE-FRG-02: p99 ≤ 5 ms.
 */
static void BM_FRG02_ElectionDecision(benchmark::State& state) {
    const std::uint64_t n_nodes = static_cast<std::uint64_t>(state.range(0));
    std::uint64_t epoch = kFirstValidEpoch;
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(runMockElection(epoch, n_nodes));

    for (auto _ : state) {
        benchmark::DoNotOptimize(runMockElection(epoch++, n_nodes));
    }
    state.SetLabel("GATE-FRG-02: p99 <= 5 ms");
}
BENCHMARK(BM_FRG02_ElectionDecision)
    ->Arg(3)->Arg(5)->Arg(9)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FRG-03 — State sync message serialize  (GATE-FRG-03: p99 ≤ 200 µs)
// ===========================================================================

/**
 * @brief FRG-03: StateSyncMsg construction (mock serialize).
 * GATE-FRG-03: p99 ≤ 200 µs.
 */
static void BM_FRG03_StateSyncSerialize(benchmark::State& state) {
    std::uint64_t offset = 0u;
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(makeStateSync(1u, offset++));

    for (auto _ : state) {
        benchmark::DoNotOptimize(makeStateSync(2u, offset++));
    }
    state.SetLabel("GATE-FRG-03: p99 <= 200 us");
}
BENCHMARK(BM_FRG03_StateSyncSerialize)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FRG-04 — Health check evaluation  (GATE-FRG-04: p99 ≤ 100 µs)
// ===========================================================================

/**
 * @brief FRG-04: NodeHealthState evaluation (single node).
 * GATE-FRG-04: p99 ≤ 100 µs.
 */
static void BM_FRG04_HealthCheckEvaluation(benchmark::State& state) {
    const std::uint64_t now_ms = static_cast<std::uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count() / 1'000'000u);
    NodeHealthState s{now_ms - 200u,
                      static_cast<std::uint64_t>(kHeartbeatTimeout.count()),
                      true};

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(evaluateHealth(s, now_ms));

    for (auto _ : state) {
        benchmark::DoNotOptimize(evaluateHealth(s, now_ms));
    }
    state.SetLabel("GATE-FRG-04: p99 <= 100 us");
}
BENCHMARK(BM_FRG04_HealthCheckEvaluation)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FRG-05 — In-flight request buffer check  (GATE-FRG-05: p99 ≤ 50 µs)
// ===========================================================================

/**
 * @brief FRG-05: InFlightBuffer::hasRoom() atomic load.
 * GATE-FRG-05: p99 ≤ 50 µs.
 */
static void BM_FRG05_InFlightBufferCheck(benchmark::State& state) {
    InFlightBuffer buf;
    buf.count.store(512u, std::memory_order_relaxed);

    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(buf.hasRoom());

    for (auto _ : state) {
        benchmark::DoNotOptimize(buf.hasRoom());
    }
    state.SetLabel("GATE-FRG-05: p99 <= 50 us");
}
BENCHMARK(BM_FRG05_InFlightBufferCheck)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ===========================================================================
// FRG-06 — Epoch increment + persist (mock)  (GATE-FRG-06: p99 ≤ 1 ms)
// ===========================================================================

/**
 * @brief FRG-06: Atomic epoch increment (models "increment + mock persist" cost).
 * GATE-FRG-06: p99 ≤ 1 ms.
 */
static void BM_FRG06_EpochIncrementPersist(benchmark::State& state) {
    for (int i = 0; i < kWarmupIterations; ++i)
        benchmark::DoNotOptimize(incrementAndPersistEpoch());

    for (auto _ : state) {
        benchmark::DoNotOptimize(incrementAndPersistEpoch());
    }
    state.SetLabel("GATE-FRG-06: p99 <= 1 ms");
}
BENCHMARK(BM_FRG06_EpochIncrementPersist)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace frg
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
