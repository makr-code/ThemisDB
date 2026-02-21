/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            alertmanager.cpp                                   ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:40                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     431                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "observability/alertmanager.h"
#include "utils/logger.h"
#include "utils/error_registry.h"
#include "utils/http_client_pool.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

using json = nlohmann::json;

namespace themis {
namespace observability {

// ============================================================================
// Helper – format a time_point as ISO-8601 UTC string
// ============================================================================
namespace {

std::string toISO8601(std::chrono::system_clock::time_point tp) {
    std::time_t t = std::chrono::system_clock::to_time_t(tp);
    std::tm tm{};
#if defined(_WIN32)
    gmtime_s(&tm, &t);
#else
    gmtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%dT%H:%M:%SZ");
    return oss.str();
}

// Zero time used to indicate an on-going (firing) alert to the Alertmanager
std::string zeroISO8601() {
    return "0001-01-01T00:00:00Z";
}

} // anonymous namespace

// ============================================================================
// Alertmanager Base Class Implementation
// ============================================================================

Result<void> Alertmanager::initialize(const AlertmanagerConfig& config) {
    config_ = config;
    
    THEMIS_INFO("Alertmanager initialized: endpoint={}, enabled={}",
                config_.endpoint_url, config_.enabled);
    
    if (!config_.enabled) {
        THEMIS_WARN("Alertmanager is disabled in configuration");
    }
    
    return {};
}

Result<void> Alertmanager::sendAlert(const Alert& alert) {
    // Base-class no-op: subclasses provide the concrete transport.
    (void)alert;
    return {};
}

Result<void> Alertmanager::resolveAlert(const std::string& alert_id) {
    // Base-class no-op: subclasses provide the concrete transport.
    (void)alert_id;
    return {};
}

Result<void> Alertmanager::silenceAlert(const std::string& alert_id, int duration_minutes) {
    // Base-class no-op: subclasses provide the concrete transport.
    (void)alert_id;
    (void)duration_minutes;
    return {};
}

std::vector<Alert> Alertmanager::getActiveAlerts() {
    return active_alerts_;
}

Result<void> Alertmanager::testConnection() {
    if (!config_.enabled) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            "Alertmanager is not enabled"
        });
    }
    // Base-class no-op: subclasses provide the concrete health-check.
    return {};
}

std::string Alertmanager::severityToString(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::INFO:     return "INFO";
        case AlertSeverity::WARNING:  return "WARNING";
        case AlertSeverity::ERROR:    return "ERROR";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        default:                       return "UNKNOWN";
    }
}

std::string Alertmanager::statusToString(AlertStatus status) {
    switch (status) {
        case AlertStatus::FIRING:   return "FIRING";
        case AlertStatus::RESOLVED: return "RESOLVED";
        case AlertStatus::SILENCED: return "SILENCED";
        default:                     return "UNKNOWN";
    }
}

// ============================================================================
// DefaultAlertmanager Implementation – Prometheus Alertmanager v2 HTTP API
// ============================================================================

DefaultAlertmanager::DefaultAlertmanager(const AlertmanagerConfig& config) {
    config_ = config;
}

void DefaultAlertmanager::ensureHttpPool() {
    if (http_pool_) return;
    utils::HTTPClientPool::Config pool_cfg;
    pool_cfg.max_connections = 4;
    pool_cfg.connect_timeout = std::chrono::seconds(config_.timeout_seconds);
    pool_cfg.request_timeout = std::chrono::seconds(config_.timeout_seconds);
    pool_cfg.io_threads = 1;
    pool_cfg.lock_stripes = 2;
    http_pool_ = std::make_shared<utils::HTTPClientPool>(pool_cfg);
}

Result<int> DefaultAlertmanager::postWithRetry(const std::string& path,
                                               const std::string& json_body) {
    ensureHttpPool();

    const std::string url = config_.endpoint_url + path;
    std::unordered_map<std::string, std::string> headers{
        {"Content-Type", "application/json"}
    };
    if (!config_.auth_token.empty()) {
        headers["Authorization"] = "Bearer " + config_.auth_token;
    }

    // Parse the JSON body once before the retry loop to avoid redundant work.
    const json parsed_body = json::parse(json_body);

    int attempts = std::max(1, config_.retry_count + 1);
    for (int attempt = 1; attempt <= attempts; ++attempt) {
        try {
            auto future = http_pool_->post(url, parsed_body, headers);
            auto resp = future.get();
            if (resp.isSuccess()) {
                return resp.status_code;
            }
            THEMIS_WARN("Alertmanager POST {} returned HTTP {} (attempt {}/{})",
                        path, resp.status_code, attempt, attempts);
        } catch (const std::exception& ex) {
            THEMIS_WARN("Alertmanager POST {} exception: {} (attempt {}/{})",
                        path, ex.what(), attempt, attempts);
        }
        if (attempt < attempts) {
            std::this_thread::sleep_for(
                std::chrono::milliseconds(config_.retry_delay_ms));
        }
    }

    return tl::unexpected(Error{
        errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
        "Alertmanager POST " + path + " failed after " + std::to_string(attempts) + " attempt(s)"
    });
}

