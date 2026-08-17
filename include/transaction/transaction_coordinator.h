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

/**
 * @brief Canonical protocol-agnostic lifecycle state for any transaction.
 *
 * Used as the return type of ITransactionCoordinator::getState() so that
 * recovery managers and monitoring tools can inspect transaction state
 * uniformly across all commit protocols (2PC, 3PC, SAGA, Percolator, Calvin).
 *
 * Mapping guidance for protocol implementors:
 * | Internal protocol state      | TxnLifecycleState  |
 * |------------------------------|--------------------|
 * | Not yet started / pre-begin  | UNKNOWN            |
 * | Begin recorded, no prepare   | ACTIVE             |
 * | Prepare vote in flight       | PREPARING          |
 * | All votes collected (commit) | PREPARED           |
 * | Commit decision durable      | COMMITTING         |
 * | Abort decision durable       | ABORTING           |
 * | Terminal (commit or abort)   | COMPLETED          |
 * | Recovery cannot proceed      | FAILED             |
 * | Not known to coordinator     | UNKNOWN            |
 *
 * Single-round protocols (SAGA, Percolator, Calvin) SHOULD use ACTIVE for the
 * in-flight state and COMPLETED for the terminal state, skipping PREPARING /
 * PREPARED / COMMITTING / ABORTING unless their internal model maps naturally
 * to those states.
 */
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

/**
 * @brief Canonical commit protocol identifier.
 *
 * Returned by ITransactionCoordinator::protocolType() so that callers can
 * branch on the protocol family without a dynamic_cast.
 */
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

/**
 * @brief Optional capability flags advertised by a coordinator instance.
 *
 * Callers SHOULD check the relevant flag before invoking operations that
 * are optional for some protocols.  All flags default to false.
 *
 * @note Capabilities are instance-level: two coordinators of the same
 *       protocol type may advertise different capabilities depending on
 *       their configuration (e.g. WAL enabled/disabled).
 */
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

/**
 * @brief Result type returned by all ITransactionCoordinator lifecycle methods.
 *
 * Carries a success flag, a protocol-independent error code, and a
 * human-readable message.  Protocol-specific detail should be conveyed via
 * the message field or a derived result type.
 */
struct TxnCoordinatorResult {
    /**
     * @brief Protocol-independent error categories.
     *
     * Callers switch on this to decide recovery strategy; they do not need
     * to parse the message string.
     */
    enum class ErrorCode {
        NONE = 0,              ///< Operation succeeded (ok == true).
        UNKNOWN_TRANSACTION,   ///< No active/recoverable transaction with the given ID.
        INVALID_STATE,         ///< Operation is not permitted in the current lifecycle state.
        PARTICIPANT_ABORT,     ///< One or more participants voted ABORT during prepare.
        TIMEOUT,               ///< Operation exceeded its deadline.
        RECOVERY_NEEDED,       ///< Transaction is in-doubt; automatic resolution not possible.
                               ///  Returned by getState()-adjacent operations when the coordinator
                               ///  detects a durably prepared transaction but cannot resolve it
                               ///  without a WAL-replay step (e.g. WAL unavailable at query time).
                               ///  Callers SHOULD trigger recoverInDoubt() and then retry.
        INTERNAL_ERROR         ///< Coordinator-internal failure (WAL write failed, etc.).
    };

    bool        ok      = true;             ///< True when the operation succeeded.
    ErrorCode   code    = ErrorCode::NONE;  ///< Error category when ok == false.
    std::string message;                    ///< Human-readable diagnostic detail.

    /** @brief Construct a successful result. */
    [[nodiscard]] static TxnCoordinatorResult OK() { return {}; }

    /**
     * @brief Construct a failure result.
     * @param ec   Error category.
     * @param msg  Human-readable diagnostic message.
     */
    [[nodiscard]] static TxnCoordinatorResult Fail(ErrorCode ec, std::string msg) {
        return {false, ec, std::move(msg)};
    }

    /** @brief Implicit bool conversion: true on success. */
    [[nodiscard]] explicit operator bool() const noexcept { return ok; }
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-transaction options
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Per-transaction options supplied to ITransactionCoordinator::begin().
 *
 * All fields are optional.  Coordinators apply their own defaults for
 * unset fields and silently ignore options unsupported by their protocol
 * (see CoordinatorCapabilities for what a given protocol supports).
 */
struct TxnCoordinatorOptions {
    /// Desired isolation level.  Default: READ_COMMITTED.
    themis::IsolationLevel isolation = themis::IsolationLevel::READ_COMMITTED;

