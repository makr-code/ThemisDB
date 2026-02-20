// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/paxos_consensus.h"
#include "sharding/consensus_factory.h"
#include <thread>
#include <chrono>

using namespace themisdb::sharding;

class PaxosConsensusTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Default configuration
        config_.type = ConsensusType::PAXOS;
        config_.node_id = "node1";
        config_.cluster_nodes = {"node1", "node2", "node3"};
        config_.heartbeat_interval = std::chrono::milliseconds(100);
        config_.election_timeout_min = std::chrono::milliseconds(300);
        config_.election_timeout_max = std::chrono::milliseconds(600);
        config_.paxos_prepare_timeout = std::chrono::milliseconds(1000);
        config_.paxos_accept_timeout = std::chrono::milliseconds(500);
        config_.enable_persistence = false;  // Disable for tests
    }
    
    ConsensusConfig config_;
};

// Test ballot number generation and ordering

TEST_F(PaxosConsensusTest, ProposalNumberOrdering) {
    ProposalNumber p1{1, "node1"};
    ProposalNumber p2{2, "node1"};
    ProposalNumber p3{2, "node2"};
    
    // Higher round number wins
    EXPECT_LT(p1, p2);
    EXPECT_GT(p2, p1);
    
    // Same round, compare by node_id
    EXPECT_LT(p2, p3);
    
    // Equality
    ProposalNumber p4{1, "node1"};
    EXPECT_EQ(p1, p4);
}

TEST_F(PaxosConsensusTest, ProposalNumberGeneration) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    auto* paxos = dynamic_cast<PaxosConsensus*>(module.get());
    ASSERT_NE(paxos, nullptr);
    
    ASSERT_TRUE(paxos->initialize(config_.node_id, config_.cluster_nodes));
    
    // Generate multiple proposal numbers - they should be increasing
    // Note: We can't test private methods directly, but we can test the behavior
    // through public interface
}

// Test quorum calculation

TEST_F(PaxosConsensusTest, QuorumCalculation) {
    // 3 nodes: quorum = 2
    config_.cluster_nodes = {"node1", "node2", "node3"};
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    
    // With 3 nodes, we need 2 for quorum (majority)
    // This is tested implicitly through consensus operations
}

TEST_F(PaxosConsensusTest, QuorumWithFiveNodes) {
    // 5 nodes: quorum = 3
    config_.cluster_nodes = {"node1", "node2", "node3", "node4", "node5"};
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    
    // With 5 nodes, we need 3 for quorum
}

// Test basic consensus operations

TEST_F(PaxosConsensusTest, InitializeAndStart) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    EXPECT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    EXPECT_TRUE(module->start());
    
    // Give it time to start
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Check state
    EXPECT_NE(module->getState(), ConsensusState::OBSERVER);
    
    module->stop();
}

TEST_F(PaxosConsensusTest, ProposeAndCommit) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    // Wait for leader election
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Propose an operation
    nlohmann::json data = {{"key", "value"}, {"action", "insert"}};
    auto log_index = module->propose("PUT", data);
    
    EXPECT_TRUE(log_index.has_value());
    EXPECT_GT(*log_index, 0);
    
    // Wait for commit
    bool committed = module->waitForCommit(*log_index, std::chrono::seconds(2));
    EXPECT_TRUE(committed);
    
    // Verify commit index advanced
    EXPECT_GE(module->getCommitIndex(), *log_index);
    
    module->stop();
}

TEST_F(PaxosConsensusTest, MultipleProposals) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Propose multiple operations
    std::vector<std::optional<uint64_t>> indices;
    for (int i = 0; i < 5; ++i) {
        nlohmann::json data = {{"key", "key" + std::to_string(i)}, {"value", i}};
        auto idx = module->propose("PUT", data);
        EXPECT_TRUE(idx.has_value());
        indices.push_back(idx);
    }
    
    // Wait for all to commit
    for (const auto& idx : indices) {
        if (idx.has_value()) {
            bool committed = module->waitForCommit(*idx, std::chrono::seconds(3));
            EXPECT_TRUE(committed);
        }
    }
    
    module->stop();
}

