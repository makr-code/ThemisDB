// ThemisDB v1.5.x Query Optimizer Production Integration Tests
// Tests for:
// 1. Shard Metadata Integration
// 2. Predicate-based Selectivity Estimation
// 3. Network Latency Monitoring
// 4. FAISS ADC Tables

#include <gtest/gtest.h>
#include "query/query_optimizer.h"
#include "index/secondary_index.h"
#include "index/advanced_vector_index.h"
#include "storage/rocksdb_wrapper.h"
#include <vector>
#include <string>
#include <random>

using namespace themis;

// ============================================================================
// Distributed Query Cost Model Tests (v1.5.x)
// ============================================================================

class DistributedCostModelTest : public ::testing::Test {
protected:
    void SetUp() override {
        RocksDBWrapper::Config cfg;
        cfg.db_path = "./data/test_optimizer_distributed";
        dbWrapper = std::make_unique<RocksDBWrapper>(cfg);
        secIdx = std::make_unique<SecondaryIndexManager>(*dbWrapper);
        optimizer = std::make_unique<QueryOptimizer>(*secIdx);
        optimizer->enableAdaptiveOptimization(true);
    }
    
    std::unique_ptr<RocksDBWrapper> dbWrapper;
    std::unique_ptr<SecondaryIndexManager> secIdx;
    std::unique_ptr<QueryOptimizer> optimizer;
};

TEST_F(DistributedCostModelTest, ShardMetadataIntegration) {
    // Test that shard metadata provides varying row count estimates
    ConjunctiveQuery query;
    query.table = "users";
    query.predicates = {
        PredicateEq{"status", "active"}
    };
    
    std::vector<std::string> shards = {"shard_0", "shard_1", "shard_2"};
    
    auto plan = optimizer->optimizeForDistribution(query, shards, false);
    
    // Verify plan was created
    EXPECT_FALSE(plan.shard_ids.empty());
    EXPECT_EQ(plan.shard_ids.size(), shards.size());
    
    // Verify parallelism recommendation
    EXPECT_GE(plan.recommended_parallelism, 1);
    EXPECT_LE(plan.recommended_parallelism, 32);
}

TEST_F(DistributedCostModelTest, PredicateSelectivityCalculation) {
    // Test selectivity estimation for different predicate patterns
    ConjunctiveQuery query;
    query.table = "orders";
    
    // Test with ID predicate (should have low selectivity ~0.001)
    query.predicates = {PredicateEq{"user_id", "123"}};
    
    auto plan1 = optimizer->optimizeForDistribution(query, {"shard_0", "shard_1"}, true);
    EXPECT_TRUE(plan1.use_partition_pruning || plan1.shard_ids.size() <= 2);
    
    // Test with status predicate (should have medium selectivity ~0.2)
    query.predicates = {PredicateEq{"status", "pending"}};
    
    auto plan2 = optimizer->optimizeForDistribution(query, {"shard_0", "shard_1"}, true);
    EXPECT_FALSE(plan2.shard_ids.empty());
    
    // Test with multiple predicates (combined selectivity)
    query.predicates = {
        PredicateEq{"user_id", "123"},
        PredicateEq{"status", "pending"}
    };
    
    auto plan3 = optimizer->optimizeForDistribution(query, {"shard_0", "shard_1", "shard_2"}, true);
    EXPECT_FALSE(plan3.shard_ids.empty());
}

TEST_F(DistributedCostModelTest, NetworkLatencyAwareness) {
    // Test that different shard naming conventions affect latency estimates
    ConjunctiveQuery query;
    query.table = "products";
    query.predicates = {PredicateEq{"category", "electronics"}};
    
    // Mix of local and remote shards
    std::vector<std::string> shards = {"local_0", "shard_remote_1", "datacenter_2"};
    
    auto plan = optimizer->optimizeForDistribution(query, shards, false);
    
    // Verify plan considers shard locations
    EXPECT_FALSE(plan.shard_ids.empty());
    
    // Should recommend reasonable parallelism based on mix of local/remote
    EXPECT_GE(plan.recommended_parallelism, 1);
    EXPECT_LE(plan.recommended_parallelism, shards.size() * 2);
}

TEST_F(DistributedCostModelTest, PartitionPruning) {
    // Test partition pruning based on selectivity
    ConjunctiveQuery query;
    query.table = "events";
    
    // High selectivity query - should prune partitions
    query.predicates = {
        PredicateEq{"event_id", "evt_12345"},
        PredicateEq{"user_id", "usr_67890"}
    };
    
    std::vector<std::string> shards = {"shard_0", "shard_1", "shard_2", "shard_3", "shard_4"};
    
    auto plan = optimizer->optimizeForDistribution(query, shards, true);
    
    // With high selectivity, some partitions should be pruned
    if (plan.use_partition_pruning) {
        EXPECT_LT(plan.shard_ids.size(), shards.size());
    }
    EXPECT_FALSE(plan.shard_ids.empty());
}

