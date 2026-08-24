/**
 * @file auto_failover_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.11
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <optional>
#include <queue>
#include <shared_mutex>
#include <string>
#include <thread>
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

/**
 * Failover event types for orchestration state machine.
 */
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
};

/**
 * Failover orchestration state.
 */
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

/**
 * Configuration for automatic failover behavior.
 */
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

    /// Maximum time to wait for a single health-check call before treating the node as timed-out.
    std::chrono::milliseconds health_check_call_timeout_ms{5000};       ///< 5 s default

    /// Path to the quorum log WAL file. Empty = in-memory only (not durable).
    std::string quorum_log_path;

    // ── Part B1: Adaptive health-check interval ──────────────────────────────
    /// Enable adaptive health-check interval based on rolling p95 latency.
    bool adaptive_check_interval{false};
    /// Number of samples for rolling p95 latency calculation (default 20).
    uint32_t adaptive_check_samples{20};
    /// Minimum allowed adaptive interval floor.
    std::chrono::milliseconds adaptive_check_interval_min{100};
    /// Maximum allowed adaptive interval ceiling.
    std::chrono::milliseconds adaptive_check_interval_max{5000};
    /// Number of failures in gc_grace_window that triggers GC grace period.
    uint32_t gc_grace_failure_count{3};
    /// Time window for GC grace burst detection.
    std::chrono::milliseconds gc_grace_window{1000};
    /// Grace period suppressing FAILED-state transitions after GC burst.
    std::chrono::milliseconds gc_grace_period{2000};

    // ── Part B2: Consensus quorum hardening ──────────────────────────────────
    /// Timeout for quorum wait-for loop (default 30s, matches kHeartbeatTimeout * 10).
    std::chrono::milliseconds quorum_timeout_ms{30000};
    /// Enable deterministic tie-breaking: on split-vote, smallest node_id wins.
    bool deterministic_tie_breaking{true};
    /// Heartbeat coalescing: max heartbeats per second (0 = disabled).
    uint32_t max_heartbeats_per_second{5};
};

/**
 * Result of a failover operation.
 */
struct FailoverResult {
    bool success{false};
    std::string failed_node_id;
    std::string promoted_node_id;
    std::string spare_node_id;
    std::chrono::milliseconds duration;
    std::string error_message;
};

/**
 * Automatic Failover Manager
 *
 * Orchestrates automatic failover operations across:
 * - Health monitoring
 * - Leader election
 * - Hot spare activation
 * - Network partition detection
 * - Split-brain prevention (via epoch fencing)
 *
 * Phase 4.2 Implementation: Automatic Failover Orchestration
 */
class AutoFailoverManager {
public:
    explicit AutoFailoverManager(
        const AutoFailoverConfig& config,
        std::shared_ptr<themisdb::replication::ReplicationManager> replication_mgr,
        std::shared_ptr<sharding::HealthMonitor> health_monitor,
        std::shared_ptr<sharding::HotSpareManager> spare_manager,
        std::shared_ptr<sharding::EpochFencingManager> fencing_manager
    );

    ~AutoFailoverManager();

    // Lifecycle management
    bool start();
    bool stop();
    bool isRunning() const;

    // Manual failover trigger (for testing/ops)
    bool triggerManualFailover(
        const std::string& failed_node_id,
        const std::string& target_promote_id = ""
    );

    // Query current state
    FailoverOrchestratorState getState() const;
    bool isFailoverInProgress() const;
    std::vector<std::string> getFailingNodes() const;
    std::optional<FailoverResult> getLastFailoverResult() const;

    // Configuration management
    void updateConfig(const AutoFailoverConfig& config);
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
    };

    Statistics getStatistics() const;

    // Event callback registration
    using FailoverEventCallback = std::function<void(
        FailoverEventType,
        const std::string&,  // node_id
        const std::string&   // detail message
    )>;

    void registerEventCallback(FailoverEventCallback callback);

    // State machine query — pure read, safe to call from any context.
    bool canTransition(FailoverOrchestratorState from, FailoverOrchestratorState to) const;

#ifdef THEMIS_TEST_BUILD
    // Test-only accessors for phase-gated unit coverage (Phase 2/3).
    bool testPreventSplitBrain(const std::string& node_id) {
        return preventSplitBrain(node_id);
    }
    bool testAttemptRecovery(const std::string& node_id) {
        return attemptRecovery(node_id);
    }
    void testEmitDiagnostic(FailoverErrorCode code,
                            const std::string& node_id,
                            const std::string& detail) {
        emitDiagnostic(code, node_id, detail);
    }
    // FO-IMPL-003: exposes processFailover for Wave A fencing tests.
    FailoverResult testProcessFailover(const std::string& failed_node_id) {
        FailoverTask task;
        task.failed_node_id = failed_node_id;
        task.enqueued_at    = std::chrono::steady_clock::now();
        return processFailover(task);
    }
    /// Override the health-check callable used inside performHealthChecks().
    /// When set, replaces replication_mgr_->getClusterHealth() for unit testing.
    void testSetHealthCheckOverride(
        std::function<std::map<std::string, bool>()> fn) {
        health_check_override_ = std::move(fn);
    }
