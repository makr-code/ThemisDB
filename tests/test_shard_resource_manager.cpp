#include <gtest/gtest.h>
#include "sharding/shard_resource_manager.h"
#include "sharding/shard_topology.h"
#include "sharding/gossip_config_manager.h"
#include <thread>

using namespace themis::sharding;

class ShardResourceManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto topology = std::make_shared<ShardTopology>();
        GossipConfigManagerConfig gossip_config;
        gossip_config.gossip_interval_ms = 10000; // Disable auto-gossip
        gossip_config.local_shard_id = "shard1";
        gossip_config.local_endpoint = "localhost:8001";
        gossip_manager_ = std::make_shared<GossipConfigManager>(
            gossip_config, topology
        );
    }
    
    std::shared_ptr<GossipConfigManager> gossip_manager_;
};

TEST_F(ShardResourceManagerTest, ManagerInitialization) {
    ShardResourceManager::Config config;
    config.snapshot_interval_ms = 1000;
    
    ShardResourceManager manager("shard1", gossip_manager_, config);
    EXPECT_FALSE(manager.isRunning());
}

TEST_F(ShardResourceManagerTest, SnapshotCollection) {
    ShardResourceManager manager("shard1", gossip_manager_);
    manager.start();
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto snapshot = manager.getCurrentSnapshot();
    EXPECT_GT(snapshot.health_score, 0.0f);
    EXPECT_LE(snapshot.health_score, 100.0f);
    
    manager.stop();
}

TEST_F(ShardResourceManagerTest, CanAcceptQueryThrottling) {
    ShardResourceManager::Config config;
    config.throttle_threshold = 0.5f; // 50% for testing
    
    ShardResourceManager manager("shard1", gossip_manager_, config);
    
    ShardResourceManager::QuerySpec small_query;
    small_query.estimated_memory_bytes = 100 * 1024 * 1024; // 100MB
    
    EXPECT_TRUE(manager.canAcceptQuery(small_query));
}

TEST_F(ShardResourceManagerTest, PeerResourceTracking) {
    ShardResourceManager manager("shard1", gossip_manager_);
    
    ShardResourceManager::ResourceSnapshot peer_snapshot;
    peer_snapshot.cpu_usage_percent = 75.0f;
    peer_snapshot.health_score = 80.0f;
    
    manager.receiveResourceUpdate("shard2", peer_snapshot);
    
    auto peer_resources = manager.getPeerResources();
    EXPECT_EQ(peer_resources.size(), 1);
    EXPECT_EQ(peer_resources["shard2"].cpu_usage_percent, 75.0f);
}

TEST_F(ShardResourceManagerTest, HealthyPeersFiltering) {
    ShardResourceManager manager("shard1", gossip_manager_);
    
    ShardResourceManager::ResourceSnapshot healthy;
    healthy.health_score = 95.0f;
    manager.receiveResourceUpdate("shard2", healthy);
    
    ShardResourceManager::ResourceSnapshot unhealthy;
    unhealthy.health_score = 30.0f;
    manager.receiveResourceUpdate("shard3", unhealthy);
    
    auto healthy_peers = manager.getHealthyPeers();
    EXPECT_EQ(healthy_peers.size(), 1);
    EXPECT_EQ(healthy_peers[0], "shard2");
}

TEST_F(ShardResourceManagerTest, ResourceSnapshotSerialization) {
    ShardResourceManager::ResourceSnapshot snapshot;
    snapshot.cpu_usage_percent = 50.5f;
    snapshot.ram_usage_bytes = 1024 * 1024 * 1024; // 1GB
    snapshot.ram_total_bytes = 4ULL * 1024 * 1024 * 1024; // 4GB
    snapshot.health_score = 85.0f;
    snapshot.active_queries = 10;
    snapshot.pending_queries = 5;
    snapshot.timestamp = std::chrono::system_clock::now();
    
    // Serialize to JSON
    auto json = snapshot.toJson();
    
    // Deserialize from JSON
    auto restored = ShardResourceManager::ResourceSnapshot::fromJson(json);
    
    EXPECT_FLOAT_EQ(restored.cpu_usage_percent, 50.5f);
    EXPECT_EQ(restored.ram_usage_bytes, 1024 * 1024 * 1024);
    EXPECT_EQ(restored.ram_total_bytes, 4ULL * 1024 * 1024 * 1024);
    EXPECT_FLOAT_EQ(restored.health_score, 85.0f);
    EXPECT_EQ(restored.active_queries, 10);
    EXPECT_EQ(restored.pending_queries, 5);
}

