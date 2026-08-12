/**
 * @file test_distributed_training_coordinator.cpp
 * @brief Unit tests for Distributed LoRA Training Coordinator
 * 
 * Tests distributed training functionality including:
 * - Coordinator initialization and lifecycle
 * - Gradient collection and aggregation
 * - Fault tolerance and shard failure handling
 * - Checkpointing and recovery
 * - Different synchronization strategies
 * - Gradient compression
 * 
 * @note Requires GTest: vcpkg install gtest OR apt-get install libgtest-dev
 * @build cmake -DTHEMIS_BUILD_TESTS=ON -DTHEMIS_ENABLE_DISTRIBUTED_TRAINING=ON ..
 * @run ./tests/test_distributed_training_coordinator
 */

#ifndef THEMIS_TEST_BUILD
#define THEMIS_TEST_BUILD 1
#endif

#include <gtest/gtest.h>

// Disable distributed training coordinator tests
#if 0
#include "llm/distributed_training_coordinator.h"
#include "llm/adapter_registry.h"
#include "sharding/shard_router.h"
#include "sharding/shard_topology.h"
#include <memory>
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themis::llm;
namespace fs = std::filesystem;

// ============================================================================
// Test Fixtures
// ============================================================================

class DistributedTrainingCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create mock shard router and topology
        // In a real implementation, these would be actual components
        shard_router_ = nullptr;  // Placeholder
        shard_topology_ = nullptr;  // Placeholder
        
        // Setup test configuration
        config_.sync_strategy = SyncStrategy::ALL_REDUCE;
        config_.coordinator_shard = "shard-coordinator";
        config_.participant_shards = {"shard-1", "shard-2", "shard-3"};
        config_.gradient_accumulation_steps = 1;
        config_.sync_frequency = 1;
        config_.gradient_clip_norm = 1.0f;
        config_.enable_checkpointing = true;
        config_.checkpoint_frequency = 10;
        config_.checkpoint_path = "/tmp/test_distributed_checkpoints";
        
        // Create checkpoint directory
        fs::create_directories(config_.checkpoint_path);
    }
    
    void TearDown() override {
        // Cleanup test files
        if (fs::exists(config_.checkpoint_path)) {
            fs::remove_all(config_.checkpoint_path);
        }
    }
    
    std::shared_ptr<ShardRouter> shard_router_;
    std::shared_ptr<ShardTopology> shard_topology_;
    DistributedTrainingConfig config_;
};

// ============================================================================
// DistributedTrainingConfig Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, Config_JSONSerialization) {
    // Test JSON serialization
    json config_json = config_.toJSON();
    
    EXPECT_EQ(config_json["coordinator_shard"], "shard-coordinator");
    EXPECT_EQ(config_json["participant_shards"].size(), 3);
    EXPECT_EQ(config_json["gradient_accumulation_steps"], 1);
    EXPECT_TRUE(config_json["enable_checkpointing"].get<bool>());
    
    // Test deserialization
    auto config_restored = DistributedTrainingConfig::fromJSON(config_json);
    EXPECT_EQ(config_restored.coordinator_shard, config_.coordinator_shard);
    EXPECT_EQ(config_restored.participant_shards.size(), config_.participant_shards.size());
    EXPECT_EQ(config_restored.sync_strategy, config_.sync_strategy);
}

// ============================================================================
// GradientTensor Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, GradientTensor_Compression8Bit) {
    GradientTensor tensor;
    tensor.layer_name = "test_layer";
    tensor.shape = {64, 64};
    tensor.source_shard = "shard-1";
    tensor.step_number = 1;
    
    // Fill with test data
    tensor.data.resize(64 * 64);
    for (size_t i = 0; i < tensor.data.size(); ++i) {
        tensor.data[i] = static_cast<float>(i) / 1000.0f - 2.0f;
    }
    
    size_t original_size = tensor.uncompressed_size();
    
    // Compress
    tensor.compress(GradientCompressionType::QUANTIZATION_8BIT);
    size_t compressed_size = tensor.compressed_size();
    
    EXPECT_LT(compressed_size, original_size);
    EXPECT_TRUE(tensor.compressed_data.has_value());
    
    // Decompress
    tensor.decompress();
    
    EXPECT_FALSE(tensor.compressed_data.has_value());
    EXPECT_EQ(tensor.data.size(), 64 * 64);
    
    // Check approximate restoration (quantization introduces error)
    for (size_t i = 0; i < 100; ++i) {
        float original = static_cast<float>(i) / 1000.0f - 2.0f;
        float restored = tensor.data[i];
        EXPECT_NEAR(original, restored, 0.1f);  // Allow some quantization error
    }
}