    /// Wall-clock deadline for the entire transaction lifecycle.
    /// A default-constructed time_point (epoch) means no deadline.
    std::chrono::system_clock::time_point deadline{};

    /// Opaque metadata forwarded verbatim to WAL entries and audit records.
    /// May be empty.
    std::string metadata;
};

// ─────────────────────────────────────────────────────────────────────────────
// In-doubt transaction descriptor
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Minimal descriptor for a transaction that has not yet reached a
 *        terminal state and may require recovery action.
 *
 * Returned by ITransactionCoordinator::getInDoubtTransactions() to give
 * recovery managers a coordinator-independent view of in-doubt state.
 */
struct InDoubtTxnDescriptor {
    std::string txn_id;                    ///< Coordinator-local transaction identifier.
    bool        prepare_logged  = false;   ///< True when a durable PREPARE record exists.
    bool        commit_decided  = false;   ///< True when a durable COMMIT decision exists.
    std::string detail;                    ///< Optional coordinator-provided diagnostic.
};

// ─────────────────────────────────────────────────────────────────────────────
// ITransactionCoordinator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Unified Strategy-pattern interface for all commit-protocol coordinators.
 *
 * @par Purpose
 * ThemisDB ships five commit protocols (2PC, 3PC, SAGA, Percolator, Calvin)
 * each with its own coordinator class and its own lifecycle API.  This
 * interface provides a single abstraction so that callers — connection
 * handlers, query planners, test harnesses — are decoupled from the concrete
 * protocol choice.  The coordinator is selected at construction time (or
 * injected via DI); callers use only this interface thereafter.
 *
 * @par Strategy pattern
 * @code
 * // Injection at construction time:
 * std::unique_ptr<ITransactionCoordinator> coord =
 *     CoordinatorFactory::make(CommitProtocol::TWO_PHASE_COMMIT, config);
 *
 * coord->begin("txn-42");
 * coord->prepare("txn-42");   // no-op for SAGA / Percolator / Calvin
 * coord->commit("txn-42");
 * @endcode
 *
 * @par Protocol-specific behaviour of optional methods
 * Some lifecycle methods are no-ops for single-round protocols:
 * | Protocol   | prepare()     | Notes                                       |
 * |------------|---------------|---------------------------------------------|
 * | 2PC        | Phase-1 vote  | Returns PARTICIPANT_ABORT on any ABORT vote |
 * | 3PC        | CanCommit     | Uses 3-round flow; PreCommit implicit        |
 * | SAGA       | No-op → OK    | No distributed voting; uses compensation     |
 * | Percolator | No-op → OK    | Single prewrite+commit round                |
 * | Calvin     | No-op → OK    | Deterministic; no voting needed              |
 *
 * Callers SHOULD consult capabilities() before depending on optional behaviour.
 *
 * @par WAL / Recovery contract
 * Implementations MUST honour the following durability guarantees:
 *  - **begin()**: Writes a durable BEGIN record before returning OK.
 *  - **prepare()**: Writes a durable PREPARE record for every participant
 *    vote before returning OK.  After a successful prepare(), a coordinator
 *    restart MUST be able to re-drive the transaction to commit or abort.
 *  - **commit()**: Writes a durable COMMIT_DECISION record before contacting
 *    any participant.  Guarantees idempotent re-application after restart.
 *  - **abort()**: Writes a durable ABORT_DECISION record before returning to
 *    the caller.  Compensating actions not yet complete MUST be re-driven
 *    during recoverInDoubt().
 *  - **recoverInDoubt()**: Makes forward progress: every durably prepared
 *    transaction MUST eventually reach a terminal state.
 *
 * @par Thread safety
 * Implementations MUST be thread-safe for concurrent callers operating on
 * different transaction IDs.  Concurrent calls on the same transaction ID
 * MUST be serialised internally by the coordinator.
 *
 * @par SOLID compliance
 * - **SRP**: Each coordinator drives a single transaction lifecycle at a
 *            time; observability is provided via const query methods only.
 * - **OCP**: New protocols extend by implementing this interface; existing
 *            coordinator classes are unchanged.
 * - **LSP**: Every conforming implementation honours the WAL/recovery
 *            contract above; callers can substitute any implementation.
 * - **ISP**: Capabilities not supported by a protocol are declared via
 *            capabilities(); callers check before using optional methods.
 * - **DIP**: Callers depend on ITransactionCoordinator (abstraction);
 *            concrete coordinator types are injected at construction time.
 */
class ITransactionCoordinator {
public:
    virtual ~ITransactionCoordinator() = default;

