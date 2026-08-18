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
 * multiple threads. Mutual exclusion is maintained via state_mutex_.
 *
 * ### Alert semantics
 * Three alert levels are supported:
 * - ALERT (warning): lag >= alert_threshold_ms
 * - CRITICAL (error): lag >= critical_threshold_ms
 * - FAILOVER: critical lag persists for failover_duration_ms
 *
 * ### Callbacks
 * Alert callbacks receive AlertEvent by const reference. Callback implementations
 * should be short-lived and exception-safe. Exceptions from callbacks are caught
 * and suppressed to ensure alert infrastructure stability.
 *
 * ### Lock hierarchy
 * LOCK HIERARCHY (to prevent deadlock):
 * - state_mutex_: Protects all internal state (replicas_, thresholds_, callbacks).
 *   This is the only lock used; simple single-lock design avoids complex ordering.
 * - Callbacks are invoked OUTSIDE the lock to prevent deadlock if callback tries
 *   to update replica lag.
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

    /**
     * @brief Construct an alert manager with default SLO thresholds.
     * 
     * Default thresholds:
     * - alert_threshold_ms: 10 s
     * - critical_threshold_ms: 30 s
     * - failover_threshold_ms: 60 s
     * - failover_duration_ms: 5 min
     */
    LagAlertManager();

    /**
     * @brief Destructor. 
     * 
     * Thread-safe; any callbacks still pending are NOT invoked.
     * 
     * @note Exception safety: Noexcept.
     */
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
     * New thresholds do NOT retroactively reset alert state; ongoing alerts
     * continue until lag drops below the alert threshold.
     *
     * @param thresholds New SLO thresholds (copied internally).
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
     * @note A threshold of 0 disables that alert level.
     */
    void setThresholds(const SLOThresholds& thresholds);

    /**
     * @brief Return the current SLO thresholds.
     * 
     * Thread-safe.
     * 
     * @return Current SLO thresholds (snapshot).
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
     */
    SLOThresholds thresholds() const;

    /**
     * @brief Register a callback to be invoked on alert events.
     *
     * Thread-safe. The callback receives AlertEvent by const reference.
     * Implementations should avoid blocking operations and exception handling.
     *
     * @param callback Function to invoke; nullptr to clear.
     *                 Callback is invoked OUTSIDE the internal lock to prevent
     *                 deadlock if callback updates replica lag.
     *                 Callback exceptions are caught and suppressed.
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
     * @note Callback lifetime: Callback is copied; caller retains ownership.
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
     * Creates the replica entry if it doesn't exist (auto-vivification).
     *
     * @param replica_id   Unique replica identifier.
     * @param lag_ms       Current lag in milliseconds.
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
     * @note Side effect: Tracks maximum lag since last alert; useful for diagnostics.
     */
    void updateReplicaLag(const std::string& replica_id, int64_t lag_ms);

    /**
     * @brief Update lag for multiple replicas at once (atomic batch).
     *
     * More efficient than calling updateReplicaLag() multiple times.
     * All replicas are updated under a single lock acquisition.
     * Thread-safe.
     *
     * @param lags Map of replica_id → lag_ms.
     * 
     * @note Thread safety: Acquires state_mutex_ once for all updates.
     * @note Atomicity: All updates are applied together; no interleaving.
     */
    void updateReplicaLags(const std::map<std::string, int64_t>& lags);

    /**
     * @brief Get the current lag for a specific replica.
     *
     * Thread-safe.
     * 
     * @param replica_id Replica to query.
     * @return Current lag in milliseconds, or 0 if replica not tracked.
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
     */
    int64_t getReplicaLag(const std::string& replica_id) const;

    /**
     * @brief Remove a replica from lag tracking.
     *
     * Called when a replica is removed from the cluster. Thread-safe.
     * Also clears any alert state for the removed replica.
     *
     * @param replica_id Replica to remove.
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
     */
    void removeReplica(const std::string& replica_id);

    /**
     * @brief Clear all tracked replicas.
     *
     * Thread-safe. Removes all replicas and their associated state.
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
     * 
     * @note Thread safety: Acquires state_mutex_; callback invoked OUTSIDE lock.
     * @note Alert firing: Each replica transitions are checked; alert callback
     *       may be invoked multiple times if multiple replicas cross thresholds.
     */
    bool checkAndAlertLagViolations();

    /**
     * @brief Check a specific replica for lag violations.
     *
     * Thread-safe.
     * 
     * @param replica_id Replica to check.
     * @return true if any alert was fired for this replica; false otherwise.
     * 
     * @note Thread safety: Acquires state_mutex_; callback invoked OUTSIDE lock.
     */
    bool checkReplicaLag(const std::string& replica_id);

    /**
     * @brief Return list of replicas currently exceeding alert threshold.
     *
     * Thread-safe snapshot; the list may change after the call returns.
     * Does NOT include replicas in critical state (use replicasInCritical()
     * for those).
     *
     * @return Vector of replica IDs with lag > alert_threshold_ms.
     * 
     * @note Thread safety: Acquires state_mutex_; returns copy.
     */
    std::vector<std::string> replicasInAlert() const;

    /**
     * @brief Return list of replicas currently at critical lag level.
     *
     * Thread-safe snapshot.
     *
     * @return Vector of replica IDs with lag > critical_threshold_ms.
     * 
     * @note Thread safety: Acquires state_mutex_; returns copy.
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
     * 
     * @note Thread safety: Acquires state_mutex_; returns copy.
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
     * 
     * @note Thread safety: Acquires state_mutex_; returns copy.
     */
    std::map<std::string, int64_t> allReplicaLags() const;

    /**
     * @brief Export Prometheus-format metrics text.
     *
     * Exposes per-replica metrics:
     * - `replication_lag_ms{replica_id}` — Current lag (gauge)
     * - `lag_alert_triggered_total{replica_id}` — Total alert transitions
     * - `lag_critical_triggered_total{replica_id}` — Total critical transitions
     * - `failover_initiated_total{replica_id}` — Total failover initiations
     *
     * Thread-safe. Returns Prometheus 0.0.4 format text.
     * 
     * @note Thread safety: Acquires state_mutex_; returns copy.
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
     * 
     * @note Thread safety: Acquires state_mutex_; safe to call concurrently.
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

    // LOCK HIERARCHY: single lock guards all state
    mutable std::mutex              state_mutex_;
    std::map<std::string, ReplicaState> replicas_;
    SLOThresholds                   thresholds_;
    AlertCallback                   alert_callback_;

    // -----------------------------------------------------------------------
    // Helper methods
    // -----------------------------------------------------------------------

    /**
     * @brief Check if a replica is transitioning into or currently in alert state.
     * 
     * Called under state_mutex_ lock. May invoke alert callback (outside lock).
     * Updates replica state (in_alert, in_critical, alert_count, critical_count).
     * 
     * @param state Replica state to evaluate and update.
     * @param replica_id Replica identifier (for alert events).
     * 
     * @note Lock requirement: Caller must NOT hold state_mutex_ (method acquires).
     */
    void evaluateReplicaAlerts(ReplicaState& state, const std::string& replica_id);

    /**
     * @brief Emit an alert event if a callback is registered.
     * 
     * Invokes callback OUTSIDE the state_mutex_ to prevent deadlock if
     * callback tries to update replica lag. Callback exceptions are caught.
     * 
     * @param event AlertEvent to emit.
     * 
     * @note Lock requirement: Caller must hold state_mutex_ upon entry.
     *       Method releases lock before invoking callback, then re-acquires.
     * @note Exception safety: Callback exceptions are caught and suppressed.
     */
    void emitAlert(const AlertEvent& event);

    /**
     * @brief Get current time in milliseconds since Unix epoch.
     * 
     * @return Unix epoch milliseconds (system_clock).
     * 
     * @note Thread safety: No lock required (clock operation).
     */
    static int64_t currentTimeMs();
};

} // namespace replication
} // namespace themisdb
