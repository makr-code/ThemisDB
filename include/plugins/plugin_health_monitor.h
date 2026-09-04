/**
 * @file plugin_health_monitor.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "core/concerns/i_metrics.h"
#include "plugins/self_healing_plugin.h"
#include "utils/expected.h"

namespace themis {
namespace plugins {

/**
 * @brief Configuration for health monitoring
 */
struct HealthMonitorConfig {
    /// Interval between health checks
    std::chrono::seconds check_interval{30};
    
    /// Maximum recovery attempts before giving up
    uint32_t max_recovery_attempts = 3;
    
    /// Backoff strategy: "none", "linear", "exponential"
    std::string backoff_strategy = "exponential";
    
    /// Initial backoff duration
    std::chrono::seconds initial_backoff{5};
    
    /// Maximum backoff duration
    std::chrono::seconds max_backoff{300};
    
    /// Notify administrators on critical failures
    bool notify_on_critical = true;
    
    /// Automatically disable plugin after max failed recoveries
    bool auto_disable_on_failure = true;
    
    /// Health check timeout
    std::chrono::seconds health_check_timeout{10};
    
    /// Recovery timeout
    std::chrono::seconds recovery_timeout{30};
};

/**
 * @brief Monitored plugin information
 */
struct MonitoredPlugin {
    std::string name;
    ISelfHealingPlugin* plugin;
    PluginDiagnostics last_diagnostics;
    std::chrono::system_clock::time_point last_check;
    std::chrono::system_clock::time_point last_recovery_attempt;
    
    uint32_t consecutive_failures = 0;
    uint32_t total_recovery_attempts = 0;
    uint32_t successful_recoveries = 0;
    uint32_t failed_recoveries = 0;
    
    bool enabled = true;
    bool in_recovery = false;
};

/**
 * @brief Health monitoring event types
 */
enum class MonitoringEvent {
    PLUGIN_REGISTERED,
    PLUGIN_UNREGISTERED,
    HEALTH_CHECK_STARTED,
    HEALTH_CHECK_COMPLETED,
    HEALTH_CHECK_FAILED,
    PLUGIN_UNHEALTHY_DETECTED,
    RECOVERY_STARTED,
    RECOVERY_SUCCESSFUL,
    RECOVERY_FAILED,
    PLUGIN_DISABLED,
    ADMIN_NOTIFIED
};

/**
 * @brief Monitoring event data
 */
struct MonitoringEventData {
    MonitoringEvent event;
    std::string plugin_name;
    std::chrono::system_clock::time_point timestamp;
    PluginHealthStatus health_status;
    std::string message;
    std::string details;
};

/**
 * @brief Callback for monitoring events
 */
using MonitoringEventCallback = std::function<void(const MonitoringEventData& event)>;

/**
 * @brief Plugin Health Monitor Service
 * 
 * This service continuously monitors all registered self-healing plugins:
 * - Performs periodic health checks
 * - Detects unhealthy plugins
 * - Triggers automatic recovery
 * - Notifies administrators on critical failures
 * - Maintains health history
 * 
 * Thread-Safety: All methods are thread-safe
 */
class PluginHealthMonitor {
private:
    std::unordered_map<std::string, MonitoredPlugin> monitored_plugins_;
    std::thread monitor_thread_;
    std::atomic<bool> running_{false};
    mutable std::mutex mutex_;
    
    HealthMonitorConfig config_;
    std::vector<MonitoringEventCallback> event_callbacks_;
    
    // Optional IMetrics sink for publishing health scores to Prometheus
    themis::core::concerns::IMetrics* metrics_sink_{nullptr};
    
    // Statistics
    std::atomic<uint64_t> total_health_checks_{0};
    std::atomic<uint64_t> total_recovery_attempts_{0};
    std::atomic<uint64_t> total_successful_recoveries_{0};
    std::atomic<uint64_t> total_failed_recoveries_{0};
    
public:
    /**
     * @brief Construct health monitor with configuration
     */
    explicit PluginHealthMonitor(const HealthMonitorConfig& config = HealthMonitorConfig());
    
    /**
     * @brief Destructor - stops monitoring
     */
    ~PluginHealthMonitor();
    
    // Prevent copying
    PluginHealthMonitor(const PluginHealthMonitor&) = delete;
    PluginHealthMonitor& operator=(const PluginHealthMonitor&) = delete;
    
    /**
     * @brief Start health monitoring
     * @return true if monitoring started successfully
     */
    bool startMonitoring();
    
    /**
     * @brief Stop health monitoring
     */
    void stopMonitoring();
    
    /**
     * @brief Check if monitoring is running
     */
    bool isRunning() const { return running_; }
    
    /**
     * @brief Register plugin for health monitoring
     * 
     * @param name Plugin name (must be unique)
     * @param plugin Pointer to self-healing plugin
     * @return true if registration successful
     * @note The plugin must remain valid for the lifetime of monitoring
     */
    bool registerPlugin(const std::string& name, ISelfHealingPlugin* plugin);
    