    // ─── Protocol introspection ───────────────────────────────────────────

    /**
     * @brief Return the canonical commit protocol implemented by this coordinator.
     *
     * @return A CommitProtocol enumerator.  CUSTOM is used for user-defined
     *         extension protocols.
     */
    [[nodiscard]] virtual CommitProtocol protocolType() const noexcept = 0;

    /**
     * @brief Return a human-readable protocol name suitable for logging.
     *
     * @note This string is for display and diagnostic purposes only.  Callers
     *       that need to branch on the protocol family MUST use protocolType()
     *       instead of comparing this string, as the canonical spelling is
     *       not normatively fixed across implementations.
     *
     * @return Stable, null-terminated ASCII string (e.g. @c "2PC", @c "3PC",
     *         @c "SAGA", @c "Percolator", @c "Calvin").  The pointed-to
     *         storage is valid for the lifetime of this coordinator instance.
     */
    [[nodiscard]] virtual std::string_view protocolName() const noexcept = 0;

    /**
     * @brief Return the capability flags advertised by this coordinator instance.
     *
     * Callers SHOULD check the relevant capability before invoking optional
     * operations (e.g. check @c supports_prepare_phase before relying on
     * the semantics of prepare()).
     *
     * @return A CoordinatorCapabilities value-object.
     */
    [[nodiscard]] virtual CoordinatorCapabilities capabilities() const noexcept = 0;

    // ─── Transaction lifecycle ────────────────────────────────────────────

    /**
     * @brief Begin a new transaction and record it durably in the WAL.
     *
     * @par WAL contract
     * A BEGIN record MUST be written to stable storage before this method
     * returns OK.  If the WAL write fails, Fail(INTERNAL_ERROR, …) is
     * returned and no in-memory state is created for @p txn_id.
     *
     * @param txn_id  Globally unique transaction identifier chosen by the
     *                caller.  MUST be non-empty.  Passing a duplicate ID for
     *                which a non-terminal transaction already exists MUST
     *                return Fail(INVALID_STATE, …).
     * @param opts    Per-transaction options.  See TxnCoordinatorOptions.
     * @return        OK on success; Fail(INVALID_STATE, …) if @p txn_id is
     *                already active; Fail(INTERNAL_ERROR, …) on WAL failure.
     */
    [[nodiscard]] virtual TxnCoordinatorResult begin(
       std::string_view             txn_id,
        const TxnCoordinatorOptions& opts = {}
    ) = 0;

    /**
     * @brief Phase-1 prepare: collect participant votes and persist the result.
     *
     * @par Voting protocols (2PC, 3PC)
     * Sends PREPARE to all registered participants, collects votes, and
     * writes a durable PREPARE record.  Returns OK only when every
     * participant voted COMMIT.
     *
     * @par Non-voting protocols (SAGA, Percolator, Calvin)
     * Returns OK immediately without side effects.  Implementations MUST
     * NOT modify transaction state or write WAL entries for these protocols.
     *
     * @par WAL contract
     * For voting protocols: a PREPARE_OK or PREPARE_ABORT record MUST be
     * written before this method returns.  After a successful prepare() the
     * coordinator MUST be able to re-drive the transaction to a terminal
     * state after a restart.
     *
     * @param txn_id  Active transaction identifier returned by begin().
     * @return        OK if all participants voted COMMIT (or no prepare phase
     *                exists); Fail(PARTICIPANT_ABORT, …) if any participant
     *                voted ABORT; Fail(TIMEOUT, …) if a participant did not
     *                respond in time; Fail(UNKNOWN_TRANSACTION, …) if
     *                @p txn_id is not known.
     */
    [[nodiscard]] virtual TxnCoordinatorResult prepare(
       std::string_view txn_id
    ) = 0;

