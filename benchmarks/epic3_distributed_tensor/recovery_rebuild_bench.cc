// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

#include <vector>

namespace {

void BM_Epic3RecoveryRebuildPlan(benchmark::State& state) {
  const auto shard_count = static_cast<uint32_t>(state.range(0));
  const auto failed_shards = static_cast<uint32_t>(state.range(1));

  auto manifest = themis::bench::epic::make_manifest(
      shard_count, 4ULL * 1024 * 1024,
      themis::distributed_tensor::ArtifactClass::DERIVED,
      themis::distributed_tensor::ArtifactLifecycleStage::STALE);
  themis::distributed_tensor::DefaultRecoveryManager manager;

  std::vector<std::string> failed_ids;
  failed_ids.reserve(failed_shards);
  for (uint32_t index = 0; index < failed_shards; ++index) {
    failed_ids.push_back("bench-artifact:shard:" + std::to_string(index));
  }

  for (auto _ : state) {
    auto plan = manager.create_recovery_plan(manifest, failed_ids);
    benchmark::DoNotOptimize(plan);
  }

  state.SetItemsProcessed(state.iterations() * failed_shards);
}

BENCHMARK(BM_Epic3RecoveryRebuildPlan)
    ->Args({16, 1})
    ->Args({16, 4})
    ->Args({64, 8})
    ->Unit(benchmark::kMicrosecond);

}  // namespace
