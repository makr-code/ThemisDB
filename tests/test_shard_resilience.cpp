// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/partition_detector.h"
#include "sharding/quorum_manager.h"
#include "sharding/shard_durability.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <atomic>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

class ShardResilienceTest : public ::testing::Test {
protected:
    PartitionDetectorConfig createPartitionConfig() {
        PartitionDetectorConfig config;
        config.health_check_interval = 50ms;
        config.heartbeat_timeout = 100ms;
        config.max_consecutive_failures = 2;
        config.enable_split_brain_detection = true;
        config.enable_auto_healing = true;
        return config;
    }
    
    QuorumConfig createQuorumConfig() {
        QuorumConfig config;
        config.write_quorum = QuorumType::MAJORITY;
        config.read_quorum = QuorumType::ONE;
        config.operation_timeout = 1000ms;
        config.enable_quorum_enforcement = true;
        return config;
    }
};

TEST_F(ShardResilienceTest, NetworkPartitionDetection) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    // Add nodes
    detector.addNode("node1");
    detector.addNode("node2");
    detector.addNode("node3");
    
    // Initially all nodes should be healthy
    EXPECT_EQ(detector.getNetworkHealth(), NetworkHealth::HEALTHY);
    
    // Simulate failures on node2 to create partition
    detector.recordFailure("node2");
    detector.recordFailure("node2");
    
    auto connectivity = detector.getNodeConnectivity();
    
    bool node2_unreachable = false;
    for (const auto& conn : connectivity) {
        if (conn.node_id == "node2" && !conn.reachable) {
            node2_unreachable = true;
        }
    }
    
    EXPECT_TRUE(node2_unreachable);
}

TEST_F(ShardResilienceTest, SplitBrainPrevention) {
    auto config = createPartitionConfig();
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
    
    detector.start();
    
    // Simulate partition: node1 and node2 can't reach node3
    detector.recordFailure("node3");
    detector.recordFailure("node3");
    detector.recordFailure("node3");
    
    // Wait for detection
    std::this_thread::sleep_for(200ms);

    // With split-brain detection enabled and one of three nodes unreachable,
    // detector should flag split-brain and emit at least one partition event.
    EXPECT_TRUE(partition_detected);
    EXPECT_TRUE(detector.isSplitBrainDetected());
    
    detector.stop();
}

TEST_F(ShardResilienceTest, QuorumDuringPartition) {
    auto qconfig = createQuorumConfig();
    QuorumManager qm(qconfig);
    
    // 5 nodes, need 3 for majority
    std::vector<std::string> nodes = {"node1", "node2", "node3", "node4", "node5"};
    
    // Simulate partition: only 2 nodes respond
    std::atomic<int> available_nodes{2};
    
    auto operation = [&available_nodes]([[maybe_unused]] const std::string& node_id) -> bool {
        // First 2 nodes succeed, rest fail (simulating partition)
        int current = available_nodes.fetch_sub(1, std::memory_order_relaxed);
        return current > 0;
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    // Should fail to achieve quorum (need 3, got 2)
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.acks_received, 2);
    EXPECT_EQ(result.acks_required, 3);
}

TEST_F(ShardResilienceTest, RecoveryFromPartition) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    bool partition_healed = false;
    
    detector.setPartitionCallback([&](const PartitionEvent& event) {
        if (event.is_healed) {
            partition_healed = true;
        }
    });
    
    detector.addNode("node1");
    detector.addNode("node2");
    
    // Simulate partition
    detector.recordFailure("node2");
    detector.recordFailure("node2");
    
    auto connectivity = detector.getNodeConnectivity();
    bool node2_down = false;
    for (const auto& conn : connectivity) {
        if (conn.node_id == "node2" && !conn.reachable) {
            node2_down = true;
        }
    }
    EXPECT_TRUE(node2_down);
    
    // Simulate recovery - node2 comes back
    detector.recordHeartbeat("node2", 10ms);
    
    connectivity = detector.getNodeConnectivity();
    bool node2_up = false;
    for (const auto& conn : connectivity) {
        if (conn.node_id == "node2" && conn.reachable) {
            node2_up = true;
        }
    }
    EXPECT_TRUE(node2_up);
}

