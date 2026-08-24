/**
 * @file disaster_recovery_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

#include "replication/replication_manager.h"
#include "sharding/epoch_fencing.h"

namespace themis {
namespace failover {

enum class DisasterRecoveryState {
    IDLE,
    PRECHECKS,
    SNAPSHOT_VALIDATION,
    EPOCH_FENCING,
    RESTORE,
    REPLICA_CATCHUP,
    TRAFFIC_SHIFT,
    VERIFICATION,
    COMPLETED,
    FAILED,
    ABORTED,
};

enum class DisasterRecoveryStep {
    PRECHECKS,
    SNAPSHOT_VALIDATION,
    EPOCH_FENCING,
    RESTORE,
    REPLICA_CATCHUP,
    TRAFFIC_SHIFT,
    VERIFICATION,
};

struct DisasterRecoveryConfig {
    std::chrono::milliseconds precheck_timeout{5000};
    std::chrono::milliseconds catchup_timeout{30000};
    std::chrono::milliseconds verification_timeout{10000};
    uint32_t max_verification_retries{5};

    bool require_quorum{true};
    bool enforce_epoch_fencing{true};
    bool allow_dry_run_without_managers{true};

    /// Consensus timeout for each recovery step (configurable; default 30s).
    std::chrono::milliseconds consensus_timeout_ms{30000};
};

struct DisasterRecoveryPlan {
    std::string plan_id;
    std::string primary_site;
    std::string recovery_site;
    std::string snapshot_id;

    std::vector<std::string> critical_nodes;

    bool dry_run{false};
    bool shift_traffic{true};
};

struct DisasterRecoveryStepResult {
    DisasterRecoveryStep step;
    bool success{false};
    std::string message;
};

struct DisasterRecoveryResult {
    bool success{false};
    DisasterRecoveryState final_state{DisasterRecoveryState::IDLE};
    std::chrono::milliseconds duration{0};
    std::string error_message;
    uint64_t fenced_epoch{0};

    std::vector<DisasterRecoveryStepResult> step_results;
};

/** @brief Disaster recovery manager component. */
class DisasterRecoveryManager {
public:
    using StepHook = std::function<bool(const DisasterRecoveryPlan&, std::string&)>;

    explicit DisasterRecoveryManager(
        DisasterRecoveryConfig config,
        std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr,
        std::shared_ptr<sharding::EpochFencingManager> fencing_mgr);

    /// @brief Executes a disaster recovery plan end-to-end.
    ///        Idempotent: repeated calls with the same plan_id return the cached result
    ///        without re-executing (FO-IMPL-007).
    ///        Fail-closed: if epoch fencing is enforced and fencing_mgr_ is absent or
    ///        returns an invalid epoch, the plan transitions to FAILED.
    ///        Rejects concurrent executions (returns "concurrent execution rejected" error).
    /// @param plan  The disaster recovery plan to execute. plan_id must be non-empty.
    /// @returns DisasterRecoveryResult with success flag, final state, fenced_epoch, and error.
    /// @throws Nothing — all exceptions are caught internally and surface as failed results.
    /// @thread_safety Thread-safe; concurrent callers block on execution_mutex_.
    DisasterRecoveryResult executePlan(const DisasterRecoveryPlan& plan);

    /// @brief Validates a disaster recovery plan without executing it.
    /// @param plan   The plan to validate.
    /// @param[out] error  Human-readable description of any validation failure.
    /// @returns true if the plan is valid; false otherwise.
    /// @thread_safety Thread-safe (read-only).
    bool validatePlan(const DisasterRecoveryPlan& plan, std::string& error) const;

    /// @brief Registers a step hook to intercept or override a specific DR step.
    ///        If the hook returns false, the step is treated as failed.
    /// @param step  The step to intercept.
    /// @param hook  Callable: `bool(const DisasterRecoveryPlan&, std::string& detail)`.
    /// @thread_safety Must not be called concurrently with executePlan.
    void setStepHook(DisasterRecoveryStep step, StepHook hook);
    /// @brief Removes all registered step hooks.
    /// @thread_safety Must not be called concurrently with executePlan.
    void clearStepHooks();

