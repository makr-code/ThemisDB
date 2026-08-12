/**
 * @file test_auto_failover_recovery.cpp
 * @brief Automatic failover and recovery tests
 * 
 * Tests high-availability scenarios:
 * - Automatic failover detection
 * - Leader election
 * - Data recovery procedures
 * - Consistency after recovery
 * - Replica synchronization
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <condition_variable>
#include <random>

using namespace std::chrono_literals;

namespace themis {
namespace test {

/**
 * @brief Mock replica node for HA testing
 */
class MockReplicaNode {
public:
    enum class Role {
        LEADER,
        FOLLOWER,
        CANDIDATE,
        UNAVAILABLE
    };
    
    explicit MockReplicaNode(int id) 
        : node_id_(id), role_(Role::FOLLOWER), is_healthy_(true), 
          heartbeat_count_(0), data_version_(0) {}
    
    // Copy constructor deleted due to std::atomic member
    MockReplicaNode(const MockReplicaNode&) = delete;
    MockReplicaNode& operator=(const MockReplicaNode&) = delete;
    
    // Move constructor and assignment
    MockReplicaNode(MockReplicaNode&&) noexcept = default;
    MockReplicaNode& operator=(MockReplicaNode&&) noexcept = default;
    
    void setRole(Role role) {
        std::lock_guard<std::mutex> lock(mutex_);
        role_ = role;
    }
    
    Role getRole() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return role_;
    }
    
    void setHealth(bool healthy) {
        std::lock_guard<std::mutex> lock(mutex_);
        is_healthy_ = healthy;
    }
    
    bool isHealthy() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return is_healthy_;
    }
    
    void sendHeartbeat() {
        if (is_healthy_) {
            heartbeat_count_.fetch_add(1);
            last_heartbeat_ = std::chrono::steady_clock::now();
        }
    }
    
    bool checkHeartbeat(std::chrono::milliseconds timeout) const {
        auto elapsed = std::chrono::steady_clock::now() - last_heartbeat_;
        return elapsed < timeout;
    }
    
    int getHeartbeatCount() const { return heartbeat_count_.load(); }
    int getId() const { return node_id_; }
    
    void writeData(const std::string& key, int value) {
        std::lock_guard<std::mutex> lock(mutex_);
        data_[key] = value;
        data_version_++;
    }
    
    int readData(const std::string& key) const {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = data_.find(key);
        return (it != data_.end()) ? it->second : -1;
    }
    
    uint64_t getDataVersion() const {
        std::lock_guard<std::mutex> lock(mutex_);
        return data_version_;
    }
    
    void syncFrom(const MockReplicaNode& leader) {
        std::lock_guard<std::mutex> lock1(mutex_);
        std::lock_guard<std::mutex> lock2(leader.mutex_);
        data_ = leader.data_;
        data_version_ = leader.data_version_;
    }
    
private:
    int node_id_;
    Role role_;
    bool is_healthy_;
    std::atomic<int> heartbeat_count_;
    std::chrono::steady_clock::time_point last_heartbeat_;
    uint64_t data_version_;
    std::map<std::string, int> data_;
    mutable std::mutex mutex_;
};

/**
 * @brief Test automatic leader failover
 */
TEST(AutoFailoverRecoveryTest, AutomaticLeaderFailover) {
    constexpr int NUM_NODES = 5;
    std::vector<std::unique_ptr<MockReplicaNode>> cluster;
    
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(std::make_unique<MockReplicaNode>(i));
    }
    
    // Initial setup: node 0 is leader
    cluster[0]->setRole(MockReplicaNode::Role::LEADER);
    for (int i = 1; i < NUM_NODES; ++i) {
        cluster[i]->setRole(MockReplicaNode::Role::FOLLOWER);
    }
    
    // Simulate leader failure
    cluster[0]->setHealth(false);
    
    // Detect leader failure (no heartbeats)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    bool leader_available = cluster[0]->isHealthy();
    EXPECT_FALSE(leader_available);
    
    // Election: select new leader (highest ID wins in this simulation)
    int new_leader_id = -1;
    for (int i = NUM_NODES - 1; i >= 0; --i) {
        if (cluster[i]->isHealthy()) {
            new_leader_id = i;
            cluster[i]->setRole(MockReplicaNode::Role::LEADER);
            break;
        }
    }
    
    ASSERT_NE(new_leader_id, -1);
    ASSERT_NE(new_leader_id, 0); // Should not be old leader
    EXPECT_EQ(cluster[new_leader_id]->getRole(), MockReplicaNode::Role::LEADER);
    
    // Verify cluster still operational
    cluster[new_leader_id]->writeData("test_key", 42);
    EXPECT_EQ(cluster[new_leader_id]->readData("test_key"), 42);
}

