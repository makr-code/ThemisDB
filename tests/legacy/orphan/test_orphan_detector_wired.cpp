/**
 * @file test_orphan_detector_wired.cpp
 * @brief Tests for OrphanDetector wired to DistributedCoordinator (v1.8.0, Issue #202).
 *
 * Verifies that:
 *   - OrphanDetector can be constructed with a DistributedCoordinator* (new overload).
 *   - detectOrphans() uses DistributedCoordinator::listInFlightTransactions() when wired.
 *   - isOrphaned() uses DistributedCoordinator::getTransaction() when wired.
 *   - Fallback to the per-call CrossShardTransactionCoordinator works correctly.
 *   - Null / unwired cases remain safe (no crash, correct return values).
 */

#include <gtest/gtest.h>
#include "sharding/orphan_detector.h"
#include "sharding/distributed_coordinator.h"
#include "sharding/cross_shard_transaction.h"
#include "sharding/consensus_module.h"
#include "sharding/shard_topology.h"
#include "sharding/gossip_config_manager.h"
#include <chrono>
#include <filesystem>
#include <memory>
#include <string>
#ifdef _WIN32
#  include <process.h>
#  define getpid _getpid
#else
#  include <unistd.h>
#endif

using namespace themisdb::sharding;
using namespace themis::sharding;

// ============================================================================
// Minimal mock for ConsensusModule (mirrors existing tests)
// ============================================================================

class MockConsensusForWired : public ConsensusModule {
public:
    ConsensusType getType() const override { return ConsensusType::RAFT; }
    bool initialize(const std::string&, const std::vector<std::string>&) override { return true; }
    bool start() override { return true; }
    void stop() override {}
    bool isLeader() const override { return true; }
    std::string getLeaderId() const override { return "leader-1"; }
    ConsensusState getState() const override { return ConsensusState::LEADER; }

    std::optional<uint64_t> propose(const std::string&, const nlohmann::json&) override {
        return ++last_index_;
    }

    bool waitForCommit(uint64_t, std::chrono::milliseconds) override { return true; }
    std::vector<ConsensusLogEntry> readLog(uint64_t, std::optional<uint64_t>) override { return {}; }
    uint64_t getCommitIndex() const override { return last_index_; }
    uint64_t getLastLogIndex() const override { return last_index_; }
    bool addNode(const std::string&, const std::string&) override { return true; }
    bool removeNode(const std::string&) override { return true; }
    bool transferLeadership(const std::string&) override { return true; }
    bool takeSnapshot(const nlohmann::json&) override { return true; }
    bool restoreSnapshot(const nlohmann::json&) override { return true; }
    ConsensusStats getStats() const override {
        return ConsensusStats{0, 0, 0, ConsensusState::LEADER, "leader-1", 1, 1,
                              std::chrono::milliseconds(0), 0, 0};
    }
    nlohmann::json getStatus() const override { return nlohmann::json::object(); }
    void onCommit(std::function<void(const ConsensusLogEntry&)>) override {}
    void onStateChange(std::function<void(ConsensusState, ConsensusState)>) override {}
    void onLeaderChange(std::function<void(const std::string&, const std::string&)>) override {}

private:
    uint64_t last_index_{0};
};

// ============================================================================
// Helpers – build a CrossShardTransactionCoordinator
// ============================================================================

static std::shared_ptr<CrossShardTransactionCoordinator> makeTxnCoordinator()
{
    auto consensus = std::make_shared<MockConsensusForWired>();
    CrossShardTransactionConfig cfg;
    cfg.transaction_log_path =
        (std::filesystem::temp_directory_path() /
         ("themisdb_orphan_wired_" + std::to_string(::getpid()) + ".jsonl")).string();
    auto coord = std::make_shared<CrossShardTransactionCoordinator>(cfg, consensus);
    coord->initialize();
    return coord;
}

// ============================================================================
// Helpers – build a DistributedCoordinator
// ============================================================================

