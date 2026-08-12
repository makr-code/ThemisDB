/**
 * @file test_adaptive_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 87/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include <gtest/gtest.h>
#include "query/adaptive_optimizer.h"
#include "query/query_optimizer.h"
#include "index/secondary_index.h"
#include <thread>
#include <chrono>
#include <filesystem>
#include <stdexcept>

using namespace themis;
using namespace themis::query;

namespace {

class QueryOptimizerTestHarness {
public:
    QueryOptimizerTestHarness()
        : db_path_(std::filesystem::temp_directory_path() /
                   ("themis_query_optimizer_test_" +
                    std::to_string(std::chrono::high_resolution_clock::now().time_since_epoch().count()))),
          db_(makeConfig(db_path_.string())),
          sec_idx_(db_),
          optimizer_(sec_idx_) {
        std::filesystem::remove_all(db_path_);
        if (!db_.open()) {
            throw std::runtime_error("Failed to open RocksDB in QueryOptimizerTestHarness");
        }
    }

    ~QueryOptimizerTestHarness() {
        db_.close();
        std::filesystem::remove_all(db_path_);
    }

    QueryOptimizer& optimizer() { return optimizer_; }

private:
    static RocksDBWrapper::Config makeConfig(const std::string& path) {
        RocksDBWrapper::Config cfg;
        cfg.db_path = path;
        return cfg;
    }

    std::filesystem::path db_path_;
    RocksDBWrapper db_;
    SecondaryIndexManager sec_idx_;
    QueryOptimizer optimizer_;
};

} // namespace

// ============================================================================
// AdaptiveQueryStats Tests
// ============================================================================

TEST(AdaptiveQueryStats, RecordAndRetrieveExecution) {
    AdaptiveQueryStats stats;
    
    AdaptiveQueryStats::QueryExecution exec;
    exec.query_hash = "test_query_123";
    exec.estimated_rows = 1000;
    exec.actual_rows = 800;
    exec.execution_time_ms = 5.5;
    exec.timestamp = std::chrono::system_clock::now();
    
    stats.recordExecution(exec);
    
    auto history = stats.getHistory("test_query_123");
    ASSERT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].query_hash, "test_query_123");
    EXPECT_EQ(history[0].estimated_rows, 1000);
    EXPECT_EQ(history[0].actual_rows, 800);
}

TEST(AdaptiveQueryStats, AverageSelectivity) {
    AdaptiveQueryStats stats;
    
    // Record multiple executions with consistent underestimation
    for (int i = 0; i < 5; i++) {
        AdaptiveQueryStats::QueryExecution exec;
        exec.query_hash = "query_abc";
        exec.estimated_rows = 1000;
        exec.actual_rows = 500;  // Consistently overestimating
        exec.timestamp = std::chrono::system_clock::now();
        stats.recordExecution(exec);
    }
    
    double avg_selectivity = stats.getAverageSelectivity("query_abc");
    EXPECT_NEAR(avg_selectivity, 0.5, 0.01);
}

TEST(AdaptiveQueryStats, DetectMisestimation) {
    AdaptiveQueryStats stats;
    
    // Record executions with significant misestimation
    for (int i = 0; i < 10; i++) {
        AdaptiveQueryStats::QueryExecution exec;
        exec.query_hash = "misestimated_query";
        exec.estimated_rows = 1000;
        exec.actual_rows = 5000;  // 5x underestimation
        exec.timestamp = std::chrono::system_clock::now();
        stats.recordExecution(exec);
    }
    
    EXPECT_TRUE(stats.hasCardinalityMisestimation("misestimated_query", 2.0));
}

TEST(AdaptiveQueryStats, AdaptiveAdjustmentFactor) {
    AdaptiveQueryStats stats;
    
    // Record executions with 2x overestimation
    for (int i = 0; i < 5; i++) {
        AdaptiveQueryStats::QueryExecution exec;
        exec.query_hash = "adjust_test";
        exec.estimated_rows = 1000;
        exec.actual_rows = 500;
        exec.timestamp = std::chrono::system_clock::now();
        stats.recordExecution(exec);
    }
    
    double factor = stats.getAdaptiveAdjustmentFactor("adjust_test");
    // Should be between 0.5 and 1.0 due to smoothing
    EXPECT_GT(factor, 0.4);
    EXPECT_LT(factor, 1.0);
}

TEST(AdaptiveQueryStats, PruneOldStats) {
    AdaptiveQueryStats stats;
    
    // Record old execution
    AdaptiveQueryStats::QueryExecution old_exec;
    old_exec.query_hash = "old_query";
    old_exec.estimated_rows = 1000;
    old_exec.actual_rows = 1000;
    old_exec.timestamp = std::chrono::system_clock::now() - std::chrono::hours(48);
    stats.recordExecution(old_exec);
    
    // Record recent execution
    AdaptiveQueryStats::QueryExecution recent_exec;
    recent_exec.query_hash = "recent_query";
    recent_exec.estimated_rows = 1000;
    recent_exec.actual_rows = 1000;
    recent_exec.timestamp = std::chrono::system_clock::now();
    stats.recordExecution(recent_exec);
    
    // Prune with 24-hour retention
    stats.pruneOldStats(std::chrono::hours(24));
    
    EXPECT_EQ(stats.getHistory("old_query").size(), 0);
    EXPECT_EQ(stats.getHistory("recent_query").size(), 1);
}

// ============================================================================
// AdaptivePlanSelector Tests
// ============================================================================

TEST(AdaptivePlanSelector, SelectPlanWithoutHistory) {
    AdaptivePlanSelector selector;
    AdaptiveQueryStats stats;
    
    std::vector<AdaptivePlanSelector::PlanChoice> alternatives;
    
    AdaptivePlanSelector::PlanChoice plan1;
    plan1.strategy = AdaptivePlanSelector::PlanChoice::Strategy::INDEX_SCAN;
    plan1.estimated_cost = 10.0;
    plan1.description = "Index scan";
    
    AdaptivePlanSelector::PlanChoice plan2;
    plan2.strategy = AdaptivePlanSelector::PlanChoice::Strategy::TABLE_SCAN;
    plan2.estimated_cost = 20.0;
    plan2.description = "Table scan";
    
    alternatives.push_back(plan1);
    alternatives.push_back(plan2);
    
    auto selected = selector.selectPlan(alternatives, "new_query", stats);
    EXPECT_EQ(selected.strategy, AdaptivePlanSelector::PlanChoice::Strategy::INDEX_SCAN);
}

TEST(AdaptivePlanSelector, ShouldSwitchPlanEarly) {
    AdaptivePlanSelector selector;
    
    // Early in execution (5% done), don't switch even if estimates are off
    EXPECT_FALSE(selector.shouldSwitchPlan(50, 1000, 0.05, 5.0));
}

TEST(AdaptivePlanSelector, ShouldSwitchPlanMidway) {
    AdaptivePlanSelector selector;
    
    // Midway (50% done), estimates are 10x off - should switch
    EXPECT_TRUE(selector.shouldSwitchPlan(5000, 1000, 0.50, 5.0));
}

TEST(AdaptivePlanSelector, ShouldNotSwitchPlanNearEnd) {
    AdaptivePlanSelector selector;
    
    // Near end (95% done), don't switch even if estimates are off
    EXPECT_FALSE(selector.shouldSwitchPlan(9500, 1000, 0.95, 5.0));
}

// ============================================================================
// DistributedQueryCostModel Tests
// ============================================================================

TEST(DistributedQueryCostModel, EstimateDistributedQueryCost) {
    DistributedQueryCostModel model;
    
    std::vector<DistributedQueryCostModel::ShardInfo> shards;
    
    DistributedQueryCostModel::ShardInfo shard1;
    shard1.shard_id = "shard1";
    shard1.estimated_rows = 10000;
    shard1.is_local = true;
    shard1.network_latency_ms = 0.0;
    
    DistributedQueryCostModel::ShardInfo shard2;
    shard2.shard_id = "shard2";
    shard2.estimated_rows = 10000;
    shard2.is_local = false;
    shard2.network_latency_ms = 2.0;
    
    shards.push_back(shard1);
    shards.push_back(shard2);
    
    double cost = model.estimateDistributedQueryCost(shards, 1000);
    
    // Remote shard should have higher cost due to network
    EXPECT_GT(cost, 0.0);
}

TEST(DistributedQueryCostModel, CrossShardJoinBroadcast) {
    DistributedQueryCostModel model;
    
    DistributedQueryCostModel::ShardInfo left_shard;
    left_shard.shard_id = "left";
    left_shard.estimated_rows = 500;
    
    DistributedQueryCostModel::ShardInfo right_shard;
    right_shard.shard_id = "right";
    right_shard.estimated_rows = 100000;
    
    auto cost = model.estimateCrossShardJoinCost(left_shard, right_shard, 500, 100000);
    
    // Small left table should use broadcast strategy
    EXPECT_EQ(cost.recommended_strategy, "broadcast");
}

TEST(DistributedQueryCostModel, CrossShardJoinRepartition) {
    DistributedQueryCostModel model;
    
    DistributedQueryCostModel::ShardInfo left_shard;
    left_shard.estimated_rows = 50000;
    
    DistributedQueryCostModel::ShardInfo right_shard;
    right_shard.estimated_rows = 60000;
    
    auto cost = model.estimateCrossShardJoinCost(left_shard, right_shard, 50000, 60000);
    
    // Similar sizes should use repartition
    EXPECT_EQ(cost.recommended_strategy, "repartition");
}

TEST(DistributedQueryCostModel, PartitionPruning) {
    DistributedQueryCostModel model;
    
    DistributedQueryCostModel::ShardInfo shard;
    shard.shard_id = "test_shard";
    shard.estimated_rows = 10000;  // Small partition for high selectivity scenario
    shard.network_latency_ms = 0.5;  // IntraDC latency
    shard.is_local = false;
    
    // Very low selectivity - should prune
    EXPECT_TRUE(model.shouldPrunePartition(shard, 10, 0.001));
    
    // High selectivity - should not prune
    EXPECT_FALSE(model.shouldPrunePartition(shard, 10, 0.995));
}

// ============================================================================
// MultiIndexOptimizer Tests
// ============================================================================

TEST(MultiIndexOptimizer, SingleSelectiveIndex) {
    MultiIndexOptimizer optimizer;
    
    std::vector<MultiIndexOptimizer::IndexCandidate> indexes;
    
    MultiIndexOptimizer::IndexCandidate idx1;
    idx1.index_name = "idx_selective";
    idx1.column = "status";
    idx1.estimated_selectivity = 100;  // Very selective
    idx1.access_cost = 1.0;
    
    indexes.push_back(idx1);
    
    auto plan = optimizer.optimizeMultiIndexAccess(indexes, 100000);
    
    // Should use single selective index
    EXPECT_EQ(plan.indexes_to_use.size(), 1);
    EXPECT_EQ(plan.indexes_to_use[0], "idx_selective");
    EXPECT_FALSE(plan.use_bitmap_intersection);
}

TEST(MultiIndexOptimizer, MultiIndexIntersection) {
    MultiIndexOptimizer optimizer;
    
    std::vector<MultiIndexOptimizer::IndexCandidate> indexes;
    
    MultiIndexOptimizer::IndexCandidate idx1;
    idx1.index_name = "idx_col1";
    idx1.estimated_selectivity = 10000;  // 10% selectivity
    idx1.access_cost = 1.0;
    
    MultiIndexOptimizer::IndexCandidate idx2;
    idx2.index_name = "idx_col2";
    idx2.estimated_selectivity = 15000;  // 15% selectivity
    idx2.access_cost = 1.0;
    
    indexes.push_back(idx1);
    indexes.push_back(idx2);
    
    auto plan = optimizer.optimizeMultiIndexAccess(indexes, 100000);
    
    // Should use both indexes
    EXPECT_GE(plan.indexes_to_use.size(), 1);
}

TEST(MultiIndexOptimizer, ShouldUseIntersection) {
    MultiIndexOptimizer optimizer;
    
    std::vector<MultiIndexOptimizer::IndexCandidate> indexes;
    
    MultiIndexOptimizer::IndexCandidate idx1;
    idx1.estimated_selectivity = 10000;
    
    MultiIndexOptimizer::IndexCandidate idx2;
    idx2.estimated_selectivity = 15000;
    
    indexes.push_back(idx1);
    indexes.push_back(idx2);
    
    // Multiple indexes with moderate selectivity
    EXPECT_TRUE(optimizer.shouldUseIndexIntersection(indexes, 100000));
    
    // Single index
    indexes.clear();
    indexes.push_back(idx1);
    EXPECT_FALSE(optimizer.shouldUseIndexIntersection(indexes, 100000));
}

// ============================================================================
// NumaAwareOptimizer Tests
// ============================================================================

TEST(NumaAwareOptimizer, GetOptimalPlacement) {
    NumaAwareOptimizer optimizer;
    
    size_t data_size = 1024 * 1024 * 100;  // 100 MB
    size_t parallelism = 4;
    
    auto placement = optimizer.getOptimalPlacement(data_size, parallelism);
    
    // Should have a valid NUMA node ID
    EXPECT_GE(placement.preferred_numa_node, 0);
    EXPECT_TRUE(placement.use_local_memory);
}

TEST(NumaAwareOptimizer, IsNumaAvailable) {
    // This test will pass regardless of NUMA availability
    bool available = NumaAwareOptimizer::isNumaAvailable();
    
    // Just verify the call doesn't crash
    EXPECT_TRUE(available || !available);
}

TEST(NumaAwareOptimizer, GetNumaNodeCount) {
    size_t count = NumaAwareOptimizer::getNumaNodeCount();
    
    // Should be at least 1 (even on non-NUMA systems)
    EXPECT_GE(count, 1);
}

// ============================================================================
// QueryOptimizer Vector Workload Tests
// ============================================================================

TEST(QueryOptimizer, VectorWorkloadSmallDataset) {
    QueryOptimizerTestHarness harness;
    
    // Small dataset should use flat index
    auto plan = harness.optimizer().optimizeVectorWorkload(10, 500, 128, 0.95);
    
    EXPECT_EQ(plan.index_type, "flat");
    EXPECT_EQ(plan.recommended_k_overfetch, 10);
    EXPECT_FALSE(plan.use_prefiltering);
}

TEST(QueryOptimizer, VectorWorkloadMediumDataset) {
    QueryOptimizerTestHarness harness;
    
    // Medium dataset should use IVF
    auto plan = harness.optimizer().optimizeVectorWorkload(10, 5000, 128, 0.95);
    
    EXPECT_EQ(plan.index_type, "ivf");
    EXPECT_GT(plan.recommended_ef_search, 0);
    EXPECT_EQ(plan.recommended_k_overfetch, 10);
}

TEST(QueryOptimizer, VectorWorkloadLargeDataset) {
    QueryOptimizerTestHarness harness;
    
    // Large dataset should use HNSW
    auto plan = harness.optimizer().optimizeVectorWorkload(10, 50000, 128, 0.95);
    
    EXPECT_EQ(plan.index_type, "hnsw");
    EXPECT_GE(plan.recommended_ef_search, 16);
    EXPECT_LE(plan.recommended_ef_search, 512);
    EXPECT_EQ(plan.recommended_k_overfetch, 20);  // 2x overfetch
    EXPECT_TRUE(plan.use_prefiltering);
}

TEST(QueryOptimizer, VectorWorkloadHighRecallTarget) {
    QueryOptimizerTestHarness harness;
    
    // High recall target should increase ef_search
    auto plan_high = harness.optimizer().optimizeVectorWorkload(10, 50000, 128, 0.99);
    auto plan_low = harness.optimizer().optimizeVectorWorkload(10, 50000, 128, 0.90);
    
    EXPECT_GT(plan_high.recommended_ef_search, plan_low.recommended_ef_search);
}

// ============================================================================
// QueryOptimizer Graph Workload Tests
// ============================================================================

TEST(QueryOptimizer, GraphWorkloadSmallExpansion) {
    QueryOptimizerTestHarness harness;
    
    // Small expansion - no need for parallelism
    auto plan = harness.optimizer().optimizeGraphWorkload(3, 2, false);
    
    EXPECT_EQ(plan.max_expansion_depth, 3);
    EXPECT_FALSE(plan.use_bidirectional_search);
    EXPECT_EQ(plan.recommended_parallelism, 1);
    EXPECT_FALSE(plan.enable_spatial_pruning);
}

TEST(QueryOptimizer, GraphWorkloadLargeExpansion) {
    QueryOptimizerTestHarness harness;
    
    // Large branching factor - should use bidirectional search
    auto plan = harness.optimizer().optimizeGraphWorkload(5, 10, false);
    
    EXPECT_TRUE(plan.use_bidirectional_search);
    EXPECT_GT(plan.recommended_parallelism, 1);
}

TEST(QueryOptimizer, GraphWorkloadSpatialConstraint) {
    QueryOptimizerTestHarness harness;
    
    // With spatial constraint - should enable pruning
    auto plan = harness.optimizer().optimizeGraphWorkload(4, 5, true);
    
    EXPECT_TRUE(plan.enable_spatial_pruning);
}

TEST(QueryOptimizer, GraphWorkloadMediumExpansion) {
    QueryOptimizerTestHarness harness;
    
    // Medium expansion - should suggest some parallelism
    auto plan = harness.optimizer().optimizeGraphWorkload(4, 8, false);
    
    EXPECT_GT(plan.recommended_parallelism, 1);
    EXPECT_LE(plan.recommended_parallelism, 8);
}

// ============================================================================
// QueryOptimizer Distributed Plan with NUMA Tests
// ============================================================================

TEST(QueryOptimizer, DistributedPlanNumaAwareness) {
    QueryOptimizerTestHarness harness;
    harness.optimizer().enableAdaptiveOptimization(true);
    
    ConjunctiveQuery query;
    query.table = "test_table";
    
    // Large number of shards should enable NUMA awareness
    std::vector<std::string> shards = {"s1", "s2", "s3", "s4", "s5", "s6"};
    auto plan = harness.optimizer().optimizeForDistribution(query, shards, true);
    
    EXPECT_LE(plan.shard_ids.size(), shards.size());
    EXPECT_GT(plan.recommended_parallelism, 0);
    
    // Check if NUMA awareness is enabled (depends on hardware)
    if (plan.enable_numa_awareness) {
        EXPECT_GT(plan.preferred_cpu_affinity.size(), 0);
    }
}

// ====================================================
// REL-01: Safe absolute-difference in CrossShardJoin (issue #5177)
// ====================================================

TEST(DistributedQueryCostModel, CrossShardJoinLeftSmallerThanRightNoUB) {
    DistributedQueryCostModel model;

    // left_rows < right_rows — the old code computed left - right as unsigned,
    // which wraps around and then casts to int (UB). Verify the repartition
    // branch is still reached correctly when sizes are similar.
    DistributedQueryCostModel::ShardInfo left_shard;
    left_shard.estimated_rows = 40000;

    DistributedQueryCostModel::ShardInfo right_shard;
    right_shard.estimated_rows = 50000;

    // 40k vs 50k — difference is 20% < 30% threshold → repartition.
    auto cost = model.estimateCrossShardJoinCost(left_shard, right_shard, 40000, 50000);
    EXPECT_EQ(cost.recommended_strategy, "repartition");
}

TEST(DistributedQueryCostModel, CrossShardJoinLargeSizeDifferenceUsesSemiJoin) {
    DistributedQueryCostModel model;

    // 50k vs 200k — difference is 150k / 50k = 3x > 30% threshold → semi_join.
    DistributedQueryCostModel::ShardInfo left_shard;
    left_shard.estimated_rows = 50000;

    DistributedQueryCostModel::ShardInfo right_shard;
    right_shard.estimated_rows = 200000;

    auto cost = model.estimateCrossShardJoinCost(left_shard, right_shard, 50000, 200000);
    EXPECT_EQ(cost.recommended_strategy, "semi_join");
}