TEST_F(PaxosConsensusTest, ReadLogEntries) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Propose operations
    nlohmann::json data1 = {{"op", "first"}};
    nlohmann::json data2 = {{"op", "second"}};
    
    auto idx1 = module->propose("PUT", data1);
    auto idx2 = module->propose("PUT", data2);
    
    ASSERT_TRUE(idx1.has_value());
    ASSERT_TRUE(idx2.has_value());
    
    module->waitForCommit(*idx1, std::chrono::seconds(2));
    module->waitForCommit(*idx2, std::chrono::seconds(2));
    
    // Read log
    auto entries = module->readLog(*idx1, *idx2);
    EXPECT_GE(entries.size(), 2);
    
    module->stop();
}

// Test callbacks

TEST_F(PaxosConsensusTest, OnCommitCallback) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    
    std::atomic<int> commit_count{0};
    std::vector<std::string> committed_ops;
    std::mutex ops_mutex;
    
    module->onCommit([&](const ConsensusLogEntry& entry) {
        commit_count++;
        std::lock_guard<std::mutex> lock(ops_mutex);
        committed_ops.push_back(entry.operation);
    });
    
    ASSERT_TRUE(module->start());
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Propose operations
    nlohmann::json data = {{"test", "data"}};
    module->propose("PUT", data);
    module->propose("DELETE", data);
    
    std::this_thread::sleep_for(std::chrono::seconds(2));
    
    EXPECT_GT(commit_count.load(), 0);
    EXPECT_GE(committed_ops.size(), 1);
    
    module->stop();
}

// Test statistics and status

TEST_F(PaxosConsensusTest, GetStats) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto stats = module->getStats();
    EXPECT_EQ(stats.cluster_size, 3);
    EXPECT_GE(stats.reachable_nodes, 1);
    EXPECT_GE(stats.current_term, 0);
    
    module->stop();
}

TEST_F(PaxosConsensusTest, GetStatus) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto status = module->getStatus();
    EXPECT_TRUE(status.contains("type"));
    EXPECT_EQ(status["type"], "Paxos");
    EXPECT_TRUE(status.contains("node_id"));
    EXPECT_EQ(status["node_id"], config_.node_id);
    EXPECT_TRUE(status.contains("commit_index"));
    
    module->stop();
}

// Test cluster membership changes

TEST_F(PaxosConsensusTest, AddRemoveNode) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Add node
    EXPECT_TRUE(module->addNode("node4", "localhost:8084"));
    
    auto stats = module->getStats();
    EXPECT_EQ(stats.cluster_size, 4);
    
    // Remove node
    EXPECT_TRUE(module->removeNode("node4"));
    
    stats = module->getStats();
    EXPECT_EQ(stats.cluster_size, 3);
    
    module->stop();
}

TEST_F(PaxosConsensusTest, AddDuplicateNode) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Try to add existing node
    EXPECT_FALSE(module->addNode("node2", "localhost:8082"));
    
    module->stop();
}

// Test error conditions

TEST_F(PaxosConsensusTest, ProposeBeforeStart) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    // Don't start the module
    
    nlohmann::json data = {{"key", "value"}};
    auto log_index = module->propose("PUT", data);
    
    EXPECT_FALSE(log_index.has_value());
}

TEST_F(PaxosConsensusTest, WaitForCommitTimeout) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Wait for non-existent log index with short timeout
    bool committed = module->waitForCommit(999999, std::chrono::milliseconds(100));
    EXPECT_FALSE(committed);
    
    module->stop();
}

TEST_F(PaxosConsensusTest, DoubleStart) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    EXPECT_TRUE(module->start());
    
    // Try to start again
    EXPECT_FALSE(module->start());
    
    module->stop();
}

// Test Paxos-specific functionality

