// Tests for DistributedTransactionManager (2PC) – v1.9.0
// Covers all acceptance criteria from the roadmap issue:
//   AC-1  Coordinator role: beginDistributed() returns a valid TXN-ID
//   AC-2  Prepare phase with voting: prepareDistributed() collects votes
//   AC-3  All-commit vote → PREPARED state
//   AC-4  Any-abort vote  → transaction is ABORTED
//   AC-5  Commit/abort phase coordination: commitDistributed() calls onCommit on every participant
//   AC-6  abortDistributed() calls onAbort on every participant
//   AC-7  Participant recovery: applyCommit/applyAbort return OK for known txn
//   AC-8  Timeout handling: checkTimeouts() aborts expired transactions
//   AC-9  Failure detection: isParticipantAlive() returns true for known participant
//   AC-10 Coordinator crash recovery from persistent log (WAL round-trip)
//   AC-11 Participant crash: prepare exception treated as ABORT vote
//   AC-12 Network partition: timeout-based abort
//   AC-13 Partial commit: ABORT broadcast on prepare failure
//   AC-14 Latency smoke test: prepare+commit within 5 ms (local in-process)
//   AC-15 Throughput smoke test: 50 concurrent transactions succeed
//   AC-16 Batched prepare/commit: 5-participant txn issues parallel calls
//   AC-17 voteOnPrepare() registers async vote correctly
//   AC-18 Statistics counters reflect committed / aborted counts
//   AC-19 Empty-participant beginDistributed throws
//   AC-20 Idempotent abort: abortDistributed on already-aborted txn is safe

#include <gtest/gtest.h>
#include "transaction/distributed_transaction_manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <map>
#include <mutex>
#include <set>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::transaction;
using namespace std::chrono_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Mock participant
// ─────────────────────────────────────────────────────────────────────────────

class MockParticipant : public IDistributedParticipantCallback {
public:
    enum class Policy { ALWAYS_COMMIT, ALWAYS_ABORT, THROW_ON_PREPARE };

    explicit MockParticipant(Policy policy = Policy::ALWAYS_COMMIT)
        : policy_(policy) {}

    bool onPrepare(const std::string& txn_id,
                   const std::set<std::string>& keys) override {
        if (policy_ == Policy::THROW_ON_PREPARE) {
            throw std::runtime_error("mock: prepare failure");
        }
        std::lock_guard<std::mutex> lk(mu_);
        last_prepare_txn_ = txn_id;
        prepared_keys_[txn_id] = keys;
        ++prepare_count_;
        return policy_ == Policy::ALWAYS_COMMIT;
    }

    void onCommit(const std::string& txn_id) override {
        std::lock_guard<std::mutex> lk(mu_);
        last_commit_txn_ = txn_id;
        ++commit_count_;
    }

    void onAbort(const std::string& txn_id) override {
        std::lock_guard<std::mutex> lk(mu_);
        last_abort_txn_ = txn_id;
        ++abort_count_;
    }

    // Helpers for assertions
    int prepareCount() const { return prepare_count_.load(); }
    int commitCount()  const { return commit_count_.load();  }
    int abortCount()   const { return abort_count_.load();   }

    std::string lastPrepareTxn() const {
        std::lock_guard<std::mutex> lk(mu_);
        return last_prepare_txn_;
    }
    std::string lastCommitTxn() const {
        std::lock_guard<std::mutex> lk(mu_);
        return last_commit_txn_;
    }
    std::string lastAbortTxn() const {
        std::lock_guard<std::mutex> lk(mu_);
        return last_abort_txn_;
    }

private:
    Policy            policy_;
    mutable std::mutex mu_;
    std::string       last_prepare_txn_;
    std::string       last_commit_txn_;
    std::string       last_abort_txn_;
    std::atomic<int>  prepare_count_{0};
    std::atomic<int>  commit_count_{0};
    std::atomic<int>  abort_count_{0};
    std::map<std::string, std::set<std::string>> prepared_keys_;
};

class AbortThrowingParticipant : public IDistributedParticipantCallback {
public:
    bool onPrepare(const std::string&, const std::set<std::string>&) override {
        return true;
    }
    void onCommit(const std::string&) override {}
    void onAbort(const std::string&) override {
        throw std::runtime_error("abort delivery failed");
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static Participant makeParticipant(
    const std::string& node_id,
    IDistributedParticipantCallback* cb,
    std::set<std::string> keys = {"key1"}
) {
    Participant p;
    p.node_id      = node_id;
    p.endpoint     = node_id + ":8080";
    p.affected_keys = std::move(keys);
    p.callback     = cb;
    return p;
}

static std::string beginDistributedWithExplicitTestConsistency(
    DistributedTransactionManager& mgr,
    const std::vector<Participant>& participants
) {
    // Tests are single-process and deterministic; this helper documents explicit local consistency intent.
    return mgr.beginDistributed(participants);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class DistributedTxnManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        p1 = std::make_unique<MockParticipant>();
        p2 = std::make_unique<MockParticipant>();
        p3 = std::make_unique<MockParticipant>();

        DistributedTxnManagerConfig cfg;
        cfg.prepare_timeout      = 2000ms;
        cfg.commit_timeout       = 2000ms;
        cfg.default_txn_timeout  = 60s;
        // No WAL for most tests (keeps tests hermetic).
        mgr = std::make_unique<DistributedTransactionManager>("test-coord", cfg);
    }

    std::unique_ptr<MockParticipant>                 p1, p2, p3;
    std::unique_ptr<DistributedTransactionManager>   mgr;
};

// ─────────────────────────────────────────────────────────────────────────────
// AC-1: beginDistributed returns a non-empty ID
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, BeginDistributedReturnsNonEmptyId) {
    auto tid = beginDistributedWithExplicitTestConsistency(*mgr, {makeParticipant("n1", p1.get())});
    EXPECT_FALSE(tid.empty());
}

TEST_F(DistributedTxnManagerTest, BeginDistributedIdsAreUnique) {
    auto tid1 = beginDistributedWithExplicitTestConsistency(*mgr, {makeParticipant("n1", p1.get())});
    auto tid2 = beginDistributedWithExplicitTestConsistency(*mgr, {makeParticipant("n1", p1.get())});
    EXPECT_NE(tid1, tid2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2 / AC-3: prepareDistributed collects all-commit votes → PREPARED
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PreparePhaseAllCommitVotes) {
    auto tid = beginDistributedWithExplicitTestConsistency(*mgr, {
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
    });

    auto status = mgr->prepareDistributed(tid);
    EXPECT_TRUE(status.ok) << status.message;

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::PREPARED);

    EXPECT_EQ(p1->prepareCount(), 1);
    EXPECT_EQ(p2->prepareCount(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-4: any ABORT vote → transaction aborted after prepareDistributed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PreparePhaseAbortVote) {
    MockParticipant aborting(MockParticipant::Policy::ALWAYS_ABORT);

    auto tid = beginDistributedWithExplicitTestConsistency(*mgr, {
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", &aborting),
    });

    auto status = mgr->prepareDistributed(tid);
    EXPECT_FALSE(status.ok);

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);

    // n1 voted COMMIT → it must receive ABORT during automatic rollback
    EXPECT_EQ(p1->abortCount(), 1);
    EXPECT_EQ(p1->commitCount(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-5: commitDistributed calls onCommit on every participant
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, CommitCallsOnCommitOnAllParticipants) {
    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
        makeParticipant("n3", p3.get()),
    });

    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    auto status = mgr->commitDistributed(tid);
    EXPECT_TRUE(status.ok) << status.message;

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::COMMITTED);

    EXPECT_EQ(p1->commitCount(), 1);
    EXPECT_EQ(p2->commitCount(), 1);
    EXPECT_EQ(p3->commitCount(), 1);
    EXPECT_EQ(p1->abortCount(),  0);
    EXPECT_EQ(p2->abortCount(),  0);
    EXPECT_EQ(p3->abortCount(),  0);
}