    /**
     * @brief Unregister plugin from monitoring
     * 
     * @param name Plugin name
     * @return true if plugin was found and unregistered
     */
    bool unregisterPlugin(const std::string& name);
    
    /**
     * @brief Perform immediate health check
     * 
     * @param name Plugin name
     * @return Diagnostic result or error if plugin not found
     */
    Result<PluginDiagnostics> checkPluginHealth(const std::string& name);
    
    /**
     * @brief Trigger immediate recovery for a plugin
     * 
     * @param name Plugin name
     * @return Recovery result
     */
    Result<RecoveryResult> triggerRecovery(const std::string& name);
    
    /**
     * @brief Get monitoring statistics for a plugin
     * 
     * @param name Plugin name
     * @return Monitored plugin information
     */
    Result<MonitoredPlugin> getPluginStats(const std::string& name) const;
    
    /**
     * @brief Get all monitored plugins
     * 
     * @return Map of plugin name to monitoring info
     */
    std::unordered_map<std::string, MonitoredPlugin> getAllPluginStats() const;
    
    /**
     * @brief Get global monitoring statistics
     */
    struct GlobalStats {
        uint64_t total_health_checks = 0;
        uint64_t total_recovery_attempts;
        uint64_t total_successful_recoveries;
        uint64_t total_failed_recoveries;
        size_t monitored_plugins_count;
        size_t healthy_plugins_count;
        size_t unhealthy_plugins_count;
        size_t recovering_plugins_count;
    };
    
    GlobalStats getGlobalStats() const;
    
    /**
     * @brief Register event callback
     * 
     * @param callback Function to be called on monitoring events
     */
    void registerEventCallback(MonitoringEventCallback callback);
    
    /**
     * @brief Clear all event callbacks
     */
    void clearEventCallbacks();
    
    /**
     * @brief Update monitoring configuration
     * 
     * @param config New configuration
     * @note Changes take effect on next monitoring cycle
     */
    void updateConfig(const HealthMonitorConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const HealthMonitorConfig& getConfig() const { return config_; }
    
    /**
     * @brief Enable/disable monitoring for specific plugin
     * 
     * @param name Plugin name
     * @param enabled true to enable, false to disable
     * @return true if plugin found and updated
     */
    bool setPluginMonitoringEnabled(const std::string& name, bool enabled);
    
    /**
     * @brief Attach a metrics sink for publishing per-plugin health scores.
     *
     * When set, the monitor emits a `plugin_health_score` gauge (0.0–1.0) to
     * @p sink after every health check cycle.  Pass nullptr to detach.
     * The caller must ensure the sink outlives the monitor.
     *
     * @param sink  IMetrics backend; e.g. a PrometheusMetricsAdapter instance.
     */
    void attachMetrics(themis::core::concerns::IMetrics* sink);
    
    /**
     * @brief Get singleton instance
     */
    static PluginHealthMonitor& instance();
    
private:
    /**
     * @brief Main monitoring loop (runs in separate thread)
     */
    void monitoringLoop();
    
    /**
     * @brief Check health of a single plugin
     */
    void checkPlugin(MonitoredPlugin& plugin);
    
    /**
     * @brief Handle unhealthy plugin
     */
    void handleUnhealthyPlugin(MonitoredPlugin& plugin);
    
    /**
     * @brief Attempt recovery with backoff
     */
    RecoveryResult attemptRecoveryWithBackoff(MonitoredPlugin& plugin);
    
    /**
     * @brief Calculate backoff duration
     */
    std::chrono::seconds calculateBackoff(uint32_t attempt_count) const;
    
    /**
     * @brief Notify administrators
     */
    void notifyAdministrators(
        const std::string& plugin_name,
        const PluginDiagnostics& diagnostics,
        const std::string& message
    );
    
    /**
     * @brief Emit monitoring event
     */
    void emitEvent(const MonitoringEventData& event);
    
    /**
     * @brief Disable plugin after max failures
     */
    void disablePlugin(MonitoredPlugin& plugin, const std::string& reason);

    /**
     * @brief Compute health score in [0.0, 1.0] from diagnostics.
     *
     * Formula:
     *   HEALTHY   → 1.0 − (error_rate × 0.2)
     *   DEGRADED  → 0.7 − (error_rate × 0.2)
     *   UNHEALTHY → 0.3 − (error_rate × 0.2)
     *   CRITICAL / RECOVERING → max(0.0, 0.1 − error_rate × 0.1)
     */
    static double computeHealthScore(const PluginDiagnostics& diag) noexcept;

    /**
     * @brief Publish plugin_health_score gauge to metrics_sink_ (if attached).
     *  Must be called with mutex_ held.
     */
    void publishHealthScore(const MonitoredPlugin& plugin) noexcept;
};

} // namespace plugins
} // namespace themis
