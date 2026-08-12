#include "sharding/partition_detector.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

class PartitionDetectorTest : public ::testing::Test {
protected:
    PartitionDetectorConfig createConfig() {
        PartitionDetectorConfig config;
        config.health_check_interval = 50ms;
        config.heartbeat_timeout = 100ms;
        config.max_consecutive_failures = 2;
        config.enable_split_brain_detection = true;
        config.enable_auto_healing = true;
        return config;
    }
};

TEST_F(PartitionDetectorTest, InitialState) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    EXPECT_EQ(detector.getNetworkHealth(), NetworkHealth::HEALTHY);
    EXPECT_FALSE(detector.isSplitBrainDetected());
}

TEST_F(PartitionDetectorTest, AddRemoveNodes) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.addNode("node2");
    detector.addNode("node3");
    
    auto connectivity = detector.getNodeConnectivity();
    EXPECT_EQ(connectivity.size(), 3);
    
    detector.removeNode("node2");
    connectivity = detector.getNodeConnectivity();
    EXPECT_EQ(connectivity.size(), 2);
}

TEST_F(PartitionDetectorTest, RecordHeartbeat) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.recordHeartbeat("node1", 10ms);
    
    auto connectivity = detector.getNodeConnectivity();
    ASSERT_EQ(connectivity.size(), 1);
    EXPECT_TRUE(connectivity[0].reachable);
    EXPECT_EQ(connectivity[0].last_rtt, 10ms);
}

TEST_F(PartitionDetectorTest, RecordFailures) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    
    // Record multiple failures
    detector.recordFailure("node1");
    detector.recordFailure("node1");
    
    auto connectivity = detector.getNodeConnectivity();
    ASSERT_EQ(connectivity.size(), 1);
    EXPECT_FALSE(connectivity[0].reachable);
    EXPECT_EQ(connectivity[0].consecutive_failures, 2);
}

TEST_F(PartitionDetectorTest, PartitionDetectionWithCallback) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    bool partition_detected = false;
    detector.setPartitionCallback([&](const PartitionEvent& event) {
        if (!event.is_healed) {
            partition_detected = true;
        }
    });
    
    // Add nodes
    detector.addNode("node1");
    detector.addNode("node2");
    detector.addNode("node3");
    
    // Start monitoring
    detector.start();
    std::this_thread::sleep_for(100ms);
    
    // Simulate failures on some nodes to create partition
    detector.recordFailure("node2");
    detector.recordFailure("node2");
    detector.recordFailure("node2");
    
    std::this_thread::sleep_for(200ms);
    
    detector.stop();
    
    // Check network health degraded
    auto health = detector.getNetworkHealth();
    EXPECT_NE(health, NetworkHealth::HEALTHY);
}

TEST_F(PartitionDetectorTest, HealthCheckLoop) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    int health_check_count = 0;
    detector.setHealthCheckCallback([&](const std::string& node_id) {
        health_check_count++;
        return true;
    });
    
    detector.addNode("node1");
    detector.addNode("node2");
    
    detector.start();
    std::this_thread::sleep_for(200ms);
    detector.stop();
    
    // Should have performed multiple health checks
    EXPECT_GT(health_check_count, 0);
}

TEST_F(PartitionDetectorTest, NetworkHealthTransitions) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.addNode("node2");
    detector.addNode("node3");
    
    detector.start();
    
    // Initially healthy
    EXPECT_EQ(detector.getNetworkHealth(), NetworkHealth::HEALTHY);
    
    // Make one node unreachable
    detector.recordFailure("node1");
    detector.recordFailure("node1");
    detector.recordFailure("node1");
    
    std::this_thread::sleep_for(100ms);
    
    // Should be degraded
    auto health = detector.getNetworkHealth();
    EXPECT_TRUE(health == NetworkHealth::DEGRADED || health == NetworkHealth::PARTITIONED);
    
    detector.stop();
}

TEST_F(PartitionDetectorTest, PartitionHealing) {
    auto config = createConfig();
    config.enable_auto_healing = true;
    PartitionDetector detector(config);
    
    bool healed = false;
    detector.setPartitionCallback([&](const PartitionEvent& event) {
        if (event.is_healed) {
            healed = true;
        }
    });
    
    detector.addNode("node1");
    detector.addNode("node2");
    
    detector.start();
    
    // Create partition
    detector.recordFailure("node1");
    detector.recordFailure("node1");
    detector.recordFailure("node1");
    
    std::this_thread::sleep_for(100ms);
    
    // Heal partition
    detector.recordHeartbeat("node1", 10ms);
    detector.recordHeartbeat("node2", 10ms);
    
    std::this_thread::sleep_for(200ms);
    
    detector.stop();
}

TEST_F(PartitionDetectorTest, Statistics) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.setHealthCheckCallback([](const std::string&) { return true; });
    
    detector.addNode("node1");
    detector.start();
    
    std::this_thread::sleep_for(200ms);
    
    const auto& stats = detector.getStatistics();
    EXPECT_GT(stats.total_health_checks, 0);
    
    detector.stop();
}

TEST_F(PartitionDetectorTest, PartitionHistory) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.addNode("node2");
    detector.addNode("node3");
    
    auto history = detector.getPartitionHistory();
    EXPECT_EQ(history.size(), 0);  // No partitions initially
}

TEST_F(PartitionDetectorTest, PacketLossTracking) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    
    // Record some heartbeats and failures
    detector.recordHeartbeat("node1", 10ms);
    detector.recordFailure("node1");
    detector.recordHeartbeat("node1", 10ms);
    detector.recordFailure("node1");
    
    auto connectivity = detector.getNodeConnectivity();
    ASSERT_EQ(connectivity.size(), 1);
    
    // Packet loss rate should be tracked
    EXPECT_GT(connectivity[0].packet_loss_rate, 0.0);
}

TEST_F(PartitionDetectorTest, ConsecutiveFailuresReset) {
    auto config = createConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    
    // Record failures
    detector.recordFailure("node1");
    detector.recordFailure("node1");
    
    auto connectivity = detector.getNodeConnectivity();
    EXPECT_EQ(connectivity[0].consecutive_failures, 2);
    
    // Successful heartbeat should reset
    detector.recordHeartbeat("node1", 10ms);
    
    connectivity = detector.getNodeConnectivity();
    EXPECT_EQ(connectivity[0].consecutive_failures, 0);
}