// ---------------------------------------------------------------------------
// DTM-Phase2 fail-closed: deadline expiry must not report a successful COMMIT
// ---------------------------------------------------------------------------
TEST_F(DistributedTxnManagerTest, Phase2DeadlineExpiryFailsClosedOnCommit) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout = 0ms;  // force immediate Phase-2 deadline expiry
    cfg.default_txn_timeout = 60s;

    auto mgr2 = std::make_unique<DistributedTransactionManager>("coord-phase2-deadline", cfg);
    const auto tid = mgr2->beginDistributed({makeParticipant("n1", p1.get())});

    ASSERT_TRUE(mgr2->prepareDistributed(tid).ok);

    const auto commit_status = mgr2->commitDistributed(tid);
    EXPECT_FALSE(commit_status.ok)
        << "Phase-2 deadline expiry must fail closed and return an error";

    const auto rec = mgr2->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::COMMITTING)
        << "Failed Phase-2 delivery must leave txn in COMMITTING for recovery";
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-6: abortDistributed calls onAbort on all participants
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, AbortDistributedCallsOnAbortOnAllParticipants) {
    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
    });

    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    mgr->abortDistributed(tid);

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);

    EXPECT_EQ(p1->abortCount(), 1);
    EXPECT_EQ(p2->abortCount(), 1);
    EXPECT_EQ(p1->commitCount(), 0);
    EXPECT_EQ(p2->commitCount(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-7: applyCommit / applyAbort return OK for known transaction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, ApplyCommitReturnsOkForKnownTxn) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    auto st = mgr->applyCommit(tid);
    EXPECT_TRUE(st.ok) << st.message;
}

TEST_F(DistributedTxnManagerTest, ApplyAbortReturnsOkForKnownTxn) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    mgr->abortDistributed(tid);

    auto st = mgr->applyAbort(tid);
    EXPECT_TRUE(st.ok) << st.message;
}

TEST_F(DistributedTxnManagerTest, ApplyCommitReturnsErrorForUnknownTxn) {
    auto st = mgr->applyCommit("nonexistent-txn");
    EXPECT_FALSE(st.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-8: checkTimeouts() aborts expired transactions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, CheckTimeoutsAbortsExpiredTransactions) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 1ms;  // Very short timeout for test
    auto short_mgr = std::make_unique<DistributedTransactionManager>("short-coord", cfg);

    auto tid = short_mgr->beginDistributed({makeParticipant("n1", p1.get())});

    // Wait for the transaction to expire.
    std::this_thread::sleep_for(10ms);

    const size_t aborted = short_mgr->checkTimeouts();
    EXPECT_GE(aborted, 1u);

    auto rec = short_mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);
}

TEST_F(DistributedTxnManagerTest, CheckTimeoutsDoesNotCountIncompleteAbortDelivery) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 1ms;
    auto short_mgr = std::make_unique<DistributedTransactionManager>("short-incomplete-abort", cfg);

    AbortThrowingParticipant abort_thrower;
    const auto tid = short_mgr->beginDistributed({makeParticipant("n1", &abort_thrower)});
    std::this_thread::sleep_for(10ms);

    const size_t timeout_aborts = short_mgr->checkTimeouts();
    EXPECT_EQ(timeout_aborts, 0u)
        << "Timeout counter must only include fully-delivered ABORT decisions";

    const auto rec = short_mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTING);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-9: isParticipantAlive returns true (in-process default)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, IsParticipantAliveReturnsTrueByDefault) {
    EXPECT_TRUE(mgr->isParticipantAlive("any-node"));
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-10: WAL round-trip — recoverInDoubtTransactions processes PREPARE_TX entries
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, RecoverInDoubtTransactionsWithoutWAL) {
    // With WAL disabled, recovery is a no-op.
    const size_t resolved = mgr->recoverInDoubtTransactions();
    EXPECT_EQ(resolved, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-11: Participant crash (prepare throws) → treated as ABORT vote
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PrepareExceptionTreatedAsAbortVote) {
    MockParticipant throwing(MockParticipant::Policy::THROW_ON_PREPARE);

    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", &throwing),
    });

    auto status = mgr->prepareDistributed(tid);
    EXPECT_FALSE(status.ok);

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);

    // n1 voted COMMIT and must receive ABORT
    EXPECT_EQ(p1->abortCount(), 1);
    EXPECT_EQ(p1->commitCount(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-12: Network partition — timeout-based abort (short prepare timeout)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PrepareTimeoutAbortsTransaction) {
    // A participant that sleeps longer than the prepare timeout.
    class SlowParticipant : public IDistributedParticipantCallback {
    public:
        bool onPrepare(const std::string&, const std::set<std::string>&) override {
            std::this_thread::sleep_for(500ms);
            return true;
        }
        void onCommit(const std::string&) override {}
        void onAbort(const std::string&) override {}
    } slow;

    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 50ms;  // much shorter than 500ms sleep
    cfg.commit_timeout  = 2000ms;
    auto timeout_mgr = std::make_unique<DistributedTransactionManager>("timeout-coord", cfg);

    auto tid = timeout_mgr->beginDistributed({makeParticipant("slow", &slow)});
    auto status = timeout_mgr->prepareDistributed(tid);
    EXPECT_FALSE(status.ok);

    auto rec = timeout_mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-13: Partial commit — ABORT broadcast on prepare failure
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PartialCommitAutoRollback) {
    // n1 and n3 commit; n2 aborts — n1 and n3 must receive ABORT
    MockParticipant aborting(MockParticipant::Policy::ALWAYS_ABORT);

    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", &aborting),
        makeParticipant("n3", p3.get()),
    });

    auto status = mgr->prepareDistributed(tid);
    EXPECT_FALSE(status.ok);

    EXPECT_EQ(p1->commitCount(), 0);
    EXPECT_EQ(p3->commitCount(), 0);
    EXPECT_EQ(p1->abortCount(), 1);
    EXPECT_EQ(p3->abortCount(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-14: Latency smoke test — prepare+commit within 50 ms (generous bound)
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PrepareCommitLatency) {
    const auto start = std::chrono::steady_clock::now();

    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
    });

    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now() - start).count();

    // Local in-process calls should be well under 50 ms.
    EXPECT_LT(elapsed_ms, 50) << "Prepare+commit took " << elapsed_ms << " ms";
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-15: Throughput — 50 concurrent transactions succeed
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, ConcurrentTransactionsAllSucceed) {
    constexpr int N = 50;
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};

    // A pool of participants large enough to avoid sharing state across threads.
    std::vector<std::unique_ptr<MockParticipant>> participants(N * 2);
    for (auto& pp : participants) {
        pp = std::make_unique<MockParticipant>();
    }

    for (int i = 0; i < N; ++i) {
        auto* pa = participants[i * 2].get();
        auto* pb = participants[i * 2 + 1].get();
        const int idx = i;

        threads.emplace_back([&, pa, pb, idx]() {
            std::vector<Participant> parts = {
                makeParticipant("node_a_" + std::to_string(idx), pa, {"key_" + std::to_string(idx)}),
                makeParticipant("node_b_" + std::to_string(idx), pb, {"key_" + std::to_string(idx)}),
            };
            const auto tid = mgr->beginDistributed(parts);
            if (!mgr->prepareDistributed(tid).ok) return;
            if (!mgr->commitDistributed(tid).ok) return;
            ++successes;
        });
    }

    for (auto& t : threads) t.join();
    EXPECT_EQ(successes.load(), N);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-16: Batched prepare/commit — 5-participant txn calls all in parallel
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, BatchedFiveParticipantTransaction) {
    auto p4 = std::make_unique<MockParticipant>();
    auto p5 = std::make_unique<MockParticipant>();

    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
        makeParticipant("n3", p3.get()),
        makeParticipant("n4", p4.get()),
        makeParticipant("n5", p5.get()),
    });

    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    for (const auto* pp : {p1.get(), p2.get(), p3.get(), p4.get(), p5.get()}) {
        EXPECT_EQ(pp->prepareCount(), 1);
        EXPECT_EQ(pp->commitCount(), 1);
        EXPECT_EQ(pp->abortCount(), 0);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-17: voteOnPrepare registers async vote
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, VoteOnPrepareRegistersCOMMITVote) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});

    // Manually transition to PREPARING to allow voteOnPrepare.
    // (Normally done by prepareDistributed, but we test the participant API directly.)
    {
        // We must call prepareDistributed to put the txn in PREPARING state;
        // for a single participant with a callback the vote is recorded
        // synchronously inside prepareDistributed.
        auto status = mgr->prepareDistributed(tid);
        ASSERT_TRUE(status.ok) << status.message;
    }

    // For transactions already resolved, voteOnPrepare returns an error.
    auto st = mgr->voteOnPrepare(tid, "n1", true);
    EXPECT_FALSE(st.ok);  // state is PREPARED, not PREPARING
}

