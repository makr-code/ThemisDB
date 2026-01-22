/**
 * High Availability Tests for Replication Manager
 * 
 * Tests health monitoring, failure detection, and automatic failover
 */

#include <gtest/gtest.h>
#include "replication/replication_manager.h"
#include <filesystem>
#include <thread>
#include <chrono>

using namespace themisdb::replication;

// Test listener to capture events
class TestReplicationListener : public IReplicationListener {
public:
    std::vector<std::string> events;
    std::string last_leader_id;
    std::string last_failed_leader;
    std::string last_new_leader;
    bool failover_success = false;
    std::vector<std::string> last_partition_nodes;
    
    void onRoleChange(ReplicationRole old_role, ReplicationRole new_role) override {
        events.push_back("role_change");
    }
    
    void onLeaderElected(const std::string& leader_id) override {
        last_leader_id = leader_id;
        events.push_back("leader_elected:" + leader_id);
    }
    
    void onReplicaAdded(const ReplicaInfo& replica) override {
        events.push_back("replica_added:" + replica.node_id);
    }
    
    void onReplicaRemoved(const std::string& node_id) override {
        events.push_back("replica_removed:" + node_id);
    }
    
    void onConflictDetected(const std::string& document_id) override {
        events.push_back("conflict:" + document_id);
    }
    
    void onReplicationLagWarning(int64_t lag_ms) override {
        events.push_back("lag_warning:" + std::to_string(lag_ms));
    }
    
    void onReplicaHealthChanged(const std::string& node_id, HealthStatus old_status, HealthStatus new_status) override {
        events.push_back("health_changed:" + node_id);
    }
    
    void onFailoverStarted(const std::string& failed_leader_id, const std::string& new_leader_id) override {
        last_failed_leader = failed_leader_id;
        last_new_leader = new_leader_id;
        events.push_back("failover_started");
    }
    
    void onFailoverCompleted(const std::string& new_leader_id, bool success) override {
        failover_success = success;
        events.push_back("failover_completed:" + std::string(success ? "success" : "failed"));
    }
    
    void onNetworkPartitionDetected(const std::vector<std::string>& unreachable_nodes) override {
        last_partition_nodes = unreachable_nodes;
        events.push_back("network_partition");
    }
};

class ReplicationHATest : public ::testing::Test {
protected:
    void SetUp() override {
        test_dir_ = "/tmp/themis_replication_ha_test";
        std::filesystem::remove_all(test_dir_);
        std::filesystem::create_directories(test_dir_);
        
        // Configure replication
        config_.enabled = true;
        config_.mode = ReplicationMode::ASYNC;
        config_.wal_directory = test_dir_ + "/wal";
        config_.heartbeat_interval_ms = 100;  // Fast heartbeat for testing
        config_.failure_detection_timeout_ms = 500;  // Quick failure detection
        config_.degraded_lag_threshold_ms = 200;
        config_.enable_auto_failover = true;
        config_.max_consecutive_failures = 2;
        
        std::filesystem::create_directories(config_.wal_directory);
    }
    
    void TearDown() override {
        std::filesystem::remove_all(test_dir_);
    }
    
    std::string test_dir_;
    ReplicationConfig config_;
};

// ============================================================================
// Health Status Tests
// ============================================================================

TEST_F(ReplicationHATest, ReplicaHealthStatusTransitions) {
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:5001";
    replica.health_status = HealthStatus::UNKNOWN;
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    // Initially should be healthy with recent heartbeat
    replica.updateHealthStatus(500, 200);
    EXPECT_EQ(replica.health_status, HealthStatus::HEALTHY);
    EXPECT_EQ(replica.consecutive_failures, 0);
    
    // Simulate lag - should be degraded
    replica.last_heartbeat = std::chrono::system_clock::now() - std::chrono::milliseconds(300);
    replica.updateHealthStatus(500, 200);
    EXPECT_EQ(replica.health_status, HealthStatus::DEGRADED);
    
    // Simulate timeout - should be failed
    replica.last_heartbeat = std::chrono::system_clock::now() - std::chrono::milliseconds(600);
    replica.updateHealthStatus(500, 200);
    EXPECT_EQ(replica.health_status, HealthStatus::FAILED);
    EXPECT_GT(replica.consecutive_failures, 0);
}

TEST_F(ReplicationHATest, ReplicaHealthRecovery) {
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:5001";
    replica.last_heartbeat = std::chrono::system_clock::now() - std::chrono::milliseconds(600);
    replica.consecutive_failures = 3;
    replica.health_status = HealthStatus::FAILED;
    
    // Update heartbeat to recent time
    replica.last_heartbeat = std::chrono::system_clock::now();
    replica.updateHealthStatus(500, 200);
    
    // Should recover to healthy
    EXPECT_EQ(replica.health_status, HealthStatus::HEALTHY);
    EXPECT_EQ(replica.consecutive_failures, 0);
}

