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
