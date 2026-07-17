<<<<<<< HEAD
/**
 * @file planner_decision_bench.cc
 * @brief Benchmarks for EPIC 2.5 DefaultQueryPlanner path-selection overhead.
 *
 * Measures the wall-clock cost of @ref QueryPlanner::selectPath() across all
 * five canonical execution paths and representative fallback scenarios. The goal
 * is to confirm that planner decision overhead stays well below the 5-second
 * SLA tail-latency budget mandated by ADR E2-003.
 *
 * ## What this measures
 * - **Planner-only latency**: time inside `selectPath()`, excluding downstream
 *   ANN/graph/tensor retrieval. This must be sub-microsecond on modern hardware.
 * - **Path-selection distribution**: how often each path is selected under
 *   representative eligibility mixes (useful for capacity planning).
 * - **Module gap threshold monitoring**: counts how many decisions were blocked
 *   by `FallbackReason::ModuleGapThreshold` to track gap-fix progress impact.
 *
 * ## Build
 *   cmake --preset linux-release -DTHEMIS_BUILD_EPIC2=ON
 *   cmake --build --preset linux-release --target planner_decision_bench
 *
 * @see src/evaluation/include/query_planner.h
 * @see docs/EPIC2_QUERY_PLANNER.md
 * @see docs/adr/adr-e2-003-query-planner-routing-model.md
 */

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <initializer_list>
#include <memory>
#include <string>

#include "include/query_planner.h"

using namespace themis::evaluation;

// ---------------------------------------------------------------------------
// Benchmark helpers
// ---------------------------------------------------------------------------

namespace {

/// @brief Canonical fully-enabled eligibility (allows Path 1).
ExecutionEligibility fullEligibility() {
    ExecutionEligibility e;
    e.cuda_available              = true;
    e.ann_enabled                 = true;
    e.gpu_error_handling_gate     = true;
    e.gpu_parity_validated        = true;
    e.force_exact                 = false;
    e.force_cpu                   = false;
    e.query_thread_safety_ok      = true;
    e.query_exception_handling_ok = true;
    e.index_buffer_safety_ok      = true;
    e.distributed_multi_shard     = false;
    e.shard_manifests_available   = false;
    return e;
}

/// @brief Default policy configuration.
PlannerConfig defaultConfig() {
    PlannerConfig cfg;
    cfg.max_staleness_ms             = 5'000;
    cfg.max_delta_lag                = 1'000;
    cfg.min_residual_threshold       = 0.95;
    cfg.max_rank_cap                 = 1'000;
    cfg.min_shard_summary_confidence = 0.80;
    cfg.policy_version               = "v1-bench";
    cfg.staleness_threshold_key      = "tensor.max_staleness_ms";
    return cfg;
}

/// @brief No artifact present (zero-value sentinel — triggers Path 1).
TensorArtifactFreshness noArtifact() {
    return TensorArtifactFreshness{};
}

/// @brief Fresh artifact that passes all freshness gates (triggers Path 2).
TensorArtifactFreshness freshArtifact() {
    TensorArtifactFreshness f;
    f.artifact_age_ms     = 1'000;
    f.delta_lag           = 100;
    f.residual_threshold  = 0.97;
    f.rank_cap            = 500;
    f.rebuild_in_progress = false;
    return f;
}

/// @brief Stale artifact (age exceeds max_staleness_ms — triggers Path 4).
TensorArtifactFreshness staleArtifact() {
    TensorArtifactFreshness f;
    f.artifact_age_ms     = 10'000; // > 5 000 ms → stale
    f.delta_lag           = 100;
    f.residual_threshold  = 0.97;
    f.rank_cap            = 500;
    f.rebuild_in_progress = false;
    return f;
}

/**
 * @brief Observe and count planner decisions by path for gap-threshold monitoring.
 *
 * Records the last decision path and a per-category count useful for detecting
 * regressions in module gap thresholds over time.
 */
class CountingObserver final : public PlannerObserver {
public:
    void onDecision(const PlannerDecision& d, uint64_t /*latency_us*/) noexcept override {
        counts[static_cast<int>(d.path)]++;
        if (d.fallback_reason == FallbackReason::ModuleGapThreshold) {
            gap_threshold_blocks++;
        }
    }

    int counts[6]{};               ///< Index = ExecutionPath numeric value (1-5).
    int gap_threshold_blocks{0};   ///< Decisions blocked by module gap threshold.
};

/**
 * @brief Run `selectPath()` N times and return average latency in nanoseconds.
 *
 * @param planner  The planner under test.
 * @param e        Eligibility signals for the scenario.
 * @param f        Tensor artifact freshness.
 * @param c        Policy configuration.
 * @param N        Number of iterations.
 * @return         Average wall-clock time per call in nanoseconds.
 */
double benchmarkPath(
    QueryPlanner&                  planner,
    const ExecutionEligibility&    e,
    const TensorArtifactFreshness& f,
    const PlannerConfig&           c,
    int                            N)
{
    const auto t0 = std::chrono::steady_clock::now();
    for (int i = 0; i < N; ++i) {
        // Volatile sink prevents the compiler from optimising away the call.
        volatile auto d = planner.selectPath(e, f, c);
        (void)d;
    }
    const auto t1 = std::chrono::steady_clock::now();
    const double total_ns = static_cast<double>(
        std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count());
    return total_ns / static_cast<double>(N);
}

} // namespace

// ---------------------------------------------------------------------------
// Main — benchmark runner
// ---------------------------------------------------------------------------