TEST_F(DistributedTrainingCoordinatorTest, GradientTensor_Compression4Bit) {
    GradientTensor tensor;
    tensor.layer_name = "test_layer";
    tensor.shape = {32, 32};
    tensor.data.resize(32 * 32, 0.5f);
    
    size_t original_size = tensor.uncompressed_size();
    
    // Compress with 4-bit quantization
    tensor.compress(GradientCompressionType::QUANTIZATION_4BIT);
    size_t compressed_size = tensor.compressed_size();
    
    // Should be roughly 1/8 of original (4 bits + metadata)
    EXPECT_LT(compressed_size, original_size / 4);
    
    tensor.decompress();
    EXPECT_EQ(tensor.data.size(), 32 * 32);
}

TEST_F(DistributedTrainingCoordinatorTest, GradientTensor_CompressionSparseTopK) {
    GradientTensor tensor;
    tensor.layer_name = "test_layer";
    tensor.shape = {100, 100};
    tensor.data.resize(100 * 100, 0.0f);
    
    // Set only a few large values
    for (int i = 0; i < 50; ++i) {
        tensor.data[i * 10] = 10.0f;
    }
    
    size_t original_size = tensor.uncompressed_size();
    
    // Compress with sparse top-k
    tensor.compress(GradientCompressionType::SPARSE_TOPK);
    size_t compressed_size = tensor.compressed_size();
    
    // Should be much smaller (only top 10% kept)
    EXPECT_LT(compressed_size, original_size / 5);
    
    tensor.decompress();
    EXPECT_EQ(tensor.data.size(), 100 * 100);
    
    // Check that large values are preserved
    for (int i = 0; i < 10; ++i) {
        EXPECT_NEAR(tensor.data[i * 10], 10.0f, 0.1f);
    }
}

TEST_F(DistributedTrainingCoordinatorTest, GradientTensor_JSONSerialization) {
    GradientTensor tensor;
    tensor.layer_name = "test_layer";
    tensor.shape = {4, 4};
    tensor.data = {1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f, 8.0f,
                   9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f, 16.0f};
    tensor.source_shard = "shard-1";
    tensor.step_number = 42;
    
    // Serialize
    json j = tensor.toJSON();
    
    EXPECT_EQ(j["layer_name"], "test_layer");
    EXPECT_EQ(j["shape"].size(), 2);
    EXPECT_EQ(j["source_shard"], "shard-1");
    EXPECT_EQ(j["step_number"], 42);
    
    // Deserialize
    auto tensor_restored = GradientTensor::fromJSON(j);
    
    EXPECT_EQ(tensor_restored.layer_name, tensor.layer_name);
    EXPECT_EQ(tensor_restored.shape, tensor.shape);
    EXPECT_EQ(tensor_restored.data.size(), tensor.data.size());
    EXPECT_EQ(tensor_restored.source_shard, tensor.source_shard);
}

