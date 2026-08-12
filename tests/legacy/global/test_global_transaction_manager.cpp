// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unit tests for GlobalTransactionManager (multi-region ACID guarantees)

#include <gtest/gtest.h>
#include "transaction/global_transaction_manager.h"
#include "sharding/truetime.h"
#include <atomic>
#include <mutex>
#include <stdexcept>
#include <string>
#include <vector>

using namespace themis::transaction;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers: mock participant
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief In-process mock region participant for testing.
 */
class MockRegionParticipant : public IGlobalRegionParticipant {
public:
    enum class PreparePolicy { ALWAYS_COMMIT, ALWAYS_ABORT, THROW };

    explicit MockRegionParticipant(PreparePolicy policy = PreparePolicy::ALWAYS_COMMIT)
        : policy_(policy) {}

    bool prepare(const std::string& txn_id, const nlohmann::json& /*ops*/) override {
        if (policy_ == PreparePolicy::THROW) {
            throw std::runtime_error("mock prepare failure");
        }
        {
            std::lock_guard<std::mutex> lk(str_mutex_);
            last_prepare_txn_ = txn_id;
        }
        ++prepare_count_;
        return policy_ == PreparePolicy::ALWAYS_COMMIT;
    }

    void commit(const std::string& txn_id, int64_t commit_ts) override {
        {
            std::lock_guard<std::mutex> lk(str_mutex_);
            last_commit_txn_ = txn_id;
        }
        last_commit_ts_.store(commit_ts);
        ++commit_count_;
    }

    void abort(const std::string& txn_id) override {
        {
            std::lock_guard<std::mutex> lk(str_mutex_);
            last_abort_txn_ = txn_id;
        }
        ++abort_count_;
    }

    // Query helpers
    int     prepareCount()    const { return prepare_count_.load(); }
    int     commitCount()     const { return commit_count_.load(); }
    int     abortCount()      const { return abort_count_.load(); }
    int64_t lastCommitTs()    const { return last_commit_ts_.load(); }

    std::string lastPrepareTxn() const {
        std::lock_guard<std::mutex> lk(str_mutex_);
        return last_prepare_txn_;
    }
    std::string lastCommitTxn() const {
        std::lock_guard<std::mutex> lk(str_mutex_);
        return last_commit_txn_;
    }
    std::string lastAbortTxn() const {
        std::lock_guard<std::mutex> lk(str_mutex_);
        return last_abort_txn_;
    }

private:
    PreparePolicy          policy_;
    std::atomic<int>       prepare_count_{0};
    std::atomic<int>       commit_count_{0};
    std::atomic<int>       abort_count_{0};
    std::atomic<int64_t>   last_commit_ts_{0};

    mutable std::mutex str_mutex_;
    std::string        last_prepare_txn_;
    std::string        last_commit_txn_;
    std::string        last_abort_txn_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Fixture
// ─────────────────────────────────────────────────────────────────────────────

class GlobalTransactionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Minimal TrueTime with a tiny uncertainty so tests don't wait long
        TrueTime::Config tt_cfg;
        tt_cfg.base_uncertainty_us  = 1;   // 1 µs → negligible commit-wait
        tt_cfg.sync_interval_s      = 300; // don't spawn NTP threads in tests
        truetime_ = std::make_shared<TrueTime>(tt_cfg);

        GlobalTransactionManager::Config cfg;
        // No WAL directory → WAL disabled (keeps tests hermetic)
        gtm_ = std::make_unique<GlobalTransactionManager>("test-coord", truetime_, cfg);
    }

    void TearDown() override {
        gtm_.reset();
        truetime_.reset();
    }

    std::shared_ptr<TrueTime>              truetime_;
    std::unique_ptr<GlobalTransactionManager> gtm_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Tests: region management
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, RegisterAndCountRegions) {
    MockRegionParticipant p1, p2;
    EXPECT_EQ(gtm_->regionCount(), 0u);

    gtm_->registerRegion("us-east-1", &p1);
    EXPECT_EQ(gtm_->regionCount(), 1u);

    gtm_->registerRegion("eu-west-1", &p2);
    EXPECT_EQ(gtm_->regionCount(), 2u);
}

TEST_F(GlobalTransactionManagerTest, UnregisterRegion) {
    MockRegionParticipant p1;
    gtm_->registerRegion("us-east-1", &p1);
    EXPECT_EQ(gtm_->regionCount(), 1u);

    EXPECT_TRUE(gtm_->unregisterRegion("us-east-1"));
    EXPECT_EQ(gtm_->regionCount(), 0u);

    EXPECT_FALSE(gtm_->unregisterRegion("us-east-1")); // already gone
}

