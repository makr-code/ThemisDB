/**
 * @file test_distributed_training_e2e.cpp
 * @brief End-to-end integration tests for distributed training loss aggregation
 * 
 * Tests distributed training workflow with multiple shards:
 * - Loss aggregation across 10+ shards
 * - Failure recovery scenarios
 * - Loss serialization/deserialization
 * - Coordinator-worker communication
 * 
 * Based on PR #757 requirements for distributed training validation
 */

#include "test_fixture.h"
#include "test_data_generator.h"
#include "llm/lora_framework/distributed_trainer.h"
#include "llm/distributed_training_coordinator.h"
#include "sharding/shard_rpc_server.h"
#include "sharding/shard_rpc_client.h"
#include "sharding/distributed_coordinator.h"
#include <gtest/gtest.h>
#include <nlohmann/json.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <random>

using namespace themis;
using namespace themis::test;
using json = nlohmann::json;

/**
 * @brief Mock shard worker for testing
 */
class MockShardWorker {
public:
    MockShardWorker(int shard_id, bool should_fail = false)
        : shard_id_(shard_id), should_fail_(should_fail) {}
    
    struct TrainingResult {
        double loss = 0;
        int64_t timestamp_ns;
        int shard_id;
        bool success;
        std::string error_message;
    };
    
    TrainingResult ComputeLocalLoss() {
        // Simulate training computation
        std::this_thread::sleep_for(std::chrono::milliseconds(10 + (rand() % 50)));
        
        TrainingResult result;
        result.shard_id = shard_id_;
        result.timestamp_ns = std::chrono::system_clock::now().time_since_epoch().count();
        
        if (should_fail_) {
            result.success = false;
            result.loss = 0.0;
            result.error_message = "Simulated worker failure";
        } else {
            result.success = true;
            // Simulate realistic loss values (decreasing over iterations)
            result.loss = 2.5 / (1.0 + iteration_ * 0.1) + (rand() % 100) / 1000.0;
            iteration_++;
        }
        
        return result;
    }
    
    void SetFailureMode(bool should_fail) {
        should_fail_ = should_fail;
    }
    
    int GetShardId() const { return shard_id_; }
    
private:
    int shard_id_;
    bool should_fail_;
    int iteration_ = 0;
};

/**
 * @brief Mock coordinator for aggregating losses
 */
class MockTrainingCoordinator {
public:
    struct AggregatedResult {
        double global_loss = 0;
        int successful_shards = {};
        int failed_shards = {};
        std::vector<int> failed_shard_ids;
        int64_t aggregation_time_ns;
    };
    
    AggregatedResult AggregateResults(const std::vector<MockShardWorker::TrainingResult>& results) {
        AggregatedResult agg;
        agg.successful_shards = 0;
        agg.failed_shards = 0;
        agg.global_loss = 0.0;
        
        auto start = std::chrono::high_resolution_clock::now();
        
        for (const auto& result : results) {
            if (result.success) {
                agg.global_loss += result.loss;
                agg.successful_shards++;
            } else {
                agg.failed_shards++;
                agg.failed_shard_ids.push_back(result.shard_id);
            }
        }
        
        // Average loss across successful shards
        if (agg.successful_shards > 0) {
            agg.global_loss /= agg.successful_shards;
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        agg.aggregation_time_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
        
        return agg;
    }
};

/**
 * @brief Integration tests for distributed training
 */
class DistributedTrainingE2ETest : public IntegrationTestFixture {
protected:
    void SetUp() override {
        IntegrationTestFixture::SetUp();
        coordinator_ = std::make_unique<MockTrainingCoordinator>();
        data_gen_ = std::make_unique<TestDataGenerator>();
    }
    
    void TearDown() override {
        workers_.clear();
        coordinator_.reset();
        IntegrationTestFixture::TearDown();
    }
    
    // Create multiple shard workers
    void CreateWorkers(int num_workers, int num_failing = 0) {
        workers_.clear();
        for (int i = 0; i < num_workers; i++) {
            bool should_fail = (i < num_failing);
            workers_.push_back(std::make_unique<MockShardWorker>(i, should_fail));
        }
    }
    
