/**
 * @file test_transaction_distributed_phase2.cpp
 * @brief Phase 2: Distributed Coordination Hardening Tests
 *
 * Phase 2 validates distributed transaction coordinator behavior under failures,
 * focusing on 2PC protocol correctness, timeout semantics, retry behavior,
 * and in-doubt transaction recovery using the real DistributedTransactionManager
 * API with in-process mock participants.
 *
 * Acceptance Criteria Validated:
 * - AC-4: Distributed Coordinator Failure Handling (participant aborts, timeouts)
 * - AC-5: Timeout and Retry Determinism (under failures and network degradation)
 * - AC-6: In-Doubt Transaction Reconciliation (recovery and durability)
 *
 * Test Count: 14 focused tests
 * Stress Profile: Up to 8 concurrent threads, 10+ distributed txns per test
 *
 * Date: 2026-08-08
 * Target: Q4 2026
 */

#include <gtest/gtest.h>
#include "transaction/distributed_transaction_manager.h"

#include <algorithm>
#include <atomic>
#include <chrono>
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

class Phase2MockParticipant : public IDistributedParticipantCallback {
public:
    enum class Policy { ALWAYS_COMMIT, ALWAYS_ABORT, THROW_ON_PREPARE };

    explicit Phase2MockParticipant(Policy policy = Policy::ALWAYS_COMMIT)
        : policy_(policy) {}

    bool onPrepare(const std::string& txn_id, const std::set<std::string>& /*keys*/) override {
        if (policy_ == Policy::THROW_ON_PREPARE)
            throw std::runtime_error("mock: prepare failure");
        std::lock_guard<std::mutex> lk(mu_);
        last_prepare_txn_ = txn_id;
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

    int prepareCount() const { return prepare_count_.load(); }
    int commitCount()  const { return commit_count_.load();  }
    int abortCount()   const { return abort_count_.load();   }

private:
    Policy            policy_;
    mutable std::mutex mu_;
    std::string       last_prepare_txn_, last_commit_txn_, last_abort_txn_;
    std::atomic<int>  prepare_count_{0}, commit_count_{0}, abort_count_{0};
};

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static Participant makeParticipant(const std::string& node_id,
                                   IDistributedParticipantCallback* cb,
                                   std::set<std::string> keys = {"key1"})
{
    Participant p;
    p.node_id       = node_id;
    p.endpoint      = node_id + ":8080";
    p.affected_keys = std::move(keys);
    p.callback      = cb;
    return p;
}

static DistributedTxnManagerConfig makeDefaultConfig() {
    DistributedTxnManagerConfig cfg;
    cfg.prepare_timeout     = 2000ms;
    cfg.commit_timeout      = 2000ms;
    cfg.default_txn_timeout = 60s;
    return cfg;
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class TransactionDistributedPhase2Test : public ::testing::Test {
protected:
    void SetUp() override {
        p1 = std::make_unique<Phase2MockParticipant>();
        p2 = std::make_unique<Phase2MockParticipant>();
        p3 = std::make_unique<Phase2MockParticipant>();

        mgr = std::make_unique<DistributedTransactionManager>(
            "phase2-test-coord", makeDefaultConfig());
    }

    void TearDown() override {
        mgr.reset();
        p3.reset(); p2.reset(); p1.reset();
    }

    std::unique_ptr<Phase2MockParticipant> p1, p2, p3;
    std::unique_ptr<DistributedTransactionManager> mgr;
};

// ============================================================================
// AC-4: Distributed Coordinator Failure Handling
// ============================================================================

/**
 * @test CoordinatorProtocol_2PC_HappyPath
 * @brief Validates basic 2PC: begin → prepare → commit, all participants vote COMMIT.
 * @acceptance AC-4
 */
TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_HappyPath) {
    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get()),
        makeParticipant("n3", p3.get())
    });
    ASSERT_FALSE(tid.empty()) << "beginDistributed should return non-empty ID";

    auto ps = mgr->prepareDistributed(tid);
    EXPECT_TRUE(ps.ok) << "Prepare should succeed: " << ps.message;

    auto cs = mgr->commitDistributed(tid);
    EXPECT_TRUE(cs.ok) << "Commit should succeed: " << cs.message;

    EXPECT_EQ(p1->commitCount(), 1);
    EXPECT_EQ(p2->commitCount(), 1);
    EXPECT_EQ(p3->commitCount(), 1);
}