TEST_F(GlobalTransactionManagerTest, RegisterNullParticipantThrows) {
    EXPECT_THROW(
        gtm_->registerRegion("us-east-1", nullptr),
        std::invalid_argument
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: beginTransaction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, BeginTransactionReturnsUniqueIds) {
    MockRegionParticipant p;
    gtm_->registerRegion("ap-northeast-1", &p);

    const auto id1 = gtm_->beginTransaction({"ap-northeast-1"});
    const auto id2 = gtm_->beginTransaction({"ap-northeast-1"});

    EXPECT_FALSE(id1.empty());
    EXPECT_FALSE(id2.empty());
    EXPECT_NE(id1, id2);
}

TEST_F(GlobalTransactionManagerTest, BeginTransactionEmptyRegionsThrows) {
    EXPECT_THROW(
        gtm_->beginTransaction({}),
        std::invalid_argument
    );
}

TEST_F(GlobalTransactionManagerTest, BeginTransactionUnknownRegionThrows) {
    EXPECT_THROW(
        gtm_->beginTransaction({"nonexistent-region"}),
        std::invalid_argument
    );
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: addOperation
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, AddOperationSuccess) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    nlohmann::json op = {{"type", "PUT"}, {"key", "users:1"}, {"value", "alice"}};

    EXPECT_TRUE(gtm_->addOperation(txn_id, "us-east-1", op));
}

TEST_F(GlobalTransactionManagerTest, AddOperationUnknownTxn) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    EXPECT_FALSE(gtm_->addOperation("no-such-txn", "us-east-1", {}));
}

TEST_F(GlobalTransactionManagerTest, AddOperationRegionNotInTxn) {
    MockRegionParticipant p1, p2;
    gtm_->registerRegion("us-east-1", &p1);
    gtm_->registerRegion("eu-west-1", &p2);

    // Transaction only involves us-east-1
    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    EXPECT_FALSE(gtm_->addOperation(txn_id, "eu-west-1", {}));
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: commit – happy path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, CommitSingleRegionSuccess) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    const auto outcome = gtm_->commit(txn_id);

    EXPECT_TRUE(outcome.committed());
    EXPECT_EQ(outcome.transaction_id, txn_id);
    EXPECT_GT(outcome.commit_timestamp_ns, 0);

    // Participant must have been called in order
    EXPECT_EQ(p.prepareCount(), 1);
    EXPECT_EQ(p.commitCount(),  1);
    EXPECT_EQ(p.abortCount(),   0);
    EXPECT_EQ(p.lastPrepareTxn(), txn_id);
    EXPECT_EQ(p.lastCommitTxn(),  txn_id);
    EXPECT_EQ(p.lastCommitTs(),   outcome.commit_timestamp_ns);
}

