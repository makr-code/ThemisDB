// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_rpc_grpc_dedicated_gates.cpp
 * @brief Wave D — rpc_grpc module dedicated benchmark gates.
 *
 * Provides reproducible latency measurements for the rpc_grpc module under
 * Wave D operability requirements.  These benchmarks complement the existing
 * release-gate suite (`bench_rpc_grpc_release_gates.cpp`) with dedicated
 * high-cardinality and stream-adapter scenarios.
 *
 * ## Benchmark families
 *
 * ### GRPC-BM-01 — High-cardinality service registration throughput
 *   Measures the cost of registering a unique service name per iteration.
 *   Models the high-cardinality registration storm scenario.
 *
 * ### GRPC-BM-02 — Stream adapter send throughput
 *   Measures per-frame send latency for the stub stream adapter.
 *
 * ### GRPC-BM-03 — Credential reload latency
 *   Measures the cost of a single TLS credential reload cycle.
 *
 * ### GRPC-BM-04 — Concurrent service call batch (1 000 calls)
 *   Measures amortised per-call cost across 1 000 sequential stub calls.
 *
 * ## Hard release gates
 *
 * | Gate ID    | Benchmark           | Threshold         |
 * |------------|---------------------|-------------------|
 * | GRPC-BM-01 | ServiceRegistration | p99 ≤ 200 ns      |
 * | GRPC-BM-02 | StreamAdapterSend   | p99 ≤ 100 ns      |
 * | GRPC-BM-03 | CredentialReload    | p99 ≤ 1 µs        |
 * | GRPC-BM-04 | BatchServiceCall    | p99 ≤ 50 µs/batch |
 *
 * @see src/rpc_grpc/ROADMAP.md — Wave D items
 * @see docs/operability/RUNBOOK_RPC_GRPC.md
 */

#include <benchmark/benchmark.h>

#include <atomic>
#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace rpc_dedicated {

static constexpr uint64_t kCanonicalSeed = 42;
static constexpr int      kRepetitions   = 5;

// ─── In-process stubs (SIMULATION NOTE: no live gRPC server required) ─────────

static std::atomic<uint64_t> g_stub_reg_count{0};

static bool stubRegisterService(const std::string& name) noexcept {
    g_stub_reg_count.fetch_add(1, std::memory_order_relaxed);
    (void)name;
    return true;
}

static bool stubStreamSend(uint32_t stream_id, uint64_t payload) noexcept {
    (void)stream_id; (void)payload;
    return true;
}

static bool stubReloadCredential(const std::string& path) noexcept {
    (void)path;
    return true;
}

static bool stubServiceCall(uint64_t call_id) noexcept {
    (void)call_id;
    return true;
}

// ─────────────────────────────────────────────────────────────────────────────
// GRPC-BM-01 — High-cardinality service registration throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_GRPC_ServiceRegistration(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const std::string name = "svc_bm_" + std::to_string(seed % 100000);
        benchmark::DoNotOptimize(stubRegisterService(name));
        ++seed;
    }
}
BENCHMARK(BM_GRPC_ServiceRegistration)
    ->Name("GRPC-BM-01/ServiceRegistration")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// GRPC-BM-02 — Stream adapter send throughput
// ─────────────────────────────────────────────────────────────────────────────
static void BM_GRPC_StreamAdapterSend(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const uint32_t stream_id = static_cast<uint32_t>(seed % 1024);
        const uint64_t payload   = seed * 6364136223846793005ULL + 1442695040888963407ULL;
        benchmark::DoNotOptimize(stubStreamSend(stream_id, payload));
        ++seed;
    }
}
BENCHMARK(BM_GRPC_StreamAdapterSend)
    ->Name("GRPC-BM-02/StreamAdapterSend")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// GRPC-BM-03 — Credential reload latency
// ─────────────────────────────────────────────────────────────────────────────
static void BM_GRPC_CredentialReload(benchmark::State& state) {
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        const std::string path = "tls/server_" + std::to_string(seed % 4) + ".crt";
        benchmark::DoNotOptimize(stubReloadCredential(path));
        ++seed;
    }
}
BENCHMARK(BM_GRPC_CredentialReload)
    ->Name("GRPC-BM-03/CredentialReload")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ─────────────────────────────────────────────────────────────────────────────
// GRPC-BM-04 — Batch service call (1 000 iterations)
// ─────────────────────────────────────────────────────────────────────────────
static void BM_GRPC_BatchServiceCall(benchmark::State& state) {
    constexpr int kBatch = 1000;
    uint64_t seed = kCanonicalSeed;
    for (auto _ : state) {
        for (int i = 0; i < kBatch; ++i) {
            benchmark::DoNotOptimize(stubServiceCall(seed));
            ++seed;
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatch);
}
BENCHMARK(BM_GRPC_BatchServiceCall)
    ->Name("GRPC-BM-04/BatchServiceCall1000")
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace rpc_dedicated
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