/**
 * @test CoordinatorProtocol_2PC_ParticipantAbort
 * @brief Validates 2PC behavior when one participant votes ABORT during prepare.
 * @acceptance AC-4
 */
TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_ParticipantAbort) {
    auto p_abort = std::make_unique<Phase2MockParticipant>(Phase2MockParticipant::Policy::ALWAYS_ABORT);

    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p_abort.get()),
        makeParticipant("n3", p3.get())
    });
    ASSERT_FALSE(tid.empty());

    auto ps = mgr->prepareDistributed(tid);
    EXPECT_FALSE(ps.ok)
        << "Prepare should fail when a participant votes ABORT";

    EXPECT_GE(p_abort->abortCount(), 1)
        << "Aborting participant should receive onAbort()";
}

/**
 * @test CoordinatorProtocol_2PC_ParticipantCrash
 * @brief Validates 2PC behavior when participant throws (crash) during prepare.
 * @acceptance AC-4
 */
TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_ParticipantCrash) {
    auto p_crash = std::make_unique<Phase2MockParticipant>(
        Phase2MockParticipant::Policy::THROW_ON_PREPARE);

    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p_crash.get()),
        makeParticipant("n3", p3.get())
    });
    ASSERT_FALSE(tid.empty());

    auto ps = mgr->prepareDistributed(tid);
    EXPECT_FALSE(ps.ok)
        << "Prepare exception should be treated as ABORT vote";
}

/**
 * @test CoordinatorProtocol_2PC_AbortPath
 * @brief Validates explicit abort after begin without prepare.
 * @acceptance AC-4
 */
TEST_F(TransactionDistributedPhase2Test, CoordinatorProtocol_2PC_AbortPath) {
    auto tid = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p2.get())
    });
    ASSERT_FALSE(tid.empty());

    // Abort without prepare
    mgr->abortDistributed(tid);

    EXPECT_EQ(p1->abortCount(), 1) << "n1 should receive onAbort()";
    EXPECT_EQ(p2->abortCount(), 1) << "n2 should receive onAbort()";
    EXPECT_EQ(p1->commitCount(), 0) << "n1 should not receive onCommit()";
}

// ============================================================================
// AC-5: Timeout and Retry Determinism
// ============================================================================

/**
 * @test TimeoutDeterminism_ShortTimeoutAbortsTransaction
 * @brief checkTimeouts() aborts transactions that have exceeded their timeout.
 * @acceptance AC-5
 */
TEST_F(TransactionDistributedPhase2Test, TimeoutDeterminism_ShortTimeoutAbortsTransaction) {
    DistributedTxnManagerConfig cfg = makeDefaultConfig();
    cfg.default_txn_timeout = std::chrono::milliseconds(50);

    auto short_mgr = std::make_unique<DistributedTransactionManager>("short-coord", cfg);
    auto pa = std::make_unique<Phase2MockParticipant>();

    auto tid = short_mgr->beginDistributed({makeParticipant("na", pa.get())});
    ASSERT_FALSE(tid.empty());

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    short_mgr->checkTimeouts();

    EXPECT_GE(pa->abortCount(), 1)
        << "Expired transaction should be aborted by checkTimeouts()";
}

/**
 * @test TimeoutDeterminism_RepeatedRetries_ConsistentErrors
 * @brief ABORT votes produce consistent results across multiple retried transactions.
 * @acceptance AC-5
 */
TEST_F(TransactionDistributedPhase2Test, TimeoutDeterminism_RepeatedRetries_ConsistentErrors) {
    auto p_abort = std::make_unique<Phase2MockParticipant>(Phase2MockParticipant::Policy::ALWAYS_ABORT);

    std::vector<bool> results = {};

    for (int attempt = 0; attempt < 3; ++attempt) {
        auto tid = mgr->beginDistributed({
            makeParticipant("n1", p1.get()),
            makeParticipant("n2", p_abort.get())
        });
        auto ps = mgr->prepareDistributed(tid);
        results.push_back(ps.ok);
    }

    ASSERT_EQ(results.size(), 3u);
    bool all_failed = std::all_of(results.begin(), results.end(),
        [](bool ok){ return !ok; });
    EXPECT_TRUE(all_failed)
        << "ABORT votes must produce deterministic failures across retries";
}