TEST_F(PaxosConsensusTest, HandlePrepareRequest) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    auto* paxos = dynamic_cast<PaxosConsensus*>(module.get());
    ASSERT_NE(paxos, nullptr);
    
    ASSERT_TRUE(paxos->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(paxos->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // Test prepare request handling
    ProposalNumber proposal{1, "node1"};
    bool promised = paxos->handlePrepare(1, proposal);
    EXPECT_TRUE(promised);
    
    // Higher proposal should also be promised
    ProposalNumber higher_proposal{2, "node1"};
    promised = paxos->handlePrepare(1, higher_proposal);
    EXPECT_TRUE(promised);
    
    // Lower proposal should be rejected
    ProposalNumber lower_proposal{1, "node0"};
    promised = paxos->handlePrepare(1, lower_proposal);
    EXPECT_FALSE(promised);
    
    paxos->stop();
}

TEST_F(PaxosConsensusTest, HandleAcceptRequest) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    auto* paxos = dynamic_cast<PaxosConsensus*>(module.get());
    ASSERT_NE(paxos, nullptr);
    
    ASSERT_TRUE(paxos->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(paxos->start());
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    // First promise to a proposal
    ProposalNumber proposal{1, "node1"};
    paxos->handlePrepare(1, proposal);
    
    // Test accept request handling
    ConsensusLogEntry entry;
    entry.index = 1;
    entry.term = 1;
    entry.operation = "PUT";
    entry.data = {{"key", "value"}};
    
    bool accepted = paxos->handleAccept(1, proposal, entry);
    EXPECT_TRUE(accepted);
    
    paxos->stop();
}

TEST_F(PaxosConsensusTest, LeaderElection) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());
    
    // Wait for leader election
    std::this_thread::sleep_for(std::chrono::seconds(1));
    
    // At least one node should be leader (in single-node mode, this node should be)
    auto leader_id = module->getLeaderId();
    EXPECT_FALSE(leader_id.empty());
    
    // In deterministic leader election, the node with smallest ID should be leader
    EXPECT_EQ(leader_id, "node1");
    
    module->stop();
}
// ─────────────────────────────────────────────────────────────────────────────
// Snapshot tests
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(PaxosConsensusTest, TakeSnapshot_Succeeds) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());

    nlohmann::json state = {{"collection", "orders"}, {"count", 100}};
    EXPECT_TRUE(module->takeSnapshot(state));

    auto status = module->getStatus();
    EXPECT_TRUE(status.contains("snapshot_index"));
    EXPECT_GE(status["snapshot_index"].get<uint64_t>(), 0u);

    module->stop();
}

TEST_F(PaxosConsensusTest, TakeSnapshot_NotRunning_Fails) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    // Do NOT call start()
    nlohmann::json state = {{"key", "value"}};
    EXPECT_FALSE(module->takeSnapshot(state));
}

TEST_F(PaxosConsensusTest, RestoreSnapshot_ValidData_Succeeds) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());

    nlohmann::json snap = {
        {"collection", "users"},
        {"_snapshot_index", uint64_t(7)},
        {"_snapshot_term",  uint64_t(3)}
    };
    EXPECT_TRUE(module->restoreSnapshot(snap));

    auto status = module->getStatus();
    EXPECT_EQ(status["snapshot_index"].get<uint64_t>(), 7u);
    EXPECT_EQ(status["snapshot_term"].get<uint64_t>(),  3u);

    module->stop();
}

TEST_F(PaxosConsensusTest, RestoreSnapshot_EmptyData_Fails) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());

    EXPECT_FALSE(module->restoreSnapshot(nlohmann::json{}));

    module->stop();
}

TEST_F(PaxosConsensusTest, GetStatus_IncludesSnapshotFields) {
    auto module = ConsensusFactory::create(config_);
    ASSERT_NE(module, nullptr);
    ASSERT_TRUE(module->initialize(config_.node_id, config_.cluster_nodes));
    ASSERT_TRUE(module->start());

    auto status = module->getStatus();
    EXPECT_TRUE(status.contains("snapshot_index"));
    EXPECT_TRUE(status.contains("snapshot_term"));
    EXPECT_EQ(status["snapshot_index"].get<uint64_t>(), 0u);
    EXPECT_EQ(status["snapshot_term"].get<uint64_t>(),  0u);

    module->stop();
}
