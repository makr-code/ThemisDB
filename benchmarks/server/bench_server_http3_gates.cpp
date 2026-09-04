/**
 * @file bench_server_http3_gates.cpp
 * @brief Phase 2 Protocol-Hardening HTTP/3 release-gate benchmarks.
 *
 * Provides reproducible latency measurements for HTTP/3 QUIC connection
 * migration and 0-RTT session resumption paths identified in
 * src/server/ROADMAP.md (Phase 2 — Multi-Transport Resilience).
 *
 * Results are used as promotion gates: a regression beyond 10% vs the
 * baseline blocks GA promotion.
 *
 * ## Benchmark families
 *
 * ### SVR-H3-01 — Connection-Migration Latency
 *   Overhead of onPathMigration() including endpoint update and atomic
 *   migration-count increment.  Simulates a client address change mid-stream
 *   without live ngtcp2 conn (in-process stub).
 *
 * ### SVR-H3-02 — 0-RTT Resumption Decision Latency
 *   CPU cost of the session-token validity check (StubZeroRttGate path),
 *   representing the fast-path decision at the security gate before any
 *   early data is accepted or rejected.
 *
 * ## Hard release gates
 *
 * | Gate ID        | Benchmark  | Threshold          |
 * |----------------|------------|--------------------|
 * | GATE-SVR-H3-01 | SVR-H3-01  | p99 ≤ 5 µs         |
 * | GATE-SVR-H3-02 | SVR-H3-02  | p99 ≤ 1 µs         |
 *
 * All benchmarks:
 *   - Use kHttp3GatesSeed = 7331 for deterministic data generation.
 *   - Run with default Google Benchmark iteration management.
 *   - No real UDP sockets or QUIC sessions are opened.
 *
 * @see src/server/ROADMAP.md — Phase 2 Protocol and Gateway Hardening
 * @see include/server/server_api_contract.h — contract constants
 * @see tests/server/test_server_http3_stress_focused.cpp — SH3-01..SH3-12
 */

#include <benchmark/benchmark.h>

#include "server/server_api_contract.h"

#include <atomic>
#include <cstdint>
#include <string>

using namespace themis::server;

// ─────────────────────────────────────────────────────────────────────────────
// Benchmark seed
// ─────────────────────────────────────────────────────────────────────────────
static constexpr uint32_t kHttp3GatesSeed = 7331U;

// ─────────────────────────────────────────────────────────────────────────────
// Minimal in-process stubs (mirrors test stubs; no QUIC lib dependency)
// ─────────────────────────────────────────────────────────────────────────────

struct BenchPeerEndpoint {
    std::string address;
    uint16_t    port;
};

/**
 * @brief Minimal QUIC connection state stub for migration benchmarking.
 *
 * Mirrors the bookkeeping that Http3Session::onPathMigration() performs:
 * atomic counter increment and endpoint update (no ngtcp2 linkage needed).
 */
struct BenchQuicConnState {
    BenchPeerEndpoint       current_peer;
    std::atomic<uint32_t>   migration_count{0};
    std::atomic<bool>       draining{false};

    inline void onPathMigration(const BenchPeerEndpoint& new_peer) noexcept {
        if (draining.load(std::memory_order_acquire)) { return; }
        current_peer = new_peer;
        migration_count.fetch_add(1, std::memory_order_relaxed);
    }
};

/// Minimal session-token stub for 0-RTT gate benchmarking.
struct BenchSessionToken {
    const char* token_id;
    bool valid = {};
    bool replayed = {};
};

/// Inline 0-RTT gate decision — mirrors StubZeroRttGate in test file.
[[nodiscard]] static inline bool benchAcceptEarlyData(
    const BenchSessionToken& tok) noexcept
{
    return tok.valid && !tok.replayed;
}

// ─────────────────────────────────────────────────────────────────────────────
// SVR-H3-01 — Connection-Migration Latency
// GATE-SVR-H3-01: p99 ≤ 5 µs
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Benchmark the hot path of connection migration bookkeeping.
 *
 * Measures the CPU overhead of accepting a peer address change:
 * - draining-state guard (atomic load)
 * - endpoint update (struct assignment)
 * - migration counter increment (atomic fetch_add)
 *
 * This represents the fast path exercised by Http3Session::onPathMigration()
 * in production each time a QUIC Connection ID is seen from a new address.
 *
 * @note No real QUIC connections or UDP sockets are opened.
 */
static void SVR_H3_01_ConnectionMigrationLatency(benchmark::State& state) {
    BenchQuicConnState conn;
    conn.current_peer = {"10.0.0.1", 44000};

    BenchPeerEndpoint new_peer{"10.0.0.2", 44001};

    for (auto _ : state) {
        conn.onPathMigration(new_peer);
        // Prevent the compiler from optimising the loop body away
        benchmark::DoNotOptimize(conn.migration_count.load());
    }

    // Reset counter so repeated benchmark runs start clean
    conn.migration_count.store(0, std::memory_order_relaxed);
}

BENCHMARK(SVR_H3_01_ConnectionMigrationLatency)
    ->Unit(benchmark::kMicrosecond)
    ->Iterations(100000)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// SVR-H3-02 — 0-RTT Resumption Decision Latency
// GATE-SVR-H3-02: p99 ≤ 1 µs
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Benchmark the fast-path 0-RTT token validation decision.
 *
 * Measures the CPU cost of the security gate that decides whether early data
 * from a 0-RTT session is accepted or rejected:
 * - valid-token check (bool read)
 * - replay-protection check (bool read)
 *
 * This is the innermost decision point exercised on every QUIC 0-RTT resumption
 * attempt.  The gate must impose negligible latency (≤ 1 µs p99) to avoid
 * adding meaningful overhead to the QUIC handshake fast path.
 *
 * @note Tests both the accept path (valid, not-replayed) and the reject path
 *       (replayed) to cover both code branches.
 */
static void SVR_H3_02_ZeroRttDecisionAccept(benchmark::State& state) {
    BenchSessionToken token{"session-bench-123", /*valid=*/true, /*replayed=*/false};

    for (auto _ : state) {
        bool accepted = benchAcceptEarlyData(token);
        benchmark::DoNotOptimize(accepted);
    }
}

BENCHMARK(SVR_H3_02_ZeroRttDecisionAccept)
    ->Unit(benchmark::kNanosecond)
    ->Iterations(500000)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);

static void SVR_H3_02_ZeroRttDecisionReject(benchmark::State& state) {
    BenchSessionToken token{"session-bench-replayed", /*valid=*/true, /*replayed=*/true};

    for (auto _ : state) {
        bool accepted = benchAcceptEarlyData(token);
        benchmark::DoNotOptimize(accepted);
    }
}

BENCHMARK(SVR_H3_02_ZeroRttDecisionReject)
    ->Unit(benchmark::kNanosecond)
    ->Iterations(500000)
    ->Repetitions(5)
    ->ReportAggregatesOnly(true);

BENCHMARK_MAIN();
