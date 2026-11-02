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
#include "sharding/cross_shard_transaction.h"
#include "sharding/detail/two_phase_commit_contract.h"
#include "transaction/recoverable_two_phase_coordinator.h"
#include <vector>
#include <string>
#include <set>
#include <map>
#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <filesystem>
#include <random>
#include <stdexcept>
#include <system_error>

using namespace themis::sharding;

// Serialise an ops JSON array into the coordinator payload format
static EpochToken testToken(const std::string& shard_id, const std::string& issuer) {
    EpochToken token;
    token.epoch = 1;
    token.issuer = issuer;
    token.shard_id = shard_id;
    token.issued_at = std::chrono::system_clock::now();
    return token;
}

static std::shared_ptr<EpochFencingManager> testFencingManager(const std::string& shard_id) {
    EpochFencingConfig fencing_cfg;
    fencing_cfg.shard_id = shard_id;
    fencing_cfg.node_id = "participant-" + shard_id;
    fencing_cfg.auto_stonith = false;
    return std::make_shared<EpochFencingManager>(std::move(fencing_cfg));
}

static auto testTokenProvider(const std::string& issuer) {
    return [issuer](const std::string& shard_id) -> std::optional<EpochToken> {
        return testToken(shard_id, issuer);
    };
}

static std::string opsToPayload(const nlohmann::json& ops,
                               const std::string& shard_id = "shard-1",
                               const std::string& issuer = "coord") {
    nlohmann::json j;
    j["operations"] = ops;
    const auto token = testToken(shard_id, issuer);
    j["fencing_token"] = {
        {"epoch", token.epoch},
        {"issuer", token.issuer},
        {"shard_id", token.shard_id},
        {"issued_at_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
            token.issued_at.time_since_epoch()).count()}
    };
    return j.dump();
}

class TemporaryTransactionLog {
public:
    explicit TemporaryTransactionLog(const std::string& prefix) {
        const auto temp_root = std::filesystem::temp_directory_path();
        for (;;) {
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();
            const auto suffix = std::random_device{}();
            directory_ = temp_root /
                (prefix + "_" + std::to_string(stamp) + "_" + std::to_string(suffix));

            std::error_code error;
            if (std::filesystem::create_directory(directory_, error)) {
                break;
            }
            if (error) {
                throw std::filesystem::filesystem_error(
                    "Unable to create temporary transaction log directory",
                    directory_,
                    error);
            }
        }
    }

    ~TemporaryTransactionLog() {
        std::error_code error;
        std::filesystem::remove_all(directory_, error);
    }

    [[nodiscard]] std::filesystem::path logPath() const {
        return directory_ / "transactions.log";
    }

private:
    std::filesystem::path directory_;
};

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
        fencing_manager_ = testFencingManager("shard-1");
        cfg.fencing_manager = fencing_manager_;

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
    static std::string makePayload(const nlohmann::json& ops,
                                   bool force_abort = false,
                                   const std::string& issuer = "coord",
                                   const std::string& shard_id = "shard-1") {
        nlohmann::json j;
        j["operations"] = ops;
        const auto token = testToken(shard_id, issuer);
        j["fencing_token"] = {
            {"epoch", token.epoch},
            {"issuer", token.issuer},
            {"shard_id", token.shard_id},
            {"issued_at_ms", std::chrono::duration_cast<std::chrono::milliseconds>(
                token.issued_at.time_since_epoch()).count()}
        };
        if (force_abort) {
          j["operations"]["__force_abort"] = true;
        }
        return j.dump();
    }
    std::unique_ptr<TwoPhaseCommitParticipant> participant_;
    std::shared_ptr<EpochFencingManager> fencing_manager_;

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
    bool vote = participant_->onPrepare(txn, "coordinator",
                                        makePayload(ops, false, "coordinator"));
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

TEST(TwoPhaseCommitParticipantRecoveryTest, DurableDecisionSurvivesParticipantRestart) {
    const auto wal_directory = (std::filesystem::temp_directory_path() /
        ("themis_participant_recovery_" +
         std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()))).string();
    const std::string transaction_id = "participant-restart-commit";
    TwoPhaseCommitParticipant::Config config;
    config.wal_directory = wal_directory;
    config.fencing_manager = testFencingManager("shard-recovery");

    {
        TwoPhaseCommitParticipant participant("shard-recovery", config);
        const auto payload = opsToPayload(nlohmann::json::array({
            {{"type", "insert"}, {"key", "recovery-key"}, {"value", "recovery-value"}}
        }), "shard-recovery", "coordinator");
        ASSERT_TRUE(participant.onPrepare(transaction_id, "coordinator", payload));
    }

    std::atomic<int> applied_count{0};
    std::atomic<int> restored_lock_count{0};
    {
        TwoPhaseCommitParticipant participant(
            "shard-recovery", config, [&restored_lock_count](
                const std::string&, const nlohmann::json&) {
                restored_lock_count.fetch_add(1, std::memory_order_relaxed);
                return true;
            }, [&applied_count](
                const std::string&, const nlohmann::json&, int64_t) {
                applied_count.fetch_add(1, std::memory_order_relaxed);
                return true;
            });
        EXPECT_EQ(participant.recoverFromWAL(), 1u);
        EXPECT_EQ(participant.getTransactionState(transaction_id),
                  ParticipantTxnState::PREPARED);
        EXPECT_EQ(restored_lock_count.load(std::memory_order_relaxed), 1);
        ASSERT_TRUE(participant.onCommit(transaction_id));
        EXPECT_EQ(applied_count.load(std::memory_order_relaxed), 1);
    }

    {
        TwoPhaseCommitParticipant participant("shard-recovery", config);
        EXPECT_EQ(participant.recoverFromWAL(), 0u);
        EXPECT_EQ(participant.getTransactionState(transaction_id),
                  ParticipantTxnState::COMMITTED);
        EXPECT_EQ(applied_count.load(std::memory_order_relaxed), 1);
    }

    std::filesystem::remove_all(wal_directory);
}

TEST_F(TwoPhaseCommitParticipantTest, MissingFencingTokenVotesAbortBeforeLocking) {
    const auto payload = nlohmann::json{{"operations", nlohmann::json::array()}}.dump();
    EXPECT_FALSE(participant_->onPrepare("txn-missing-token", "coord", payload));
    EXPECT_TRUE(locked_txns_.empty());
    EXPECT_EQ(participant_->getTransactionState("txn-missing-token"),
              ParticipantTxnState::ABORTED);
}