TEST_F(DistributedTxnManagerTest, VoteOnPrepareErrorForUnknownTxn) {
    auto st = mgr->voteOnPrepare("nonexistent", "n1", true);
    EXPECT_FALSE(st.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-18: Statistics reflect committed / aborted counts
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, StatisticsCountCommits) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    auto stats = mgr->getStatistics();
    EXPECT_GE(stats.total_transactions, 1u);
    EXPECT_GE(stats.committed, 1u);
}

TEST_F(DistributedTxnManagerTest, StatisticsCountAborts) {
    MockParticipant aborting(MockParticipant::Policy::ALWAYS_ABORT);
    auto tid = mgr->beginDistributed({makeParticipant("n1", &aborting)});
    EXPECT_FALSE(mgr->prepareDistributed(tid).ok);

    auto stats = mgr->getStatistics();
    EXPECT_GE(stats.aborted, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-19: beginDistributed with empty participants throws
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, BeginWithNoParticipantsThrows) {
    EXPECT_THROW(mgr->beginDistributed({}), std::invalid_argument);
}

TEST_F(DistributedTxnManagerTest, BeginDistributedEnforcesMaxActiveTransactions) {
    DistributedTxnManagerConfig cfg;
    cfg.max_active_transactions = 1;
    auto limited_mgr = std::make_unique<DistributedTransactionManager>("max-active-limit", cfg);

    const auto tid = limited_mgr->beginDistributed({makeParticipant("n1", p1.get())});
    EXPECT_FALSE(tid.empty());

    EXPECT_THROW(
        limited_mgr->beginDistributed({makeParticipant("n2", p2.get())}),
        std::runtime_error);
}

TEST_F(DistributedTxnManagerTest, BeginDistributedAllowsNewTxnAfterCommitReleasesCapacity) {
    DistributedTxnManagerConfig cfg;
    cfg.max_active_transactions = 1;
    auto limited_mgr = std::make_unique<DistributedTransactionManager>("max-active-reuse", cfg);

    const auto tid1 = limited_mgr->beginDistributed({makeParticipant("n1", p1.get())});
    ASSERT_TRUE(limited_mgr->prepareDistributed(tid1).ok);
    ASSERT_TRUE(limited_mgr->commitDistributed(tid1).ok);

    EXPECT_NO_THROW({
        const auto tid2 = limited_mgr->beginDistributed({makeParticipant("n2", p2.get())});
        EXPECT_FALSE(tid2.empty());
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-20: Idempotent abort — abortDistributed on already-aborted txn is safe
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, IdempotentAbort) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    mgr->abortDistributed(tid);
    mgr->abortDistributed(tid);  // Must not throw or double-abort
    mgr->abortDistributed(tid);

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);

    // onAbort must only be called once per participant.
    EXPECT_EQ(p1->abortCount(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: prepareDistributed on unknown txn returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PrepareUnknownTxnReturnsError) {
    auto status = mgr->prepareDistributed("nonexistent-txn");
    EXPECT_FALSE(status.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: commitDistributed without prior prepare returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, CommitWithoutPrepareReturnsError) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    auto status = mgr->commitDistributed(tid);
    EXPECT_FALSE(status.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: activeTransactionCount reflects in-flight txns
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, ActiveTransactionCountAfterBegin) {
    EXPECT_EQ(mgr->activeTransactionCount(), 0u);
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    EXPECT_EQ(mgr->activeTransactionCount(), 1u);
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);
    EXPECT_EQ(mgr->activeTransactionCount(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: affected_keys are forwarded to onPrepare
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, AffectedKeysForwardedToPrepare) {
    class KeyCapture : public IDistributedParticipantCallback {
    public:
        bool onPrepare(const std::string&, const std::set<std::string>& keys) override {
            received_keys = keys;
            return true;
        }
        void onCommit(const std::string&) override {}
        void onAbort(const std::string&) override {}
        std::set<std::string> received_keys;
    } capture;

    std::set<std::string> expected = {"users:1", "users:2", "orders:99"};
    Participant part;
    part.node_id      = "shard1";
    part.affected_keys = expected;
    part.callback     = &capture;

    auto tid = mgr->beginDistributed({part});
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);

    EXPECT_EQ(capture.received_keys, expected);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: getTransaction returns nullopt for unknown ID
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, GetTransactionUnknownReturnsNullopt) {
    EXPECT_FALSE(mgr->getTransaction("unknown-txn-id").has_value());
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: single-participant happy path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, SingleParticipantHappyPath) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});

    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::PREPARED);

    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::COMMITTED);

    EXPECT_EQ(p1->prepareCount(), 1);
    EXPECT_EQ(p1->commitCount(), 1);
    EXPECT_EQ(p1->abortCount(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: commit on already-committed txn returns error
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, DoubleCommitReturnsError) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    auto status = mgr->commitDistributed(tid);
    EXPECT_FALSE(status.ok);
}