TEST_F(ShardResilienceTest, CascadeFailureHandling) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    // Add 5 nodes
    for (int i = 1; i <= 5; ++i) {
        detector.addNode("node" + std::to_string(i));
    }
    
    EXPECT_EQ(detector.getNetworkHealth(), NetworkHealth::HEALTHY);

    detector.start();
    
    // Simulate cascade failure: multiple nodes fail
    detector.recordFailure("node2");
    detector.recordFailure("node2");
    
    detector.recordFailure("node3");
    detector.recordFailure("node3");
    
    detector.recordFailure("node4");
    detector.recordFailure("node4");

    // Give background health loop time to update network state
    std::this_thread::sleep_for(150ms);
    
    // Should detect degraded or partitioned state
    NetworkHealth health = detector.getNetworkHealth();
    EXPECT_TRUE(health == NetworkHealth::DEGRADED || 
                health == NetworkHealth::PARTITIONED);

    detector.stop();
}

TEST_F(ShardResilienceTest, QuorumWithMultipleFailures) {
    auto qconfig = createQuorumConfig();
    QuorumManager qm(qconfig);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3", "node4", "node5"};
    
    // Simulate 2 nodes failing
    std::vector<std::string> failed_nodes = {"node3", "node4"};
    
    auto operation = [&failed_nodes]([[maybe_unused]] const std::string& node_id) -> bool {
        // Check if node is in failed list
        return std::find(failed_nodes.begin(), failed_nodes.end(), node_id) 
               == failed_nodes.end();
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    // Should still achieve quorum (need 3, got 3: node1, node2, node5)
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.acks_received, 3);
    EXPECT_EQ(result.acks_required, 3);
}

TEST_F(ShardResilienceTest, PartitionDetectorStatistics) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.addNode("node2");
    
    // Record some failures
    detector.recordFailure("node1");
    detector.recordFailure("node2");
    
    const auto& stats = detector.getStatistics();
    
    EXPECT_GE(stats.failed_health_checks.load(), 2);
}

TEST_F(ShardResilienceTest, HealthCheckLoopOperation) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    
    // Start health check loop
    detector.start();
    
    // Let it run for a bit
    std::this_thread::sleep_for(200ms);
    
    // Stop health check loop
    detector.stop();
    
    // Should have performed some health checks
    const auto& stats = detector.getStatistics();
    EXPECT_GT(stats.total_health_checks.load(), 0);
}

TEST_F(ShardResilienceTest, PacketLossTracking) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    
    // Alternate between success and failure
    detector.recordHeartbeat("node1", 10ms);
    detector.recordFailure("node1");
    detector.recordHeartbeat("node1", 10ms);
    detector.recordFailure("node1");
    
    auto connectivity = detector.getNodeConnectivity();
    ASSERT_EQ(connectivity.size(), 1);
    
    // Packet loss rate should be between 0 and 1
    EXPECT_GE(connectivity[0].packet_loss_rate, 0.0);
    EXPECT_LE(connectivity[0].packet_loss_rate, 1.0);
}

TEST_F(ShardResilienceTest, QuorumTimeout) {
    auto qconfig = createQuorumConfig();
    qconfig.operation_timeout = 100ms;
    QuorumManager qm(qconfig);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    // Simulate slow/hanging operations
    auto operation = [](const std::string& node_id) -> bool {
        std::this_thread::sleep_for(200ms);  // Exceed timeout
        return true;
    };
    
    auto start = std::chrono::steady_clock::now();
    auto result = qm.executeWrite(operation, nodes);
    auto end = std::chrono::steady_clock::now();
    
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    // Should timeout and not wait for full 600ms (3 * 200ms)
    EXPECT_LT(elapsed.count(), 500);
}