TEST_F(TwoPhaseCommitParticipantTest, WrongShardFencingTokenVotesAbort) {
    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"key", "wrong-shard"}});
    EXPECT_FALSE(participant_->onPrepare(
        "txn-wrong-shard", "coord", makePayload(ops, false, "coord", "shard-2")));
}

TEST_F(TwoPhaseCommitParticipantTest, FutureFencingTokenVotesAbort) {
    nlohmann::json ops = nlohmann::json::array();
    auto payload = nlohmann::json::parse(makePayload(ops));
    payload["fencing_token"]["epoch"] = 2;
    EXPECT_FALSE(participant_->onPrepare("txn-future-token", "coord", payload.dump()));
}

TEST_F(TwoPhaseCommitParticipantTest, EpochBumpBetweenPrepareAndCommitBlocksApply) {
    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"key", "stale-after-prepare"}});
    ASSERT_TRUE(participant_->onPrepare(
        "txn-stale-after-prepare", "coord", makePayload(ops)));

    static_cast<void>(fencing_manager_->bumpEpoch("test failover"));
    EXPECT_FALSE(participant_->onCommit("txn-stale-after-prepare"));
    EXPECT_TRUE(applied_ops_.empty());
}

TEST(TwoPhaseCommitParticipantFencingTest, MissingFencingManagerVotesAbort) {
    TwoPhaseCommitParticipant::Config cfg;
    cfg.wal_directory = "";
    cfg.fencing_manager = nullptr;
    cfg.sync_wal_writes = false;
    TwoPhaseCommitParticipant participant("shard-1", cfg);
    nlohmann::json ops = nlohmann::json::array();
    EXPECT_FALSE(participant.onPrepare("txn-no-manager", "coord", opsToPayload(ops)));
}

// ─────────────────────────────────────────────────────────────────────────────
// Prepare failure → abort path
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitParticipantTest, PrepareFailureVotesAbort) {
    const std::string txn = "txn-002";

    // Force the validate callback to return false
    nlohmann::json ops = nlohmann::json::object();
    ops["__force_abort"] = true;

    bool vote = participant_->onPrepare(txn, "coordinator",
                                        makePayload(ops, false, "coordinator"));
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

TEST_F(TwoPhaseCommitParticipantTest, DuplicatePrepareRejectsMismatchedTokenIssuer) {
    const std::string txn = "txn-duplicate-issuer";
    const nlohmann::json ops = nlohmann::json::array();

    ASSERT_TRUE(participant_->onPrepare(txn, "coord", makePayload(ops)));

    auto mismatched_payload = nlohmann::json::parse(makePayload(ops, false, "spoofed-coordinator"));
    EXPECT_FALSE(participant_->onPrepare(txn, "coord", mismatched_payload.dump()));
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
    participant_->onPrepare("s1", "c", makePayload(ops, false, "c"));
    participant_->onCommit("s1");
    participant_->onPrepare("s2", "c", makePayload(ops, false, "c"));
    participant_->onCommit("s2");

    // 1 abort
    participant_->onPrepare("s3", "c", makePayload(ops, false, "c"));
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
        cfg.fencing_manager = testFencingManager(id);
        return std::make_unique<TwoPhaseCommitParticipant>(id, cfg);
    };

    auto p1 = make_participant("shard-A");
    auto p2 = make_participant("shard-B");
    auto p3 = make_participant("shard-C");

    const std::string txn   = "txn-crash-recovery";
    nlohmann::json    ops   = nlohmann::json::array();
    ops.push_back({{"key", "x"}});

    // Phase 1 – coordinator prepares all participants, then crashes
    EXPECT_TRUE(p1->onPrepare(txn, "coord", opsToPayload(ops, "shard-A")));
    EXPECT_TRUE(p2->onPrepare(txn, "coord", opsToPayload(ops, "shard-B")));
    EXPECT_TRUE(p3->onPrepare(txn, "coord", opsToPayload(ops, "shard-C")));

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
        cfg.fencing_manager = testFencingManager(id);
        return std::make_unique<TwoPhaseCommitParticipant>(id, cfg);
    };

    auto pA = make_participant("shard-A");
    auto pB = make_participant("shard-B");
    auto pC = make_participant("shard-C");

    const std::string txn  = "txn-partial";
    nlohmann::json    ops  = nlohmann::json::array();
    ops.push_back({{"key", "y"}});
    // All prepare
    EXPECT_TRUE(pA->onPrepare(txn, "coord", opsToPayload(ops, "shard-A")));
    EXPECT_TRUE(pB->onPrepare(txn, "coord", opsToPayload(ops, "shard-B")));
    EXPECT_TRUE(pC->onPrepare(txn, "coord", opsToPayload(ops, "shard-C")));

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

    std::vector<std::thread> threads = {};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i, &commits, &aborts]() {
            const std::string txn = "concurrent-" + std::to_string(i);
            nlohmann::json ops    = nlohmann::json::array();
            ops.push_back({{"key", txn}});
            const std::string payload = opsToPayload(ops);

            bool vote = participant_->onPrepare(txn, "coord", payload);
            if (vote) {
                if (participant_->onCommit(txn)) {
                  ++commits;
                }
            } else {
                participant_->onAbort(txn);
                ++aborts;
            }
        });
    }

    for (auto& t : threads) {
      t.join();
    }

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

// =============================================================================
// TwoPhaseCommitCoordinator tests
// =============================================================================

#include "sharding/two_phase_commit_coordinator.h"

using namespace themis::sharding;

// Helper: build a TwoPhaseCommitParticipant with no-op callbacks (no WAL)
static std::unique_ptr<TwoPhaseCommitParticipant>
makeParticipant(const std::string& id) {
    TwoPhaseCommitParticipant::Config cfg;
    cfg.wal_directory   = "";
    cfg.sync_wal_writes = false;
    cfg.fencing_manager = testFencingManager(id);
    return std::make_unique<TwoPhaseCommitParticipant>(id, cfg);
}

class TwoPhaseCommitCoordinatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        TwoPhaseCommitCoordinator::Config cfg;
        cfg.wal_directory   = "";
        cfg.sync_wal_writes = false;
        cfg.fencing_token_provider = testTokenProvider("coord-test");

        coord_ = std::make_unique<TwoPhaseCommitCoordinator>("coord-test", cfg);

        p1_ = makeParticipant("shard-1");
        p2_ = makeParticipant("shard-2");
        p3_ = makeParticipant("shard-3");

        coord_->registerParticipant("shard-1", p1_.get());
        coord_->registerParticipant("shard-2", p2_.get());
        coord_->registerParticipant("shard-3", p3_.get());
    }

    void TearDown() override {
        coord_.reset();
        p1_.reset(); p2_.reset(); p3_.reset();
    }

    std::unique_ptr<TwoPhaseCommitCoordinator> coord_;
    std::unique_ptr<TwoPhaseCommitParticipant> p1_, p2_, p3_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Basic registration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, RegisterParticipantsAndCount) {
    EXPECT_EQ(coord_->participantCount(), 3u);

    EXPECT_TRUE(coord_->unregisterParticipant("shard-3"));
    EXPECT_EQ(coord_->participantCount(), 2u);

    // Unregistering again returns false
    EXPECT_FALSE(coord_->unregisterParticipant("shard-3"));
}

