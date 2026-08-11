// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/// @file bench_tensor_cpu_gpu_breakeven.cc
/// @brief Phase D: CPU/GPU break-even validation for distributed tensor operations.
///
/// ## Purpose
///
/// This benchmark measures pure-CPU baseline latency for the tensor operations
/// used in the distributed tensor pipeline (summary scoring, partial-refit
/// dispatch decision, exact-fetch dispatch decision).  It establishes the
/// CPU baseline against which GPU-acceleration benefit can be measured once
/// GPU hardware is available.
///
/// ### Design Notes (Phase D)
/// - No unconditional GPU dependency: all benchmarks run on CPU only.
/// - GPU paths are gated behind THEMIS_ENABLE_CUDA; absent that flag the
///   benchmark records CPU timings as break-even baselines.
/// - The break-even point is declared when GPU speedup >= 4x over CPU baseline
///   at the relevant input size (defined in ROADMAP.md Phase D).
///
/// ### Benchmark IDs
///   BGPU-01  CPU baseline: scoring N shard summaries (vectorised)
///   BGPU-02  CPU baseline: partial-refit decision for N delta entries
///   BGPU-03  CPU baseline: freshness consensus check for N shards
///   BGPU-04  CPU baseline: exact-fetch dispatch loop (round-trips counted)
///
/// ### Break-Even Gate (Phase D, Target 2027)
///   - GPU speedup >= 4x for BGPU-01 at N=1024
///   - No unconditional GPU dependency on any Phase C tensor path

#include <benchmark/benchmark.h>

#include "shard_summary_coordinator.h"
#include "snapshot_update_worker.h"
#include "tensor/tensor_summary_types.h"

#include <cstdint>
#include <memory>
#include <numeric>
#include <string>
#include <vector>

namespace {

using namespace themis::distributed_tensor;
using namespace themis::tensor;

constexpr uint64_t kCanonicalRngSeed = 42u;

// Fixed reference time so TTL expiry never interferes.
constexpr int64_t kRefNowMs = 9'000'000'000'000LL;
constexpr int64_t kRecentMs = kRefNowMs - 30'000LL; // 30 s ago

// ── Helpers ──────────────────────────────────────────────────────────────────

/// Build a coordinator with N freshly-refreshed shards (CPU path only).
ShardSummaryCoordinator makeFreshCoordinator(int n) {
    ShardSummaryCoordinator c;
    for (int i = 0; i < n; ++i) {
        const std::string sid = "s-" + std::to_string(i);
        c.registerShard(sid);
        ShardSummary s;
        s.shard_id = sid;
        s.shard_relevance =
            0.5f + static_cast<float>(i % 10) * 0.04f;
        s.freshness_state = SummaryFreshnessState::FRESH;
        s.freshness_ttl_seconds = 3600;
        c.refreshShard(sid, s, kRecentMs);
    }
    return c;
}

std::vector<ShardSummary> makeSummaries(int n) {
    std::vector<ShardSummary> v;
    v.reserve(n);
    for (int i = 0; i < n; ++i) {
        ShardSummary s;
        s.shard_id = "s-" + std::to_string(i);
        s.shard_relevance = 0.5f + static_cast<float>(i % 10) * 0.04f;
        s.shard_healthy = true;
        s.freshness_state = SummaryFreshnessState::FRESH;
        s.freshness_ttl_seconds = 3600;
        v.push_back(std::move(s));
    }
    return v;
}

DeltaWindow makeDeltaWindow(int n) {
    DeltaWindow w;
    w.artifact_id = "art-bench";
    w.sequence_start = 1;
    w.sequence_end = static_cast<uint64_t>(n);
    w.extracted_at_ms = 1;
    for (int i = 0; i < n; ++i) {
        DeltaLogEntry e;
        e.sequence_number = static_cast<uint64_t>(i + 1);
        e.mutation_type = (i % 3 == 0) ? DeltaMutationType::INSERT
                                        : DeltaMutationType::UPDATE;
        e.affected_entity_id = "node-" + std::to_string(i);
        e.recorded_at_ms = kRecentMs + i;
        e.source_transaction_id = "tx-" + std::to_string(i);
        e.payload_size_bytes = 256;
        w.total_payload_size_bytes += e.payload_size_bytes;
        w.entries.push_back(std::move(e));
    }
    return w;
}

// ── BGPU-01: CPU baseline — scoring N shard summaries ────────────────────────

void BGPU01_ShardScoringCPU(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeFreshCoordinator(n);
    const auto summaries = makeSummaries(n);
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto decisions =
            c.routeSummaryFirst(summaries, AccuracyMode::ADVISORY, kRefNowMs);
        // Accumulate to prevent dead-code elimination.
        float acc = 0.0f;
        for (const auto& d : decisions) {
            acc += d.advisory_score;
        }
        benchmark::DoNotOptimize(acc);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BGPU-01 cpu-baseline shard-scoring");
}
BENCHMARK(BGPU01_ShardScoringCPU)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BGPU-02: CPU baseline — partial-refit decision for N delta entries ────────

void BGPU02_PartialRefitDecisionCPU(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    SnapshotBasedUpdateWorker worker;
    // Use a moderate artifact size so decision covers PATCH and PARTIAL_REFIT
    // ranges across the Arg values.
    constexpr uint64_t kArtifactBytes = 256 * 1024; // 256 KiB
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto window = makeDeltaWindow(n);
        const auto decision =
            worker.decideUpdateStrategy(window, kArtifactBytes, /*residual=*/0.04);
        benchmark::DoNotOptimize(decision);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BGPU-02 cpu-baseline refit-decision");
}
BENCHMARK(BGPU02_PartialRefitDecisionCPU)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BGPU-03: CPU baseline — freshness consensus check ────────────────────────

void BGPU03_FreshnessConsensusCPU(benchmark::State& state) {
    const int n = static_cast<int>(state.range(0));
    auto c = makeFreshCoordinator(n);

    std::vector<std::string> shard_ids;
    shard_ids.reserve(n);
    for (int i = 0; i < n; ++i) {
        shard_ids.push_back("s-" + std::to_string(i));
    }
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    for (auto _ : state) {
        const auto res = c.checkFreshnessConsensus(shard_ids, kRefNowMs);
        benchmark::DoNotOptimize(res);
    }

    state.SetItemsProcessed(state.iterations() * n);
    state.SetLabel("BGPU-03 cpu-baseline consensus");
}
BENCHMARK(BGPU03_FreshnessConsensusCPU)
    ->Arg(16)
    ->Arg(64)
    ->Arg(256)
    ->Arg(1024)
    ->Unit(benchmark::kMicrosecond)
    ->UseRealTime();

// ── BGPU-04: CPU baseline — exact-fetch dispatch loop (no GPU) ───────────────

void BGPU04_ExactFetchDispatchCPU(benchmark::State& state) {
    // No fetcher → measures coordinator overhead only (no I/O).
    ShardSummaryCoordinator c;
    benchmark::DoNotOptimize(kCanonicalRngSeed);

    ExactFetchRequest req;
    req.shard_id = "s-0";
    req.artifact_id = "art-bench";
    req.timeout_ms = 5000;

    for (auto _ : state) {
        const auto result = c.fetchExact(req);
        benchmark::DoNotOptimize(result);
    }

    state.SetItemsProcessed(state.iterations());
    state.SetLabel("BGPU-04 cpu-baseline exact-dispatch (no-fetcher overhead)");
}
BENCHMARK(BGPU04_ExactFetchDispatchCPU)
    ->Iterations(50000)
    ->Unit(benchmark::kNanosecond)
    ->UseRealTime();

} // namespace