TEST_F(GlobalTransactionManagerTest, CommitMultipleRegionsSuccess) {
    MockRegionParticipant p1, p2, p3;
    gtm_->registerRegion("us-east-1",      &p1);
    gtm_->registerRegion("eu-west-1",      &p2);
    gtm_->registerRegion("ap-northeast-1", &p3);

    const auto txn_id = gtm_->beginTransaction({"us-east-1", "eu-west-1", "ap-northeast-1"});
    gtm_->addOperation(txn_id, "us-east-1",      {{"type","PUT"},{"key","k1"}});
    gtm_->addOperation(txn_id, "eu-west-1",      {{"type","PUT"},{"key","k2"}});
    gtm_->addOperation(txn_id, "ap-northeast-1", {{"type","DEL"},{"key","k3"}});

    const auto outcome = gtm_->commit(txn_id);
    EXPECT_TRUE(outcome.committed());

    EXPECT_EQ(p1.prepareCount(), 1); EXPECT_EQ(p1.commitCount(), 1);
    EXPECT_EQ(p2.prepareCount(), 1); EXPECT_EQ(p2.commitCount(), 1);
    EXPECT_EQ(p3.prepareCount(), 1); EXPECT_EQ(p3.commitCount(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: commit – abort paths
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, CommitAbortedWhenOneRegionVotesAbort) {
    MockRegionParticipant commit_p;
    MockRegionParticipant abort_p(MockRegionParticipant::PreparePolicy::ALWAYS_ABORT);

    gtm_->registerRegion("us-east-1", &commit_p);
    gtm_->registerRegion("eu-west-1", &abort_p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1", "eu-west-1"});
    const auto outcome = gtm_->commit(txn_id);

    EXPECT_EQ(outcome.result, GlobalTxnResult::ABORTED);
    EXPECT_FALSE(outcome.reason.empty());

    // Both participants receive an ABORT in Phase 2
    EXPECT_EQ(commit_p.abortCount(), 1);
    EXPECT_EQ(abort_p.abortCount(),  1);
    EXPECT_EQ(commit_p.commitCount(), 0);
    EXPECT_EQ(abort_p.commitCount(),  0);
}

TEST_F(GlobalTransactionManagerTest, CommitAbortedWhenParticipantThrows) {
    MockRegionParticipant throw_p(MockRegionParticipant::PreparePolicy::THROW);
    MockRegionParticipant ok_p;

    gtm_->registerRegion("us-east-1", &throw_p);
    gtm_->registerRegion("eu-west-1", &ok_p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1", "eu-west-1"});
    const auto outcome = gtm_->commit(txn_id);

    EXPECT_EQ(outcome.result, GlobalTxnResult::ABORTED);
    EXPECT_EQ(ok_p.commitCount(), 0);
}

TEST_F(GlobalTransactionManagerTest, CommitUnknownTxnReturnsError) {
    const auto outcome = gtm_->commit("nonexistent-txn");
    EXPECT_EQ(outcome.result, GlobalTxnResult::ERROR);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: idempotent re-commit
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, IdempotentRecommit) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    const auto first  = gtm_->commit(txn_id);
    ASSERT_TRUE(first.committed());

    // Second commit should return COMMITTED without calling the participant again
    const auto second = gtm_->commit(txn_id);
    EXPECT_TRUE(second.committed());
    EXPECT_EQ(p.prepareCount(), 1);   // NOT incremented
    EXPECT_EQ(p.commitCount(),  1);   // NOT incremented
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: explicit abort
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, ExplicitAbortBeforeCommit) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    EXPECT_TRUE(gtm_->abort(txn_id));

    // Transaction is now completed (aborted); re-abort should succeed
    EXPECT_TRUE(gtm_->abort(txn_id));

    // No prepare was ever sent
    EXPECT_EQ(p.prepareCount(), 0);
    EXPECT_EQ(p.commitCount(),  0);
}

TEST_F(GlobalTransactionManagerTest, AbortUnknownTxnReturnsFalse) {
    EXPECT_FALSE(gtm_->abort("does-not-exist"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: transaction state introspection
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, GetTransactionStateAfterBegin) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    const auto state  = gtm_->getTransactionState(txn_id);

    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, GlobalTxnState::ACTIVE);
}

TEST_F(GlobalTransactionManagerTest, GetTransactionStateAfterCommit) {
    MockRegionParticipant p;
    gtm_->registerRegion("us-east-1", &p);

    const auto txn_id = gtm_->beginTransaction({"us-east-1"});
    gtm_->commit(txn_id);

    const auto state = gtm_->getTransactionState(txn_id);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, GlobalTxnState::COMPLETED);
}

TEST_F(GlobalTransactionManagerTest, GetTransactionStateUnknownTxn) {
    EXPECT_FALSE(gtm_->getTransactionState("no-such-txn").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(GlobalTransactionManagerTest, StatisticsAfterCommitsAndAborts) {
    MockRegionParticipant p_ok;
    MockRegionParticipant p_abort(MockRegionParticipant::PreparePolicy::ALWAYS_ABORT);

    gtm_->registerRegion("us-east-1", &p_ok);
    gtm_->registerRegion("eu-west-1", &p_abort);

    // Two committed transactions (single-region)
    for (int i = 0; i < 2; ++i) {
        auto tid = gtm_->beginTransaction({"us-east-1"});
        gtm_->commit(tid);
    }

    // One aborted transaction (two-region – second region votes ABORT)
    auto tid_abort = gtm_->beginTransaction({"us-east-1", "eu-west-1"});
    gtm_->commit(tid_abort);

    const auto stats = gtm_->getStatistics();
    EXPECT_EQ(stats["total_transactions"].get<uint64_t>(), 3u);
    EXPECT_EQ(stats["total_commits"].get<uint64_t>(),      2u);
    EXPECT_EQ(stats["total_aborts"].get<uint64_t>(),       1u);
    EXPECT_EQ(stats["registered_regions"].get<size_t>(),   2u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Tests: null TrueTime guard
// ─────────────────────────────────────────────────────────────────────────────

TEST(GlobalTransactionManagerNullTrueTimeTest, ConstructorThrowsOnNullTrueTime) {
    EXPECT_THROW(
        GlobalTransactionManager("coord", nullptr),
        std::invalid_argument
    );
}
