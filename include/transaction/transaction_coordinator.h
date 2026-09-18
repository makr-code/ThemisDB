/**
 * @file transaction_coordinator.h
 * @brief Unified strategy-pattern interface for transaction coordinators.
 *
 * ITransactionCoordinator is the single abstraction that callers use
 * regardless of the underlying commit protocol (2PC, 3PC, SAGA, Calvin).
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// ITransactionCoordinator — Unified Strategy-pattern interface for all
// commit-protocol coordinators in ThemisDB.
//
// Supersedes the ad-hoc coordinator hierarchy and provides a single
// abstraction that callers can use regardless of the underlying protocol
// (2PC, 3PC, SAGA, Percolator, Calvin, or a custom extension).
//
// Design goals
//   - Strategy pattern: coordinators are interchangeable at the call site.
//   - SOLID compliance: ISP via capabilities(), OCP via extension without
//     modification, LSP via the WAL/recovery contract stated in Doxygen.
//   - API mockability: pure-virtual interface, no state, no concrete types
//     in the public contract.
//   - WAL/recovery: every lifecycle method documents its durability
//     contract so recovery managers can rely on consistent guarantees.
//
// Relationship to existing interfaces
//   ITransactionCoordinator
//     └─ IRecoverableTwoPhaseCoordinator    (2PC/3PC)
//     └─ (future) ISagaCoordinator          (SAGA extension point)
//     └─ (future) IPercolatorCoordinator    (Percolator extension point)
//
// Migration path for existing coordinators
//   - TwoPhaseCommitCoordinator    → implements both ITransactionCoordinator
//                                    and IRecoverableTwoPhaseCoordinator
//   - GlobalTransactionManager     → same dual-interface approach
//   - DistributedTransactionManager→ same dual-interface approach
//   - CrossShardTransactionCoordinator → same dual-interface approach
//   See docs/ITRANSACTION_COORDINATOR.md for the phased migration plan.

#pragma once

#include "transaction/isolation_level.h"

#include <chrono>
#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Protocol-agnostic transaction lifecycle state
// ─────────────────────────────────────────────────────────────────────────────

enum class TxnLifecycleState {
    ACTIVE,      ///< Transaction begun; prepare phase not yet started.
    PREPARING,   ///< Prepare phase is in progress; votes being collected.
    PREPARED,    ///< All participant votes collected; no durable final decision yet.
    COMMITTING,  ///< Durable COMMIT decision written; applying to participants.
    ABORTING,    ///< Durable ABORT decision written; compensation in progress.
    COMPLETED,   ///< Terminal: commit or abort fully applied across all participants.
    FAILED,      ///< Terminal: coordinator cannot make further progress automatically.
    UNKNOWN      ///< This coordinator has no record of the given transaction ID.
};

// ─────────────────────────────────────────────────────────────────────────────
// Commit protocol identifier
// ─────────────────────────────────────────────────────────────────────────────

enum class CommitProtocol {
    TWO_PHASE_COMMIT,    ///< Classical blocking 2PC (Gray 1978).
    THREE_PHASE_COMMIT,  ///< Non-blocking 3PC (CanCommit / PreCommit / DoCommit).
    SAGA,                ///< SAGA with per-step compensating actions (Garcia-Molina 1987).
    PERCOLATOR,          ///< Google Percolator optimistic MVCC protocol.
    CALVIN,              ///< Calvin deterministic pre-ordered execution (Thomson 2012).
    CUSTOM               ///< User-defined extension protocol.
};

// ─────────────────────────────────────────────────────────────────────────────
// Capability flags
// ─────────────────────────────────────────────────────────────────────────────

struct CoordinatorCapabilities {
    bool supports_prepare_phase   = false; ///< Explicit Phase-1 vote round (2PC, 3PC).
    bool supports_pre_commit      = false; ///< Phase-2a PreCommit before final commit (3PC).
    bool supports_compensation    = false; ///< Compensating rollback steps (SAGA).
    bool supports_optimistic_mvcc = false; ///< Optimistic MVCC read-then-CAS writes (Percolator).
    bool supports_deterministic   = false; ///< Deterministic pre-ordering before execution (Calvin).
    bool supports_wal_recovery    = false; ///< Coordinator can recover in-doubt txns from WAL.
    bool supports_snapshot_read   = false; ///< Exposes MVCC snapshot read semantics to callers.
};

// ─────────────────────────────────────────────────────────────────────────────
// Operation result
// ─────────────────────────────────────────────────────────────────────────────

struct TxnCoordinatorResult {
    enum class ErrorCode {
        NONE = 0,              ///< Operation succeeded (ok == true).
        UNKNOWN_TRANSACTION,   ///< No active/recoverable transaction with the given ID.
        INVALID_STATE,         ///< Operation is not permitted in the current lifecycle state.
        PARTICIPANT_ABORT,     ///< One or more participants voted ABORT during prepare.
        TIMEOUT,               ///< Operation exceeded its deadline.
        RECOVERY_NEEDED,       ///< Transaction is in-doubt; automatic resolution not possible.
        INTERNAL_ERROR         ///< Coordinator-internal failure (WAL write failed, etc.).
    };

    bool        ok      = true;             ///< True when the operation succeeded.
    ErrorCode   code    = ErrorCode::NONE;  ///< Error category when ok == false.
    std::string message;                    ///< Human-readable diagnostic detail.

    [[nodiscard]] static TxnCoordinatorResult OK() { return {}; }

    [[nodiscard]] static TxnCoordinatorResult Fail(ErrorCode ec, std::string msg) {
        return {false, ec, std::move(msg)};
    }

    [[nodiscard]] explicit operator bool() const noexcept { return ok; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-transaction options
// ─────────────────────────────────────────────────────────────────────────────

struct TxnCoordinatorOptions {
    themis::IsolationLevel isolation = themis::IsolationLevel::READ_COMMITTED;

    std::chrono::system_clock::time_point deadline{};

    std::string metadata;
};

// ─────────────────────────────────────────────────────────────────────────────
// In-doubt transaction descriptor
// ─────────────────────────────────────────────────────────────────────────────

struct InDoubtTxnDescriptor {
    std::string txn_id;                    ///< Coordinator-local transaction identifier.
    bool        prepare_logged  = false;   ///< True when a durable PREPARE record exists.
    bool        commit_decided  = false;   ///< True when a durable COMMIT decision exists.
    std::string detail;                    ///< Optional coordinator-provided diagnostic.
};

// ─────────────────────────────────────────────────────────────────────────────
// ITransactionCoordinator
// ─────────────────────────────────────────────────────────────────────────────

class ITransactionCoordinator {
public:
    /**
     * @brief ITransaction Coordinator.
     * @return Return value.
     */
    virtual ~ITransactionCoordinator() = default;

    // ─── Protocol introspection ───────────────────────────────────────────

    [[nodiscard]] virtual CommitProtocol protocolType() const noexcept = 0;

    [[nodiscard]] virtual std::string_view protocolName() const noexcept = 0;

    [[nodiscard]] virtual CoordinatorCapabilities capabilities() const noexcept = 0;

    // ─── Transaction lifecycle ────────────────────────────────────────────

    [[nodiscard]] virtual TxnCoordinatorResult begin(
       std::string_view             txn_id,
        const TxnCoordinatorOptions& opts = {}
    ) = 0;

    [[nodiscard]] virtual TxnCoordinatorResult prepare(
       std::string_view txn_id
    ) = 0;

    [[nodiscard]] virtual TxnCoordinatorResult commit(
       std::string_view txn_id
    ) = 0;

    [[nodiscard]] virtual TxnCoordinatorResult abort(
       std::string_view txn_id
    ) = 0;

    // ─── State query ──────────────────────────────────────────────────────

    [[nodiscard]] virtual TxnLifecycleState getState(
        std::string_view txn_id
    ) const = 0;

    // ─── WAL / Recovery ───────────────────────────────────────────────────

    [[nodiscard]] virtual std::size_t recoverInDoubt() = 0;

    [[nodiscard]] virtual std::vector<InDoubtTxnDescriptor>
    getInDoubtTransactions() const = 0;

    // ─── Non-copyable, movable ────────────────────────────────────────────

    ITransactionCoordinator(const ITransactionCoordinator&)             = delete;
    ITransactionCoordinator& operator=(const ITransactionCoordinator&)  = delete;
    ITransactionCoordinator(ITransactionCoordinator&&)                  noexcept = default;
    ITransactionCoordinator& operator=(ITransactionCoordinator&&)       noexcept = default;

protected:
    ITransactionCoordinator() = default;
};

} // namespace themis::transaction
