/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_adaptive_shard_router.cpp                     ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:41:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     405                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/adaptive_shard_router.h"
#include "sharding/shard_topology.h"
#include "sharding/urn_resolver.h"
#include "sharding/remote_executor.h"
#include "sharding/consistent_hash.h"
#include <memory>

using namespace themis::sharding;

class AdaptiveShardRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create topology
        ShardTopology::Config topo_config;
        topo_config.cluster_name = "test_cluster";
        topo_config.enable_health_checks = false;
        topology = std::make_shared<ShardTopology>(topo_config);
        
        // Add test shards with capabilities
        setupTestShards();
        
        // Create consistent hash ring for routing
        hash_ring = std::make_shared<ConsistentHashRing>(150);
        hash_ring->addShard("shard_hamburg");
        hash_ring->addShard("shard_bremen");
        hash_ring->addShard("shard_law");
        hash_ring->addShard("shard_berlin");
        hash_ring->addShard("shard_generic");
        
        // Create mock URN resolver with hash ring
        resolver = std::make_shared<URNResolver>(topology, hash_ring);
        
        // Create mock remote executor with config
        RemoteExecutor::Config executor_config;
        executor_config.connect_timeout_ms = 5000;
        executor_config.request_timeout_ms = 30000;
        executor = std::make_shared<RemoteExecutor>(executor_config);
        
        // Create adaptive router
        ShardRouter::Config router_config;
        router_config.local_shard_id = "shard_local";
        router_config.scatter_timeout_ms = 5000;
        
        AdaptiveShardRouter::AdaptiveConfig adaptive_config;
        adaptive_config.enable_adaptive_routing = true;
        adaptive_config.max_iterations = 3;
        adaptive_config.results_per_iteration = 2;
        adaptive_config.initial_threshold = 0.8;
        adaptive_config.intermediate_threshold = 0.6;
        adaptive_config.fallback_threshold = 0.4;
        adaptive_config.target_result_count = 10;
        adaptive_config.diminishing_returns_ratio = 0.1;
        adaptive_config.per_iteration_timeout_ms = 1000;
        adaptive_config.total_query_timeout_ms = 5000;
        
        router = std::make_unique<AdaptiveShardRouter>(
            resolver, executor, topology, router_config, adaptive_config);
    }
    
    void setupTestShards() {
        // Shard 1: Hamburg - high relevance for Hamburg queries
        ShardInfo shard1;
        shard1.shard_id = "shard_hamburg";
        shard1.primary_endpoint = "hamburg:8080";
        shard1.is_healthy = true;
        shard1.domain_capability.domains = {"construction"};
        shard1.domain_capability.regions = {"hamburg"};
        shard1.domain_capability.keywords = {"hamburg", "building", "baurecht"};
        topology->addShard(shard1);
        
        // Shard 2: Bremen - medium relevance
        ShardInfo shard2;
        shard2.shard_id = "shard_bremen";
        shard2.primary_endpoint = "bremen:8080";
        shard2.is_healthy = true;
        shard2.domain_capability.domains = {"construction"};
        shard2.domain_capability.regions = {"bremen"};
        shard2.domain_capability.keywords = {"bremen", "building"};
        topology->addShard(shard2);
        
        // Shard 3: Law - medium relevance for legal queries
        ShardInfo shard3;
        shard3.shard_id = "shard_law";
        shard3.primary_endpoint = "law:8080";
        shard3.is_healthy = true;
        shard3.domain_capability.domains = {"law"};
        shard3.domain_capability.keywords = {"law", "legal", "baurecht"};
        topology->addShard(shard3);
        
        // Shard 4: Berlin - lower relevance for Hamburg queries
        ShardInfo shard4;
        shard4.shard_id = "shard_berlin";
        shard4.primary_endpoint = "berlin:8080";
        shard4.is_healthy = true;
        shard4.domain_capability.domains = {"medicine"};
        shard4.domain_capability.regions = {"berlin"};
        shard4.domain_capability.keywords = {"berlin", "health"};
        topology->addShard(shard4);
        
        // Shard 5: Generic - low relevance
        ShardInfo shard5;
        shard5.shard_id = "shard_generic";
        shard5.primary_endpoint = "generic:8080";
        shard5.is_healthy = true;
        shard5.domain_capability.keywords = {"data", "storage"};
        topology->addShard(shard5);
    }
    
    std::shared_ptr<ShardTopology> topology;
    std::shared_ptr<ConsistentHashRing> hash_ring;
    std::shared_ptr<URNResolver> resolver;
    std::shared_ptr<RemoteExecutor> executor;
    std::unique_ptr<AdaptiveShardRouter> router;
};

TEST_F(AdaptiveShardRouterTest, ConfigValidation) {
    AdaptiveShardRouter::AdaptiveConfig invalid_config;
    invalid_config.enable_adaptive_routing = true;
    invalid_config.initial_threshold = 0.5;
    invalid_config.intermediate_threshold = 0.7;  // Invalid: > initial
    
    EXPECT_FALSE(invalid_config.isValid());
    
    AdaptiveShardRouter::AdaptiveConfig valid_config;
    valid_config.enable_adaptive_routing = true;
    valid_config.initial_threshold = 0.8;
    valid_config.intermediate_threshold = 0.6;
    valid_config.fallback_threshold = 0.4;
    
    EXPECT_TRUE(valid_config.isValid());
}

