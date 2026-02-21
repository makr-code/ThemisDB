/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            alertmanager.h                                     ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     214                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e1ba78e5d  2026-02-20  observability: production-ready Alertmanager, W3C TraceCo... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <chrono>
#include <memory>
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
    
    // Helper: Convert severity to string
    static std::string severityToString(AlertSeverity severity);
    
    // Helper: Convert status to string
    static std::string statusToString(AlertStatus status);
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

} // namespace observability
} // namespace themis