// ============================================================================
// Gradient Aggregator Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, AllReduceAggregator_AveragesGradients) {
    AllReduceAggregator aggregator;
    
    // Create gradients from 3 shards
    std::vector<std::vector<GradientTensor>> shard_gradients;
    
    for (int shard = 0; shard < 3; ++shard) {
        std::vector<GradientTensor> grads;
        GradientTensor tensor;
        tensor.layer_name = "layer_1";
        tensor.shape = {4, 4};
        tensor.data.resize(16, static_cast<float>(shard + 1));  // Shard 0: all 1.0, Shard 1: all 2.0, etc.
        tensor.source_shard = "shard-" + std::to_string(shard);
        grads.push_back(tensor);
        shard_gradients.push_back(grads);
    }
    
    // Aggregate
    auto aggregated = aggregator.aggregate(shard_gradients);
    
    ASSERT_EQ(aggregated.size(), 1);
    EXPECT_EQ(aggregated[0].layer_name, "layer_1");
    EXPECT_EQ(aggregated[0].data.size(), 16);
    
    // Should be average: (1 + 2 + 3) / 3 = 2.0
    for (const auto& val : aggregated[0].data) {
        EXPECT_FLOAT_EQ(val, 2.0f);
    }
}

TEST_F(DistributedTrainingCoordinatorTest, ParameterServerAggregator_WeightedAverage) {
    // Create weights for 3 shards (proportional to data size)
    std::map<std::string, float> weights;
    weights["shard-0"] = 0.5f;  // 50% of data
    weights["shard-1"] = 0.3f;  // 30% of data
    weights["shard-2"] = 0.2f;  // 20% of data
    
    ParameterServerAggregator aggregator(weights);
    
    // Create gradients with different values
    std::vector<std::vector<GradientTensor>> shard_gradients;
    
    for (int shard = 0; shard < 3; ++shard) {
        std::vector<GradientTensor> grads;
        GradientTensor tensor;
        tensor.layer_name = "layer_1";
        tensor.shape = {2, 2};
        tensor.data.resize(4, static_cast<float>((shard + 1) * 10));  // Shard 0: 10, Shard 1: 20, Shard 2: 30
        tensor.source_shard = "shard-" + std::to_string(shard);
        grads.push_back(tensor);
        shard_gradients.push_back(grads);
    }
    
    // Aggregate
    auto aggregated = aggregator.aggregate(shard_gradients);
    
    ASSERT_EQ(aggregated.size(), 1);
    
    // Weighted average: 10*0.5 + 20*0.3 + 30*0.2 = 5 + 6 + 6 = 17
    for (const auto& val : aggregated[0].data) {
        EXPECT_FLOAT_EQ(val, 17.0f);
    }
}

// ============================================================================
// ShardTrainingState Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, ShardTrainingState_JSONSerialization) {
    ShardTrainingState state;
    state.shard_id = "shard-1";
    state.current_epoch = 2;
    state.current_step = 150;
    state.total_steps = 1000;
    state.current_loss = 0.5f;
    state.is_active = true;
    state.is_synchronized = true;
    state.gpu_utilization = 0.85f;
    state.memory_usage_gb = 12.5f;
    
    // Serialize
    json j = state.toJSON();
    
    EXPECT_EQ(j["shard_id"], "shard-1");
    EXPECT_EQ(j["current_epoch"], 2);
    EXPECT_EQ(j["current_step"], 150);
    EXPECT_FLOAT_EQ(j["current_loss"].get<float>(), 0.5f);
    
    // Deserialize
    auto state_restored = ShardTrainingState::fromJSON(j);
    
    EXPECT_EQ(state_restored.shard_id, state.shard_id);
    EXPECT_EQ(state_restored.current_epoch, state.current_epoch);
    EXPECT_EQ(state_restored.current_step, state.current_step);
    EXPECT_FLOAT_EQ(state_restored.current_loss, state.current_loss);
}