TEST_F(ReplicationHATest, ReplicationLagCalculation) {
    ReplicaInfo replica;
    replica.last_heartbeat = std::chrono::system_clock::now() - std::chrono::milliseconds(250);
    
    int64_t lag = replica.replicationLagMs();
    EXPECT_GE(lag, 240);  // Allow some timing variance
    EXPECT_LE(lag, 260);
}

// ============================================================================
// Manager Health Monitoring Tests
// ============================================================================

TEST_F(ReplicationHATest, ManagerInitialization) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    EXPECT_EQ(manager.getRole(), ReplicationRole::FOLLOWER);
    
    manager.shutdown();
}

TEST_F(ReplicationHATest, GetReplicaHealthStatus) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    // Add some replicas
    ReplicaInfo replica1;
    replica1.node_id = "replica-1";
    replica1.endpoint = "localhost:5001";
    replica1.health_status = HealthStatus::HEALTHY;
    replica1.last_heartbeat = std::chrono::system_clock::now();
    
    ReplicaInfo replica2;
    replica2.node_id = "replica-2";
    replica2.endpoint = "localhost:5002";
    replica2.health_status = HealthStatus::DEGRADED;
    replica2.last_heartbeat = std::chrono::system_clock::now() - std::chrono::milliseconds(300);
    
    manager.addReplica(replica1);
    manager.addReplica(replica2);
    
    auto health_status = manager.getReplicaHealthStatus();
    EXPECT_EQ(health_status.size(), 2);
    
    manager.shutdown();
}

TEST_F(ReplicationHATest, QuorumDetection) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    // Single node should have quorum (itself)
    EXPECT_TRUE(manager.hasQuorum());
    
    // Add voting members
    ReplicaInfo replica1;
    replica1.node_id = "replica-1";
    replica1.endpoint = "localhost:5001";
    replica1.is_voting_member = true;
    replica1.health_status = HealthStatus::HEALTHY;
    replica1.last_heartbeat = std::chrono::system_clock::now();
    
    manager.addReplica(replica1);
    
    // With 2 total voting members (self + replica1), quorum needs 2 healthy
    EXPECT_TRUE(manager.hasQuorum());
    
    manager.shutdown();
}

TEST_F(ReplicationHATest, NetworkPartitionDetection) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    // Add multiple replicas
    for (int i = 0; i < 5; ++i) {
        ReplicaInfo replica;
        replica.node_id = "replica-" + std::to_string(i);
        replica.endpoint = "localhost:500" + std::to_string(i);
        replica.health_status = (i < 3) ? HealthStatus::FAILED : HealthStatus::HEALTHY;
        replica.last_heartbeat = (i < 3) 
            ? std::chrono::system_clock::now() - std::chrono::seconds(10)
            : std::chrono::system_clock::now();
        
        manager.addReplica(replica);
    }
    
    // More than half failed = partition
    EXPECT_TRUE(manager.detectNetworkPartition());
    
    manager.shutdown();
}

// ============================================================================
// Read Preference Tests
// ============================================================================

TEST_F(ReplicationHATest, ReadPreferenceConfiguration) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    // Default should be PRIMARY_PREFERRED
    EXPECT_EQ(manager.getReadPreference(), ReadPreference::PRIMARY_PREFERRED);
    
    // Change to SECONDARY
    manager.setReadPreference(ReadPreference::SECONDARY);
    EXPECT_EQ(manager.getReadPreference(), ReadPreference::SECONDARY);
    
    // Change to NEAREST
    manager.setReadPreference(ReadPreference::NEAREST);
    EXPECT_EQ(manager.getReadPreference(), ReadPreference::NEAREST);
    
    manager.shutdown();
}

// ============================================================================
// Failover Tests
// ============================================================================

TEST_F(ReplicationHATest, ManualFailover) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    auto listener = std::make_shared<TestReplicationListener>();
    manager.addListener(listener);
    
    // Get initial stats
    auto stats_before = manager.getStats();
    uint64_t manual_failovers_before = stats_before.manual_failovers.load();
    
    // Trigger manual failover
    manager.triggerFailover("test-node-id");
    
    // Stats should be updated
    auto stats_after = manager.getStats();
    EXPECT_GT(stats_after.manual_failovers.load(), manual_failovers_before);
    
    manager.shutdown();
}

TEST_F(ReplicationHATest, PromoteToLeader) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    // Initially should be follower
    EXPECT_EQ(manager.getRole(), ReplicationRole::FOLLOWER);
    
    // Promote to leader
    bool promoted = manager.promoteToLeader();
    
    // After promotion should be leader
    if (promoted) {
        EXPECT_EQ(manager.getRole(), ReplicationRole::LEADER);
    }
    
    manager.shutdown();
}

// ============================================================================
// Statistics Tests
// ============================================================================

TEST_F(ReplicationHATest, HAStatisticsTracking) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    const auto& stats = manager.getStats();
    
    // Initial values should be zero
    EXPECT_EQ(stats.automatic_failovers.load(), 0);
    EXPECT_EQ(stats.manual_failovers.load(), 0);
    EXPECT_EQ(stats.replica_failures_detected.load(), 0);
    EXPECT_EQ(stats.network_partitions_detected.load(), 0);
    
    manager.shutdown();
}

