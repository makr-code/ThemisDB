// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

namespace {

void BM_Epic2StorageStrategySelection(benchmark::State& state) {
  const auto node_count = static_cast<uint32_t>(state.range(0));
  const auto require_accelerator = state.range(1) != 0;
  const auto nodes = themis::bench::epic::make_nodes(
      node_count, 1024ULL * 1024 * 1024, require_accelerator);

  themis::distributed_tensor::DefaultShardPlacementStrategy strategy;
  const auto constraint =
      require_accelerator
          ? themis::distributed_tensor::PlacementConstraint::ACCELERATOR_REQUIRED
          : themis::distributed_tensor::PlacementConstraint::NONE;

  for (auto _ : state) {
    auto plan = strategy.compute_placement("bench-artifact", 32,
                                           16ULL * 1024 * 1024, nodes,
                                           std::nullopt, constraint);
    auto valid = strategy.validate_placement(plan, constraint);
    auto optimized = strategy.optimize_placement(plan, nodes);

    benchmark::DoNotOptimize(plan);
    benchmark::DoNotOptimize(valid);
    benchmark::DoNotOptimize(optimized);
  }

  state.SetItemsProcessed(state.iterations() * 32);
  state.counters["nodes"] = static_cast<double>(node_count);
}

BENCHMARK(BM_Epic2StorageStrategySelection)
    ->ArgsProduct({{4, 12, 24}, {0, 1}})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