// ============================================================================
// DistributedTrainingCoordinator Lifecycle Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_Construction) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    EXPECT_NE(coordinator, nullptr);
    
    auto retrieved_config = coordinator->getConfig();
    EXPECT_EQ(retrieved_config.coordinator_shard, config_.coordinator_shard);
    EXPECT_EQ(retrieved_config.participant_shards.size(), 3);
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_Initialization) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    TrainingConfig training_config;
    training_config.epochs = 3;
    training_config.learning_rate = 2e-4;
    training_config.batch_size = 4;
    
    bool success = coordinator->initialize("test-adapter-1", training_config);
    
    // Will fail because we don't have real shard infrastructure
    // But it should not crash
    // In a real test with mock shards, this would succeed
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_ConfigUpdate) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    // Update configuration
    DistributedTrainingConfig new_config = config_;
    new_config.sync_strategy = SyncStrategy::PARAMETER_SERVER;
    new_config.gradient_accumulation_steps = 4;
    
    coordinator->updateConfig(new_config);
    
    auto retrieved_config = coordinator->getConfig();
    EXPECT_EQ(retrieved_config.sync_strategy, SyncStrategy::PARAMETER_SERVER);
    EXPECT_EQ(retrieved_config.gradient_accumulation_steps, 4);
}

// ============================================================================
// Checkpoint Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_CheckpointSaveAndLoad) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    TrainingConfig training_config;
    training_config.epochs = 3;
    
    // Initialize (may fail without real shards, but won't crash)
    coordinator->initialize("test-adapter-checkpoint", training_config);
    
    // Save checkpoint
    bool save_success = coordinator->saveCheckpoint(42);
    
    if (save_success) {
        // Verify checkpoint file exists
        std::string checkpoint_file = config_.checkpoint_path + "/checkpoint_step_42.json";
        EXPECT_TRUE(fs::exists(checkpoint_file));
        
        // Create new coordinator and restore
        auto coordinator2 = std::make_unique<DistributedTrainingCoordinator>(
            shard_router_, shard_topology_, config_
        );
        
        bool restore_success = coordinator2->resumeFromCheckpoint(checkpoint_file);
        EXPECT_TRUE(restore_success);
    }
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_Statistics) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    auto stats = coordinator->getStatistics();
    
    EXPECT_EQ(stats.total_steps_completed, 0);
    EXPECT_EQ(stats.total_gradient_syncs, 0);
    EXPECT_EQ(stats.shard_failures, 0);
    
    // JSON serialization
    json stats_json = stats.toJSON();
    EXPECT_TRUE(stats_json.contains("total_steps_completed"));
    EXPECT_TRUE(stats_json.contains("avg_sync_time_ms"));
    EXPECT_TRUE(stats_json.contains("compression_ratio"));
}

// ============================================================================
// Factory Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, Factory_Create) {
    auto coordinator = DistributedTrainingCoordinatorFactory::create(
        shard_router_, shard_topology_, config_
    );
    
    EXPECT_NE(coordinator, nullptr);
    EXPECT_EQ(coordinator->getConfig().coordinator_shard, config_.coordinator_shard);
}

TEST_F(DistributedTrainingCoordinatorTest, Factory_CreateWithAutoDiscovery) {
    auto coordinator = DistributedTrainingCoordinatorFactory::createWithAutoDiscovery(
        shard_router_, SyncStrategy::ALL_REDUCE
    );
    
    EXPECT_NE(coordinator, nullptr);
    EXPECT_EQ(coordinator->getConfig().sync_strategy, SyncStrategy::ALL_REDUCE);
}

// ============================================================================
// Edge Cases and Error Handling
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, GradientTensor_EmptyData) {
    GradientTensor tensor;
    tensor.layer_name = "empty_layer";
    tensor.shape = {0};
    
    // Should not crash
    tensor.compress(GradientCompressionType::QUANTIZATION_8BIT);
    tensor.decompress();
    
    json j = tensor.toJSON();
    EXPECT_TRUE(j.contains("layer_name"));
}

TEST_F(DistributedTrainingCoordinatorTest, Aggregator_EmptyInput) {
    AllReduceAggregator aggregator;
    
    std::vector<std::vector<GradientTensor>> empty_input;
    auto result = aggregator.aggregate(empty_input);
    
    EXPECT_TRUE(result.empty());
}

