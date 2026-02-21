/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_health_monitor.h                            ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     346                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "plugins/self_healing_plugin.h"
#include "plugins/plugin_manager.h"
#include "utils/logger.h"
#include <string>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <chrono>
#include <functional>

/**
 * @file plugin_health_monitor.h
 * @brief Health monitoring service for self-healing plugins
 * 
 * The PluginHealthMonitor continuously monitors all registered self-healing
 * plugins and triggers automatic recovery when issues are detected.
 */

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
        uint64_t total_health_checks;
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
};

} // namespace plugins
} // namespace themis
