/**
 * @file disaster_recovery_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

class DisasterRecoveryManager {
public:
    using StepHook = std::function<bool(const DisasterRecoveryPlan&, std::string&)>;

    /**
     * @brief Disaster Recovery Manager.
     * @param[in] config Input parameter.
     * @param[in] replication_mgr Input parameter.
     * @param[in] fencing_mgr Input parameter.
     * @return Return value.
     */
    explicit DisasterRecoveryManager(
        DisasterRecoveryConfig config,
        std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr,
        std::shared_ptr<sharding::EpochFencingManager> fencing_mgr);

    /**
     * @brief Execute Plan.
     * @param[in] plan Input parameter.
     * @return Return value.
     */
    DisasterRecoveryResult executePlan(const DisasterRecoveryPlan& plan);

    /**
     * @brief Validate Plan.
     * @param[in] plan Input parameter.
     * @param[in,out] error Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validatePlan(const DisasterRecoveryPlan& plan, std::string& error) const;

    /**
     * @brief Set Step Hook.
     * @param[in] step Input parameter.
     * @param[in] hook Input parameter.
     */
    void setStepHook(DisasterRecoveryStep step, StepHook hook);
    /**
     * @brief Clear Step Hooks.
     */
    void clearStepHooks();

    /**
     * @brief Get State.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    DisasterRecoveryState getState() const noexcept;

    struct Statistics {
        uint64_t total_runs{0};
        uint64_t successful_runs{0};
        uint64_t failed_runs{0};
        uint64_t aborted_runs{0};
        std::chrono::milliseconds average_duration{0};
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

#ifdef THEMIS_TEST_BUILD
    /**
     * @brief Clear Idempotency Cache.
     * @details Calls: lock(), clear().
     */
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

    /**
     * @brief Run Step.
     * @param[in] step Input parameter.
     * @param[in] state Input parameter.
     * @param[in] plan Input parameter.
     * @param[in,out] result Input/output parameter.
     * @param[in,out] error Input/output parameter.
     * @param[in,out] fenced_epoch Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool runStep(DisasterRecoveryStep step,
                 DisasterRecoveryState state,
                 const DisasterRecoveryPlan& plan,
                 DisasterRecoveryResult& result,
                 std::string& error,
                 uint64_t& fenced_epoch);

    /**
     * @brief Run Prechecks.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool runPrechecks(const DisasterRecoveryPlan& plan, std::string& detail);
    /**
     * @brief Validate Snapshot.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool validateSnapshot(const DisasterRecoveryPlan& plan, std::string& detail);
    /**
     * @brief Apply Epoch Fencing.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @param[in,out] fenced_epoch Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool applyEpochFencing(const DisasterRecoveryPlan& plan, std::string& detail, uint64_t& fenced_epoch);
    /**
     * @brief Run Restore.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool runRestore(const DisasterRecoveryPlan& plan, std::string& detail);
    /**
     * @brief Wait For Catchup.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool waitForCatchup(const DisasterRecoveryPlan& plan, std::string& detail);
    /**
     * @brief Shift Traffic.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool shiftTraffic(const DisasterRecoveryPlan& plan, std::string& detail);
    /**
     * @brief Verify Recovered State.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool verifyRecoveredState(const DisasterRecoveryPlan& plan, std::string& detail);

    /**
     * @brief Transition State.
     * @param[in] next Input parameter.
     * @note Exception safety: noexcept.
     */
    void transitionState(DisasterRecoveryState next) noexcept;
    /**
     * @brief Update Statistics.
     * @param[in] result Input parameter.
     */
    void updateStatistics(const DisasterRecoveryResult& result);

    DisasterRecoveryConfig config_;
    std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr_;
    std::shared_ptr<sharding::EpochFencingManager> fencing_mgr_;

    std::unordered_map<DisasterRecoveryStep, StepHook, EnumHash> hooks_;

    mutable std::mutex state_mutex_;
    std::atomic<DisasterRecoveryState> state_{DisasterRecoveryState::IDLE};

    // Guards against concurrent invocations of executePlan.
    mutable std::mutex execution_mutex_;

    mutable std::mutex idempotency_mutex_;
    std::unordered_map<std::string, DisasterRecoveryResult> completed_plans_;

    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::vector<std::chrono::milliseconds> durations_;
};

}  // namespace failover
}  // namespace themis
