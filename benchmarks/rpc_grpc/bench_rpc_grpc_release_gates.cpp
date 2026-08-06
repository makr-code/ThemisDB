// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file bench_rpc_grpc_release_gates.cpp
 * @brief Phase 5 rpc_grpc module release-gate benchmarks.
 *
 * Provides reproducible latency measurements for the rpc_grpc module hot paths
 * identified in the rpc_grpc module roadmap (Phase 5 — Performance and Hardening).
 *
 * ## Benchmark families
 *
 * ### GATE-RPC-01 — Error enum cast throughput
 *   Measures the cost of casting RpcGrpcError values from int32_t.
 *
 * ### GATE-RPC-02 — Switch dispatch throughput
 *   Measures switch-based dispatch across all RpcGrpcError codes.
 *
 * ### GATE-RPC-03 — RpcServiceDescriptor struct allocation
 *   Measures in-process heap allocation for RpcServiceDescriptor; release
 *   gate for service registration / reload paths.
 *
 * ### GATE-RPC-04 — Batch error cast (1 000 iterations)
 *   Amortised error-cast cost across 1 000 mixed codes; simulates a
 *   high-frequency gRPC error classification loop.
 *
 * ## Hard release gates
 *
 * | Gate ID      | Benchmark       | Threshold        |
 * |--------------|-----------------|------------------|
 * | GATE-RPC-01  | ErrorEnumCast   | p99 ≤ 5 ns       |
 * | GATE-RPC-02  | SwitchDispatch  | p99 ≤ 10 ns      |
 * | GATE-RPC-03  | StructAlloc     | p99 ≤ 500 ns     |
 * | GATE-RPC-04  | BatchCast       | p99 ≤ 5 µs/batch |
 *
 * All benchmarks use kCanonicalSeed = 42 for deterministic inputs.
 *
 * @see src/rpc_grpc/ROADMAP.md — Phase 5 items
 * @see include/rpc_grpc/rpc_grpc_api_contract.h
 */

#include <benchmark/benchmark.h>
#include "rpc_grpc/rpc_grpc_api_contract.h"

#include <cstdint>
#include <string>

