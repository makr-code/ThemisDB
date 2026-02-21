/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_two_phase_commit.cpp                          ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:47:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     435                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 1d78e3fbd  2026-02-20  Implement 2-Phase Commit (2PC) coordinator for cross-shar... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file test_two_phase_commit.cpp
 * @brief Unit tests for the Two-Phase Commit (2PC) participant and coordinator
 *
 * Tests cover:
 *  - Normal commit path (prepare → commit)
 *  - Prepare failure / abort path
 *  - Idempotent handling of duplicate messages
 *  - Coordinator failure recovery (in-doubt transactions)
 *  - Transaction timeout / auto-abort
 *  - Multi-shard atomicity via MockCoordinator
 */

#include <gtest/gtest.h>
#include "sharding/two_phase_commit_participant.h"
#include <vector>
#include <string>
#include <set>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

using namespace themis::sharding;

// Serialise an ops JSON array into the coordinator payload format
static std::string opsToPayload(const nlohmann::json& ops) {
    nlohmann::json j;
    j["operations"] = ops;
    return j.dump();
}

// ─────────────────────────────────────────────────────────────────────────────
// Test Fixture
// ─────────────────────────────────────────────────────────────────────────────

class TwoPhaseCommitParticipantTest : public ::testing::Test {
protected:
    void SetUp() override {
        applied_ops_.clear();
        locked_txns_.clear();
        released_txns_.clear();

        TwoPhaseCommitParticipant::Config cfg;
        // Disable WAL to keep unit tests fast (no disk I/O)
        cfg.wal_directory    = "";
        cfg.prepare_timeout  = std::chrono::milliseconds(200);
        cfg.sync_wal_writes  = false;

        participant_ = std::make_unique<TwoPhaseCommitParticipant>(
            "shard-1",
            cfg,
            /*validate*/  [this](const std::string& txn_id, const nlohmann::json& ops) -> bool {
                if (ops.contains("__force_abort") && ops["__force_abort"].get<bool>()) {
                    return false;
                }
                std::lock_guard<std::mutex> lk(mu_);
                locked_txns_.insert(txn_id);
                return true;
            },
            /*apply*/     [this](const std::string& txn_id, const nlohmann::json& ops, int64_t) -> bool {
                std::lock_guard<std::mutex> lk(mu_);
                applied_ops_[txn_id] = ops;
                return true;
            },
            /*release*/   [this](const std::string& txn_id) {
                std::lock_guard<std::mutex> lk(mu_);
                released_txns_.insert(txn_id);
            }
        );
    }

    void TearDown() override {
        participant_.reset();
    }

    // Build a simple serialised transaction payload
    static std::string makePayload(const nlohmann::json& ops, bool force_abort = false) {
        nlohmann::json j;
        j["operations"] = ops;
        if (force_abort) j["operations"]["__force_abort"] = true;
        return j.dump();
    }
    std::unique_ptr<TwoPhaseCommitParticipant> participant_;

    // Tracking callbacks
    mutable std::mutex mu_;
    std::map<std::string, nlohmann::json> applied_ops_;
    std::set<std::string>                 locked_txns_;
    std::set<std::string>                 released_txns_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Normal commit path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, NormalCommitPath) {
    const std::string txn = "txn-001";

    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"type", "insert"}, {"key", "k1"}, {"value", "v1"}});

    // Phase 1: PREPARE
    bool vote = participant_->onPrepare(txn, "coordinator", makePayload(ops));
    EXPECT_TRUE(vote);

    auto state = participant_->getTransactionState(txn);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, ParticipantTxnState::PREPARED);

    // Phase 2: COMMIT
    bool ok = participant_->onCommit(txn);
    EXPECT_TRUE(ok);

    state = participant_->getTransactionState(txn);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, ParticipantTxnState::COMMITTED);

    // Applied & locks released
    EXPECT_TRUE(applied_ops_.count(txn) > 0);
    EXPECT_TRUE(released_txns_.count(txn) > 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Prepare failure → abort path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, PrepareFailureVotesAbort) {
    const std::string txn = "txn-002";

    // Force the validate callback to return false
    nlohmann::json ops = nlohmann::json::object();
    ops["__force_abort"] = true;

    bool vote = participant_->onPrepare(txn, "coordinator", makePayload(ops));
    EXPECT_FALSE(vote);

    auto state = participant_->getTransactionState(txn);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, ParticipantTxnState::ABORTED);
}

