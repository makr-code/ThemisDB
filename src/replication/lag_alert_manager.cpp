// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file lag_alert_manager.cpp
 * @brief Implementation of LagAlertManager.
 *
 * @see include/replication/lag_alert_manager.h
 */

#include "replication/lag_alert_manager.h"

#include <algorithm>
#include <chrono>
#include <sstream>

namespace themisdb {
namespace replication {

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

LagAlertManager::LagAlertManager()
{
    // Default thresholds are set in SLOThresholds struct initialization
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void LagAlertManager::setThresholds(const SLOThresholds& thresholds)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    thresholds_ = thresholds;
}

SLOThresholds LagAlertManager::thresholds() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    return thresholds_;
}

void LagAlertManager::setAlertCallback(AlertCallback callback)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    alert_callback_ = std::move([[maybe_unused]] callback);
}

// ---------------------------------------------------------------------------
// Replica lag updates
// ---------------------------------------------------------------------------

void LagAlertManager::updateReplicaLag(const std::string& replica_id,
                                       int64_t           lag_ms)
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    auto& state = replicas_[replica_id];
    state.lag.replica_id = replica_id;
    state.lag.lag_ms = lag_ms;
    state.lag.last_update_ms = currentTimeMs();

    if (lag_ms > state.lag.max_lag_since_alert_ms) {
        state.lag.max_lag_since_alert_ms = lag_ms;
    }
}

void LagAlertManager::updateReplicaLags(
    const std::map<std::string, int64_t>& lags)
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    const int64_t now = currentTimeMs();
    for (const auto& [replica_id, lag_ms] : lags) {
        auto& state = replicas_[replica_id];
        state.lag.replica_id = replica_id;
        state.lag.lag_ms = lag_ms;
        state.lag.last_update_ms = now;

        if (lag_ms > state.lag.max_lag_since_alert_ms) {
            state.lag.max_lag_since_alert_ms = lag_ms;
        }
    }
}

int64_t LagAlertManager::getReplicaLag(const std::string& replica_id) const
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    const auto it = replicas_.find(replica_id);
    return (it != replicas_.end()) ? it->second.lag.lag_ms : 0;
}

void LagAlertManager::removeReplica(const std::string& replica_id)
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    replicas_.erase(replica_id);
}

void LagAlertManager::clearAllReplicas()
{
    std::lock_guard<std::mutex> lock(state_mutex_);
    replicas_.clear();
}

// ---------------------------------------------------------------------------
// Alert evaluation
// ---------------------------------------------------------------------------

bool LagAlertManager::checkAndAlertLagViolations()
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    bool any_alert = false;
    for (auto& [replica_id, state] : replicas_) {
        evaluateReplicaAlerts(state, replica_id);
        if (state.in_alert || state.in_critical) {
            any_alert = true;
        }
    }
    return any_alert;
}

bool LagAlertManager::checkReplicaLag(const std::string& replica_id)
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    const auto it = replicas_.find(replica_id);
    if (it == replicas_.end()) {
      return false;
    }

    evaluateReplicaAlerts(it->second, replica_id);
    return it->second.in_alert || it->second.in_critical;
}

std::vector<std::string> LagAlertManager::replicasInAlert() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    std::vector<std::string> result = {};

    for (const auto& [replica_id, state] : replicas_) {
        if (state.in_alert && !state.in_critical) {
            result.push_back(replica_id);
        }
    }
    return result;
}

std::vector<std::string> LagAlertManager::replicasInCritical() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    std::vector<std::string> result = {};

    for (const auto& [replica_id, state] : replicas_) {
        if (state.in_critical) {
            result.push_back(replica_id);
        }
    }
    return result;
}

std::vector<std::string> LagAlertManager::replicasEligibleForFailover() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    std::vector<std::string> result;
    const auto now = std::chrono::steady_clock::now();

    for (const auto& [replica_id, state] : replicas_) {
        // Only replicas in sustained critical lag are eligible
        if (!state.in_critical) {
          continue;
        }

        // Check if critical duration threshold is exceeded
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - state.critical_start_time).count();

        if (duration >= thresholds_.failover_duration_ms) {
            result.push_back(replica_id);
        }
    }
    return result;
}

// ---------------------------------------------------------------------------
// Metrics
// ---------------------------------------------------------------------------

std::map<std::string, int64_t> LagAlertManager::allReplicaLags() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    std::map<std::string, int64_t> result = {};

    for (const auto& [replica_id, state] : replicas_) {
        result[replica_id] = state.lag.lag_ms;
    }
    return result;
}