// ─────────────────────────────────────────────────────────────────────────────
// Happy-path commit across all shards
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, SuccessfulCommitAllShards) {
    const std::string txn = "coord-txn-001";

    nlohmann::json ops1 = nlohmann::json::array();
    ops1.push_back({{"type", "insert"}, {"key", "k1"}, {"value", "v1"}});

    nlohmann::json ops2 = nlohmann::json::array();
    ops2.push_back({{"type", "insert"}, {"key", "k2"}, {"value", "v2"}});

    auto outcome = coord_->commit(txn, {
        {"shard-1", ops1},
        {"shard-2", ops2}
    });

    EXPECT_TRUE(outcome.committed());
    EXPECT_EQ(outcome.transaction_id, txn);

    // Both participants should be COMMITTED
    EXPECT_EQ(p1_->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(p2_->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    // shard-3 was not involved
    EXPECT_EQ(p3_->getTransactionState(txn), std::nullopt);
}

// ─────────────────────────────────────────────────────────────────────────────
// Single participant votes ABORT → all abort
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, OneAbortVoteAbortsAllShards) {
    const std::string txn = "coord-txn-002";

    // Participant that always votes ABORT
    TwoPhaseCommitParticipant::Config cfg;
    cfg.wal_directory = "";
    cfg.fencing_manager = testFencingManager("shard-stat-abort");
    auto aborter = std::make_unique<TwoPhaseCommitParticipant>(
        "shard-abort",
        cfg,
        /*validate*/ [](const std::string&, const nlohmann::json&) { return false; },
        nullptr,
        nullptr
    );
    coord_->registerParticipant("shard-abort", aborter.get());

    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"key", "x"}});

    auto outcome = coord_->commit(txn, {
        {"shard-1",     ops},
        {"shard-abort", ops}
    });

    EXPECT_EQ(outcome.result, CoordinatorTxnResult::ABORTED);

    EXPECT_EQ(p1_->getTransactionState(txn),      ParticipantTxnState::ABORTED);
    EXPECT_EQ(aborter->getTransactionState(txn),  ParticipantTxnState::ABORTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Empty ops_per_shard
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, EmptyOpsMapReturnsAborted) {
    auto outcome = coord_->commit("coord-txn-empty", {});
    EXPECT_EQ(outcome.result, CoordinatorTxnResult::ABORTED);
}

TEST(TwoPhaseCommitCoordinatorFencingTest, MissingTokenProviderFailsBeforePrepare) {
    TwoPhaseCommitCoordinator coord("coord-no-provider");
    auto participant = makeParticipant("shard-1");
    coord.registerParticipant("shard-1", participant.get());

    nlohmann::json ops = nlohmann::json::array();
    const auto outcome = coord.commit("txn-no-provider", {{"shard-1", ops}});
    EXPECT_EQ(outcome.result, CoordinatorTxnResult::ERROR);
    EXPECT_FALSE(participant->getTransactionState("txn-no-provider").has_value());
}

TEST(TwoPhaseCommitCoordinatorFencingTest, FailedParticipantCommitIsNotReportedAsSuccess) {
    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory = "";
    cfg.sync_wal_writes = false;
    cfg.fencing_token_provider = testTokenProvider("coord-failed-apply");
    TwoPhaseCommitCoordinator coord("coord-failed-apply", cfg);

    TwoPhaseCommitParticipant::Config participant_cfg;
    participant_cfg.wal_directory = "";
    participant_cfg.fencing_manager = testFencingManager("shard-apply-fails");
    TwoPhaseCommitParticipant participant(
        "shard-apply-fails", participant_cfg, nullptr,
        [](const std::string&, const nlohmann::json&, int64_t) { return false; });
    coord.registerParticipant("shard-apply-fails", &participant);

    nlohmann::json ops = nlohmann::json::array();
    const auto outcome = coord.commit("txn-apply-fails", {{"shard-apply-fails", ops}});
    EXPECT_EQ(outcome.result, CoordinatorTxnResult::ERROR);
    EXPECT_EQ(coord.getTransactionState("txn-apply-fails"),
              CoordinatorTxnState::COMMIT_DECIDED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Unknown shard returns FAILURE
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, UnknownShardReturnsError) {
    nlohmann::json ops = nlohmann::json::array();
    auto outcome = coord_->commit("coord-txn-unknown-shard", {
        {"shard-does-not-exist", ops}
    });
    EXPECT_EQ(outcome.result, CoordinatorTxnResult::FAILURE);
}

// ─────────────────────────────────────────────────────────────────────────────
// Idempotent re-commit of completed transaction
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, RecommittingCompletedTxnIsIdempotent) {
    const std::string txn = "coord-txn-idem";
    nlohmann::json ops    = nlohmann::json::array();
    ops.push_back({{"key", "y"}});

    auto o1 = coord_->commit(txn, {{"shard-1", ops}});
    EXPECT_TRUE(o1.committed());

    // Second call with same txn_id → must return COMMITTED idempotently
    auto o2 = coord_->commit(txn, {{"shard-1", ops}});
    EXPECT_TRUE(o2.committed());
}

TEST_F(TwoPhaseCommitCoordinatorTest, RecommittingAbortedTxnPreservesAbortDecision) {
    const std::string txn = "coord-txn-idem-abort";
    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"key", "reject"}});

    TwoPhaseCommitParticipant rejecting_participant(
        "shard-reject-prepare",
        [](const std::string&, const nlohmann::json&) { return false; });
    coord_->registerParticipant("shard-reject-prepare", &rejecting_participant);

    const auto first = coord_->commit(txn, {{"shard-reject-prepare", ops}});
    ASSERT_EQ(first.result, CoordinatorTxnResult::ABORTED);

    const auto repeated = coord_->commit(txn, {{"shard-reject-prepare", ops}});
    EXPECT_EQ(repeated.result, CoordinatorTxnResult::ABORTED);
}

