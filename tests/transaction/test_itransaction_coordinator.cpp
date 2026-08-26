// Tests for the ITransactionCoordinator interface (Issue #5374).
//
// This file verifies:
//   ITC-1   CommitProtocol enum values are all distinct
//   ITC-2   CoordinatorCapabilities defaults are all false
//   ITC-3   TxnCoordinatorResult::OK() is success, operator bool() is true
//   ITC-4   TxnCoordinatorResult::Fail() carries code and message
//   ITC-5   TxnCoordinatorResult::operator bool() is false on failure
//   ITC-6   TxnCoordinatorOptions defaults (isolation READ_COMMITTED)
//   ITC-7   InDoubtTxnDescriptor defaults (prepare_logged/commit_decided false)
//   ITC-8   Mock 2PC coordinator: full lifecycle begin→prepare→commit
//   ITC-9   Mock 2PC coordinator: begin→abort path
//   ITC-10  Mock 2PC coordinator: prepare returns PARTICIPANT_ABORT on any-ABORT
//   ITC-11  Mock SAGA coordinator: prepare() is a no-op returning OK
//   ITC-12  Mock Percolator coordinator: capabilities flags
//   ITC-13  Mock Calvin coordinator: capabilities flags
//   ITC-14  MockCoordinator: getState() returns UNKNOWN for unknown txn_id
//   ITC-15  MockCoordinator: recoverInDoubt() returns 0 when nothing in-doubt
//   ITC-16  MockCoordinator: getInDoubtTransactions() returns empty vector
//   ITC-17  Duplicate txn_id: begin() returns INVALID_STATE
//   ITC-18  Commit unknown txn: returns UNKNOWN_TRANSACTION
//   ITC-19  Abort unknown txn: returns UNKNOWN_TRANSACTION
//   ITC-20  Prepare unknown txn: returns UNKNOWN_TRANSACTION
//   ITC-21  Options: isolation level is forwarded correctly
//   ITC-22  Options: metadata string is forwarded correctly
//   ITC-23  Capabilities: 2PC sets supports_prepare_phase and supports_wal_recovery
//   ITC-24  Capabilities: SAGA sets supports_compensation
//   ITC-25  Capabilities: Percolator sets supports_optimistic_mvcc
//   ITC-26  Capabilities: Calvin sets supports_deterministic
//   ITC-27  Capabilities: 3PC sets supports_pre_commit
//   ITC-28  TxnCoordinatorResult: NONE code on success
//   ITC-29  TxnCoordinatorResult: all ErrorCode enumerators compile
//   ITC-30  getInDoubtTransactions: returns descriptors for in-doubt txns
//   ITC-31  recoverInDoubt: returns count of resolved transactions
//   ITC-32  getState: returns ACTIVE for active transaction
//   ITC-33  getState: returns PREPARED after successful prepare()
//   ITC-34  getState: returns COMPLETED after commit()
//   ITC-35  getState: returns COMPLETED after abort()
//   ITC-36  Coordinator is non-copyable
//   ITC-37  ITransactionCoordinator is pure-virtual (no concrete state)
//   ITC-38  protocolName() returns stable, non-empty string
//   ITC-39  protocolType() matches protocolName() for all built-in protocols
//   ITC-40  2PC: commit() before prepare() returns INVALID_STATE
//   ITC-41  getInDoubtTransactions: commit_decided = true for COMMITTING state

#include "transaction/transaction_coordinator.h"

#include <gtest/gtest.h>
#include <chrono>
#include <memory>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

using namespace themis::transaction;
using namespace std::string_literals;

// ─────────────────────────────────────────────────────────────────────────────
// Minimal stateful mock coordinator for lifecycle tests
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Minimal in-memory mock of ITransactionCoordinator.
 *
 * Records method invocations and models a simple state machine so that
 * the interface contract tests can assert on real state transitions.
 *
 * This mock represents a configurable 2PC coordinator for most tests;
 * its protocol flags can be overridden to simulate other protocols.
 *
 * @note Not thread-safe — designed for single-threaded unit tests only.
 *       The ITransactionCoordinator interface requires thread safety;
 *       production implementations must add synchronisation.
 */
