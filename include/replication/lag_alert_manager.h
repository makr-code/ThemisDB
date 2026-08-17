// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file lag_alert_manager.h
 * @brief Replication Lag Alert Manager API for ThemisDB replication.
 *
 * Monitors replication lag across all replicas and triggers alerts when lag
 * exceeds configured SLO thresholds. Supports tiered alerting:
 * - Alert threshold (10s default): Initial notification
 * - Critical threshold (30s default): Escalated alert
 * - Failover threshold (60s default): Trigger automatic failover if sustained
 *
 * ### Features
 * - Per-replica lag tracking with configurable SLO thresholds
 * - Automatic failover initiation after sustained critical lag (5 min default)
 * - Prometheus-format metrics export
 * - Thread-safe all public methods
 * - Configurable via environment variables and code
 *
 * ### Metrics exported
 * - `replication_lag_ms{replica_id}` — Current lag per replica
 * - `lag_alert_triggered{replica_id}` — Lag exceeded alert threshold
 * - `lag_critical_triggered{replica_id}` — Lag exceeded critical threshold
 * - `failover_initiated{replica_id}` — Failover due to sustained high lag
 *
 * @see include/replication/replication_manager.h
 * @see src/replication/ROADMAP.md — §3.2 Replication lag monitoring and SLO enforcement
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace themisdb {
namespace replication {

// ============================================================================
// ReplicaLag — Per-replica lag tracking
// ============================================================================

/**
 * @brief Current replication lag for a single replica.
 *
 * Lag is measured as the time elapsed since the latest committed transaction
 * on the leader minus the time it was applied on the replica (or an approximation
 * thereof based on sequence numbers and wall-clock time).
 */
struct ReplicaLag {
    std::string             replica_id;        ///< Unique replica identifier
    int64_t                 lag_ms = 0;        ///< Current lag in milliseconds
    int64_t                 last_update_ms = 0; ///< Unix epoch ms of last update
    int64_t                 max_lag_since_alert_ms = 0; ///< Peak lag since last alert
};

// ============================================================================
// SLOThresholds — Configurable alert thresholds
// ============================================================================

/**
 * @brief SLO thresholds for replication lag monitoring.
 *
 * Defines the tiered alert strategy:
 * - alert_threshold_ms: Lag must exceed this to trigger an "alert" (warning-level)
 * - critical_threshold_ms: Lag must exceed this to trigger "critical" (error-level)
 * - failover_threshold_ms: Lag must exceed this to consider failover
 * - failover_duration_ms: How long critical lag must persist to trigger failover
 *
 * All values are in milliseconds. A value of 0 disables the threshold.
 */
struct SLOThresholds {
    int64_t alert_threshold_ms = 10'000;        ///< 10 seconds: initial alert
    int64_t critical_threshold_ms = 30'000;     ///< 30 seconds: escalation
    int64_t failover_threshold_ms = 60'000;     ///< 60 seconds: failover trigger
    int64_t failover_duration_ms = 5 * 60'000;  ///< 5 minutes: sustained critical
};

// ============================================================================
// AlertEvent — Emitted when thresholds are crossed
// ============================================================================

/**
 * @brief Event describing a lag alert condition.
 */
struct AlertEvent {
    enum class Level {
        ALERT,      ///< Warning-level: lag exceeded alert threshold
        CRITICAL,   ///< Error-level: lag exceeded critical threshold
        FAILOVER    ///< Failover initiated due to sustained critical lag
    };

    Level               level;
    std::string         replica_id;
    int64_t             lag_ms;
    std::string         message;
    int64_t             timestamp_ms; ///< Unix epoch milliseconds
};

// ============================================================================
// AlertCallback — Invoked when alerts trigger
// ============================================================================

/**
 * @brief Callback signature for alert notifications.
 *
 * Invoked when a lag threshold is crossed (alert, critical, or failover).
 * Implementations should be short-lived and exception-safe.
 */
using AlertCallback = std::function<void(const AlertEvent&)>;

// ============================================================================
// LagAlertManager — Main API
// ============================================================================

/**
 * @brief Replication lag monitoring and alert management.
 *
 * Tracks lag for each replica and triggers alerts when SLO thresholds are
 * exceeded. Supports automatic failover initiation when critical lag persists
 * for a configured duration.
 *
 * ### Thread safety
 * All public methods are thread-safe and may be called concurrently from
 * multiple threads.
 *
 * ### Usage example
 * @code
 *   LagAlertManager lag_monitor;
 *
 *   SLOThresholds thresholds;
 *   thresholds.alert_threshold_ms = 5000;      // 5 seconds
 *   thresholds.critical_threshold_ms = 15000;  // 15 seconds
 *   thresholds.failover_threshold_ms = 45000;  // 45 seconds
 *   thresholds.failover_duration_ms = 3 * 60000; // 3 minutes
 *
 *   lag_monitor.setThresholds(thresholds);
 *   lag_monitor.setAlertCallback([](const AlertEvent& evt) {
 *       std::cerr << "LAG ALERT [" << (int)evt.level << "]: "
 *                  << evt.replica_id << " lag=" << evt.lag_ms << "ms\n";
 *   });
 *
 *   // Periodically update replica lag (e.g., from heartbeat responses)
 *   lag_monitor.updateReplicaLag("replica-1", 5500);
 *   lag_monitor.updateReplicaLag("replica-2", 12000);
 *
 *   // Periodically check and emit alerts
 *   lag_monitor.checkAndAlertLagViolations();
 *
 *   // Export metrics
 *   std::cout << lag_monitor.exportPrometheusMetrics();
 * @endcode
 */
class LagAlertManager {
public:
    // -----------------------------------------------------------------------
    // Construction / destruction
    // -----------------------------------------------------------------------

    LagAlertManager();
    ~LagAlertManager() = default;

    // Non-copyable; movable
    LagAlertManager(const LagAlertManager&)            = delete;
    LagAlertManager& operator=(const LagAlertManager&) = delete;
    LagAlertManager(LagAlertManager&&)                 = default;
    LagAlertManager& operator=(LagAlertManager&&)      = default;

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Set the SLO thresholds for all replicas.
     *
     * Thread-safe. Changes apply immediately to future lag checks.
     *
     * @param thresholds New SLO thresholds (copied internally).
     */
    void setThresholds(const SLOThresholds& thresholds);

    /**
     * @brief Return the current SLO thresholds.
     * Thread-safe.
     */
    SLOThresholds thresholds() const;

    /**
     * @brief Register a callback to be invoked on alert events.
     *
     * Thread-safe. The callback receives AlertEvent by const reference.
     * Implementations should avoid blocking operations.
     *
     * @param callback Function to invoke; nullptr to clear.
     */
    void setAlertCallback(AlertCallback callback);

    // -----------------------------------------------------------------------
    // Replica lag updates
    // -----------------------------------------------------------------------

    /**
     * @brief Update the current lag for a replica.
     *
     * Thread-safe. Typically called from replication heartbeat handlers or
     * periodic lag measurement tasks. Updates timestamp automatically.
     *
     * @param replica_id   Unique replica identifier.
     * @param lag_ms       Current lag in milliseconds.
     */
    void updateReplicaLag(const std::string& replica_id, int64_t lag_ms);

    /**
     * @brief Update lag for multiple replicas at once (atomic batch).
     *
     * More efficient than calling updateReplicaLag() multiple times.
     * Thread-safe.
     *
     * @param lags Map of replica_id → lag_ms.
     */
    void updateReplicaLags(const std::map<std::string, int64_t>& lags);

    /**
     * @brief Get the current lag for a specific replica.
     *
     * Thread-safe.
     * @param replica_id Replica to query.
     * @return Current lag in milliseconds, or 0 if replica not tracked.
     */
    int64_t getReplicaLag(const std::string& replica_id) const;

    /**
     * @brief Remove a replica from lag tracking.
     *
     * Called when a replica is removed from the cluster. Thread-safe.
     *
     * @param replica_id Replica to remove.
     */
    void removeReplica(const std::string& replica_id);

    /**
     * @brief Clear all tracked replicas.
     *
     * Thread-safe.
     */
    void clearAllReplicas();

    // -----------------------------------------------------------------------
    // Alert evaluation
    // -----------------------------------------------------------------------

    /**
     * @brief Check all replicas for lag violations and emit alerts if needed.
     *
     * Should be called periodically (e.g., every 5-10 seconds) to evaluate
     * lag thresholds and trigger alerts. May invoke the registered callback
     * zero or more times depending on how many thresholds are crossed.
     *
     * Thread-safe. Blocks briefly to acquire internal locks.
     *
     * @return true if any alerts were fired; false otherwise.
     */
    bool checkAndAlertLagViolations();

    /**
     * @brief Check a specific replica for lag violations.
     *
     * Thread-safe.
     * @param replica_id Replica to check.
     * @return true if any alert was fired for this replica; false otherwise.
     */
    bool checkReplicaLag(const std::string& replica_id);

    /**
     * @brief Return list of replicas currently exceeding alert threshold.
     *
     * Thread-safe snapshot; the list may change after the call returns.
     *
     * @return Vector of replica IDs with lag > alert_threshold_ms.
     */
    std::vector<std::string> replicasInAlert() const;

    /**
     * @brief Return list of replicas currently at critical lag level.
     *
     * Thread-safe snapshot.
     *
     * @return Vector of replica IDs with lag > critical_threshold_ms.
     */
    std::vector<std::string> replicasInCritical() const;

    /**
     * @brief Return list of replicas currently with sustained critical lag.
     *
     * These are replicas that have been above critical_threshold_ms for longer
     * than failover_duration_ms and are eligible for automatic failover.
     *
     * Thread-safe snapshot.
     *
     * @return Vector of replica IDs eligible for failover due to lag.
     */
    std::vector<std::string> replicasEligibleForFailover() const;

    // -----------------------------------------------------------------------
    // Metrics
    // -----------------------------------------------------------------------

    /**
     * @brief Get current lag for all tracked replicas.
     *
     * Thread-safe snapshot.
     *
     * @return Map of replica_id → current lag_ms.
     */
    std::map<std::string, int64_t> allReplicaLags() const;

    /**
     * @brief Export Prometheus-format metrics text.
     *
     * Exposes per-replica metrics:
     * - `replication_lag_ms{replica_id}` — Current lag
     * - `lag_alert_triggered{replica_id}` — Total alert triggers
     * - `lag_critical_triggered{replica_id}` — Total critical triggers
     * - `failover_initiated{replica_id}` — Total failover initiations
     *
     * Thread-safe.
     */
    std::string exportPrometheusMetrics() const;

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    /**
     * @brief Get alert statistics for a replica.
     *
     * Thread-safe.
     *
     * @param replica_id Replica to query.
     * @return Tuple of (alert_count, critical_count, failover_count), or
     *         all zeros if replica not tracked.
     */
    std::tuple<uint64_t, uint64_t, uint64_t> getAlertStats(
        const std::string& replica_id) const;

private:
    // -----------------------------------------------------------------------
    // Internal state
    // -----------------------------------------------------------------------

    struct ReplicaState {
        ReplicaLag      lag;
        bool            in_alert = false;
        bool            in_critical = false;
        std::chrono::steady_clock::time_point critical_start_time;

        uint64_t        alert_count = 0;
        uint64_t        critical_count = 0;
        uint64_t        failover_count = 0;
    };

    mutable std::mutex              state_mutex_;
    std::map<std::string, ReplicaState> replicas_;
    SLOThresholds                   thresholds_;
    AlertCallback                   alert_callback_;

    // -----------------------------------------------------------------------
    // Helper methods
    // -----------------------------------------------------------------------

    /**
     * Check if a replica is transitioning into or currently in alert state.
     * Must be called with state_mutex_ held.
     */
    void evaluateReplicaAlerts(ReplicaState& state, const std::string& replica_id);

    /**
     * Emit an alert event if a callback is registered.
     */
    void emitAlert(const AlertEvent& event);

    /**
     * Get current time in milliseconds since Unix epoch.
     */
    static int64_t currentTimeMs();
};

} // namespace replication
} // namespace themisdb