namespace {

class RejectingPhaseTwoParticipant final : public ShardRPCServer::RequestHandler {
public:
    explicit RejectingPhaseTwoParticipant(
        bool prepare_vote = true,
        bool commit_ack = false,
        bool abort_ack = true)
        : prepare_vote(prepare_vote)
        , commit_ack(commit_ack)
        , abort_ack(abort_ack) {}

    bool onPrepare(const std::string&, const std::string&, const std::string&) override {
        return prepare_vote;
    }

    bool onCommit(const std::string&) override {
        ++commit_calls;
        return commit_ack;
    }

    bool onAbort(const std::string&) override {
        ++abort_calls;
        return abort_ack;
    }

    ShardRPCServer::HealthInfo onHealthCheck() override {
        return {};
    }

    int commit_calls = 0;
    int abort_calls = 0;
    bool prepare_vote;
    bool commit_ack;
    bool abort_ack;
};

} // namespace

TEST(TwoPhaseCommitContractTest, DecisionPersistenceFailurePreventsPhaseTwo) {
    bool phase_two_called = false;
    const auto result = themis::sharding::detail::executeTwoPhaseCommit(
        [] { return true; },
        [](bool) { return false; },
        [&phase_two_called](bool) {
            phase_two_called = true;
            return true;
        });

    EXPECT_EQ(result, themis::sharding::detail::TwoPhaseCommitExecutionResult::FAILURE);
    EXPECT_FALSE(phase_two_called);
}

TEST(TwoPhaseCommitContractTest, FailedCommitAcknowledgementNeverChangesDecisionToAbort) {
    std::vector<std::string> events;
    const auto result = themis::sharding::detail::executeTwoPhaseCommit(
        [&events] {
            events.emplace_back("prepare");
            return true;
        },
        [&events](bool commit) {
            events.emplace_back(commit ? "persist-commit" : "persist-abort");
            return true;
        },
        [&events](bool commit) {
            events.emplace_back(commit ? "commit" : "abort");
            return false;
        });

    EXPECT_EQ(result, themis::sharding::detail::TwoPhaseCommitExecutionResult::FAILURE);
    EXPECT_EQ(events, (std::vector<std::string>{
        "prepare", "persist-commit", "commit"}));
}

TEST_F(TwoPhaseCommitCoordinatorTest, FailedCommitAcknowledgementReturnsErrorWithoutAbort) {
    RejectingPhaseTwoParticipant participant;
    coord_->registerParticipant("shard-reject-commit", &participant);
    const nlohmann::json ops = nlohmann::json::array({{{"key", "phase-two-failure"}}});

    const auto result = coord_->commit("txn-phase-two-failure", {
        {"shard-reject-commit", ops}
    });

    EXPECT_EQ(result.result, CoordinatorTxnResult::FAILURE);
    EXPECT_EQ(participant.commit_calls, 1);
    EXPECT_EQ(participant.abort_calls, 0);
    EXPECT_EQ(coord_->getTransactionState("txn-phase-two-failure"),
              CoordinatorTxnState::COMMIT_DECIDED);
    const auto recoverable = coord_->getRecoverableTransactions();
    ASSERT_EQ(recoverable.size(), 1u);
    EXPECT_TRUE(recoverable.front().decision_recorded);
    EXPECT_TRUE(recoverable.front().decision_commit);
}

TEST_F(TwoPhaseCommitCoordinatorTest, FailedAbortAcknowledgementReturnsError) {
    RejectingPhaseTwoParticipant participant(false, true, false);
    coord_->registerParticipant("shard-reject-abort", &participant);
    const nlohmann::json ops = nlohmann::json::array({{{"key", "abort-ack-failure"}}});

    const auto result = coord_->commit("txn-abort-ack-failure", {
        {"shard-reject-abort", ops}
    });

    EXPECT_EQ(result.result, CoordinatorTxnResult::FAILURE);
    EXPECT_EQ(participant.commit_calls, 0);
    EXPECT_EQ(participant.abort_calls, 1);
    EXPECT_EQ(coord_->getTransactionState("txn-abort-ack-failure"),
              CoordinatorTxnState::ABORT_DECIDED);
    const auto recoverable = coord_->getRecoverableTransactions();
    ASSERT_EQ(recoverable.size(), 1u);
    EXPECT_TRUE(recoverable.front().decision_recorded);
    EXPECT_FALSE(recoverable.front().decision_commit);
}
TEST(CrossShardTwoPhaseCommitContractTest, ExplicitPrepareCommitAndRepeatedCommitMatchCanonicalDecision) {
    TemporaryTransactionLog transaction_log("themis-cross-shard-2pc-contract");
    themisdb::sharding::CrossShardTransactionConfig config;
    config.transaction_log_path = transaction_log.logPath().string();
    config.prepare_timeout = std::chrono::milliseconds(20);
    config.commit_timeout = std::chrono::milliseconds(20);
    config.abort_timeout = std::chrono::milliseconds(20);
    themisdb::sharding::CrossShardTransactionCoordinator coordinator(config, nullptr);

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cross-2pc-contract",
        themisdb::sharding::TransactionProtocol::TWO_PHASE_COMMIT));
    ASSERT_TRUE(coordinator.addParticipant(
        "txn-cross-2pc-contract", "shard-a", "inproc://contract-shard-a", {"write"}));
    ASSERT_TRUE(coordinator.prepare("txn-cross-2pc-contract"));
    EXPECT_EQ(coordinator.getTransactionState("txn-cross-2pc-contract"),
              themisdb::sharding::TransactionState::PREPARED);

    EXPECT_TRUE(coordinator.commit("txn-cross-2pc-contract"));
    EXPECT_TRUE(coordinator.commit("txn-cross-2pc-contract"));
    EXPECT_EQ(coordinator.getTransactionState("txn-cross-2pc-contract"),
              themisdb::sharding::TransactionState::COMMITTED);
}