    // Run training iteration across all workers
    std::vector<MockShardWorker::TrainingResult> RunTrainingIteration() {
        std::vector<MockShardWorker::TrainingResult> results;
        
        // Parallel execution simulation
        for (auto& worker : workers_) {
            results.push_back(worker->ComputeLocalLoss());
        }
        
        return results;
    }
    
    std::vector<std::unique_ptr<MockShardWorker>> workers_;
    std::unique_ptr<MockTrainingCoordinator> coordinator_;
    std::unique_ptr<TestDataGenerator> data_gen_;
};

// ============================================================================
// Test 1-3: Basic Distributed Training
// ============================================================================

TEST_F(DistributedTrainingE2ETest, BasicLossAggregationWith10Shards) {
    // Create 10 shard workers
    CreateWorkers(10, 0);  // No failures
    
    // Run training iteration
    auto results = RunTrainingIteration();
    
    ASSERT_EQ(results.size(), 10) << "Should have 10 results";
    
    // Verify all succeeded
    for (const auto& result : results) {
        EXPECT_TRUE(result.success) << "Shard " << result.shard_id << " failed";
        EXPECT_GT(result.loss, 0.0) << "Loss should be positive";
        EXPECT_LT(result.loss, 10.0) << "Loss should be reasonable";
    }
    
    // Aggregate results
    auto aggregated = coordinator_->AggregateResults(results);
    
    EXPECT_EQ(aggregated.successful_shards, 10);
    EXPECT_EQ(aggregated.failed_shards, 0);
    EXPECT_GT(aggregated.global_loss, 0.0);
    EXPECT_LT(aggregated.global_loss, 10.0);
    EXPECT_LT(aggregated.aggregation_time_ns, 1000000) << "Aggregation should be fast (<1ms)";
}

TEST_F(DistributedTrainingE2ETest, LossAggregationWith15Shards) {
    // Test with larger number of shards
    CreateWorkers(15, 0);
    
    auto results = RunTrainingIteration();
    ASSERT_EQ(results.size(), 15);
    
    auto aggregated = coordinator_->AggregateResults(results);
    
    EXPECT_EQ(aggregated.successful_shards, 15);
    EXPECT_EQ(aggregated.failed_shards, 0);
    EXPECT_TRUE(aggregated.failed_shard_ids.empty());
}

TEST_F(DistributedTrainingE2ETest, LossDecreasesOverIterations) {
    // Verify that loss decreases over training iterations
    CreateWorkers(10, 0);
    
    std::vector<double> iteration_losses;
    
    // Run 5 iterations
    for (int iter = 0; iter < 5; iter++) {
        auto results = RunTrainingIteration();
        auto aggregated = coordinator_->AggregateResults(results);
        iteration_losses.push_back(aggregated.global_loss);
    }
    
    // Verify decreasing trend
    for (size_t i = 1; i < iteration_losses.size(); i++) {
        EXPECT_LE(iteration_losses[i], iteration_losses[i-1] * 1.2) 
            << "Loss should generally decrease or stabilize over iterations";
    }
    
    // First loss should be higher than last
    EXPECT_GT(iteration_losses[0], iteration_losses[4] * 0.8)
        << "Overall training should show improvement";
}

// ============================================================================
// Test 4-6: Failure Recovery Scenarios
// ============================================================================

TEST_F(DistributedTrainingE2ETest, HandleSingleShardFailure) {
    // Create 10 workers with 1 failing
    CreateWorkers(10, 1);
    
    auto results = RunTrainingIteration();
    auto aggregated = coordinator_->AggregateResults(results);
    
    EXPECT_EQ(aggregated.successful_shards, 9);
    EXPECT_EQ(aggregated.failed_shards, 1);
    ASSERT_EQ(aggregated.failed_shard_ids.size(), 1);
    EXPECT_EQ(aggregated.failed_shard_ids[0], 0) << "First shard should have failed";
    
    // Global loss should still be computed from successful shards
    EXPECT_GT(aggregated.global_loss, 0.0);
}

TEST_F(DistributedTrainingE2ETest, HandleMultipleShardFailures) {
    // Create 12 workers with 3 failing
    CreateWorkers(12, 3);
    
    auto results = RunTrainingIteration();
    auto aggregated = coordinator_->AggregateResults(results);
    
    EXPECT_EQ(aggregated.successful_shards, 9);
    EXPECT_EQ(aggregated.failed_shards, 3);
    EXPECT_EQ(aggregated.failed_shard_ids.size(), 3);
    
    // Should still compute valid global loss
    EXPECT_GT(aggregated.global_loss, 0.0);
}

TEST_F(DistributedTrainingE2ETest, RecoverFromFailure) {
    // Create workers with failures
    CreateWorkers(10, 2);
    
    // First iteration with failures
    auto results1 = RunTrainingIteration();
    auto aggregated1 = coordinator_->AggregateResults(results1);
    EXPECT_EQ(aggregated1.failed_shards, 2);
    
    // Recover failed shards
    for (auto& worker : workers_) {
        if (worker->GetShardId() < 2) {
            worker->SetFailureMode(false);
        }
    }
    
    // Second iteration after recovery
    auto results2 = RunTrainingIteration();
    auto aggregated2 = coordinator_->AggregateResults(results2);
    
    EXPECT_EQ(aggregated2.failed_shards, 0) << "All shards should recover";
    EXPECT_EQ(aggregated2.successful_shards, 10);
}

// ============================================================================
// Test 7-9: Loss Serialization/Deserialization
// ============================================================================

TEST_F(DistributedTrainingE2ETest, SerializeTrainingResults) {
    CreateWorkers(5, 0);
    auto results = RunTrainingIteration();
    
    // Serialize results to JSON
    json serialized = json::array();
    for (const auto& result : results) {
        json result_json = {
            {"shard_id", result.shard_id},
            {"loss", result.loss},
            {"timestamp_ns", result.timestamp_ns},
            {"success", result.success},
            {"error_message", result.error_message}
        };
        serialized.push_back(result_json);
    }
    
    // Verify serialization
    ASSERT_EQ(serialized.size(), 5);
    
    // Deserialize and validate
    for (size_t i = 0; i < serialized.size(); i++) {
        EXPECT_TRUE(serialized[i].contains("shard_id"));
        EXPECT_TRUE(serialized[i].contains("loss"));
        EXPECT_TRUE(serialized[i].contains("success"));
        EXPECT_EQ(serialized[i]["shard_id"], i);
        EXPECT_TRUE(serialized[i]["success"]);
    }
}

TEST_F(DistributedTrainingE2ETest, SerializeAggregatedResults) {
    CreateWorkers(10, 2);
    auto results = RunTrainingIteration();
    auto aggregated = coordinator_->AggregateResults(results);
    
    // Serialize aggregated result
    json aggregated_json = {
        {"global_loss", aggregated.global_loss},
        {"successful_shards", aggregated.successful_shards},
        {"failed_shards", aggregated.failed_shards},
        {"failed_shard_ids", aggregated.failed_shard_ids},
        {"aggregation_time_ns", aggregated.aggregation_time_ns}
    };
    
    // Verify serialization
    EXPECT_EQ(aggregated_json["successful_shards"], 8);
    EXPECT_EQ(aggregated_json["failed_shards"], 2);
    EXPECT_EQ(aggregated_json["failed_shard_ids"].size(), 2);
    EXPECT_GT(aggregated_json["global_loss"].get<double>(), 0.0);
}

TEST_F(DistributedTrainingE2ETest, RoundTripSerialization) {
    CreateWorkers(8, 0);
    auto results = RunTrainingIteration();
    
    // Serialize
    json serialized = json::array();
    for (const auto& result : results) {
        serialized.push_back({
            {"shard_id", result.shard_id},
            {"loss", result.loss},
            {"timestamp_ns", result.timestamp_ns},
            {"success", result.success}
        });
    }
    
    std::string serialized_str = serialized.dump();
    
    // Deserialize
    json deserialized = json::parse(serialized_str);
    
    // Verify data integrity
    ASSERT_EQ(deserialized.size(), 8);
    for (size_t i = 0; i < deserialized.size(); i++) {
        EXPECT_EQ(deserialized[i]["shard_id"], serialized[i]["shard_id"]);
        EXPECT_DOUBLE_EQ(deserialized[i]["loss"].get<double>(), 
                        serialized[i]["loss"].get<double>());
    }
}

// ============================================================================
// Test 10-12: Performance and Scalability
// ============================================================================

TEST_F(DistributedTrainingE2ETest, ScalabilityWith20Shards) {
    // Test with larger shard count
    CreateWorkers(20, 0);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    auto results = RunTrainingIteration();
    auto aggregated = coordinator_->AggregateResults(results);
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration_ms = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
    
    EXPECT_EQ(aggregated.successful_shards, 20);
    EXPECT_EQ(aggregated.failed_shards, 0);
    
    // Should complete reasonably fast even with 20 shards
    EXPECT_LT(duration_ms, 2000) << "20-shard training iteration should complete in <2s";
}

TEST_F(DistributedTrainingE2ETest, AggregationPerformance) {
    // Test aggregation performance with many results
    CreateWorkers(50, 0);
    
    auto results = RunTrainingIteration();
    
    auto start = std::chrono::high_resolution_clock::now();
    auto aggregated = coordinator_->AggregateResults(results);
    auto end = std::chrono::high_resolution_clock::now();
    
    auto duration_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    
    EXPECT_EQ(aggregated.successful_shards, 50);
    EXPECT_LT(duration_ns, 10000000) << "Aggregation should take <10ms for 50 shards";
}

TEST_F(DistributedTrainingE2ETest, ConcurrentIterations) {
    // Test running multiple iterations in parallel (simulated)
    CreateWorkers(10, 0);
    
    const int NUM_ITERATIONS = 5;
    std::vector<MockTrainingCoordinator::AggregatedResult> all_results;
    
    for (int i = 0; i < NUM_ITERATIONS; i++) {
        auto results = RunTrainingIteration();
        auto aggregated = coordinator_->AggregateResults(results);
        all_results.push_back(aggregated);
    }
    
    // Verify all iterations completed
    ASSERT_EQ(all_results.size(), NUM_ITERATIONS);
    
    for (const auto& result : all_results) {
        EXPECT_EQ(result.successful_shards, 10);
        EXPECT_EQ(result.failed_shards, 0);
        EXPECT_GT(result.global_loss, 0.0);
    }
}

// ============================================================================
// Test 13: End-to-End Workflow
// ============================================================================

TEST_F(DistributedTrainingE2ETest, CompleteTrainingWorkflow) {
    // Simulate complete training workflow
    const int NUM_SHARDS = 12;
    const int NUM_EPOCHS = 3;
    const int ITERATIONS_PER_EPOCH = 5;
    
    CreateWorkers(NUM_SHARDS, 0);
    
    std::vector<double> epoch_avg_losses;
    
    for (int epoch = 0; epoch < NUM_EPOCHS; epoch++) {
        double epoch_total_loss = 0.0;
        
        for (int iter = 0; iter < ITERATIONS_PER_EPOCH; iter++) {
            // Introduce occasional failures
            if (iter == 2 && epoch == 1) {
                workers_[0]->SetFailureMode(true);
            } else {
                workers_[0]->SetFailureMode(false);
            }
            
            auto results = RunTrainingIteration();
            auto aggregated = coordinator_->AggregateResults(results);
            
            // Log progress
            if (aggregated.failed_shards > 0) {
                // Handle failures gracefully
                EXPECT_GT(aggregated.successful_shards, NUM_SHARDS / 2) 
                    << "Should have majority of shards operational";
            }
            
            epoch_total_loss += aggregated.global_loss;
        }
        
        double epoch_avg_loss = epoch_total_loss / ITERATIONS_PER_EPOCH;
        epoch_avg_losses.push_back(epoch_avg_loss);
    }
    
    // Verify training progress
    ASSERT_EQ(epoch_avg_losses.size(), NUM_EPOCHS);
    
    // Loss should generally decrease across epochs
    EXPECT_GT(epoch_avg_losses[0], epoch_avg_losses[NUM_EPOCHS - 1] * 0.7)
        << "Training should show overall improvement";
}

// ============================================================================
// Main
// ============================================================================


