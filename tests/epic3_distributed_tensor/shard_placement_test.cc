/// @file shard_placement_test.cc
/// @brief Unit tests for factorization-aware shard placement strategies
/// @author ThemisDB EPIC 3 Implementation Team
/// @date 2026-07-02

#include <gtest/gtest.h>
#include "src/distributed_tensor/include/shard_placement.h"

namespace themis {
namespace distributed_tensor {
namespace {

/// @brief Fixture for shard placement tests
class ShardPlacementTest : public ::testing::Test {
 protected:
  void SetUp() override {
    // Create a simple hardware profile with 4 shards
    hardware_.replication_factor = 2;
    hardware_.enable_factorization_aware = true;

    for (int i = 0; i < 4; ++i) {
      ShardDescriptor shard;
      shard.shard_id = "shard_" + std::to_string(i);
      shard.capacity_bytes = 1024 * 1024 * 1024;  // 1 GB each
      shard.used_bytes = 256 * 1024 * 1024;       // 256 MB used
      shard.latency_ms = 10 + (i * 5);            // 10, 15, 20, 25 ms
      shard.tier = (i % 2 == 0) ? "GPU" : "CPU";
      shard.is_healthy = true;
      shard.reliability_score = 0.95f + (i * 0.01f);
      hardware_.shards.push_back(shard);
    }
  }

  HardwareProfile hardware_;

  /// @brief Create a basic artifact manifest for testing
  ArtifactManifest create_test_artifact(
      uint64_t size_bytes = 100 * 1024 * 1024,
      ArtifactClass artifact_class = ArtifactClass::PRIMARY,
      std::string semantic_hint = "") {
    ArtifactManifest manifest;
    manifest.artifact_id = "test_artifact_" + std::to_string(++artifact_counter_);
    manifest.size_bytes = size_bytes;
    manifest.artifact_class = artifact_class;
    manifest.semantic_hint = semantic_hint;
    manifest.replication_factor = 2;
    manifest.version = "1.0.0";
    return manifest;
  }

 private:
  static int artifact_counter_;
};

int ShardPlacementTest::artifact_counter_ = 0;

// ============================================================================
// Test: RoundRobinPlacementStrategy
// ============================================================================

TEST_F(ShardPlacementTest, RoundRobinPlacement) {
  RoundRobinPlacementStrategy strategy;
  EXPECT_EQ(strategy.type(), PlacementStrategyType::ROUND_ROBIN);
  EXPECT_EQ(strategy.name(), "RoundRobin");
  EXPECT_TRUE(strategy.supports_artifact_class(ArtifactClass::PRIMARY));
}

TEST_F(ShardPlacementTest, RoundRobinPlacesArtifact) {
  RoundRobinPlacementStrategy strategy;
  auto manifest = create_test_artifact();

  PlacementConfig config;
  config.target_replication_factor = 2;

  auto result = strategy.place(manifest, hardware_, config, nullptr);

  EXPECT_TRUE(result.success);
  EXPECT_EQ(result.shard_placements.size(), 2);
  EXPECT_FALSE(result.shard_placements[0].empty());
}

TEST_F(ShardPlacementTest, RoundRobinFailsWithNoShards) {
  RoundRobinPlacementStrategy strategy;
  auto manifest = create_test_artifact();
  HardwareProfile empty_hardware;

  PlacementConfig config;
  auto result = strategy.place(manifest, empty_hardware, config, nullptr);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.shard_placements.size(), 0);
}

TEST_F(ShardPlacementTest, RoundRobinFailsWithInsufficientCapacity) {
  RoundRobinPlacementStrategy strategy;
  auto manifest = create_test_artifact(10 * 1024 * 1024 * 1024);  // 10 GB, too large

  PlacementConfig config;
  auto result = strategy.place(manifest, hardware_, config, nullptr);

  EXPECT_FALSE(result.success);
  EXPECT_EQ(result.shard_placements.size(), 0);
}

// ============================================================================
// Test: FactorizedPlacementStrategy
// ============================================================================

TEST_F(ShardPlacementTest, FactorizedPlacementStrategy) {
  FactorizedPlacementStrategy strategy;
  EXPECT_EQ(strategy.type(), PlacementStrategyType::FACTORIZED);
  EXPECT_EQ(strategy.name(), "Factorized");
  EXPECT_TRUE(strategy.supports_artifact_class(ArtifactClass::PRIMARY));
  EXPECT_FALSE(strategy.supports_artifact_class(ArtifactClass::EPHEMERAL));
}

TEST_F(ShardPlacementTest, FactorizedPlacesArtifact) {
  FactorizedPlacementStrategy strategy;
  auto manifest = create_test_artifact(100 * 1024 * 1024, ArtifactClass::PRIMARY, "TT_CORE");

  PlacementConfig config;
  config.target_replication_factor = 2;

  auto result = strategy.place(manifest, hardware_, config, nullptr);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.shard_placements.size(), 0);
  EXPECT_LE(result.shard_placements.size(), 2);
}

TEST_F(ShardPlacementTest, FactorizedPlacesFactorMatrix) {
  FactorizedPlacementStrategy strategy;
  auto manifest = create_test_artifact(50 * 1024 * 1024, ArtifactClass::PRIMARY, "FACTOR_MATRIX");

  PlacementConfig config;
  config.target_replication_factor = 2;

  auto result = strategy.place(manifest, hardware_, config, nullptr);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.shard_placements.size(), 0);
}

