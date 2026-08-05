/**
 * @file alerting_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: alerting_engine.cpp | Version: 0.0.13 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 506
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=5, M=3, L=0
 * PR History (last 5): #3793 feat(observability): rule-b... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "observability/alerting_engine.h"
#include "utils/logger.h"
#include "utils/http_client_pool.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <ctime>
#include <sstream>
#include <iomanip>

#ifdef ERROR
#undef ERROR
#endif

using json = nlohmann::json;

namespace themis {
namespace observability {

// ============================================================================
// Internal helpers
// ============================================================================
namespace {

std::string toISO8601Engine(std::chrono::system_clock::time_point tp) {
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

std::string severityStr(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::INFO:     return "INFO";
        case AlertSeverity::WARNING:  return "WARNING";
        case AlertSeverity::ERROR:    return "ERROR";
        case AlertSeverity::CRITICAL: return "CRITICAL";
        default:                       return "UNKNOWN";
    }
}

std::string statusStr(AlertStatus status) {
    switch (status) {
        case AlertStatus::FIRING:   return "FIRING";
        case AlertStatus::RESOLVED: return "RESOLVED";
        case AlertStatus::SILENCED: return "SILENCED";
        default:                     return "UNKNOWN";
    }
}

// Build a JSON object representing the alert for webhook payloads.
json alertToJson(const Alert& alert) {
    json j;
    j["alert_name"] = alert.alert_name;
    j["alert_id"]   = alert.alert_id;
    j["severity"]   = severityStr(alert.severity);
    j["status"]     = statusStr(alert.status);
    j["message"]    = alert.message;
    j["fired_at"]   = toISO8601Engine(alert.fired_at);

    json labels = json::object();
    for (const auto& [k, v] : alert.labels) {
        labels[k] = v;
    }
    j["labels"] = labels;

    json annotations = json::object();
    for (const auto& [k, v] : alert.annotations) {
        annotations[k] = v;
    }
    j["annotations"] = annotations;

    return j;
}

} // anonymous namespace

// ============================================================================
// LogNotificationChannel
// ============================================================================

Result<void> LogNotificationChannel::send(const Alert& alert) {
    const std::string prefix =
        "[ALERT][" + severityStr(alert.severity) + "] " +
        alert.alert_name + " (" + statusStr(alert.status) + "): " +
        alert.message;

    switch (alert.severity) {
        case AlertSeverity::CRITICAL:
            THEMIS_CRITICAL("{}", prefix);
            break;
        case AlertSeverity::ERROR:
            THEMIS_ERROR("{}", prefix);
            break;
        case AlertSeverity::WARNING:
            THEMIS_WARN("{}", prefix);
            break;
        default:
            THEMIS_INFO("{}", prefix);
            break;
    }
    return {};
}

// ============================================================================
// WebhookNotificationChannel
// ============================================================================

WebhookNotificationChannel::WebhookNotificationChannel(WebhookChannelConfig config)
    : config_(std::move(config)) {}

Result<void> WebhookNotificationChannel::send(const Alert& alert) {
    if (config_.url.empty()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "WebhookNotificationChannel: endpoint URL is empty"
        });
    }

    utils::HTTPClientPool::Config pool_cfg;
    pool_cfg.max_connections  = 2;
    pool_cfg.connect_timeout  = std::chrono::seconds(config_.timeout_seconds);
    pool_cfg.request_timeout  = std::chrono::seconds(config_.timeout_seconds);
    pool_cfg.io_threads       = 1;
    pool_cfg.lock_stripes     = 1;

    utils::HTTPClientPool pool(pool_cfg);

    std::unordered_map<std::string, std::string> headers{
        {"Content-Type", "application/json"}
    };
    for (const auto& [k, v] : config_.headers) {
        headers[k] = v;
    }

    const json payload = alertToJson(alert);

    try {
        auto future = pool.post(config_.url, payload, headers);
        auto resp   = future.get();
        if (!resp.isSuccess()) {
            return tl::unexpected(Error{
                errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
                "WebhookNotificationChannel: HTTP " + std::to_string(resp.status_code) +
                " from " + config_.url
            });
        }
    } catch (const std::exception& ex) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            std::string("WebhookNotificationChannel: ") + ex.what()
        });
    }

    return {};
}

// ============================================================================
// SlackNotificationChannel
// ============================================================================

SlackNotificationChannel::SlackNotificationChannel(SlackChannelConfig config)
    : config_(std::move(config)) {}

// static
std::string SlackNotificationChannel::severityColor(AlertSeverity severity) {
    switch (severity) {
        case AlertSeverity::CRITICAL: return "#7b0000";
        case AlertSeverity::ERROR:    return "#e01e5a";
        case AlertSeverity::WARNING:  return "#ff9900";
        default:                       return "#36a64f";
    }
}

Result<void> SlackNotificationChannel::send(const Alert& alert) {
    if (config_.webhook_url.empty()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "SlackNotificationChannel: webhook_url is empty"
        });
    }

    const std::string status_str = statusStr(alert.status);
    const std::string sev_str    = severityStr(alert.severity);
    const std::string color      = severityColor(alert.severity);

    // Build attachment fields from labels
    json fields = json::array();
    for (const auto& [k, v] : alert.labels) {
        json field;
        field["title"] = k;
        field["value"] = v;
        field["short"] = true;
        fields.push_back(field);
    }

    json attachment;
    attachment["fallback"] = sev_str + ": " + alert.alert_name + " — " + alert.message;
    attachment["color"]    = color;
    attachment["title"]    = "[" + sev_str + "] " + alert.alert_name;
    attachment["text"]     = alert.message;
    attachment["ts"]       = std::chrono::duration_cast<std::chrono::seconds>(
                                 alert.fired_at.time_since_epoch()).count();
    attachment["fields"]   = fields;
    attachment["footer"]   = "ThemisDB Alerting Engine";

    json payload;
    payload["attachments"] = json::array({attachment});
    if (!config_.channel.empty()) {
        payload["channel"] = config_.channel;
    }
    if (!config_.username.empty()) {
        payload["username"] = config_.username;
    }

    utils::HTTPClientPool::Config pool_cfg;
    pool_cfg.max_connections  = 2;
    pool_cfg.connect_timeout  = std::chrono::seconds(config_.timeout_seconds);
    pool_cfg.request_timeout  = std::chrono::seconds(config_.timeout_seconds);
    pool_cfg.io_threads       = 1;
    pool_cfg.lock_stripes     = 1;

    utils::HTTPClientPool pool(pool_cfg);

    std::unordered_map<std::string, std::string> headers{
        {"Content-Type", "application/json"}
    };

    try {
        auto future = pool.post(config_.webhook_url, payload, headers);
        auto resp   = future.get();
        if (!resp.isSuccess()) {
            return tl::unexpected(Error{
                errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
                "SlackNotificationChannel: HTTP " + std::to_string(resp.status_code)
            });
        }
    } catch (const std::exception& ex) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            std::string("SlackNotificationChannel: ") + ex.what()
        });
    }

    return {};
}

// ============================================================================
// AlertingEngine
// ============================================================================

AlertingEngine::AlertingEngine(std::shared_ptr<Alertmanager> backend)
    : backend_(std::move(backend)) {}

// --- Channel management ------------------------------------------------------

void AlertingEngine::addChannel(std::shared_ptr<INotificationChannel> channel) {
    if (!channel) return;
    std::lock_guard<std::mutex> lock(channels_mutex_);
    channels_.push_back(std::move(channel));
}

void AlertingEngine::clearChannels() {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    channels_.clear();
}

std::vector<std::shared_ptr<INotificationChannel>> AlertingEngine::channels() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return channels_;
}

size_t AlertingEngine::channelCount() const {
    std::lock_guard<std::mutex> lock(channels_mutex_);
    return channels_.size();
}

// --- Predefined rules --------------------------------------------------------

void AlertingEngine::loadDefaultRules() {
    struct DefaultRuleDef {
        const char* rule_id;
        const char* rule_name;
        const char* metric_name;
        AlertRuleOperator op;
        double threshold;
        AlertSeverity severity;
        const char* message_template;
    };

    static const DefaultRuleDef kDefaults[] = {
        {
            "default_cpu_high",
            "High CPU Utilization",
            "themis_cpu_usage_percent",
            AlertRuleOperator::GREATER_THAN,
            80.0,
            AlertSeverity::WARNING,
            "CPU utilization {value}% exceeds threshold 80% on metric {metric}"
        },
        {
            "default_memory_high",
            "High Memory Utilization",
            "themis_memory_usage_percent",
            AlertRuleOperator::GREATER_THAN,
            90.0,
            AlertSeverity::ERROR,
            "Memory utilization {value}% exceeds threshold 90% on metric {metric}"
        },
        {
            "default_query_latency_p99",
            "High Query Latency P99",
            "themis_query_latency_p99_ms",
            AlertRuleOperator::GREATER_THAN,
            1000.0,
            AlertSeverity::WARNING,
            "P99 query latency {value}ms exceeds 1000ms threshold on metric {metric}"
        },
        {
            "default_error_rate_high",
            "Elevated Error Rate",
            "themis_error_rate_percent",
            AlertRuleOperator::GREATER_THAN,
            5.0,
            AlertSeverity::ERROR,
            "Error rate {value}% exceeds 5% threshold on metric {metric}"
        },
        {
            "default_disk_low",
            "Low Disk Free Space",
            "themis_disk_free_percent",
            AlertRuleOperator::LESS_THAN,
            10.0,
            AlertSeverity::CRITICAL,
            "Disk free space {value}% is below 10% threshold on metric {metric}"
        },
        {
            "default_query_queue_deep",
            "Deep Query Queue",
            "themis_query_queue_depth",
            AlertRuleOperator::GREATER_THAN,
            100.0,
            AlertSeverity::WARNING,
            "Query queue depth {value} exceeds 100 on metric {metric}"
        },
        {
            "default_cache_miss_high",
            "High Cache Miss Rate",
            "themis_cache_miss_rate_percent",
            AlertRuleOperator::GREATER_THAN,
            50.0,
            AlertSeverity::WARNING,
            "Cache miss rate {value}% exceeds 50% threshold on metric {metric}"
        },
        {
            "default_write_amplification",
            "High Storage Write Amplification",
            "themis_storage_write_amplification",
            AlertRuleOperator::GREATER_THAN,
            20.0,
            AlertSeverity::WARNING,
            "Storage write amplification {value}x exceeds 20x threshold on metric {metric}"
        },
    };

    for (const auto& def : kDefaults) {
        // Skip if already registered (idempotent).
        if (rule_manager_.getRule(def.rule_id).has_value()) continue;

        AlertRule rule;
        rule.rule_id          = def.rule_id;
        rule.rule_name        = def.rule_name;
        rule.metric_name      = def.metric_name;
        rule.op               = def.op;
        rule.threshold        = def.threshold;
        rule.severity         = def.severity;
        rule.message_template = def.message_template;
        rule.enabled          = true;
        rule.labels["component"] = "alerting_engine";

        auto res = rule_manager_.addRule(rule);
        if (!res.has_value()) {
            THEMIS_WARN("AlertingEngine::loadDefaultRules: failed to add rule '{}': {}",
                        def.rule_id, res.error().message());
        }
    }
}

// --- Evaluation --------------------------------------------------------------

int AlertingEngine::evaluateAndNotify(const std::map<std::string, double>& metrics) {
    return rule_manager_.evaluateRules(metrics, *this);
}

// --- Alertmanager overrides --------------------------------------------------

Result<void> AlertingEngine::dispatchToChannels(const Alert& alert) {
    std::vector<std::shared_ptr<INotificationChannel>> snapshot;
    {
        std::lock_guard<std::mutex> lock(channels_mutex_);
        snapshot = channels_;
    }

    std::vector<std::string> failures;
    for (const auto& ch : snapshot) {
        auto res = ch->send(alert);
        if (!res.has_value()) {
            failures.push_back(ch->channelType() + ": " + res.error().message());
            THEMIS_WARN("AlertingEngine: channel '{}' failed to send alert '{}': {}",
                        ch->channelType(), alert.alert_name, res.error().message());
        }
    }

    if (!failures.empty()) {
        std::ostringstream oss;
        for (size_t i = 0; i < failures.size(); ++i) {
            if (i != 0) {
                oss << "; ";
            }
            oss << failures[i];
        }
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_NET_CONNECTION_REFUSED,
            "AlertingEngine::dispatchToChannels partial failure: " + oss.str()
        });
    }
    return {};
}

Result<void> AlertingEngine::sendAlert(const Alert& alert) {
    upsertActiveAlert(alert);

    auto channel_result = dispatchToChannels(alert);

    Result<void> backend_result;
    if (backend_) {
        backend_result = backend_->sendAlert(alert);
        if (!backend_result.has_value()) {
            THEMIS_WARN("AlertingEngine: backend sendAlert failed for '{}': {}",
                        alert.alert_name, backend_result.error().message());
        }
    }

    if (!channel_result.has_value()) {
        return channel_result;
    }
    if (backend_ && !backend_result.has_value()) {
        return backend_result;
    }
    return {};
}

Result<void> AlertingEngine::resolveAlert(const std::string& alert_id) {
    auto active = findActiveAlertById(alert_id);
    Result<void> channel_result;
    if (active.has_value()) {
        active->status = AlertStatus::RESOLVED;
        active->resolved_at = std::chrono::system_clock::now();
        channel_result = dispatchToChannels(*active);
        removeActiveAlertById(alert_id);
    }

    Result<void> backend_result;
    if (backend_) {
        backend_result = backend_->resolveAlert(alert_id);
        if (!backend_result.has_value()) {
            THEMIS_WARN("AlertingEngine: backend resolveAlert failed for '{}': {}",
                        alert_id, backend_result.error().message());
        }
    }

    if (active.has_value() && !channel_result.has_value()) {
        return channel_result;
    }
    if (backend_ && !backend_result.has_value()) {
        return backend_result;
    }
    return {};
}

Result<void> AlertingEngine::silenceAlert(const std::string& alert_id,
                                          int duration_minutes) {
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                           [&](const Alert& a) { return a.alert_id == alert_id; });
    if (auto active = findActiveAlertById(alert_id); active.has_value()) {
        active->status = AlertStatus::SILENCED;
        upsertActiveAlert(*active);
    }

    if (backend_) {
        return backend_->silenceAlert(alert_id, duration_minutes);
    }
    return {};
}

Result<void> AlertingEngine::testConnection() {
    if (backend_) {
        return backend_->testConnection();
    }
    // No backend configured — engine is self-contained; return success.
    return {};
}

} // namespace observability
} // namespace themis
