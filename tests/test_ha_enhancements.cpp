/**
 * High Availability Enhancement Tests
 * Tests for multi-region replication, cascading replication, and failover
 */

#include <gtest/gtest.h>
#include "replication/replication_manager.h"
#include <thread>
#include <chrono>
#include <filesystem>

using namespace themisdb::replication;

class HAEnhancementTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Setup replication configuration
        config_.enabled = true;
        config_.mode = ReplicationMode::ASYNC;
        config_.wal_directory = "./data/test_ha_wal";
        config_.heartbeat_interval_ms = 100;
        config_.replication_timeout_ms = 1000;
        
        // Create replication manager
        manager_ = std::make_unique<ReplicationManager>(config_);
    }
    
    void TearDown() override {
        if (manager_) {
            manager_->shutdown();
        }
        manager_.reset();
        
        // Cleanup
        std::filesystem::remove_all(config_.wal_directory);
    }
    
    ReplicationConfig config_;
    std::unique_ptr<ReplicationManager> manager_;
};

// Test multi-region replication setup
TEST_F(HAEnhancementTest, MultiRegionSetup) {
    ASSERT_TRUE(manager_->initialize());
    
    // Enable multi-region replication
    std::string region_id = "us-east-1";
    std::vector<std::string> peer_regions = {
        "us-west-2:18765",
        "eu-central-1:18765",
        "ap-southeast-1:18765"
    };
    
    EXPECT_TRUE(manager_->enableMultiRegion(region_id, peer_regions));
    
    // Verify replicas were added
    auto replicas = manager_->getReplicas();
    EXPECT_GE(replicas.size(), peer_regions.size());
}

// Test replica promotion
TEST_F(HAEnhancementTest, ReplicaPromotion) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add a replica
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:18766";
    replica.role = ReplicationRole::FOLLOWER;
    replica.last_heartbeat = std::chrono::system_clock::now();
    replica.is_voting_member = true;
    
    manager_->addReplica(replica);
    
    // Promote replica to primary
    EXPECT_TRUE(manager_->promoteReplica("replica-1"));
}

// Test cascading replication
TEST_F(HAEnhancementTest, CascadingReplication) {
    ASSERT_TRUE(manager_->initialize());
    
    // Setup cascading replication topology
    std::string source_replica = "replica-1";
    std::vector<std::string> target_replicas = {
        "replica-2",
        "replica-3"
    };
    
    EXPECT_TRUE(manager_->setupCascadingReplication(source_replica, target_replicas));
}

// Test replication lag monitoring
TEST_F(HAEnhancementTest, ReplicationLagMonitoring) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add replicas with different lag
    ReplicaInfo replica1;
    replica1.node_id = "replica-1";
    replica1.endpoint = "localhost:18766";
    replica1.role = ReplicationRole::FOLLOWER;
    replica1.last_heartbeat = std::chrono::system_clock::now();
    
    manager_->addReplica(replica1);
    
    // Get replication lag
    int64_t lag = manager_->getReplicationLag("replica-1");
    
    // Lag should be measurable (>= 0)
    EXPECT_GE(lag, 0);
}

// Test cluster health checks
TEST_F(HAEnhancementTest, ClusterHealth) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add some replicas
    for (int i = 0; i < 3; i++) {
        ReplicaInfo replica;
        replica.node_id = "replica-" + std::to_string(i);
        replica.endpoint = "localhost:" + std::to_string(18766 + i);
        replica.role = ReplicationRole::FOLLOWER;
        replica.last_heartbeat = std::chrono::system_clock::now();
        
        manager_->addReplica(replica);
    }
    
    // Get cluster health
    auto health = manager_->getClusterHealth();
    
    // Should have entries for all nodes (self + replicas)
    EXPECT_GE(health.size(), 1);
    
    // Verify health status format
    for (const auto& [node_id, is_healthy] : health) {
        EXPECT_FALSE(node_id.empty());
        // Health can be true or false, just checking it's set
    }
}

// Test Prometheus metrics export
TEST_F(HAEnhancementTest, PrometheusMetrics) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add some replicas
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:18766";
    replica.role = ReplicationRole::FOLLOWER;
    replica.datacenter = "us-east-1";
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    manager_->addReplica(replica);
    
    // Export Prometheus metrics
    std::string metrics = manager_->exportPrometheusMetrics();
    
    // Verify metrics format
    EXPECT_FALSE(metrics.empty());
    EXPECT_NE(metrics.find("themisdb_"), std::string::npos);
    EXPECT_NE(metrics.find("# HELP"), std::string::npos);
    EXPECT_NE(metrics.find("# TYPE"), std::string::npos);
}

