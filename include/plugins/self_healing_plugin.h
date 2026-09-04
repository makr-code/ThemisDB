/**
 * @file self_healing_plugin.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 97/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

namespace themis {
namespace plugins {

/**
 * @brief Health status of a plugin
 */
enum class PluginHealthStatus {
    HEALTHY,           ///< Plugin is functioning normally
    DEGRADED,          ///< Plugin has reduced functionality but is operational
    UNHEALTHY,         ///< Plugin has errors but may recover
    CRITICAL,          ///< Plugin requires immediate reload or replacement
    RECOVERING         ///< Plugin is attempting self-repair
};

/**
 * @brief Detailed diagnostic information about plugin health
 */
struct PluginDiagnostics {
    PluginHealthStatus status = PluginHealthStatus::HEALTHY;
    std::string error_message;
    std::string error_code;
    
    // Error metrics
    uint64_t error_count = 0;
    uint64_t recovery_attempts = 0;
    uint64_t successful_recoveries = 0;
    uint64_t failed_recoveries = 0;
    
    // Resource metrics
    size_t memory_usage_bytes = 0;
    double cpu_usage_percent = 0.0;
    uint64_t open_file_handles = 0;
    uint64_t active_connections = 0;
    
    // Recent errors (max 10)
    std::vector<std::string> recent_errors;
    std::chrono::system_clock::time_point last_error_time;
    
    // Recovery recommendation
    std::string suggested_recovery_action;
    std::vector<std::string> available_recovery_strategies;
    
    // Performance metrics
    double average_response_time_ms = 0.0;
    uint64_t total_requests = 0;
    uint64_t failed_requests = 0;
    
    /**
     * @brief Get error rate (0.0 to 1.0)
     */
    double getErrorRate() const {
        if (total_requests == 0) {
          return 0.0;
        }
        return static_cast<double>(failed_requests) / total_requests;
    }
    
    /**
     * @brief Check if plugin needs immediate attention
     */
    bool needsImmediateAttention() const {
        return status == PluginHealthStatus::CRITICAL || 
               status == PluginHealthStatus::RECOVERING ||
               getErrorRate() > 0.5;
    }
};

/**
 * @brief Recovery action types
 */
enum class RecoveryAction {
    NONE,                  ///< No recovery needed
    CLEANUP_RESOURCES,     ///< Free resources and clear caches
    RECONNECT,             ///< Reconnect to external services
    RELOAD_CONFIG,         ///< Reload configuration
    ROLLBACK_STATE,        ///< Rollback to last checkpoint
    RESTART_PLUGIN,        ///< Full plugin restart
    SWITCH_TO_BACKUP,      ///< Switch to backup/fallback plugin
    NOTIFY_ADMIN           ///< Escalate to administrator
};

/**
 * @brief Self-Healing Plugin Interface
 * 
 * Plugins can implement this interface to support autonomous error recovery.
 * The PluginHealthMonitor service will periodically call performHealthCheck()
 * and trigger attemptSelfRepair() when issues are detected.
 * 
 * Thread-Safety: All methods must be thread-safe
 */
class ISelfHealingPlugin {
public:
    virtual ~ISelfHealingPlugin() = default;
    
    /**
     * @brief Perform comprehensive health check
     * 
     * This method should check:
     * - Connection status (for network plugins)
     * - Resource usage (memory, file handles, etc.)
     * - Error rates
     * - Response times
     * 
     * @return Detailed diagnostic information
     * @note This method is called periodically by PluginHealthMonitor
     * @note Must be thread-safe
     */
    [[nodiscard]] virtual PluginDiagnostics performHealthCheck() = 0;
    
    /**
     * @brief Attempt automatic self-repair
     * 
     * This method is called when performHealthCheck() indicates the plugin
     * is unhealthy. The plugin should attempt to recover based on the
     * diagnostics information.
     * 
     * Common recovery strategies:
     * - Reconnect to external services
     * - Clear caches
     * - Reload configuration
     * - Rollback to last checkpoint
     * 
     * @param diagnostics Current diagnostic information
     * @return true if repair was successful, false otherwise
     * @note Must be thread-safe
     * @note Should complete within reasonable time (< 30 seconds)
     */
    [[nodiscard]] virtual bool attemptSelfRepair(const PluginDiagnostics& diagnostics) = 0;
    
    /**
     * @brief Clean up resources
     * 
     * Release resources that may be causing issues:
     * - Close unused connections
     * - Clear caches
     * - Free temporary memory
     * - Close file handles
     * 
     * @return true if cleanup was successful
     * @note Must be thread-safe
     */
    [[nodiscard]] virtual bool cleanupResources() = 0;
    
    /**
     * @brief Rollback to last known good state
     * 
     * Restore plugin to the state captured by the most recent saveCheckpoint().
     * This should restore:
     * - Configuration
     * - Connection parameters
     * - Internal state
     * 
     * @return true if rollback was successful
     * @note Must be thread-safe
     */
    [[nodiscard]] virtual bool rollbackToLastGoodState() = 0;
    
    /**
     * @brief Save current state as a checkpoint
     * 
     * Capture the current "known good" state for potential rollback.
     * This should save:
     * - Configuration
     * - Connection parameters
     * - Critical internal state
     * 
     * The plugin should maintain a limited number of checkpoints (e.g., 5)
     * to avoid excessive memory usage.
     * 
     * @note Call this after successful initialization and after major
     *       configuration changes
     * @note Must be thread-safe
     */
    virtual void saveCheckpoint() = 0;
    
    /**
     * @brief Get available recovery strategies
     * 
     * @return List of recovery actions this plugin supports
     * @note This is informational and helps the health monitor
     *       decide which recovery action to trigger
     */
    [[nodiscard]] virtual std::vector<RecoveryAction> getRecoveryStrategies() const = 0;
    
    /**
     * @brief Execute specific recovery action
     * 
     * @param action The recovery action to execute
     * @return true if action was successful
     * @note Must be thread-safe
     */
    virtual bool executeRecoveryAction(RecoveryAction action) {
        // Default implementation delegates to existing methods
        switch (action) {
            case RecoveryAction::CLEANUP_RESOURCES:
                return cleanupResources();
            case RecoveryAction::ROLLBACK_STATE:
                return rollbackToLastGoodState();
            default:
                return false;
        }
    }
    
    /**
     * @brief Get recovery configuration
     * 
     * @return JSON configuration for recovery behavior
     * 
     * Example configuration:
     * {
     *   "max_recovery_attempts": 3,
     *   "recovery_timeout_seconds": 30,
     *   "backoff_strategy": "exponential",
     *   "notify_admin_on_failure": true
     * }
     */
    virtual std::string getRecoveryConfiguration() const {
        return R"({
            "max_recovery_attempts": 3,
            "recovery_timeout_seconds": 30,
            "backoff_strategy": "exponential",
            "notify_admin_on_failure": true
        })";
    }
};

/**
 * @brief Recovery result information
 */
struct RecoveryResult {
    bool successful = false;
    RecoveryAction action_taken = RecoveryAction::NONE;
    std::string message;
    std::chrono::milliseconds duration{0};
    PluginHealthStatus status_after_recovery = PluginHealthStatus::UNHEALTHY;
};

/**
 * @brief Callback for recovery events
 */
using RecoveryEventCallback = std::function<void(
    const std::string& plugin_name,
    const RecoveryResult& result
)>;

} // namespace plugins
} // namespace themis
