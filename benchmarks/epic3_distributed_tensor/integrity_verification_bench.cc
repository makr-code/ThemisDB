// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include <benchmark/benchmark.h>

#include "epic_benchmark_scenarios.h"

namespace {

void BM_Epic3IntegrityVerification(benchmark::State& state) {
  const auto shard_count = static_cast<uint32_t>(state.range(0));
  auto manifest = themis::bench::epic::make_manifest(
      shard_count, 2ULL * 1024 * 1024,
      themis::distributed_tensor::ArtifactClass::PRIMARY,
      themis::distributed_tensor::ArtifactLifecycleStage::ACTIVE);
  themis::distributed_tensor::DefaultIntegrityVerificationEngine engine;

  for (auto _ : state) {
    auto receipt = engine.compute_verification(manifest);
    auto verified = engine.verify_integrity(manifest, receipt);
    benchmark::DoNotOptimize(receipt);
    benchmark::DoNotOptimize(verified);
  }

  state.SetItemsProcessed(state.iterations() * shard_count);
}

BENCHMARK(BM_Epic3IntegrityVerification)
    ->Arg(4)
    ->Arg(32)
    ->Arg(128)
    ->Unit(benchmark::kMicrosecond);

}  // namespace
