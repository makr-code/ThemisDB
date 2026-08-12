#include "sharding/quorum_manager.h"
#include <gtest/gtest.h>
#include <thread>
#include <chrono>

using namespace themisdb::sharding;
using namespace std::chrono_literals;

class QuorumManagerTest : public ::testing::Test {
protected:
    QuorumConfig createConfig(QuorumType write_quorum = QuorumType::MAJORITY,
                             QuorumType read_quorum = QuorumType::ONE) {
        QuorumConfig config;
        config.write_quorum = write_quorum;
        config.read_quorum = read_quorum;
        config.operation_timeout = 1000ms;
        config.enable_quorum_enforcement = true;
        return config;
    }
};

TEST_F(QuorumManagerTest, WriteQuorumSizeCalculation) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    EXPECT_EQ(qm.getWriteQuorumSize(3), 2);  // 3/2 + 1 = 2
    EXPECT_EQ(qm.getWriteQuorumSize(5), 3);  // 5/2 + 1 = 3
    EXPECT_EQ(qm.getWriteQuorumSize(7), 4);  // 7/2 + 1 = 4
}

TEST_F(QuorumManagerTest, WriteQuorumAll) {
    auto config = createConfig(QuorumType::ALL);
    QuorumManager qm(config);
    
    EXPECT_EQ(qm.getWriteQuorumSize(3), 3);
    EXPECT_EQ(qm.getWriteQuorumSize(5), 5);
}

TEST_F(QuorumManagerTest, WriteQuorumOne) {
    auto config = createConfig(QuorumType::ONE);
    QuorumManager qm(config);
    
    EXPECT_EQ(qm.getWriteQuorumSize(3), 1);
    EXPECT_EQ(qm.getWriteQuorumSize(5), 1);
}

TEST_F(QuorumManagerTest, CustomQuorumSize) {
    auto config = createConfig(QuorumType::CUSTOM);
    config.custom_write_quorum = 4;
    QuorumManager qm(config);
    
    EXPECT_EQ(qm.getWriteQuorumSize(5), 4);
    EXPECT_EQ(qm.getWriteQuorumSize(3), 3);  // Limited by total nodes
}

TEST_F(QuorumManagerTest, SuccessfulWriteWithQuorum) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    auto operation = []([[maybe_unused]] const std::string& node_id) -> bool {
        return true;  // All succeed
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.acks_received, 3);
    EXPECT_EQ(result.acks_required, 2);
    EXPECT_EQ(result.successful_nodes.size(), 3);
}

TEST_F(QuorumManagerTest, FailedWriteNoQuorum) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    int success_count = 0;
    auto operation = [&success_count]([[maybe_unused]] const std::string& node_id) -> bool {
        return success_count++ < 1;  // Only first one succeeds
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    EXPECT_FALSE(result.success);
    EXPECT_LT(result.acks_received, result.acks_required);
}

TEST_F(QuorumManagerTest, WriteWithPartialFailure) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    auto operation = [](const std::string& node_id) -> bool {
        return node_id != "node3";  // node3 fails
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    EXPECT_TRUE(result.success);  // 2 out of 3 is quorum
    EXPECT_EQ(result.acks_received, 2);
    EXPECT_EQ(result.successful_nodes.size(), 2);
    EXPECT_EQ(result.failed_nodes.size(), 1);
}

TEST_F(QuorumManagerTest, ReadQuorumExecution) {
    auto config = createConfig(QuorumType::MAJORITY, QuorumType::ONE);
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    auto operation = []([[maybe_unused]] const std::string& node_id) -> std::optional<std::string> {
        return "data";
    };
    
    auto result = qm.executeRead(operation, nodes);
    
    EXPECT_TRUE(result.success);
    EXPECT_GE(result.acks_received, 1);
}

TEST_F(QuorumManagerTest, ReadQuorumAllFail) {
    auto config = createConfig(QuorumType::MAJORITY, QuorumType::ONE);
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    auto operation = []([[maybe_unused]] const std::string& node_id) -> std::optional<std::string> {
        return std::nullopt;  // All fail
    };
    
    auto result = qm.executeRead(operation, nodes);
    
    EXPECT_FALSE(result.success);
}

TEST_F(QuorumManagerTest, QuorumAchievability) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    EXPECT_TRUE(qm.isQuorumAchievable(3, true));   // 2/3 achievable
    EXPECT_TRUE(qm.isQuorumAchievable(5, true));   // 3/5 achievable
    EXPECT_FALSE(qm.isQuorumAchievable(1, true));  // 1 node can't have majority
}

TEST_F(QuorumManagerTest, DisabledQuorumEnforcement) {
    auto config = createConfig(QuorumType::MAJORITY);
    config.enable_quorum_enforcement = false;
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    int call_count = 0;
    auto operation = [&call_count]([[maybe_unused]] const std::string& node_id) -> bool {
        call_count++;
        return false;  // All fail
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    // Should still return success when enforcement disabled
    EXPECT_TRUE(result.success);
    EXPECT_EQ(call_count, 3);  // All nodes called
}

TEST_F(QuorumManagerTest, Statistics) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    qm.resetStatistics();
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    auto operation = [](const std::string&) { return true; };
    
    qm.executeWrite(operation, nodes);
    qm.executeWrite(operation, nodes);
    
    const auto& stats = qm.getStatistics();
    EXPECT_EQ(stats.total_writes, 2);
    EXPECT_EQ(stats.successful_writes, 2);
    EXPECT_EQ(stats.failed_writes, 0);
}

TEST_F(QuorumManagerTest, ConfigUpdate) {
    auto config = createConfig(QuorumType::MAJORITY);
    QuorumManager qm(config);
    
    EXPECT_EQ(qm.getWriteQuorumSize(3), 2);
    
    config.write_quorum = QuorumType::ALL;
    qm.updateConfig(config);
    
    EXPECT_EQ(qm.getWriteQuorumSize(3), 3);
}

TEST_F(QuorumManagerTest, OperationTimeout) {
    auto config = createConfig(QuorumType::MAJORITY);
    config.operation_timeout = 100ms;
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    auto operation = []([[maybe_unused]] const std::string& node_id) -> bool {
        std::this_thread::sleep_for(200ms);  // Longer than timeout
        return true;
    };
    
    auto start = std::chrono::steady_clock::now();
    auto result = qm.executeWrite(operation, nodes);
    auto duration = std::chrono::steady_clock::now() - start;
    
    // Should timeout
    EXPECT_LT(duration, 500ms);  // Much less than total sleep time
}

TEST_F(QuorumManagerTest, FailFastMode) {
    auto config = createConfig(QuorumType::MAJORITY);
    config.fail_fast = true;
    QuorumManager qm(config);
    
    std::vector<std::string> nodes = {"node1", "node2", "node3"};
    
    int call_count = 0;
    auto operation = [&call_count]([[maybe_unused]] const std::string& node_id) -> bool {
        call_count++;
        return true;
    };
    
    auto result = qm.executeWrite(operation, nodes);
    
    EXPECT_TRUE(result.success);
    // With fail-fast, might not call all nodes once quorum reached
    EXPECT_GE(result.acks_received, 2);  // At least quorum
}
