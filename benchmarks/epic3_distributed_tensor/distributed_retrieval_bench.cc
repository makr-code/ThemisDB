// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

namespace {

void BM_Epic3DistributedRetrieval(benchmark::State& state) {
  const auto shard_count = static_cast<uint32_t>(state.range(0));
  const auto degraded_inputs = state.range(1) != 0;

  auto manifest = themis::bench::epic::make_manifest(
      shard_count, 8ULL * 1024 * 1024,
      themis::distributed_tensor::ArtifactClass::PRIMARY,
      degraded_inputs
          ? themis::distributed_tensor::ArtifactLifecycleStage::RECOVERING
          : themis::distributed_tensor::ArtifactLifecycleStage::ACTIVE);
  themis::bench::epic::attach_integrity_receipt(manifest, true);

  const auto dependencies =
      degraded_inputs
          ? themis::bench::epic::make_dependencies(true, true,
                                                   themis::distributed_tensor::
                                                       ArtifactLifecycleStage::ACTIVE,
                                                   60)
          : themis::bench::epic::make_dependencies();

  themis::distributed_tensor::DefaultDistributedTensorPlanner planner;
  for (auto _ : state) {
    auto plan = planner.plan_tensor_retrieval(
        manifest, dependencies,
        themis::distributed_tensor::RetrievalLocation::ANY_TIER);
    auto optimized = planner.optimize_retrieval_plan(plan);
    benchmark::DoNotOptimize(plan);
    benchmark::DoNotOptimize(optimized);
  }

  state.SetItemsProcessed(state.iterations() * shard_count);
}

BENCHMARK(BM_Epic3DistributedRetrieval)
    ->ArgsProduct({{8, 64, 256}, {0, 1}})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