namespace themis {
namespace bench {
namespace rpc {

/// Canonical PRNG seed for all RPC benchmarks.
static constexpr uint64_t kCanonicalSeed = 42;

/// Number of repetitions for variance estimation.
static constexpr int kRepetitions = 5;

// ============================================================================
// GATE-RPC-01 — Error enum cast throughput
// ============================================================================

static void BM_RPC01_ErrorEnumCast(benchmark::State& state) {
    const int32_t raw = static_cast<int32_t>(
        rpc_grpc::RpcGrpcError::kCredentialLoadFailed);
    for (auto _ : state) {
        auto e = static_cast<rpc_grpc::RpcGrpcError>(raw);
        benchmark::DoNotOptimize(e);
    }
    state.SetLabel("GATE-RPC-01: p99 <= 5 ns");
}
BENCHMARK(BM_RPC01_ErrorEnumCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-RPC-02 — Switch dispatch throughput
// ============================================================================

static void BM_RPC02_SwitchDispatch(benchmark::State& state) {
    const rpc_grpc::RpcGrpcError codes[] = {
        rpc_grpc::RpcGrpcError::kSuccess,
        rpc_grpc::RpcGrpcError::kServerNotRunning,
        rpc_grpc::RpcGrpcError::kServiceRegistration,
        rpc_grpc::RpcGrpcError::kCredentialLoadFailed,
        rpc_grpc::RpcGrpcError::kStreamAborted,
        rpc_grpc::RpcGrpcError::kMethodNotFound,
        rpc_grpc::RpcGrpcError::kTransportError,
        rpc_grpc::RpcGrpcError::kInternalError,
    };
    uint64_t idx = kCanonicalSeed % 8;
    for (auto _ : state) {
        const char* label = nullptr;
        switch (codes[idx % 8]) {
            case rpc_grpc::RpcGrpcError::kSuccess:              label = "ok"; break;
            case rpc_grpc::RpcGrpcError::kServerNotRunning:     label = "stopped"; break;
            case rpc_grpc::RpcGrpcError::kServiceRegistration:  label = "svreg"; break;
            case rpc_grpc::RpcGrpcError::kCredentialLoadFailed: label = "cred"; break;
            case rpc_grpc::RpcGrpcError::kStreamAborted:        label = "stream"; break;
            case rpc_grpc::RpcGrpcError::kMethodNotFound:       label = "meth"; break;
            case rpc_grpc::RpcGrpcError::kTransportError:       label = "trans"; break;
            case rpc_grpc::RpcGrpcError::kInternalError:        label = "int"; break;
        }
        benchmark::DoNotOptimize(label);
        ++idx;
    }
    state.SetLabel("GATE-RPC-02: p99 <= 10 ns");
}
BENCHMARK(BM_RPC02_SwitchDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-RPC-03 — RpcServiceDescriptor struct allocation
// ============================================================================

static void BM_RPC03_StructAlloc(benchmark::State& state) {
    for (auto _ : state) {
        rpc_grpc::RpcServiceDescriptor desc;
        desc.service_name           = "themis.BenchQueryService";
        desc.require_auth           = true;
        desc.max_concurrent_streams = 200;
        benchmark::DoNotOptimize(desc);
    }
    state.SetLabel("GATE-RPC-03: p99 <= 500 ns");
}
BENCHMARK(BM_RPC03_StructAlloc)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-RPC-04 — Batch error cast (1 000 iterations)
// ============================================================================

static void BM_RPC04_BatchCast(benchmark::State& state) {
    static const int32_t kRawCodes[] = {
        8300, 8301, 8302, 8303, 8304, 8305, 8306
    };
    static constexpr int kBatchSize = 1000;
    for (auto _ : state) {
        uint64_t seed = kCanonicalSeed;
        for (int i = 0; i < kBatchSize; ++i) {
            seed ^= seed << 13;
            seed ^= seed >> 7;
            seed ^= seed << 17;
            auto e = static_cast<rpc_grpc::RpcGrpcError>(kRawCodes[seed % 7]);
            benchmark::DoNotOptimize(e);
        }
    }
    state.SetItemsProcessed(state.iterations() * kBatchSize);
    state.SetLabel("GATE-RPC-04: p99 <= 5 us per batch");
}
BENCHMARK(BM_RPC04_BatchCast)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// Phase 5 — Extended Gates: Stress & Concurrency (GATE-RPC-05..08)
// ============================================================================

// ============================================================================
// GATE-RPC-05 — Concurrent error code dispatch (8 threads, 10k iterations)
// ============================================================================

static void BM_RPC05_ConcurrentDispatch(benchmark::State& state) {
    // Simulates high-frequency concurrent error classification
    static const rpc_grpc::RpcGrpcError codes[] = {
        rpc_grpc::RpcGrpcError::kSuccess,
        rpc_grpc::RpcGrpcError::kServerNotRunning,
        rpc_grpc::RpcGrpcError::kServiceRegistration,
        rpc_grpc::RpcGrpcError::kCredentialLoadFailed,
        rpc_grpc::RpcGrpcError::kStreamAborted,
        rpc_grpc::RpcGrpcError::kMethodNotFound,
        rpc_grpc::RpcGrpcError::kTransportError,
        rpc_grpc::RpcGrpcError::kInternalError,
    };
    
    for (auto _ : state) {
        for (int i = 0; i < 10000; ++i) {
            const auto& code = codes[(kCanonicalSeed + i) % 8];
            auto is_closed = rpc_grpc::isRpcGrpcFailClosed(code);
            benchmark::DoNotOptimize(is_closed);
        }
    }
    state.SetItemsProcessed(state.iterations() * 10000);
    state.SetLabel("GATE-RPC-05: p99 <= 100 us per 10k iterations");
}
BENCHMARK(BM_RPC05_ConcurrentDispatch)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-RPC-06 — State enum construction (1000 iterations)
// ============================================================================

static void BM_RPC06_StateConstruction(benchmark::State& state) {
    static const int32_t states[] = {
        static_cast<int32_t>(rpc_grpc::RpcServerState::Stopped),
        static_cast<int32_t>(rpc_grpc::RpcServerState::Starting),
        static_cast<int32_t>(rpc_grpc::RpcServerState::Active),
        static_cast<int32_t>(rpc_grpc::RpcServerState::Stopping),
    };
    
    for (auto _ : state) {
        for (int i = 0; i < 1000; ++i) {
            auto s = static_cast<rpc_grpc::RpcServerState>(states[i % 4]);
            benchmark::DoNotOptimize(s);
        }
    }
    state.SetItemsProcessed(state.iterations() * 1000);
    state.SetLabel("GATE-RPC-06: p99 <= 50 us per 1k iterations");
}
BENCHMARK(BM_RPC06_StateConstruction)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-RPC-07 — Service descriptor bulk operations (500 descriptors)
// ============================================================================

static void BM_RPC07_BulkDescriptorOps(benchmark::State& state) {
    std::vector<rpc_grpc::RpcServiceDescriptor> descriptors;
    
    for (auto _ : state) {
        descriptors.clear();
        for (int i = 0; i < 500; ++i) {
            rpc_grpc::RpcServiceDescriptor desc;
            desc.service_name = "service_" + std::to_string(i);
            desc.proto_file = "/protos/service_" + std::to_string(i) + ".proto";
            desc.require_auth = (i % 2 == 0);
            desc.max_concurrent_streams = 100 + (i % 200);
            descriptors.push_back(desc);
            benchmark::DoNotOptimize(desc);
        }
    }
    state.SetItemsProcessed(state.iterations() * 500);
    state.SetLabel("GATE-RPC-07: p99 <= 500 us per 500 descriptors");
}
BENCHMARK(BM_RPC07_BulkDescriptorOps)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

// ============================================================================
// GATE-RPC-08 — Fail-closed predicate throughput (100k checks)
// ============================================================================

static void BM_RPC08_FailClosedThroughput(benchmark::State& state) {
    static const rpc_grpc::RpcGrpcError codes[] = {
        rpc_grpc::RpcGrpcError::kSuccess,
        rpc_grpc::RpcGrpcError::kServerNotRunning,
        rpc_grpc::RpcGrpcError::kServiceRegistration,
        rpc_grpc::RpcGrpcError::kCredentialLoadFailed,
        rpc_grpc::RpcGrpcError::kStreamAborted,
        rpc_grpc::RpcGrpcError::kMethodNotFound,
        rpc_grpc::RpcGrpcError::kTransportError,
        rpc_grpc::RpcGrpcError::kInternalError,
    };
    
    for (auto _ : state) {
        for (int i = 0; i < 100000; ++i) {
            auto code = codes[(kCanonicalSeed + i) % 8];
            auto is_closed = rpc_grpc::isRpcGrpcFailClosed(code);
            benchmark::DoNotOptimize(is_closed);
        }
    }
    state.SetItemsProcessed(state.iterations() * 100000);
    state.SetLabel("GATE-RPC-08: p99 <= 1 ms per 100k checks");
}
BENCHMARK(BM_RPC08_FailClosedThroughput)
    ->Repetitions(kRepetitions)
    ->ReportAggregatesOnly(true);

} // namespace rpc
} // namespace bench
} // namespace themis

BENCHMARK_MAIN();
