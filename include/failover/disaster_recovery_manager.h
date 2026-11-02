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
#include <cstdint>
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

enum class DisasterRecoveryErrorCode {
    NONE,
    INVALID_PLAN,
    NOT_CONFIGURED,
    ADAPTER_FAILURE,
    VERIFICATION_FAILED,
    STALE_REPLICATION_POSITION,
    QUORUM_UNAVAILABLE,
    FENCING_UNAVAILABLE,
    METADATA_PERSISTENCE_FAILED,
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
    /** @brief Append-only file used to persist completed DR plan identities. */
    std::string metadata_log_path;
};

struct DisasterRecoveryPlan {
    std::string plan_id;
    std::string primary_site;
    std::string recovery_site;
    std::string snapshot_id;
    /** @brief Expected content checksum verified after snapshot and restore. */
    std::string snapshot_checksum;
    /** @brief Minimum replication sequence and exact source term required at catch-up. */
    uint64_t required_lsn{0};
    uint64_t required_term{0};

    std::vector<std::string> critical_nodes;

    bool dry_run{false};
    bool shift_traffic{true};
};

struct DisasterRecoveryStepResult {
    DisasterRecoveryStep step;
    bool success{false};
    std::string message;
    DisasterRecoveryErrorCode error_code{DisasterRecoveryErrorCode::NONE};
};

struct DisasterRecoveryResult {
    bool success{false};
    DisasterRecoveryState final_state{DisasterRecoveryState::IDLE};
    std::chrono::milliseconds duration{0};
    std::string error_message;
    uint64_t fenced_epoch{0};
    DisasterRecoveryErrorCode error_code{DisasterRecoveryErrorCode::NONE};

    std::vector<DisasterRecoveryStepResult> step_results;
};

/**
 * @brief Independently observed evidence for a completed DR adapter operation.
 *
 * Snapshot and restore steps require a matching checksum; catch-up requires
 * the requested term and at least the requested LSN; traffic shift requires
 * the recovery-site identity. Empty evidence is never accepted.
 */
struct DisasterRecoveryVerificationProof {
    std::string checksum;
    uint64_t lsn{0};
    uint64_t term{0};
    std::string observed_site;
};

class DisasterRecoveryManager {
public:
    /**
     * @brief Executes and verifies one external DR operation.
     * @param plan The validated recovery plan.
     * @param detail Output diagnostic; populated on success and failure.
     * @return True only when the operation completed and was verified.
     * @note Production adapters must perform the real side effect and independently
     *       verify it; returning true without doing so makes the orchestration unsafe.
     * @note Hooks run synchronously; the manager does not impose a timeout or
     *       cancellation boundary on hook execution.
     */
    using StepHook = std::function<bool(const DisasterRecoveryPlan&, std::string&)>;
    using VerificationHook = std::function<std::optional<DisasterRecoveryVerificationProof>(
        const DisasterRecoveryPlan&, std::string&)>;

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
     * @brief Execute an ordered disaster-recovery plan.
     * @param[in] plan Plan to validate and execute.
     * @return A completed result only when every required operation succeeds.
     *         Missing snapshot, restore, catch-up, traffic-shift, or verification
     *         adapters fail the result; traffic shift is not required when disabled
     *         by the plan. Dry-run plans do not perform external side effects.
     * @note Steps completed before a failure are not compensated or rolled back.
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
     * @brief Configure the real operation adapter for a DR step.
     * @param step External step to configure; internal prechecks/fencing cannot
     *             be overridden.
     * @param hook Adapter operation; true means the remote side acknowledged it.
     * @note Acknowledgement alone is insufficient: a matching verification
     *       hook is also required for non-dry-run execution.
     */
    void setStepHook(DisasterRecoveryStep step, StepHook hook);
    /**
     * @brief Configure independent read-back verification for a DR step.
     * @param step External step whose result is independently observed.
     * @param hook Returns checksum, replication-position, or site evidence;
     *             nullopt or mismatched evidence fails the step.
     */
    void setVerificationHook(DisasterRecoveryStep step, VerificationHook hook);
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
        completed_plan_fingerprints_.clear();
    }

    /** @brief Override quorum only in focused tests; unavailable in production builds. */
    void testSetQuorumOverride(bool has_quorum) noexcept {
        test_quorum_override_ = has_quorum;
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
                 uint64_t& fenced_epoch,
                 DisasterRecoveryErrorCode& error_code);

    [[nodiscard]] bool runAdapterStep(
        DisasterRecoveryStep step,
        const DisasterRecoveryPlan& plan,
        std::string& detail,
        DisasterRecoveryErrorCode& error_code);

    [[nodiscard]] bool persistCompletedPlan(
        const DisasterRecoveryPlan& plan,
        const DisasterRecoveryResult& result,
        std::string& detail);

    void loadCompletedPlans();

    /**
     * @brief Run Prechecks.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool runPrechecks(const DisasterRecoveryPlan& plan,
                      std::string& detail,
                      DisasterRecoveryErrorCode& error_code);
    /**
     * @brief Apply Epoch Fencing.
     * @param[in] plan Input parameter.
     * @param[in,out] detail Input/output parameter.
     * @param[in,out] fenced_epoch Input/output parameter.
     * @return True when the operation succeeds.
     */
    bool applyEpochFencing(const DisasterRecoveryPlan& plan,
                          std::string& detail,
                          uint64_t& fenced_epoch,
                          DisasterRecoveryErrorCode& error_code);

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
    std::unordered_map<DisasterRecoveryStep, VerificationHook, EnumHash> verification_hooks_;

    mutable std::mutex state_mutex_;
    std::atomic<DisasterRecoveryState> state_{DisasterRecoveryState::IDLE};

    // Guards against concurrent invocations of executePlan.
    mutable std::mutex execution_mutex_;

    mutable std::mutex idempotency_mutex_;
    std::unordered_map<std::string, DisasterRecoveryResult> completed_plans_;
    std::unordered_map<std::string, std::string> completed_plan_fingerprints_;
    std::string metadata_load_error_;

    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::vector<std::chrono::milliseconds> durations_;

#ifdef THEMIS_TEST_BUILD
    std::optional<bool> test_quorum_override_;
#endif
};

}  // namespace failover
}  // namespace themis
