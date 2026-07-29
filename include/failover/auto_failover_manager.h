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

/*
 * ThemisDB | File: auto_failover_manager.h | Version: 0.0.11 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 271
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5043 [Docs][failover] refresh mo... (2026-05-12) | #4553 [MODULE] failover: Phase 4 ... (2026-04-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
#endif

private:
    // Lock order (must be acquired in this order when nesting):
    //   failover_mutex_ → stats_mutex_ → callbacks_mutex_
    // Configuration and managers
    AutoFailoverConfig config_;
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

    std::queue<FailoverTask> failover_queue_;

    // Tracking
    std::map<std::string, int> consecutive_failures_;
    mutable std::shared_mutex tracking_mutex_;
    std::optional<FailoverResult> last_failover_result_;

    // Statistics
    mutable std::mutex stats_mutex_;
    Statistics stats_;
    std::vector<std::chrono::milliseconds> failover_durations_;

    // Event callbacks
    mutable std::mutex callbacks_mutex_;
    std::vector<FailoverEventCallback> event_callbacks_;

    // Helper methods - monitoring loop
    void monitoringLoop();
    void performHealthChecks();
    void checkForNetworkPartitions();
    void detectNodeFailures();
    void updateFailureTracking(const std::string& node_id, bool is_healthy);

    // Helper methods - failover orchestration
    void failoverLoop();
    FailoverResult processFailover(const FailoverTask& task);
    bool checkAndWaitForQuorum();
    bool startLeaderElection(const std::string& failed_node_id);
    bool selectAndPromoteReplica(const std::string& failed_node_id, std::string& promoted_id);
    bool activateSpareIfNeeded(const std::string& failed_node_id);
    bool updateMetadata(const std::string& old_leader_id, const std::string& new_leader_id);
    bool verifyFailoverCompletion(const FailoverTask& task);

    // Split-brain prevention
    bool preventSplitBrain(const std::string& failed_node_id);

    // Network partition handling
    bool handleNetworkPartition();
    bool isNetworkPartitionedFromQuorum() const;

    // Recovery after failover
    bool attemptRecovery(const std::string& failed_node_id);
    bool waitForNodeRecovery(const std::string& node_id, uint32_t max_attempts);

    // State machine
    void transitionState(FailoverOrchestratorState new_state);

    // Unified diagnostics helper — logs the canonical error code and fires event callbacks.
    void emitDiagnostic(FailoverErrorCode code,
                        const std::string& node_id,
                        const std::string& detail);

    // Logging and callbacks
    void emitEvent(FailoverEventType type, const std::string& node_id, const std::string& detail);
    void updateStatistics(const FailoverResult& result);
};

}  // namespace failover
}  // namespace themis