std::string LagAlertManager::exportPrometheusMetrics() const
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    std::ostringstream oss = {};

    // Lag per replica
    oss << "# HELP replication_lag_ms Current replication lag per replica (ms)\n"
        << "# TYPE replication_lag_ms gauge\n";
    for (const auto& [replica_id, state] : replicas_) {
        oss << "replication_lag_ms{replica_id=\"" << replica_id << "\"} "
            << state.lag.lag_ms << "\n";
    }

    // Alert triggers
    oss << "# HELP lag_alert_triggered_total Total alert threshold crossings\n"
        << "# TYPE lag_alert_triggered_total counter\n";
    for (const auto& [replica_id, state] : replicas_) {
        if (state.alert_count > 0) {
            oss << "lag_alert_triggered_total{replica_id=\"" << replica_id << "\"} "
                << state.alert_count << "\n";
        }
    }

    // Critical triggers
    oss << "# HELP lag_critical_triggered_total Total critical threshold crossings\n"
        << "# TYPE lag_critical_triggered_total counter\n";
    for (const auto& [replica_id, state] : replicas_) {
        if (state.critical_count > 0) {
            oss << "lag_critical_triggered_total{replica_id=\"" << replica_id << "\"} "
                << state.critical_count << "\n";
        }
    }

    // Failover initiated
    oss << "# HELP failover_initiated_total Total failover initiations due to lag\n"
        << "# TYPE failover_initiated_total counter\n";
    for (const auto& [replica_id, state] : replicas_) {
        if (state.failover_count > 0) {
            oss << "failover_initiated_total{replica_id=\"" << replica_id << "\"} "
                << state.failover_count << "\n";
        }
    }

    return oss.str();
}

// ---------------------------------------------------------------------------
// Statistics
// ---------------------------------------------------------------------------

std::tuple<uint64_t, uint64_t, uint64_t> LagAlertManager::getAlertStats(
    const std::string& replica_id) const
{
    std::lock_guard<std::mutex> lock(state_mutex_);

    const auto it = replicas_.find(replica_id);
    if (it == replicas_.end()) {
        return {0, 0, 0};
    }

    return {it->second.alert_count, it->second.critical_count,
            it->second.failover_count};
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------

void LagAlertManager::evaluateReplicaAlerts(ReplicaState&      state,
                                            const std::string& replica_id)
{
    // Note: Caller must hold state_mutex_

    const int64_t lag = state.lag.lag_ms;
    const auto now = std::chrono::steady_clock::now();

    bool was_in_alert = state.in_alert;
    bool was_in_critical = state.in_critical;

    // Determine current alert state
    bool should_be_alert =
        (thresholds_.alert_threshold_ms > 0) &&
        (lag >= thresholds_.alert_threshold_ms);

    bool should_be_critical =
        (thresholds_.critical_threshold_ms > 0) &&
        (lag >= thresholds_.critical_threshold_ms);

    // Handle critical state transitions
    if (should_be_critical && !was_in_critical) {
        // Entering critical state
        state.in_critical = true;
        state.critical_start_time = now;
        state.in_alert = true;  // Critical implies alert
        ++state.critical_count;

        AlertEvent evt;
        evt.level = AlertEvent::Level::CRITICAL;
        evt.replica_id = replica_id;
        evt.lag_ms = lag;
        evt.message = "Replica lag critical: " + std::to_string(lag) + "ms";
        evt.timestamp_ms = currentTimeMs();
        emitAlert(evt);

    } else if (!should_be_critical && was_in_critical) {
        // Leaving critical state
        state.in_critical = false;
        if (!should_be_alert) {
            state.in_alert = false;
        }

    } else if (should_be_critical && was_in_critical) {
        // Still in critical state; check if eligible for failover
        const auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - state.critical_start_time).count();

        if (duration >= thresholds_.failover_duration_ms) {
            // Emit failover event (once per sustained critical period)
            // In a real implementation, this would check if we've already emitted
            // a failover event for this critical period
            AlertEvent evt;
            evt.level = AlertEvent::Level::FAILOVER;
            evt.replica_id = replica_id;
            evt.lag_ms = lag;
            evt.message = "Failover initiated: sustained critical lag for " +
                          std::to_string(duration) + "ms";
            evt.timestamp_ms = currentTimeMs();
            emitAlert(evt);
            ++state.failover_count;
        }
    }

    // Handle alert state transitions (only if not critical)
    if (!should_be_critical) {
        if (should_be_alert && !was_in_alert) {
            // Entering alert state
            state.in_alert = true;
            ++state.alert_count;

            AlertEvent evt;
            evt.level = AlertEvent::Level::ALERT;
            evt.replica_id = replica_id;
            evt.lag_ms = lag;
            evt.message = "Replica lag alert: " + std::to_string(lag) + "ms";
            evt.timestamp_ms = currentTimeMs();
            emitAlert(evt);

        } else if (!should_be_alert && was_in_alert) {
            // Leaving alert state
            state.in_alert = false;
        }
    }
}

void LagAlertManager::emitAlert(const AlertEvent& event)
{
    // Note: Caller must hold state_mutex_.
    // Adopt the caller-held lock so we can safely unlock/lock around callback.
    std::unique_lock<std::mutex> lock(state_mutex_, std::adopt_lock);

    AlertCallback cb;
    cb = alert_callback_;

    // Release lock before invoking callback to avoid potential deadlocks
    lock.unlock();

    try {
        if (cb) {
            cb([[maybe_unused]] event);
        }
    } catch (...) {
        // Suppress exceptions from callback to ensure alert infrastructure stays robust
    }

    // Re-acquire lock for caller
    lock.lock();
    lock.release();  // caller's lock_guard still owns final unlock
}

int64_t LagAlertManager::currentTimeMs()
{
    using Clock = std::chrono::system_clock;
    const auto now = Clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
}

} // namespace replication
} // namespace themisdb