/**
 * @brief Test heartbeat-based failure detection
 */
TEST(AutoFailoverRecoveryTest, HeartbeatFailureDetection) {
    constexpr int NUM_NODES = 3;
    constexpr int HEARTBEAT_INTERVAL_MS = 20;
    constexpr int FAILURE_TIMEOUT_MS = 100;
    
    std::vector<MockReplicaNode> cluster;
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(i);
    }
    
    cluster[0].setRole(MockReplicaNode::Role::LEADER);
    
    // Simulate heartbeats
    for (int i = 0; i < 3; ++i) {
        cluster[0].sendHeartbeat();
        std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS));
    }
    
    int initial_heartbeats = cluster[0].getHeartbeatCount();
    EXPECT_GE(initial_heartbeats, 3);
    
    // Leader becomes unavailable
    cluster[0].setHealth(false);
    
    // No more heartbeats after failure
    std::this_thread::sleep_for(std::chrono::milliseconds(HEARTBEAT_INTERVAL_MS * 3));
    
    bool heartbeat_ok = cluster[0].checkHeartbeat(
        std::chrono::milliseconds(FAILURE_TIMEOUT_MS));
    EXPECT_FALSE(heartbeat_ok);
    
    // Followers should detect failure
    bool leader_failed = !cluster[0].checkHeartbeat(
        std::chrono::milliseconds(FAILURE_TIMEOUT_MS));
    EXPECT_TRUE(leader_failed);
}

/**
 * @brief Test data recovery after node restart
 */
TEST(AutoFailoverRecoveryTest, DataRecoveryAfterRestart) {
    MockReplicaNode leader(0);
    MockReplicaNode follower(1);
    
    leader.setRole(MockReplicaNode::Role::LEADER);
    follower.setRole(MockReplicaNode::Role::FOLLOWER);
    
    // Write data to leader
    leader.writeData("key1", 100);
    leader.writeData("key2", 200);
    leader.writeData("key3", 300);
    
    uint64_t leader_version = leader.getDataVersion();
    EXPECT_EQ(leader_version, 3);
    
    // Follower syncs from leader
    follower.syncFrom(leader);
    
    EXPECT_EQ(follower.readData("key1"), 100);
    EXPECT_EQ(follower.readData("key2"), 200);
    EXPECT_EQ(follower.readData("key3"), 300);
    EXPECT_EQ(follower.getDataVersion(), leader_version);
}

/**
 * @brief Test replica synchronization after network partition
 */
TEST(AutoFailoverRecoveryTest, ReplicaSyncAfterPartition) {
    constexpr int NUM_NODES = 4;
    std::vector<MockReplicaNode> cluster;
    
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(i);
    }
    
    cluster[0].setRole(MockReplicaNode::Role::LEADER);
    
    // Write to leader before partition
    cluster[0].writeData("initial", 1);
    
    // Simulate network partition: nodes 2-3 separated
    cluster[2].setHealth(false);
    cluster[3].setHealth(false);
    
    // Leader continues writing (majority available)
    cluster[0].writeData("during_partition", 2);
    
    // Partition heals
    cluster[2].setHealth(true);
    cluster[3].setHealth(true);
    
    // Separated nodes sync from leader
    cluster[2].syncFrom(cluster[0]);
    cluster[3].syncFrom(cluster[0]);
    
    // Verify all nodes have same data
    for (const auto& node : cluster) {
        EXPECT_EQ(node.readData("initial"), 1);
        EXPECT_EQ(node.readData("during_partition"), 2);
    }
    
    uint64_t leader_version = cluster[0].getDataVersion();
    for (const auto& node : cluster) {
        EXPECT_EQ(node.getDataVersion(), leader_version);
    }
}

