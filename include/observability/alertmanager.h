/**
 * @file alertmanager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include "utils/expected.h"

// Forward declaration to avoid header bloat
namespace themis { namespace utils { class HTTPClientPool; } }

namespace themis {
namespace observability {

/**
 * Alert severity levels
 */
enum class AlertSeverity {
    INFO,       // Informational alert
    WARNING,    // Warning - potential issue
    ERROR,      // Error - requires attention
    CRITICAL    // Critical - requires immediate action
};

/**
 * Alert status
 */
enum class AlertStatus {
    FIRING,     // Alert is currently firing
    RESOLVED,   // Alert has been resolved
    SILENCED    // Alert is silenced/acknowledged
};

/**
 * Alert definition
 */
struct Alert {
    std::string alert_name;         // Unique alert name
    std::string alert_id;           // Unique alert ID
    AlertSeverity severity;         // Alert severity
    AlertStatus status;             // Current status
    std::string message;            // Alert message
    std::map<std::string, std::string> labels;   // Alert labels (component, instance, etc.)
    std::map<std::string, std::string> annotations;  // Additional context
    std::chrono::system_clock::time_point fired_at;  // When alert started firing
    std::chrono::system_clock::time_point resolved_at;  // When alert was resolved (if applicable)
    
    Alert()
        : severity(AlertSeverity::INFO)
        , status(AlertStatus::FIRING)
        , fired_at(std::chrono::system_clock::now()) {}
};

/**
 * Alertmanager configuration
 */
struct AlertmanagerConfig {
    std::string endpoint_url;       // Alertmanager endpoint URL
    std::string auth_token;         // Optional authentication token
    int timeout_seconds;            // Request timeout
    bool enabled;                   // Whether alerting is enabled
    std::vector<std::string> receivers;  // Alert receivers (email, slack, pagerduty)
    int retry_count;                // Number of retries on transient failures (0 = no retry)
    int retry_delay_ms;             // Delay between retries in milliseconds
    
    AlertmanagerConfig()
        : timeout_seconds(10)
        , enabled(false)
        , retry_count(3)
        , retry_delay_ms(500) {}
};

/**
 * Base Alertmanager interface for ThemisDB observability.
 *
 * Provides integration with Prometheus Alertmanager or compatible systems:
 * - Send alerts for critical system events
 * - Resolve alerts when issues are fixed
 * - Silence / acknowledge active alerts
 *
 * Production use: instantiate DefaultAlertmanager which implements the
 * Prometheus Alertmanager v2 HTTP API with retry and auth-token support.
 */
class Alertmanager {
public:
    Alertmanager() = default;
    explicit Alertmanager(const AlertmanagerConfig& config) : config_(config) {}
    virtual ~Alertmanager() = default;
    
    /**
     * Initialize alertmanager with configuration.
     * @param config: Alertmanager configuration
     * @return Result<void> on success, Error on failure
     */
    virtual Result<void> initialize(const AlertmanagerConfig& config);
    
    /**
     * Send an alert to the alertmanager backend.
     * @param alert: Alert to send
     * @return Result<void> on success, Error on failure
     */
    virtual Result<void> sendAlert(const Alert& alert);
    
    /**
     * Resolve a previously-fired alert.
     * @param alert_id: ID of alert to resolve
     * @return Result<void> on success, Error on failure
     */
    virtual Result<void> resolveAlert(const std::string& alert_id);
    
    /**
     * Silence an alert for a given duration.
     * @param alert_id: ID of alert to silence
     * @param duration_minutes: Duration to silence for
     * @return Result<void> on success, Error on failure
     */
    virtual Result<void> silenceAlert(const std::string& alert_id, int duration_minutes);
    
    /**
     * Get all currently active (firing or silenced) alerts.
     * @return Vector of active alerts
     */
    virtual std::vector<Alert> getActiveAlerts();
    
    /**
     * Test connectivity to the alertmanager backend.
     * @return Result<void> on success, Error on failure
     */
    virtual Result<void> testConnection();
    
    /**
     * Check if alertmanager is enabled
     * @return true if enabled, false otherwise
     */
    bool isEnabled() const { return config_.enabled; }
    
    /**
     * Get alertmanager configuration
     * @return Current configuration
     */
    const AlertmanagerConfig& getConfig() const { return config_; }
    
protected:
    AlertmanagerConfig config_;
    std::vector<Alert> active_alerts_;
    mutable std::mutex active_alerts_mutex_;
    
    // Helper: Convert severity to string
    static std::string severityToString(AlertSeverity severity);
    
    // Helper: Convert status to string
    static std::string statusToString(AlertStatus status);

    [[nodiscard]] std::optional<Alert> findActiveAlertById(const std::string& alert_id) const;
    void upsertActiveAlert(const Alert& alert);
    [[nodiscard]] bool removeActiveAlertById(const std::string& alert_id, Alert* removed = nullptr);
};

/**
 * Default Alertmanager implementation with Prometheus Alertmanager v2 API integration.
 * 
 * When enabled, sends alerts via HTTP POST to the Prometheus Alertmanager REST API
 * (/api/v2/alerts and /api/v2/silences).  Supports retry-on-failure and connection
 * health-checks.  When disabled, alerts are logged locally only.
 */
class DefaultAlertmanager : public Alertmanager {
public:
    DefaultAlertmanager() = default;
    explicit DefaultAlertmanager(const AlertmanagerConfig& config);
    ~DefaultAlertmanager() override = default;
    
    Result<void> initialize(const AlertmanagerConfig& config) override;
    Result<void> sendAlert(const Alert& alert) override;
    Result<void> resolveAlert(const std::string& alert_id) override;
    Result<void> silenceAlert(const std::string& alert_id, int duration_minutes) override;
    std::vector<Alert> getActiveAlerts() override;
    Result<void> testConnection() override;

private:
    // Lazily-created HTTP client pool (only allocated when enabled)
    std::shared_ptr<utils::HTTPClientPool> http_pool_;