TEST_F(ShardResourceManagerTest, OverloadedPeersDetection) {
    ShardResourceManager manager("shard1", gossip_manager_);
    
    // Add a lightly loaded peer
    ShardResourceManager::ResourceSnapshot light;
    light.cpu_usage_percent = 30.0f;
    light.ram_usage_bytes = 1ULL * 1024 * 1024 * 1024; // 1GB
    light.ram_total_bytes = 8ULL * 1024 * 1024 * 1024; // 8GB
    manager.receiveResourceUpdate("shard2", light);
    
    // Add an overloaded peer
    ShardResourceManager::ResourceSnapshot overloaded;
    overloaded.cpu_usage_percent = 95.0f;
    overloaded.ram_usage_bytes = 7ULL * 1024 * 1024 * 1024; // 7GB
    overloaded.ram_total_bytes = 8ULL * 1024 * 1024 * 1024; // 8GB
    manager.receiveResourceUpdate("shard3", overloaded);
    
    auto overloaded_peers = manager.getOverloadedPeers(0.85f);
    EXPECT_EQ(overloaded_peers.size(), 1);
    EXPECT_EQ(overloaded_peers[0], "shard3");
}

TEST_F(ShardResourceManagerTest, UpdateQueryMetrics) {
    ShardResourceManager manager("shard1", gossip_manager_);
    
    manager.updateQueryMetrics(5, 3, 12.5f);
    
    auto snapshot = manager.getCurrentSnapshot();
    EXPECT_EQ(snapshot.active_queries, 5);
    EXPECT_EQ(snapshot.pending_queries, 3);
    EXPECT_FLOAT_EQ(snapshot.avg_query_latency_ms, 12.5f);
}

TEST_F(ShardResourceManagerTest, GetPeerResource) {
    ShardResourceManager manager("shard1", gossip_manager_);
    
    ShardResourceManager::ResourceSnapshot peer_snapshot;
    peer_snapshot.cpu_usage_percent = 60.0f;
    peer_snapshot.health_score = 75.0f;
    manager.receiveResourceUpdate("shard2", peer_snapshot);
    
    auto result = manager.getPeerResource("shard2");
    ASSERT_TRUE(result.has_value());
    EXPECT_FLOAT_EQ(result->cpu_usage_percent, 60.0f);
    EXPECT_FLOAT_EQ(result->health_score, 75.0f);
    
    auto missing = manager.getPeerResource("shard999");
    EXPECT_FALSE(missing.has_value());
}

#ifndef _WIN32
// Linux-specific test to verify CPU usage returns valid percentage
// Note: This tests the happy path. Error path (malformed /proc/stat) is validated
// via unit test of parsing logic (see standalone test_proc_stat_parsing.cpp)
TEST_F(ShardResourceManagerTest, CpuUsageReturnsValidPercentage) {
    ShardResourceManager manager("shard1", gossip_manager_);
    
    // Test that getCpuUsage() returns a valid value within expected range
    auto snapshot = manager.getCurrentSnapshot();
    
    // CPU usage should be a valid percentage (0-100)
    EXPECT_GE(snapshot.cpu_usage_percent, 0.0f);
    EXPECT_LE(snapshot.cpu_usage_percent, 100.0f);
    
    // Call it again to ensure differential calculation works
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    auto snapshot2 = manager.getCurrentSnapshot();
    
    EXPECT_GE(snapshot2.cpu_usage_percent, 0.0f);
    EXPECT_LE(snapshot2.cpu_usage_percent, 100.0f);
}
#endif