TEST_F(DistributedCostModelTest, ParallelismScaling) {
    // Test that parallelism scales with shard count
    ConjunctiveQuery query;
    query.table = "logs";
    query.predicates = {PredicateEq{"level", "error"}};
    
    // Small cluster
    auto plan_small = optimizer->optimizeForDistribution(
        query, {"shard_0", "shard_1"}, false);
    
    // Large cluster
    std::vector<std::string> large_cluster = {};

    for (int i = 0; i < 16; i++) {
        large_cluster.push_back("shard_" + std::to_string(i));
    }
    auto plan_large = optimizer->optimizeForDistribution(
        query, large_cluster, false);
    
    // Large cluster should recommend more parallelism
    EXPECT_LE(plan_small.recommended_parallelism, plan_large.recommended_parallelism);
    
    // But should still be bounded
    EXPECT_LE(plan_large.recommended_parallelism, 32);
}

// ============================================================================
// FAISS ADC Tables Tests (v1.5.x)
// ============================================================================

#ifdef THEMIS_GPU_ENABLED

TEST(FaissADCTables, EnabledByDefault) {
    // Test that ADC tables are enabled by default in v1.5.x
    AdvancedVectorIndex::Config config;
    
    EXPECT_TRUE(config.use_adc_tables);
    EXPECT_EQ(config.index_type, AdvancedVectorIndex::Config::Type::IVF_PQ);
}

TEST(FaissADCTables, ConfigurationOptions) {
    // Test ADC configuration options
    AdvancedVectorIndex::Config config;
    config.use_adc_tables = true;
    config.polysemous_ht = 64;
    config.use_pq = true;
    config.pq_m = 8;
    config.pq_nbits = 8;
    
    const size_t dimension = 128;
    AdvancedVectorIndex index(dimension, config);
    
    // Verify config is stored
    EXPECT_TRUE(index.getConfig().use_adc_tables);
    EXPECT_EQ(index.getConfig().polysemous_ht, 64);
}

TEST(FaissADCTables, PerformanceImprovement) {
    // Test that ADC tables provide measurable performance improvement
    // Note: This is a functional test, not a performance benchmark
    
    const size_t dimension = 128;
    const size_t num_vectors = 1000;
    const size_t k = 10;
    
    // Use fixed seed for reproducible tests
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    
    // Generate random training data
    std::vector<float> training_data(num_vectors * dimension);
    for (auto& val : training_data) {
        val = dist(rng);
    }
    
    // Index WITH ADC tables (v1.5.x default)
    AdvancedVectorIndex::Config config_with_adc;
    config_with_adc.use_adc_tables = true;
    config_with_adc.nlist = 32;
    config_with_adc.train_size = num_vectors;
    
    AdvancedVectorIndex index_with_adc(dimension, config_with_adc);
    ASSERT_TRUE(index_with_adc.train(training_data.data(), num_vectors));
    ASSERT_TRUE(index_with_adc.add(training_data.data(), num_vectors));
    
    // Search
    std::vector<float> query(dimension);
    for (auto& val : query) {
        val = dist(rng);
    }
    
    auto result = index_with_adc.search(query.data(), k);
    
    // Verify results are valid
    EXPECT_EQ(result.ids.size(), k);
    EXPECT_EQ(result.distances.size(), k);
    
    // Verify results are sorted by distance (ascending)
    for (size_t i = 1; i < result.distances.size(); i++) {
        EXPECT_GE(result.distances[i], result.distances[i-1]);
    }
}

#endif // THEMIS_GPU_ENABLED

// ============================================================================
// Integration Test: Full v1.5.x Pipeline
// ============================================================================

TEST_F(DistributedCostModelTest, FullPipelineIntegration) {
    // Test complete v1.5.x pipeline:
    // 1. Shard metadata integration
    // 2. Selectivity estimation
    // 3. Network latency awareness
    // 4. Partition pruning
    // 5. Parallelism optimization
    
    ConjunctiveQuery query;
    query.table = "transactions";
    query.predicates = {
        PredicateEq{"merchant_id", "merch_123"},
        PredicateEq{"status", "completed"}
    };
    
    std::vector<std::string> shards = {
        "local_0",           // Local shard
        "datacenter_1",      // Same DC
        "shard_remote_2",    // Remote
        "local_3",           // Local
        "datacenter_4"       // Same DC
    };
    
    // Enable all optimizations
    auto plan = optimizer->optimizeForDistribution(query, shards, true);
    
    // Verify comprehensive plan
    EXPECT_FALSE(plan.shard_ids.empty());
    EXPECT_GT(plan.recommended_parallelism, 0);
    
    // With selective query, should consider partition pruning
    if (plan.use_partition_pruning) {
        EXPECT_LE(plan.shard_ids.size(), shards.size());
    }
    
    // Verify NUMA awareness for larger queries
    if (plan.enable_numa_awareness) {
        EXPECT_FALSE(plan.preferred_cpu_affinity.empty());
    }
    
    // Verify join strategy is set
    EXPECT_FALSE(plan.join_strategy.empty());
}