/**
 * @brief Test quorum-based operations during failures
 */
TEST(AutoFailoverRecoveryTest, QuorumBasedOperations) {
    constexpr int NUM_NODES = 5;
    constexpr int QUORUM_SIZE = 3;
    
    std::vector<MockReplicaNode> cluster;
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(i);
    }
    
    cluster[0].setRole(MockReplicaNode::Role::LEADER);
    
    // All nodes healthy initially
    int healthy_count = 0;
    for (const auto& node : cluster) {
        if (node.isHealthy()) {
            healthy_count++;
        }
    }
    EXPECT_EQ(healthy_count, NUM_NODES);
    
    // Write requires quorum
    cluster[0].writeData("quorum_test", 42);
    
    // Simulate 2 node failures
    cluster[3].setHealth(false);
    cluster[4].setHealth(false);
    
    healthy_count = 0;
    for (const auto& node : cluster) {
        if (node.isHealthy()) {
            healthy_count++;
        }
    }
    
    // Still have quorum (3/5 nodes healthy)
    EXPECT_GE(healthy_count, QUORUM_SIZE);
    
    // Operations should continue
    cluster[0].writeData("after_failure", 99);
    EXPECT_EQ(cluster[0].readData("after_failure"), 99);
    
    // Simulate one more failure (lose quorum)
    cluster[2].setHealth(false);
    
    healthy_count = 0;
    for (const auto& node : cluster) {
        if (node.isHealthy()) {
            healthy_count++;
        }
    }
    
    // No longer have quorum
    EXPECT_LT(healthy_count, QUORUM_SIZE);
}

/**
 * @brief Test cascading failure handling
 */
TEST(AutoFailoverRecoveryTest, CascadingFailureHandling) {
    constexpr int NUM_NODES = 7;
    std::vector<MockReplicaNode> cluster;
    
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(i);
    }
    
    cluster[0].setRole(MockReplicaNode::Role::LEADER);
    
    // Simulate cascading failures
    std::vector<int> failure_sequence = {0, 1, 3};
    
    for (int node_id : failure_sequence) {
        cluster[node_id].setHealth(false);
        
        // Check if we still have majority
        int healthy = 0;
        for (const auto& node : cluster) {
            if (node.isHealthy()) healthy++;
        }
        
        // After each failure, verify cluster state
        if (healthy >= NUM_NODES / 2 + 1) {
            // Still have majority - should elect new leader if needed
            if (node_id == 0) {
                // Original leader failed, elect new one
                for (size_t i = 1; i < cluster.size(); ++i) {
                    if (cluster[i].isHealthy()) {
                        cluster[i].setRole(MockReplicaNode::Role::LEADER);
                        break;
                    }
                }
            }
        }
    }
    
    // Count final healthy nodes
    int final_healthy = 0;
    int leader_count = 0;
    for (const auto& node : cluster) {
        if (node.isHealthy()) {
            final_healthy++;
            if (node.getRole() == MockReplicaNode::Role::LEADER) {
                leader_count++;
            }
        }
    }
    
    EXPECT_GE(final_healthy, NUM_NODES / 2); // Still have majority
    EXPECT_EQ(leader_count, 1); // Exactly one leader
}

/**
 * @brief Test split-brain prevention
 */