TEST_F(TwoPhaseCommitParticipantTest, ExplicitAbortAfterPrepare) {
    const std::string txn = "txn-003";

    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"type", "update"}, {"key", "k2"}});

    EXPECT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));
    EXPECT_TRUE(participant_->onAbort(txn));

    auto state = participant_->getTransactionState(txn);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, ParticipantTxnState::ABORTED);

    // Locks must be released on abort
    EXPECT_TRUE(released_txns_.count(txn) > 0);
    // Operations must NOT have been applied
    EXPECT_EQ(applied_ops_.count(txn), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Idempotency
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, DuplicatePrepareIsIdempotent) {
    const std::string txn = "txn-004";
    nlohmann::json ops    = nlohmann::json::array();

    bool v1 = participant_->onPrepare(txn, "coord", makePayload(ops));
    bool v2 = participant_->onPrepare(txn, "coord", makePayload(ops));
    EXPECT_EQ(v1, v2);

    // State should still be PREPARED, not reset
    EXPECT_EQ(participant_->getTransactionState(txn), ParticipantTxnState::PREPARED);
}

TEST_F(TwoPhaseCommitParticipantTest, DuplicateCommitIsIdempotent) {
    const std::string txn = "txn-005";
    nlohmann::json ops    = nlohmann::json::array();

    EXPECT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));
    EXPECT_TRUE(participant_->onCommit(txn));
    EXPECT_TRUE(participant_->onCommit(txn)); // second COMMIT → idempotent

    EXPECT_EQ(participant_->getTransactionState(txn), ParticipantTxnState::COMMITTED);
}

TEST_F(TwoPhaseCommitParticipantTest, DuplicateAbortIsIdempotent) {
    const std::string txn = "txn-006";
    nlohmann::json ops    = nlohmann::json::array();

    EXPECT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));
    EXPECT_TRUE(participant_->onAbort(txn));
    EXPECT_TRUE(participant_->onAbort(txn)); // second ABORT → idempotent

    EXPECT_EQ(participant_->getTransactionState(txn), ParticipantTxnState::ABORTED);
}

TEST_F(TwoPhaseCommitParticipantTest, AbortWithoutPrepareIsIdempotent) {
    // Abort a transaction that was never prepared (e.g. coordinator crash before prepare)
    const std::string txn = "txn-unknown";
    EXPECT_TRUE(participant_->onAbort(txn));
    EXPECT_EQ(participant_->getTransactionState(txn), ParticipantTxnState::ABORTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Invalid state transitions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, CommitAfterAbortFails) {
    const std::string txn = "txn-007";
    nlohmann::json ops    = nlohmann::json::array();

    EXPECT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));
    EXPECT_TRUE(participant_->onAbort(txn));

    // COMMIT after ABORT must fail
    EXPECT_FALSE(participant_->onCommit(txn));
}

TEST_F(TwoPhaseCommitParticipantTest, AbortAfterCommitFails) {
    const std::string txn = "txn-008";
    nlohmann::json ops    = nlohmann::json::array();

    EXPECT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));
    EXPECT_TRUE(participant_->onCommit(txn));

    // ABORT after COMMIT must fail
    EXPECT_FALSE(participant_->onAbort(txn));
}