    // Build the shared client pool if not already initialised
    void ensureHttpPool();

    // Send a JSON payload to the Alertmanager with retry logic.
    // Returns the HTTP status code on success or an Error on final failure.
    Result<int> postWithRetry(const std::string& path, const std::string& json_body);
};

/**
 * Comparison operators used in custom alert rule conditions.
 */
enum class AlertRuleOperator {
    GREATER_THAN,           // metric_value > threshold
    GREATER_THAN_OR_EQUAL,  // metric_value >= threshold
    LESS_THAN,              // metric_value < threshold
    LESS_THAN_OR_EQUAL,     // metric_value <= threshold
    EQUAL,                  // metric_value == threshold (within epsilon)
    NOT_EQUAL               // metric_value != threshold
};

/**
 * A user-defined alert rule that fires when a named metric satisfies a threshold condition.
 *
 * Rules are evaluated by AlertRuleManager::evaluateRules() against a metric snapshot
 * and translated into Alert objects that are dispatched via Alertmanager.
 */
struct AlertRule {
    std::string rule_id;        // Unique rule identifier (auto-generated when empty on addRule)
    std::string rule_name;      // Human-readable name displayed in fired alerts
    std::string metric_name;    // Name of the metric to monitor (must match MetricsCollector namespace)
    AlertRuleOperator op;       // Comparison operator applied to the metric value
    double threshold;           // Threshold value used in the condition
    AlertSeverity severity;     // Severity of alerts fired by this rule
    std::string message_template; // Alert message; {metric} and {value} are substituted at evaluation time
    std::map<std::string, std::string> labels;      // Extra labels attached to fired alerts
    std::map<std::string, std::string> annotations; // Extra annotations attached to fired alerts
    bool enabled;               // When false the rule is stored but never evaluated

    AlertRule()
        : op(AlertRuleOperator::GREATER_THAN)
        , threshold(0.0)
        , severity(AlertSeverity::WARNING)
        , enabled(true) {}
};

/**
 * Manager for custom user-defined alert rules.
 *
 * Provides a full CRUD API for alert rules plus an evaluation method that checks
 * all enabled rules against a metric snapshot and fires/resolves alerts via an
 * Alertmanager backend.
 *
 * Thread-safety: all methods are thread-safe via an internal mutex.
 *
 * Typical usage:
 * @code
 *   AlertRuleManager mgr;
 *   AlertRule rule;
 *   rule.metric_name = "themis_query_latency_p99_ms";
 *   rule.op          = AlertRuleOperator::GREATER_THAN;
 *   rule.threshold   = 500.0;
 *   rule.severity    = AlertSeverity::WARNING;
 *   rule.message_template = "P99 latency {value}ms exceeds threshold";
 *   auto res = mgr.addRule(rule);
 *   // ...
 *   mgr.evaluateRules(metric_snapshot, alertmanager);
 * @endcode
 */
class AlertRuleManager {
public:
    AlertRuleManager() = default;
    ~AlertRuleManager() = default;

    // Non-copyable, non-movable (std::mutex member is not movable)
    AlertRuleManager(const AlertRuleManager&) = delete;
    AlertRuleManager& operator=(const AlertRuleManager&) = delete;
    AlertRuleManager(AlertRuleManager&&) = delete;
    AlertRuleManager& operator=(AlertRuleManager&&) = delete;

    /**
     * Register a new alert rule.
     * If rule.rule_id is empty a unique ID is generated automatically.
     * @return The assigned rule_id on success, or an Error if the ID already exists.
     */
    Result<std::string> addRule(AlertRule rule);

    /**
     * Remove a rule by ID.
     * @return Result<void> on success, or Error if the rule is not found.
     */
    Result<void> removeRule(const std::string& rule_id);

    /**
     * Retrieve a rule by ID.
     * @return Copy of the AlertRule on success, or Error if not found.
     */
    Result<AlertRule> getRule(const std::string& rule_id) const;

    /**
     * Replace an existing rule with an updated version (identified by rule.rule_id).
     * @return Result<void> on success, or Error if the rule_id is not found.
     */
    Result<void> updateRule(const AlertRule& rule);

    /**
     * List all registered rules (enabled and disabled).
     */
    std::vector<AlertRule> listRules() const;

    /**
     * Evaluate all enabled rules against a metric snapshot and dispatch alerts.
     *
     * For each rule whose condition is satisfied a FIRING alert is sent via
     * @p alertmanager (if not already active).  When a previously-firing rule
     * no longer satisfies its condition the corresponding alert is resolved.
     *
     * @param metrics  Map of metric_name → current value.
     * @param alertmanager  Alertmanager used to send/resolve alerts.
     * @return Number of rules whose condition was satisfied during this evaluation.
     */
    int evaluateRules(const std::map<std::string, double>& metrics,
                      Alertmanager& alertmanager);

    /**
     * Remove all registered rules and clear any tracked alert state.
     */
    void clearRules();

    /**
     * Return the number of registered rules.
     */
    size_t ruleCount() const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, AlertRule> rules_;
    // Tracks active alert IDs for rules that are currently firing: rule_id → alert_id
    std::map<std::string, std::string> active_rule_alerts_;

    // Evaluate a single comparison; returns true when condition is met.
    static bool evaluateCondition(double value, AlertRuleOperator op, double threshold);

    // Generate a unique rule ID.
    static std::string generateRuleId();

    // Expand {metric} and {value} placeholders in a message template.
    static std::string expandMessage(const std::string& tmpl,
                                     const std::string& metric_name,
                                     double value);
};

} // namespace observability
} // namespace themis