// Test manual failover
TEST_F(HAEnhancementTest, ManualFailover) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add target replica
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:18766";
    replica.role = ReplicationRole::FOLLOWER;
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    manager_->addReplica(replica);
    
    // Trigger failover to specific node
    EXPECT_TRUE(manager_->triggerFailover("replica-1"));
}

// Test automatic leader election
TEST_F(HAEnhancementTest, LeaderElection) {
    ASSERT_TRUE(manager_->initialize());
    
    // Initially should be a follower
    auto initial_role = manager_->getRole();
    
    // Promote to leader
    EXPECT_TRUE(manager_->promoteToLeader());
    
    // Verify role changed
    auto new_role = manager_->getRole();
    EXPECT_EQ(new_role, ReplicationRole::LEADER);
}

// Test leader demotion
TEST_F(HAEnhancementTest, LeaderDemotion) {
    ASSERT_TRUE(manager_->initialize());
    
    // Promote to leader first
    manager_->promoteToLeader();
    EXPECT_EQ(manager_->getRole(), ReplicationRole::LEADER);
    
    // Demote to follower
    EXPECT_TRUE(manager_->demoteToFollower());
}

// Test replica addition and removal
TEST_F(HAEnhancementTest, ReplicaManagement) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add replica
    ReplicaInfo replica;
    replica.node_id = "test-replica";
    replica.endpoint = "localhost:18766";
    replica.role = ReplicationRole::FOLLOWER;
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    manager_->addReplica(replica);
    
    auto replicas_after_add = manager_->getReplicas();
    size_t count_after_add = replicas_after_add.size();
    
    // Remove replica
    manager_->removeReplica("test-replica");
    
    auto replicas_after_remove = manager_->getReplicas();
    EXPECT_LT(replicas_after_remove.size(), count_after_add);
}

// Test replication with failover
TEST_F(HAEnhancementTest, ReplicationWithFailover) {
    ASSERT_TRUE(manager_->initialize());
    
    // Promote to leader to accept writes
    manager_->promoteToLeader();
    
    // Add a replica
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:18766";
    replica.role = ReplicationRole::FOLLOWER;
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    manager_->addReplica(replica);
    
    // Replicate a write
    WALEntry entry;
    entry.operation = "INSERT";
    entry.collection = "test_collection";
    entry.document_id = "doc1";
    entry.data = "{\"field\": \"value\"}";
    entry.timestamp = std::chrono::system_clock::now();
    
    // This will append to WAL
    bool result = manager_->replicate(entry);
    EXPECT_TRUE(result);
    
    // Get stats
    const auto& stats = manager_->getStats();
    EXPECT_GT(stats.entries_replicated.load(), 0);
}

// Test multi-datacenter replication
TEST_F(HAEnhancementTest, MultiDatacenterReplication) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add replicas from different datacenters
    std::vector<std::string> datacenters = {"dc1", "dc2", "dc3"};
    
    for (size_t i = 0; i < datacenters.size(); i++) {
        ReplicaInfo replica;
        replica.node_id = "replica-dc" + std::to_string(i);
        replica.endpoint = "dc" + std::to_string(i) + ".example.com:18765";
        replica.datacenter = datacenters[i];
        replica.role = ReplicationRole::FOLLOWER;
        replica.last_heartbeat = std::chrono::system_clock::now();
        replica.priority = 10 - static_cast<int32_t>(i);  // Higher priority for dc1
        
        manager_->addReplica(replica);
    }
    
    // Verify all replicas added
    auto replicas = manager_->getReplicas();
    EXPECT_GE(replicas.size(), datacenters.size());
}

// Test replication statistics
TEST_F(HAEnhancementTest, ReplicationStatistics) {
    ASSERT_TRUE(manager_->initialize());
    manager_->promoteToLeader();
    
    // Replicate multiple entries
    for (int i = 0; i < 10; i++) {
        WALEntry entry;
        entry.operation = "INSERT";
        entry.collection = "test_collection";
        entry.document_id = "doc" + std::to_string(i);
        entry.data = "{\"id\": " + std::to_string(i) + "}";
        entry.timestamp = std::chrono::system_clock::now();
        
        manager_->replicate(entry);
    }
    
    // Check statistics
    const auto& stats = manager_->getStats();
    EXPECT_GE(stats.entries_replicated.load(), 10);
}

// Test heartbeat mechanism
TEST_F(HAEnhancementTest, HeartbeatMechanism) {
    ASSERT_TRUE(manager_->initialize());
    
    // Add replica
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:18766";
    replica.role = ReplicationRole::FOLLOWER;
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    manager_->addReplica(replica);
    
    // Wait for heartbeat interval
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Replica should still be considered "recent" since we just created it
    auto replicas = manager_->getReplicas();
    EXPECT_FALSE(replicas.empty());
}


