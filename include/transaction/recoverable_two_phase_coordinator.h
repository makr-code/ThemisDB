/**
 * @file recoverable_two_phase_coordinator.h
 * @brief Two-phase commit coordinator with WAL-based crash recovery.
 *
 * Extends the base 2PC coordinator with a durable write-ahead log
 * so that in-doubt transactions survive coordinator crashes.
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License
//
// Unified 2PC Recovery Interface — shared contract for all 2PC coordinators.
//
// Multiple independent coordinator implementations exist in ThemisDB:
//   1. TwoPhaseCommitCoordinator      (sharding/two_phase_commit_coordinator.h)
//   2. CrossShardTransactionCoordinator (sharding/cross_shard_transaction.h)
//   3. DistributedTransactionCoordinator (sharding/distributed_transaction.h)
//   4. GlobalTransactionManager       (transaction/global_transaction_manager.h)
//   5. DistributedTransactionManager  (transaction/distributed_transaction_manager.h)
//
// This header provides:
//   - IRecoverableTwoPhaseCoordinator — extended interface that every coordinator
//     exposes coordinator-local recovery state to a global reporting pass.
//   - GlobalTwoPhaseCommitRecoveryManager — header-only orchestrator that drives
//     one recovery pass across all registered coordinators and returns a
//     unified report.

#pragma once

#include "transaction/in_doubt_recovery_coordinator.h"
#include <cstddef>
#include <string>
#include <vector>

namespace themis::transaction {

// ─────────────────────────────────────────────────────────────────────────────
// Canonical normalized recovery state
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Canonical recovery state used across heterogeneous 2PC coordinators.
 *
 * Each coordinator implementation keeps its own internal state machine.
 * This enum provides a normalized view for global recovery, diagnostics,
 * and interoperability.
 */
enum class RecoverableTwoPhaseState {
    ACTIVE,      ///< Transaction exists but has not reached PREPARE yet.
    PREPARING,   ///< Phase 1 is still collecting votes.
    PREPARED,    ///< All participants prepared; no durable final decision yet.
    COMMITTING,  ///< Durable COMMIT decision exists; Phase 2 in progress.
    ABORTING,    ///< Durable ABORT decision exists; Phase 2 in progress.
    COMPLETED,   ///< Final decision was fully replayed/completed.
    FAILED,      ///< Recovery cannot make further progress automatically.
    UNKNOWN      ///< Coordinator cannot classify the transaction safely.
};

// ─────────────────────────────────────────────────────────────────────────────
// Per-transaction recovery snapshot
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Normalized per-transaction snapshot exposed by recoverable coordinators.
 *
 * Coordinators fill this struct from their internal state so that the global
 * recovery manager can compute aggregated in-doubt counts without needing
 * to know coordinator-specific types.
 */
struct RecoverableTwoPhaseTransaction {
    std::string              transaction_id;                    ///< Coordinator-local transaction identifier.
    RecoverableTwoPhaseState state = RecoverableTwoPhaseState::UNKNOWN; ///< Canonical recovery state.
    bool                     decision_recorded = false;         ///< True when a durable COMMIT/ABORT decision exists.
    bool                     decision_commit   = false;         ///< Valid when decision_recorded is true.
};

// ─────────────────────────────────────────────────────────────────────────────
// IRecoverableTwoPhaseCoordinator
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Extended recovery interface for globally orchestrated 2PC recovery.
 *
 * Coordinators implementing this contract can participate in a single
 * recovery pass that reports normalized pre/post in-doubt counts while still
 * using their native WAL or snapshot backend internally.
 *
 * The following implementations currently expose this interface:
 * - TwoPhaseCommitCoordinator
 * - CrossShardTransactionCoordinator
 * - DistributedTransactionCoordinator
 * - GlobalTransactionManager
 * - DistributedTransactionManager
 *
 * This shared reporting interface does not select a canonical runtime
 * coordinator, unify their WAL formats, or provide a system readiness gate.
 */
class IRecoverableTwoPhaseCoordinator : public IInDoubtRecoveryCoordinator {
public:
    ~IRecoverableTwoPhaseCoordinator() override = default;

    /**
     * @brief Return a stable coordinator identifier for recovery reports.
     * @return Human-readable coordinator type name (e.g. "TwoPhaseCommitCoordinator").
     */
    [[nodiscard]] virtual std::string recoveryCoordinatorName() const = 0;

    /**
     * @brief Return the durable backend used by this coordinator.
     * @return Backend name such as "WAL", "WAL/snapshot", or "disabled".
     */
    [[nodiscard]] virtual std::string recoveryBackendName() const = 0;

