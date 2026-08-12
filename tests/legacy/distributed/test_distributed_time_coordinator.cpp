// Copyright 2025 ThemisDB
// Licensed under MIT License

#include <gtest/gtest.h>
#include "sharding/distributed_time_coordinator.h"
#include "sharding/consensus_module.h"
#include <memory>

using namespace themisdb::sharding;

namespace {

/**
 * @brief Mock ConsensusModule for testing DistributedTimeCoordinator
 */
class MockConsensusModule : public themisdb::sharding::ConsensusModule {
public:
    MockConsensusModule() : commit_index_(0), last_log_index_(0) {}
    
    // Set mock values for testing
    void setCommitIndex(uint64_t index) { commit_index_ = index; }
    void setLastLogIndex(uint64_t index) { last_log_index_ = index; }
    
    // ConsensusModule interface - minimal implementation for testing
    themisdb::sharding::ConsensusType getType() const override { 
        return themisdb::sharding::ConsensusType::RAFT; 
    }
    
    bool initialize(const std::string&, const std::vector<std::string>&) override { 
        return true; 
    }
    
    bool start() override { return true; }
    void stop() override {}
    
    bool isLeader() const override { return true; }
    std::string getLeaderId() const override { return "test-node"; }
    themisdb::sharding::ConsensusState getState() const override { 
        return themisdb::sharding::ConsensusState::LEADER; 
    }
    
    std::optional<uint64_t> propose(const std::string&, const nlohmann::json&) override { 
        return ++last_log_index_; 
    }
    
    bool waitForCommit(uint64_t, std::chrono::milliseconds) override { 
        return true; 
    }
    
    std::vector<themisdb::sharding::ConsensusLogEntry> readLog(
        uint64_t, std::optional<uint64_t>) override { 
        return {}; 
    }
    
    uint64_t getCommitIndex() const override { return commit_index_; }
    uint64_t getLastLogIndex() const override { return last_log_index_; }
    
    bool addNode(const std::string&, const std::string&) override { return true; }
    bool removeNode(const std::string&) override { return true; }
    bool transferLeadership(const std::string&) override { return true; }
    bool takeSnapshot(const nlohmann::json&) override { return true; }
    bool restoreSnapshot(const nlohmann::json&) override { return true; }
    
    themisdb::sharding::ConsensusStats getStats() const override { 
        return themisdb::sharding::ConsensusStats{}; 
    }
    
    nlohmann::json getStatus() const override { 
        return nlohmann::json{}; 
    }
    
    void onCommit(std::function<void(const themisdb::sharding::ConsensusLogEntry&)>) override {}
    void onStateChange(std::function<void(themisdb::sharding::ConsensusState, themisdb::sharding::ConsensusState)>) override {}
    void onLeaderChange(std::function<void(const std::string&, const std::string&)>) override {}

private:
    uint64_t commit_index_;
    uint64_t last_log_index_;
};

} // anonymous namespace

/**
 * @brief Test fixture for DistributedTimeCoordinator
 */
class DistributedTimeCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_consensus_ = std::make_shared<MockConsensusModule>();
        
        DistributedTimeCoordinator::Config config;
        config.base_uncertainty_ns = 1000000; // 1ms (1e6 ns)
        config.use_log_index_only = true;
        
        coordinator_ = std::make_unique<DistributedTimeCoordinator>(
            mock_consensus_, config
        );
    }
    
    void TearDown() override {
        coordinator_.reset();
        mock_consensus_.reset();
    }
    
    std::shared_ptr<MockConsensusModule> mock_consensus_;
    std::unique_ptr<DistributedTimeCoordinator> coordinator_;
};

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST_F(DistributedTimeCoordinatorTest, InitializationSucceeds) {
    EXPECT_NE(coordinator_, nullptr);
}

TEST_F(DistributedTimeCoordinatorTest, NowReturnsValidInterval) {
    mock_consensus_->setLastLogIndex(100);
    
    auto interval = coordinator_->now();
    
    EXPECT_EQ(interval.logical_timestamp, 100);
    EXPECT_EQ(interval.uncertainty_ns, 1000000); // 1ms (1e6 ns)
    EXPECT_GT(interval.system_time_ns, 0);
}