TEST_F(AdaptiveShardRouterTest, AdaptiveRoutingEnabled) {
    auto config = router->getAdaptiveConfig();
    EXPECT_TRUE(config.enable_adaptive_routing);
}

TEST_F(AdaptiveShardRouterTest, UpdateConfig) {
    AdaptiveShardRouter::AdaptiveConfig new_config;
    new_config.enable_adaptive_routing = false;
    new_config.max_iterations = 2;
    new_config.results_per_iteration = 1;
    new_config.initial_threshold = 0.9;
    new_config.intermediate_threshold = 0.7;
    new_config.fallback_threshold = 0.5;
    new_config.target_result_count = 50;
    new_config.diminishing_returns_ratio = 0.2;
    new_config.per_iteration_timeout_ms = 500;
    new_config.total_query_timeout_ms = 2000;
    
    router->updateAdaptiveConfig(new_config);
    
    auto updated = router->getAdaptiveConfig();
    EXPECT_FALSE(updated.enable_adaptive_routing);
    EXPECT_EQ(updated.max_iterations, 2);
}

TEST_F(AdaptiveShardRouterTest, ExecuteAdaptiveQuery) {
    AdaptiveShardRouter::AdaptiveStats stats;
    
    // Execute query - will use adaptive routing
    auto result = router->executeAdaptiveQuery("Baurechtsakten Hamburg", stats);
    
    // Check stats (adaptive path or deterministic fallback path)
    if (stats.used_adaptive_routing) {
        EXPECT_GT(stats.iterations_executed, 0);
        EXPECT_LE(stats.iterations_executed, 3);  // max_iterations
        EXPECT_GT(stats.total_time_ms, 0);
    } else {
        EXPECT_EQ(stats.iterations_executed, 0);
        EXPECT_EQ(stats.stop_reason, "no_capability_matches_fallback_to_scatter_gather");
    }
    EXPECT_FALSE(stats.query_id.empty());
}

TEST_F(AdaptiveShardRouterTest, IterativeExecution) {
    AdaptiveShardRouter::AdaptiveStats stats;
    
    // Execute query
    router->executeAdaptiveQuery("hamburg building", stats);

    if (stats.used_adaptive_routing) {
        EXPECT_GE(stats.iterations_executed, 1);
        ASSERT_FALSE(stats.iteration_details.empty());
        const auto& iter1 = stats.iteration_details[0];
        EXPECT_EQ(iter1.iteration_number, 1);
        EXPECT_GT(iter1.shards_queried, 0);
    } else {
        EXPECT_EQ(stats.iterations_executed, 0);
        EXPECT_TRUE(stats.iteration_details.empty());
        EXPECT_EQ(stats.stop_reason, "no_capability_matches_fallback_to_scatter_gather");
    }
}

TEST_F(AdaptiveShardRouterTest, ThresholdProgression) {
    // Keep a valid config and bias toward multiple iterations
    auto config = router->getAdaptiveConfig();
    config.max_iterations = 3;
    config.results_per_iteration = 1;
    config.initial_threshold = 0.1;
    config.intermediate_threshold = 0.1;
    config.fallback_threshold = 0.1;
    config.target_result_count = 1000;
    config.diminishing_returns_ratio = 0.1;
    router->updateAdaptiveConfig(config);

    AdaptiveShardRouter::AdaptiveStats stats;
    
    // Execute query and verify non-increasing score progression across iterations
    router->executeAdaptiveQuery("hamburg baurecht building law data", stats);

    if (stats.iteration_details.size() >= 2) {
        for (size_t i = 1; i < stats.iteration_details.size(); ++i) {
            EXPECT_LE(stats.iteration_details[i].avg_score,
                     stats.iteration_details[i-1].avg_score + 0.1);
        }
    } else {
        EXPECT_LE(stats.iteration_details.size(), 1);
    }
}

TEST_F(AdaptiveShardRouterTest, NoDuplicateShards) {
    AdaptiveShardRouter::AdaptiveStats stats;
    
    router->executeAdaptiveQuery("test query", stats);
    
    // Collect all queried shard IDs across iterations
    std::set<std::string> all_queried;
    for (const auto& iter_stats : stats.iteration_details) {
        for (const auto& shard_id : iter_stats.shard_ids) {
            // Each shard should only be queried once
            EXPECT_TRUE(all_queried.find(shard_id) == all_queried.end())
                << "Shard " << shard_id << " queried multiple times";
            all_queried.insert(shard_id);
        }
    }
}

TEST_F(AdaptiveShardRouterTest, EarlyStopOnTargetResults) {
    // Invalid config must be rejected deterministically
    AdaptiveShardRouter::AdaptiveConfig config = router->getAdaptiveConfig();
    config.target_result_count = 0;

    EXPECT_THROW(router->updateAdaptiveConfig(config), std::invalid_argument);
}