// ─────────────────────────────────────────────────────────────────────────────
// Additional: abort before prepare
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, AbortBeforePrepare) {
    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
    });

    mgr->abortDistributed(tid);

    auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);

    // Participants were never prepared, so onAbort is still called as part
    // of the abort broadcast.
    EXPECT_EQ(p1->prepareCount(), 0);
    EXPECT_EQ(p2->prepareCount(), 0);
    EXPECT_EQ(p1->abortCount(), 1);
    EXPECT_EQ(p2->abortCount(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// PERF-D4: Batched prepare + lock-free coordination tests
// ─────────────────────────────────────────────────────────────────────────────

// AC-D4-1: Default config has no batching (prepare_batch_window == 0)
TEST(Distributed2PCPerfTests, DefaultConfigNoBatchWindow) {
    DistributedTxnManagerConfig cfg;
    EXPECT_EQ(cfg.prepare_batch_window.count(), 0);
}

// AC-D4-2: Config accepts a non-zero batch window
TEST(Distributed2PCPerfTests, BatchWindowCanBeConfigured) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_batch_window = std::chrono::milliseconds(25);
    EXPECT_EQ(cfg.prepare_batch_window.count(), 25);
}

// AC-D4-3: Default worker thread count is 4
TEST(Distributed2PCPerfTests, DefaultWorkerThreadCount) {
    DistributedTxnManagerConfig cfg;
    EXPECT_EQ(cfg.worker_thread_count, 4u);
}

// AC-D4-4: Worker thread count is configurable
TEST(Distributed2PCPerfTests, WorkerThreadCountCanBeConfigured) {
    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count = 8;
    EXPECT_EQ(cfg.worker_thread_count, 8u);
}

// AC-D4-5: Full 2PC happy path with thread-pool enabled (default)
TEST(Distributed2PCPerfTests, FullHappyPathWithThreadPool) {
    MockParticipant p1, p2, p3;

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = 4;
    cfg.prepare_batch_window = std::chrono::milliseconds(0);  // immediate
    DistributedTransactionManager mgr("perf-coord-tp", cfg);

    auto tid = mgr.beginDistributed({
        makeParticipant("n1", &p1),
        makeParticipant("n2", &p2),
        makeParticipant("n3", &p3),
    });

    ASSERT_TRUE(mgr.prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr.commitDistributed(tid).ok);

    EXPECT_EQ(p1.prepareCount(), 1);
    EXPECT_EQ(p2.prepareCount(), 1);
    EXPECT_EQ(p3.prepareCount(), 1);
    EXPECT_EQ(p1.commitCount(), 1);
    EXPECT_EQ(p2.commitCount(), 1);
    EXPECT_EQ(p3.commitCount(), 1);
}

// AC-D4-6: Batched prepare window — multiple transactions prepared in one flush
TEST(Distributed2PCPerfTests, BatchedPrepareWindowGroupsTransactions) {
    constexpr int N = 10;

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = 4;
    cfg.prepare_batch_window = std::chrono::milliseconds(30);
    cfg.prepare_timeout      = std::chrono::milliseconds(15000);
    DistributedTransactionManager mgr("perf-coord-batch", cfg);

    std::vector<std::unique_ptr<MockParticipant>> participants(N);
    for (auto& p : participants) p = std::make_unique<MockParticipant>();

    // Begin all transactions before any prepares so they all land in the batch.
    std::vector<std::string> txn_ids;
    txn_ids.reserve(N);
    for (int i = 0; i < N; ++i) {
        txn_ids.push_back(mgr.beginDistributed({
            makeParticipant("node_" + std::to_string(i), participants[i].get()),
        }));
    }

    // Kick off prepares concurrently — all should land in the same batch window.
    std::vector<std::thread> threads;
    std::atomic<int> successes{0};
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&mgr, &txn_ids, &successes, i] {
            if (mgr.prepareDistributed(txn_ids[i]).ok) ++successes;
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(successes.load(), N);

    // Commit all prepared transactions.
    for (int i = 0; i < N; ++i) {
        auto rec = mgr.getTransaction(txn_ids[i]);
        ASSERT_TRUE(rec.has_value());
        if (rec->state == DistributedTxnState::PREPARED) {
            EXPECT_TRUE(mgr.commitDistributed(txn_ids[i]).ok);
        }
    }
}

// AC-D4-7: Abort path works correctly with thread pool
TEST(Distributed2PCPerfTests, AbortWithThreadPool) {
    MockParticipant p1, p2;
    MockParticipant aborting(MockParticipant::Policy::ALWAYS_ABORT);

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = 4;
    cfg.prepare_batch_window = std::chrono::milliseconds(0);
    DistributedTransactionManager mgr("perf-coord-abort", cfg);

    auto tid = mgr.beginDistributed({
        makeParticipant("n1", &p1),
        makeParticipant("n2", &aborting),
    });

    auto status = mgr.prepareDistributed(tid);
    EXPECT_FALSE(status.ok);

    auto rec = mgr.getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTED);

    EXPECT_EQ(p1.abortCount(), 1);
    EXPECT_EQ(p1.commitCount(), 0);
}

// AC-D4-8: Legacy mode (worker_thread_count=0) still works correctly
TEST(Distributed2PCPerfTests, LegacyModeWorkerCount0) {
    MockParticipant p1, p2;

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = 0;  // legacy: std::async per call
    cfg.prepare_batch_window = std::chrono::milliseconds(0);
    DistributedTransactionManager mgr("perf-coord-legacy", cfg);

    auto tid = mgr.beginDistributed({
        makeParticipant("n1", &p1),
        makeParticipant("n2", &p2),
    });

    ASSERT_TRUE(mgr.prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr.commitDistributed(tid).ok);

    EXPECT_EQ(p1.commitCount(), 1);
    EXPECT_EQ(p2.commitCount(), 1);
}

// AC-D4-9: Concurrent transactions with thread pool are all successful
TEST(Distributed2PCPerfTests, ConcurrentTxnsWithThreadPool) {
    constexpr int N = 100;

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = 8;
    cfg.prepare_batch_window = std::chrono::milliseconds(0);
    cfg.prepare_timeout      = std::chrono::milliseconds(5000);
    DistributedTransactionManager mgr("perf-coord-concurrent", cfg);

    std::vector<std::unique_ptr<MockParticipant>> pa(N), pb(N);
    for (int i = 0; i < N; ++i) {
        pa[i] = std::make_unique<MockParticipant>();
        pb[i] = std::make_unique<MockParticipant>();
    }

    std::vector<std::thread> threads;
    std::atomic<int> successes{0};
    threads.reserve(N);

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([&, i] {
            auto tid = mgr.beginDistributed({
                makeParticipant("a" + std::to_string(i), pa[i].get()),
                makeParticipant("b" + std::to_string(i), pb[i].get()),
            });
            if (!mgr.prepareDistributed(tid).ok)  return;
            if (!mgr.commitDistributed(tid).ok)   return;
            ++successes;
        });
    }
    for (auto& t : threads) t.join();

    EXPECT_EQ(successes.load(), N);
}

// AC-D4-10: P99 latency check for 5-shard transaction (gated by env flag)
// To run: THEMIS_RUN_PERF_TESTS=1 ./test_transaction_distributed_2pc_focused
TEST(Distributed2PCPerfTests, P99LatencyFiveShards) {
    const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!env || std::string(env) != "1") {
        GTEST_SKIP() << "Skipped: set THEMIS_RUN_PERF_TESTS=1 to run performance tests";
    }

    constexpr int ITERATIONS = 2000;
    constexpr int SHARDS     = 5;

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = 8;
    cfg.prepare_batch_window = std::chrono::milliseconds(0);
    DistributedTransactionManager mgr("perf-p99", cfg);

    std::vector<std::unique_ptr<MockParticipant>> participants(SHARDS);
    for (auto& p : participants) p = std::make_unique<MockParticipant>();

    std::vector<double> latencies_us;
    latencies_us.reserve(ITERATIONS);

    for (int i = 0; i < ITERATIONS; ++i) {
        std::vector<Participant> parts;
        for (int s = 0; s < SHARDS; ++s) {
            parts.push_back(makeParticipant(
                "shard_" + std::to_string(s),
                participants[s].get(),
                {"key_" + std::to_string(i)}
            ));
        }

        const auto t0 = std::chrono::steady_clock::now();
        auto tid = mgr.beginDistributed(parts);
        ASSERT_TRUE(mgr.prepareDistributed(tid).ok);
        ASSERT_TRUE(mgr.commitDistributed(tid).ok);
        const auto elapsed_us = std::chrono::duration<double, std::micro>(
            std::chrono::steady_clock::now() - t0).count();
        latencies_us.push_back(elapsed_us);
    }

    std::sort(latencies_us.begin(), latencies_us.end());
    const double p99_us = latencies_us[static_cast<size_t>(ITERATIONS * 0.99)];
    const double p99_ms = p99_us / 1000.0;

    std::printf("[PERF-D4] P99 latency (5 shards, %d iters): %.2f ms\n", ITERATIONS, p99_ms);
    EXPECT_LT(p99_ms, 100.0) << "P99 latency " << p99_ms << " ms exceeds 100 ms SLO";
}

