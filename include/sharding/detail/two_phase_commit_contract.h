/**
 * @file two_phase_commit_contract.h
 * @brief Shared execution ordering and failure semantics for sharding 2PC coordinators.
 */
#pragma once

namespace themis::sharding::detail {

/**
 * @brief Result of executing the common two-phase commit decision contract.
 * @note EXECUTION_FAILED avoids the Windows SDK's FAILURE macro.
 */
enum class TwoPhaseCommitExecutionResult {
    COMMITTED,
    ABORTED,
    EXECUTION_FAILED
};

/**
 * @brief Run the canonical coordinator-side 2PC ordering.
 *
 * A failed prepare selects ABORT; a successful prepare selects COMMIT. The
 * decision-record callback must succeed before phase two is applied. Once
 * COMMIT is selected, a phase-two failure is an error/in-doubt result and must
 * never be converted into an ABORT. A prepare exception selects ABORT; decision
 * persistence or application exceptions return EXECUTION_FAILED.
 *
 * @param prepare_all Invokes phase one and returns true only when every
 *        participant voted to commit.
 * @param persist_decision Records the selected decision according to the
 *        coordinator's configured persistence policy; false prevents phase
 *        two from being sent. If persistence is disabled, crash durability is
 *        not provided by this callback.
 * @param apply_decision Broadcasts the selected COMMIT or ABORT and returns
 *        true only when all required participant acknowledgements succeed.
 * @tparam Prepare Callable that runs phase one.
 * @tparam PersistDecision Callable that records the selected decision.
 * @tparam ApplyDecision Callable that broadcasts and applies the decision.
 * @return COMMITTED or ABORTED when the selected decision and phase two
 *         complete; EXECUTION_FAILED when decision persistence or application
 *         fails or throws. A prepare exception selects ABORT and continues
 *         through the decision callbacks. An EXECUTION_FAILED result after a
 *         recorded decision means that decision remains authoritative; durable
 *         replay is available only when the coordinator's persistence backend
 *         is configured.
 */
template <typename Prepare, typename PersistDecision, typename ApplyDecision>
[[nodiscard]] inline TwoPhaseCommitExecutionResult executeTwoPhaseCommit(
    Prepare&& prepare_all,
    PersistDecision&& persist_decision,
    ApplyDecision&& apply_decision) {
    bool prepared = false;
    try {
        prepared = prepare_all();
    } catch (...) {
        prepared = false;
    }

    try {
        if (!persist_decision(prepared)) {
            return TwoPhaseCommitExecutionResult::EXECUTION_FAILED;
        }
    } catch (...) {
        return TwoPhaseCommitExecutionResult::EXECUTION_FAILED;
    }

    try {
        if (!apply_decision(prepared)) {
            return TwoPhaseCommitExecutionResult::EXECUTION_FAILED;
        }
    } catch (...) {
        return TwoPhaseCommitExecutionResult::EXECUTION_FAILED;
    }

    return prepared ? TwoPhaseCommitExecutionResult::COMMITTED
                    : TwoPhaseCommitExecutionResult::ABORTED;
}

} // namespace themis::sharding::detail