// ============================================================================
// Test: CostAwarePlacementStrategy
// ============================================================================

TEST_F(ShardPlacementTest, CostAwarePlacementStrategy) {
  CostAwarePlacementStrategy strategy;
  EXPECT_EQ(strategy.type(), PlacementStrategyType::COST_AWARE);
  EXPECT_EQ(strategy.name(), "CostAware");
}

TEST_F(ShardPlacementTest, CostAwarePlacesArtifact) {
  CostAwarePlacementStrategy strategy;
  auto manifest = create_test_artifact(80 * 1024 * 1024, ArtifactClass::PRIMARY);

  PlacementConfig config;
  config.target_replication_factor = 2;
  config.latency_weight = 0.3f;
  config.capacity_weight = 0.3f;
  config.reliability_weight = 0.4f;

  auto result = strategy.place(manifest, hardware_, config, nullptr);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.shard_placements.size(), 0);
  EXPECT_GT(result.placement_cost, 0.0f);
  EXPECT_GT(result.estimated_latency_ms, 0.0f);
}

// ============================================================================
// Test: BalancedPlacementStrategy
// ============================================================================

TEST_F(ShardPlacementTest, BalancedPlacementStrategy) {
  BalancedPlacementStrategy strategy;
  EXPECT_EQ(strategy.type(), PlacementStrategyType::BALANCED);
  EXPECT_EQ(strategy.name(), "Balanced");
  EXPECT_TRUE(strategy.supports_artifact_class(ArtifactClass::PRIMARY));
}

TEST_F(ShardPlacementTest, BalancedPlacesArtifact) {
  BalancedPlacementStrategy strategy;
  auto manifest = create_test_artifact(100 * 1024 * 1024, ArtifactClass::PRIMARY, "TT_CORE");

  PlacementConfig config;
  config.target_replication_factor = 2;
  config.use_factorization_hints = true;

  auto result = strategy.place(manifest, hardware_, config, nullptr);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.shard_placements.size(), 0);
}

// ============================================================================
// Test: PlacementStrategyFactory
// ============================================================================

TEST_F(ShardPlacementTest, FactoryCreatesStrategies) {
  auto round_robin = PlacementStrategyFactory::create_strategy(
      PlacementStrategyType::ROUND_ROBIN);
  EXPECT_NE(round_robin, nullptr);
  EXPECT_EQ(round_robin->type(), PlacementStrategyType::ROUND_ROBIN);

  auto factorized = PlacementStrategyFactory::create_strategy(
      PlacementStrategyType::FACTORIZED);
  EXPECT_NE(factorized, nullptr);
  EXPECT_EQ(factorized->type(), PlacementStrategyType::FACTORIZED);

  auto cost_aware = PlacementStrategyFactory::create_strategy(
      PlacementStrategyType::COST_AWARE);
  EXPECT_NE(cost_aware, nullptr);
  EXPECT_EQ(cost_aware->type(), PlacementStrategyType::COST_AWARE);

  auto balanced = PlacementStrategyFactory::create_strategy(
      PlacementStrategyType::BALANCED);
  EXPECT_NE(balanced, nullptr);
  EXPECT_EQ(balanced->type(), PlacementStrategyType::BALANCED);
}