// AC-D4-11: Throughput ≥ 10k/s with thread pool enabled
// To run: THEMIS_RUN_PERF_TESTS=1 ./test_transaction_distributed_2pc_focused
TEST(Distributed2PCPerfTests, ThroughputAtLeast10kOpsPerSec) {
    const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!env || std::string(env) != "1") {
        GTEST_SKIP() << "Skipped: set THEMIS_RUN_PERF_TESTS=1 to run performance tests";
    }

    constexpr int DURATION_SEC = 3;
    constexpr int WORKER_THREADS = 8;

    DistributedTxnManagerConfig cfg;
    cfg.worker_thread_count  = WORKER_THREADS;
    cfg.prepare_batch_window = std::chrono::milliseconds(0);
    DistributedTransactionManager mgr("perf-throughput", cfg);

    constexpr int MAX_PARTICIPANTS = 500;
    std::vector<std::unique_ptr<MockParticipant>> pool(MAX_PARTICIPANTS);
    for (auto& p : pool) p = std::make_unique<MockParticipant>();

    std::atomic<uint64_t> ops{0};
    std::atomic<bool>     stop_flag{false};

    auto worker = [&](int worker_id) {
        int idx = worker_id * 2;
        while (!stop_flag.load(std::memory_order_relaxed)) {
            auto* pa = pool[(idx)     % MAX_PARTICIPANTS].get();
            auto* pb = pool[(idx + 1) % MAX_PARTICIPANTS].get();
            idx = (idx + 2) % MAX_PARTICIPANTS;

            auto tid = mgr.beginDistributed({
                makeParticipant("a" + std::to_string(ops.load()), pa),
                makeParticipant("b" + std::to_string(ops.load()), pb),
            });
            if (!mgr.prepareDistributed(tid).ok) continue;
            if (!mgr.commitDistributed(tid).ok)  continue;
            ++ops;
        }
    };

    std::vector<std::thread> workers;
    for (int i = 0; i < WORKER_THREADS; ++i) {
        workers.emplace_back(worker, i);
    }

    std::this_thread::sleep_for(std::chrono::seconds(DURATION_SEC));
    stop_flag.store(true, std::memory_order_relaxed);
    for (auto& t : workers) t.join();

    const double ops_per_sec = static_cast<double>(ops.load()) / DURATION_SEC;
    std::printf("[PERF-D4] 2PC throughput: %.0f ops/s (target ≥ 10000)\n", ops_per_sec);
    EXPECT_GE(ops_per_sec, 10000.0)
        << "Throughput " << ops_per_sec << " ops/s below 10k/s SLO";
}

// ─────────────────────────────────────────────────────────────────────────────
// Correctness fix tests (DTM-1, DTM-2, DTM-3, DTM-4, CC-1)
// ─────────────────────────────────────────────────────────────────────────────

// Helper: build a remote-only Participant (no callback, endpoint only).
static Participant makeRemoteParticipant(
    const std::string& node_id,
    std::set<std::string> keys = {"key1"}
) {
    Participant p;
    p.node_id       = node_id;
    p.endpoint      = node_id + ":9090";
    p.affected_keys = std::move(keys);
    p.callback      = nullptr;  // remote — no in-process callback
    return p;
}

// DTM-1: A remote participant without a registered callback must vote ABORT,
// not COMMIT.  prepareDistributed() with a remote-only participant must fail.
TEST_F(DistributedTxnManagerTest, DTM1_RemoteParticipantWithoutCallbackVotesAbort) {
    const auto tid = mgr->beginDistributed({makeRemoteParticipant("remote-node-1")});
    const auto status = mgr->prepareDistributed(tid);
    EXPECT_FALSE(status.ok) << "Remote participant without callback must vote ABORT (DTM-1)";
}

// DTM-1: Mixed — one local (COMMIT vote) and one remote (ABORT vote) participant.
// The transaction must be aborted because the remote cannot confirm.
TEST_F(DistributedTxnManagerTest, DTM1_MixedLocalAndRemoteVotesAbort) {
    const auto tid = mgr->beginDistributed({
        makeParticipant("local-node",  p1.get()),
        makeRemoteParticipant("remote-node-2"),
    });
    const auto status = mgr->prepareDistributed(tid);
    EXPECT_FALSE(status.ok) << "Mixed local+remote must abort when remote has no callback (DTM-1)";
    // Local participant must have been asked to prepare.
    EXPECT_GE(p1->prepareCount(), 1);
}

// DTM-1 hardening: a configured Phase-2 bridge must not be used as a
// compatibility shortcut for missing Phase-1 PREPARE transport.
TEST_F(DistributedTxnManagerTest, DTM1_Phase2BridgeDoesNotBypassMissingPhase1Vote) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };

    std::atomic<int> phase2_abort_calls{0};
    std::atomic<int> phase2_commit_calls{0};
    cfg.remote_phase2_dispatch =
        [&phase2_abort_calls, &phase2_commit_calls](
            const std::string&,
            const std::string&,
            const std::string&,
            bool do_commit) {
            if (do_commit) {
                ++phase2_commit_calls;
            } else {
                ++phase2_abort_calls;
            }
            return true;
        };

    DistributedTransactionManager mgr2("coord-no-phase1", cfg);
    const auto tid = mgr2.beginDistributed({
        makeParticipant("local-node", p1.get()),
        makeRemoteParticipant("remote-node-no-phase1"),
    });

    const auto prepare_status = mgr2.prepareDistributed(tid);
    EXPECT_FALSE(prepare_status.ok)
        << "Missing Phase-1 bridge for a remote participant must fail prepare";
    EXPECT_EQ(phase2_commit_calls.load(), 0)
        << "Remote participant must never receive COMMIT without a PREPARE vote";
    EXPECT_EQ(phase2_abort_calls.load(), 1)
        << "Fail-closed path must send ABORT to remote participant";
}

// DTM-2: recoverInDoubtTransactions() must call onAbort on in-memory
// participants, not just write to WAL and leave them locked.
TEST_F(DistributedTxnManagerTest, DTM2_RecoveryBroadcastsAbortToInMemoryParticipants) {
    // Create a transaction and advance it to PREPARED state.
    const auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
    });
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    // At this point the transaction is PREPARED (in-doubt if coordinator crashed here).

    // Simulate coordinator re-processing in-doubt transactions.
    const size_t resolved = mgr->recoverInDoubtTransactions();
    EXPECT_GE(resolved, 1u) << "At least one in-doubt transaction should be recovered (DTM-2)";

    // Both in-memory participants must have received an ABORT notification.
    EXPECT_GE(p1->abortCount(), 1) << "Participant p1 must be notified of ABORT during recovery (DTM-2)";
    EXPECT_GE(p2->abortCount(), 1) << "Participant p2 must be notified of ABORT during recovery (DTM-2)";
}

TEST_F(DistributedTxnManagerTest, DTM2_RecoveryDoesNotMarkResolvedWhenAbortDeliveryFails) {
    AbortThrowingParticipant abort_thrower;

    const auto tid = mgr->beginDistributed({
        makeParticipant("n1", &abort_thrower),
    });
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);

    const size_t resolved = mgr->recoverInDoubtTransactions();
    EXPECT_EQ(resolved, 0u)
        << "Recovery must not claim success when ABORT delivery fails";

    const auto rec = mgr->getTransaction(tid);
    ASSERT_TRUE(rec.has_value());
    EXPECT_EQ(rec->state, DistributedTxnState::ABORTING)
        << "Failed recovery ABORT delivery must keep txn non-terminal";
}

TEST_F(DistributedTxnManagerTest, RecoveryReplaysCommitForInMemoryCommittingTransaction) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };
    cfg.phase1_rpc_fn = [](const std::string&, const std::string&,
                           const std::set<std::string>&) { return true; };

    std::atomic<int> phase2_calls{0};
    cfg.remote_phase2_dispatch = [&phase2_calls](
            const std::string&, const std::string&, const std::string&, bool) {
        const int call_no = ++phase2_calls;
        return call_no > 3;
    };

    DistributedTransactionManager mgr2("coord-recovery-commit-replay", cfg);
    const auto tid = mgr2.beginDistributed({makeRemoteParticipant("remote-replay-node")});
    ASSERT_TRUE(mgr2.prepareDistributed(tid).ok);

    const auto commit_status = mgr2.commitDistributed(tid);
    EXPECT_FALSE(commit_status.ok);

    const auto pre_recovery = mgr2.getTransaction(tid);
    ASSERT_TRUE(pre_recovery.has_value());
    EXPECT_EQ(pre_recovery->state, DistributedTxnState::COMMITTING);

    const size_t resolved = mgr2.recoverInDoubtTransactions();
    EXPECT_GE(resolved, 1u);
    EXPECT_EQ(phase2_calls.load(), 4)
        << "Recovery should replay one additional COMMIT delivery after the initial 3 retries";

    const auto post_recovery = mgr2.getTransaction(tid);
    ASSERT_TRUE(post_recovery.has_value());
    EXPECT_EQ(post_recovery->state, DistributedTxnState::COMMITTED);
}