Result<void> DefaultAlertmanager::initialize(const AlertmanagerConfig& config) {
    config_ = config;
    http_pool_.reset(); // reset so ensureHttpPool re-creates on next call
    
    THEMIS_INFO("DefaultAlertmanager initialized: endpoint={}, enabled={}",
                config_.endpoint_url, config_.enabled);
    THEMIS_INFO("  Timeout: {}s  Retries: {}  Retry-delay: {}ms",
                config_.timeout_seconds, config_.retry_count, config_.retry_delay_ms);
    THEMIS_INFO("  Receivers: {}", config_.receivers.size());
    
    if (!config_.enabled) {
        THEMIS_WARN("Alertmanager is disabled – alerts will only be logged");
        return {};
    }

    // Eagerly test connectivity so callers learn of misconfiguration early.
    auto conn = testConnection();
    if (!conn) {
        THEMIS_WARN("Alertmanager connectivity check failed: {}",
                    conn.error().message);
    }

    return {};
}

Result<void> DefaultAlertmanager::sendAlert(const Alert& alert) {
    // Always log the alert regardless of enabled state.
    std::ostringstream ss;
    ss << "ALERT [" << severityToString(alert.severity) << "] "
       << alert.alert_name << ": " << alert.message;
    
    switch (alert.severity) {
        case AlertSeverity::INFO:
            THEMIS_INFO("{}", ss.str());
            break;
        case AlertSeverity::WARNING:
            THEMIS_WARN("{}", ss.str());
            break;
        case AlertSeverity::ERROR:
        case AlertSeverity::CRITICAL:
            THEMIS_ERROR("{}", ss.str());
            break;
    }
    for (const auto& [key, value] : alert.labels) {
        THEMIS_DEBUG("  Label: {}={}", key, value);
    }
    for (const auto& [key, value] : alert.annotations) {
        THEMIS_DEBUG("  Annotation: {}={}", key, value);
    }
    
    // Maintain local active-alerts list.
    if (alert.status == AlertStatus::FIRING) {
        active_alerts_.push_back(alert);
        THEMIS_DEBUG("Alert added to active alerts (total: {})", active_alerts_.size());
    }
    
    if (!config_.enabled) {
        THEMIS_DEBUG("Alertmanager disabled – alert logged only");
        return {};
    }

    // Build Prometheus Alertmanager v2 payload.
    // POST /api/v2/alerts  body: [ { labels, annotations, startsAt, endsAt } ]
    json labels_obj = json::object();
    labels_obj["alertname"] = alert.alert_name;
    labels_obj["severity"]  = severityToString(alert.severity);
    labels_obj["alertid"]   = alert.alert_id;
    for (const auto& [k, v] : alert.labels) { labels_obj[k] = v; }

    json annotations_obj = json::object();
    annotations_obj["summary"] = alert.message;
    for (const auto& [k, v] : alert.annotations) { annotations_obj[k] = v; }

    json payload = json::array();
    json entry;
    entry["labels"]      = labels_obj;
    entry["annotations"] = annotations_obj;
    entry["startsAt"]    = toISO8601(alert.fired_at);
    // A zero endsAt signals to Alertmanager that the alert is still firing.
    entry["endsAt"] = (alert.status == AlertStatus::RESOLVED)
                          ? toISO8601(alert.resolved_at)
                          : zeroISO8601();
    entry["generatorURL"] = "";
    payload.push_back(entry);

    auto result = postWithRetry("/api/v2/alerts", payload.dump());
    if (!result) {
        THEMIS_ERROR("Failed to send alert to Alertmanager: {}", result.error().message);
        return tl::unexpected(result.error());
    }

    THEMIS_INFO("Alert {} sent to Alertmanager (HTTP {})", alert.alert_name, *result);
    return {};
}

