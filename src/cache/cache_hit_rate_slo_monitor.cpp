/**
 * @file cache_hit_rate_slo_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=1, H=0, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cache/cache_hit_rate_slo_monitor.h"

#include <sstream>

#include "utils/logger.h"

namespace themis {
namespace cache {

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

CacheHitRateSloMonitor::CacheHitRateSloMonitor(std::shared_ptr<observability::Alertmanager> alertmanager)
    : CacheHitRateSloMonitor(Config{}, std::move(alertmanager)) {}

CacheHitRateSloMonitor::CacheHitRateSloMonitor(const Config &config,
                                               std::shared_ptr<observability::Alertmanager> alertmanager)
    : config_(config), alertmanager_(std::move(alertmanager)),
      last_warning_alert_time_(std::chrono::steady_clock::time_point::min()),
      last_critical_alert_time_(std::chrono::steady_clock::time_point::min()),
      last_latency_warning_alert_time_(std::chrono::steady_clock::time_point::min()),
      last_latency_critical_alert_time_(std::chrono::steady_clock::time_point::min()) {}

// ---------------------------------------------------------------------------
// Core API
// ---------------------------------------------------------------------------

void CacheHitRateSloMonitor::recordLatency(Tier tier, double latency_ms) {
    const auto idx = static_cast<std::size_t>(tier);
    if (idx >= static_cast<std::size_t>(Tier::COUNT)) {
        return;
    }
    if (latency_ms < 0.0)
        latency_ms = 0.0;
    latency_hist_[idx].record(latency_ms);
}

CacheHitRateSloMonitor::EvaluationResult CacheHitRateSloMonitor::evaluate(const CacheMetrics &metrics) {
    std::lock_guard<std::mutex> lock(mutex_);

    EvaluationResult result;

    uint64_t hits  = metrics.l1_hits.load() + metrics.l2_hits.load() + metrics.l3_hits.load();
    uint64_t total = hits + metrics.misses.load();

    result.total_requests = total;
    result.hit_rate       = (total > 0) ? static_cast<double>(hits) / static_cast<double>(total) : 0.0;

    // --- Compute aggregate latency percentiles across all tiers ---
    // We read each tier histogram atomically and compute a combined estimate.
    // To avoid holding the mutex during bucket reads (atomics are lock-free),
    // we read directly here while mutex is held for alert state consistency.
    {
        // Build a combined bucket view
        constexpr std::size_t kB = LatencyHistogram::kNumBuckets;
        uint64_t combined[kB]    = {};
        uint64_t combined_total  = 0;
        for (std::size_t t = 0; t < static_cast<std::size_t>(Tier::COUNT); ++t) {
            for (std::size_t b = 0; b < kB; ++b) {
                combined[b] += latency_hist_[t].buckets[b].load(std::memory_order_relaxed);
            }
            combined_total += latency_hist_[t].count.load(std::memory_order_relaxed);
        }

        auto percentileFromCombined = [&](double p) -> double {
            if (combined_total == 0) {
                return 0.0;
            }
            uint64_t target = static_cast<uint64_t>(static_cast<double>(combined_total) * p);
            if (target == 0) {
                target = 1;
            }
            uint64_t cumulative = 0;
            for (std::size_t i = 0; i < kB; ++i) {
                cumulative += combined[i];
                if (cumulative >= target) {
                    return LatencyHistogram::kMidpointsMs[i];
                }
            }
            return LatencyHistogram::kMidpointsMs[static_cast<int>(kB - 1)];
        };

        result.p50_latency_ms = percentileFromCombined(0.50);
        result.p95_latency_ms = percentileFromCombined(0.95);
        result.p99_latency_ms = percentileFromCombined(0.99);
    }

    // --- Latency SLO evaluation ---
    if (result.p99_latency_ms > 0.0) {
        ViolationLevel desired_latency = ViolationLevel::NONE;
        if (config_.p99_critical_ms > 0.0 && result.p99_latency_ms >= config_.p99_critical_ms) {
            desired_latency = ViolationLevel::CRITICAL;
        } else if (config_.p99_warn_ms > 0.0 && result.p99_latency_ms >= config_.p99_warn_ms) {
            desired_latency = ViolationLevel::WARNING;
        }

        result.latency_level = desired_latency;

        if (desired_latency != ViolationLevel::NONE) {
            if (desired_latency != active_latency_violation_ || isLatencyCooldownExpired(desired_latency)) {
                if (active_latency_violation_ != ViolationLevel::NONE && active_latency_violation_ != desired_latency) {
                    resolveLatencyAlerts(result);
                }
                fireLatencyAlert(desired_latency, result.p99_latency_ms, result);
            }
        } else {
            if (active_latency_violation_ != ViolationLevel::NONE) {
                resolveLatencyAlerts(result);
            }
        }
    }
    result.latency_level = active_latency_violation_;

    // Not enough traffic to make a meaningful hit-rate evaluation
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

    // C3: Structured hit-rate snapshot telemetry — emitted on every evaluation.
    THEMIS_DEBUG("{{\"event\":\"hit_rate_snapshot\",\"interval_ms\":0,"
                 "\"hit_rate\":{:.4f},\"miss_rate\":{:.4f},\"total_requests\":{}}}",
                 result.hit_rate, (total > 0 ? 1.0 - result.hit_rate : 0.0), total);

    return result;
}

// ---------------------------------------------------------------------------
// Status & Inspection
// ---------------------------------------------------------------------------

CacheHitRateSloMonitor::EvaluationResult CacheHitRateSloMonitor::getLastResult() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return last_result_;
}

CacheHitRateSloMonitor::ViolationLevel CacheHitRateSloMonitor::getCurrentViolationLevel() const {
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
    j["hit_rate"]                   = last_result_.hit_rate;
    j["total_requests"]             = last_result_.total_requests;
    j["violation_level"]            = violationLevelToString(active_violation_);
    j["thresholds"]["warning"]      = config_.warning_threshold;
    j["thresholds"]["critical"]     = config_.critical_threshold;
    j["thresholds"]["min_requests"] = config_.min_requests;

    nlohmann::json alerts = nlohmann::json::array();
    if (!active_warning_alert_id_.empty()) {
        alerts.push_back({{"id", active_warning_alert_id_}, {"status", "FIRING"}});
    }
    if (!active_critical_alert_id_.empty()) {
        alerts.push_back({{"id", active_critical_alert_id_}, {"status", "FIRING"}});
    }
    if (!active_latency_warning_alert_id_.empty()) {
        alerts.push_back({{"id", active_latency_warning_alert_id_}, {"status", "FIRING"}});
    }
    if (!active_latency_critical_alert_id_.empty()) {
        alerts.push_back({{"id", active_latency_critical_alert_id_}, {"status", "FIRING"}});
    }
    j["alerts"] = alerts;

    // Per-tier and aggregate latency percentiles
    auto tierJson = [&](Tier tier) {
        const auto &h = latency_hist_[static_cast<std::size_t>(tier)];
        return nlohmann::json{
            {"p50_ms", h.percentileMs(0.50)},
            {"p95_ms", h.percentileMs(0.95)},
            {"p99_ms", h.percentileMs(0.99)},
        };
    };

    j["latency"]["p50_ms"]          = last_result_.p50_latency_ms;
    j["latency"]["p95_ms"]          = last_result_.p95_latency_ms;
    j["latency"]["p99_ms"]          = last_result_.p99_latency_ms;
    j["latency"]["violation_level"] = violationLevelToString(active_latency_violation_);
    j["latency"]["l1"]              = tierJson(Tier::L1);
    j["latency"]["l2"]              = tierJson(Tier::L2);
    j["latency"]["l3"]              = tierJson(Tier::L3);
    if (config_.p99_warn_ms > 0.0) {
        j["latency"]["thresholds"]["p99_warn_ms"] = config_.p99_warn_ms;
    }
    if (config_.p99_critical_ms > 0.0) {
        j["latency"]["thresholds"]["p99_critical_ms"] = config_.p99_critical_ms;
    }

    return j;
}

std::vector<std::string> CacheHitRateSloMonitor::getActiveAlertIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> ids = {};

    if (!active_warning_alert_id_.empty()) {
        ids.push_back(active_warning_alert_id_);
    }
    if (!active_critical_alert_id_.empty()) {
        ids.push_back(active_critical_alert_id_);
    }
    return ids;
}

void CacheHitRateSloMonitor::setAlertmanager(std::shared_ptr<observability::Alertmanager> alertmanager) {
    std::lock_guard<std::mutex> lock(mutex_);
    alertmanager_ = std::move(alertmanager);
}

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::string CacheHitRateSloMonitor::violationLevelToString(ViolationLevel level) {
    switch (level) {
        case ViolationLevel::NONE:
            return "NONE";
        case ViolationLevel::WARNING:
            return "WARNING";
        case ViolationLevel::CRITICAL:
            return "CRITICAL";
    }
    return "UNKNOWN";
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void CacheHitRateSloMonitor::fireAlert(ViolationLevel level, double hit_rate, uint64_t total_requests,
                                       EvaluationResult &result) {
    observability::Alert alert = buildAlert(level, hit_rate, total_requests);

    if (level == ViolationLevel::WARNING) {
        active_warning_alert_id_ = alert.alert_id;
        last_warning_alert_time_ = std::chrono::steady_clock::now();
    } else {
        active_critical_alert_id_ = alert.alert_id;
        last_critical_alert_time_ = std::chrono::steady_clock::now();
    }
    active_violation_  = level;
    result.alert_fired = true;

    THEMIS_WARN("Cache hit rate SLO violation [{}]: hit_rate={:.3f} (threshold={:.3f}) "
                "total_requests={}",
                violationLevelToString(level), hit_rate,
                (level == ViolationLevel::CRITICAL ? config_.critical_threshold : config_.warning_threshold),
                total_requests);

    // C3: Structured SLO breach telemetry event.
    THEMIS_WARN("{{\"event\":\"slo_breach\",\"slo_threshold\":{:.4f},"
                "\"actual_hit_rate\":{:.4f},\"level\":\"{}\","
                "\"cache_name\":\"{}\"}}",
                (level == ViolationLevel::CRITICAL ? config_.critical_threshold : config_.warning_threshold),
                hit_rate, violationLevelToString(level), config_.cache_name);

    if (alertmanager_) {
        auto send_result = alertmanager_->sendAlert(alert);
        if (!send_result) {
            THEMIS_ERROR("CacheHitRateSloMonitor: failed to send alert '{}': {}", alert.alert_id,
                         send_result.error().message());
        }
    }
}

void CacheHitRateSloMonitor::resolveActiveAlerts(EvaluationResult &result) {
    result.alert_resolved = true;

    if (!active_warning_alert_id_.empty()) {
        THEMIS_INFO("Cache hit rate SLO recovered (WARNING resolved): alert={}", active_warning_alert_id_);
        if (alertmanager_) {
            auto res = alertmanager_->resolveAlert(active_warning_alert_id_);
            if (!res) {
                THEMIS_ERROR("CacheHitRateSloMonitor: failed to resolve alert '{}': {}", active_warning_alert_id_,
                             res.error().message());
            }
        }
        active_warning_alert_id_.clear();
    }

    if (!active_critical_alert_id_.empty()) {
        THEMIS_INFO("Cache hit rate SLO recovered (CRITICAL resolved): alert={}", active_critical_alert_id_);
        if (alertmanager_) {
            auto res = alertmanager_->resolveAlert(active_critical_alert_id_);
            if (!res) {
                THEMIS_ERROR("CacheHitRateSloMonitor: failed to resolve alert '{}': {}", active_critical_alert_id_,
                             res.error().message());
            }
        }
        active_critical_alert_id_.clear();
    }

    active_violation_ = ViolationLevel::NONE;
}

observability::Alert CacheHitRateSloMonitor::buildAlert(ViolationLevel level, double hit_rate,
                                                        uint64_t total_requests) const {
    observability::Alert alert;
    alert.alert_id   = makeAlertId(config_.cache_name, level);
    alert.alert_name = "CacheHitRateSloViolation";
    alert.status     = observability::AlertStatus::FIRING;

    if (level == ViolationLevel::CRITICAL) {
        alert.severity = observability::AlertSeverity::CRITICAL;
    } else {
        alert.severity = observability::AlertSeverity::WARNING;
    }

    std::ostringstream msg = {};
    msg << "Cache hit rate SLO violation: hit_rate=" << hit_rate << " is below " << violationLevelToString(level)
        << " threshold=" << (level == ViolationLevel::CRITICAL ? config_.critical_threshold : config_.warning_threshold)
        << " (total_requests=" << total_requests << ")";
    alert.message = msg.str();

    alert.labels["component"]  = "cache";
    alert.labels["cache_name"] = config_.cache_name;
    alert.labels["severity"]   = violationLevelToString(level);
    alert.labels["alertname"]  = "CacheHitRateSloViolation";

    alert.annotations["hit_rate"]       = std::to_string(hit_rate);
    alert.annotations["total_requests"] = std::to_string(total_requests);
    alert.annotations["threshold"]
        = std::to_string(level == ViolationLevel::CRITICAL ? config_.critical_threshold : config_.warning_threshold);

    return alert;
}

bool CacheHitRateSloMonitor::isCooldownExpired(ViolationLevel level) const {
    auto &last_time = (level == ViolationLevel::CRITICAL) ? last_critical_alert_time_ : last_warning_alert_time_;

    if (last_time == std::chrono::steady_clock::time_point::min()) {
        return true; // Never fired
    }

    auto elapsed
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_time).count();

    return elapsed >= config_.alert_cooldown_seconds;
}

std::string CacheHitRateSloMonitor::makeAlertId(const std::string &cache_name, ViolationLevel level) {
    return "cache_hit_rate_" + cache_name + "_" + (level == ViolationLevel::CRITICAL ? "critical" : "warning");
}

// ---------------------------------------------------------------------------
// Latency alert helpers
// ---------------------------------------------------------------------------

void CacheHitRateSloMonitor::fireLatencyAlert(ViolationLevel level, double p99_ms, EvaluationResult &result) {
    observability::Alert alert = buildLatencyAlert(level, p99_ms);

    if (level == ViolationLevel::WARNING) {
        active_latency_warning_alert_id_ = alert.alert_id;
        last_latency_warning_alert_time_ = std::chrono::steady_clock::now();
    } else {
        active_latency_critical_alert_id_ = alert.alert_id;
        last_latency_critical_alert_time_ = std::chrono::steady_clock::now();
    }
    active_latency_violation_  = level;
    result.latency_alert_fired = true;

    THEMIS_WARN("Cache latency SLO violation [{}]: p99={:.3f}ms (threshold={:.3f}ms)", violationLevelToString(level),
                p99_ms, (level == ViolationLevel::CRITICAL ? config_.p99_critical_ms : config_.p99_warn_ms));

    // C3: Structured latency SLO breach telemetry event.
    THEMIS_WARN("{{\"event\":\"latency_slo_breach\",\"p99_ms\":{:.3f},"
                "\"threshold_ms\":{:.3f},\"level\":\"{}\","
                "\"cache_name\":\"{}\"}}",
                p99_ms,
                (level == ViolationLevel::CRITICAL ? config_.p99_critical_ms : config_.p99_warn_ms),
                violationLevelToString(level), config_.cache_name);

    if (alertmanager_) {
        auto send_result = alertmanager_->sendAlert(alert);
        if (!send_result) {
            THEMIS_ERROR("CacheHitRateSloMonitor: failed to send latency alert '{}': {}", alert.alert_id,
                         send_result.error().message());
        }
    }
}

void CacheHitRateSloMonitor::resolveLatencyAlerts(EvaluationResult &result) {
    result.latency_alert_resolved = true;

    if (!active_latency_warning_alert_id_.empty()) {
        THEMIS_INFO("Cache latency SLO recovered (WARNING resolved): alert={}", active_latency_warning_alert_id_);
        if (alertmanager_) {
            auto res = alertmanager_->resolveAlert(active_latency_warning_alert_id_);
            if (!res) {
                THEMIS_ERROR("CacheHitRateSloMonitor: failed to resolve latency alert '{}': {}",
                             active_latency_warning_alert_id_, res.error().message());
            }
        }
        active_latency_warning_alert_id_.clear();
    }

    if (!active_latency_critical_alert_id_.empty()) {
        THEMIS_INFO("Cache latency SLO recovered (CRITICAL resolved): alert={}", active_latency_critical_alert_id_);
        if (alertmanager_) {
            auto res = alertmanager_->resolveAlert(active_latency_critical_alert_id_);
            if (!res) {
                THEMIS_ERROR("CacheHitRateSloMonitor: failed to resolve latency alert '{}': {}",
                             active_latency_critical_alert_id_, res.error().message());
            }
        }
        active_latency_critical_alert_id_.clear();
    }

    active_latency_violation_ = ViolationLevel::NONE;
}

observability::Alert CacheHitRateSloMonitor::buildLatencyAlert(ViolationLevel level, double p99_ms) const {
    observability::Alert alert;
    alert.alert_id   = makeLatencyAlertId(config_.cache_name, level);
    alert.alert_name = "CacheLatencySloViolation";
    alert.status     = observability::AlertStatus::FIRING;
    alert.severity   = (level == ViolationLevel::CRITICAL) ? observability::AlertSeverity::CRITICAL
                                                           : observability::AlertSeverity::WARNING;

    std::ostringstream msg = {};
    msg << "Cache latency SLO violation: p99=" << p99_ms << "ms is above " << violationLevelToString(level)
        << " threshold=" << (level == ViolationLevel::CRITICAL ? config_.p99_critical_ms : config_.p99_warn_ms) << "ms";
    alert.message = msg.str();

    alert.labels["component"]  = "cache";
    alert.labels["cache_name"] = config_.cache_name;
    alert.labels["severity"]   = violationLevelToString(level);
    alert.labels["alertname"]  = "CacheLatencySloViolation";

    alert.annotations["p99_ms"] = std::to_string(p99_ms);
    alert.annotations["threshold"]
        = std::to_string(level == ViolationLevel::CRITICAL ? config_.p99_critical_ms : config_.p99_warn_ms);

    return alert;
}

bool CacheHitRateSloMonitor::isLatencyCooldownExpired(ViolationLevel level) const {
    auto &last_time
        = (level == ViolationLevel::CRITICAL) ? last_latency_critical_alert_time_ : last_latency_warning_alert_time_;

    if (last_time == std::chrono::steady_clock::time_point::min()) {
        return true;
    }

    auto elapsed
        = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - last_time).count();

    return elapsed >= config_.alert_cooldown_seconds;
}

std::string CacheHitRateSloMonitor::makeLatencyAlertId(const std::string &cache_name, ViolationLevel level) {
    return "cache_latency_" + cache_name + "_" + (level == ViolationLevel::CRITICAL ? "critical" : "warning");
}

} // namespace cache
} // namespace themis