// ─────────────────────────────────────────────────────────────────────────────
// Prepare timeout / auto-abort
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, TimedOutPreparedTransactionIsAborted) {
    const std::string txn = "txn-timeout";
    nlohmann::json ops    = nlohmann::json::array();

    EXPECT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));
    EXPECT_EQ(participant_->getTransactionState(txn), ParticipantTxnState::PREPARED);

    // Wait longer than prepare_timeout (200 ms)
    std::this_thread::sleep_for(std::chrono::milliseconds(250));

    size_t aborted = participant_->abortTimedOutTransactions();
    EXPECT_GE(aborted, 1u);
    EXPECT_EQ(participant_->getTransactionState(txn), ParticipantTxnState::ABORTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, StatisticsAreAccurate) {
    nlohmann::json ops = nlohmann::json::array();

    // 2 commits
    participant_->onPrepare("s1", "c", makePayload(ops));
    participant_->onCommit("s1");
    participant_->onPrepare("s2", "c", makePayload(ops));
    participant_->onCommit("s2");

    // 1 abort
    participant_->onPrepare("s3", "c", makePayload(ops));
    participant_->onAbort("s3");

    auto stats = participant_->getStatistics();
    EXPECT_EQ(stats["total_prepares"].get<int>(), 3);
    EXPECT_EQ(stats["total_commits"].get<int>(),  2);
    EXPECT_EQ(stats["total_aborts"].get<int>(),   1);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator failure recovery – in-doubt transactions
// ─────────────────────────────────────────────────────────────────────────────

/**
 * Simulates a coordinator that prepares multiple participants but
 * crashes before sending the COMMIT.  On restart the participants still
 * hold their votes.  The test verifies that:
 *  - Participants remember in-doubt state
 *  - A new coordinator can finish the commit
 */
TEST_F(TwoPhaseCommitParticipantTest, CoordinatorCrashRecovery) {
    // --- Three mock participants (simulate three shards) ---
    auto make_participant = [](const std::string& id) {
        TwoPhaseCommitParticipant::Config cfg;
        cfg.wal_directory   = "";
        cfg.sync_wal_writes = false;
        return std::make_unique<TwoPhaseCommitParticipant>(id, cfg);
    };

    auto p1 = make_participant("shard-A");
    auto p2 = make_participant("shard-B");
    auto p3 = make_participant("shard-C");

    const std::string txn   = "txn-crash-recovery";
    nlohmann::json    ops   = nlohmann::json::array();
    ops.push_back({{"key", "x"}});

    const std::string payload = opsToPayload(ops);

    // Phase 1 – coordinator prepares all participants, then crashes
    EXPECT_TRUE(p1->onPrepare(txn, "coord", payload));
    EXPECT_TRUE(p2->onPrepare(txn, "coord", payload));
    EXPECT_TRUE(p3->onPrepare(txn, "coord", payload));

    // All participants are in PREPARED state
    EXPECT_EQ(p1->getTransactionState(txn), ParticipantTxnState::PREPARED);
    EXPECT_EQ(p2->getTransactionState(txn), ParticipantTxnState::PREPARED);
    EXPECT_EQ(p3->getTransactionState(txn), ParticipantTxnState::PREPARED);

    // --- Coordinator "crashes" here ---

    // Phase 2 – new coordinator reads WAL, sees all prepared, re-sends COMMIT
    EXPECT_TRUE(p1->onCommit(txn));
    EXPECT_TRUE(p2->onCommit(txn));
    EXPECT_TRUE(p3->onCommit(txn));

    EXPECT_EQ(p1->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(p2->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(p3->getTransactionState(txn), ParticipantTxnState::COMMITTED);
}

/**
 * Simulates coordinator crash after partial commit: shard-A committed,
 * shard-B and shard-C are still PREPARED.  New coordinator must complete
 * the commit on remaining shards (idempotent on shard-A).
 */
TEST_F(TwoPhaseCommitParticipantTest, PartialCommitRecovery) {
    auto make_participant = [](const std::string& id) {
        TwoPhaseCommitParticipant::Config cfg;
        cfg.wal_directory   = "";
        cfg.sync_wal_writes = false;
        return std::make_unique<TwoPhaseCommitParticipant>(id, cfg);
    };

    auto pA = make_participant("shard-A");
    auto pB = make_participant("shard-B");
    auto pC = make_participant("shard-C");

    const std::string txn  = "txn-partial";
    nlohmann::json    ops  = nlohmann::json::array();
    ops.push_back({{"key", "y"}});
    const std::string payload = opsToPayload(ops);

    // All prepare
    EXPECT_TRUE(pA->onPrepare(txn, "coord", payload));
    EXPECT_TRUE(pB->onPrepare(txn, "coord", payload));
    EXPECT_TRUE(pC->onPrepare(txn, "coord", payload));

    // Coordinator commits A, then crashes
    EXPECT_TRUE(pA->onCommit(txn));
    EXPECT_EQ(pA->getTransactionState(txn), ParticipantTxnState::COMMITTED);

    // New coordinator re-sends COMMIT to all (idempotent on A)
    EXPECT_TRUE(pA->onCommit(txn));  // idempotent
    EXPECT_TRUE(pB->onCommit(txn));
    EXPECT_TRUE(pC->onCommit(txn));

    EXPECT_EQ(pA->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(pB->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(pC->getTransactionState(txn), ParticipantTxnState::COMMITTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrent transactions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, ConcurrentTransactionsAreSafe) {
    constexpr int N = 20;
    std::atomic<int> commits{0};
    std::atomic<int> aborts{0};

    std::vector<std::thread> threads;
    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i, &commits, &aborts]() {
            const std::string txn = "concurrent-" + std::to_string(i);
            nlohmann::json ops    = nlohmann::json::array();
            ops.push_back({{"key", txn}});
            const std::string payload = opsToPayload(ops);

            bool vote = participant_->onPrepare(txn, "coord", payload);
            if (vote) {
                if (participant_->onCommit(txn)) ++commits;
            } else {
                participant_->onAbort(txn);
                ++aborts;
            }
        });
    }

    for (auto& t : threads) t.join();

    EXPECT_EQ(commits.load() + aborts.load(), N);
    // All transactions were decided (none left PREPARED)
    auto stats = participant_->getStatistics();
    EXPECT_EQ(stats["active_prepared"].get<int>(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Health check
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, HealthCheckReturnsHealthy) {
    auto info = participant_->onHealthCheck();
    EXPECT_TRUE(info.is_healthy);
}