Result<void> DefaultAlertmanager::resolveAlert(const std::string& alert_id) {
    THEMIS_INFO("Resolving alert: {}", alert_id);
    
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                           [&alert_id](const Alert& a) { return a.alert_id == alert_id; });
    
    Alert resolved_alert;
    if (it != active_alerts_.end()) {
        it->status     = AlertStatus::RESOLVED;
        it->resolved_at = std::chrono::system_clock::now();
        resolved_alert  = *it;
        active_alerts_.erase(it);
        THEMIS_INFO("Alert {} resolved (removed from active alerts)", alert_id);
    } else {
        THEMIS_WARN("Alert {} not found in local active alerts", alert_id);
        if (!config_.enabled) return {};
        // Construct a minimal resolved alert for the API call.
        resolved_alert.alert_id   = alert_id;
        resolved_alert.status     = AlertStatus::RESOLVED;
        resolved_alert.resolved_at = std::chrono::system_clock::now();
        resolved_alert.fired_at   = resolved_alert.resolved_at; // best-effort
    }
    
    if (!config_.enabled) {
        return {};
    }

    // Resolve by setting endsAt = now
    json labels_obj = json::object();
    labels_obj["alertname"] = resolved_alert.alert_name.empty() ? alert_id : resolved_alert.alert_name;
    labels_obj["alertid"]   = alert_id;
    for (const auto& [k, v] : resolved_alert.labels) { labels_obj[k] = v; }

    json payload = json::array();
    json entry;
    entry["labels"]   = labels_obj;
    entry["startsAt"] = toISO8601(resolved_alert.fired_at);
    entry["endsAt"]   = toISO8601(resolved_alert.resolved_at);
    payload.push_back(entry);

    auto result = postWithRetry("/api/v2/alerts", payload.dump());
    if (!result) {
        THEMIS_ERROR("Failed to resolve alert in Alertmanager: {}", result.error().message);
        return tl::unexpected(result.error());
    }

    THEMIS_INFO("Alert {} resolved in Alertmanager (HTTP {})", alert_id, *result);
    return {};
}

Result<void> DefaultAlertmanager::silenceAlert(const std::string& alert_id,
                                               int duration_minutes) {
    THEMIS_INFO("Silencing alert {} for {} minutes", alert_id, duration_minutes);
    
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                           [&alert_id](const Alert& a) { return a.alert_id == alert_id; });
    if (it != active_alerts_.end()) {
        it->status = AlertStatus::SILENCED;
        THEMIS_INFO("Alert {} silenced in local store", alert_id);
    } else {
        THEMIS_WARN("Alert {} not found in local active alerts", alert_id);
    }
    
    if (!config_.enabled) {
        return {};
    }

    // POST /api/v2/silences
    auto now    = std::chrono::system_clock::now();
    auto end_tp = now + std::chrono::minutes(duration_minutes);

    json matcher;
    matcher["name"]    = "alertid";
    matcher["value"]   = alert_id;
    matcher["isRegex"] = false;
    matcher["isEqual"] = true;

    json payload;
    payload["matchers"]  = json::array({matcher});
    payload["startsAt"]  = toISO8601(now);
    payload["endsAt"]    = toISO8601(end_tp);
    payload["createdBy"] = "themisdb";
    payload["comment"]   = "Silenced via ThemisDB API for " +
                           std::to_string(duration_minutes) + " minutes";

    auto result = postWithRetry("/api/v2/silences", payload.dump());
    if (!result) {
        THEMIS_ERROR("Failed to silence alert in Alertmanager: {}", result.error().message);
        return tl::unexpected(result.error());
    }

    THEMIS_INFO("Alert {} silenced in Alertmanager (HTTP {})", alert_id, *result);
    return {};
}

std::vector<Alert> DefaultAlertmanager::getActiveAlerts() {
    THEMIS_DEBUG("Getting active alerts (count: {})", active_alerts_.size());
    return active_alerts_;
}

Result<void> DefaultAlertmanager::testConnection() {
    if (!config_.enabled) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            "Alertmanager is not enabled in configuration"
        });
    }
    
    THEMIS_INFO("Testing Alertmanager connection to: {}", config_.endpoint_url);

    ensureHttpPool();
    const std::string url = config_.endpoint_url + "/api/v2/status";
    std::unordered_map<std::string, std::string> headers;
    if (!config_.auth_token.empty()) {
        headers["Authorization"] = "Bearer " + config_.auth_token;
    }

    try {
        auto future = http_pool_->get(url, headers);
        auto resp   = future.get();
        if (resp.isSuccess()) {
            THEMIS_INFO("Alertmanager connection OK (HTTP {})", resp.status_code);
            return {};
        }
        THEMIS_WARN("Alertmanager status endpoint returned HTTP {}", resp.status_code);
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            "Alertmanager /api/v2/status returned HTTP " + std::to_string(resp.status_code)
        });
    } catch (const std::exception& ex) {
        THEMIS_ERROR("Alertmanager connection test failed: {}", ex.what());
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            std::string("Alertmanager connection test failed: ") + ex.what()
        });
    }
}

} // namespace observability
} // namespace themis