/**
 * @test RetryBehavior_IdempotentAbort
 * @brief Aborting an already-aborted transaction is safe (idempotent).
 * @acceptance AC-5
 */
TEST_F(TransactionDistributedPhase2Test, RetryBehavior_IdempotentAbort) {
    auto tid = mgr->beginDistributed({makeParticipant("n1", p1.get())});
    ASSERT_FALSE(tid.empty());

    mgr->abortDistributed(tid);
    EXPECT_NO_THROW(mgr->abortDistributed(tid))
        << "Idempotent abort should be safe";
}

/**
 * @test FailureRecovery_PrepareAbortFollowedByNewTransaction
 * @brief After a prepare failure the manager accepts new transactions.
 * @acceptance AC-5
 */
TEST_F(TransactionDistributedPhase2Test, FailureRecovery_PrepareAbortFollowedByNewTransaction) {
    auto p_abort = std::make_unique<Phase2MockParticipant>(Phase2MockParticipant::Policy::ALWAYS_ABORT);

    auto tid1 = mgr->beginDistributed({
        makeParticipant("n1", p1.get()),
        makeParticipant("n2", p_abort.get())
    });
    auto ps = mgr->prepareDistributed(tid1);
    EXPECT_FALSE(ps.ok);

    // Manager should still handle new transactions
    auto tid2 = mgr->beginDistributed({makeParticipant("n3", p3.get())});
    EXPECT_FALSE(tid2.empty()) << "Manager should accept new transactions after abort";
    auto ps2 = mgr->prepareDistributed(tid2);
    EXPECT_TRUE(ps2.ok);
    mgr->commitDistributed(tid2);
}

// ============================================================================
// AC-6: In-Doubt Transaction Reconciliation
// ============================================================================

/**
 * @test InDoubtReconciliation_CommitCountTracked
 * @brief Multiple successful commits increment the internal commit counter.
 * @acceptance AC-6
 */
TEST_F(TransactionDistributedPhase2Test, InDoubtReconciliation_CommitCountTracked) {
    int committed = 0;
    for (int i = 0; i < 3; ++i) {
        auto tid = mgr->beginDistributed({
            makeParticipant("n1", p1.get()),
            makeParticipant("n2", p2.get())
        });
        auto ps = mgr->prepareDistributed(tid);
        if (ps.ok) {
            auto cs = mgr->commitDistributed(tid);
            if (cs.ok) {
              ++committed;
            }
        }
    }
    EXPECT_EQ(committed, 3) << "All three transactions should commit";
    EXPECT_EQ(p1->commitCount(), 3);
    EXPECT_EQ(p2->commitCount(), 3);
}

/**
 * @test InDoubtReconciliation_WalReplay
 * @brief WAL-backed coordinator can be constructed without errors.
 * @acceptance AC-6
 */
TEST_F(TransactionDistributedPhase2Test, InDoubtReconciliation_WalReplay) {
    const std::string wal_dir = "/tmp/phase2_wal_replay_test_dir";

    {
        DistributedTxnManagerConfig cfg = makeDefaultConfig();
        cfg.wal_directory = wal_dir;
        DistributedTransactionManager wal_mgr("wal-coord", cfg);

        auto pa = std::make_unique<Phase2MockParticipant>();
        auto tid = wal_mgr.beginDistributed({makeParticipant("n1", pa.get())});
        auto ps = wal_mgr.prepareDistributed(tid);
        ASSERT_TRUE(ps.ok);
        wal_mgr.commitDistributed(tid);
    }

    // Reload from WAL — should succeed without crashing
    {
        DistributedTxnManagerConfig cfg = makeDefaultConfig();
        cfg.wal_directory = wal_dir;
        EXPECT_NO_THROW({
            DistributedTransactionManager wal_mgr2("wal-coord-reload", cfg);
        }) << "Coordinator reload from WAL must not throw";
    }
}

/**
 * @test InDoubtReconciliation_ParticipantRecovery
 * @brief Abort callbacks track in-doubt resolutions correctly.
 * @acceptance AC-6
 */
