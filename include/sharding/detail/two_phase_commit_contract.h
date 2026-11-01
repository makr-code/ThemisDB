/**
 * @file two_phase_commit_contract.h
 * @brief Shared execution ordering and failure semantics for sharding 2PC coordinators.
 */
#pragma once

#include <functional>

namespace themis::sharding::detail {

/**
 * @brief Result of executing the common two-phase commit decision contract.
 */
enum class TwoPhaseCommitExecutionResult {
    COMMITTED,
    ABORTED,
    ERROR
};

/**
 * @brief Run the canonical coordinator-side 2PC ordering.
 *
 * A failed prepare selects ABORT; a successful prepare selects COMMIT. The
 * decision-record callback must succeed before phase two is applied. Once
 * COMMIT is selected, a phase-two failure is an error/in-doubt result and must
 * never be converted into an ABORT. Callback exceptions are treated as failures.
 *
 * @param prepare_all Invokes phase one and returns true only when every
 *        participant voted to commit.
 * @param persist_decision Records the selected decision according to the
 *        coordinator's configured persistence policy; false prevents phase
 *        two from being sent. If persistence is disabled, crash durability is
 *        not provided by this callback.
 * @param apply_decision Broadcasts the selected COMMIT or ABORT and returns
 *        true only when all required participant acknowledgements succeed.
 * @return COMMITTED or ABORTED when the selected decision and phase two
 *         complete; ERROR when a callback fails or throws. An ERROR after a
 *         recorded decision means that decision remains authoritative; durable
 *         replay is available only when the coordinator's persistence backend
 *         is configured.
 */
[[nodiscard]] inline TwoPhaseCommitExecutionResult executeTwoPhaseCommit(
    const std::function<bool()>& prepare_all,
    const std::function<bool(bool)>& persist_decision,
    const std::function<bool(bool)>& apply_decision) {
    bool prepared = false;
    try {
        prepared = prepare_all();
    } catch (...) {
        prepared = false;
    }

    try {
        if (!persist_decision(prepared)) {
            return TwoPhaseCommitExecutionResult::ERROR;
        }
    } catch (...) {
        return TwoPhaseCommitExecutionResult::ERROR;
    }

    try {
        if (!apply_decision(prepared)) {
            return TwoPhaseCommitExecutionResult::ERROR;
        }
    } catch (...) {
        return TwoPhaseCommitExecutionResult::ERROR;
    }

    return prepared ? TwoPhaseCommitExecutionResult::COMMITTED
                    : TwoPhaseCommitExecutionResult::ABORTED;
}

} // namespace themis::sharding::detail
