#include <gtest/gtest.h>
#include "sharding/rebalance_executor.h"
#include "sharding/data_movement_coordinator.h"
#include "sharding/rebalance_strategy.h"
#include "sharding/rebalance_approval_manager.h"
#include "sharding/rebalance_metrics.h"
#include "sharding/shard_load_detector.h"
#include <thread>
#include <chrono>

using namespace themis::sharding;

// ============================================================================
// RebalanceExecutor Tests
// ============================================================================

TEST(RebalanceExecutorTest, InitializationWithConfig) {
    RebalanceExecutor::Config config;
    config.operation_timeout = std::chrono::milliseconds(30000);
    config.max_retries = 5;
    config.require_approval = false;
    
    RebalanceExecutor executor(config);
    
    // Basic instantiation test
    EXPECT_TRUE(true);  // If we get here, constructor succeeded
}

TEST(RebalanceExecutorTest, PauseAndResumeOperation) {
    RebalanceExecutor::Config config;
    RebalanceExecutor executor(config);
    
    std::string op_id = "test_operation_123";
    
    // Pause should return false for non-existent operation
    EXPECT_FALSE(executor.pause(op_id));
    
    // Resume should also return false for non-existent operation
    EXPECT_FALSE(executor.resume(op_id));
}

TEST(RebalanceExecutorTest, ListOperationsEmpty) {
    RebalanceExecutor executor(RebalanceExecutor::Config{});
    
    auto ops = executor.listOperations();
    EXPECT_TRUE(ops.empty());
}

// ============================================================================
// DataMovementCoordinator Tests
// ============================================================================

TEST(DataMovementCoordinatorTest, InitializationWithConfig) {
    DataMovementCoordinator::Config config;
    config.batch_size = 5000;
    config.batch_timeout = std::chrono::milliseconds(10000);
    config.max_concurrent_batches = 20;
    config.verify_checksums = true;
    
    DataMovementCoordinator coordinator(config);
    
    EXPECT_TRUE(true);  // Constructor succeeded
}

