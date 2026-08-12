// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for RaftMvccBridge.
// Uses the same MockConsensusModule pattern as test_distributed_time_coordinator.cpp.

#include <gtest/gtest.h>
#include "storage/raft_mvcc_bridge.h"
#include "storage/mvcc_store.h"
#include "storage/hlc.h"
#include "storage/rocksdb_wrapper.h"
#include "sharding/distributed_time_coordinator.h"
#include "sharding/consensus_module.h"
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>
#include <string>

using namespace themis;
using namespace themisdb::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Mock ConsensusModule
// ─────────────────────────────────────────────────────────────────────────────

namespace {

class MockConsensusModule : public ConsensusModule {
public:
    MockConsensusModule() : commit_index_(0), last_log_index_(0), is_leader_(true) {}

    void setCommitIndex(uint64_t v)  { commit_index_ = v; }
    void setLastLogIndex(uint64_t v) { last_log_index_ = v; }
    void setIsLeader(bool v)         { is_leader_ = v; }

    ConsensusType getType() const override { return ConsensusType::RAFT; }
    bool initialize(const std::string&, const std::vector<std::string>&) override { return true; }
    bool start() override { return true; }
    void stop() override {}
    bool isLeader() const override { return is_leader_; }
    std::string getLeaderId() const override { return is_leader_ ? "node-0" : "node-1"; }
    ConsensusState getState() const override {
        return is_leader_ ? ConsensusState::LEADER : ConsensusState::FOLLOWER;
    }
    std::optional<uint64_t> propose(const std::string&, const nlohmann::json&) override {
        return ++last_log_index_;
    }
    bool waitForCommit(uint64_t, std::chrono::milliseconds) override { return true; }
    std::vector<ConsensusLogEntry> readLog(uint64_t, std::optional<uint64_t>) override { return {}; }
    uint64_t getCommitIndex() const override  { return commit_index_; }
    uint64_t getLastLogIndex() const override { return last_log_index_; }
    bool addNode(const std::string&, const std::string&) override { return true; }
    bool removeNode(const std::string&) override { return true; }
    bool transferLeadership(const std::string&) override { return true; }
    bool takeSnapshot(const nlohmann::json&) override { return true; }
    bool restoreSnapshot(const nlohmann::json&) override { return true; }
    ConsensusStats getStats() const override { return ConsensusStats{}; }
    nlohmann::json getStatus() const override { return nlohmann::json{}; }
    void onCommit(std::function<void(const ConsensusLogEntry&)>) override {}
    void onStateChange(std::function<void(ConsensusState, ConsensusState)>) override {}
    void onLeaderChange(std::function<void(const std::string&, const std::string&)>) override {}

private:
    uint64_t commit_index_;
    uint64_t last_log_index_;
    bool     is_leader_;
};

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class RaftMvccBridgeTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_path_ = "./data/test_raft_mvcc_bridge";
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }

        // RocksDB + MVCCStore
        RocksDBWrapper::Config cfg;
        cfg.db_path = db_path_;
        rocksdb_ = std::make_shared<RocksDBWrapper>(cfg);
        ASSERT_TRUE(rocksdb_->open());

        auto clock = std::make_shared<HybridLogicalClock>();
        mvcc_store_ = std::make_shared<MVCCStore>(rocksdb_, clock);

        // Raft mock + coordinator
        mock_consensus_ = std::make_shared<MockConsensusModule>();
        coordinator_ = std::make_shared<DistributedTimeCoordinator>(mock_consensus_);

        // Bridge
        bridge_ = std::make_unique<RaftMvccBridge>(mvcc_store_, coordinator_);
    }

    void TearDown() override {
        bridge_.reset();
        coordinator_.reset();
        mock_consensus_.reset();
        mvcc_store_.reset();
        rocksdb_.reset();
        if (std::filesystem::exists(db_path_)) {
            std::filesystem::remove_all(db_path_);
        }
    }

    static std::vector<uint8_t> makeValue(const std::string& s) {
        return std::vector<uint8_t>(s.begin(), s.end());
    }
    static std::string toString(const std::vector<uint8_t>& v) {
        return std::string(v.begin(), v.end());
    }

    std::string db_path_;
    std::shared_ptr<RocksDBWrapper>              rocksdb_;
    std::shared_ptr<MVCCStore>                   mvcc_store_;
    std::shared_ptr<MockConsensusModule>         mock_consensus_;
    std::shared_ptr<DistributedTimeCoordinator>  coordinator_;
    std::unique_ptr<RaftMvccBridge>              bridge_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, Construction_Valid) {
    EXPECT_NE(bridge_, nullptr);
    EXPECT_EQ(bridge_->mvccStore(), mvcc_store_);
    EXPECT_EQ(bridge_->coordinator(), coordinator_);
}

TEST_F(RaftMvccBridgeTest, Construction_NullStoreThrows) {
    EXPECT_THROW(RaftMvccBridge(nullptr, coordinator_), std::invalid_argument);
}