    /**
     * @brief Phase-2 commit: apply all prepared operations across participants.
     *
     * @par Voting protocols (2PC, 3PC)
     * MUST be called only after a successful prepare().  Sends COMMIT to
     * all participants and waits for acknowledgement.
     *
     * @par SAGA
     * Executes all registered forward steps in dependency order.
     *
     * @par Percolator
     * Writes the commit timestamp and clears transaction locks from the
     * primary write cell.
     *
     * @par Calvin
     * Applies the deterministically ordered write batch.
     *
     * @par WAL contract
     * A durable COMMIT_DECISION record MUST be written before contacting any
     * participant.  This guarantees idempotent re-application after restart.
     *
     * @param txn_id  Transaction to commit.  For voting protocols, MUST be
     *                in the PREPARED state.  For single-round protocols, MUST
     *                be in the ACTIVE state.
     * @return        OK when all participants confirm commit;
     *                Fail(INVALID_STATE, …) if the state precondition is not
     *                met; Fail(UNKNOWN_TRANSACTION, …) if @p txn_id is
     *                not known.
     */
    [[nodiscard]] virtual TxnCoordinatorResult commit(
       std::string_view txn_id
    ) = 0;

    /**
     * @brief Abort the transaction and trigger compensation or rollback.
     *
     * @par Voting protocols (2PC, 3PC)
     * Broadcasts ABORT to all participants and releases acquired locks.
     *
     * @par SAGA
     * Executes all registered compensating actions in reverse dependency
     * order.  Retries are performed per the saga step retry policy.
     *
     * @par Percolator
     * Releases all held transaction locks (primary and secondary cells).
     *
     * @par Calvin
     * Discards the queued write batch without execution.
     *
     * @par WAL contract
     * A durable ABORT_DECISION record MUST be written before returning to
     * the caller.  Compensating actions that have not yet completed MUST be
     * re-driven during recoverInDoubt().
     *
     * @param txn_id  Transaction to abort.
     * @return        OK when rollback/compensation is complete;
     *                Fail(INVALID_STATE, …) if the transaction is already
     *                in a terminal state;
     *                Fail(UNKNOWN_TRANSACTION, …) if @p txn_id is not known.
     */
    [[nodiscard]] virtual TxnCoordinatorResult abort(
       std::string_view txn_id
    ) = 0;

    // ─── State query ──────────────────────────────────────────────────────

    /**
     * @brief Query the current lifecycle state of a transaction.
     *
     * Returns a protocol-agnostic TxnLifecycleState so that recovery managers
     * can inspect transaction state uniformly across all commit protocols.
     * Implementations SHOULD map their internal state to the canonical values
     * defined in TxnLifecycleState; see that enum for the mapping guidance.
     *
     * @param txn_id  Transaction to query.
     * @return        Canonical lifecycle state, or TxnLifecycleState::UNKNOWN
     *                when @p txn_id is not known to this coordinator.
     */
    [[nodiscard]] virtual TxnLifecycleState getState(
        std::string_view txn_id
    ) const = 0;

    // ─── WAL / Recovery ───────────────────────────────────────────────────

    /**
     * @brief Drive all in-doubt transactions to a terminal state from WAL.
     *
     * Called after a coordinator restart by a recovery manager.  Each
     * in-doubt transaction is re-driven to commit (if a durable COMMIT
     * decision exists) or abort (otherwise).
     *
     * @par Guarantee
     * Implementations MUST make forward progress: every transaction for
     * which a durable PREPARE record exists MUST eventually be committed
     * or aborted, even across multiple recoverInDoubt() calls.
     *
     * @return Number of transactions that were successfully resolved (driven
     *         to a terminal state) during this invocation.
     */
    [[nodiscard]] virtual std::size_t recoverInDoubt() = 0;

    /**
     * @brief Return a point-in-time snapshot of all in-doubt transactions.
     *
     * Provides recovery managers and monitoring tools with a
     * coordinator-independent view of in-doubt state.  The snapshot may be
     * stale by the time the caller acts on it; callers SHOULD re-query after
     * calling recoverInDoubt().
     *
     * @return Descriptors for all non-terminal transactions known to this
     *         coordinator.  An empty vector means the coordinator has no
     *         in-doubt transactions.
     */
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
