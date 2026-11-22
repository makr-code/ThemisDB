/**
 * @file bench_transaction_dedicated_gates.cpp
 * @brief Wave D — deferred transaction performance gate registrations.
 *
 * The case names preserve planned targets, but each case is skipped until it
 * exercises production transaction APIs and collects request-level latency.
 * The previous in-process no-op stubs provided no performance evidence.
 *
 * ## Gate definitions
 * - TX-BM-01 : Commit p95 latency          ≤ 500 µs  per commit
 * - TX-BM-02 : Abort p95 latency           ≤ 200 µs  per abort
 * - TX-BM-03 : 2PC round-trip p99 latency  ≤ 5 000 µs per distributed commit
 * - TX-BM-04 : Concurrent throughput       ≥ 10 000 tx/sec (8 threads)
 *
 * ## Labels
 * wave_d;benchmark;not_release_critical
 *
 * @version 1.0.0
 * @see docs/operability/RUNBOOK_TRANSACTION_ENGINE.md
 * @see src/transaction/ROADMAP.md — Wave D contribution
 */

#include <benchmark/benchmark.h>

namespace {
constexpr char kNotMeasuredMessage[] =
    "not measured: prior cases used in-process no-op stubs; production transaction APIs "
    "and request-level latency collection are required";
} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// TX-BM-01 — Commit p95 latency
// Gate: ≤ 500 µs per commit on representative hardware.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Transaction_CommitP95(benchmark::State& state) {
    state.SkipWithMessage(kNotMeasuredMessage);
}
BENCHMARK(BM_Transaction_CommitP95)
    ->Name("BM_Transaction_CommitP95")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// TX-BM-02 — Abort p95 latency
// Gate: ≤ 200 µs per abort on representative hardware.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Transaction_AbortP95(benchmark::State& state) {
    state.SkipWithMessage(kNotMeasuredMessage);
}
BENCHMARK(BM_Transaction_AbortP95)
    ->Name("BM_Transaction_AbortP95")
    ->Iterations(10000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// TX-BM-03 — 2PC round-trip p99 latency
// Gate: ≤ 5 000 µs per distributed commit on representative hardware.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Transaction_2PCRoundTripP99(benchmark::State& state) {
    state.SkipWithMessage(kNotMeasuredMessage);
}
BENCHMARK(BM_Transaction_2PCRoundTripP99)
    ->Name("BM_Transaction_2PCRoundTripP99")
    ->Iterations(5000)
    ->Unit(benchmark::kMicrosecond);

// ─────────────────────────────────────────────────────────────────────────────
// TX-BM-04 — Concurrent throughput (8 threads)
// Gate: ≥ 10 000 tx/sec aggregate throughput.
// ─────────────────────────────────────────────────────────────────────────────
static void BM_Transaction_ConcurrentThroughput(benchmark::State& state) {
    state.SkipWithMessage(kNotMeasuredMessage);
}
BENCHMARK(BM_Transaction_ConcurrentThroughput)
    ->Name("BM_Transaction_ConcurrentThroughput")
    ->Threads(8)
    ->Iterations(1000)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
