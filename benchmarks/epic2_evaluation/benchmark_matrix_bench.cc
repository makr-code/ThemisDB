// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

namespace {

void BM_Epic2BenchmarkMatrixPipeline(benchmark::State& state) {
  const auto shard_count = static_cast<uint32_t>(state.range(0));
  const auto node_count = static_cast<uint32_t>(state.range(1));

  const auto nodes =
      themis::bench::epic::make_nodes(node_count, 512ULL * 1024 * 1024);
  themis::distributed_tensor::DefaultShardPlacementStrategy placement_strategy;
  themis::distributed_tensor::DefaultIntegrityVerificationEngine integrity_engine;
  themis::distributed_tensor::DefaultDistributedTensorPlanner planner;
  themis::distributed_tensor::DefaultRecoveryManager recovery_manager;

  for (auto _ : state) {
    auto placement = placement_strategy.compute_placement(
        "bench-artifact", shard_count, 8ULL * 1024 * 1024, nodes);
    auto manifest = themis::bench::epic::make_manifest_from_plan(placement);
    auto receipt = integrity_engine.compute_verification(manifest);
    auto dependencies = themis::bench::epic::make_dependencies();
    auto retrieval_plan = planner.plan_tensor_retrieval(
        manifest, dependencies,
        themis::distributed_tensor::RetrievalLocation::ANY_TIER);
    auto recovery_plan = recovery_manager.create_recovery_plan(
        manifest, {manifest.shard_placements().front().shard_id});

    benchmark::DoNotOptimize(placement);
    benchmark::DoNotOptimize(receipt);
    benchmark::DoNotOptimize(retrieval_plan);
    benchmark::DoNotOptimize(recovery_plan);
    benchmark::ClobberMemory();
  }

  state.SetItemsProcessed(state.iterations() * shard_count);
  state.counters["nodes"] = static_cast<double>(node_count);
}

BENCHMARK(BM_Epic2BenchmarkMatrixPipeline)
    ->ArgsProduct({{8, 32, 128}, {3, 8, 16}})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
