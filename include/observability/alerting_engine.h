/**
 * @file alerting_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "observability/alertmanager.h"
#include "utils/expected.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <map>

namespace themis {
namespace observability {

// ============================================================================
// INotificationChannel — pluggable notification backend
// ============================================================================

/**
 * Abstract interface for notification channel backends.
 *
 * Implementations send alert payloads to external systems (Slack, Teams,
 * PagerDuty, generic webhooks, local log, etc.).  Each channel is invoked
 * synchronously inside AlertingEngine::sendAlert(); expensive channels should
 * perform their own async dispatch internally.
 */
class INotificationChannel {
public:
    virtual ~INotificationChannel() = default;

    /**
     * Return a human-readable type identifier (e.g., "log", "webhook", "slack").
     */
    [[nodiscard]] virtual std::string channelType() const = 0;

    /**
     * Send a notification for the given alert.
     *
     * @param alert  The alert being fired (status == FIRING) or resolved
     *               (status == RESOLVED).
     * @return Result<void> on success, or an Error describing the failure.
     */
    [[nodiscard]] virtual Result<void> send(const Alert& alert) = 0;
};

// ============================================================================
// LogNotificationChannel — writes alerts to the structured logger
// ============================================================================

/**
 * Notification channel that logs alert payloads via the ThemisDB logger.
 *
 * Severity mapping:
 *  - AlertSeverity::INFO     → THEMIS_INFO
 *  - AlertSeverity::WARNING  → THEMIS_WARN
 *  - AlertSeverity::ERROR    → THEMIS_ERROR
 *  - AlertSeverity::CRITICAL → THEMIS_CRITICAL
 */
class LogNotificationChannel : public INotificationChannel {
public:
    LogNotificationChannel() = default;
    ~LogNotificationChannel() override = default;

    std::string channelType() const override { return "log"; }

    /**
     * Log the alert using the appropriate log level.
     * Always returns success; logging failures are swallowed.
     */
    Result<void> send(const Alert& alert) override;
};

// ============================================================================
// WebhookNotificationChannel — posts JSON payload to any HTTP endpoint
// ============================================================================

/**
 * Configuration for a generic HTTP webhook notification channel.
 */
struct WebhookChannelConfig {
    /// Full URL of the webhook endpoint.
    std::string url;

    /// Additional HTTP headers (e.g., {"Authorization": "Bearer token"}).
    std::map<std::string, std::string> headers;

    /// Connection / request timeout in seconds.
    int timeout_seconds = 5;

    WebhookChannelConfig() = default;
};

/**
 * Notification channel that POSTs a JSON alert payload to an HTTP webhook.
 *
 * The JSON body format:
 * @code
 * {
 *   "alert_name": "...",
 *   "alert_id":   "...",
 *   "severity":   "WARNING",
 *   "status":     "FIRING",
 *   "message":    "...",
 *   "labels":     { ... },
 *   "annotations":{ ... },
 *   "fired_at":   "2026-01-01T00:00:00Z"
 * }
 * @endcode
 */
class WebhookNotificationChannel : public INotificationChannel {
public:
    explicit WebhookNotificationChannel(WebhookChannelConfig config);
    ~WebhookNotificationChannel() override = default;

    std::string channelType() const override { return "webhook"; }

    /**
     * POST the alert as JSON to the configured endpoint.
     * Returns an error if the HTTP request fails or returns a non-2xx status.
     */
    Result<void> send(const Alert& alert) override;

    const WebhookChannelConfig& config() const { return config_; }

private:
    WebhookChannelConfig config_;
};

// ============================================================================
// SlackNotificationChannel — posts Slack-formatted webhook message
// ============================================================================

/**
 * Configuration for a Slack incoming-webhook notification channel.
 */
struct SlackChannelConfig {
    /// Slack Incoming Webhook URL (https://hooks.slack.com/services/...).
    std::string webhook_url;

    /// Optional channel override (e.g., "#themis-alerts").
    std::string channel;

    /// Bot display name override.
    std::string username = "ThemisDB Alerting";

    /// Connection / request timeout in seconds.
    int timeout_seconds = 5;

    SlackChannelConfig() = default;
};

/**
 * Notification channel that posts formatted messages to a Slack channel
 * via a Slack Incoming Webhook URL.
 *
 * Severity → attachment color mapping:
 *  - INFO     → "#36a64f" (green)
 *  - WARNING  → "#ff9900" (orange)
 *  - ERROR    → "#e01e5a" (red)
 *  - CRITICAL → "#7b0000" (dark red)
 */
class SlackNotificationChannel : public INotificationChannel {
public:
    explicit SlackNotificationChannel(SlackChannelConfig config);
    ~SlackNotificationChannel() override = default;

    std::string channelType() const override { return "slack"; }

    /**
     * POST a Slack attachment payload to the configured webhook URL.
     * Returns an error if the HTTP request fails or returns a non-2xx status.
     */
    Result<void> send(const Alert& alert) override;

    const SlackChannelConfig& config() const { return config_; }

private:
    SlackChannelConfig config_;