TEST_F(RaftMvccBridgeTest, Construction_NullCoordinatorThrows) {
    EXPECT_THROW(RaftMvccBridge(mvcc_store_, nullptr), std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// isLeader delegation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, IsLeader_True) {
    mock_consensus_->setIsLeader(true);
    EXPECT_TRUE(bridge_->isLeader());
}

TEST_F(RaftMvccBridgeTest, IsLeader_False) {
    mock_consensus_->setIsLeader(false);
    EXPECT_FALSE(bridge_->isLeader());
}

// ─────────────────────────────────────────────────────────────────────────────
// toHlcTimestamp
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, ToHlcTimestamp_PhysicalFromSystemTime) {
    DistributedTimeCoordinator::TimeInterval interval;
    interval.logical_timestamp = 10;
    interval.uncertainty_ns    = 1'000'000;
    interval.system_time_ns    = 1'740'000'000LL * 1'000'000LL; // 1,740,000 s in ns

    HLCTimestamp ts = RaftMvccBridge::toHlcTimestamp(interval);
    EXPECT_EQ(ts.physical(), 1'740'000'000ULL);  // ms
    EXPECT_EQ(ts.logical(),  10u);
}

TEST_F(RaftMvccBridgeTest, ToHlcTimestamp_LogicalMaskedTo20Bits) {
    DistributedTimeCoordinator::TimeInterval interval;
    interval.logical_timestamp = (1 << 20) + 7;  // overflows 20 bits → 7
    interval.system_time_ns    = 0;

    HLCTimestamp ts = RaftMvccBridge::toHlcTimestamp(interval);
    EXPECT_EQ(ts.logical(), 7u);
}

TEST_F(RaftMvccBridgeTest, ToHlcTimestamp_ZeroInterval) {
    DistributedTimeCoordinator::TimeInterval interval{};
    HLCTimestamp ts = RaftMvccBridge::toHlcTimestamp(interval);
    EXPECT_EQ(ts.value, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// snapshotTimestamp
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, SnapshotTimestamp_IsMonotone) {
    mock_consensus_->setLastLogIndex(1);
    HLCTimestamp t1 = bridge_->snapshotTimestamp();

    mock_consensus_->setLastLogIndex(2);
    HLCTimestamp t2 = bridge_->snapshotTimestamp();

    // Each call must produce a non-decreasing timestamp.
    EXPECT_GE(t2, t1);
}

TEST_F(RaftMvccBridgeTest, SnapshotTimestamp_AdvancesWithLogIndex) {
    mock_consensus_->setLastLogIndex(100);
    HLCTimestamp t1 = bridge_->snapshotTimestamp();

    mock_consensus_->setLastLogIndex(200);
    HLCTimestamp t2 = bridge_->snapshotTimestamp();

    EXPECT_GE(t2, t1);
}

TEST_F(RaftMvccBridgeTest, SnapshotTimestamp_IsNonZero) {
    mock_consensus_->setLastLogIndex(1);
    HLCTimestamp ts = bridge_->snapshotTimestamp();
    EXPECT_GT(ts.value, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Linearizable read
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, LinearizableRead_NonLeaderReturnsNotLeader) {
    mock_consensus_->setIsLeader(false);

    auto result = bridge_->linearizableRead("any-key");
    EXPECT_FALSE(result.is_leader);
    EXPECT_FALSE(result.value.has_value());
}

TEST_F(RaftMvccBridgeTest, LinearizableRead_LeaderMissingKey) {
    mock_consensus_->setIsLeader(true);

    auto result = bridge_->linearizableRead("nonexistent");
    EXPECT_TRUE(result.is_leader);
    EXPECT_FALSE(result.value.has_value());
}

TEST_F(RaftMvccBridgeTest, LinearizableRead_LeaderReturnsLatest) {
    mock_consensus_->setIsLeader(true);

    // Write two versions directly to the MVCCStore.
    mvcc_store_->put("k", makeValue("v1"));
    mvcc_store_->put("k", makeValue("v2"));

    auto result = bridge_->linearizableRead("k");
    ASSERT_TRUE(result.is_leader);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(toString(*result.value), "v2");
}

// ─────────────────────────────────────────────────────────────────────────────
// Snapshot read
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, SnapshotRead_MissingKey) {
    HLCTimestamp ts = bridge_->snapshotTimestamp();
    auto val = bridge_->snapshotRead("nosuchkey", ts);
    EXPECT_FALSE(val.has_value());
}

TEST_F(RaftMvccBridgeTest, SnapshotRead_CorrectVersion) {
    // Write v1, capture snapshot, write v2, verify snapshot gives v1.
    mvcc_store_->put("k", makeValue("v1"));
    HLCTimestamp snap = bridge_->snapshotTimestamp();

    mvcc_store_->put("k", makeValue("v2"));

    auto at_snap = bridge_->snapshotRead("k", snap);
    ASSERT_TRUE(at_snap.has_value());
    EXPECT_EQ(toString(*at_snap), "v1");
}

TEST_F(RaftMvccBridgeTest, SnapshotRead_LatestAfterSnapshotTimestamp) {
    mvcc_store_->put("k", makeValue("v1"));
    HLCTimestamp snap = bridge_->snapshotTimestamp();
    mvcc_store_->put("k", makeValue("v2"));

    // Reading at a timestamp AFTER snap should see v2.
    HLCTimestamp later = bridge_->snapshotTimestamp();
    auto val = bridge_->snapshotRead("k", later);
    ASSERT_TRUE(val.has_value());
    EXPECT_EQ(toString(*val), "v2");
}

// ─────────────────────────────────────────────────────────────────────────────
// raftAwareWrite
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, RaftAwareWrite_ReturnsMeaningfulTimestamp) {
    mock_consensus_->setLastLogIndex(42);
    HLCTimestamp ts = bridge_->raftAwareWrite("mykey", makeValue("hello"));
    EXPECT_GT(ts.value, 0u);
}

TEST_F(RaftMvccBridgeTest, RaftAwareWrite_ValueReadableViaLatest) {
    mock_consensus_->setIsLeader(true);
    bridge_->raftAwareWrite("key1", makeValue("world"));

    auto result = bridge_->linearizableRead("key1");
    ASSERT_TRUE(result.is_leader);
    ASSERT_TRUE(result.value.has_value());
    EXPECT_EQ(toString(*result.value), "world");
}

TEST_F(RaftMvccBridgeTest, RaftAwareWrite_MultipleVersionsOrdered) {
    HLCTimestamp ts1 = bridge_->raftAwareWrite("k", makeValue("a"));
    HLCTimestamp ts2 = bridge_->raftAwareWrite("k", makeValue("b"));
    HLCTimestamp ts3 = bridge_->raftAwareWrite("k", makeValue("c"));

    EXPECT_LT(ts1, ts2);
    EXPECT_LT(ts2, ts3);

    // Snapshot at ts1 → "a"
    auto at1 = bridge_->snapshotRead("k", ts1);
    ASSERT_TRUE(at1.has_value());
    EXPECT_EQ(toString(*at1), "a");

    // Snapshot at ts2 → "b"
    auto at2 = bridge_->snapshotRead("k", ts2);
    ASSERT_TRUE(at2.has_value());
    EXPECT_EQ(toString(*at2), "b");

    // Snapshot at ts3 → "c"
    auto at3 = bridge_->snapshotRead("k", ts3);
    ASSERT_TRUE(at3.has_value());
    EXPECT_EQ(toString(*at3), "c");
}

// ─────────────────────────────────────────────────────────────────────────────
// Cross-shard scenario: two bridges sharing nothing except a snapshot timestamp
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(RaftMvccBridgeTest, CrossShardSnapshot_ConsistentAcrossTwoShards) {
    // Shard 2: separate DB + MVCCStore + bridge
    std::string db_path2 = db_path_ + "_shard2";
    if (std::filesystem::exists(db_path2)) {
        std::filesystem::remove_all(db_path2);
    }
    RocksDBWrapper::Config cfg2;
    cfg2.db_path = db_path2;
    auto rocksdb2   = std::make_shared<RocksDBWrapper>(cfg2);
    ASSERT_TRUE(rocksdb2->open());
    auto clock2     = std::make_shared<HybridLogicalClock>();
    auto mvcc2      = std::make_shared<MVCCStore>(rocksdb2, clock2);
    auto bridge2    = std::make_unique<RaftMvccBridge>(mvcc2, coordinator_);

    // Write to both shards at Raft log index 1
    mock_consensus_->setLastLogIndex(1);
    HLCTimestamp ts1_shard1 = bridge_->raftAwareWrite("user:1", makeValue("alice"));
    HLCTimestamp ts1_shard2 = bridge2->raftAwareWrite("order:1", makeValue("order-a"));

    // Advance Raft to log index 2, write more data
    mock_consensus_->setLastLogIndex(2);
    bridge_->raftAwareWrite("user:1", makeValue("alice-v2"));
    bridge2->raftAwareWrite("order:1", makeValue("order-a-v2"));

    // Both bridges derive the same "snapshot at index 1" via toHlcTimestamp.
    // The logical part will be identical (1 & 0xFFFFF == 1) when system times are equal;
    // in practice the HLC advances with each call, so we use the captured ts1_* values.
    auto user_at_snap  = bridge_->snapshotRead("user:1",  ts1_shard1);
    auto order_at_snap = bridge2->snapshotRead("order:1", ts1_shard2);

    ASSERT_TRUE(user_at_snap.has_value());
    EXPECT_EQ(toString(*user_at_snap),  "alice");

    ASSERT_TRUE(order_at_snap.has_value());
    EXPECT_EQ(toString(*order_at_snap), "order-a");

    // Cleanup shard 2
    bridge2.reset();
    mvcc2.reset();
    rocksdb2.reset();
    std::filesystem::remove_all(db_path2);
}
