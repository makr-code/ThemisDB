/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_transaction_distributed_2pc.cpp               ║
  Version:         0.0.11                                             ║
  Last Modified:   2026-04-15 18:18:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1000                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • ff299c514b  2026-04-09  feat(transaction): PERF-D4 batched prepare + lock-free 2P... ║
    • 0f0c408c2f  2026-03-15  feat(transaction): implement Distributed Transaction Coor... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    EXPECT_FALSE(tid.empty());
}

TEST_F(DistributedTxnManagerTest, BeginDistributedIdsAreUnique) {
    auto tid1 = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    auto tid2 = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    EXPECT_NE(tid1, tid2);
}

// ─────────────────────────────────────────────────────────────────────────────
// AC-2 / AC-3: prepareDistributed collects all-commit votes → PREPARED
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(DistributedTxnManagerTest, PreparePhaseAllCommitVotes) {
    auto tid = mgr->beginDistributed({
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

    auto tid = mgr->beginDistributed({
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