TEST_F(ShardPlacementTest, FactoryCreatesAllStrategies) {
  auto strategies = PlacementStrategyFactory::create_all_strategies();
  EXPECT_EQ(strategies.size(), 4);
  EXPECT_NE(strategies.find(PlacementStrategyType::ROUND_ROBIN), strategies.end());
  EXPECT_NE(strategies.find(PlacementStrategyType::FACTORIZED), strategies.end());
  EXPECT_NE(strategies.find(PlacementStrategyType::COST_AWARE), strategies.end());
  EXPECT_NE(strategies.find(PlacementStrategyType::BALANCED), strategies.end());
}

TEST_F(ShardPlacementTest, FactoryRecommendsStrategy) {
  auto recommended = PlacementStrategyFactory::recommend_strategy(hardware_);
  EXPECT_NE(recommended, PlacementStrategyType::ROUND_ROBIN);  // Not for 4 shards
}

// ============================================================================
// Test: ArtifactPlacementAnalyzer
// ============================================================================

TEST_F(ShardPlacementTest, DefaultAnalyzerIdentifiesTTCores) {
  auto& analyzer = get_default_analyzer();
  auto manifest = create_test_artifact(100 * 1024 * 1024, ArtifactClass::PRIMARY, "TT_CORE");

  bool should_use = analyzer.should_use_factorized_placement(manifest);
  EXPECT_TRUE(should_use);
}

TEST_F(ShardPlacementTest, DefaultAnalyzerRejectsEphemeral) {
  auto& analyzer = get_default_analyzer();
  auto manifest = create_test_artifact(100 * 1024 * 1024, ArtifactClass::EPHEMERAL);

  bool should_use = analyzer.should_use_factorized_placement(manifest);
  EXPECT_FALSE(should_use);
}

TEST_F(ShardPlacementTest, DefaultAnalyzerValidatesPlacement) {
  auto& analyzer = get_default_analyzer();
  auto manifest = create_test_artifact(100 * 1024 * 1024);

  std::string error = analyzer.validate_placement_feasibility(manifest, hardware_);
  EXPECT_TRUE(error.empty());
}

TEST_F(ShardPlacementTest, DefaultAnalyzerDetectsInsufficientCapacity) {
  auto& analyzer = get_default_analyzer();
  auto manifest = create_test_artifact(10 * 1024 * 1024 * 1024);  // 10 GB total

  std::string error = analyzer.validate_placement_feasibility(manifest, hardware_);
  EXPECT_FALSE(error.empty());
}

// ============================================================================
// Test: PlacementCostModel
// ============================================================================

TEST_F(ShardPlacementTest, DefaultCostModelComputesCost) {
  auto& cost_model = get_default_cost_model();
  auto manifest = create_test_artifact();

  std::vector<ShardDescriptor> shards = {hardware_.shards[0], hardware_.shards[1]};
  PlacementConfig config;

  float cost = cost_model.compute_cost(manifest, shards, hardware_, config);
  EXPECT_GT(cost, 0.0f);
  EXPECT_LT(cost, 1.0f);
}

TEST_F(ShardPlacementTest, DefaultCostModelEstimatesLatency) {
  auto& cost_model = get_default_cost_model();

  std::vector<ShardDescriptor> shards = {hardware_.shards[0], hardware_.shards[1]};

  float latency = cost_model.estimate_latency(shards, hardware_);
  EXPECT_GE(latency, 10.0f);  // Minimum latency is 10 ms
}

// ============================================================================
// Test: HardwareProfile Utilities
// ============================================================================

TEST_F(ShardPlacementTest, HardwareProfileFindsShard) {
  auto shard = hardware_.find_shard("shard_0");
  EXPECT_NE(shard, nullptr);
  EXPECT_EQ(shard->shard_id, "shard_0");

  auto missing = hardware_.find_shard("nonexistent");
  EXPECT_EQ(missing, nullptr);
}

TEST_F(ShardPlacementTest, HardwareProfileFindsCapacity) {
  std::string best_shard = hardware_.find_shard_with_most_capacity();
  EXPECT_FALSE(best_shard.empty());
}

TEST_F(ShardPlacementTest, HardwareProfileFindsLatency) {
  std::string lowest_latency = hardware_.find_shard_with_lowest_latency();
  EXPECT_FALSE(lowest_latency.empty());
  EXPECT_EQ(lowest_latency, "shard_0");  // Has lowest latency (10 ms)
}

