/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cache_hit_rate_slo_monitor.cpp                     ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-03-02 03:56:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     271                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5fdae26bd  2026-02-24  feat(cache): implement cache hit rate SLO alerting ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cache/cache_hit_rate_slo_monitor.h"
#include "utils/logger.h"
#include <sstream>

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CacheHitRateSloMonitor::CacheHitRateSloMonitor(
    const Config& config,
    std::shared_ptr<observability::Alertmanager> alertmanager)
    : config_(config)
    , alertmanager_(std::move(alertmanager))
    , last_warning_alert_time_(std::chrono::steady_clock::time_point::min())
    , last_critical_alert_time_(std::chrono::steady_clock::time_point::min()) {}

// ---------------------------------------------------------------------------
// Core API
// ---------------------------------------------------------------------------

CacheHitRateSloMonitor::EvaluationResult
CacheHitRateSloMonitor::evaluate(const CacheMetrics& metrics) {
    std::lock_guard<std::mutex> lock(mutex_);

    EvaluationResult result;

    uint64_t hits  = metrics.l1_hits.load() + metrics.l2_hits.load() + metrics.l3_hits.load();
    uint64_t total = hits + metrics.misses.load();

    result.total_requests = total;
    result.hit_rate       = (total > 0) ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;

    // Not enough traffic to make a meaningful evaluation
    if (total < config_.min_requests) {
        result.level = ViolationLevel::NONE;
        last_result_ = result;
        return result;
    }

    // Determine desired violation level based on thresholds
    ViolationLevel desired = ViolationLevel::NONE;
    if (result.hit_rate < config_.critical_threshold) {
        desired = ViolationLevel::CRITICAL;
    } else if (result.hit_rate < config_.warning_threshold) {
        desired = ViolationLevel::WARNING;
    }

    result.level = desired;

    if (desired != ViolationLevel::NONE) {
        // Fire or upgrade alert when violation detected (subject to cooldown)
        if (desired != active_violation_ || isCooldownExpired(desired)) {
            // Resolve previous alert if switching levels
            if (active_violation_ != ViolationLevel::NONE && active_violation_ != desired) {
                resolveActiveAlerts(result);
            }
            fireAlert(desired, result.hit_rate, total, result);
        }
    } else {
        // Hit rate is healthy – resolve any open alerts
        if (active_violation_ != ViolationLevel::NONE) {
            resolveActiveAlerts(result);
        }
    }

    last_result_ = result;
    return result;
}

// ---------------------------------------------------------------------------
// Status & Inspection
// ---------------------------------------------------------------------------

CacheHitRateSloMonitor::EvaluationResult
CacheHitRateSloMonitor::getLastResult() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_result_;
}

CacheHitRateSloMonitor::ViolationLevel
CacheHitRateSloMonitor::getCurrentViolationLevel() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_violation_;
}

bool CacheHitRateSloMonitor::isSloViolated() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return active_violation_ != ViolationLevel::NONE;
}

nlohmann::json CacheHitRateSloMonitor::getStatus() const {
    std::lock_guard<std::mutex> lock(mutex_);

    nlohmann::json j;
    j["hit_rate"]        = last_result_.hit_rate;
    j["total_requests"]  = last_result_.total_requests;
    j["violation_level"] = violationLevelToString(active_violation_);
    j["thresholds"]["warning"]  = config_.warning_threshold;
    j["thresholds"]["critical"] = config_.critical_threshold;
    j["thresholds"]["min_requests"] = config_.min_requests;

    nlohmann::json alerts = nlohmann::json::array();
    if (!active_warning_alert_id_.empty()) {
        alerts.push_back({{"id", active_warning_alert_id_}, {"status", "FIRING"}});
    }
    if (!active_critical_alert_id_.empty()) {
        alerts.push_back({{"id", active_critical_alert_id_}, {"status", "FIRING"}});
    }
    j["alerts"] = alerts;

    return j;
}

std::vector<std::string> CacheHitRateSloMonitor::getActiveAlertIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids;
    if (!active_warning_alert_id_.empty())  ids.push_back(active_warning_alert_id_);
    if (!active_critical_alert_id_.empty()) ids.push_back(active_critical_alert_id_);
    return ids;
}