TEST(CrossShardTwoPhaseCommitContractTest, FailedCommitAcknowledgementRemainsUnknownWithoutAbort) {
    int prepare_calls = 0;
    int commit_calls = 0;
    int abort_calls = 0;
    TemporaryTransactionLog transaction_log("themis-cross-shard-2pc-phase-two-failure");
    themisdb::sharding::CrossShardTransactionConfig config;
    config.transaction_log_path = transaction_log.logPath().string();
    config.commit_timeout = std::chrono::milliseconds(20);
    config.in_process_rpc_response_handler =
        [&prepare_calls, &commit_calls, &abort_calls](
            std::string method, nlohmann::json) -> nlohmann::json {
            if (method == "prepare") {
                ++prepare_calls;
                return {{"vote", "commit"}, {"status", "prepared"}};
            }
            if (method == "commit") {
                ++commit_calls;
                return {{"status", "rejected"}};
            }
            if (method == "abort") {
                ++abort_calls;
                return {{"status", "aborted"}};
            }
            return {{"status", "error"}};
        };
    themisdb::sharding::CrossShardTransactionCoordinator coordinator(config, nullptr);

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cross-2pc-phase-two-failure",
        themisdb::sharding::TransactionProtocol::TWO_PHASE_COMMIT));
    ASSERT_TRUE(coordinator.addParticipant(
        "txn-cross-2pc-phase-two-failure",
        "shard-a",
        "inproc://contract-phase-two-failure",
        {"write"}));
    ASSERT_TRUE(coordinator.prepare("txn-cross-2pc-phase-two-failure"));

    EXPECT_FALSE(coordinator.commit("txn-cross-2pc-phase-two-failure"));
    EXPECT_EQ(prepare_calls, 1);
    EXPECT_EQ(commit_calls, 1);
    EXPECT_EQ(abort_calls, 0);
    EXPECT_EQ(coordinator.getTransactionState("txn-cross-2pc-phase-two-failure"),
              themisdb::sharding::TransactionState::UNKNOWN);
}

TEST(CrossShardTwoPhaseCommitContractTest, FailedAbortAcknowledgementRemainsUnknown) {
    int abort_calls = 0;
    TemporaryTransactionLog transaction_log("themis-cross-shard-2pc-abort-ack-failure");
    themisdb::sharding::CrossShardTransactionConfig config;
    config.transaction_log_path = transaction_log.logPath().string();
    config.in_process_rpc_response_handler =
        [&abort_calls](std::string method, nlohmann::json) -> nlohmann::json {
            if (method == "prepare") {
                return {{"vote", "abort"}, {"status", "rejected"}};
            }
            if (method == "abort") {
                ++abort_calls;
                return {{"status", "rejected"}};
            }
            return {{"status", "unexpected"}};
        };
    themisdb::sharding::CrossShardTransactionCoordinator coordinator(config, nullptr);

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cross-2pc-abort-ack-failure",
        themisdb::sharding::TransactionProtocol::TWO_PHASE_COMMIT));
    ASSERT_TRUE(coordinator.addParticipant(
        "txn-cross-2pc-abort-ack-failure",
        "shard-a",
        "inproc://contract-abort-ack-failure",
        {"write"}));

    EXPECT_FALSE(coordinator.commit("txn-cross-2pc-abort-ack-failure"));
    EXPECT_EQ(abort_calls, 1);
    EXPECT_EQ(coordinator.getTransactionState("txn-cross-2pc-abort-ack-failure"),
              themisdb::sharding::TransactionState::UNKNOWN);
}

TEST(CrossShardTwoPhaseCommitContractTest, EmptyPrepareFailureSelectsAbortAndIsIdempotent) {
    TemporaryTransactionLog transaction_log("themis-cross-shard-2pc-abort-contract");
    themisdb::sharding::CrossShardTransactionConfig config;
    config.transaction_log_path = transaction_log.logPath().string();
    themisdb::sharding::CrossShardTransactionCoordinator coordinator(config, nullptr);

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cross-2pc-abort",
        themisdb::sharding::TransactionProtocol::TWO_PHASE_COMMIT));
    EXPECT_FALSE(coordinator.commit("txn-cross-2pc-abort"));
    EXPECT_EQ(coordinator.getTransactionState("txn-cross-2pc-abort"),
              themisdb::sharding::TransactionState::ABORTED);
    EXPECT_TRUE(coordinator.abort("txn-cross-2pc-abort"));
}