TEST_F(ShardPlacementTest, ShardDescriptorUtilities) {
  auto shard = hardware_.shards[0];

  uint64_t available = shard.available_bytes();
  EXPECT_GT(available, 0);
  EXPECT_LT(available, shard.capacity_bytes);

  float util = shard.utilization_percent();
  EXPECT_GT(util, 0.0f);
  EXPECT_LT(util, 100.0f);
}

// ============================================================================
// Test: ShardPlacementCoordinator
// ============================================================================

TEST_F(ShardPlacementTest, CoordinatorInitializes) {
  ShardPlacementCoordinator coordinator(hardware_);

  EXPECT_EQ(coordinator.hardware().shards.size(), 4);
  EXPECT_NE(coordinator.get_strategy(), nullptr);
}

TEST_F(ShardPlacementTest, CoordinatorPlacesArtifact) {
  ShardPlacementCoordinator coordinator(hardware_);
  auto manifest = create_test_artifact();

  auto config = std::make_shared<PlacementConfig>();
  config->target_replication_factor = 2;

  auto result = coordinator.place_artifact(manifest, config);

  EXPECT_TRUE(result.success);
  EXPECT_GT(result.shard_placements.size(), 0);
}

TEST_F(ShardPlacementTest, CoordinatorRebalances) {
  ShardPlacementCoordinator coordinator(hardware_);

  std::vector<ArtifactManifest> manifests;
  for (int i = 0; i < 3; ++i) {
    manifests.push_back(create_test_artifact(50 * 1024 * 1024));
  }

  auto config = std::make_shared<PlacementConfig>();
  auto results = coordinator.rebalance_placements(manifests, config);

  EXPECT_EQ(results.size(), 3);
  for (const auto& result : results) {
    EXPECT_TRUE(result.success);
  }
}

TEST_F(ShardPlacementTest, CoordinatorValidatesPlacements) {
  ShardPlacementCoordinator coordinator(hardware_);

  std::vector<ArtifactManifest> manifests;
  auto manifest = create_test_artifact();
  manifest.shard_placements = {"shard_0", "shard_1"};
  manifests.push_back(manifest);

  auto errors = coordinator.validate_placements(manifests);
  EXPECT_EQ(errors.size(), 0);  // Valid placements
}

TEST_F(ShardPlacementTest, CoordinatorDetectsInvalidPlacements) {
  ShardPlacementCoordinator coordinator(hardware_);

  std::vector<ArtifactManifest> manifests;
  auto manifest = create_test_artifact();
  manifest.shard_placements = {"invalid_shard"};
  manifests.push_back(manifest);

  auto errors = coordinator.validate_placements(manifests);
  EXPECT_GT(errors.size(), 0);  // Invalid placement detected
}

// ============================================================================
// Test: Integration Scenarios
// ============================================================================

TEST_F(ShardPlacementTest, MultipleArtifactPlacement) {
  ShardPlacementCoordinator coordinator(hardware_);

  // Place multiple artifacts of different types
  auto tt_core = create_test_artifact(100 * 1024 * 1024, ArtifactClass::PRIMARY, "TT_CORE");
  auto factor = create_test_artifact(80 * 1024 * 1024, ArtifactClass::PRIMARY, "FACTOR_MATRIX");
  auto derived = create_test_artifact(50 * 1024 * 1024, ArtifactClass::DERIVED);

  auto result1 = coordinator.place_artifact(tt_core, nullptr);
  auto result2 = coordinator.place_artifact(factor, nullptr);
  auto result3 = coordinator.place_artifact(derived, nullptr);

  EXPECT_TRUE(result1.success);
  EXPECT_TRUE(result2.success);
  EXPECT_TRUE(result3.success);
}

TEST_F(ShardPlacementTest, PlacementConsidersDegradedShards) {
  // Degrade one shard
  hardware_.shards[3].is_healthy = false;

  ShardPlacementCoordinator coordinator(hardware_);
  auto manifest = create_test_artifact();

  auto result = coordinator.place_artifact(manifest, nullptr);

  EXPECT_TRUE(result.success);
  // Verify degraded shard is not used if possible
  for (const auto& shard_id : result.shard_placements) {
    EXPECT_NE(shard_id, "shard_3");
  }
}

}  // namespace
}  // namespace distributed_tensor
}  // namespace themis