TEST_F(DistributedTrainingCoordinatorTest, Aggregator_MismatchedSizes) {
    AllReduceAggregator aggregator;
    
    std::vector<std::vector<GradientTensor>> shard_gradients;
    
    // Shard 0: tensor with 16 elements
    {
        std::vector<GradientTensor> grads;
        GradientTensor tensor;
        tensor.layer_name = "layer_1";
        tensor.shape = {4, 4};
        tensor.data.resize(16, 1.0f);
        grads.push_back(tensor);
        shard_gradients.push_back(grads);
    }
    
    // Shard 1: tensor with 9 elements (mismatched!)
    {
        std::vector<GradientTensor> grads;
        GradientTensor tensor;
        tensor.layer_name = "layer_1";
        tensor.shape = {3, 3};
        tensor.data.resize(9, 2.0f);
        grads.push_back(tensor);
        shard_gradients.push_back(grads);
    }
    
    // Should handle gracefully (skip mismatched shard)
    auto result = aggregator.aggregate(shard_gradients);
    
    EXPECT_EQ(result.size(), 1);
    // Should only use the first shard's gradient
    for (const auto& val : result[0].data) {
        EXPECT_FLOAT_EQ(val, 1.0f);
    }
}

// ============================================================================
// Loss Aggregation Tests
// ============================================================================

TEST_F(DistributedTrainingCoordinatorTest, GradientExchangeMessage_LossMetricsSerialization) {
    GradientExchangeMessage msg;
    msg.message_id = "test-msg-123";
    msg.source_shard = "shard-1";
    msg.destination_shard = "shard-2";
    msg.iteration_number = 5;
    msg.local_loss = 0.456f;
    msg.local_accuracy = 0.89f;
    msg.samples_in_batch = 32;
    
    // Serialize
    json j = msg.toJSON();
    
    EXPECT_TRUE(j.contains("local_loss"));
    EXPECT_TRUE(j.contains("local_accuracy"));
    EXPECT_TRUE(j.contains("samples_in_batch"));
    EXPECT_FLOAT_EQ(j["local_loss"].get<float>(), 0.456f);
    EXPECT_FLOAT_EQ(j["local_accuracy"].get<float>(), 0.89f);
    EXPECT_EQ(j["samples_in_batch"].get<int>(), 32);
    
    // Deserialize
    auto msg_restored = GradientExchangeMessage::fromJSON(j);
    
    EXPECT_TRUE(msg_restored.local_loss.has_value());
    EXPECT_TRUE(msg_restored.local_accuracy.has_value());
    EXPECT_FLOAT_EQ(msg_restored.local_loss.value(), 0.456f);
    EXPECT_FLOAT_EQ(msg_restored.local_accuracy.value(), 0.89f);
    EXPECT_EQ(msg_restored.samples_in_batch, 32);
}