void CacheHitRateSloMonitor::setAlertmanager(
    std::shared_ptr<observability::Alertmanager> alertmanager) {
    std::lock_guard<std::mutex> lock(mutex_);
    alertmanager_ = std::move(alertmanager);
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::string CacheHitRateSloMonitor::violationLevelToString(ViolationLevel level) {
    switch (level) {
        case ViolationLevel::NONE:     return "NONE";
        case ViolationLevel::WARNING:  return "WARNING";
        case ViolationLevel::CRITICAL: return "CRITICAL";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void CacheHitRateSloMonitor::fireAlert(
    ViolationLevel level, double hit_rate, uint64_t total_requests,
    EvaluationResult& result) {

    observability::Alert alert = buildAlert(level, hit_rate, total_requests);

    if (level == ViolationLevel::WARNING) {
        active_warning_alert_id_  = alert.alert_id;
        last_warning_alert_time_  = std::chrono::steady_clock::now();
    } else {
        active_critical_alert_id_  = alert.alert_id;
        last_critical_alert_time_  = std::chrono::steady_clock::now();
    }
    active_violation_ = level;
    result.alert_fired = true;

    THEMIS_WARN("Cache hit rate SLO violation [{}]: hit_rate={:.3f} (threshold={:.3f}) "
                "total_requests={}",
                violationLevelToString(level), hit_rate,
                (level == ViolationLevel::CRITICAL ? config_.critical_threshold
                                                   : config_.warning_threshold),
                total_requests);

    if (alertmanager_) {
        auto send_result = alertmanager_->sendAlert(alert);
        if (!send_result) {
            THEMIS_ERROR("CacheHitRateSloMonitor: failed to send alert '{}': {}",
                         alert.alert_id, send_result.error().message());
        }
    }
}

void CacheHitRateSloMonitor::resolveActiveAlerts(EvaluationResult& result) {
    result.alert_resolved = true;

    if (!active_warning_alert_id_.empty()) {
        THEMIS_INFO("Cache hit rate SLO recovered (WARNING resolved): alert={}",
                    active_warning_alert_id_);
        if (alertmanager_) {
            auto res = alertmanager_->resolveAlert(active_warning_alert_id_);
            if (!res) {
                THEMIS_ERROR("CacheHitRateSloMonitor: failed to resolve alert '{}': {}",
                             active_warning_alert_id_, res.error().message());
            }
        }
        active_warning_alert_id_.clear();
    }

    if (!active_critical_alert_id_.empty()) {
        THEMIS_INFO("Cache hit rate SLO recovered (CRITICAL resolved): alert={}",
                    active_critical_alert_id_);
        if (alertmanager_) {
            auto res = alertmanager_->resolveAlert(active_critical_alert_id_);
            if (!res) {
                THEMIS_ERROR("CacheHitRateSloMonitor: failed to resolve alert '{}': {}",
                             active_critical_alert_id_, res.error().message());
            }
        }
        active_critical_alert_id_.clear();
    }

    active_violation_ = ViolationLevel::NONE;
}

observability::Alert CacheHitRateSloMonitor::buildAlert(
    ViolationLevel level, double hit_rate, uint64_t total_requests) const {

    observability::Alert alert;
    alert.alert_id   = makeAlertId(config_.cache_name, level);
    alert.alert_name = "CacheHitRateSloViolation";
    alert.status     = observability::AlertStatus::FIRING;

    if (level == ViolationLevel::CRITICAL) {
        alert.severity = observability::AlertSeverity::CRITICAL;
    } else {
        alert.severity = observability::AlertSeverity::WARNING;
    }

    std::ostringstream msg;
    msg << "Cache hit rate SLO violation: hit_rate=" << hit_rate
        << " is below " << violationLevelToString(level) << " threshold="
        << (level == ViolationLevel::CRITICAL ? config_.critical_threshold
                                              : config_.warning_threshold)
        << " (total_requests=" << total_requests << ")";
    alert.message = msg.str();

    alert.labels["component"]       = "cache";
    alert.labels["cache_name"]      = config_.cache_name;
    alert.labels["severity"]        = violationLevelToString(level);
    alert.labels["alertname"]       = "CacheHitRateSloViolation";

    alert.annotations["hit_rate"]        = std::to_string(hit_rate);
    alert.annotations["total_requests"]  = std::to_string(total_requests);
    alert.annotations["threshold"]       = std::to_string(
        level == ViolationLevel::CRITICAL ? config_.critical_threshold
                                          : config_.warning_threshold);

    return alert;
}

bool CacheHitRateSloMonitor::isCooldownExpired(ViolationLevel level) const {
    auto& last_time = (level == ViolationLevel::CRITICAL)
                      ? last_critical_alert_time_
                      : last_warning_alert_time_;

    if (last_time == std::chrono::steady_clock::time_point::min()) {
        return true;  // Never fired
    }

    auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - last_time
    ).count();

    return elapsed >= config_.alert_cooldown_seconds;
}

std::string CacheHitRateSloMonitor::makeAlertId(
    const std::string& cache_name, ViolationLevel level) {
    return "cache_hit_rate_" + cache_name + "_" +
           (level == ViolationLevel::CRITICAL ? "critical" : "warning");
}

} // namespace cache
} // namespace themis