    /**
     * @brief Return the currently known recoverable/non-final transactions.
     *
     * The returned snapshot reflects the coordinator's view at the time of
     * the call.  Non-final means any state except COMPLETED.
     *
     * @return Canonical recovery snapshot for global orchestration.
     */
    [[nodiscard]] virtual std::vector<RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const = 0;
};

// ─────────────────────────────────────────────────────────────────────────────
// Recovery report types
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Per-coordinator result of a global recovery pass.
 */
struct RecoverableTwoPhaseCoordinatorReport {
    std::string coordinator_name;  ///< Value from recoveryCoordinatorName().
    std::string backend_name;      ///< Value from recoveryBackendName().
    size_t      in_doubt_before = 0; ///< Non-final transaction count before recovery.
    size_t      resolved        = 0; ///< Transactions successfully re-driven.
    size_t      in_doubt_after  = 0; ///< Non-final transaction count after recovery.
};

/**
 * @brief Aggregate report for a global 2PC recovery pass.
 */
struct GlobalTwoPhaseCommitRecoveryReport {
    size_t coordinator_count = 0; ///< Number of coordinators visited.
    size_t in_doubt_before   = 0; ///< Aggregate non-final count before recovery.
    size_t resolved          = 0; ///< Aggregate transactions resolved.
    size_t in_doubt_after    = 0; ///< Aggregate non-final count after recovery.

    /// Per-coordinator breakdown.
    std::vector<RecoverableTwoPhaseCoordinatorReport> coordinators;

    /// False if any supplied coordinator is missing or unresolved work remains.
    bool ready_for_service = false;

    /**
    * @brief Whether startup must remain blocked after the recovery pass.
    * @return True if any coordinator is missing or unresolved work remains.
    */
    [[nodiscard]] bool startupBlocked() const noexcept {
        return !ready_for_service;
    }
};

// ─────────────────────────────────────────────────────────────────────────────
// GlobalTwoPhaseCommitRecoveryManager
// ─────────────────────────────────────────────────────────────────────────────

/**
 * @brief Run one normalized recovery pass across multiple 2PC coordinators.
 *
 * This is a stateless header-only orchestrator. Call recoverAll() once after
 * coordinator restart to drive every registered coordinator through its
 * recovery sequence and obtain a unified report. Callers must withhold service
 * readiness unless ready_for_service is true. This class does not establish
 * cross-coordinator WAL compatibility or multi-process recovery guarantees.
 *
 * Example:
 * @code
 *   std::vector<IRecoverableTwoPhaseCoordinator*> coordinators = {
 *       &twoPhaseCoord, &globalTxnMgr, &distributedTxnMgr
 *   };
 *   auto report = GlobalTwoPhaseCommitRecoveryManager::recoverAll(coordinators);
 *   assert(report.in_doubt_after == 0);
 * @endcode
 */
class GlobalTwoPhaseCommitRecoveryManager {
public:
    /**
     * @brief Recover all supplied coordinators and return a unified report.
     *
     * Iterates @p coordinators in order, calls recoverInDoubtTransactions()
     * on each, and accumulates before/after counts and per-coordinator details.
     * Null pointers in the list are silently skipped.
     *
     * @param coordinators Recovery-capable coordinators to visit exactly once.
     * @return             Aggregated counts and per-coordinator results.
     *                      ready_for_service is false when a supplied coordinator
     *                      is null or any non-final transaction remains.
     *                     Use startupBlocked() to test readiness.
     */
    [[nodiscard]] static GlobalTwoPhaseCommitRecoveryReport recoverAll(
        const std::vector<IRecoverableTwoPhaseCoordinator*>& coordinators
    ) {
        GlobalTwoPhaseCommitRecoveryReport report;
        report.coordinator_count = coordinators.size();
        report.coordinators.reserve(coordinators.size());
        bool all_coordinators_present = true;

        for (auto* coordinator : coordinators) {
            if (!coordinator) {
                all_coordinators_present = false;
                continue;
            }
            RecoverableTwoPhaseCoordinatorReport coordinator_report;
            coordinator_report.coordinator_name = coordinator->recoveryCoordinatorName();
            coordinator_report.backend_name     = coordinator->recoveryBackendName();
            coordinator_report.in_doubt_before  =
                coordinator->getRecoverableTransactions().size();

            coordinator_report.resolved =
                coordinator->recoverInDoubtTransactions();
            coordinator_report.in_doubt_after =
                coordinator->getRecoverableTransactions().size();

            report.in_doubt_before += coordinator_report.in_doubt_before;
            report.resolved        += coordinator_report.resolved;
            report.in_doubt_after  += coordinator_report.in_doubt_after;
            report.coordinators.push_back(std::move(coordinator_report));
        }

        report.ready_for_service =
            all_coordinators_present && report.in_doubt_after == 0;
        return report;
    }
};

} // namespace themis::transaction