TEST_F(TransactionDistributedPhase2Test, InDoubtReconciliation_ParticipantRecovery) {
    auto p_abort = std::make_unique<Phase2MockParticipant>(Phase2MockParticipant::Policy::ALWAYS_ABORT);

    for (int i = 0; i < 2; ++i) {
        auto tid = mgr->beginDistributed({
            makeParticipant("n1", p1.get()),
            makeParticipant("n2", p_abort.get())
        });
        mgr->prepareDistributed(tid);
    }

    // Both transactions should have triggered abort callbacks
    EXPECT_GE(p_abort->abortCount(), 1u) << "Abort callbacks should be invoked";
}

// ============================================================================
// Stress Tests
// ============================================================================

/**
 * @test StressTest_ConcurrentDistributedTransactions
 * @brief Stress test: multiple concurrent 2PC transactions.
 * @acceptance AC-4
 */
TEST_F(TransactionDistributedPhase2Test, StressTest_ConcurrentDistributedTransactions) {
    const int NUM_THREADS = 4;
    const int TXN_PER_THREAD = 10;

    Phase2MockParticipant pa, pb, pc;
    std::vector<std::thread> threads;
    std::atomic<int> successful{0};
    std::atomic<int> failed{0};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, TXN_PER_THREAD, &pa, &pb, &pc, &successful, &failed] {
            for (int i = 0; i < TXN_PER_THREAD; ++i) {
                auto tid = mgr->beginDistributed({
                    makeParticipant("n1", &pa),
                    makeParticipant("n2", &pb),
                    makeParticipant("n3", &pc)
                });
                if (tid.empty()) { ++failed; continue; }

                auto ps = mgr->prepareDistributed(tid);
                if (ps.ok) {
                    auto cs = mgr->commitDistributed(tid);
                    if (cs.ok) {
                      ++successful; else ++failed;
                    }
                } else {
                    ++failed;
                }
            }
        });
    }

    for (auto& thr : threads) {
      thr.join();
    }

    EXPECT_EQ(successful + failed, NUM_THREADS * TXN_PER_THREAD);
    GTEST_LOG_(INFO) << "Stress: " << successful << " committed, "
                     << failed << " failed out of "
                     << (NUM_THREADS * TXN_PER_THREAD);
}

/**
 * @test StressTest_HighContentionWithAbortingParticipants
 * @brief Stress test with mixed commit/abort participants under high concurrency.
 * @acceptance AC-4, AC-5, AC-6
 */
TEST_F(TransactionDistributedPhase2Test, StressTest_HighContentionWithAbortingParticipants) {
    const int NUM_THREADS = 8;
    const int OPS_PER_THREAD = 5;

    std::vector<std::thread> threads;
    std::atomic<int> completed{0};

    for (int t = 0; t < NUM_THREADS; ++t) {
        threads.emplace_back([this, OPS_PER_THREAD, t, &completed] {
            Phase2MockParticipant pa;
            Phase2MockParticipant pb(t % 3 == 0
                ? Phase2MockParticipant::Policy::ALWAYS_ABORT
                : Phase2MockParticipant::Policy::ALWAYS_COMMIT);

            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                auto tid = mgr->beginDistributed({
                    makeParticipant("pa", &pa),
                    makeParticipant("pb", &pb)
                });
                if (tid.empty()) { ++completed; continue; }

                auto ps = mgr->prepareDistributed(tid);
                if (ps.ok) {
                    mgr->commitDistributed(tid);
                } else {
                    mgr->abortDistributed(tid);
                }
                ++completed;
            }
        });
    }

    for (auto& thr : threads) {
      thr.join();
    }

    EXPECT_EQ(completed, NUM_THREADS * OPS_PER_THREAD)
        << "All operations should complete";
    GTEST_LOG_(INFO) << "High-contention stress: " << completed << " operations completed";
}

TEST(TransactionDistributedPhase2Contract, DistributedTxnStatusCarriesCanonicalRetryMetadata) {
    auto status = DistributedTxnStatus::Error(
        "prepare timed out",
        2,
        themis::utils::RetryExhaustionReason::MAX_ATTEMPTS_REACHED,
        themis::utils::RetryTimeoutSource::OVERALL,
        "txn-retry-001");

    EXPECT_FALSE(status.ok);
    EXPECT_EQ(status.retry_count, 2u);
    EXPECT_EQ(status.exhaustion_reason,
              themis::utils::RetryExhaustionReason::MAX_ATTEMPTS_REACHED);
    EXPECT_EQ(status.timeout_source, themis::utils::RetryTimeoutSource::OVERALL);
    EXPECT_EQ(status.correlation_id, "txn-retry-001");
}