#endif

private:
    // Lock order (must be acquired in this order when nesting):
    //   failover_mutex_ → stats_mutex_ → callbacks_mutex_
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
    std::map<std::string, int> consecutive_failures_;
    mutable std::shared_mutex tracking_mutex_;

    /// Monotonically increasing topology version; incremented on every node-set change.
    std::atomic<uint64_t> topology_version_{0};

    /// @brief Captures an immutable topology snapshot under tracking_mutex_.
    /// @thread_safety Caller must hold tracking_mutex_ (shared or exclusive).
    TopologySnapshot captureTopologySnapshot() const;
    std::optional<FailoverResult> last_failover_result_;

    // ── Part B1: Adaptive interval + GC grace state ──────────────────────────
    /// Rolling latency samples for adaptive interval calculation. Protected by monitor_mutex_.
    std::vector<std::chrono::milliseconds> health_check_latency_samples_;
    /// Timestamps of recent failures for GC grace burst detection. Protected by tracking_mutex_.
    std::vector<std::chrono::steady_clock::time_point> recent_failure_timestamps_;
    /// Expiry time of the active GC grace period. Zero = no active grace period.
    std::chrono::steady_clock::time_point gc_grace_expiry_{};
    /// Current adaptive check interval (updated when adaptive_check_interval=true).
    std::chrono::milliseconds current_check_interval_{500};

    // ── Part B2: Quorum-hardening coalescing state ────────────────────────────
    /// Last heartbeat coalescing window start. Protected by failover_mutex_.
    std::chrono::steady_clock::time_point heartbeat_coalesce_window_start_{};
    /// Heartbeat count in current coalescing window. Protected by failover_mutex_.
    uint32_t heartbeat_coalesce_count_{0};

    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::vector<std::chrono::milliseconds> failover_durations_;

    // Event callbacks
    mutable std::mutex callbacks_mutex_;
    std::vector<FailoverEventCallback> event_callbacks_;

    // Helper methods - monitoring loop
    /// @brief Main monitoring loop; runs on monitoring_thread_. Polls health, partitions, failures.
    /// @thread_safety Must only be called from monitoring_thread_.
    void monitoringLoop();
    /// @brief Performs a bounded health-check round for all monitored nodes.
    ///        Each individual call is capped by health_check_call_timeout_ms (default 5 s).
    ///        On timeout, emits HEARTBEAT_MISSED diagnostic.
    /// @thread_safety Called from monitoringLoop; must not be called from other threads.
    void performHealthChecks();
    /// @brief Checks for evidence of a network partition and triggers handling if detected.
    /// @thread_safety Called from monitoringLoop.
    void checkForNetworkPartitions();
    /// @brief Evaluates tracked failure counts and enqueues a FailoverTask if threshold exceeded.
    ///        Uses topology snapshots to detect concurrent topology changes; retries up to 3 times.
    /// @thread_safety Called from monitoringLoop under failover_mutex_.
    void detectNodeFailures();
    /// @brief Updates the per-node failure counter.
    ///        Increments topology_version_ atomically when failure state changes.
    /// @param node_id   The node whose health status changed.
    /// @param is_healthy True if the node is currently healthy.
    /// @thread_safety Must be called under tracking_mutex_.
    void updateFailureTracking(const std::string& node_id, bool is_healthy);

    /// Performs a single bounded health-check for one node.
    /// @returns true if the node is healthy; false if unhealthy or timed-out.
    bool performBoundedHealthCheck(const std::string& node_id) noexcept;

    // ── Part B1: Adaptive interval + GC grace helpers ─────────────────────────
    /// @brief Updates adaptive check interval from rolling p95 latency.
    /// @param last_latency Duration of the most recent health-check call.
    /// @thread_safety Caller must hold monitor_mutex_.
    void updateAdaptiveInterval(std::chrono::milliseconds last_latency);

    /// @brief Checks if GC grace period applies; activates grace period on burst.
    /// @param node_id Node that experienced the failure.
    /// @returns true if the failure should be suppressed (grace period active).
    /// @thread_safety Must be called under tracking_mutex_ (exclusive).
    bool checkAndApplyGcGrace(const std::string& node_id);

    // Helper methods - failover orchestration
    /// @brief Main failover orchestration loop; drains the failover task queue.
    /// @thread_safety Must only be called from failover_thread_.
    void failoverLoop();
    /// @brief Processes a single failover task end-to-end.
    ///        Transitions state machine through VERIFYING_FAILURE → CHECKING_QUORUM →
    ///        STARTING_LEADER_ELECTION → UPDATING_METADATA → COMPLETING_FAILOVER.
    /// @param task The failover task to execute.
    /// @returns FailoverResult with success flag, promoted node id, and error detail.
    /// @thread_safety Must only be called from failoverLoop.
    FailoverResult processFailover(const FailoverTask& task);
    /// @brief Waits for cluster quorum to be confirmed; persists QUORUM_REACHED to QuorumLog.
    /// @returns true if quorum reached within quorum_timeout_ms; false otherwise.
    ///          Fail-closed: returns false if QuorumLog write fails.
    /// @thread_safety Must only be called from failoverLoop.
    bool checkAndWaitForQuorum();
    bool startLeaderElection(const std::string& failed_node_id);
    /// @brief Selects the best available replica and promotes it to primary.
    ///        Persists PROMOTE to QuorumLog before promotion.
    ///        Verifies fencing via preventSplitBrain() before any replica promotion.
    /// @param failed_node_id  The node that failed and must be replaced.
    /// @param[out] promoted_id  Set to the node ID of the promoted replica on success.
    /// @returns true if promotion succeeded.
    /// @thread_safety Must only be called from failoverLoop.
    bool selectAndPromoteReplica(const std::string& failed_node_id, std::string& promoted_id);
    bool activateSpareIfNeeded(const std::string& failed_node_id);
    bool updateMetadata(const std::string& old_leader_id, const std::string& new_leader_id);
    bool verifyFailoverCompletion(const FailoverTask& task);

    // Split-brain prevention
    /// @brief Prevents split-brain by acquiring an exclusive epoch fence.
    /// @details Fails closed when no EpochFencingManager is configured and
    ///          enable_split_brain_prevention is true. Emits SPLIT_BRAIN_DETECTED
    ///          diagnostic on failure.
    /// @param failed_node_id Node being failed over.
    /// @returns true if fencing succeeded or prevention is disabled; false if failed closed.
    /// @thread_safety Must be called from failoverLoop thread only.
    bool preventSplitBrain(const std::string& failed_node_id);

    /// @brief Selects the winning candidate on split-vote using deterministic tie-breaking.
    /// @param candidates Non-empty vector of candidate node IDs with equal vote counts.
    /// @returns The tie-breaking winner (smallest lexicographic node_id).
    /// @thread_safety No locks required; operates on the passed-in copy.
    std::string resolveSplitVote(const std::vector<std::string>& candidates) const;

    // Network partition handling
    bool handleNetworkPartition();
    bool isNetworkPartitionedFromQuorum() const;

    // Recovery after failover
    bool attemptRecovery(const std::string& failed_node_id);
    bool waitForNodeRecovery(const std::string& node_id, uint32_t max_attempts);

    // State machine
    /// @brief Transitions the orchestrator state machine to new_state.
    ///        Logs a warning if the transition is not in the canonical table.
    ///        Valid canonical transitions:
    ///        IDLE → VERIFYING_FAILURE → CHECKING_QUORUM →
    ///        STARTING_LEADER_ELECTION → LEADER_ELECTION_IN_PROGRESS →
    ///        UPDATING_METADATA → COMPLETING_FAILOVER → IDLE.
    ///        FAILED is reachable from any state; IDLE is always reachable as reset.
    /// @param new_state Target state.
    /// @thread_safety Must be called under failover_mutex_.
    void transitionState(FailoverOrchestratorState new_state);

    // Unified diagnostics helper — logs the canonical error code and fires event callbacks.
    // Exception-safe guarantee: Basic (noexcept wrapper ensures no exceptions escape to caller)
    void emitDiagnostic(FailoverErrorCode code,
                        const std::string& node_id,
                        const std::string& detail) noexcept;

    // Logging and callbacks
    // Exception-safe guarantee: Basic (catches all exceptions from callbacks internally)
    void emitEvent(FailoverEventType type, const std::string& node_id, const std::string& detail) noexcept;
    void updateStatistics(const FailoverResult& result);

#ifdef THEMIS_TEST_BUILD
    /// Injectable health-check override; used only in unit tests.
    std::function<std::map<std::string, bool>()> health_check_override_;
#endif
};

}  // namespace failover
}  // namespace themis