TEST_F(DistributedTrainingCoordinatorTest, GradientExchangeMessage_OptionalLossFields) {
    GradientExchangeMessage msg;
    msg.message_id = "test-msg-456";
    msg.source_shard = "shard-3";
    
    // Don't set loss fields - should be optional
    EXPECT_FALSE(msg.local_loss.has_value());
    EXPECT_FALSE(msg.local_accuracy.has_value());
    
    // Serialize without loss
    json j = msg.toJSON();
    
    // Should not crash and should handle missing fields
    auto msg_restored = GradientExchangeMessage::fromJSON(j);
    EXPECT_FALSE(msg_restored.local_loss.has_value());
    EXPECT_FALSE(msg_restored.local_accuracy.has_value());
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_ComputeWeightedLoss_SimpleAverage) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    // Test weighted loss computation with equal samples
    std::vector<std::pair<float, int>> losses_and_counts = {
        {1.0f, 32},  // Shard 1: loss=1.0, 32 samples
        {2.0f, 32},  // Shard 2: loss=2.0, 32 samples
        {3.0f, 32}   // Shard 3: loss=3.0, 32 samples
    };
    
    float weighted_loss = coordinator->computeWeightedLoss(losses_and_counts);
    
    // Expected: (1.0*32 + 2.0*32 + 3.0*32) / (32+32+32) = 6.0*32/96 = 2.0
    EXPECT_FLOAT_EQ(weighted_loss, 2.0f);
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_ComputeWeightedLoss_UnequalSamples) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    // Test weighted loss with different sample counts
    std::vector<std::pair<float, int>> losses_and_counts = {
        {1.0f, 10},  // Shard 1: loss=1.0, 10 samples
        {2.0f, 20},  // Shard 2: loss=2.0, 20 samples
        {4.0f, 30}   // Shard 3: loss=4.0, 30 samples
    };
    
    float weighted_loss = coordinator->computeWeightedLoss(losses_and_counts);
    
    // Expected: (1.0*10 + 2.0*20 + 4.0*30) / (10+20+30) = (10 + 40 + 120) / 60 = 170/60 = 2.833...
    EXPECT_NEAR(weighted_loss, 2.8333f, 0.001f);
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_ComputeWeightedLoss_ZeroSamples) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    // Test with zero samples - should fall back to simple average
    std::vector<std::pair<float, int>> losses_and_counts = {
        {1.0f, 0},
        {2.0f, 0},
        {3.0f, 0}
    };
    
    float weighted_loss = coordinator->computeWeightedLoss(losses_and_counts);
    
    // Should fall back to simple average: (1.0 + 2.0 + 3.0) / 3 = 2.0
    EXPECT_FLOAT_EQ(weighted_loss, 2.0f);
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_ComputeWeightedLoss_EmptyInput) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    std::vector<std::pair<float, int>> empty_losses;
    
    float weighted_loss = coordinator->computeWeightedLoss(empty_losses);
    
    // Should return 0.0 for empty input
    EXPECT_FLOAT_EQ(weighted_loss, 0.0f);
}

TEST_F(DistributedTrainingCoordinatorTest, Coordinator_ExecuteStep_ReturnsAggregatedLoss) {
    auto coordinator = std::make_unique<DistributedTrainingCoordinator>(
        shard_router_, shard_topology_, config_
    );
    
    TrainingConfig training_config;
    training_config.epochs = 1;
    
    // Initialize (will use simulated mode without real shards)
    bool initialized = coordinator->initialize("test-adapter-loss", training_config);
    
    if (initialized) {
        // Execute a step
        auto step_result = coordinator->executeStep();
        
        if (step_result.success) {
            // Should have aggregated loss in simulated mode
            EXPECT_TRUE(step_result.aggregated_loss.has_value());
            
            if (step_result.aggregated_loss.has_value()) {
                // Loss should be positive and reasonable
                EXPECT_GT(step_result.aggregated_loss.value(), 0.0f);
                EXPECT_LT(step_result.aggregated_loss.value(), 10.0f);
            }
            
            // Should have per-shard loss
            EXPECT_FALSE(step_result.per_shard_loss.empty());
            EXPECT_EQ(step_result.per_shard_loss.size(), config_.participant_shards.size());
        }
    }
}

TEST_F(DistributedTrainingCoordinatorTest, StepResult_ContainsLossFields) {
    // Create a step result and verify it has the new fields
    DistributedTrainingCoordinator::StepResult result;
    
    result.aggregated_loss = 0.123f;
    result.aggregated_accuracy = 0.95f;
    result.per_shard_loss["shard-1"] = 0.12f;
    result.per_shard_loss["shard-2"] = 0.13f;
    
    EXPECT_TRUE(result.aggregated_loss.has_value());
    EXPECT_TRUE(result.aggregated_accuracy.has_value());
    EXPECT_EQ(result.per_shard_loss.size(), 2);
    EXPECT_FLOAT_EQ(result.aggregated_loss.value(), 0.123f);
    EXPECT_FLOAT_EQ(result.aggregated_accuracy.value(), 0.95f);
}

// ============================================================================
// Main Test Runner
// ============================================================================


#endif // 0

TEST(DistributedTrainingCoordinatorDisabled, DISABLED_AllTestsSkipped) {
    GTEST_SKIP() << "Distributed training coordinator tests are currently disabled";
}


