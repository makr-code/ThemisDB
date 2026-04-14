/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            health_monitor.h                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     223                                            ║
    • Open Issues:     TODOs: 1, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * Node Health Status
 */
enum class HealthStatus {
    HEALTHY = 0,      // Responding to heartbeats
    SUSPECT = 1,      // Missed 1-2 heartbeats
    DOWN = 2,         // Missed 3+ heartbeats
    RECOVERING = 3    // Coming back online
};

/**
 * Health Check Result
 */
struct HealthCheckResult {
    std::string node_id;
    HealthStatus status;
    std::chrono::steady_clock::time_point last_check;
    std::chrono::milliseconds response_time;
    uint32_t consecutive_failures = 0;
    uint32_t consecutive_successes = 0;
    std::string error_message;
    
    bool isHealthy() const {
        return status == HealthStatus::HEALTHY || status == HealthStatus::RECOVERING;
    }
};

/**
 * Failover Action
 */
enum class FailoverAction {
    NONE = 0,
    PROMOTE_STANDBY = 1,      // Promote standby to active
    ROUTE_TO_BACKUP = 2,      // Route traffic to backup primary
    MARK_DEGRADED = 3         // Mark primary as degraded (partial availability)
};

/**
 * Failover Event
 */
struct FailoverEvent {
    std::string failed_node_id;
    std::string promoted_node_id;  // Empty if no promotion
    FailoverAction action;
    std::chrono::steady_clock::time_point timestamp;
    std::string reason;
};

/**
 * Health Monitor Configuration
 */
struct HealthMonitorConfig {
    std::chrono::milliseconds heartbeat_interval{1000};  // 1 second
    std::chrono::milliseconds health_check_timeout{500}; // 500ms per check
    uint32_t max_consecutive_failures = 3;  // Mark DOWN after 3 failures
    uint32_t successes_for_recovery = 3;     // Consecutive successes needed for RECOVERING → HEALTHY
    
    bool auto_failover_enabled = true;
    bool auto_promote_standby = true;
    std::chrono::milliseconds failover_cooldown{10000};  // 10 seconds between failovers
    
    // HTTP health check endpoint
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
    HealthMonitor(const HealthMonitorConfig& config,
                  std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                  std::shared_ptr<ReplicaTopology> topology);
    
    // Constructor with custom HTTP client pool
    HealthMonitor(const HealthMonitorConfig& config,
                  std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                  std::shared_ptr<ReplicaTopology> topology,
                  std::shared_ptr<utils::HTTPClientPool> http_pool);
    
    // Constructor with custom HTTP client pool and ThreadPoolManager
    HealthMonitor(const HealthMonitorConfig& config,
                  std::shared_ptr<MultiPrimaryCoordinator> primary_coordinator,
                  std::shared_ptr<ReplicaTopology> topology,
                  std::shared_ptr<utils::HTTPClientPool> http_pool,
                  std::shared_ptr<utils::ThreadPoolManager> thread_pool);
    
    ~HealthMonitor();
    
    /**
     * Start monitoring thread
     */
    void start();
    
    /**
     * Stop monitoring thread
     */
    void stop();
    
    /**
     * Check health of a specific node (synchronous)
     */
    HealthCheckResult checkNodeHealth(const std::string& node_id, const std::string& endpoint);
    
    /**
     * Get health status of all nodes
     */
    std::map<std::string, HealthCheckResult> getAllHealthStatuses() const;
    
    /**
     * Get health status of specific node
     */
    std::optional<HealthCheckResult> getHealthStatus(const std::string& node_id) const;
    
    /**
     * Manually trigger failover
     */
    bool triggerManualFailover(const std::string& failed_node_id, 
                               const std::string& promote_node_id);
    
    /**
     * Disable auto-failover (for maintenance)
     */
    void setAutoFailoverEnabled(bool enabled);
    
    /**
     * Get failover history (last N events)
     */
    std::vector<FailoverEvent> getFailoverHistory(size_t max_events = 10) const;
    
    /**
     * Get statistics
     */
    struct Statistics {
        uint64_t total_health_checks = 0;
        uint64_t failed_health_checks = 0;
        uint64_t auto_failovers_triggered = 0;
        uint64_t manual_failovers_triggered = 0;
        std::chrono::steady_clock::time_point last_failover_time;
    };
    
    Statistics getStatistics() const;

private:
    void monitoringLoop();
    void performHealthChecks();
    void handleNodeFailure(const std::string& node_id);
    bool shouldTriggerFailover(const std::string& node_id) const;
    std::optional<std::string> selectStandbyForPromotion() const;
    void recordFailoverEvent(const FailoverEvent& event);
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