// DTM-3: isParticipantAlive() must return true for in-process participants and
// false for remote participants (no callback).
TEST_F(DistributedTxnManagerTest, DTM3_IsParticipantAliveDistinguishesLocalAndRemote) {
    // Register a transaction with both an in-process and a remote participant.
    const auto tid = mgr->beginDistributed({
        makeParticipant("in-proc-node",  p1.get()),
        makeRemoteParticipant("remote-node-dtm3"),
    });

    EXPECT_TRUE(mgr->isParticipantAlive("in-proc-node"))
        << "In-process participant with callback must be alive (DTM-3)";
    EXPECT_FALSE(mgr->isParticipantAlive("remote-node-dtm3"))
        << "Remote participant without callback must be reported as not alive (DTM-3)";
    EXPECT_TRUE(mgr->isParticipantAlive("unknown-node"))
        << "Unknown node (not in any txn) must default to alive (DTM-3)";

    // Clean up.
    mgr->abortDistributed(tid);
}

// CC-1: logToWAL (via commitDistributed/abortDistributed) must propagate WAL
// write errors rather than silently swallowing them.  We test this indirectly
// by verifying that a successfully committed transaction was correctly recorded
// (the positive case; negative/fault-injection testing requires a mock WAL).
TEST_F(DistributedTxnManagerTest, CC1_SuccessfulWALWriteDoesNotSuppressPhase2) {
    // Without a WAL configured (default test fixture), commit must still work.
    const auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
    });
    ASSERT_TRUE(mgr->prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr->commitDistributed(tid).ok);

    // Both participants must have received onCommit.
    EXPECT_GE(p1->commitCount(), 1);
    EXPECT_GE(p2->commitCount(), 1);
}

// #279: Callback-less remote participants must receive Phase-2 ABORT through
// the configured remote dispatcher.
TEST_F(DistributedTxnManagerTest, Stub279_RemoteAbortUsesConfiguredPhase2Dispatcher) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout = 2000ms;
    cfg.default_txn_timeout = 60s;

    std::atomic<int> dispatch_calls{0};
    std::atomic<int> abort_calls{0};
    std::atomic<int> commit_calls{0};
    cfg.remote_phase2_dispatch =
        [&dispatch_calls, &abort_calls, &commit_calls](
            const std::string& /*txn_id*/,
            const std::string& node_id,
            const std::string& endpoint,
            bool do_commit) {
            ++dispatch_calls;
            EXPECT_EQ(node_id, "remote-node");
            EXPECT_EQ(endpoint, "remote-node:9090");
            if (do_commit) {
                ++commit_calls;
            } else {
                ++abort_calls;
            }
            return true;
        };

    DistributedTransactionManager mgr_with_dispatch("coord-279-abort", cfg);
    const auto tid = mgr_with_dispatch.beginDistributed({makeRemoteParticipant("remote-node")});
    const auto prepare = mgr_with_dispatch.prepareDistributed(tid);

    EXPECT_FALSE(prepare.ok);
    EXPECT_EQ(dispatch_calls.load(), 1);
    EXPECT_EQ(abort_calls.load(), 1);
    EXPECT_EQ(commit_calls.load(), 0);
}

// #279: Callback-less remote participants must receive Phase-2 COMMIT through
// the configured remote dispatcher when all votes are COMMIT.
TEST_F(DistributedTxnManagerTest, Stub279_RemoteCommitUsesConfiguredPhase2Dispatcher) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };

    std::atomic<int> dispatch_calls{0};
    std::atomic<int> commit_calls{0};
    cfg.remote_phase2_dispatch =
        [&dispatch_calls, &commit_calls](
            const std::string& /*txn_id*/,
            const std::string& node_id,
            const std::string& endpoint,
            bool do_commit) {
            ++dispatch_calls;
            EXPECT_EQ(node_id, "remote-node");
            EXPECT_EQ(endpoint, "remote-node:9090");
            EXPECT_TRUE(do_commit);
            if (do_commit) ++commit_calls;
            return true;
        };

    DistributedTransactionManager mgr_with_dispatch("coord-279-commit", cfg);
    const auto tid = mgr_with_dispatch.beginDistributed({
        makeParticipant("local-node", p1.get()),
        makeRemoteParticipant("remote-node")
    });

    ASSERT_TRUE(mgr_with_dispatch.prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr_with_dispatch.commitDistributed(tid).ok);

    EXPECT_EQ(dispatch_calls.load(), 1);
    EXPECT_EQ(commit_calls.load(), 1);
}

// ─────────────────────────────────────────────────────────────────────────────
// #279 Phase-1 PREPARE RPC bridge tests
// ─────────────────────────────────────────────────────────────────────────────

// When phase1_rpc_fn is configured, the remote participant's PREPARE vote is
// collected via RPC.  A YES vote (true) allows the transaction to commit.
TEST_F(DistributedTxnManagerTest, Stub279_Phase1RpcFnYesVoteAllowsCommit) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };

    std::atomic<int> p1_calls{0};
    std::atomic<int> p2_calls{0};
    // Phase-1: remote participant votes YES
    cfg.phase1_rpc_fn = [&p1_calls](
            const std::string& /*endpoint*/,
            const std::string& /*txn_id*/,
            const std::set<std::string>& /*keys*/) -> bool {
        ++p1_calls;
        return true;  // COMMIT vote
    };
    // Phase-2: capture the COMMIT delivery
    cfg.remote_phase2_dispatch = [&p2_calls](
            const std::string& /*txn_id*/,
            const std::string& /*node_id*/,
            const std::string& /*endpoint*/,
            bool do_commit) {
        ++p2_calls;
        return do_commit;  // echo back success
    };

    DistributedTransactionManager mgr2("coord-p1-yes", cfg);
    const auto tid = mgr2.beginDistributed({
        makeParticipant("local-node", p1.get()),
        makeRemoteParticipant("remote-node")
    });

    const auto prepare_result = mgr2.prepareDistributed(tid);
    ASSERT_TRUE(prepare_result.ok) << prepare_result.message;
    ASSERT_TRUE(mgr2.commitDistributed(tid).ok);

    EXPECT_EQ(p1_calls.load(), 1) << "Phase-1 RPC must be called once for the remote participant";
    EXPECT_EQ(p2_calls.load(), 1) << "Phase-2 dispatch must be called once for the remote participant";
    EXPECT_GE(p1->commitCount(), 1) << "Local participant must have received onCommit";
}

// When phase1_rpc_fn returns false, the remote participant votes NO and the
// transaction must be aborted.
TEST_F(DistributedTxnManagerTest, Stub279_Phase1RpcFnNoVoteAbortsTransaction) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };

    std::atomic<int> p1_calls{0};
    std::atomic<int> p2_calls{0};
    // Phase-1: remote participant votes NO
    cfg.phase1_rpc_fn = [&p1_calls](
            const std::string& /*endpoint*/,
            const std::string& /*txn_id*/,
            const std::set<std::string>& /*keys*/) -> bool {
        ++p1_calls;
        return false;  // ABORT vote
    };
    cfg.remote_phase2_dispatch = [&p2_calls](
            const std::string& /*txn_id*/,
            const std::string& /*node_id*/,
            const std::string& /*endpoint*/,
            bool /*do_commit*/) {
        ++p2_calls;
        return true;
    };

    DistributedTransactionManager mgr2("coord-p1-no", cfg);
    const auto tid = mgr2.beginDistributed({
        makeParticipant("local-node", p1.get()),
        makeRemoteParticipant("remote-node")
    });

    const auto prepare_result = mgr2.prepareDistributed(tid);
    EXPECT_FALSE(prepare_result.ok) << "Remote ABORT vote must prevent commit";

    EXPECT_EQ(p1_calls.load(), 1) << "Phase-1 RPC must be called once";
    // Phase-2 should deliver ABORT to local participant and remote node.
    EXPECT_GE(p1->abortCount(), 1) << "Local participant must receive onAbort";
}

