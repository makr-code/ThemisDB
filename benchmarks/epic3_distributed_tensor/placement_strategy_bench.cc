// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

namespace {

void BM_Epic3PlacementStrategy(benchmark::State& state) {
  const auto shard_count = static_cast<uint32_t>(state.range(0));
  const auto node_count = static_cast<uint32_t>(state.range(1));
  const auto nodes =
      themis::bench::epic::make_nodes(node_count, 768ULL * 1024 * 1024);

  themis::distributed_tensor::DefaultShardPlacementStrategy strategy;
  for (auto _ : state) {
    auto plan = strategy.compute_placement(
        "bench-artifact", shard_count, 8ULL * 1024 * 1024, nodes);
    auto valid = strategy.validate_placement(
        plan, themis::distributed_tensor::PlacementConstraint::NONE);
    benchmark::DoNotOptimize(plan);
    benchmark::DoNotOptimize(valid);
  }

  state.SetItemsProcessed(state.iterations() * shard_count);
  state.counters["nodes"] = static_cast<double>(node_count);
}

BENCHMARK(BM_Epic3PlacementStrategy)
    ->ArgsProduct({{8, 64, 256}, {3, 8, 16}})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
