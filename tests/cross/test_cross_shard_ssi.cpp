/*
 * ThemisDB | Test: test_cross_shard_ssi.cpp
 * Unit tests for Distributed Serializable Snapshot Isolation (SSI):
 *   - CrossShardSSIManager read/write-set registration
 *   - RW conflict detection (write-skew, phantom)
 *   - WW conflict detection (lost update)
 *   - Cross-transaction isolation (no false positives within same txn)
 *   - Lifecycle: clearTransaction() cleans up state
 *   - Configuration: predicate locking disable disables detection
 *   - CrossShardTransactionCoordinator integration
 */

#include <gtest/gtest.h>

#include "sharding/consensus_module.h"
#include "sharding/cross_shard_ssi_manager.h"
#include "sharding/cross_shard_transaction.h"

#include <memory>
#include <string>
#include <vector>

namespace themisdb { namespace sharding { 

// ============================================================================
// Minimal FakeConsensusModule (reuse pattern from focused tests)
// ============================================================================

class FakeConsensusModuleSSI final : public ConsensusModule {
public:
    [[nodiscard]] ConsensusType getType() const override { return ConsensusType::RAFT; }
    [[nodiscard]] bool initialize(const std::string&, const std::vector<std::string>&) override { return true; }
    [[nodiscard]] bool start() override { return true; }
    void stop() override {}
    [[nodiscard]] bool isLeader() const override { return true; }
    [[nodiscard]] std::string getLeaderId() const override { return "leader"; }
    [[nodiscard]] ConsensusState getState() const override { return ConsensusState::LEADER; }
    [[nodiscard]] std::optional<uint64_t> propose(const std::string&, const nlohmann::json&) override {
        return ++idx_;
    }
    [[nodiscard]] bool waitForCommit(uint64_t, std::chrono::milliseconds) override { return true; }
    [[nodiscard]] std::vector<ConsensusLogEntry> readLog(uint64_t, std::optional<uint64_t>) override { return {}; }
    [[nodiscard]] uint64_t getCommitIndex() const override { return idx_; }
    [[nodiscard]] uint64_t getLastLogIndex() const override { return idx_; }
    [[nodiscard]] bool addNode(const std::string&, const std::string&) override { return true; }
    [[nodiscard]] bool removeNode(const std::string&) override { return true; }
    [[nodiscard]] bool transferLeadership(const std::string&) override { return true; }
    [[nodiscard]] bool takeSnapshot(const nlohmann::json&) override { return true; }
    [[nodiscard]] bool restoreSnapshot(const nlohmann::json&) override { return true; }
    [[nodiscard]] ConsensusStats getStats() const override {
        ConsensusStats s{};
        s.current_term = 1;
        s.commit_index = idx_;
        s.last_applied = idx_;
        s.state = ConsensusState::LEADER;
        s.current_leader = "leader";
        s.cluster_size = 1;
        s.reachable_nodes = 1;
        return s;
    }
    [[nodiscard]] nlohmann::json getStatus() const override { return {}; }
    void onCommit(std::function<void(const ConsensusLogEntry&)>) override {}
    void onStateChange(std::function<void(ConsensusState, ConsensusState)>) override {}
    void onLeaderChange(std::function<void(const std::string&, const std::string&)>) override {}
private:
    uint64_t idx_ = 0;
};

// ============================================================================
// CrossShardSSIManager unit tests
// ============================================================================

class CrossShardSSIManagerTest : public ::testing::Test {
protected:
    CrossShardSSIManager mgr;
};

// ── Basic registration ───────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, InitiallyNoTrackedTransactions) {
    EXPECT_EQ(mgr.trackedTransactionCount(), 0u);
}

TEST_F(CrossShardSSIManagerTest, RegisterReadSetIncreasesTrackedCount) {
    CrossShardPredicateLock pl{"shard1", "a", "z"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    EXPECT_EQ(mgr.trackedTransactionCount(), 1u);
}

TEST_F(CrossShardSSIManagerTest, RegisterWriteSetIncreasesTrackedCount) {
    mgr.registerWriteSet("txn1", "shard1", {"key_x"});
    EXPECT_EQ(mgr.trackedTransactionCount(), 1u);
}

TEST_F(CrossShardSSIManagerTest, ClearTransactionReducesCount) {
    CrossShardPredicateLock pl{"shard1", "a", "z"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn1", "shard1", {"key_x"});
    EXPECT_EQ(mgr.trackedTransactionCount(), 1u);

    mgr.clearTransaction("txn1");
    EXPECT_EQ(mgr.trackedTransactionCount(), 0u);
}

TEST_F(CrossShardSSIManagerTest, ClearUnknownTransactionIsNoOp) {
    EXPECT_NO_THROW(mgr.clearTransaction("does_not_exist"));
    EXPECT_EQ(mgr.trackedTransactionCount(), 0u);
}

// ── No-conflict cases ────────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, NoConflictWhenOnlyOneTxn) {
    CrossShardPredicateLock pl{"shard1", "a", "m"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn1", "shard1", {"key_b"});

    auto conflicts = mgr.validateAtPrepare("txn1");
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(CrossShardSSIManagerTest, NoConflictWhenWriteKeyOutsideOtherReadRange) {
    // txn1 reads [a, m]; txn2 writes key outside that range.
    CrossShardPredicateLock pl{"shard1", "a", "m"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn2", "shard1", {"z_key"});  // "z" > "m"

    auto conflicts = mgr.validateAtPrepare("txn2");
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(CrossShardSSIManagerTest, NoConflictWhenPredicateLockingDisabled) {
    CrossShardSSIManager::Config cfg;
    cfg.enable_predicate_locking = false;
    mgr.setConfig(cfg);

    CrossShardPredicateLock pl{"shard1", "a", "z"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn2", "shard1", {"key_b"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    EXPECT_TRUE(conflicts.empty());
}

// ── RW conflict detection ────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, RWConflictDetected_WriteInOtherReadRange) {
    // txn1 read predicate covers [a, z]; txn2 writes "key_b" — overlap.
    CrossShardPredicateLock pl{"shard1", "a", "z"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn2", "shard1", {"key_b"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "rw");
    EXPECT_EQ(conflicts.front().transaction_id, "txn2");
    EXPECT_EQ(conflicts.front().conflicting_transaction_id, "txn1");
}

TEST_F(CrossShardSSIManagerTest, RWConflictDetected_OtherWriteInMyReadRange) {
    // txn2 read predicate covers [a, z]; txn1 already wrote "key_b".
    CrossShardPredicateLock pl{"shard1", "a", "z"};
    mgr.registerReadSet("txn2", "shard1", {pl});
    mgr.registerWriteSet("txn1", "shard1", {"key_b"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "rw");
    EXPECT_EQ(conflicts.front().transaction_id, "txn2");
    EXPECT_EQ(conflicts.front().conflicting_transaction_id, "txn1");
}

TEST_F(CrossShardSSIManagerTest, RWConflictDetected_CrossShardPredicateAndWrite) {
    // txn1 reads range on shard1; txn2 writes same key on shard2.
    // (Simulates hash-partitioned namespace where same logical key can land on
    //  different shards — coordinator tracks application-level keys.)
    CrossShardPredicateLock pl{"shard1", "acc:", "acc:~"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn2", "shard2", {"acc:42"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "rw");
}

TEST_F(CrossShardSSIManagerTest, SingleKeyPredicateConflict) {
    // Single-key predicate (end_key empty) — txn2 writes exact same key.
    CrossShardPredicateLock pl{"shard1", "exact_key", ""};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn2", "shard1", {"exact_key"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "rw");
}

TEST_F(CrossShardSSIManagerTest, SingleKeyPredicateNoConflictDifferentKey) {
    CrossShardPredicateLock pl{"shard1", "exact_key", ""};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.registerWriteSet("txn2", "shard1", {"other_key"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    EXPECT_TRUE(conflicts.empty());
}

// ── WW conflict detection ────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, WWConflictDetected_SameKeySameShard) {
    mgr.registerWriteSet("txn1", "shard1", {"shared_key"});
    mgr.registerWriteSet("txn2", "shard1", {"shared_key"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "ww");
    EXPECT_EQ(conflicts.front().key, "shared_key");
}

TEST_F(CrossShardSSIManagerTest, WWConflictDetected_SameKeyDifferentShards) {
    // Application-level key overlap across shards (hash partitioning scenario).
    mgr.registerWriteSet("txn1", "shard1", {"app:entity:100"});
    mgr.registerWriteSet("txn2", "shard2", {"app:entity:100"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "ww");
}

TEST_F(CrossShardSSIManagerTest, NoWWConflictDifferentKeys) {
    mgr.registerWriteSet("txn1", "shard1", {"key_a"});
    mgr.registerWriteSet("txn2", "shard1", {"key_b"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    EXPECT_TRUE(conflicts.empty());
}

// ── Lifecycle / cleanup ──────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, AfterClearNoConflictDetected) {
    CrossShardPredicateLock pl{"shard1", "a", "z"};
    mgr.registerReadSet("txn1", "shard1", {pl});
    mgr.clearTransaction("txn1");

    mgr.registerWriteSet("txn2", "shard1", {"key_b"});
    auto conflicts = mgr.validateAtPrepare("txn2");
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(CrossShardSSIManagerTest, MultipleTransactionsOnlyConflictingOnesReported) {
    // txn1 reads [a, m]; txn3 reads [n, z].
    // txn2 writes "key_b" (conflicts with txn1 only).
    CrossShardPredicateLock pl1{"shard1", "a", "m"};
    CrossShardPredicateLock pl3{"shard1", "n", "z"};
    mgr.registerReadSet("txn1", "shard1", {pl1});
    mgr.registerReadSet("txn3", "shard1", {pl3});
    mgr.registerWriteSet("txn2", "shard1", {"key_b"});

    auto conflicts = mgr.validateAtPrepare("txn2");
    ASSERT_EQ(conflicts.size(), 1u);
    EXPECT_EQ(conflicts.front().conflicting_transaction_id, "txn1");
}

// ── Configuration ────────────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, DefaultConfigHasPredicateLockingEnabled) {
    auto cfg = mgr.getConfig();
    EXPECT_TRUE(cfg.enable_predicate_locking);
}

TEST_F(CrossShardSSIManagerTest, SetConfigUpdatesEffectively) {
    CrossShardSSIManager::Config cfg;
    cfg.enable_predicate_locking = false;
    cfg.max_predicate_locks_per_txn = 5;
    mgr.setConfig(cfg);

    auto got = mgr.getConfig();
    EXPECT_FALSE(got.enable_predicate_locking);
    EXPECT_EQ(got.max_predicate_locks_per_txn, 5u);
}

// ── Edge cases ───────────────────────────────────────────────────────────────

TEST_F(CrossShardSSIManagerTest, EmptyTransactionIdIgnored) {
    EXPECT_NO_THROW(mgr.registerWriteSet("", "shard1", {"key"}));
    EXPECT_NO_THROW(mgr.registerReadSet("", "shard1", {}));
    EXPECT_NO_THROW(mgr.clearTransaction(""));
    auto conflicts = mgr.validateAtPrepare("");
    EXPECT_TRUE(conflicts.empty());
}

TEST_F(CrossShardSSIManagerTest, ValidateUnknownTxnReturnsEmpty) {
    auto conflicts = mgr.validateAtPrepare("nonexistent_txn");
    EXPECT_TRUE(conflicts.empty());
}

// ============================================================================
// CrossShardTransactionCoordinator integration tests
// ============================================================================

/// Helper to make a coordinator with a safe temp log path and SSI enabled.
static CrossShardTransactionCoordinator makeTestCoordinator() {
    CrossShardTransactionConfig cfg;
    cfg.transaction_log_path = "/tmp/test_ssi_txn.jsonl";
    cfg.ssi_config.enable_predicate_locking = true;
    auto consensus = std::make_shared<FakeConsensusModuleSSI>();
    return CrossShardTransactionCoordinator(cfg, consensus);
}

TEST(CrossShardCoordinatorSSITest, RegisterReadWriteSetNoThrow) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping on Windows (path/file constraints)";
#endif
    auto coord = makeTestCoordinator();

    CrossShardPredicateLock pl{"shard1", "user:", "user:~"};
    EXPECT_NO_THROW(coord.registerShardReadSet("txn_a", "shard1", {pl}));
    EXPECT_NO_THROW(coord.registerShardWriteSet("txn_a", "shard1", {"user:42"}));
}

TEST(CrossShardCoordinatorSSITest, ValidateCrossShardSSIReturnsConflict) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping on Windows (path/file constraints)";
#endif
    auto coord = makeTestCoordinator();

    // txn_reader performs a range scan.
    CrossShardPredicateLock pl{"shard1", "order:", "order:~"};
    coord.registerShardReadSet("txn_reader", "shard1", {pl});

    // txn_writer inserts into the same range on a different shard.
    coord.registerShardWriteSet("txn_writer", "shard2", {"order:999"});

    auto conflicts = coord.validateCrossShardSSI("txn_writer");
    ASSERT_FALSE(conflicts.empty());
    EXPECT_EQ(conflicts.front().conflict_type, "rw");
}

TEST(CrossShardCoordinatorSSITest, SetGetSSIConfig) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping on Windows (path/file constraints)";
#endif
    auto coord = makeTestCoordinator();

    CrossShardSSIManager::Config cfg;
    cfg.enable_predicate_locking = false;
    coord.setSSIConfig(cfg);

    EXPECT_FALSE(coord.getSSIConfig().enable_predicate_locking);
}

TEST(CrossShardCoordinatorSSITest, ValidateCrossShardSSINoConflictWhenDisabled) {
#ifdef _WIN32
    GTEST_SKIP() << "Skipping on Windows (path/file constraints)";
#endif
    auto coord = makeTestCoordinator();

    // Disable SSI first.
    CrossShardSSIManager::Config cfg;
    cfg.enable_predicate_locking = false;
    coord.setSSIConfig(cfg);

    CrossShardPredicateLock pl{"shard1", "a", "z"};
    coord.registerShardReadSet("txn1", "shard1", {pl});
    coord.registerShardWriteSet("txn2", "shard1", {"key_b"});

    auto conflicts = coord.validateCrossShardSSI("txn2");
    EXPECT_TRUE(conflicts.empty());
}
} } // namespace themisdb::sharding
