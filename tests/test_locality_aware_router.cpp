#include <gtest/gtest.h>
#include "sharding/locality_aware_router.h"

using namespace themis::sharding;

class LocalityAwareRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        topology_ = std::make_shared<ShardTopology>();
        
        // Add test shards
        for (int i = 1; i <= 5; ++i) {
            ShardInfo shard;
            shard.shard_id = "shard" + std::to_string(i);
            shard.primary_endpoint = "localhost:5000" + std::to_string(i);
            shard.datacenter = (i <= 3) ? "dc1" : "dc2";
            shard.is_healthy = true;
            topology_->addShard(shard);
        }
        
        GossipConfigManagerConfig gossip_config;
        gossip_config.gossip_interval_ms = 10000; // Disable auto-gossip for tests
        gossip_config.local_shard_id = "shard1";
        gossip_config.local_endpoint = "localhost:8001";
        
        auto gossip_mgr = std::make_shared<GossipConfigManager>(gossip_config, topology_);
        resource_mgr_ = std::make_shared<ShardResourceManager>("shard1", gossip_mgr);
    }
    
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<ShardResourceManager> resource_mgr_;
};

TEST_F(LocalityAwareRouterTest, RouterInitialization) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    auto stats = router.getStatistics();
    EXPECT_EQ(stats.queries_routed.load(), 0);
}

TEST_F(LocalityAwareRouterTest, LocalDataPreference) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    // Register local data
    router.updateDataPlacement("users", "user:123", "shard1");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.query_aql = "FOR u IN users FILTER u._key == '123' RETURN u";
    spec.accessed_collections = {"users"};
    spec.accessed_keys = {"user:123"};
    
    std::string target = router.routeQuery(spec);
    EXPECT_EQ(target, "shard1");  // Should prefer local shard
}

TEST_F(LocalityAwareRouterTest, RemoteDataRouting) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    // Data is on shard2, not local
    router.updateDataPlacement("orders", "order:456", "shard2");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"orders"};
    spec.accessed_keys = {"order:456"};
    
    std::string target = router.routeQuery(spec);
    EXPECT_EQ(target, "shard2");  // Should route to where data is
}

TEST_F(LocalityAwareRouterTest, LoadBalancing) {
    LocalityAwareRouter::Config config;
    config.load_weight = 0.7f;      // Prioritize load over locality
    config.locality_weight = 0.3f;
    
    LocalityAwareRouter router("shard1", topology_, resource_mgr_, config);
    
    // Simulate shard2 being overloaded
    ShardResourceManager::ResourceSnapshot overloaded;
    overloaded.cpu_usage_percent = 95.0f;
    overloaded.health_score = 30.0f;
    resource_mgr_->receiveResourceUpdate("shard2", overloaded);
    
    // Even if data is on shard2, should route elsewhere due to load
    router.updateDataPlacement("products", "prod:789", "shard2");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"products"};
    spec.accessed_keys = {"prod:789"};
    
    std::string target = router.routeQuery(spec);
    EXPECT_NE(target, "shard2");  // Should avoid overloaded shard
}

TEST_F(LocalityAwareRouterTest, AffinityScoreCalculation) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    router.updateDataPlacement("users", "user:1", "shard1");
    router.updateDataPlacement("users", "user:2", "shard1");
    router.updateDataPlacement("users", "user:3", "shard2");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users"};
    spec.accessed_keys = {"user:1", "user:2", "user:3"};
    
    auto affinities = router.computeAffinity(spec);
    
    // shard1 has 2/3 of data, should have higher locality score
    auto shard1_affinity = std::find_if(affinities.begin(), affinities.end(),
        [](const auto& a) { return a.shard_id == "shard1"; });
    
    auto shard2_affinity = std::find_if(affinities.begin(), affinities.end(),
        [](const auto& a) { return a.shard_id == "shard2"; });
    
    ASSERT_NE(shard1_affinity, affinities.end());
    ASSERT_NE(shard2_affinity, affinities.end());
    
    EXPECT_GT(shard1_affinity->locality_score, shard2_affinity->locality_score);
}