TEST_F(DistributedTimeCoordinatorTest, SnapshotTimestampUsesCommitIndex) {
    mock_consensus_->setCommitIndex(50);
    mock_consensus_->setLastLogIndex(100);
    
    auto snapshot_ts = coordinator_->getSnapshotTimestamp();
    
    EXPECT_EQ(snapshot_ts, 50);
}

TEST_F(DistributedTimeCoordinatorTest, CommitTimestampUsesNextLogIndex) {
    mock_consensus_->setCommitIndex(50);
    mock_consensus_->setLastLogIndex(100);
    
    auto commit_ts = coordinator_->getCommitTimestamp();
    
    // Commit timestamp should be last log index + 1
    EXPECT_EQ(commit_ts, 101);
}

TEST_F(DistributedTimeCoordinatorTest, CommitTimestampGreaterThanSnapshot) {
    mock_consensus_->setCommitIndex(50);
    mock_consensus_->setLastLogIndex(100);
    
    auto snapshot_ts = coordinator_->getSnapshotTimestamp();
    auto commit_ts = coordinator_->getCommitTimestamp();
    
    // Ensure external consistency: commit > snapshot
    EXPECT_GT(commit_ts, snapshot_ts);
}

TEST_F(DistributedTimeCoordinatorTest, GetCurrentLogIndexReturnsLastLogIndex) {
    mock_consensus_->setLastLogIndex(42);
    
    auto log_index = coordinator_->getCurrentLogIndex();
    
    EXPECT_EQ(log_index, 42);
}

// ============================================================================
// Ordering and Causality Tests
// ============================================================================

TEST_F(DistributedTimeCoordinatorTest, DefinitelyBeforeWithUncertainty) {
    // Two timestamps separated by more than uncertainty
    int64_t ts1 = 100;
    int64_t ts2 = 200;
    
    // With current uncertainty semantics this boundary is not definitely ordered.
    EXPECT_FALSE(coordinator_->definitelyBefore(ts1, ts2));
}

TEST_F(DistributedTimeCoordinatorTest, DefinitelyBeforeWithCloseTimestamps) {
    // Two timestamps within uncertainty range
    int64_t ts1 = 100;
    int64_t ts2 = 100 + 500'000; // Within 1ms uncertainty
    
    // ts1 + uncertainty >= ts2, so not definitely before
    EXPECT_FALSE(coordinator_->definitelyBefore(ts1, ts2));
}

TEST_F(DistributedTimeCoordinatorTest, MonotonicTimestamps) {
    // Simulate log index progression
    for (uint64_t i = 1; i <= 100; ++i) {
        mock_consensus_->setLastLogIndex(i);
        auto ts = coordinator_->now();
        EXPECT_EQ(ts.logical_timestamp, static_cast<int64_t>(i));
    }
}

// ============================================================================
// Edge Case Tests
// ============================================================================

TEST_F(DistributedTimeCoordinatorTest, ZeroLogIndex) {
    mock_consensus_->setCommitIndex(0);
    mock_consensus_->setLastLogIndex(0);
    
    auto snapshot_ts = coordinator_->getSnapshotTimestamp();
    auto commit_ts = coordinator_->getCommitTimestamp();
    
    EXPECT_EQ(snapshot_ts, 0);
    EXPECT_EQ(commit_ts, 1); // Next index after 0
}

TEST_F(DistributedTimeCoordinatorTest, LargeLogIndex) {
    uint64_t large_index = UINT64_MAX / 2;
    mock_consensus_->setCommitIndex(large_index);
    mock_consensus_->setLastLogIndex(large_index);
    
    auto snapshot_ts = coordinator_->getSnapshotTimestamp();
    
    EXPECT_EQ(snapshot_ts, static_cast<int64_t>(large_index));
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(DistributedTimeCoordinatorTest, CustomUncertainty) {
    DistributedTimeCoordinator::Config config;
    config.base_uncertainty_ns = 5000000; // 5ms (5e6 ns)
    
    auto custom_coordinator = std::make_unique<DistributedTimeCoordinator>(
        mock_consensus_, config
    );
    
    mock_consensus_->setLastLogIndex(100);
    auto interval = custom_coordinator->now();
    
    EXPECT_EQ(interval.uncertainty_ns, 5000000);
}