static std::shared_ptr<DistributedCoordinator> makeDistCoordinator()
{
    auto topology  = std::make_shared<ShardTopology>();
    ShardInfo si;
    si.shard_id        = "shard-test";
    si.primary_endpoint = "localhost:50099";
    si.is_healthy      = true;
    topology->addShard(si);

    GossipConfigManagerConfig gcfg;
    gcfg.local_shard_id = "shard-test";
    gcfg.local_endpoint = "localhost:50099";
    gcfg.enabled        = false;
    auto gossip = std::make_shared<GossipConfigManager>(gcfg, topology);

    return std::make_shared<DistributedCoordinator>("shard-test", topology, gossip);
}

// ============================================================================
// Test fixture
// ============================================================================

class OrphanDetectorWiredTest : public ::testing::Test {
protected:
    void SetUp() override {
        txn_coord_  = makeTxnCoordinator();
        dist_coord_ = makeDistCoordinator();
        dist_coord_->setTransactionCoordinator(txn_coord_.get());
    }

    void TearDown() override {
        dist_coord_->setTransactionCoordinator(nullptr);
        txn_coord_->stop();
    }

    std::shared_ptr<CrossShardTransactionCoordinator> txn_coord_;
    std::shared_ptr<DistributedCoordinator>            dist_coord_;
};

// ============================================================================
// Construction tests
// ============================================================================

TEST(OrphanDetectorWiredConstructionTest, DefaultConstructorStillWorks) {
    sharding::OrphanDetector::Config cfg;
    EXPECT_NO_THROW(sharding::OrphanDetector det(cfg));
}

TEST(OrphanDetectorWiredConstructionTest, ConstructWithNullDistCoord) {
    sharding::OrphanDetector::Config cfg;
    EXPECT_NO_THROW(sharding::OrphanDetector det(cfg, nullptr));
}

TEST_F(OrphanDetectorWiredTest, ConstructWithDistCoordSucceeds) {
    sharding::OrphanDetector::Config cfg;
    EXPECT_NO_THROW(sharding::OrphanDetector det(cfg, dist_coord_.get()));
}

// ============================================================================
// listInFlightTransactions / getTransaction delegation
// ============================================================================

TEST_F(OrphanDetectorWiredTest, ListInFlightTransactions_EmptyInitially) {
    auto txns = dist_coord_->listInFlightTransactions();
    EXPECT_TRUE(txns.empty());
}