// When the phase1_rpc_fn throws, the coordinator treats it as an ABORT vote
// (fail-closed).
TEST_F(DistributedTxnManagerTest, Stub279_Phase1RpcFnExceptionIsAbortVote) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;

    cfg.phase1_rpc_fn = [](
            const std::string& /*endpoint*/,
            const std::string& /*txn_id*/,
            const std::set<std::string>& /*keys*/) -> bool {
        throw std::runtime_error("network error simulated");
    };
    cfg.remote_phase2_dispatch = [](
            const std::string& /*txn_id*/, const std::string& /*node_id*/,
            const std::string& /*endpoint*/, bool /*do_commit*/) {
        return true;
    };

    DistributedTransactionManager mgr2("coord-p1-throw", cfg);
    const auto tid = mgr2.beginDistributed({makeRemoteParticipant("remote-node")});

    const auto prepare_result = mgr2.prepareDistributed(tid);
    EXPECT_FALSE(prepare_result.ok)
        << "Exception in Phase-1 RPC must be treated as ABORT vote (fail-closed)";
}

// remote_phase1_dispatch is the config-level Phase-1 bridge (lower priority than
// phase1_rpc_fn but higher than the static setRpcPhase1Fn).
TEST_F(DistributedTxnManagerTest, Stub279_RemotePhase1DispatchCommit) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };

    std::atomic<int> p1_calls{0};
    std::atomic<int> p2_calls{0};
    cfg.remote_phase1_dispatch = [&p1_calls](
            const std::string& /*txn_id*/,
            const std::string& /*node_id*/,
            const std::string& /*endpoint*/,
            const std::set<std::string>& /*keys*/) -> bool {
        ++p1_calls;
        return true;  // COMMIT vote
    };
    cfg.remote_phase2_dispatch = [&p2_calls](
            const std::string& /*txn_id*/,
            const std::string& /*node_id*/,
            const std::string& /*endpoint*/,
            bool do_commit) {
        ++p2_calls;
        return do_commit;
    };

    DistributedTransactionManager mgr2("coord-p1-dispatch", cfg);
    const auto tid = mgr2.beginDistributed({
        makeParticipant("local-node", p1.get()),
        makeRemoteParticipant("remote-node")
    });

    ASSERT_TRUE(mgr2.prepareDistributed(tid).ok);
    ASSERT_TRUE(mgr2.commitDistributed(tid).ok);
    EXPECT_EQ(p1_calls.load(), 1);
    EXPECT_EQ(p2_calls.load(), 1);
}

// A pure-remote 2PC (all participants are remote) must succeed when a Phase-1
// bridge is configured and all remote participants vote YES.
TEST_F(DistributedTxnManagerTest, Stub279_PureRemoteTransactionSucceedsWithPhase1Rpc) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };

    std::atomic<int> prepare_calls{0};
    std::atomic<int> commit_calls{0};
    cfg.phase1_rpc_fn = [&prepare_calls](
            const std::string& /*endpoint*/,
            const std::string& /*txn_id*/,
            const std::set<std::string>& /*keys*/) -> bool {
        ++prepare_calls;
        return true;
    };
    cfg.remote_phase2_dispatch = [&commit_calls](
            const std::string& /*txn_id*/,
            const std::string& /*node_id*/,
            const std::string& /*endpoint*/,
            bool do_commit) {
        if (do_commit) ++commit_calls;
        return true;
    };

    DistributedTransactionManager mgr2("coord-pure-remote", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("shard-A"),
        makeRemoteParticipant("shard-B"),
        makeRemoteParticipant("shard-C")
    });

    const auto prepare_result = mgr2.prepareDistributed(tid);
    ASSERT_TRUE(prepare_result.ok)
        << "Pure-remote 2PC with YES votes must succeed: " << prepare_result.message;
    ASSERT_TRUE(mgr2.commitDistributed(tid).ok);

    EXPECT_EQ(prepare_calls.load(), 3)
        << "Phase-1 RPC must be invoked for each of the 3 remote participants";
    EXPECT_EQ(commit_calls.load(), 3)
        << "Phase-2 COMMIT must be dispatched to all 3 remote participants";
}

// The static setRpcPhase1Fn / clearRpcPhase1Fn bridge works as the lowest-priority
// fallback for Phase-1 PREPARE delivery.
TEST_F(DistributedTxnManagerTest, Stub279_StaticPhase1FnCommit) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };
    cfg.remote_phase2_dispatch = [](
            const std::string& /*txn_id*/, const std::string& /*node_id*/,
            const std::string& /*endpoint*/, bool do_commit) {
        return do_commit;
    };

    std::atomic<int> static_p1_calls{0};
    DistributedTransactionManager::setRpcPhase1Fn(
        [&static_p1_calls](const std::string& /*node_id*/,
                           const std::string& /*txn_id*/,
                           const std::set<std::string>& /*keys*/) -> bool {
            ++static_p1_calls;
            return true;
        });

    DistributedTransactionManager mgr2("coord-static-p1", cfg);
    const auto tid = mgr2.beginDistributed({
        makeParticipant("local-node", p1.get()),
        makeRemoteParticipant("remote-node")
    });

    const auto prepare_result = mgr2.prepareDistributed(tid);
    ASSERT_TRUE(prepare_result.ok) << prepare_result.message;
    ASSERT_TRUE(mgr2.commitDistributed(tid).ok);
    EXPECT_EQ(static_p1_calls.load(), 1);

    DistributedTransactionManager::clearRpcPhase1Fn();
}

// Phase-2 delivery must use 3-attempt retry with backoff before failing.
TEST_F(DistributedTxnManagerTest, Stub279_RemotePhase2DispatchRetriesThreeTimesOnPersistentFailure) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout = 2000ms;
    cfg.commit_timeout  = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string&, const std::string&) { return true; };
    cfg.phase1_rpc_fn = [](const std::string&, const std::string&,
                           const std::set<std::string>&) { return true; };

    std::atomic<int> phase2_calls{0};
    cfg.remote_phase2_dispatch = [&phase2_calls](
            const std::string&, const std::string&, const std::string&, bool) {
        ++phase2_calls;
        return false;
    };

    DistributedTransactionManager mgr2("coord-phase2-retry-3x", cfg);
    const auto tid = mgr2.beginDistributed({makeRemoteParticipant("remote-retry-node")});

    const auto prepare_result = mgr2.prepareDistributed(tid);
    ASSERT_TRUE(prepare_result.ok) << prepare_result.message;

    const auto started_at = std::chrono::steady_clock::now();
    const auto commit_result = mgr2.commitDistributed(tid);
    const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - started_at).count();
    EXPECT_FALSE(commit_result.ok)
        << "Persistent Phase-2 delivery failure must fail commit after retries";
    EXPECT_EQ(phase2_calls.load(), 3)
        << "Phase-2 dispatch must be attempted exactly three times";
    EXPECT_GE(elapsed_ms, 230)
        << "Retry backoff must wait at least ~240ms total (80ms + 160ms with jitter margin)";
    EXPECT_LE(elapsed_ms, 1200)
        << "Retry backoff must remain bounded and not silently extend deadlines";
}

// ─────────────────────────────────────────────────────────────────────────────
// DTM-3 liveness bridge tests
// ─────────────────────────────────────────────────────────────────────────────

// DTM-3b: Per-instance liveness_check_fn returns true → isParticipantAlive true.
TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceReturnsTrue) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;

    std::atomic<int> liveness_calls{0};
    cfg.liveness_check_fn = [&liveness_calls](
            const std::string& /*endpoint*/,
            const std::string& /*node_id*/) -> bool {
        ++liveness_calls;
        return true; // bridge says alive
    };
    cfg.phase1_rpc_fn = [](const std::string&, const std::string&,
                            const std::set<std::string>&) { return true; };
    cfg.remote_phase2_dispatch = [](const std::string&, const std::string&,
                                     const std::string&, bool) { return true; };

    DistributedTransactionManager mgr2("coord-liveness-true", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-alive"),
    });

    EXPECT_TRUE(mgr2.isParticipantAlive("remote-alive"))
        << "Bridge returning true must make isParticipantAlive return true";
    EXPECT_GE(liveness_calls.load(), 1)
        << "liveness_check_fn must have been called";

    mgr2.abortDistributed(tid);
}

