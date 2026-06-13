/**
 * @file health_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include "sharding/multi_primary_coordinator.h"
#include "sharding/replica_topology.h"
#include "utils/http_client_pool.h"
#include "utils/thread_pool_manager.h"

namespace themis::sharding {

/** @brief Node-level health state used by failover state machine. */
enum class HealthStatus {
    HEALTHY = 0,      // Responding to heartbeats
    SUSPECT = 1,      // Missed 1-2 heartbeats
    DOWN = 2,         // Missed 3+ heartbeats
    RECOVERING = 3    // Coming back online
};

/** @brief Result payload for one node health-check cycle. */
struct HealthCheckResult {
    /** @brief Node identifier for the checked target. */
    std::string node_id;
    /** @brief Current computed health state after state-machine transition. */
    HealthStatus status;
    /** @brief Timestamp of most recent check execution. */
    std::chrono::steady_clock::time_point last_check;
    /** @brief End-to-end health endpoint response latency. */
    std::chrono::milliseconds response_time;
    /** @brief Consecutive failed checks. */
    uint32_t consecutive_failures = 0;
    /** @brief Consecutive successful checks. */
    uint32_t consecutive_successes = 0;
    /** @brief Diagnostic message for failures/timeouts. */
    std::string error_message;
    
    /**
     * @brief Convenience predicate for operationally healthy states.
     * @return true for HEALTHY and RECOVERING states.
     */
    bool isHealthy() const {
        return status == HealthStatus::HEALTHY || status == HealthStatus::RECOVERING;
    }
};

/** @brief Action type recorded for one failover event. */
enum class FailoverAction {
    NONE = 0,
    PROMOTE_STANDBY = 1,      // Promote standby to active
    ROUTE_TO_BACKUP = 2,      // Route traffic to backup primary
    MARK_DEGRADED = 3         // Mark primary as degraded (partial availability)
};

/** @brief Audit event describing one manual/automatic failover decision. */
struct FailoverEvent {
    /** @brief Failed source node id that triggered routing/promotion decision. */
    std::string failed_node_id;
    /** @brief Promoted node id (empty when action does not promote). */
    std::string promoted_node_id;  // Empty if no promotion
    /** @brief Effective failover action taken. */
    FailoverAction action;
    /** @brief Event timestamp. */
    std::chrono::steady_clock::time_point timestamp;
    /** @brief Human-readable reason text for audit trail. */
    std::string reason;
};

/** @brief Runtime tuning for heartbeat checks and failover behavior. */
struct HealthMonitorConfig {
    /** @brief Period between monitoring iterations. */
    std::chrono::milliseconds heartbeat_interval{1000};  // 1 second
    /** @brief Timeout for each HTTP health check request. */
    std::chrono::milliseconds health_check_timeout{500}; // 500ms per check
    /** @brief Failure threshold before node is considered DOWN. */
    uint32_t max_consecutive_failures = 3;  // Mark DOWN after 3 failures
    /** @brief Success threshold to transition RECOVERING to HEALTHY. */
    uint32_t successes_for_recovery = 3;     // Consecutive successes needed for RECOVERING → HEALTHY
    
    /** @brief Enable or disable automatic failover handling. */
    bool auto_failover_enabled = true;
    /** @brief Allow automatic standby promotion when failover triggers. */
    bool auto_promote_standby = true;
    /** @brief Minimum interval between automatic failover actions. */
    std::chrono::milliseconds failover_cooldown{10000};  // 10 seconds between failovers
    
    // HTTP health check endpoint
    /** @brief Relative health check path appended to node endpoint base URL. */
    std::string health_check_path = "/health";
};

/**
 * Health Monitor
 * 
 * Monitors health of all nodes (primaries + replicas) and triggers auto-failover.
 * Features:
 * - Periodic heartbeat checks (HTTP ping)
 * - State machine: HEALTHY → SUSPECT → DOWN → RECOVERING
 * - Auto-failover: Promotes standby when active primary fails
 * - Manual override: Operators can disable auto-promotion
 * - Event log for audit trail
 */
class HealthMonitor {
public:
    /**
     * @brief Construct monitor with default HTTP client pool.
     * @param config Health monitor configuration.
     * @param primary_coordinator Primary failover coordinator.
     * @param topology Replica topology provider.
     */
    HealthMonitor(const HealthMonitorConfig& config,
                  std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                  std::shared_ptr<ReplicaTopology> topology);
    
    /**
     * @brief Construct monitor with custom HTTP client pool.
     */
    HealthMonitor(const HealthMonitorConfig& config,
                  std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                  std::shared_ptr<ReplicaTopology> topology,
                  std::shared_ptr<utils::HTTPClientPool> http_pool);
    