    // Build the Slack message attachment color string from severity.
    static std::string severityColor(AlertSeverity severity);
};

// ============================================================================
// AlertingEngine — rule-based alerting with pluggable notification channels
// ============================================================================

/**
 * Central alerting engine that combines:
 *  - An AlertRuleManager for rule CRUD and evaluation
 *  - A set of pluggable INotificationChannel backends
 *  - An optional Alertmanager backend for Prometheus Alertmanager state tracking
 *
 * The engine itself extends Alertmanager so that AlertRuleManager::evaluateRules()
 * can be called with `*this` as the alertmanager argument.  When a rule fires,
 * sendAlert() is invoked on the engine which in turn dispatches the alert to all
 * registered notification channels and (optionally) to the wrapped backend.
 *
 * Typical usage:
 * @code
 *   AlertingEngine engine;
 *   engine.addChannel(std::make_shared<LogNotificationChannel>());
 *
 *   SlackChannelConfig slack;
 *   slack.webhook_url = "https://hooks.slack.com/services/...";
 *   engine.addChannel(std::make_shared<SlackNotificationChannel>(slack));
 *
 *   engine.loadDefaultRules();  // predefined rules for CPU, memory, latency, ...
 *
 *   // Periodically:
 *   std::map<std::string, double> metrics = collectCurrentMetrics();
 *   int fired = engine.evaluateAndNotify(metrics);
 * @endcode
 *
 * Thread-safety: addChannel/clearChannels and evaluateAndNotify are thread-safe
 * via an internal mutex on the channels vector.  AlertRuleManager is independently
 * thread-safe.
 */
class AlertingEngine : public Alertmanager {
public:
    /**
     * Construct an AlertingEngine.
     * @param backend  Optional Alertmanager backend for Prometheus state tracking.
     *                 If nullptr, alert state is tracked in-process only.
     */
    explicit AlertingEngine(std::shared_ptr<Alertmanager> backend = nullptr);
    ~AlertingEngine() override = default;

    // Non-copyable (contains std::mutex)
    AlertingEngine(const AlertingEngine&) = delete;
    AlertingEngine& operator=(const AlertingEngine&) = delete;

    // -------------------------------------------------------------------------
    // Channel management
    // -------------------------------------------------------------------------

    /**
     * Register a notification channel.  Channels are called in registration order.
     */
    void addChannel(std::shared_ptr<INotificationChannel> channel);

    /**
     * Remove all registered notification channels.
     */
    void clearChannels();

    /**
     * Return a snapshot of the currently-registered channels.
     */
    std::vector<std::shared_ptr<INotificationChannel>> channels() const;

    /**
     * Return the number of registered channels.
     */
    size_t channelCount() const;

    // -------------------------------------------------------------------------
    // Rule management (delegates to the internal AlertRuleManager)
    // -------------------------------------------------------------------------

    /**
     * Access the underlying rule manager for CRUD operations.
     */
    AlertRuleManager& ruleManager() { return rule_manager_; }
    const AlertRuleManager& ruleManager() const { return rule_manager_; }

    /**
     * Load predefined alert rules that cover the major error and performance
     * event categories:
     *  - High CPU utilization (> 80%)
     *  - High memory utilization (> 90%)
     *  - P99 query latency spike (> 1000 ms)
     *  - Elevated error rate (> 5%)
     *  - Low disk free space (< 10%)
     *  - Deep query queue (> 100 pending queries)
     *  - High cache miss rate (> 50%)
     *  - Storage write amplification (> 20×)
     *
     * Existing rules with the same IDs are skipped (idempotent).
     */
    void loadDefaultRules();

    // -------------------------------------------------------------------------
    // Evaluation
    // -------------------------------------------------------------------------

    /**
     * Evaluate all registered rules against the provided metric snapshot and
     * dispatch notifications for any newly-firing rules via all channels.
     *
     * @param metrics  Map of metric_name → current value.
     * @return Number of rules whose condition was satisfied during this evaluation.
     */
    int evaluateAndNotify(const std::map<std::string, double>& metrics);

    // -------------------------------------------------------------------------
    // Alertmanager overrides
    // -------------------------------------------------------------------------

    /**
     * Dispatch the alert to all registered notification channels and
     * (optionally) forward to the backend Alertmanager.
     */
    Result<void> sendAlert(const Alert& alert) override;

    /**
     * Resolve the alert (forward to backend; update in-process state).
     */
    Result<void> resolveAlert(const std::string& alert_id) override;

    /**
     * Silence an alert (forwarded to backend when available).
     */
    Result<void> silenceAlert(const std::string& alert_id, int duration_minutes) override;

    /**
     * Test connectivity of the backend Alertmanager (if any).
     */
    Result<void> testConnection() override;

private:
    AlertRuleManager rule_manager_;
    std::shared_ptr<Alertmanager> backend_;

    mutable std::mutex channels_mutex_;
    std::vector<std::shared_ptr<INotificationChannel>> channels_;

    // Dispatch a single alert to all registered channels, collecting failures.
    [[nodiscard]] Result<void> dispatchToChannels(const Alert& alert);
};

} // namespace observability
} // namespace themis