    /// @brief Returns the current execution state of this manager.
    /// @thread_safety Thread-safe (atomic read).
    DisasterRecoveryState getState() const noexcept;

    struct Statistics {
        uint64_t total_runs{0};
        uint64_t successful_runs{0};
        uint64_t failed_runs{0};
        uint64_t aborted_runs{0};
        std::chrono::milliseconds average_duration{0};
    };

    /// @brief Returns a snapshot of accumulated execution statistics.
    /// @thread_safety Thread-safe (mutex-protected).
    Statistics getStatistics() const;

#ifdef THEMIS_TEST_BUILD
    /// @brief Clears the idempotency cache (test support only).
    void clearIdempotencyCache() {
        std::lock_guard<std::mutex> lock(idempotency_mutex_);
        completed_plans_.clear();
    }
#endif

private:
    struct EnumHash {
        template <typename T>
        size_t operator()(T t) const noexcept {
            return static_cast<size_t>(t);
        }
    };

    /// @brief Runs a single DR step, using a registered hook if present.
    /// @param step   The step to run.
    /// @param state  The DisasterRecoveryState associated with this step.
    /// @param plan   The plan being executed.
    /// @param[out] result  Updated with step outcome.
    /// @param[out] error   Human-readable failure detail on false return.
    /// @param[out] fenced_epoch  Updated by EPOCH_FENCING step on success.
    /// @returns true if the step succeeded.
    bool runStep(DisasterRecoveryStep step,
                 DisasterRecoveryState state,
                 const DisasterRecoveryPlan& plan,
                 DisasterRecoveryResult& result,
                 std::string& error,
                 uint64_t& fenced_epoch);

    bool runPrechecks(const DisasterRecoveryPlan& plan, std::string& detail);
    bool validateSnapshot(const DisasterRecoveryPlan& plan, std::string& detail);
    /// @brief Applies epoch fencing via EpochFencingManager.
    ///        Fail-closed: returns false if fencing_mgr_ is absent and enforce_epoch_fencing=true.
    /// @param plan  The active DR plan.
    /// @param[out] detail  Failure description on false return.
    /// @param[out] fenced_epoch  Set to the epoch returned by the fencing manager on success.
    /// @returns true if fencing succeeded or fencing is disabled.
    bool applyEpochFencing(const DisasterRecoveryPlan& plan, std::string& detail, uint64_t& fenced_epoch);
    bool runRestore(const DisasterRecoveryPlan& plan, std::string& detail);
    bool waitForCatchup(const DisasterRecoveryPlan& plan, std::string& detail);
    bool shiftTraffic(const DisasterRecoveryPlan& plan, std::string& detail);
    bool verifyRecoveredState(const DisasterRecoveryPlan& plan, std::string& detail);

    void transitionState(DisasterRecoveryState next) noexcept;
    void updateStatistics(const DisasterRecoveryResult& result);

    DisasterRecoveryConfig config_;
    std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr_;
    std::shared_ptr<sharding::EpochFencingManager> fencing_mgr_;

    std::unordered_map<DisasterRecoveryStep, StepHook, EnumHash> hooks_;

    mutable std::mutex state_mutex_;
    std::atomic<DisasterRecoveryState> state_{DisasterRecoveryState::IDLE};

    // Guards against concurrent invocations of executePlan.
    mutable std::mutex execution_mutex_;

    /// @brief Guards idempotency map access.
    mutable std::mutex idempotency_mutex_;
    /// @brief Maps plan_id → cached result for idempotent execution (FO-IMPL-007).
    std::unordered_map<std::string, DisasterRecoveryResult> completed_plans_;

    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::vector<std::chrono::milliseconds> durations_;
};

}  // namespace failover
}  // namespace themis