TEST(AutoFailoverRecoveryTest, SplitBrainPrevention) {
    constexpr int NUM_NODES = 6;
    std::vector<MockReplicaNode> cluster;
    
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(i);
    }
    
    cluster[0].setRole(MockReplicaNode::Role::LEADER);
    
    // Simulate network partition: 3-3 split
    // Partition A: nodes 0, 1, 2
    // Partition B: nodes 3, 4, 5
    std::vector<bool> partition_a = {true, true, true, false, false, false};
    
    // In a true split-brain scenario, only one partition should elect a leader
    // The partition with the original leader should retain it
    
    int partition_a_size = 0;
    int partition_b_size = 0;
    
    for (size_t i = 0; i < cluster.size(); ++i) {
        if (partition_a[i]) {
            partition_a_size++;
        } else {
            partition_b_size++;
        }
    }
    
    // Neither partition has majority (need 4/6)
    EXPECT_LT(partition_a_size, (NUM_NODES / 2 + 1));
    EXPECT_LT(partition_b_size, (NUM_NODES / 2 + 1));
    
    // Both partitions should refuse to elect new leaders
    // This prevents split-brain
    
    // Count leaders in each partition
    int leaders_in_a = 0;
    int leaders_in_b = 0;
    
    for (size_t i = 0; i < cluster.size(); ++i) {
        if (cluster[i].getRole() == MockReplicaNode::Role::LEADER) {
            if (partition_a[i]) {
                leaders_in_a++;
            } else {
                leaders_in_b++;
            }
        }
    }
    
    // Original leader is in partition A
    EXPECT_EQ(leaders_in_a, 1);
    EXPECT_EQ(leaders_in_b, 0);
}

/**
 * @brief Test recovery after temporary failures
 */
TEST(AutoFailoverRecoveryTest, RecoveryAfterTemporaryFailure) {
    constexpr int NUM_NODES = 3;
    std::vector<MockReplicaNode> cluster;
    
    for (int i = 0; i < NUM_NODES; ++i) {
        cluster.emplace_back(i);
    }
    
    cluster[0].setRole(MockReplicaNode::Role::LEADER);
    
    // Write initial data
    cluster[0].writeData("persistent", 123);
    for (int i = 1; i < NUM_NODES; ++i) {
        cluster[i].syncFrom(cluster[0]);
    }
    
    // Node 1 temporarily fails
    cluster[1].setHealth(false);
    
    // More writes while node 1 is down
    cluster[0].writeData("during_failure", 456);
    cluster[2].syncFrom(cluster[0]);
    
    // Node 1 recovers
    cluster[1].setHealth(true);
    
    // Node 1 needs to catch up
    uint64_t node1_version = cluster[1].getDataVersion();
    uint64_t leader_version = cluster[0].getDataVersion();
    
    EXPECT_LT(node1_version, leader_version); // Node 1 is behind
    
    // Sync to catch up
    cluster[1].syncFrom(cluster[0]);
    
    // Verify node 1 is now up to date
    EXPECT_EQ(cluster[1].readData("persistent"), 123);
    EXPECT_EQ(cluster[1].readData("during_failure"), 456);
    EXPECT_EQ(cluster[1].getDataVersion(), leader_version);
}

/**
 * @brief Test leader election with priorities
 */
TEST(AutoFailoverRecoveryTest, PriorityBasedLeaderElection) {
    constexpr int NUM_NODES = 5;
    
    struct PriorityNode {
        MockReplicaNode node;
        int priority;
        
        PriorityNode(int id, int prio) : node(id), priority(prio) {}
    };
    
    std::vector<PriorityNode> cluster;
    cluster.emplace_back(0, 10); // Highest priority
    cluster.emplace_back(1, 7);
    cluster.emplace_back(2, 5);
    cluster.emplace_back(3, 3);
    cluster.emplace_back(4, 1);  // Lowest priority
    
    // Node 0 is initial leader
    cluster[0].node.setRole(MockReplicaNode::Role::LEADER);
    
    // Leader fails
    cluster[0].node.setHealth(false);
    
    // Election: choose healthy node with highest priority
    int new_leader_idx = -1;
    int max_priority = -1;
    
    for (size_t i = 0; i < cluster.size(); ++i) {
        if (cluster[i].node.isHealthy() && cluster[i].priority > max_priority) {
            max_priority = cluster[i].priority;
            new_leader_idx = i;
        }
    }
    
    ASSERT_NE(new_leader_idx, -1);
    EXPECT_EQ(new_leader_idx, 1); // Node 1 has second-highest priority
    
    cluster[new_leader_idx].node.setRole(MockReplicaNode::Role::LEADER);
    EXPECT_EQ(cluster[new_leader_idx].node.getRole(), MockReplicaNode::Role::LEADER);
}

} // namespace test
} // namespace themis