TEST(CrossShardTwoPhaseCommitContractTest, ExplicitAbortBroadcastsAndCanBeRepeated) {
    TemporaryTransactionLog transaction_log("themis-cross-shard-2pc-explicit-abort");
    themisdb::sharding::CrossShardTransactionConfig config;
    config.transaction_log_path = transaction_log.logPath().string();
    config.abort_timeout = std::chrono::milliseconds(20);
    themisdb::sharding::CrossShardTransactionCoordinator coordinator(config, nullptr);

    ASSERT_TRUE(coordinator.beginTransaction(
        "txn-cross-2pc-explicit-abort",
        themisdb::sharding::TransactionProtocol::TWO_PHASE_COMMIT));
    ASSERT_TRUE(coordinator.addParticipant(
        "txn-cross-2pc-explicit-abort", "shard-a", "inproc://contract-abort-shard", {"write"}));

    EXPECT_TRUE(coordinator.abort("txn-cross-2pc-explicit-abort"));
    EXPECT_TRUE(coordinator.abort("txn-cross-2pc-explicit-abort"));
    EXPECT_FALSE(coordinator.commit("txn-cross-2pc-explicit-abort"));
    EXPECT_EQ(coordinator.getTransactionState("txn-cross-2pc-explicit-abort"),
              themisdb::sharding::TransactionState::ABORTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator state tracking
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, TransactionStateAfterCommit) {
    const std::string txn = "coord-txn-state";
    nlohmann::json ops    = nlohmann::json::array();
    ops.push_back({{"key", "z"}});

    coord_->commit(txn, {{"shard-1", ops}});

    auto state = coord_->getTransactionState(txn);
    ASSERT_TRUE(state.has_value());
    EXPECT_EQ(*state, CoordinatorTxnState::COMPLETED);
}

TEST_F(TwoPhaseCommitCoordinatorTest, UnknownTransactionStateIsNullopt) {
    EXPECT_EQ(coord_->getTransactionState("does-not-exist"), std::nullopt);
}

// ─────────────────────────────────────────────────────────────────────────────
// Statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, StatisticsAreAccurate) {
    nlohmann::json ops = nlohmann::json::array();
    ops.push_back({{"key", "a"}});

    coord_->commit("stat-txn-1", {{"shard-1", ops}});
    coord_->commit("stat-txn-2", {{"shard-2", ops}});

    // Force an abort
    TwoPhaseCommitParticipant::Config cfg;
    cfg.wal_directory = "";
    auto aborter = std::make_unique<TwoPhaseCommitParticipant>(
        "shard-stat-abort", cfg,
        [](const std::string&, const nlohmann::json&) { return false; },
        nullptr, nullptr
    );
    coord_->registerParticipant("shard-stat-abort", aborter.get());
    coord_->commit("stat-txn-3", {{"shard-stat-abort", ops}});

    auto stats = coord_->getStatistics();
    EXPECT_EQ(stats["total_transactions"].get<uint64_t>(), 3u);
    EXPECT_EQ(stats["total_commits"].get<uint64_t>(),      2u);
    EXPECT_EQ(stats["total_aborts"].get<uint64_t>(),       1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Multi-shard atomicity: all three shards must reach same decision
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, AllShardsReachSameDecision) {
    const std::string txn = "coord-txn-atomicity";
    nlohmann::json ops    = nlohmann::json::array();
    ops.push_back({{"key", "multi"}});

    auto outcome = coord_->commit(txn, {
        {"shard-1", ops},
        {"shard-2", ops},
        {"shard-3", ops}
    });

    EXPECT_TRUE(outcome.committed());

    EXPECT_EQ(p1_->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(p2_->getTransactionState(txn), ParticipantTxnState::COMMITTED);
    EXPECT_EQ(p3_->getTransactionState(txn), ParticipantTxnState::COMMITTED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Concurrent coordinator transactions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, ConcurrentCoordinatorTransactionsAreSafe) {
    // Use separate participants per transaction to avoid conflicting state
    constexpr int N = 10;
    std::atomic<int> commits{0}, aborts{0};

    // Create N independent participants so each thread operates independently
    std::vector<std::unique_ptr<TwoPhaseCommitParticipant>> parts(N);
    for (int i = 0; i < N; ++i) {
        parts[i] = makeParticipant("shard-cc-" + std::to_string(i));
        coord_->registerParticipant("shard-cc-" + std::to_string(i), parts[i].get());
    }

    std::vector<std::thread> threads = {};

    for (int i = 0; i < N; ++i) {
        threads.emplace_back([this, i, &commits, &aborts]() {
            const std::string txn     = "cc-txn-" + std::to_string(i);
            const std::string shard   = "shard-cc-" + std::to_string(i);
            nlohmann::json ops        = nlohmann::json::array();
            ops.push_back({{"key", txn}});

            auto outcome = coord_->commit(txn, {{shard, ops}});
            if (outcome.committed()) {
              ++commits;
            }
            else                    ++aborts;
        });
    }

    for (auto& t : threads) {
      t.join();
    }

    EXPECT_EQ(commits.load() + aborts.load(), N);

    auto stats = coord_->getStatistics();
    EXPECT_EQ(stats["total_transactions"].get<uint64_t>(), static_cast<uint64_t>(N));
}

// =============================================================================
// ShardRPCClientAdapter tests
// =============================================================================

#include "sharding/shard_rpc_client_adapter.h"

/**
 * The adapter wraps a ShardRPCClient and exposes the RequestHandler interface.
 * The ShardRPCClient in this environment uses the in-process simulation path
 * when the endpoint is not a valid gRPC host, so the tests verify adapter
 * construction and the protocol translation logic.
 */

// ─────────────────────────────────────────────────────────────────────────────
// Adapter construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(ShardRPCClientAdapterTest, ConstructWithConfig) {
    ShardRPCClient::Config cfg;
    cfg.endpoint     = "localhost:50051";
    cfg.timeout_ms   = 1000;
    cfg.max_retries  = 0; // no retries so tests are fast

    // Construction must not throw
    EXPECT_NO_THROW(ShardRPCClientAdapter adapter(cfg));
}

TEST(ShardRPCClientAdapterTest, HealthCheckReturnsStructuredInfo) {
    ShardRPCClient::Config cfg;
    cfg.endpoint     = "localhost:0"; // unreachable
    cfg.timeout_ms   = 50;
    cfg.max_retries  = 0;

    ShardRPCClientAdapter adapter(cfg);

    // HealthInfo must always be populated (not crash); is_healthy may be false
    auto info = adapter.onHealthCheck();
    // No assertion on is_healthy – network is not available in tests
    (void)info;
}

// ─────────────────────────────────────────────────────────────────────────────
// Adapter payload parsing: malformed data → ABORT vote
// ─────────────────────────────────────────────────────────────────────────────

TEST(ShardRPCClientAdapterTest, MalformedPayloadVotesAbort) {
    ShardRPCClient::Config cfg;
    cfg.endpoint    = "localhost:0";
    cfg.timeout_ms  = 50;
    cfg.max_retries = 0;

    ShardRPCClientAdapter adapter(cfg);

    // Invalid JSON → must return false (ABORT vote) without crashing
    bool vote = adapter.onPrepare("txn-bad", "coord", "{not-valid-json!!!}");
    EXPECT_FALSE(vote);
}

TEST(ShardRPCClientAdapterTest, MissingOrMismatchedFencingTokenVotesAbort) {
    ShardRPCClient::Config cfg;
    cfg.endpoint = "localhost:0";
    cfg.timeout_ms = 50;
    cfg.max_retries = 0;

    ShardRPCClientAdapter adapter(cfg);

    const auto missing_token = nlohmann::json{
        {"operations", nlohmann::json::array()}
    }.dump();
    EXPECT_FALSE(adapter.onPrepare("txn-no-token", "coord", missing_token));

    auto mismatched = nlohmann::json::parse(
        opsToPayload(nlohmann::json::array(), "shard-1", "other-coordinator"));
    EXPECT_FALSE(adapter.onPrepare("txn-wrong-issuer", "coord", mismatched.dump()));
}

TEST(ShardRPCClientAdapterTest, FailedRemoteAbortIsNotReportedAsAcknowledged) {
    ShardRPCClient::Config cfg;
    cfg.endpoint = "localhost:0";
    cfg.timeout_ms = 50;
    cfg.max_retries = 0;

    ShardRPCClientAdapter adapter(cfg);
    EXPECT_FALSE(adapter.onAbort("txn-abort-nack"));
}

TEST(ShardRPCClientAdapterTest, RemoteAbortRejectionIsNotReportedAsAcknowledged) {
    ShardRPCClient::Config cfg;
    cfg.endpoint = "inproc://adapter-abort-rejection";
    cfg.max_retries = 1;
    int abort_calls = 0;
    ShardRPCClientAdapter adapter(
        cfg,
        [&abort_calls](std::string method, nlohmann::json) -> nlohmann::json {
            if (method == "abort") {
                ++abort_calls;
                return {{"status", "rejected"}};
            }
            return {{"status", "error"}};
        });

    EXPECT_FALSE(adapter.onAbort("txn-abort-rejected"));
    EXPECT_EQ(abort_calls, 1);
}

TEST(ShardRPCClientAdapterTest, InProcessHandlerRejectsRemoteEndpoint) {
    ShardRPCClient::Config cfg;
    cfg.endpoint = "shard-a:50051";

    EXPECT_THROW(
        ShardRPCClientAdapter adapter(cfg, {}),
        std::invalid_argument);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator: registerParticipantByEndpoint
// ─────────────────────────────────────────────────────────────────────────────

TEST(TwoPhaseCommitCoordinatorEndpointTest, RegisterByEndpointIncreasesCount) {
    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory   = "";
    cfg.sync_wal_writes = false;
    cfg.fencing_token_provider = testTokenProvider("coord-ep");

    TwoPhaseCommitCoordinator coord("coord-ep", cfg);
    EXPECT_EQ(coord.participantCount(), 0u);

    ShardRPCClient::Config rpc_cfg;
    rpc_cfg.endpoint    = "shard-a:50051";
    rpc_cfg.timeout_ms  = 500;
    rpc_cfg.max_retries = 0;

    coord.registerParticipantByEndpoint("shard-a", rpc_cfg);
    EXPECT_EQ(coord.participantCount(), 1u);

    coord.registerParticipantByEndpoint("shard-b", rpc_cfg);
    EXPECT_EQ(coord.participantCount(), 2u);
}

TEST(TwoPhaseCommitCoordinatorEndpointTest, UnregisterRemovesEndpointParticipant) {
    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory   = "";
    cfg.sync_wal_writes = false;
    cfg.fencing_token_provider = testTokenProvider("coord-ep2");

    TwoPhaseCommitCoordinator coord("coord-ep2", cfg);

    ShardRPCClient::Config rpc_cfg;
    rpc_cfg.endpoint    = "shard-x:50051";
    rpc_cfg.timeout_ms  = 500;
    rpc_cfg.max_retries = 0;

    coord.registerParticipantByEndpoint("shard-x", rpc_cfg);
    EXPECT_EQ(coord.participantCount(), 1u);

    EXPECT_TRUE(coord.unregisterParticipant("shard-x"));
    EXPECT_EQ(coord.participantCount(), 0u);
}

TEST(TwoPhaseCommitCoordinatorEndpointTest, RegisterByEndpointCanBeOverwritten) {
    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory   = "";
    cfg.sync_wal_writes = false;
    cfg.fencing_token_provider = testTokenProvider("coord-ep3");

    TwoPhaseCommitCoordinator coord("coord-ep3", cfg);

    ShardRPCClient::Config rpc_cfg;
    rpc_cfg.endpoint    = "shard-y:50051";
    rpc_cfg.timeout_ms  = 500;
    rpc_cfg.max_retries = 0;

    coord.registerParticipantByEndpoint("shard-y", rpc_cfg);

    // Re-register same shard with different endpoint (update)
    rpc_cfg.endpoint = "shard-y-new:50052";
    coord.registerParticipantByEndpoint("shard-y", rpc_cfg);

    // Count should still be 1 (overwrite, not duplicate)
    EXPECT_EQ(coord.participantCount(), 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Coordinator: recoverInDoubtTransactions
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(TwoPhaseCommitCoordinatorTest, RecoverWithoutWALReturnsZero) {
    // When WAL is disabled (wal_directory=""), recovery is a no-op that returns 0
    size_t resolved = coord_->recoverInDoubtTransactions();
    EXPECT_EQ(resolved, 0u);
}

TEST(TwoPhaseCommitCoordinatorRecoveryTest, RecoverWithWALAfterCleanRun) {
    // A coordinator that committed all transactions cleanly should report 0
    // in-doubt transactions on recovery.
    //
    // Use a RAII guard to ensure the WAL directory is cleaned up even if
    // an assertion fails.
    const auto wal_dir = (std::filesystem::temp_directory_path() /
                          ("2pc_coord_wal_" +
                           std::to_string(::getpid()) + "_" +
                           std::to_string(
                               std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count())))
                             .string();

    struct WalDirGuard {
        const std::string& path;
        ~WalDirGuard() { std::filesystem::remove_all(path); }
    } guard{wal_dir};

    std::filesystem::create_directories(wal_dir);

    {
        TwoPhaseCommitCoordinator::Config cfg;
        cfg.wal_directory   = wal_dir;
        cfg.sync_wal_writes = false;
        cfg.fencing_token_provider = testTokenProvider("coord-wal-test");

        TwoPhaseCommitCoordinator coord("coord-wal-test", cfg);

        TwoPhaseCommitParticipant::Config p_cfg;
        p_cfg.wal_directory = "";
        p_cfg.fencing_manager = testFencingManager("shard-wal");
        auto participant    = std::make_unique<TwoPhaseCommitParticipant>("shard-wal", p_cfg);
        coord.registerParticipant("shard-wal", participant.get());

        nlohmann::json ops = nlohmann::json::array();
        ops.push_back({{"key", "a"}});

        auto outcome = coord.commit("wal-txn-1", {{"shard-wal", ops}});
        EXPECT_TRUE(outcome.committed());
    }

    // "Restart": new coordinator instance reads the same WAL
    {
        TwoPhaseCommitCoordinator::Config cfg;
        cfg.wal_directory   = wal_dir;
        cfg.sync_wal_writes = false;
        cfg.fencing_token_provider = testTokenProvider("coord-wal-test");

        TwoPhaseCommitCoordinator coord("coord-wal-test", cfg);

        // All transactions were completed cleanly – nothing to re-drive
        size_t resolved = coord.recoverInDoubtTransactions();
        EXPECT_EQ(resolved, 0u);
    }
    // guard destructor removes wal_dir
}

TEST(TwoPhaseCommitCoordinatorRecoveryTest, RecoverWithWALReplaysDurableCommitDecision) {
    const auto wal_dir = (std::filesystem::temp_directory_path() /
                          ("2pc_coord_wal_replay_" +
                           std::to_string(::getpid()) + "_" +
                           std::to_string(
                               std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count())))
                             .string();

    struct WalDirGuard {
        const std::string& path;
        ~WalDirGuard() { std::filesystem::remove_all(path); }
    } guard{wal_dir};

    std::filesystem::create_directories(wal_dir);

    const std::string txn = "wal-txn-replay-commit";
    {
        WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = wal_dir;
        wal_cfg.sync_on_write = true;
        WALManager wal(wal_cfg);

        TwoPhaseCommitParticipant::Config p_cfg;
        p_cfg.wal_directory = "";
        p_cfg.sync_wal_writes = false;
        p_cfg.fencing_manager = testFencingManager("shard-wal");
        auto prepared_participant =
            std::make_unique<TwoPhaseCommitParticipant>("shard-wal", p_cfg);
        const auto ops = nlohmann::json::array();
        ASSERT_TRUE(prepared_participant->onPrepare(
            txn, "coord-wal-test", opsToPayload(ops, "shard-wal", "coord-wal-test")));
        ASSERT_EQ(prepared_participant->getTransactionState(txn),
                  ParticipantTxnState::PREPARED);

        WALEntry begin_entry;
        begin_entry.type = WALEntryType::BEGIN_TX;
        begin_entry.transaction_id = txn;
        begin_entry.data = {
            {"transaction_id", txn},
            {"coordinator_id", "coord-wal-test"},
            {"shards", nlohmann::json::array({"shard-wal"})}
        };
        wal.append(begin_entry);

        WALEntry decision_entry;
        decision_entry.type = WALEntryType::COMMIT_TX;
        decision_entry.transaction_id = txn;
        decision_entry.data = {
            {"transaction_id", txn},
            {"coordinator_id", "coord-wal-test"},
            {"phase", "decision"},
            {"decision", "commit"}
        };
        wal.append(decision_entry);
        wal.flush();
    }

    RejectingPhaseTwoParticipant participant;

    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory = wal_dir;
    cfg.sync_wal_writes = true;
    cfg.fencing_token_provider = testTokenProvider("coord-wal-test");

    TwoPhaseCommitCoordinator coord("coord-wal-test", cfg);
    coord.registerParticipant("shard-wal", &participant);

    EXPECT_EQ(coord.recoverInDoubtTransactions(), 0u);
    EXPECT_EQ(coord.getTransactionState(txn), CoordinatorTxnState::COMMIT_DECIDED);
    auto in_doubt = coord.getRecoverableTransactions();
    ASSERT_EQ(in_doubt.size(), 1u);
    EXPECT_TRUE(in_doubt.front().decision_recorded);
    EXPECT_TRUE(in_doubt.front().decision_commit);

    participant.commit_ack = true;
    EXPECT_EQ(coord.recoverInDoubtTransactions(), 1u);
    EXPECT_EQ(coord.getTransactionState(txn), CoordinatorTxnState::COMPLETED);
    EXPECT_EQ(participant.commit_calls, 2);
}

TEST(TwoPhaseCommitCoordinatorRecoveryTest,
     ConservativeAbortRemainsInDoubtUntilAcknowledged) {
    const auto wal_dir = (std::filesystem::temp_directory_path() /
                          ("2pc_coord_wal_abort_replay_" +
                           std::to_string(::getpid()) + "_" +
                           std::to_string(
                               std::chrono::steady_clock::now()
                                   .time_since_epoch()
                                   .count())))
                             .string();

    struct WalDirGuard {
        const std::string& path;
        ~WalDirGuard() { std::filesystem::remove_all(path); }
    } guard{wal_dir};

    std::filesystem::create_directories(wal_dir);
    const std::string txn = "wal-txn-conservative-abort";
    {
        WALManagerConfig wal_cfg;
        wal_cfg.wal_directory = wal_dir;
        wal_cfg.sync_on_write = true;
        WALManager wal(wal_cfg);

        WALEntry begin_entry;
        begin_entry.type = WALEntryType::BEGIN_TX;
        begin_entry.transaction_id = txn;
        begin_entry.data = {
            {"transaction_id", txn},
            {"coordinator_id", "coord-wal-abort-test"},
            {"shards", nlohmann::json::array({"shard-abort"})}
        };
        wal.append(begin_entry);
        wal.flush();
    }

    RejectingPhaseTwoParticipant participant(false, true, false);
    TwoPhaseCommitCoordinator::Config cfg;
    cfg.wal_directory = wal_dir;
    cfg.sync_wal_writes = true;
    TwoPhaseCommitCoordinator coord("coord-wal-abort-test", cfg);
    coord.registerParticipant("shard-abort", &participant);

    EXPECT_EQ(coord.recoverInDoubtTransactions(), 0u);
    EXPECT_EQ(coord.getTransactionState(txn), CoordinatorTxnState::ABORT_DECIDED);
    const auto in_doubt = coord.getRecoverableTransactions();
    ASSERT_EQ(in_doubt.size(), 1u);
    EXPECT_TRUE(in_doubt.front().decision_recorded);
    EXPECT_FALSE(in_doubt.front().decision_commit);

    participant.abort_ack = true;
    EXPECT_EQ(coord.recoverInDoubtTransactions(), 1u);
    EXPECT_EQ(coord.getTransactionState(txn), CoordinatorTxnState::COMPLETED);
    EXPECT_EQ(participant.abort_calls, 2);
}

namespace {

class FakeRecoverableCoordinator
    : public themis::transaction::IRecoverableTwoPhaseCoordinator {
public:
    FakeRecoverableCoordinator(std::string name,
                               std::string backend,
                               size_t before,
                               size_t resolved,
                               size_t after)
        : name_(std::move(name))
        , backend_(std::move(backend))
        , before_(before)
        , resolved_(resolved)
        , after_(after) {}

    size_t recoverInDoubtTransactions() override {
        recovered_ = true;
        return resolved_;
    }

    std::string recoveryCoordinatorName() const override { return name_; }
    std::string recoveryBackendName() const override { return backend_; }

    std::vector<themis::transaction::RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const override {
        const auto count = recovered_ ? after_ : before_;
        std::vector<themis::transaction::RecoverableTwoPhaseTransaction> txns;
        txns.reserve(count);
        for (size_t i = 0; i < count; ++i) {
            txns.push_back({
                "txn-" + std::to_string(i),
                themis::transaction::RecoverableTwoPhaseState::PREPARED,
                false,
                false
            });
        }
        return txns;
    }

private:
    std::string name_;
    std::string backend_;
    size_t before_;
    size_t resolved_;
    size_t after_;
    mutable bool recovered_{false};
};

} // namespace

TEST(GlobalTwoPhaseCommitRecoveryManagerTest, AggregatesCrossCoordinatorRecoveryReport) {
    FakeRecoverableCoordinator c1("coord-a", "WAL", 2, 2, 0);
    FakeRecoverableCoordinator c2("coord-b", "WAL/snapshot", 3, 1, 2);

    auto report =
        themis::transaction::GlobalTwoPhaseCommitRecoveryManager::recoverAll(
            {&c1, &c2});

    ASSERT_EQ(report.coordinator_count, 2u);
    EXPECT_EQ(report.in_doubt_before, 5u);
    EXPECT_EQ(report.resolved, 3u);
    EXPECT_EQ(report.in_doubt_after, 2u);
    ASSERT_EQ(report.coordinators.size(), 2u);
    EXPECT_EQ(report.coordinators[0].coordinator_name, "coord-a");
    EXPECT_EQ(report.coordinators[1].backend_name, "WAL/snapshot");
}
