/**
 * @file alertmanager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=4, H=1, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/alertmanager.h"
#include "utils/logger.h"
#include "utils/error_registry.h"
#include "utils/http_client_pool.h"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <atomic>
#include <cmath>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#ifdef ERROR
#undef ERROR
#endif

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
    std::ostringstream oss = {};
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

Result<void> Alertmanager::sendAlert([[maybe_unused]] const Alert& alert) {
    return tl::unexpected(Error{
        errors::ErrorCode::ERR_UTIL_UNSUPPORTED_OPERATION,
        "Alertmanager::sendAlert requires a concrete backend implementation"
    });
}

Result<void> Alertmanager::resolveAlert([[maybe_unused]] const std::string& alert_id) {
    return tl::unexpected(Error{
        errors::ErrorCode::ERR_UTIL_UNSUPPORTED_OPERATION,
        "Alertmanager::resolveAlert requires a concrete backend implementation"
    });
}

Result<void> Alertmanager::silenceAlert([[maybe_unused]] const std::string& alert_id, [[maybe_unused]] int duration_minutes) {
    return tl::unexpected(Error{
        errors::ErrorCode::ERR_UTIL_UNSUPPORTED_OPERATION,
        "Alertmanager::silenceAlert requires a concrete backend implementation"
    });
}

std::vector<Alert> Alertmanager::getActiveAlerts() {
    std::lock_guard<std::mutex> lock(active_alerts_mutex_);
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

std::optional<Alert> Alertmanager::findActiveAlertById(const std::string& alert_id) const {
    std::lock_guard<std::mutex> lock(active_alerts_mutex_);
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                           [&]([[maybe_unused]] const Alert& alert) { return alert.alert_id == alert_id; });
    if (it == active_alerts_.end()) {
        return std::nullopt;
    }
    return *it;
}

void Alertmanager::upsertActiveAlert(const Alert& alert) {
    std::lock_guard<std::mutex> lock(active_alerts_mutex_);
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                           [&]([[maybe_unused]] const Alert& existing) { return existing.alert_id == alert.alert_id; });
    if (it == active_alerts_.end()) {
        active_alerts_.push_back(alert);
    } else {
        *it = alert;
    }
}

bool Alertmanager::removeActiveAlertById(const std::string& alert_id, Alert* removed) {
    std::lock_guard<std::mutex> lock(active_alerts_mutex_);
    auto it = std::find_if(active_alerts_.begin(), active_alerts_.end(),
                           [&]([[maybe_unused]] const Alert& alert) { return alert.alert_id == alert_id; });
    if (it == active_alerts_.end()) {
        return false;
    }
    if (removed != nullptr) {
        *removed = *it;
    }
    active_alerts_.erase(it);
    return true;
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
    if (http_pool_) {
      return;
    }
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

    int attempts = (std::max)(1, config_.retry_count + 1);
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
    THEMIS_INFO("  Receivers: {}",static_cast<int>(config_.receivers.size()));
    
    if (!config_.enabled) {
        THEMIS_WARN("Alertmanager is disabled – alerts will only be logged");
        return {};
    }

    // Eagerly test connectivity so callers learn of misconfiguration early.
    auto conn = testConnection();
    if (!conn) {
        THEMIS_WARN("Alertmanager connectivity check failed: {}",
                conn.error().message());
    }

    return {};
}

Result<void> DefaultAlertmanager::sendAlert(const Alert& alert) {
    // Always log the alert regardless of enabled state.
    std::ostringstream ss = {};
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
        [[fallthrough]];\n        case AlertSeverity::CRITICAL:
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
        upsertActiveAlert(alert);
        {
            std::lock_guard<std::mutex> lock(active_alerts_mutex_);
            THEMIS_DEBUG("Alert added to active alerts (total: {})",static_cast<int>(active_alerts_.size()));
        }
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
        THEMIS_ERROR("Failed to send alert to Alertmanager: {}", result.error().message());
        return tl::unexpected(result.error());
    }

    THEMIS_INFO("Alert {} sent to Alertmanager (HTTP {})", alert.alert_name, *result);
    return {};
}

Result<void> DefaultAlertmanager::resolveAlert(const std::string& alert_id) {
    THEMIS_INFO("Resolving alert: {}", alert_id);
    
    Alert resolved_alert = {};
    if (removeActiveAlertById(alert_id, &resolved_alert)) {
        resolved_alert.status = AlertStatus::RESOLVED;
        resolved_alert.resolved_at = std::chrono::system_clock::now();
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
        THEMIS_ERROR("Failed to resolve alert in Alertmanager: {}", result.error().message());
        return tl::unexpected(result.error());
    }

    THEMIS_INFO("Alert {} resolved in Alertmanager (HTTP {})", alert_id, *result);
    return {};
}

Result<void> DefaultAlertmanager::silenceAlert(const std::string& alert_id,
                                               int duration_minutes) {
    THEMIS_INFO("Silencing alert {} for {} minutes", alert_id, duration_minutes);
    
    if (auto alert = findActiveAlertById(alert_id); alert.has_value()) {
        alert->status = AlertStatus::SILENCED;
        upsertActiveAlert(*alert);
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
        THEMIS_ERROR("Failed to silence alert in Alertmanager: {}", result.error().message());
        return tl::unexpected(result.error());
    }

    THEMIS_INFO("Alert {} silenced in Alertmanager (HTTP {})", alert_id, *result);
    return {};
}

std::vector<Alert> DefaultAlertmanager::getActiveAlerts() {
    const auto alerts = Alertmanager::getActiveAlerts();
    THEMIS_DEBUG("Getting active alerts (count: {})",static_cast<int>(alerts.size()));
    return alerts;
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
    std::unordered_map<std::string, std::string> headers = {};

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

// ============================================================================
// AlertRuleManager Implementation – Custom user-defined alert rules via API
// ============================================================================

namespace {

// Counter for generating unique rule IDs within this process.
std::atomic<uint64_t> g_rule_id_counter{0};

} // anonymous namespace

std::string AlertRuleManager::generateRuleId() {
    auto now = std::chrono::system_clock::now().time_since_epoch();
    auto ts  = static_cast<uint64_t>(
        std::chrono::duration_cast<std::chrono::milliseconds>(now).count());
    uint64_t seq = g_rule_id_counter.fetch_add(1, std::memory_order_relaxed);
    std::ostringstream oss = {};
    oss << "rule_" << std::hex << ts << "_" << seq;
    return oss.str();
}

std::string AlertRuleManager::expandMessage(const std::string& tmpl,
                                            const std::string& metric_name,
                                            double value) {
    std::string result = tmpl;
    // Replace {metric} placeholder
    std::string metric_token = "{metric}";
    for (std::string::size_type pos = result.find(metric_token);
         pos != std::string::npos;
         pos = result.find(metric_token, pos + static_cast<int>(metric_name.size()) )) {
        result.replace(pos,static_cast<int>(metric_token.size()), metric_name);
    }
    // Replace {value} placeholder
    std::string value_token = "{value}";
    std::string value_str   = std::to_string(value);
    // Trim trailing zeros for readability
    auto dot_pos = value_str.find('.');
    if (dot_pos != std::string::npos) {
        value_str.erase(value_str.find_last_not_of('0') + 1);
        if (value_str.back() == '.') {
          value_str.pop_back();
        }
    }
    for (std::string::size_type pos = result.find(value_token);
         pos != std::string::npos;
         pos = result.find(value_token, pos + static_cast<int>(value_str.size()) )) {
        result.replace(pos,static_cast<int>(value_token.size()), value_str);
    }
    return result;
}

bool AlertRuleManager::evaluateCondition(double value,
                                         AlertRuleOperator op,
                                         double threshold) {
    constexpr double kEpsilon = 1e-9;
    switch (op) {
        case AlertRuleOperator::GREATER_THAN:          return value >  threshold;
        case AlertRuleOperator::GREATER_THAN_OR_EQUAL: return value >= threshold;
        case AlertRuleOperator::LESS_THAN:             return value <  threshold;
        case AlertRuleOperator::LESS_THAN_OR_EQUAL:    return value <= threshold;
        case AlertRuleOperator::EQUAL:                 return std::abs(value - threshold) < kEpsilon;
        case AlertRuleOperator::NOT_EQUAL:             return std::abs(value - threshold) >= kEpsilon;
        default:                                       return false;
    }
}

Result<std::string> AlertRuleManager::addRule(AlertRule rule) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (rule.rule_id.empty()) {
        rule.rule_id = generateRuleId();
    }

    if (rules_.count(rule.rule_id)) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "Alert rule with id '" + rule.rule_id + "' already exists"
        });
    }
    if (rule.metric_name.empty()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "AlertRule.metric_name must not be empty"
        });
    }

    const std::string id = rule.rule_id;
    rules_.emplace(id, std::move(rule));
    THEMIS_INFO("AlertRuleManager: added rule '{}' for metric '{}'",
                id, rules_[id].metric_name);
    return id;
}

Result<void> AlertRuleManager::removeRule(const std::string& rule_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_POLICY_NOT_FOUND,
            "Alert rule '" + rule_id + "' not found"
        });
    }
    rules_.erase(it);
    active_rule_alerts_.erase(rule_id);
    THEMIS_INFO("AlertRuleManager: removed rule '{}'", rule_id);
    return {};
}

Result<AlertRule> AlertRuleManager::getRule(const std::string& rule_id) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rules_.find(rule_id);
    if (it == rules_.end()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_POLICY_NOT_FOUND,
            "Alert rule '" + rule_id + "' not found"
        });
    }
    return it->second;
}

Result<void> AlertRuleManager::updateRule(const AlertRule& rule) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rules_.find(rule.rule_id);
    if (it == rules_.end()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_POLICY_NOT_FOUND,
            "Alert rule '" + rule.rule_id + "' not found"
        });
    }
    if (rule.metric_name.empty()) {
        return tl::unexpected(Error{
            errors::ErrorCode::ERR_UTIL_INVALID_ARGUMENT,
            "AlertRule.metric_name must not be empty"
        });
    }
    it->second = rule;
    THEMIS_INFO("AlertRuleManager: updated rule '{}' for metric '{}'",
                rule.rule_id, rule.metric_name);
    return {};
}

std::vector<AlertRule> AlertRuleManager::listRules() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<AlertRule> result = {};

    result.reserve(rules_.size());
    for (const auto& [id, rule] : rules_) {
        result.push_back(rule);
    }
    return result;
}

int AlertRuleManager::evaluateRules(const std::map<std::string, double>& metrics,
                                    Alertmanager& alertmanager) {
    // Phase 1: snapshot the required actions while holding the lock.
    // No external calls (alertmanager) are made here to keep the critical section short.
    struct FireAction {
        std::string rule_id;
        std::string metric_name;  // captured for logging, avoids map look-up after lock release
        Alert       alert;
    };
    struct ResolveAction {
        std::string rule_id;
        std::string alert_id;
    };

    std::vector<FireAction>    to_fire;
    std::vector<ResolveAction> to_resolve;
    int already_firing = 0;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        for (const auto& [rule_id, rule] : rules_) {
            if (!rule.enabled) {
              continue;
            }

            auto metric_it = metrics.find(rule.metric_name);
            if (metric_it == metrics.end()) {
                THEMIS_DEBUG("AlertRuleManager: metric '{}' not present in snapshot (rule '{}')",
                             rule.metric_name, rule_id);
                continue;
            }

            const double value     = metric_it->second;
            const bool   firing    = evaluateCondition(value, rule.op, rule.threshold);
            const bool   was_active = (active_rule_alerts_.count(rule_id) > 0);

            if (firing && !was_active) {
                Alert alert;
                alert.alert_id   = rule_id + "_alert";
                alert.alert_name = rule.rule_name.empty() ? rule_id : rule.rule_name;
                alert.severity   = rule.severity;
                alert.status     = AlertStatus::FIRING;
                alert.message    = rule.message_template.empty()
                                       ? (rule.metric_name + " threshold exceeded")
                                       : expandMessage(rule.message_template, rule.metric_name, value);
                alert.labels     = rule.labels;
                alert.labels["rule_id"]     = rule_id;
                alert.labels["metric_name"] = rule.metric_name;
                alert.annotations           = rule.annotations;
                to_fire.push_back({rule_id, rule.metric_name, std::move(alert)});
            } else if (!firing && was_active) {
                to_resolve.push_back({rule_id, active_rule_alerts_[rule_id]});
            } else if (firing) {
                ++already_firing;
            }
        }
    } // lock released – alertmanager calls happen outside the critical section

    // Phase 2: fire new alerts outside the lock.
    int newly_fired = 0;
    for (auto& action : to_fire) {
        auto res = alertmanager.sendAlert(action.alert);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (res) {
                active_rule_alerts_[action.rule_id] = action.alert.alert_id;
                THEMIS_INFO("AlertRuleManager: rule '{}' fired (metric={}, alert_id={})",
                            action.rule_id, action.metric_name, action.alert.alert_id);
            } else {
                THEMIS_WARN("AlertRuleManager: failed to send alert for rule '{}': {}",
                            action.rule_id, res.error().message());
            }
        }
        ++newly_fired;
    }

    // Phase 3: resolve cleared alerts outside the lock.
    for (auto& action : to_resolve) {
        auto res = alertmanager.resolveAlert(action.alert_id);
        {
            std::lock_guard<std::mutex> lock(mutex_);
            if (res) {
                active_rule_alerts_.erase(action.rule_id);
                THEMIS_INFO("AlertRuleManager: rule '{}' resolved (alert_id={})",
                            action.rule_id, action.alert_id);
            } else {
                THEMIS_WARN("AlertRuleManager: failed to resolve alert for rule '{}': {}",
                            action.rule_id, res.error().message());
            }
        }
    }

    return already_firing + newly_fired;
}

void AlertRuleManager::clearRules() {
    std::lock_guard<std::mutex> lock(mutex_);
    rules_.clear();
    active_rule_alerts_.clear();
    THEMIS_INFO("AlertRuleManager: all rules cleared");
}

size_t AlertRuleManager::ruleCount() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return static_cast<int>(rules_.size());
}

} // namespace observability
} // namespace themis