class StatefulMockCoordinator : public ITransactionCoordinator {
public:
    /// Construction-time configuration.
    struct Config {
        CommitProtocol     protocol         = CommitProtocol::TWO_PHASE_COMMIT;
        std::string        name             = "2PC-mock";  ///< Owned string — no lifetime hazard.
        CoordinatorCapabilities caps{
            /* supports_prepare_phase  */ true,
            /* supports_pre_commit     */ false,
            /* supports_compensation   */ false,
            /* supports_optimistic_mvcc*/ false,
            /* supports_deterministic  */ false,
            /* supports_wal_recovery   */ true,
            /* supports_snapshot_read  */ false
        };
        bool prepare_returns_abort = false; ///< Simulate a participant ABORT vote.
    };

    explicit StatefulMockCoordinator(Config cfg = Config{}) : cfg_(std::move(cfg)) {}

    // ─── Protocol introspection ───────────────────────────────────────────

    CommitProtocol protocolType() const noexcept override {
        return cfg_.protocol;
    }

    std::string_view protocolName() const noexcept override {
        return cfg_.name;
    }

    CoordinatorCapabilities capabilities() const noexcept override {
        return cfg_.caps;
    }

    // ─── Lifecycle ────────────────────────────────────────────────────────

    TxnCoordinatorResult begin(
        std::string_view txn_id,
        const TxnCoordinatorOptions& opts = {}
    ) override {
        if (txn_id.empty()) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::INVALID_STATE,
                "txn_id must not be empty");
        }
        const std::string key{txn_id};
        if (states_.count(key)) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::INVALID_STATE,
                "duplicate txn_id: " + key);
        }
        states_[key] = TxnLifecycleState::ACTIVE;
        options_[key] = opts;
        return TxnCoordinatorResult::OK();
    }

    TxnCoordinatorResult prepare(std::string_view txn_id) override {
        const std::string key{txn_id};
        if (!states_.count(key)) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::UNKNOWN_TRANSACTION,
                "unknown txn: " + key);
        }
        // Non-voting protocols: no-op — state must not advance.
        if (!cfg_.caps.supports_prepare_phase) {
            return TxnCoordinatorResult::OK();
        }
        if (cfg_.prepare_returns_abort) {
            states_[key] = TxnLifecycleState::ABORTING;
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::PARTICIPANT_ABORT,
                "participant voted ABORT");
        }
        states_[key] = TxnLifecycleState::PREPARED;
        return TxnCoordinatorResult::OK();
    }

    TxnCoordinatorResult commit(std::string_view txn_id) override {
        const std::string key{txn_id};
        if (!states_.count(key)) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::UNKNOWN_TRANSACTION,
                "unknown txn: " + key);
        }
        // Enforce state precondition for voting protocols.
        if (cfg_.caps.supports_prepare_phase &&
            states_[key] != TxnLifecycleState::PREPARED) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::INVALID_STATE,
                "voting protocol: commit() requires PREPARED state");
        }
        states_[key] = TxnLifecycleState::COMPLETED;
        return TxnCoordinatorResult::OK();
    }

    TxnCoordinatorResult abort(std::string_view txn_id) override {
        const std::string key{txn_id};
        if (!states_.count(key)) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::UNKNOWN_TRANSACTION,
                "unknown txn: " + key);
        }
        auto& s = states_[key];
        if (s == TxnLifecycleState::COMPLETED) {
            return TxnCoordinatorResult::Fail(
                TxnCoordinatorResult::ErrorCode::INVALID_STATE,
                "txn already completed");
        }
        s = TxnLifecycleState::COMPLETED;
        return TxnCoordinatorResult::OK();
    }

    // ─── State query ──────────────────────────────────────────────────────

    TxnLifecycleState getState(std::string_view txn_id) const override {
        auto it = states_.find(std::string{txn_id});
        if (it == states_.end()) {
            return TxnLifecycleState::UNKNOWN;
        }
        return it->second;
    }

    // ─── Recovery ─────────────────────────────────────────────────────────

    std::size_t recoverInDoubt() override {
        std::size_t resolved = 0;
        for (auto& [id, state] : states_) {
            if (state == TxnLifecycleState::PREPARED ||
                state == TxnLifecycleState::PREPARING) {
                state = TxnLifecycleState::COMPLETED;
                ++resolved;
            }
        }
        return resolved;
    }

    std::vector<InDoubtTxnDescriptor> getInDoubtTransactions() const override {
        std::vector<InDoubtTxnDescriptor> result;
        for (const auto& [id, state] : states_) {
            if (state == TxnLifecycleState::COMPLETED ||
                state == TxnLifecycleState::FAILED) {
                continue;
            }
            InDoubtTxnDescriptor desc;
            desc.txn_id         = id;
            desc.prepare_logged = (state == TxnLifecycleState::PREPARED ||
                                   state == TxnLifecycleState::COMMITTING ||
                                   state == TxnLifecycleState::ABORTING);
            desc.commit_decided = (state == TxnLifecycleState::COMMITTING);
            result.push_back(std::move(desc));
        }
        return result;
    }

    // ─── Test helpers ─────────────────────────────────────────────────────

    const TxnCoordinatorOptions& storedOptions(std::string_view txn_id) const {
        return options_.at(std::string{txn_id});
    }

    /// Inject an in-doubt transaction directly (simulates coordinator restart after prepare).
    void injectPreparedTxn(std::string_view txn_id) {
        states_[std::string{txn_id}] = TxnLifecycleState::PREPARED;
    }

    /// Inject a transaction in COMMITTING state (durable commit decision written).
    void injectCommittingTxn(std::string_view txn_id) {
        states_[std::string{txn_id}] = TxnLifecycleState::COMMITTING;
    }

