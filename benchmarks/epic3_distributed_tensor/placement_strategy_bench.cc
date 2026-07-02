/// @file placement_strategy_bench.cc
/// @brief Benchmarks for factorization-aware shard placement strategies
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02

#include <benchmark/benchmark.h>
#include "src/distributed_tensor/include/shard_placement.h"

namespace themis {
namespace distributed_tensor {
namespace {

/// @brief Create a hardware profile with N shards for benchmarking
HardwareProfile create_benchmark_hardware(int num_shards) {
  HardwareProfile hardware;
  hardware.replication_factor = 2;
  hardware.enable_factorization_aware = true;

  for (int i = 0; i < num_shards; ++i) {
    ShardDescriptor shard;
    shard.shard_id = "shard_" + std::to_string(i);
    shard.capacity_bytes = 10ULL * 1024 * 1024 * 1024;  // 10 GB each
    shard.used_bytes = (2ULL * 1024 * 1024 * 1024) + (i * 100 * 1024 * 1024);  // Varied usage
    shard.latency_ms = 10 + (i % 5) * 5;
    shard.tier = (i % 3 == 0) ? "GPU" : ((i % 3 == 1) ? "CPU" : "NVRAM");
    shard.is_healthy = (i % 10 != 9);  // One in ten degraded
    shard.reliability_score = 0.90f + (i % 10) * 0.01f;
    hardware.shards.push_back(shard);
  }

  return hardware;
}

/// @brief Create test artifact manifests
ArtifactManifest create_benchmark_artifact(
    uint64_t size_bytes = 500 * 1024 * 1024,
    const std::string& semantic_hint = "TT_CORE") {
  ArtifactManifest manifest;
  manifest.artifact_id = "benchmark_artifact";
  manifest.size_bytes = size_bytes;
  manifest.artifact_class = ArtifactClass::PRIMARY;
  manifest.semantic_hint = semantic_hint;
  manifest.replication_factor = 2;
  manifest.version = "1.0.0";
  return manifest;
}

// ============================================================================
// Benchmark: Round-Robin Strategy
// ============================================================================

static void BM_RoundRobinPlacement_4Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(4);
  RoundRobinPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_RoundRobinPlacement_4Shards);

static void BM_RoundRobinPlacement_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  RoundRobinPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_RoundRobinPlacement_16Shards);

static void BM_RoundRobinPlacement_64Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(64);
  RoundRobinPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_RoundRobinPlacement_64Shards);

// ============================================================================
// Benchmark: Factorized Strategy
// ============================================================================

static void BM_FactorizedPlacement_4Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(4);
  FactorizedPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_FactorizedPlacement_4Shards);

static void BM_FactorizedPlacement_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  FactorizedPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_FactorizedPlacement_16Shards);

static void BM_FactorizedPlacement_64Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(64);
  FactorizedPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_FactorizedPlacement_64Shards);

// ============================================================================
// Benchmark: Cost-Aware Strategy
// ============================================================================

static void BM_CostAwarePlacement_4Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(4);
  CostAwarePlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_CostAwarePlacement_4Shards);

static void BM_CostAwarePlacement_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  CostAwarePlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_CostAwarePlacement_16Shards);

static void BM_CostAwarePlacement_64Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(64);
  CostAwarePlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_CostAwarePlacement_64Shards);

// ============================================================================
// Benchmark: Balanced Strategy
// ============================================================================

static void BM_BalancedPlacement_4Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(4);
  BalancedPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_BalancedPlacement_4Shards);

static void BM_BalancedPlacement_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  BalancedPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_BalancedPlacement_16Shards);

static void BM_BalancedPlacement_64Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(64);
  BalancedPlacementStrategy strategy;
  PlacementConfig config;
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = strategy.place(manifest, hardware, config, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_BalancedPlacement_64Shards);

// ============================================================================
// Benchmark: Factory and Strategy Switching
// ============================================================================

static void BM_FactoryCreation(benchmark::State& state) {
  for (auto _ : state) {
    auto strategies = PlacementStrategyFactory::create_all_strategies();
    benchmark::DoNotOptimize(strategies);
  }
}
BENCHMARK(BM_FactoryCreation);

// ============================================================================
// Benchmark: Coordinator Operations
// ============================================================================

static void BM_CoordinatorPlacement_4Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(4);
  ShardPlacementCoordinator coordinator(hardware);
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    auto result = coordinator.place_artifact(manifest, nullptr);
    benchmark::DoNotOptimize(result);
  }
}
BENCHMARK(BM_CoordinatorPlacement_4Shards);

static void BM_CoordinatorRebalance_10Artifacts_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  ShardPlacementCoordinator coordinator(hardware);

  std::vector<ArtifactManifest> manifests;
  for (int i = 0; i < 10; ++i) {
    manifests.push_back(create_benchmark_artifact(100 * 1024 * 1024 * (i + 1)));
  }

  for (auto _ : state) {
    auto results = coordinator.rebalance_placements(manifests, nullptr);
    benchmark::DoNotOptimize(results);
  }
}
BENCHMARK(BM_CoordinatorRebalance_10Artifacts_16Shards);

static void BM_CoordinatorValidation_10Artifacts_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  ShardPlacementCoordinator coordinator(hardware);

  std::vector<ArtifactManifest> manifests;
  for (int i = 0; i < 10; ++i) {
    auto manifest = create_benchmark_artifact(100 * 1024 * 1024 * (i + 1));
    manifest.shard_placements = {"shard_0", "shard_1"};
    manifests.push_back(manifest);
  }

  for (auto _ : state) {
    auto errors = coordinator.validate_placements(manifests);
    benchmark::DoNotOptimize(errors);
  }
}
BENCHMARK(BM_CoordinatorValidation_10Artifacts_16Shards);

// ============================================================================
// Benchmark: Cost Model
// ============================================================================

static void BM_CostModelComputation_4Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(4);
  auto& cost_model = get_default_cost_model();
  auto manifest = create_benchmark_artifact();
  PlacementConfig config;

  std::vector<ShardDescriptor> shards = {hardware.shards[0], hardware.shards[1]};

  for (auto _ : state) {
    float cost = cost_model.compute_cost(manifest, shards, hardware, config);
    benchmark::DoNotOptimize(cost);
  }
}
BENCHMARK(BM_CostModelComputation_4Shards);

static void BM_LatencyEstimation(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  auto& cost_model = get_default_cost_model();

  std::vector<ShardDescriptor> shards = {hardware.shards[0], hardware.shards[1]};

  for (auto _ : state) {
    float latency = cost_model.estimate_latency(shards, hardware);
    benchmark::DoNotOptimize(latency);
  }
}
BENCHMARK(BM_LatencyEstimation);

// ============================================================================
// Benchmark: Artifact Analyzer
// ============================================================================

static void BM_AnalyzerSuitability(benchmark::State& state) {
  auto& analyzer = get_default_analyzer();
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    bool should_use = analyzer.should_use_factorized_placement(manifest);
    benchmark::DoNotOptimize(should_use);
  }
}
BENCHMARK(BM_AnalyzerSuitability);

static void BM_AnalyzerValidation_16Shards(benchmark::State& state) {
  auto hardware = create_benchmark_hardware(16);
  auto& analyzer = get_default_analyzer();
  auto manifest = create_benchmark_artifact();

  for (auto _ : state) {
    std::string error = analyzer.validate_placement_feasibility(manifest, hardware);
    benchmark::DoNotOptimize(error);
  }
}
BENCHMARK(BM_AnalyzerValidation_16Shards);

}  // namespace
}  // namespace distributed_tensor
}  // namespace themis

BENCHMARK_MAIN();