/**
 * @brief Entry point for the planner decision benchmark suite.
 *
 * Prints per-path average latency in nanoseconds and a gap-threshold block count
 * to stdout. Intended to be run manually or from a CI performance regression check.
 *
 * Exit code 0 on success; 1 if any measured latency exceeds the 10 µs soft
 * threshold (indicating a serious regression in planner overhead).
 *
 * @return 0 on success, 1 on latency regression.
 */
int main() {
    constexpr int N        = 100'000;
    constexpr double SOFT_LIMIT_NS = 10'000.0; // 10 µs — soft regression threshold

    CountingObserver obs;
    auto planner = makeDefaultQueryPlanner(&obs);

    const auto e_full   = fullEligibility();
    const auto f_none   = noArtifact();
    const auto f_fresh  = freshArtifact();
    const auto f_stale  = staleArtifact();
    const auto cfg      = defaultConfig();

    // --- Path 1: ANN Only ---
    const double p1_ns = benchmarkPath(*planner, e_full, f_none, cfg, N);
    std::printf("[Path 1 — ANN Only]             avg %.1f ns / call\n", p1_ns);

    // --- Path 2: ANN + Tensor Summary ---
    const double p2_ns = benchmarkPath(*planner, e_full, f_fresh, cfg, N);
    std::printf("[Path 2 — ANN + Tensor Summary] avg %.1f ns / call\n", p2_ns);

    // --- Path 4 via stale tensor ---
    const double p4_stale_ns = benchmarkPath(*planner, e_full, f_stale, cfg, N);
    std::printf("[Path 4 — Stale Tensor]         avg %.1f ns / call\n", p4_stale_ns);

    // --- Path 4 via force_exact ---
    {
        auto e_exact = e_full;
        e_exact.force_exact = true;
        const double p4_exact_ns = benchmarkPath(*planner, e_exact, f_none, cfg, N);
        std::printf("[Path 4 — force_exact]          avg %.1f ns / call\n", p4_exact_ns);
    }

    // --- Path 4 via module gap threshold ---
    {
        auto e_gap = e_full;
        e_gap.index_buffer_safety_ok = false;
        const double p4_gap_ns = benchmarkPath(*planner, e_gap, f_none, cfg, N);
        std::printf("[Path 4 — ModuleGapThreshold]   avg %.1f ns / call (blocks: %d)\n",
                    p4_gap_ns, obs.gap_threshold_blocks);
    }

    // --- Path 5: Distributed ---
    {
        auto e_dist = e_full;
        e_dist.distributed_multi_shard   = true;
        e_dist.shard_manifests_available = true;
        const double p5_ns = benchmarkPath(*planner, e_dist, f_none, cfg, N);
        std::printf("[Path 5 — Distributed]          avg %.1f ns / call\n", p5_ns);
    }

    std::printf("\nObserver call count: %d\n",
                obs.counts[1] + obs.counts[2] + obs.counts[3] + obs.counts[4] + obs.counts[5]);
    std::printf("Path distribution: P1=%d P2=%d P3=%d P4=%d P5=%d\n",
                obs.counts[1], obs.counts[2], obs.counts[3], obs.counts[4], obs.counts[5]);
    std::printf("ModuleGapThreshold blocks: %d\n", obs.gap_threshold_blocks);

    // Soft regression check: all paths must be < SOFT_LIMIT_NS.
    const double max_ns = std::max(std::max(p1_ns, p2_ns), p4_stale_ns);
    if (max_ns > SOFT_LIMIT_NS) {
        std::printf("\nREGRESSION: max latency %.1f ns exceeds soft threshold %.0f ns\n",
                    max_ns, SOFT_LIMIT_NS);
        return 1;
    }

    std::printf("\nAll paths within %.0f ns soft threshold — PASS\n", SOFT_LIMIT_NS);
    return 0;
}
=======
// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

namespace {

void BM_Epic2PlannerDecisionCost(benchmark::State& state) {
  const auto shard_count = static_cast<uint32_t>(state.range(0));
  const auto lifecycle_selector = static_cast<int>(state.range(1));

  auto lifecycle_stage =
      lifecycle_selector == 0
          ? themis::distributed_tensor::ArtifactLifecycleStage::ACTIVE
          : themis::distributed_tensor::ArtifactLifecycleStage::STALE;
  auto manifest = themis::bench::epic::make_manifest(
      shard_count, 4ULL * 1024 * 1024,
      themis::distributed_tensor::ArtifactClass::PRIMARY, lifecycle_stage);
  themis::bench::epic::attach_integrity_receipt(manifest, true);

  const auto dependencies =
      lifecycle_selector == 0
          ? themis::bench::epic::make_dependencies(false, false)
          : themis::bench::epic::make_dependencies(true, true,
                                                   themis::distributed_tensor::
                                                       ArtifactLifecycleStage::ACTIVE,
                                                   30);

  themis::distributed_tensor::DefaultDistributedTensorPlanner planner;
  for (auto _ : state) {
    auto plan = planner.plan_tensor_retrieval(
        manifest, dependencies,
        themis::distributed_tensor::RetrievalLocation::ANY_TIER);
    auto optimized = planner.optimize_retrieval_plan(plan);
    auto cost = planner.estimate_retrieval_cost(
        manifest, optimized.retrieval_strategy, optimized.retrieval_location);

    benchmark::DoNotOptimize(plan);
    benchmark::DoNotOptimize(optimized);
    benchmark::DoNotOptimize(cost);
  }

  state.SetItemsProcessed(state.iterations() * shard_count);
  state.counters["degraded_inputs"] = lifecycle_selector;
}

BENCHMARK(BM_Epic2PlannerDecisionCost)
    ->ArgsProduct({{4, 32, 256}, {0, 1}})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
>>>>>>> origin/develop