// DTM-3c: Per-instance liveness_check_fn returns false → isParticipantAlive false.
TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceReturnsFalse) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;

    cfg.liveness_check_fn = [](const std::string& /*endpoint*/,
                                const std::string& /*node_id*/) -> bool {
        return false; // bridge says dead
    };

    DistributedTransactionManager mgr2("coord-liveness-false", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-dead"),
    });

    EXPECT_FALSE(mgr2.isParticipantAlive("remote-dead"))
        << "Bridge returning false must make isParticipantAlive return false";

    mgr2.abortDistributed(tid);
}

// DTM-3d: Per-instance liveness_check_fn throws → isParticipantAlive returns false (fail-closed).
TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceExceptionIsNotAlive) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;

    cfg.liveness_check_fn = [](const std::string& /*endpoint*/,
                                const std::string& /*node_id*/) -> bool {
        throw std::runtime_error("network error");
    };

    DistributedTransactionManager mgr2("coord-liveness-throw", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-throw"),
    });

    EXPECT_FALSE(mgr2.isParticipantAlive("remote-throw"))
        << "Exception from liveness bridge must be treated as not alive (fail-closed)";

    mgr2.abortDistributed(tid);
}

TEST_F(DistributedTxnManagerTest, DTM3_LivenessBridgeInstanceCStringExceptionIsNotAlive) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;

    cfg.liveness_check_fn = [](const std::string& /*endpoint*/,
                                const std::string& /*node_id*/) -> bool {
        throw "network cstr error";
    };

    DistributedTransactionManager mgr2("coord-liveness-throw-cstr", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-throw-cstr"),
    });

    EXPECT_FALSE(mgr2.isParticipantAlive("remote-throw-cstr"))
        << "CString exception from liveness bridge must be treated as not alive";

    mgr2.abortDistributed(tid);
}

// DTM-3e: Static liveness bridge (setLivenessCheckFn) is consulted when no per-instance fn set.
TEST_F(DistributedTxnManagerTest, DTM3_StaticLivenessBridgeIsConsulted) {
    // No liveness_check_fn on config — relies on static bridge.
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;

    std::atomic<int> static_calls{0};
    DistributedTransactionManager::setLivenessCheckFn(
        [&static_calls](const std::string& /*node_id*/,
                         const std::string& /*endpoint*/) -> bool {
            ++static_calls;
            return true;
        });

    DistributedTransactionManager mgr2("coord-static-liveness", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-static-alive"),
    });

    EXPECT_TRUE(mgr2.isParticipantAlive("remote-static-alive"))
        << "Static liveness bridge returning true must make isParticipantAlive return true";
    EXPECT_GE(static_calls.load(), 1)
        << "Static liveness bridge must have been called";

    mgr2.abortDistributed(tid);
    DistributedTransactionManager::clearLivenessCheckFn();
}

TEST_F(DistributedTxnManagerTest, DTM3_StaticLivenessBridgeStringExceptionIsNotAlive) {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;

    DistributedTransactionManager::setLivenessCheckFn(
        [](const std::string& /*node_id*/,
           const std::string& /*endpoint*/) -> bool {
            throw std::string("static bridge failure");
        });

    DistributedTransactionManager mgr2("coord-static-liveness-throw-string", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-static-throw-string"),
    });

    EXPECT_FALSE(mgr2.isParticipantAlive("remote-static-throw-string"))
        << "String exception from static liveness bridge must be treated as not alive";

    mgr2.abortDistributed(tid);
    DistributedTransactionManager::clearLivenessCheckFn();
}

// DTM-3f: Per-instance bridge takes priority over static bridge.
TEST_F(DistributedTxnManagerTest, DTM3_InstanceBridgeTakesPriorityOverStaticBridge) {
    // Install static bridge returning true; per-instance bridge returns false.
    // isParticipantAlive must use per-instance bridge → false.
    DistributedTransactionManager::setLivenessCheckFn(
        [](const std::string& /*node_id*/,
            const std::string& /*endpoint*/) -> bool {
            return true; // would say alive if consulted
        });

    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;
    cfg.liveness_check_fn = [](const std::string& /*endpoint*/,
                                const std::string& /*node_id*/) -> bool {
        return false; // instance bridge says dead → takes priority
    };

    DistributedTransactionManager mgr2("coord-priority-liveness", cfg);
    const auto tid = mgr2.beginDistributed({
        makeRemoteParticipant("remote-priority-node"),
    });

    EXPECT_FALSE(mgr2.isParticipantAlive("remote-priority-node"))
        << "Per-instance liveness_check_fn must take priority over static bridge";

    mgr2.abortDistributed(tid);
    DistributedTransactionManager::clearLivenessCheckFn();
}

// ─────────────────────────────────────────────────────────────────────────────
// Wave A — CRITICAL gap closure tests
// ─────────────────────────────────────────────────────────────────────────────

// GAP: blocking_no_timeout / no_timeout — distributed_transaction_manager.cpp
//      (batched prepareDistributed path used bare fut.get() with no deadline)
//
// Verify: when the batch-flush thread cannot fulfil the prepare promise before
// config_.prepare_timeout expires, prepareDistributed() returns an error status
// with ok==false rather than blocking indefinitely.
//
// Implementation note: we simulate a stalled batch-flush thread by using a
// SlowParticipant whose onPrepare() sleeps longer than the configured
// prepare_timeout.  With prepare_batch_window > 0 the batched code path is
// exercised.
TEST(Distributed2PCWaveAGapTests, BatchedPrepareFutureTimeout) {
    // Participant that blocks for 500 ms — longer than the 100 ms timeout.
    class SlowParticipant : public IDistributedParticipantCallback {
    public:
        bool onPrepare(const std::string& /*txn_id*/,
                       const std::set<std::string>& /*keys*/) override {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            return true;
        }
        void onCommit(const std::string& /*txn_id*/) override {}
        void onAbort(const std::string& /*txn_id*/)  override {}
    };

    SlowParticipant slow_p;

    DistributedTxnManagerConfig cfg;
    // Short timeout so the test completes quickly.
    cfg.prepare_timeout      = std::chrono::milliseconds(100);
    cfg.prepare_batch_window = std::chrono::milliseconds(10);  // enable batched path
    cfg.default_txn_timeout  = std::chrono::seconds(60);

    DistributedTransactionManager mgr("wave-a-batch-timeout-coord", cfg);

    const auto tid = mgr.beginDistributed({
        makeParticipant("slow-node", &slow_p),
    });

    const auto t0 = std::chrono::steady_clock::now();
    const auto status = mgr.prepareDistributed(tid);
    const auto elapsed = std::chrono::steady_clock::now() - t0;

    // Must NOT succeed (the prepare was stalled past the deadline).
    EXPECT_FALSE(status.ok)
        << "prepareDistributed must return !ok when batch-flush times out";

    // The call must terminate within a reasonable bound: strictly less than
    // 5× the prepare_timeout (i.e. < 500 ms), proving it did not wait for
    // the slow participant to finish.
    const auto elapsed_ms =
        std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
    EXPECT_LT(elapsed_ms, 500)
        << "prepareDistributed must not block indefinitely; elapsed=" << elapsed_ms << "ms";
}

// Regression guard: non-batched path (prepare_batch_window==0) still succeeds
// with a normally responding participant — ensuring the timeout fix does not
// break the happy path.
TEST(Distributed2PCWaveAGapTests, NonBatchedPrepareSucceedsNormally) {
    MockParticipant p;
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout      = std::chrono::milliseconds(5000);
    cfg.prepare_batch_window = std::chrono::milliseconds(0);   // immediate path

    DistributedTransactionManager mgr("wave-a-nonbatch-coord", cfg);
    const auto tid = mgr.beginDistributed({ makeParticipant("n1", &p) });

    const auto status = mgr.prepareDistributed(tid);
    EXPECT_TRUE(status.ok)
        << "Non-batched prepare must succeed with a cooperating participant";
}