TEST_F(ReplicationHATest, PrometheusMetricsExport) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    const auto& stats = manager.getStats();
    std::string metrics = stats.toPrometheusFormat();
    
    // Check that HA metrics are included
    EXPECT_NE(metrics.find("themisdb_automatic_failovers_total"), std::string::npos);
    EXPECT_NE(metrics.find("themisdb_manual_failovers_total"), std::string::npos);
    EXPECT_NE(metrics.find("themisdb_replica_failures_detected_total"), std::string::npos);
    EXPECT_NE(metrics.find("themisdb_network_partitions_detected_total"), std::string::npos);
    
    manager.shutdown();
}

// ============================================================================
// Event Listener Tests
// ============================================================================

TEST_F(ReplicationHATest, HealthChangeEvents) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    auto listener = std::make_shared<TestReplicationListener>();
    manager.addListener(listener);
    
    // Add a replica
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:5001";
    replica.health_status = HealthStatus::HEALTHY;
    replica.last_heartbeat = std::chrono::system_clock::now();
    
    manager.addReplica(replica);
    
    // Should have received add event
    EXPECT_GT(listener->events.size(), 0);
    bool found_add = false;
    for (const auto& event : listener->events) {
        if (event.find("replica_added") != std::string::npos) {
            found_add = true;
            break;
        }
    }
    EXPECT_TRUE(found_add);
    
    manager.shutdown();
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(ReplicationHATest, HAConfigurationDefaults) {
    ReplicationConfig config;
    
    // Check default HA settings
    EXPECT_TRUE(config.enable_auto_failover);
    EXPECT_EQ(config.failure_detection_timeout_ms, 5000);
    EXPECT_EQ(config.min_quorum_for_failover, 2);
    EXPECT_EQ(config.max_consecutive_failures, 3);
    EXPECT_EQ(config.degraded_lag_threshold_ms, 5000);
    EXPECT_EQ(config.default_read_preference, ReadPreference::PRIMARY_PREFERRED);
}

TEST_F(ReplicationHATest, CustomHAConfiguration) {
    ReplicationConfig config;
    config.enable_auto_failover = false;
    config.failure_detection_timeout_ms = 10000;
    config.min_quorum_for_failover = 3;
    config.max_consecutive_failures = 5;
    config.degraded_lag_threshold_ms = 8000;
    config.default_read_preference = ReadPreference::SECONDARY_PREFERRED;
    
    EXPECT_FALSE(config.enable_auto_failover);
    EXPECT_EQ(config.failure_detection_timeout_ms, 10000);
    EXPECT_EQ(config.min_quorum_for_failover, 3);
    EXPECT_EQ(config.max_consecutive_failures, 5);
    EXPECT_EQ(config.degraded_lag_threshold_ms, 8000);
    EXPECT_EQ(config.default_read_preference, ReadPreference::SECONDARY_PREFERRED);
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST_F(ReplicationHATest, HealthMonitoringLoop) {
    config_.heartbeat_interval_ms = 50;  // Very fast for testing
    
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    auto listener = std::make_shared<TestReplicationListener>();
    manager.addListener(listener);
    
    // Add a replica with old heartbeat
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:5001";
    replica.health_status = HealthStatus::HEALTHY;
    replica.last_heartbeat = std::chrono::system_clock::now() - std::chrono::milliseconds(600);
    replica.is_voting_member = true;
    
    manager.addReplica(replica);
    
    // Wait for health monitor to detect the failure
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Perform health check
    manager.performHealthCheck();
    
    // Get updated health status
    auto health_status = manager.getReplicaHealthStatus();
    EXPECT_EQ(health_status.size(), 1);
    
    manager.shutdown();
}

TEST_F(ReplicationHATest, ReplicationWithHealthMonitoring) {
    ReplicationManager manager(config_);
    EXPECT_TRUE(manager.initialize());
    
    // Promote to leader to enable replication
    manager.promoteToLeader();
    
    // Add a healthy replica
    ReplicaInfo replica;
    replica.node_id = "replica-1";
    replica.endpoint = "localhost:5001";
    replica.health_status = HealthStatus::HEALTHY;
    replica.last_heartbeat = std::chrono::system_clock::now();
    replica.is_voting_member = true;
    
    manager.addReplica(replica);
    
    // Create WAL entry
    WALEntry entry;
    entry.operation = "INSERT";
    entry.collection = "test_collection";
    entry.document_id = "doc-1";
    entry.data = R"({"name": "test", "value": 123})";
    entry.timestamp = std::chrono::system_clock::now();
    
    // Replicate if we're leader
    if (manager.getRole() == ReplicationRole::LEADER) {
        bool success = manager.replicate(entry);
        EXPECT_TRUE(success);
        
        // Check stats
        const auto& stats = manager.getStats();
        EXPECT_GT(stats.entries_replicated.load(), 0);
    }
    
    manager.shutdown();
}
