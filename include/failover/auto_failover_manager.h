/**
 * @file auto_failover_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "failover/failover_api_contract.h"
#include "failover/topology_snapshot.h"
#include "failover/quorum_log.h"
#include "replication/replication_manager.h"
#include "sharding/epoch_fencing.h"
#include "sharding/health_monitor.h"
#include "sharding/hot_spare_manager.h"

namespace themis {
namespace failover {

enum class FailoverEventType {
    NODE_FAILURE_DETECTED,
    QUORUM_CHECK_PASSED,
    QUORUM_CHECK_FAILED,
    LEADER_ELECTION_STARTED,
    LEADER_ELECTED,
    SPARE_ACTIVATED,
    SPARE_ACTIVATION_FAILED,
    FAILOVER_COMPLETED,
    FAILOVER_CANCELLED,
    RECOVERY_STARTED,
    RECOVERY_COMPLETED,
    NETWORK_PARTITION_DETECTED,
    QUEUE_PRESSURE,            // Emitted when queue depth exceeds pressure threshold
    HEARTBEAT_MISSED,          // Emitted when a health-check call times out (maps from FailoverErrorCode::HEARTBEAT_MISSED)
    SPLIT_BRAIN_RISK_DETECTED, // Emitted when split-brain prevention fails (maps from SPLIT_BRAIN_DETECTED)
    NODE_REJOIN_FAILED,        // Emitted when a node exhausts recovery attempts (maps from FailoverErrorCode::NODE_REJOIN_FAILED)
};

enum class FailoverOrchestratorState {
    IDLE,
    DETECTING_FAILURE,
    VERIFYING_FAILURE,
    CHECKING_QUORUM,
    STARTING_LEADER_ELECTION,
    LEADER_ELECTION_IN_PROGRESS,
    ACTIVATING_SPARE,
    REDIRECTING_TRAFFIC,
    UPDATING_METADATA,
    COMPLETING_FAILOVER,
    FAILED,
};

struct AutoFailoverConfig {
    // Timing
    std::chrono::milliseconds failure_detection_interval{1000};         // 1 second
    std::chrono::milliseconds health_check_interval{500};               // 500ms
    std::chrono::milliseconds failover_timeout{30000};                  // 30 seconds
    std::chrono::milliseconds spare_activation_timeout{10000};          // 10 seconds
    std::chrono::milliseconds leader_election_timeout{15000};           // 15 seconds
    
    // Thresholds
    uint32_t consecutive_failures_before_action{3};                     // 3 consecutive failures
    uint32_t max_concurrent_failovers{2};                               // Max 2 failovers at same time
    float    queue_pressure_threshold{0.75f};                           // Emit QUEUE_PRESSURE when queue >= 75% full
    
    // Behavior
    bool enable_automatic_failover{true};
    bool enable_spare_activation{true};
    bool enable_leader_election{true};
    bool enable_network_partition_detection{true};
    bool enable_split_brain_prevention{true};
    
    // Recovery
    bool enable_automatic_recovery{true};
    std::chrono::milliseconds recovery_retry_interval{5000};            // 5 second retry
    uint32_t max_recovery_attempts{3};

    std::chrono::milliseconds health_check_call_timeout_ms{5000};       ///< 5 s default

    std::string quorum_log_path;

    // ── Part B1: Adaptive health-check interval ──────────────────────────────
    bool adaptive_check_interval{false};
    uint32_t adaptive_check_samples{20};
    std::chrono::milliseconds adaptive_check_interval_min{100};
    std::chrono::milliseconds adaptive_check_interval_max{5000};
    uint32_t gc_grace_failure_count{0};
    std::chrono::milliseconds gc_grace_window{1000};
    std::chrono::milliseconds gc_grace_period{2000};

    // ── Part B2: Consensus quorum hardening ──────────────────────────────────
    std::chrono::milliseconds quorum_timeout_ms{30000};
    bool deterministic_tie_breaking{true};
    uint32_t max_heartbeats_per_second{5};
};

struct FailoverResult {
    bool success{false};
    std::string failed_node_id;
    std::string promoted_node_id;
    std::string spare_node_id;
    std::chrono::milliseconds duration;
    std::string error_message;
};

class AutoFailoverManager {
public:
    /**
     * @brief Auto Failover Manager.
     * @param[in] config Input parameter.
     * @param[in] replication_mgr Input parameter.
     * @param[in] health_monitor Input parameter.
     * @param[in] spare_manager Input parameter.
     * @param[in] fencing_manager Input parameter.
     * @return Return value.
     */
    explicit AutoFailoverManager(
        const AutoFailoverConfig& config,
        std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr,
        std::shared_ptr<sharding::HealthMonitor> health_monitor,
        std::shared_ptr<sharding::HotSpareManager> spare_manager,
        std::shared_ptr<sharding::EpochFencingManager> fencing_manager
    );

    ~AutoFailoverManager();

    // Lifecycle management
    /**
     * @brief Start.
     * @return True when the operation succeeds.
     */
    bool start();
    /**
     * @brief Stop.
     * @return True when the operation succeeds.
     */
    bool stop();
    /**
     * @brief Is Running.
     * @return True when the operation succeeds.
     */
    bool isRunning() const;

    // Manual failover trigger (for testing/ops)
    bool triggerManualFailover(
        const std::string& failed_node_id,
        const std::string& target_promote_id = ""
    );

    // Query current state
    /**
     * @brief Get State.
     * @return Return value.
     */
    FailoverOrchestratorState getState() const;
    /**
     * @brief Is Failover In Progress.
     * @return True when the operation succeeds.
     */
    bool isFailoverInProgress() const;
    /**
     * @brief Get Failing Nodes.
     * @return Return value.
     */
    std::vector<std::string> getFailingNodes() const;
    /**
     * @brief Get Last Failover Result.
     * @return Return value.
     */
    std::optional<FailoverResult> getLastFailoverResult() const;

    // Configuration management
    /**
     * @brief Update the access control configuration.
     * @param[in] config New access control configuration.
     */
    void updateConfig(const AutoFailoverConfig& config);
    /**
     * @brief Get Config.
     * @return Return value.
     */
    AutoFailoverConfig getConfig() const;

    // Statistics and monitoring
    struct Statistics {
        uint64_t total_failovers{0};
        uint64_t successful_failovers{0};
        uint64_t failed_failovers{0};
        uint64_t network_partitions_detected{0};
        uint64_t split_brain_preventions{0};
        std::chrono::milliseconds avg_failover_time;
        std::chrono::milliseconds min_failover_time;
        std::chrono::milliseconds max_failover_time;

        // Queue-pressure telemetry (Phase 5)
        uint32_t current_queue_depth{0};
        uint32_t max_queue_depth_observed{0};
        uint64_t tasks_dropped_queue_full{0};
        uint64_t queue_pressure_events{0};

        // Retry telemetry (Phase 5)
        uint64_t total_retry_attempts{0};
        uint64_t successful_retries{0};
        uint64_t failed_retries{0};

        // Recovery telemetry
        uint64_t total_recovery_attempts{0};
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;
    /**
     * @brief Reset Statistics.
     */
    void resetStatistics();

    // Event callback registration
    using FailoverEventCallback = std::function<void(
        FailoverEventType,
        const std::string&,  // node_id
        const std::string&   // detail message
    )>;

    /**
     * @brief Register Event Callback.
     * @param[in] callback Input parameter.
     */
    void registerEventCallback(FailoverEventCallback callback);

    /**
     * @brief Can Transition.
     * @param[in] from Input parameter.
     * @param[in] to Input parameter.
     * @return True when the operation succeeds.
     */
    bool canTransition(FailoverOrchestratorState from, FailoverOrchestratorState to) const;

#ifdef THEMIS_TEST_BUILD
    /**
     * @brief Test Prevent Split Brain.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     * @details Calls: preventSplitBrain().
     */
    bool testPreventSplitBrain(const std::string& node_id) {
        return preventSplitBrain(node_id);
    }
    /**
     * @brief Test Attempt Recovery.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     * @details Calls: attemptRecovery().
     */
    bool testAttemptRecovery(const std::string& node_id) {
        return attemptRecovery(node_id);
    }
    /**
     * @brief Test Emit Diagnostic.
     * @param[in] code Input parameter.
     * @param[in] node_id Identifier of the node.
     * @param[in] detail Input parameter.
     * @details Calls: emitDiagnostic().
     */
    void testEmitDiagnostic(FailoverErrorCode code,
                            const std::string& node_id,
                            const std::string& detail) {
        emitDiagnostic(code, node_id, detail);
    }
    /**
     * @brief Test Process Failover.
     * @param[in] failed_node_id Identifier of the failed node.
     * @return Return value.
     * @details Calls: std::chrono::steady_clock::now(), processFailover().
     */
    FailoverResult testProcessFailover(const std::string& failed_node_id) {
        FailoverTask task;
        task.failed_node_id = failed_node_id;
        task.enqueued_at    = std::chrono::steady_clock::now();
        return processFailover(task);
    }
    void testSetHealthCheckOverride(
        std::function<std::map<std::string, bool>()> fn) {
        health_check_override_ = std::move(fn);
    }
    /**
     * @brief Auto Failover Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AutoFailoverManager(const AutoFailoverConfig& config)
        : AutoFailoverManager(config, nullptr, nullptr, nullptr, nullptr) {}

    /**
     * @brief Add Callback.
     * @param[in] cb Input parameter.
     * @details Calls: registerEventCallback(), std::move().
     */
    void addCallback(FailoverEventCallback cb) {
        registerEventCallback(std::move(cb));
    }
    /**
     * @brief Add Monitored Node.
     * @param[in] param Input parameter.
     * @param[in] NodeRole Input parameter.
     * @details Implements addMonitoredNode without additional internal calls.
     */
    void addMonitoredNode(const std::string& /*node_id*/, NodeRole /*role*/) {}
    /**
     * @brief Test Call Prevent Split Brain.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     * @details Calls: preventSplitBrain().
     */
    bool testCallPreventSplitBrain(const std::string& node_id) {
        return preventSplitBrain(node_id);
    }
    /**
     * @brief Test Trigger Health Check.
     * @details Calls: performHealthChecks().
     */
    void testTriggerHealthCheck() {
        performHealthChecks();
    }
    /**
     * @brief Test Trigger Recovery.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     * @details Calls: attemptRecovery().
     */
    bool testTriggerRecovery(const std::string& node_id) {
        return attemptRecovery(node_id);
    }
    void testSetRecoveryOverride(std::function<bool(const std::string&)> fn) {
        recovery_override_ = std::move(fn);
    }
#endif

private:
    // Lock order (must be acquired in this order when nesting):
    //   failover_mutex_ → stats_mutex_ → callbacks_mutex_
    //   tracking_mutex_ (exclusive) → monitor_mutex_  [only in checkAndApplyGcGrace]
    // Never hold monitor_mutex_ and attempt to acquire tracking_mutex_.
    // Configuration and managers
    AutoFailoverConfig config_;
    std::unique_ptr<QuorumLog> quorum_log_;  ///< Optional durable quorum WAL; null if path not configured.
    std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr_;
    std::shared_ptr<sharding::HealthMonitor> health_monitor_;
    std::shared_ptr<sharding::HotSpareManager> spare_manager_;
    std::shared_ptr<sharding::EpochFencingManager> fencing_manager_;

    // State management
    std::atomic<FailoverOrchestratorState> state_{FailoverOrchestratorState::IDLE};
    std::atomic<bool> running_{false};
    std::atomic<bool> failover_in_progress_{false};

    // Threading
    std::thread monitoring_thread_;
    std::thread failover_thread_;
    mutable std::mutex monitor_mutex_;
    std::mutex failover_mutex_;
    std::condition_variable failover_cv_;

    // Failover queue
    struct FailoverTask {
        std::string failed_node_id;
        std::string target_promote_id;  // empty = auto-select
        std::chrono::steady_clock::time_point enqueued_at;
    };

    std::queue<FailoverTask> failover_queue_{};  // RAII: In-class initializer ensures empty state

    // Tracking
    std::unordered_map<std::string, int> consecutive_failures_;
    mutable std::shared_mutex tracking_mutex_;

    std::atomic<uint64_t> topology_version_{0};

    /**
     * @brief Capture Topology Snapshot.
     * @return Return value.
     */
    TopologySnapshot captureTopologySnapshot() const;
    std::optional<FailoverResult> last_failover_result_;

    // ── Part B1: Adaptive interval + GC grace state ──────────────────────────
    std::vector<std::chrono::milliseconds> health_check_latency_samples_;
    std::vector<std::chrono::steady_clock::time_point> recent_failure_timestamps_;
    std::chrono::steady_clock::time_point gc_grace_expiry_{};
    std::chrono::milliseconds current_check_interval_{500};

    // ── Part B2: Quorum-hardening coalescing state ────────────────────────────
    std::chrono::steady_clock::time_point heartbeat_coalesce_window_start_{};
    uint32_t heartbeat_coalesce_count_{0};

    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::vector<std::chrono::milliseconds> failover_durations_;

    // Event callbacks
    mutable std::mutex callbacks_mutex_;
    std::vector<FailoverEventCallback> event_callbacks_;

    /**
     * @brief Monitoring Loop.
     */
    void monitoringLoop();
    /**
     * @brief Perform Health Checks.
     */
    void performHealthChecks();
    /**
     * @brief Check For Network Partitions.
     */
    void checkForNetworkPartitions();
    /**
     * @brief Detect Node Failures.
     */
    void detectNodeFailures();
    /**
     * @brief Update Failure Tracking.
     * @param[in] node_id Identifier of the node.
     * @param[in] is_healthy Input parameter.
     */
    void updateFailureTracking(const std::string& node_id, bool is_healthy);

    /**
     * @brief Perform Bounded Health Check.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool performBoundedHealthCheck(const std::string& node_id) noexcept;

    /**
     * @brief Update Adaptive Interval.
     * @param[in] last_latency Input parameter.
     */
    void updateAdaptiveInterval(std::chrono::milliseconds last_latency);

    /**
     * @brief Check And Apply Gc Grace.
     * @param[in] node_id Identifier of the node.
     * @return True when the operation succeeds.
     */
    bool checkAndApplyGcGrace(const std::string& node_id);

    /**
     * @brief Failover Loop.
     */
    void failoverLoop();
    /**
     * @brief Process Failover.
     * @param[in] task Input parameter.
     * @return Return value.
     */
    FailoverResult processFailover(const FailoverTask& task);
    /**
     * @brief Check And Wait For Quorum.
     * @return True when the operation succeeds.
     */
    bool checkAndWaitForQuorum();
    /**
     * @brief Start Leader Election.
     * @param[in] failed_node_id Identifier of the failed node.
     * @return True when the operation succeeds.
     */
    bool startLeaderElection(const std::string& failed_node_id);
    /**
     * @brief Select And Promote Replica.
     * @param[in] failed_node_id Identifier of the failed node.
     * @param[in,out] promoted_id Identifier of the promoted.
     * @return True when the operation succeeds.
     */
    bool selectAndPromoteReplica(const std::string& failed_node_id, std::string& promoted_id);
    /**
     * @brief Activate Spare If Needed.
     * @param[in] failed_node_id Identifier of the failed node.
     * @return True when the operation succeeds.
     */
    bool activateSpareIfNeeded(const std::string& failed_node_id);
    /**
     * @brief Update Metadata.
     * @param[in] old_leader_id Identifier of the old leader.
     * @param[in] new_leader_id Identifier of the new leader.
     * @return True when the operation succeeds.
     */
    bool updateMetadata(const std::string& old_leader_id, const std::string& new_leader_id);
    /**
     * @brief Verify Failover Completion.
     * @param[in] task Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyFailoverCompletion(const FailoverTask& task);

    // Split-brain prevention
    /**
     * @brief Prevent Split Brain.
     * @param[in] failed_node_id Identifier of the failed node.
     * @return True when the operation succeeds.
     */
    bool preventSplitBrain(const std::string& failed_node_id);

    /**
     * @brief Resolve Split Vote.
     * @param[in] candidates Input parameter.
     * @return Return value.
     */
    std::string resolveSplitVote(const std::vector<std::string>& candidates) const;

    // Network partition handling
    /**
     * @brief Handle Network Partition.
     * @return True when the operation succeeds.
     */
    bool handleNetworkPartition();
    /**
     * @brief Is Network Partitioned From Quorum.
     * @return True when the operation succeeds.
     */
    bool isNetworkPartitionedFromQuorum() const;

    // Recovery after failover
    /**
     * @brief Attempt Recovery.
     * @param[in] failed_node_id Identifier of the failed node.
     * @return True when the operation succeeds.
     */
    bool attemptRecovery(const std::string& failed_node_id);
    /**
     * @brief Wait For Node Recovery.
     * @param[in] node_id Identifier of the node.
     * @param[in] max_attempts Input parameter.
     * @return True when the operation succeeds.
     */
    bool waitForNodeRecovery(const std::string& node_id, uint32_t max_attempts);

    // State machine
    /**
     * @brief Transition State.
     * @param[in] new_state Input parameter.
     */
    void transitionState(FailoverOrchestratorState new_state);

    /**
     * @brief Emit Diagnostic.
     * @param[in] code Input parameter.
     * @param[in] node_id Identifier of the node.
     * @param[in] detail Input parameter.
     * @note Exception safety: noexcept.
     */
    void emitDiagnostic(FailoverErrorCode code,
                        const std::string& node_id,
                        const std::string& detail) noexcept;

    /**
     * @brief Emit Event.
     * @param[in] type Input parameter.
     * @param[in] node_id Identifier of the node.
     * @param[in] detail Input parameter.
     * @note Exception safety: noexcept.
     */
    void emitEvent(FailoverEventType type, const std::string& node_id, const std::string& detail) noexcept;
    /**
     * @brief Update Statistics.
     * @param[in] result Input parameter.
     */
    void updateStatistics(const FailoverResult& result);

    std::future<std::map<std::string, bool>> abandoned_health_check_future_;

#ifdef THEMIS_TEST_BUILD
    std::function<std::map<std::string, bool>()> health_check_override_;
    std::function<bool(const std::string&)> recovery_override_;
#endif
};

}  // namespace failover
}  // namespace themis