TEST_F(ShardResilienceTest, GracefulDegradation) {
    auto qconfig = createQuorumConfig();
    qconfig.write_quorum = QuorumType::MAJORITY;
    QuorumManager qm(qconfig);
    
    // 5 nodes initially
    std::vector<std::string> nodes = {"node1", "node2", "node3", "node4", "node5"};
    
    // All healthy - should succeed
    auto operation_all_ok = [](const std::string&) { return true; };
    auto result1 = qm.executeWrite(operation_all_ok, nodes);
    EXPECT_TRUE(result1.success);
    
    // Reduce to 3 nodes (still have quorum)
    std::vector<std::string> reduced_nodes = {"node1", "node2", "node3"};
    auto result2 = qm.executeWrite(operation_all_ok, reduced_nodes);
    EXPECT_TRUE(result2.success);
    
    // Reduce to 2 nodes (no quorum for majority)
    std::vector<std::string> too_few_nodes = {"node1", "node2"};
    auto result3 = qm.executeWrite(operation_all_ok, too_few_nodes);
    // 2 nodes means need 2 for majority (2/2 + 1 = 2)
    EXPECT_TRUE(result3.success);
}

TEST_F(ShardResilienceTest, PartitionHistoryTracking) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.addNode("node2");
    
    // Simulate partition
    detector.recordFailure("node2");
    detector.recordFailure("node2");
    
    // Check if partition history is maintained
    auto history = detector.getPartitionHistory();
    
    // Partition history is populated by the running health-check loop.
    // Without start(), failures alone must not produce partition events.
    EXPECT_TRUE(history.empty());
    EXPECT_FALSE(detector.isSplitBrainDetected());
}

TEST_F(ShardResilienceTest, CombinedPartitionAndQuorum) {
    // Test interaction between partition detection and quorum
    auto pconfig = createPartitionConfig();
    auto qconfig = createQuorumConfig();
    
    PartitionDetector detector(pconfig);
    QuorumManager qm(qconfig);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3", "node4", "node5"};
    
    // Add to partition detector
    for (const auto& node : nodes) {
        detector.addNode(node);
    }
    
    // Simulate 2 nodes failing
    detector.recordFailure("node4");
    detector.recordFailure("node4");
    detector.recordFailure("node5");
    detector.recordFailure("node5");
    
    // Get reachable nodes
    auto connectivity = detector.getNodeConnectivity();
    std::vector<std::string> reachable_nodes;
    for (const auto& conn : connectivity) {
        if (conn.reachable) {
            reachable_nodes.push_back(conn.node_id);
        }
    }
    
    // Try quorum operation with only reachable nodes
    auto operation = [](const std::string&) { return true; };
    auto result = qm.executeWrite(operation, reachable_nodes);

    // Two nodes failed, so exactly three nodes remain reachable.
    EXPECT_EQ(reachable_nodes.size(), 3);
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.acks_required, 2);
    EXPECT_EQ(result.acks_received, 3);
}

TEST_F(ShardResilienceTest, NetworkHealthTransitions) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    detector.addNode("node2");
    detector.addNode("node3");
    
    // Start healthy
    EXPECT_EQ(detector.getNetworkHealth(), NetworkHealth::HEALTHY);
    
    // Degrade one node
    detector.recordFailure("node1");
    
    // May transition to degraded (depends on thresholds)
    auto health = detector.getNetworkHealth();
    EXPECT_TRUE(health == NetworkHealth::HEALTHY || 
                health == NetworkHealth::DEGRADED);
}

TEST_F(ShardResilienceTest, QuorumAchievabilityCheck) {
    auto qconfig = createQuorumConfig();
    QuorumManager qm(qconfig);
    
    // Check with different node counts
    EXPECT_TRUE(qm.isQuorumAchievable(3, true));   // 3 nodes, write
    EXPECT_TRUE(qm.isQuorumAchievable(5, true));   // 5 nodes, write
    EXPECT_TRUE(qm.isQuorumAchievable(1, false));  // 1 node, read (ONE quorum)
    
    // Edge case: 0 nodes
    EXPECT_FALSE(qm.isQuorumAchievable(0, true));
}

TEST_F(ShardResilienceTest, RTTTracking) {
    auto config = createPartitionConfig();
    PartitionDetector detector(config);
    
    detector.addNode("node1");
    
    // Record heartbeats with different RTTs
    detector.recordHeartbeat("node1", 5ms);
    detector.recordHeartbeat("node1", 10ms);
    detector.recordHeartbeat("node1", 15ms);
    
    auto connectivity = detector.getNodeConnectivity();
    ASSERT_EQ(connectivity.size(), 1);
    
    // Last RTT should be recorded
    EXPECT_EQ(connectivity[0].last_rtt, 15ms);
}
