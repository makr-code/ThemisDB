/**
 * @file recoverable_two_phase_coordinator.h
 * @brief Common global-recovery contract for 2PC-capable coordinators.
 */

#pragma once

#include "transaction/in_doubt_recovery_coordinator.h"

#include <cstddef>
#include <string>
#include <vector>

namespace themis::transaction {

/**
 * @brief Canonical recovery state used across heterogeneous 2PC coordinators.
 *
 * The individual coordinator implementations keep their own internal state
 * machines. This enum provides a normalized view for global recovery,
 * diagnostics, and interoperability tests.
 */
enum class RecoverableTwoPhaseState {
    ACTIVE,      ///< Transaction exists but has not reached PREPARE yet.
    PREPARING,   ///< Phase 1 is still collecting votes.
    PREPARED,    ///< All participants prepared but no durable final decision yet.
    COMMITTING,  ///< Durable COMMIT decision exists and Phase 2 is in progress.
    ABORTING,    ///< Durable ABORT decision exists and Phase 2 is in progress.
    COMPLETED,   ///< Final decision was fully replayed/completed.
    FAILED,      ///< Recovery cannot make further progress automatically.
    UNKNOWN      ///< Coordinator cannot classify the transaction safely.
};

/**
 * @brief Normalized per-transaction snapshot exposed by recoverable coordinators.
 */
struct RecoverableTwoPhaseTransaction {
    std::string             transaction_id;     ///< Coordinator-local transaction identifier.
    RecoverableTwoPhaseState state = RecoverableTwoPhaseState::UNKNOWN; ///< Canonical recovery state.
    bool                    decision_recorded = false; ///< True when a durable COMMIT/ABORT decision exists.
    bool                    decision_commit = false;   ///< Valid when decision_recorded is true.
};

/**
 * @brief Extended recovery interface for globally orchestrated 2PC recovery.
 *
 * Coordinators implementing this contract can participate in a single
 * recovery pass that reports normalized pre/post in-doubt counts while still
 * using their native WAL or snapshot backend internally.
 */
class IRecoverableTwoPhaseCoordinator : public IInDoubtRecoveryCoordinator {
public:
    ~IRecoverableTwoPhaseCoordinator() override = default;

    /**
     * @brief Return a stable coordinator identifier for recovery reports.
     * @return Human-readable coordinator name.
     */
    [[nodiscard]] virtual std::string recoveryCoordinatorName() const = 0;

    /**
     * @brief Return the durable backend used by this coordinator.
     * @return Backend name such as "WAL", "WAL/snapshot", or "disabled".
     */
    [[nodiscard]] virtual std::string recoveryBackendName() const = 0;

    /**
     * @brief Return the currently known recoverable/non-final transactions.
     * @return Canonical recovery snapshot for global orchestration.
     */
    [[nodiscard]] virtual std::vector<RecoverableTwoPhaseTransaction>
    getRecoverableTransactions() const = 0;
};

/**
 * @brief Per-coordinator result of a global recovery pass.
 */
struct RecoverableTwoPhaseCoordinatorReport {
    std::string coordinator_name; ///< Name reported by recoveryCoordinatorName().
    std::string backend_name;     ///< Durable backend reported by recoveryBackendName().
    size_t      in_doubt_before = 0; ///< Non-final transaction count before replay.
    size_t      resolved = 0;        ///< Transactions resolved by recoverInDoubtTransactions().
    size_t      in_doubt_after = 0;  ///< Non-final transaction count after replay.
};

/**
 * @brief Aggregate report for a global 2PC recovery pass.
 */
struct GlobalTwoPhaseCommitRecoveryReport {
    size_t coordinator_count = 0; ///< Number of coordinators visited.
    size_t in_doubt_before = 0;   ///< Total pre-recovery in-doubt count.
    size_t resolved = 0;          ///< Total resolved count reported by coordinators.
    size_t in_doubt_after = 0;    ///< Total post-recovery in-doubt count.
    std::vector<RecoverableTwoPhaseCoordinatorReport> coordinators; ///< Per-coordinator details.
};

/**
 * @brief Run one normalized recovery pass across multiple 2PC coordinators.
 */
class GlobalTwoPhaseCommitRecoveryManager {
public:
    /**
     * @brief Recover all supplied coordinators and return a unified report.
     * @param coordinators Recovery-capable coordinators to visit exactly once.
     * @return Aggregated before/after counts and per-coordinator results.
     */
    [[nodiscard]] static GlobalTwoPhaseCommitRecoveryReport recoverAll(
        const std::vector<IRecoverableTwoPhaseCoordinator*>& coordinators
    ) {
        GlobalTwoPhaseCommitRecoveryReport report;
        report.coordinator_count = coordinators.size();

        report.coordinators.reserve(coordinators.size());
        for (auto* coordinator : coordinators) {
            if (!coordinator) {
                continue;
            }

            RecoverableTwoPhaseCoordinatorReport coordinator_report;
            coordinator_report.coordinator_name = coordinator->recoveryCoordinatorName();
            coordinator_report.backend_name = coordinator->recoveryBackendName();
            coordinator_report.in_doubt_before =
                coordinator->getRecoverableTransactions().size();
            coordinator_report.resolved =
                coordinator->recoverInDoubtTransactions();
            coordinator_report.in_doubt_after =
                coordinator->getRecoverableTransactions().size();

            report.in_doubt_before += coordinator_report.in_doubt_before;
            report.resolved += coordinator_report.resolved;
            report.in_doubt_after += coordinator_report.in_doubt_after;
            report.coordinators.push_back(std::move(coordinator_report));
        }

        return report;
    }
};

} // namespace themis::transaction