    /**
     * @brief Construct monitor with custom HTTP pool and thread-pool manager.
     */
    HealthMonitor(const HealthMonitorConfig& config,
                  std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                  std::shared_ptr<ReplicaTopology> topology,
                  std::shared_ptr<utils::HTTPClientPool> http_pool,
                  std::shared_ptr<utils::ThreadPoolManager> thread_pool);
    
    ~HealthMonitor();
    
    /** @brief Start periodic health monitoring loop. */
    void start();
    
    /** @brief Stop periodic health monitoring loop. */
    void stop();
    
    /**
     * @brief Check health of a specific node synchronously.
     * @param node_id Node identifier.
     * @param endpoint HTTP health endpoint.
     * @return Structured health-check result.
     */
    HealthCheckResult checkNodeHealth(const std::string& node_id, const std::string& endpoint);
    
    /** @brief Get health status snapshots for all known nodes. */
    std::map<std::string, HealthCheckResult> getAllHealthStatuses() const;
    
    /**
     * @brief Get health status for one node.
     * @param node_id Node identifier.
     * @return Status snapshot when available; std::nullopt otherwise.
     */
    std::optional<HealthCheckResult> getHealthStatus(const std::string& node_id) const;
    
    /**
     * @brief Trigger manual failover from failed node to target node.
     * @param failed_node_id Node considered failed.
     * @param promote_node_id Node to promote.
     * @return true on successful promotion.
     */
    bool triggerManualFailover(const std::string& failed_node_id, 
                               const std::string& promote_node_id);
    
    /**
     * @brief Enable/disable automatic failover.
     * @param enabled True to allow automatic failover.
     */
    void setAutoFailoverEnabled(bool enabled);
    
    /**
     * @brief Get failover history tail.
     * @param max_events Maximum number of most recent events.
     * @return Chronological subset of failover events.
     */
    std::vector<FailoverEvent> getFailoverHistory(size_t max_events = 10) const;
    
    /** @brief Statistics snapshot for health/failover activity. */
    struct Statistics {
        /** @brief Count of executed health checks. */
        uint64_t total_health_checks = 0;
        /** @brief Count of failed/timed-out health checks. */
        uint64_t failed_health_checks = 0;
        /** @brief Count of automatically triggered failovers. */
        uint64_t auto_failovers_triggered = 0;
        /** @brief Count of manually triggered failovers. */
        uint64_t manual_failovers_triggered = 0;
        /** @brief Timestamp of most recent failover event. */
        std::chrono::steady_clock::time_point last_failover_time;
    };
    
    /** @brief Return monitor-wide statistics snapshot. */
    Statistics getStatistics() const;

private:
    /** @brief Main background loop executing periodic checks. */
    void monitoringLoop();
    /** @brief Perform one full health-check pass over primaries and replicas. */
    void performHealthChecks();
    /** @brief Handle transition of a node into DOWN state. */
    void handleNodeFailure(const std::string& node_id);
    /** @brief Return whether failover is currently allowed by policy/cooldown. */
    bool shouldTriggerFailover(const std::string& node_id) const;
    /** @brief Select best standby candidate for promotion. */
    std::optional<std::string> selectStandbyForPromotion() const;
    /** @brief Persist one failover event into history and update timestamps. */
    void recordFailoverEvent(const FailoverEvent& event);
    /** @brief Execute HTTP health probe against endpoint URL. */
    bool performHealthCheck(const std::string& endpoint);
    
    HealthMonitorConfig config_;
    std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator_;
    std::shared_ptr<ReplicaTopology> topology_;
    std::shared_ptr<utils::HTTPClientPool> http_pool_;
    std::shared_ptr<utils::ThreadPoolManager> thread_pool_;
    
    mutable std::mutex mutex_;
    std::atomic<bool> running_{false};
    // TODO(v2.0): Remove monitor_thread_ once all clients migrate to ThreadPoolManager
    std::thread monitor_thread_;  // Kept for backward compatibility
    
    std::map<std::string, HealthCheckResult> health_statuses_;
    std::vector<FailoverEvent> failover_history_;
    std::chrono::steady_clock::time_point last_failover_time_;
    
    // Statistics
    std::atomic<uint64_t> total_health_checks_{0};
    std::atomic<uint64_t> failed_health_checks_{0};
    std::atomic<uint64_t> auto_failovers_{0};
    std::atomic<uint64_t> manual_failovers_{0};
};

} // namespace themis::sharding