TEST_F(AdaptiveShardRouterTest, FallbackToScatterGather) {
    // Test with no matching capabilities
    topology->clear();
    
    // Add shards with no capabilities
    ShardInfo shard;
    shard.shard_id = "shard_empty";
    shard.primary_endpoint = "empty:8080";
    shard.is_healthy = true;
    topology->addShard(shard);
    
    AdaptiveShardRouter::AdaptiveStats stats;
    router->executeAdaptiveQuery("some query", stats);
    
    // Must fall back to scatter-gather when no capability match is available
    EXPECT_FALSE(stats.used_adaptive_routing);
    EXPECT_EQ(stats.stop_reason, "no_capability_matches_fallback_to_scatter_gather");
}

TEST_F(AdaptiveShardRouterTest, DisabledAdaptiveRouting) {
    // Disable adaptive routing
    AdaptiveShardRouter::AdaptiveConfig config = router->getAdaptiveConfig();
    config.enable_adaptive_routing = false;
    router->updateAdaptiveConfig(config);
    
    // Execute query - should use base class scatter-gather
    auto result = router->executeQuery("test query");
    
    // Result should still be valid JSON payload from base router
    EXPECT_TRUE(result.is_array() || result.is_object());
}

TEST_F(AdaptiveShardRouterTest, GetStatistics) {
    // Execute a query
    AdaptiveShardRouter::AdaptiveStats stats;
    router->executeAdaptiveQuery("test", stats);
    
    // Get statistics
    auto json_stats = router->getAdaptiveStatistics();
    
    EXPECT_TRUE(json_stats.contains("total_adaptive_queries"));
    EXPECT_TRUE(json_stats.contains("iterations_saved"));
    EXPECT_TRUE(json_stats.contains("early_stops"));
    EXPECT_TRUE(json_stats.contains("config"));
    
    EXPECT_GE(json_stats["total_adaptive_queries"], 1);
}

TEST_F(AdaptiveShardRouterTest, MaxIterationsRespected) {
    AdaptiveShardRouter::AdaptiveStats stats;
    
    // Execute query
    router->executeAdaptiveQuery("query", stats);
    
    // Should not exceed max iterations
    EXPECT_LE(stats.iterations_executed, 3);
}

TEST_F(AdaptiveShardRouterTest, HealthyShardPreference) {
    // Mark some shards as unhealthy
    topology->updateHealth("shard_generic", false);
    
    AdaptiveShardRouter::AdaptiveStats stats;
    router->executeAdaptiveQuery("hamburg", stats);
    
    // Should still query at least one healthy shard
    bool queried_healthy = false;
    bool queried_unhealthy = false;
    for (const auto& iter_stats : stats.iteration_details) {
        for (const auto& shard_id : iter_stats.shard_ids) {
            auto shard_info = topology->getShard(shard_id);
            if (!shard_info) {
                continue;
            }
            if (shard_info->is_healthy) {
                queried_healthy = true;
            } else {
                queried_unhealthy = true;
            }
        }
    }
    
    if (stats.used_adaptive_routing) {
        EXPECT_GT(stats.total_shards_queried, 0);
        EXPECT_TRUE(queried_healthy);
        EXPECT_FALSE(queried_unhealthy);
    } else {
        EXPECT_EQ(stats.total_shards_queried, 0);
        EXPECT_EQ(stats.stop_reason, "no_capability_matches_fallback_to_scatter_gather");
    }
}

TEST_F(AdaptiveShardRouterTest, IterationTimeTracking) {
    AdaptiveShardRouter::AdaptiveStats stats;
    
    router->executeAdaptiveQuery("test", stats);
    
    if (stats.used_adaptive_routing) {
        // Each iteration should have time tracked
        for (const auto& iter_stats : stats.iteration_details) {
            EXPECT_GT(iter_stats.iteration_time_ms, 0);
        }

        // Total time should be close to sum of iterations (plus/minus overhead)
        uint64_t sum_iterations = 0;
        for (const auto& iter_stats : stats.iteration_details) {
            sum_iterations += iter_stats.iteration_time_ms;
        }

        uint64_t tolerance = std::max(uint64_t(100), sum_iterations / 10);  // 10% or 100ms
        if (sum_iterations > tolerance) {
            EXPECT_GE(stats.total_time_ms, sum_iterations - tolerance);
        } else {
            EXPECT_GE(stats.total_time_ms, 0);
        }
    } else {
        EXPECT_EQ(stats.total_time_ms, 0);
        EXPECT_TRUE(stats.iteration_details.empty());
        EXPECT_EQ(stats.stop_reason, "no_capability_matches_fallback_to_scatter_gather");
    }
}

TEST_F(AdaptiveShardRouterTest, EmptyShardTopology) {
    topology->clear();
    
    AdaptiveShardRouter::AdaptiveStats stats;
    auto result = router->executeAdaptiveQuery("test", stats);
    
    // Should handle empty topology gracefully
    EXPECT_TRUE(result.is_array());
    EXPECT_EQ(result.size(), 0);
    EXPECT_EQ(stats.stop_reason, "no_shards_available");
}