TEST_F(OrphanDetectorWiredTest, ListInFlightTransactions_AfterBeginTransaction) {
    const std::string txn_id = "wired-test-txn-list-1";
    ASSERT_TRUE(txn_coord_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    auto txns = dist_coord_->listInFlightTransactions();
    EXPECT_FALSE(txns.empty());
    bool found = false;
    for (const auto& t : txns) {
        if (t.transaction_id == txn_id) { found = true; break; }
    }
    EXPECT_TRUE(found) << "Active transaction should appear in listInFlightTransactions()";
}

TEST_F(OrphanDetectorWiredTest, GetTransaction_ReturnsNulloptForUnknown) {
    auto result = dist_coord_->getTransaction("no-such-txn-abc");
    EXPECT_FALSE(result.has_value());
}

TEST_F(OrphanDetectorWiredTest, GetTransaction_ReturnsTransactionAfterBegin) {
    const std::string txn_id = "wired-test-txn-get-1";
    ASSERT_TRUE(txn_coord_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    auto result = dist_coord_->getTransaction(txn_id);
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->transaction_id, txn_id);
}

TEST(OrphanDetectorWiredNoCoordTest, GetTransaction_NullCoordReturnsNullopt) {
    // DistributedCoordinator without a registered txn coordinator.
    auto topology  = std::make_shared<ShardTopology>();
    GossipConfigManagerConfig gcfg;
    gcfg.local_shard_id = "shard-none";
    gcfg.local_endpoint = "localhost:50098";
    gcfg.enabled        = false;
    auto gossip = std::make_shared<GossipConfigManager>(gcfg, topology);
    DistributedCoordinator dist("shard-none", topology, gossip);

    EXPECT_FALSE(dist.getTransaction("anything").has_value());
    EXPECT_TRUE(dist.listInFlightTransactions().empty());
}

// ============================================================================
// detectOrphans – uses DistributedCoordinator path
// ============================================================================

TEST_F(OrphanDetectorWiredTest, DetectOrphans_EmptyCoordinator_ReturnsEmpty) {
    sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 1;
    sharding::OrphanDetector det(cfg, dist_coord_.get());

    // No transactions → no orphans (nullptr coordinator accepted)
    auto orphans = det.detectOrphans(nullptr);
    EXPECT_TRUE(orphans.empty());
}

TEST_F(OrphanDetectorWiredTest, DetectOrphans_FreshTransaction_NotOrphaned) {
    const std::string txn_id = "wired-fresh-txn";
    ASSERT_TRUE(txn_coord_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    // Very long timeout → fresh transaction is not orphaned.
    sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 900;
    sharding::OrphanDetector det(cfg, dist_coord_.get());

    auto orphans = det.detectOrphans(nullptr);
    EXPECT_TRUE(orphans.empty());
}

// ============================================================================
// isOrphaned – uses DistributedCoordinator path
// ============================================================================

TEST_F(OrphanDetectorWiredTest, IsOrphaned_UnknownTxn_ReturnsFalse) {
    sharding::OrphanDetector::Config cfg;
    sharding::OrphanDetector det(cfg, dist_coord_.get());

    EXPECT_FALSE(det.isOrphaned("no-such-txn", nullptr));
}

TEST_F(OrphanDetectorWiredTest, IsOrphaned_FreshTransaction_ReturnsFalse) {
    const std::string txn_id = "wired-isorphaned-fresh";
    ASSERT_TRUE(txn_coord_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 900;  // Long timeout → not orphaned
    sharding::OrphanDetector det(cfg, dist_coord_.get());

    EXPECT_FALSE(det.isOrphaned(txn_id, nullptr));
}

// ============================================================================
// Fallback – no DistributedCoordinator wired, uses per-call coordinator
// ============================================================================

TEST(OrphanDetectorFallbackTest, DetectOrphans_FallsBackToCallCoordinator) {
    auto txn_coord = makeTxnCoordinator();

    const std::string txn_id = "fallback-txn-1";
    ASSERT_TRUE(txn_coord->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    // No DistributedCoordinator → falls back to the per-call coordinator.
    sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 900;
    sharding::OrphanDetector det(cfg);  // default constructor (no dist_coord)

    auto orphans = det.detectOrphans(txn_coord);
    EXPECT_TRUE(orphans.empty());  // Fresh transaction is not orphaned.

    txn_coord->stop();
}

TEST(OrphanDetectorFallbackTest, IsOrphaned_FallsBackToCallCoordinator) {
    auto txn_coord = makeTxnCoordinator();

    const std::string txn_id = "fallback-isorphaned-1";
    ASSERT_TRUE(txn_coord->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));

    sharding::OrphanDetector::Config cfg;
    cfg.timeout_seconds = 900;
    sharding::OrphanDetector det(cfg);

    EXPECT_FALSE(det.isOrphaned(txn_id, txn_coord));

    txn_coord->stop();
}

// ============================================================================
// setTransactionCoordinator detach / re-attach
// ============================================================================

TEST_F(OrphanDetectorWiredTest, SetTransactionCoordinator_DetachThenReturnEmpty) {
    // Attach a transaction so list is non-empty.
    const std::string txn_id = "detach-test-txn";
    ASSERT_TRUE(txn_coord_->beginTransaction(
        txn_id, TransactionProtocol::TWO_PHASE_COMMIT,
        IsolationLevel::SNAPSHOT_ISOLATION));
    EXPECT_FALSE(dist_coord_->listInFlightTransactions().empty());

    // Detach → list becomes empty.
    dist_coord_->setTransactionCoordinator(nullptr);
    EXPECT_TRUE(dist_coord_->listInFlightTransactions().empty());

    // Re-attach → list is non-empty again.
    dist_coord_->setTransactionCoordinator(txn_coord_.get());
    EXPECT_FALSE(dist_coord_->listInFlightTransactions().empty());
}