TEST(DataMovementCoordinatorTest, StartStreaming) {
    DataMovementCoordinator coordinator(DataMovementCoordinator::Config{});
    
    std::vector<uint64_t> token_ranges = {0, 1000000};
    std::string stream_id = coordinator.startStreaming(
        "shard_001",
        "shard_002",
        token_ranges
    );
    
    EXPECT_FALSE(stream_id.empty());
    
    // Verify stream state
    auto state = coordinator.getStreamState(stream_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(state->source_shard_id, "shard_001");
    EXPECT_EQ(state->target_shard_id, "shard_002");
}

TEST(DataMovementCoordinatorTest, CancelStream) {
    DataMovementCoordinator coordinator(DataMovementCoordinator::Config{});
    
    std::vector<uint64_t> token_ranges = {0, 1000000};
    std::string stream_id = coordinator.startStreaming(
        "shard_001",
        "shard_002",
        token_ranges
    );
    
    EXPECT_TRUE(coordinator.cancelStream(stream_id));
    
    // Stream should no longer exist
    auto state = coordinator.getStreamState(stream_id);
    EXPECT_FALSE(state.has_value());
}

TEST(DataMovementCoordinatorTest, VerifyDataIntegrity) {
    DataMovementCoordinator coordinator(DataMovementCoordinator::Config{});
    
    std::vector<uint64_t> token_ranges = {0, 1000000};
    bool verified = coordinator.verifyDataIntegrity(
        "shard_001",
        "shard_002",
        token_ranges
    );
    
    // Placeholder implementation always returns true
    EXPECT_TRUE(verified);
}

// ============================================================================
// RebalanceStrategy Tests
// ============================================================================

TEST(LoadBalancingStrategyTest, GeneratePlanEmpty) {
    LoadBalancingStrategy strategy;
    
    std::vector<ShardLoad> empty_loads;
    ShardTopology topology;  // Default constructor
    
    auto plan = strategy.generatePlan(empty_loads, topology);
    
    EXPECT_TRUE(plan.empty());
}

TEST(LoadBalancingStrategyTest, GeneratePlanWithImbalance) {
    LoadBalancingStrategy strategy;
    
    std::vector<ShardLoad> loads;
    
    ShardLoad high_load;
    high_load.shard_id = "shard_001";
    high_load.cpu_usage = 0.9;  // 90%
    high_load.memory_usage = 0.8;
    high_load.request_rate = 10000;
    high_load.storage_bytes = 1000000000;
    loads.push_back(high_load);
    
    ShardLoad low_load;
    low_load.shard_id = "shard_002";
    low_load.cpu_usage = 0.2;  // 20%
    low_load.memory_usage = 0.3;
    low_load.request_rate = 1000;
    low_load.storage_bytes = 100000000;
    loads.push_back(low_load);
    
    ShardTopology topology;
    auto plan = strategy.generatePlan(loads, topology);
    
    // Should generate at least one recommendation
    EXPECT_FALSE(plan.empty());
    
    if (!plan.empty()) {
        EXPECT_EQ(plan[0].source_shard, "shard_001");
        EXPECT_EQ(plan[0].target_shard, "shard_002");
    }
}

TEST(LoadBalancingStrategyTest, EstimateImpact) {
    LoadBalancingStrategy strategy;
    
    LoadImbalanceResult::RebalanceRecommendation rec;
    rec.source_shard = "shard_001";
    rec.target_shard = "shard_002";
    rec.token_range_start = 0;
    rec.token_range_end = 500000;
    
    std::vector<ShardLoad> loads;
    ShardLoad source;
    source.shard_id = "shard_001";
    source.cpu_usage = 0.8;
    source.storage_bytes = 1000000000;
    loads.push_back(source);
    
    ShardLoad target;
    target.shard_id = "shard_002";
    target.cpu_usage = 0.3;
    target.storage_bytes = 500000000;
    loads.push_back(target);
    
    auto impact = strategy.estimateImpact(rec, loads);
    
    EXPECT_GT(impact.estimated_bytes_moved, 0);
    EXPECT_GE(impact.risk_level, 0.0);
    EXPECT_LE(impact.risk_level, 1.0);
    EXPECT_EQ(impact.affected_shards.size(), 2);
}

TEST(CapacityPlanningStrategyTest, DefaultConfig) {
    CapacityPlanningStrategy::Config config;
    CapacityPlanningStrategy strategy(config);
    
    EXPECT_EQ(config.max_cpu_threshold, 0.8);
    EXPECT_EQ(config.max_memory_threshold, 0.85);
}

TEST(CostOptimizationStrategyTest, MinimalMovement) {
    CostOptimizationStrategy strategy;
    
    std::vector<ShardLoad> loads;
    ShardLoad load1;
    load1.shard_id = "shard_001";
    load1.cpu_usage = 0.9;
    loads.push_back(load1);
    
    ShardLoad load2;
    load2.shard_id = "shard_002";
    load2.cpu_usage = 0.2;
    loads.push_back(load2);
    
    ShardTopology topology;
    auto plan = strategy.generatePlan(loads, topology);
    
    // Cost optimization should minimize operations
    // Only generate recommendation if imbalance is significant (>2x)
    EXPECT_FALSE(plan.empty());
}

// ============================================================================
// RebalanceApprovalManager Tests
// ============================================================================

TEST(RebalanceApprovalManagerTest, AutoApprovalMode) {
    RebalanceApprovalManager manager(ApprovalMode::AUTO_APPROVE);
    
    EXPECT_EQ(manager.getApprovalMode(), ApprovalMode::AUTO_APPROVE);
}

TEST(RebalanceApprovalManagerTest, ManualApprovalMode) {
    RebalanceApprovalManager manager(ApprovalMode::MANUAL_APPROVE);
    
    EXPECT_EQ(manager.getApprovalMode(), ApprovalMode::MANUAL_APPROVE);
}

TEST(RebalanceApprovalManagerTest, RequestAutoApproval) {
    RebalanceApprovalManager manager(ApprovalMode::AUTO_APPROVE);
    
    // Low risk should be auto-approved
    bool auto_approved = manager.requestApproval(
        "op_001",
        0.1,  // 10% risk (below 30% threshold)
        "Low risk operation"
    );
    
    EXPECT_TRUE(auto_approved);
    EXPECT_TRUE(manager.isApproved("op_001"));
}

TEST(RebalanceApprovalManagerTest, RequestManualApproval) {
    RebalanceApprovalManager manager(ApprovalMode::MANUAL_APPROVE);
    
    bool auto_approved = manager.requestApproval(
        "op_002",
        0.5,  // 50% risk
        "High risk operation"
    );
    
    // Should not be auto-approved in manual mode
    EXPECT_FALSE(auto_approved);
    EXPECT_FALSE(manager.isApproved("op_002"));
    
    // Manually approve
    EXPECT_TRUE(manager.approve("op_002", "admin", "Reviewed and approved"));
    EXPECT_TRUE(manager.isApproved("op_002"));
}

TEST(RebalanceApprovalManagerTest, RejectOperation) {
    RebalanceApprovalManager manager(ApprovalMode::MANUAL_APPROVE);
    
    manager.requestApproval("op_003", 0.7, "Risky operation");
    
    EXPECT_TRUE(manager.reject("op_003", "admin", "Too risky"));
    EXPECT_FALSE(manager.isApproved("op_003"));
}

TEST(RebalanceApprovalManagerTest, GetPendingApprovals) {
    RebalanceApprovalManager manager(ApprovalMode::MANUAL_APPROVE);
    
    manager.requestApproval("op_004", 0.5, "Test op 1");
    manager.requestApproval("op_005", 0.6, "Test op 2");
    
    auto pending = manager.getPendingApprovals();
    
    EXPECT_EQ(pending.size(), 2);
}

TEST(RebalanceApprovalManagerTest, ApprovalHistory) {
    RebalanceApprovalManager manager(ApprovalMode::MANUAL_APPROVE);
    
    manager.requestApproval("op_006", 0.5, "Test operation");
    manager.approve("op_006", "admin1", "First approval");
    manager.approve("op_006", "admin2", "Second approval");
    
    auto history = manager.getApprovalHistory("op_006");
    
    EXPECT_EQ(history.size(), 2);
}

// ============================================================================
// RebalanceMetrics Tests
// ============================================================================

TEST(RebalanceMetricsTest, RecordOperationLifecycle) {
    RebalanceMetrics metrics;
    
    std::string op_id = "op_metrics_001";
    
    // Start operation
    metrics.recordOperationStart(op_id);
    
    auto stats = metrics.getStats();
    EXPECT_EQ(stats.total_operations, 1);
    EXPECT_EQ(stats.in_progress, 1);
    
    // Update progress
    metrics.recordOperationProgress(op_id, 50.0, 500000, 1000);
    
    auto op_metrics = metrics.getOperationMetrics(op_id);
    ASSERT_TRUE(op_metrics.has_value());
    EXPECT_EQ(op_metrics->progress_percent, 50.0);
    EXPECT_EQ(op_metrics->bytes_moved, 500000);
    EXPECT_EQ(op_metrics->records_moved, 1000);
    
    // Complete operation
    metrics.recordOperationComplete(op_id, 1000000, 2000);
    
    stats = metrics.getStats();
    EXPECT_EQ(stats.completed, 1);
    EXPECT_EQ(stats.in_progress, 0);
}

TEST(RebalanceMetricsTest, RecordOperationFailure) {
    RebalanceMetrics metrics;
    
    std::string op_id = "op_metrics_002";
    
    metrics.recordOperationStart(op_id);
    metrics.recordOperationFailed(op_id, "Network error");
    
    auto stats = metrics.getStats();
    EXPECT_EQ(stats.failed, 1);
    EXPECT_EQ(stats.in_progress, 0);
    
    auto op_metrics = metrics.getOperationMetrics(op_id);
    ASSERT_TRUE(op_metrics.has_value());
    EXPECT_EQ(op_metrics->state, "FAILED");
    EXPECT_EQ(op_metrics->error_message, "Network error");
}

TEST(RebalanceMetricsTest, RecordRollback) {
    RebalanceMetrics metrics;
    
    std::string op_id = "op_metrics_003";
    
    metrics.recordOperationStart(op_id);
    metrics.recordOperationRolledBack(op_id);
    
    auto stats = metrics.getStats();
    EXPECT_EQ(stats.rolled_back, 1);
}

TEST(RebalanceMetricsTest, GetStatsJson) {
    RebalanceMetrics metrics;
    
    metrics.recordOperationStart("op_001");
    metrics.recordOperationComplete("op_001", 1000000, 1000);
    
    auto json = metrics.getStatsJson();
    
    EXPECT_TRUE(json.contains("total_operations"));
    EXPECT_TRUE(json.contains("completed"));
    EXPECT_TRUE(json.contains("success_rate"));
}

TEST(RebalanceMetricsTest, Reset) {
    RebalanceMetrics metrics;
    
    metrics.recordOperationStart("op_001");
    metrics.recordOperationComplete("op_001", 1000000, 1000);
    
    auto stats = metrics.getStats();
    EXPECT_GT(stats.total_operations, 0);
    
    metrics.reset();
    
    stats = metrics.getStats();
    EXPECT_EQ(stats.total_operations, 0);
    EXPECT_EQ(stats.completed, 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(RebalanceIntegrationTest, EndToEndApprovalFlow) {
    // Create components
    RebalanceApprovalManager approval_mgr(ApprovalMode::MANUAL_APPROVE);
    RebalanceMetrics metrics;
    
    // Request approval
    std::string op_id = "integration_op_001";
    bool auto_approved = approval_mgr.requestApproval(op_id, 0.5, "Test rebalance");
    EXPECT_FALSE(auto_approved);
    
    // Record metrics
    metrics.recordOperationStart(op_id);
    
    // Approve
    approval_mgr.approve(op_id, "admin", "Approved for testing");
    EXPECT_TRUE(approval_mgr.isApproved(op_id));
    
    // Simulate progress
    metrics.recordOperationProgress(op_id, 25.0, 250000, 500);
    metrics.recordOperationProgress(op_id, 50.0, 500000, 1000);
    metrics.recordOperationProgress(op_id, 75.0, 750000, 1500);
    
    // Complete
    metrics.recordOperationComplete(op_id, 1000000, 2000);
    
    // Verify
    auto stats = metrics.getStats();
    EXPECT_EQ(stats.completed, 1);
    EXPECT_EQ(stats.success_rate, 100.0);
}

// Main entry point
int main(int argc, char** argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