TEST_F(LocalityAwareRouterTest, DataPlacementCache) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    router.updateDataPlacement("collection1", "key1", "shard2");
    
    EXPECT_TRUE(router.hasData("shard2", "collection1", "key1"));
    EXPECT_FALSE(router.hasData("shard3", "collection1", "key1"));
    
    router.removeDataPlacement("collection1", "key1");
    
    EXPECT_FALSE(router.hasData("shard2", "collection1", "key1"));
}

TEST_F(LocalityAwareRouterTest, MultipleCollections) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    // Data spread across multiple shards
    router.updateDataPlacement("users", "user:1", "shard1");
    router.updateDataPlacement("orders", "order:1", "shard2");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users", "orders"};
    spec.accessed_keys = {"user:1", "order:1"};
    
    auto target = router.routeQuery(spec);
    
    // Should route to one of the shards with data
    EXPECT_TRUE(target == "shard1" || target == "shard2");
}

TEST_F(LocalityAwareRouterTest, NoDataInCache) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"unknown"};
    spec.accessed_keys = {"unknown:key"};
    
    // Should fallback to some shard (likely local)
    auto target = router.routeQuery(spec);
    EXPECT_FALSE(target.empty());
}

TEST_F(LocalityAwareRouterTest, MultiShardRouting) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    // Data spread across multiple shards
    router.updateDataPlacement("users", "user:1", "shard1");
    router.updateDataPlacement("users", "user:2", "shard2");
    router.updateDataPlacement("users", "user:3", "shard3");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users"};
    spec.accessed_keys = {"user:1", "user:2", "user:3"};
    
    auto targets = router.routeMultiShardQuery(spec);
    
    // Should return multiple shards
    EXPECT_GE(targets.size(), 2);
}

TEST_F(LocalityAwareRouterTest, StatisticsTracking) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    router.updateDataPlacement("users", "user:1", "shard1");
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"users"};
    spec.accessed_keys = {"user:1"};
    
    // Route multiple queries
    for (int i = 0; i < 5; ++i) {
        router.routeQuery(spec);
    }
    
    auto stats = router.getStatistics();
    EXPECT_EQ(stats.queries_routed.load(), 5);
    EXPECT_GT(stats.local_routes.load(), 0);
}

TEST_F(LocalityAwareRouterTest, NetworkScoring) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"test"};
    
    // Compute affinity for shards in same DC vs different DC
    auto affinity_dc1 = router.computeShardAffinity("shard2", spec); // Same DC
    auto affinity_dc2 = router.computeShardAffinity("shard4", spec); // Different DC
    
    // Same datacenter should have better network score
    EXPECT_GT(affinity_dc1.network_score, affinity_dc2.network_score);
}

TEST_F(LocalityAwareRouterTest, LocalShardBonus) {
    LocalityAwareRouter::Config config;
    config.prefer_local_shard = true;
    config.local_shard_bonus = 0.2f;
    
    LocalityAwareRouter router("shard1", topology_, resource_mgr_, config);
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"test"};
    
    auto local_affinity = router.computeShardAffinity("shard1", spec);
    auto remote_affinity = router.computeShardAffinity("shard2", spec);
    
    // Local shard should have bonus applied
    // Even with same base scores, local should be higher
    EXPECT_GE(local_affinity.combined_score, remote_affinity.combined_score);
}

TEST_F(LocalityAwareRouterTest, JsonSerialization) {
    LocalityAwareRouter router("shard1", topology_, resource_mgr_);
    
    LocalityAwareRouter::QuerySpec spec;
    spec.accessed_collections = {"test"};
    
    router.routeQuery(spec);
    
    auto json = router.getStatisticsJson();
    
    EXPECT_TRUE(json.contains("queries_routed"));
    EXPECT_TRUE(json.contains("local_routes"));
    EXPECT_TRUE(json.contains("remote_routes"));
}

TEST_F(LocalityAwareRouterTest, CacheDisabled) {
    LocalityAwareRouter::Config config;
    config.enable_placement_cache = false;
    
    LocalityAwareRouter router("shard1", topology_, resource_mgr_, config);
    
    router.updateDataPlacement("users", "user:1", "shard1");
    
    // With cache disabled, should not find data
    EXPECT_FALSE(router.hasData("shard1", "users", "user:1"));
}