private:
    Config                                                    cfg_;
    std::unordered_map<std::string, TxnLifecycleState>        states_;
    std::unordered_map<std::string, TxnCoordinatorOptions>    options_;
};

// ─────────────────────────────────────────────────────────────────────────────
// ITC-1: CommitProtocol enum values are all distinct
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC01_CommitProtocolEnumValues) {
    EXPECT_NE(CommitProtocol::TWO_PHASE_COMMIT,   CommitProtocol::THREE_PHASE_COMMIT);
    EXPECT_NE(CommitProtocol::THREE_PHASE_COMMIT, CommitProtocol::SAGA);
    EXPECT_NE(CommitProtocol::SAGA,               CommitProtocol::PERCOLATOR);
    EXPECT_NE(CommitProtocol::PERCOLATOR,         CommitProtocol::CALVIN);
    EXPECT_NE(CommitProtocol::CALVIN,             CommitProtocol::CUSTOM);
    EXPECT_NE(CommitProtocol::TWO_PHASE_COMMIT,   CommitProtocol::CUSTOM);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-2: CoordinatorCapabilities defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC02_CapabilitiesDefaultsFalse) {
    CoordinatorCapabilities caps{};
    EXPECT_FALSE(caps.supports_prepare_phase);
    EXPECT_FALSE(caps.supports_pre_commit);
    EXPECT_FALSE(caps.supports_compensation);
    EXPECT_FALSE(caps.supports_optimistic_mvcc);
    EXPECT_FALSE(caps.supports_deterministic);
    EXPECT_FALSE(caps.supports_wal_recovery);
    EXPECT_FALSE(caps.supports_snapshot_read);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-3: TxnCoordinatorResult::OK()
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC03_ResultOk) {
    auto r = TxnCoordinatorResult::OK();
    EXPECT_TRUE(r.ok);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::NONE);
    EXPECT_TRUE(r.message.empty());
    EXPECT_TRUE(static_cast<bool>(r));
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-4 / ITC-5: TxnCoordinatorResult::Fail()
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC04_05_ResultFail) {
    auto r = TxnCoordinatorResult::Fail(
        TxnCoordinatorResult::ErrorCode::PARTICIPANT_ABORT,
        "shard-3 voted ABORT");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::PARTICIPANT_ABORT);
    EXPECT_EQ(r.message, "shard-3 voted ABORT");
    EXPECT_FALSE(static_cast<bool>(r));   // ITC-5
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-6: TxnCoordinatorOptions defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC06_OptionsDefaults) {
    TxnCoordinatorOptions opts{};
    EXPECT_EQ(opts.isolation, themis::IsolationLevel::READ_COMMITTED);
    EXPECT_EQ(opts.deadline,  std::chrono::system_clock::time_point{});
    EXPECT_TRUE(opts.metadata.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-7: InDoubtTxnDescriptor defaults
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC07_InDoubtDescriptorDefaults) {
    InDoubtTxnDescriptor desc{};
    EXPECT_TRUE(desc.txn_id.empty());
    EXPECT_FALSE(desc.prepare_logged);
    EXPECT_FALSE(desc.commit_decided);
    EXPECT_TRUE(desc.detail.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-8: Mock 2PC full commit lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC08_TwoPCCommitLifecycle) {
    StatefulMockCoordinator coord;

    auto r1 = coord.begin("txn-1");
    EXPECT_TRUE(r1) << r1.message;
    EXPECT_EQ(coord.getState("txn-1"), TxnLifecycleState::ACTIVE);

    auto r2 = coord.prepare("txn-1");
    EXPECT_TRUE(r2) << r2.message;
    EXPECT_EQ(coord.getState("txn-1"), TxnLifecycleState::PREPARED);

    auto r3 = coord.commit("txn-1");
    EXPECT_TRUE(r3) << r3.message;
    EXPECT_EQ(coord.getState("txn-1"), TxnLifecycleState::COMPLETED);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-9: Mock 2PC abort lifecycle
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC09_TwoPCAbortLifecycle) {
    StatefulMockCoordinator coord;

    ASSERT_TRUE(coord.begin("txn-2"));
    ASSERT_TRUE(coord.prepare("txn-2"));

    auto r = coord.abort("txn-2");
    EXPECT_TRUE(r) << r.message;
    EXPECT_EQ(coord.getState("txn-2"), TxnLifecycleState::COMPLETED);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-10: prepare returns PARTICIPANT_ABORT
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC10_PrepareParticipantAbort) {
    StatefulMockCoordinator::Config cfg;
    cfg.prepare_returns_abort = true;
    StatefulMockCoordinator coord(cfg);

    ASSERT_TRUE(coord.begin("txn-3"));
    auto r = coord.prepare("txn-3");
    EXPECT_FALSE(r);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::PARTICIPANT_ABORT);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-11: SAGA prepare() is a no-op returning OK
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC11_SagaPrepareIsNoOp) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::SAGA;
    cfg.name     = "SAGA-mock";
    cfg.caps     = {};   // no prepare phase
    cfg.caps.supports_compensation = true;
    cfg.caps.supports_wal_recovery = true;

    StatefulMockCoordinator coord(cfg);
    ASSERT_TRUE(coord.begin("saga-1"));

    auto r = coord.prepare("saga-1");
    EXPECT_TRUE(r) << "SAGA prepare() must be a no-op returning OK";
    // State must not move to PREPARED for SAGA
    EXPECT_EQ(coord.getState("saga-1"), TxnLifecycleState::ACTIVE);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-12: Percolator capabilities
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC12_PercolatorCapabilities) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::PERCOLATOR;
    cfg.name     = "Percolator-mock";
    cfg.caps     = {};
    cfg.caps.supports_optimistic_mvcc = true;
    cfg.caps.supports_snapshot_read   = true;
    cfg.caps.supports_wal_recovery    = true;

    StatefulMockCoordinator coord(cfg);
    auto caps = coord.capabilities();
    EXPECT_TRUE(caps.supports_optimistic_mvcc);
    EXPECT_TRUE(caps.supports_snapshot_read);
    EXPECT_FALSE(caps.supports_prepare_phase);
    EXPECT_FALSE(caps.supports_compensation);
    EXPECT_FALSE(caps.supports_deterministic);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-13: Calvin capabilities
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC13_CalvinCapabilities) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::CALVIN;
    cfg.name     = "Calvin-mock";
    cfg.caps     = {};
    cfg.caps.supports_deterministic = true;
    cfg.caps.supports_wal_recovery  = true;

    StatefulMockCoordinator coord(cfg);
    auto caps = coord.capabilities();
    EXPECT_TRUE(caps.supports_deterministic);
    EXPECT_FALSE(caps.supports_prepare_phase);
    EXPECT_FALSE(caps.supports_compensation);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-14: getState() returns UNKNOWN for unknown txn_id
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC14_GetStateUnknown) {
    StatefulMockCoordinator coord;
    EXPECT_EQ(coord.getState("no-such-txn"), TxnLifecycleState::UNKNOWN);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-15: recoverInDoubt() returns 0 when nothing in-doubt
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC15_RecoverInDoubtEmpty) {
    StatefulMockCoordinator coord;
    EXPECT_EQ(coord.recoverInDoubt(), 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-16: getInDoubtTransactions() returns empty vector when clean
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC16_GetInDoubtEmpty) {
    StatefulMockCoordinator coord;
    EXPECT_TRUE(coord.getInDoubtTransactions().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-17: Duplicate txn_id returns INVALID_STATE
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC17_DuplicateTxnIdInvalidState) {
    StatefulMockCoordinator coord;
    ASSERT_TRUE(coord.begin("dup-1"));
    auto r = coord.begin("dup-1");
    EXPECT_FALSE(r);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::INVALID_STATE);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-18: commit() on unknown txn returns UNKNOWN_TRANSACTION
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC18_CommitUnknownTxn) {
    StatefulMockCoordinator coord;
    auto r = coord.commit("not-started");
    EXPECT_FALSE(r);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::UNKNOWN_TRANSACTION);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-19: abort() on unknown txn returns UNKNOWN_TRANSACTION
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC19_AbortUnknownTxn) {
    StatefulMockCoordinator coord;
    auto r = coord.abort("not-started");
    EXPECT_FALSE(r);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::UNKNOWN_TRANSACTION);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-20: prepare() on unknown txn returns UNKNOWN_TRANSACTION
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC20_PrepareUnknownTxn) {
    StatefulMockCoordinator coord;
    auto r = coord.prepare("not-started");
    EXPECT_FALSE(r);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::UNKNOWN_TRANSACTION);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-21: Options isolation level forwarded correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC21_OptionsIsolationForwarded) {
    StatefulMockCoordinator coord;
    TxnCoordinatorOptions opts;
    opts.isolation = themis::IsolationLevel::SERIALIZABLE;

    ASSERT_TRUE(coord.begin("opts-1", opts));
    EXPECT_EQ(coord.storedOptions("opts-1").isolation,
              themis::IsolationLevel::SERIALIZABLE);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-22: Options metadata forwarded correctly
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC22_OptionsMetadataForwarded) {
    StatefulMockCoordinator coord;
    TxnCoordinatorOptions opts;
    opts.metadata = "session=xyz789";

    ASSERT_TRUE(coord.begin("opts-2", opts));
    EXPECT_EQ(coord.storedOptions("opts-2").metadata, "session=xyz789");
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-23: 2PC capabilities flags
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC23_TwoPCCapabilities) {
    StatefulMockCoordinator coord; // default is 2PC with wal_recovery
    auto caps = coord.capabilities();
    EXPECT_TRUE(caps.supports_prepare_phase);
    EXPECT_TRUE(caps.supports_wal_recovery);
    EXPECT_FALSE(caps.supports_pre_commit);
    EXPECT_FALSE(caps.supports_compensation);
    EXPECT_FALSE(caps.supports_optimistic_mvcc);
    EXPECT_FALSE(caps.supports_deterministic);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-24: SAGA capabilities flags
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC24_SagaCapabilities) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::SAGA;
    cfg.caps     = {};
    cfg.caps.supports_compensation = true;
    cfg.caps.supports_wal_recovery = true;

    StatefulMockCoordinator coord(cfg);
    auto caps = coord.capabilities();
    EXPECT_TRUE(caps.supports_compensation);
    EXPECT_FALSE(caps.supports_prepare_phase);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-25: Percolator capabilities (repeated for clarity)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC25_PercolatorOptimisticMvcc) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::PERCOLATOR;
    cfg.caps     = {};
    cfg.caps.supports_optimistic_mvcc = true;

    StatefulMockCoordinator coord(cfg);
    EXPECT_TRUE(coord.capabilities().supports_optimistic_mvcc);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-26: Calvin capabilities (repeated for clarity)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC26_CalvinDeterministic) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::CALVIN;
    cfg.caps     = {};
    cfg.caps.supports_deterministic = true;

    StatefulMockCoordinator coord(cfg);
    EXPECT_TRUE(coord.capabilities().supports_deterministic);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-27: 3PC capabilities
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC27_ThreePCPreCommit) {
    StatefulMockCoordinator::Config cfg;
    cfg.protocol = CommitProtocol::THREE_PHASE_COMMIT;
    cfg.caps     = {};
    cfg.caps.supports_prepare_phase = true;
    cfg.caps.supports_pre_commit    = true;
    cfg.caps.supports_wal_recovery  = true;

    StatefulMockCoordinator coord(cfg);
    EXPECT_TRUE(coord.capabilities().supports_pre_commit);
    EXPECT_TRUE(coord.capabilities().supports_prepare_phase);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-28: Success result has NONE error code
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC28_SuccessResultCodeNone) {
    auto r = TxnCoordinatorResult::OK();
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::NONE);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-29: All ErrorCode enumerators compile and are distinct
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC29_ErrorCodeEnumeratorsCompile) {
    using EC = TxnCoordinatorResult::ErrorCode;
    // Verify all 7 codes are defined and distinct
    std::vector<EC> codes = {
        EC::NONE,
        EC::UNKNOWN_TRANSACTION,
        EC::INVALID_STATE,
        EC::PARTICIPANT_ABORT,
        EC::TIMEOUT,
        EC::RECOVERY_NEEDED,
        EC::INTERNAL_ERROR
    };
    EXPECT_EQ(codes.size(), 7u);
    for (std::size_t i = 0; i < codes.size(); ++i) {
        for (std::size_t j = i + 1; j < codes.size(); ++j) {
            EXPECT_NE(codes[i], codes[j])
                << "ErrorCode enumerators at index " << i << " and " << j << " are equal";
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-30: getInDoubtTransactions returns descriptors for in-doubt txns
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC30_GetInDoubtDescriptors) {
    StatefulMockCoordinator coord;
    coord.injectPreparedTxn("indoubt-1");
    coord.injectPreparedTxn("indoubt-2");

    auto descs = coord.getInDoubtTransactions();
    ASSERT_EQ(descs.size(), 2u);
    // Both have prepare_logged = true
    for (const auto& d : descs) {
        EXPECT_TRUE(d.prepare_logged);
        EXPECT_FALSE(d.commit_decided);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-31: recoverInDoubt() returns count of resolved transactions
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC31_RecoverInDoubtCount) {
    StatefulMockCoordinator coord;
    coord.injectPreparedTxn("recover-1");
    coord.injectPreparedTxn("recover-2");

    EXPECT_EQ(coord.recoverInDoubt(), 2u);
    EXPECT_EQ(coord.recoverInDoubt(), 0u); // idempotent: none left
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-32 / ITC-33 / ITC-34 / ITC-35: State transitions
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC32_GetStateActive) {
    StatefulMockCoordinator coord;
    ASSERT_TRUE(coord.begin("state-1"));
    EXPECT_EQ(coord.getState("state-1"), TxnLifecycleState::ACTIVE);
}

TEST(ITransactionCoordinator, ITC33_GetStatePrepared) {
    StatefulMockCoordinator coord;
    ASSERT_TRUE(coord.begin("state-2"));
    ASSERT_TRUE(coord.prepare("state-2"));
    EXPECT_EQ(coord.getState("state-2"), TxnLifecycleState::PREPARED);
}

TEST(ITransactionCoordinator, ITC34_GetStateCompletedAfterCommit) {
    StatefulMockCoordinator coord;
    ASSERT_TRUE(coord.begin("state-3"));
    ASSERT_TRUE(coord.prepare("state-3"));
    ASSERT_TRUE(coord.commit("state-3"));
    EXPECT_EQ(coord.getState("state-3"), TxnLifecycleState::COMPLETED);
}

TEST(ITransactionCoordinator, ITC35_GetStateCompletedAfterAbort) {
    StatefulMockCoordinator coord;
    ASSERT_TRUE(coord.begin("state-4"));
    ASSERT_TRUE(coord.abort("state-4"));
    EXPECT_EQ(coord.getState("state-4"), TxnLifecycleState::COMPLETED);
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-36: ITransactionCoordinator is non-copyable
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC36_NonCopyable) {
    EXPECT_FALSE(std::is_copy_constructible_v<StatefulMockCoordinator>)
        << "Coordinator must not be copy-constructible";
    EXPECT_FALSE(std::is_copy_assignable_v<StatefulMockCoordinator>)
        << "Coordinator must not be copy-assignable";
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-37: ITransactionCoordinator is abstract (pure virtual)
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC37_InterfaceIsAbstract) {
    EXPECT_TRUE(std::is_abstract_v<ITransactionCoordinator>)
        << "ITransactionCoordinator must be abstract (pure virtual)";
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-38: protocolName() returns stable, non-empty string
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC38_ProtocolNameNonEmpty) {
    StatefulMockCoordinator coord;
    EXPECT_FALSE(coord.protocolName().empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-39: protocolType() matches protocolName() for built-in protocols
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC39_ProtocolTypeMatchesName) {
    struct Case {
        CommitProtocol protocol;
        std::string_view expected_name;
    };
    const std::vector<Case> cases = {
        {CommitProtocol::TWO_PHASE_COMMIT,    "2PC-mock"},
        {CommitProtocol::SAGA,                "SAGA-mock"},
        {CommitProtocol::PERCOLATOR,          "Percolator-mock"},
        {CommitProtocol::CALVIN,              "Calvin-mock"},
        {CommitProtocol::THREE_PHASE_COMMIT,  "3PC-mock"},
    };

    for (const auto& c : cases) {
        StatefulMockCoordinator::Config cfg;
        cfg.protocol = c.protocol;
        cfg.name     = c.expected_name;
        StatefulMockCoordinator coord(cfg);
        EXPECT_EQ(coord.protocolType(), c.protocol);
        EXPECT_EQ(coord.protocolName(), c.expected_name);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-40: 2PC commit() before prepare() returns INVALID_STATE
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC40_TwoPCCommitBeforePrepareInvalidState) {
    StatefulMockCoordinator coord; // default is 2PC (supports_prepare_phase = true)

    ASSERT_TRUE(coord.begin("early-commit"));
    // State is ACTIVE; commit() without prior prepare() must be rejected.
    auto r = coord.commit("early-commit");
    EXPECT_FALSE(r);
    EXPECT_EQ(r.code, TxnCoordinatorResult::ErrorCode::INVALID_STATE)
        << "2PC commit() before prepare() must return INVALID_STATE; msg: " << r.message;
}

// ─────────────────────────────────────────────────────────────────────────────
// ITC-41: getInDoubtTransactions() sets commit_decided = true for COMMITTING txns
// ─────────────────────────────────────────────────────────────────────────────

TEST(ITransactionCoordinator, ITC41_CommitDecidedTrueForCommitting) {
    StatefulMockCoordinator coord;
    coord.injectPreparedTxn("indoubt-prepared");   // prepare_logged=true, commit_decided=false
    coord.injectCommittingTxn("indoubt-committing"); // prepare_logged=true, commit_decided=true

    auto descs = coord.getInDoubtTransactions();
    ASSERT_EQ(descs.size(), 2u);

    bool found_prepared   = false;
    bool found_committing = false;
    for (const auto& d : descs) {
        if (d.txn_id == "indoubt-prepared") {
            EXPECT_TRUE(d.prepare_logged);
            EXPECT_FALSE(d.commit_decided) << "PREPARED: commit_decided must be false";
            found_prepared = true;
        } else if (d.txn_id == "indoubt-committing") {
            EXPECT_TRUE(d.prepare_logged);
            EXPECT_TRUE(d.commit_decided) << "COMMITTING: commit_decided must be true";
            found_committing = true;
        }
    }
    EXPECT_TRUE(found_prepared)   << "Expected descriptor for indoubt-prepared";
    EXPECT_TRUE(found_committing) << "Expected descriptor for indoubt-committing";
}
