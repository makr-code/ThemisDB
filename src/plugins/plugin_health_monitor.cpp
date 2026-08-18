/**
 * @file plugin_health_monitor.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/plugin_health_monitor.h"
#include <stdexcept>
#include "plugins/plugin_manager.h"
#include "utils/logger.h"
#include "utils/expected.h"
#include <algorithm>

namespace themis {
namespace plugins {

// ============================================================================
// Constructor & Destructor
// ============================================================================

PluginHealthMonitor::PluginHealthMonitor(const HealthMonitorConfig& config)
    : config_(config)
{
}

PluginHealthMonitor::~PluginHealthMonitor() {
    stopMonitoring();
}

// ============================================================================
// Monitoring lifecycle
// ============================================================================

bool PluginHealthMonitor::startMonitoring() {
    if (running_) {
        THEMIS_WARN("PluginHealthMonitor: already running");
        return false;
    }

    running_ = true;
    monitor_thread_ = std::thread([this]() { monitoringLoop(); });

    THEMIS_INFO("PluginHealthMonitor: started (check interval: {}s, max_recovery_attempts: {})",
                config_.check_interval.count(), config_.max_recovery_attempts);
    return true;
}

void PluginHealthMonitor::stopMonitoring() {
    if (!running_) {
        return;
    }

    running_ = false;

    if (monitor_thread_.joinable()) {
        monitor_thread_.join();
    }

    THEMIS_INFO("PluginHealthMonitor: stopped");
}

// ============================================================================
// Plugin registration
// ============================================================================

bool PluginHealthMonitor::registerPlugin(const std::string& name, ISelfHealingPlugin* plugin) {
    if (!plugin) {
        THEMIS_WARN("PluginHealthMonitor: attempted to register null plugin '{}'", name);
        return false;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    if (monitored_plugins_.count(name)) {
        THEMIS_WARN("PluginHealthMonitor: plugin '{}' already registered", name);
        return false;
    }

    MonitoredPlugin mp;
    mp.name = name;
    mp.plugin = plugin;
    mp.last_check = std::chrono::system_clock::now();
    mp.last_recovery_attempt = std::chrono::system_clock::time_point{};
    monitored_plugins_[name] = std::move(mp);

    emitEvent({
        MonitoringEvent::PLUGIN_REGISTERED, name,
        std::chrono::system_clock::now(),
        PluginHealthStatus::HEALTHY,
        "Plugin registered for health monitoring", ""
    });

    THEMIS_INFO("PluginHealthMonitor: registered plugin '{}'", name);
    return true;
}

bool PluginHealthMonitor::unregisterPlugin(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = monitored_plugins_.find(name);
    if (it == monitored_plugins_.end()) {
        return false;
    }

    monitored_plugins_.erase(it);

    emitEvent({
        MonitoringEvent::PLUGIN_UNREGISTERED, name,
        std::chrono::system_clock::now(),
        PluginHealthStatus::HEALTHY,
        "Plugin unregistered from health monitoring", ""
    });

    THEMIS_INFO("PluginHealthMonitor: unregistered plugin '{}'", name);
    return true;
}

// ============================================================================
// Manual health check & recovery
// ============================================================================

Result<PluginDiagnostics> PluginHealthMonitor::checkPluginHealth(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = monitored_plugins_.find(name);
    if (it == monitored_plugins_.end()) {
        return Err<PluginDiagnostics>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Plugin '" + name + "' not registered with health monitor");
    }

    auto& mp = it->second;
    if (!mp.plugin) {
        return Err<PluginDiagnostics>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Plugin '" + name + "' has a null pointer");
    }

    try {
        auto diag = mp.plugin->performHealthCheck();
        mp.last_diagnostics = diag;
        mp.last_check = std::chrono::system_clock::now();
        total_health_checks_++;
        return Ok(diag);
    } catch (const std::exception& e) {
        THEMIS_ERROR("PluginHealthMonitor: exception during health check for '{}': {}", name, e.what());
        PluginDiagnostics err_diag;
        err_diag.status = PluginHealthStatus::UNHEALTHY;
        err_diag.error_message = e.what();
        return Ok(err_diag);
    }
}

Result<RecoveryResult> PluginHealthMonitor::triggerRecovery(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = monitored_plugins_.find(name);
    if (it == monitored_plugins_.end()) {
        return Err<RecoveryResult>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Plugin '" + name + "' not registered with health monitor");
    }

    auto& mp = it->second;
    auto result = attemptRecoveryWithBackoff(mp);
    return Ok(result);
}

// ============================================================================
// Statistics
// ============================================================================

Result<MonitoredPlugin> PluginHealthMonitor::getPluginStats(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = monitored_plugins_.find(name);
    if (it == monitored_plugins_.end()) {
        return Err<MonitoredPlugin>(errors::ErrorCode::ERR_PLUGIN_NOT_FOUND,
            "Plugin '" + name + "' not registered with health monitor");
    }

    return Ok(it->second);
}

std::unordered_map<std::string, MonitoredPlugin> PluginHealthMonitor::getAllPluginStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return monitored_plugins_;
}

PluginHealthMonitor::GlobalStats PluginHealthMonitor::getGlobalStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    GlobalStats gs;
    gs.total_health_checks = total_health_checks_.load();
    gs.total_recovery_attempts = total_recovery_attempts_.load();
    gs.total_successful_recoveries = total_successful_recoveries_.load();
    gs.total_failed_recoveries = total_failed_recoveries_.load();
    gs.monitored_plugins_count = monitored_plugins_.size();

    gs.healthy_plugins_count = 0;
    gs.unhealthy_plugins_count = 0;
    gs.recovering_plugins_count = 0;

    for (const auto& [_, mp] : monitored_plugins_) {
        auto s = mp.last_diagnostics.status;
        if (s == PluginHealthStatus::HEALTHY || s == PluginHealthStatus::DEGRADED) {
            gs.healthy_plugins_count++;
        } else if (s == PluginHealthStatus::RECOVERING) {
            gs.recovering_plugins_count++;
        } else {
            gs.unhealthy_plugins_count++;
        }
    }

    return gs;
}

// ============================================================================
// Configuration & callbacks
// ============================================================================

void PluginHealthMonitor::registerEventCallback(MonitoringEventCallback callback) {
    std::lock_guard<std::mutex> lock(mutex_);
    event_callbacks_.push_back(std::move(callback));
}

void PluginHealthMonitor::clearEventCallbacks() {
    std::lock_guard<std::mutex> lock(mutex_);
    event_callbacks_.clear();
}

void PluginHealthMonitor::updateConfig(const HealthMonitorConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
}

bool PluginHealthMonitor::setPluginMonitoringEnabled(const std::string& name, bool enabled) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = monitored_plugins_.find(name);
    if (it == monitored_plugins_.end()) {
        return false;
    }

    it->second.enabled = enabled;
    THEMIS_INFO("PluginHealthMonitor: monitoring for '{}' {}",
                name, enabled ? "enabled" : "disabled");
    return true;
}

void PluginHealthMonitor::attachMetrics(themis::core::concerns::IMetrics* sink) {
    std::lock_guard<std::mutex> lock(mutex_);
    metrics_sink_ = sink;
    THEMIS_INFO("PluginHealthMonitor: metrics sink {}",
                sink ? "attached" : "detached");
}

// ============================================================================
// Singleton
// ============================================================================

PluginHealthMonitor& PluginHealthMonitor::instance() {
    static PluginHealthMonitor inst;
    // Configuration can be customized after retrieval via updateConfig().
    return inst;
}

// ============================================================================
// Private: monitoring loop
// ============================================================================

void PluginHealthMonitor::monitoringLoop() {
    while (running_) {
        // Sleep for check_interval in short increments so we can exit promptly
        auto deadline = std::chrono::steady_clock::now() + config_.check_interval;
        while (running_ && std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }

        if (!running_) break;

        // Take a snapshot of plugin names to avoid holding mutex during checks
        std::vector<std::string> names;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            names.reserve(monitored_plugins_.size());
            for (const auto& [n, _] : monitored_plugins_) {
                names.push_back(n);
            }
        }

        for (const auto& name : names) {
            if (!running_) break;

            std::lock_guard<std::mutex> lock(mutex_);
            auto it = monitored_plugins_.find(name);
            if (it == monitored_plugins_.end()) continue;

            auto& mp = it->second;
            if (!mp.enabled) continue;

            checkPlugin(mp);
        }
    }
}

// ============================================================================
// Private: check a single plugin
// ============================================================================

void PluginHealthMonitor::checkPlugin(MonitoredPlugin& plugin) {
    // mutex_ must be held by caller

    emitEvent({
        MonitoringEvent::HEALTH_CHECK_STARTED, plugin.name,
        std::chrono::system_clock::now(),
        plugin.last_diagnostics.status, "Health check started", ""
    });

    try {
        auto diag = plugin.plugin->performHealthCheck();
        plugin.last_diagnostics = diag;
        plugin.last_check = std::chrono::system_clock::now();
        total_health_checks_++;

        // Emit health score gauge to attached Prometheus sink (if any)
        publishHealthScore(plugin);

        emitEvent({
            MonitoringEvent::HEALTH_CHECK_COMPLETED, plugin.name,
            std::chrono::system_clock::now(),
            diag.status, "Health check completed", ""
        });

        if (diag.status != PluginHealthStatus::HEALTHY &&
            diag.status != PluginHealthStatus::DEGRADED) {
            handleUnhealthyPlugin(plugin);
        } else {
            // Reset consecutive failure count on healthy check
            plugin.consecutive_failures = 0;
        }

    } catch (const std::exception& e) {
        THEMIS_ERROR("PluginHealthMonitor: exception in health check for '{}': {}",
                     plugin.name, e.what());
        plugin.consecutive_failures++;
        // Only downgrade to UNHEALTHY if the status is not already more severe
        if (plugin.last_diagnostics.status == PluginHealthStatus::HEALTHY ||
            plugin.last_diagnostics.status == PluginHealthStatus::DEGRADED) {
            plugin.last_diagnostics.status = PluginHealthStatus::UNHEALTHY;
        }
        total_health_checks_++;

        // Emit degraded health score even when the check throws
        publishHealthScore(plugin);

        emitEvent({
            MonitoringEvent::HEALTH_CHECK_FAILED, plugin.name,
            std::chrono::system_clock::now(),
            PluginHealthStatus::UNHEALTHY,
            "Health check threw exception", e.what()
        });
    }
}

// ============================================================================
// Private: handle unhealthy plugin
// ============================================================================

void PluginHealthMonitor::handleUnhealthyPlugin(MonitoredPlugin& plugin) {
    // mutex_ must be held by caller

    plugin.consecutive_failures++;

    THEMIS_WARN("PluginHealthMonitor: plugin '{}' is unhealthy (status: {}, consecutive failures: {})",
                plugin.name,
                static_cast<int>(plugin.last_diagnostics.status),
                plugin.consecutive_failures);

    emitEvent({
        MonitoringEvent::PLUGIN_UNHEALTHY_DETECTED, plugin.name,
        std::chrono::system_clock::now(),
        plugin.last_diagnostics.status,
        "Plugin detected as unhealthy",
        plugin.last_diagnostics.error_message
    });

    // Respect max recovery attempts
    if (plugin.total_recovery_attempts >= config_.max_recovery_attempts) {
        if (config_.auto_disable_on_failure) {
            disablePlugin(plugin, "Exceeded max recovery attempts");
        } else {
            THEMIS_ERROR("PluginHealthMonitor: plugin '{}' exceeded max recovery attempts ({}); "
                         "auto-disable disabled by config",
                         plugin.name, config_.max_recovery_attempts);
            if (config_.notify_on_critical) {
                notifyAdministrators(plugin.name, plugin.last_diagnostics,
                    "Plugin exceeded max recovery attempts");
            }
        }
        return;
    }

    auto result = attemptRecoveryWithBackoff(plugin);

    if (result.successful) {
        THEMIS_INFO("PluginHealthMonitor: plugin '{}' recovered successfully", plugin.name);
    } else {
        THEMIS_WARN("PluginHealthMonitor: recovery failed for plugin '{}'", plugin.name);

        if (config_.notify_on_critical &&
            plugin.last_diagnostics.status == PluginHealthStatus::CRITICAL) {
            notifyAdministrators(plugin.name, plugin.last_diagnostics,
                "Plugin recovery failed: " + result.message);
        }
    }
}

// ============================================================================
// Private: recovery with backoff
// ============================================================================

RecoveryResult PluginHealthMonitor::attemptRecoveryWithBackoff(MonitoredPlugin& plugin) {
    // mutex_ must be held by caller

    RecoveryResult result;
    result.successful = false;
    result.action_taken = RecoveryAction::NONE;

    // Apply backoff delay if this isn't the first attempt
    if (plugin.total_recovery_attempts > 0) {
        auto backoff = calculateBackoff(plugin.total_recovery_attempts);
        auto since_last = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::system_clock::now() - plugin.last_recovery_attempt);

        if (since_last < backoff) {
            result.message = "Backoff: next attempt in " +
                std::to_string((backoff - since_last).count()) + "s";
            return result;
        }
    }

    plugin.in_recovery = true;
    plugin.last_recovery_attempt = std::chrono::system_clock::now();
    plugin.total_recovery_attempts++;
    total_recovery_attempts_++;

    emitEvent({
        MonitoringEvent::RECOVERY_STARTED, plugin.name,
        std::chrono::system_clock::now(),
        plugin.last_diagnostics.status,
        "Recovery attempt " + std::to_string(plugin.total_recovery_attempts) +
            " / " + std::to_string(config_.max_recovery_attempts),
        ""
    });

    auto start = std::chrono::steady_clock::now();

    try {
        // Choose the best available recovery strategy
        auto strategies = plugin.plugin->getRecoveryStrategies();

        // Preferred order: cleanup → reconnect → reload config → rollback → restart
        const std::vector<RecoveryAction> strategy_priority = {
            RecoveryAction::CLEANUP_RESOURCES,
            RecoveryAction::RECONNECT,
            RecoveryAction::RELOAD_CONFIG,
            RecoveryAction::ROLLBACK_STATE,
            RecoveryAction::RESTART_PLUGIN,
        };

        bool recovered = false;
        for (auto action : strategy_priority) {
            auto it = std::find(strategies.begin(), strategies.end(), action);
            if (it == strategies.end()) continue;

            recovered = plugin.plugin->executeRecoveryAction(action);
            result.action_taken = action;
            if (recovered) break;
        }

        // If no specific strategy worked, fall back to generic attemptSelfRepair.
        // action_taken is left as the last tried strategy (or NONE if none matched),
        // since attemptSelfRepair may internally combine multiple actions.
        if (!recovered) {
            recovered = plugin.plugin->attemptSelfRepair(plugin.last_diagnostics);
        }

        auto end = std::chrono::steady_clock::now();
        result.duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

        if (recovered) {
            result.successful = true;
            result.message = "Recovery succeeded";
            plugin.consecutive_failures = 0;
            plugin.successful_recoveries++;
            total_successful_recoveries_++;

            // Re-check health after recovery
            try {
                auto post_diag = plugin.plugin->performHealthCheck();
                plugin.last_diagnostics = post_diag;
                result.status_after_recovery = post_diag.status;
            } catch (...) {
                result.status_after_recovery = PluginHealthStatus::DEGRADED;
            }

            emitEvent({
                MonitoringEvent::RECOVERY_SUCCESSFUL, plugin.name,
                std::chrono::system_clock::now(),
                result.status_after_recovery, result.message, ""
            });
        } else {
            result.message = "All recovery strategies exhausted without success";
            result.status_after_recovery = plugin.last_diagnostics.status;
            plugin.failed_recoveries++;
            total_failed_recoveries_++;

            emitEvent({
                MonitoringEvent::RECOVERY_FAILED, plugin.name,
                std::chrono::system_clock::now(),
                plugin.last_diagnostics.status, result.message, ""
            });
        }

    } catch (const std::exception& e) {
        result.message = std::string("Recovery threw exception: ") + e.what();
        result.status_after_recovery = PluginHealthStatus::CRITICAL;
        plugin.failed_recoveries++;
        total_failed_recoveries_++;

        THEMIS_ERROR("PluginHealthMonitor: exception during recovery of '{}': {}",
                     plugin.name, e.what());

        emitEvent({
            MonitoringEvent::RECOVERY_FAILED, plugin.name,
            std::chrono::system_clock::now(),
            PluginHealthStatus::CRITICAL, result.message, e.what()
        });
    }

    plugin.in_recovery = false;
    return result;
}

// ============================================================================
// Private: helpers
// ============================================================================

std::chrono::seconds PluginHealthMonitor::calculateBackoff(uint32_t attempt_count) const {
    if (config_.backoff_strategy == "none") {
        return std::chrono::seconds{0};
    }

    if (config_.backoff_strategy == "linear") {
        auto duration = config_.initial_backoff * static_cast<int>(attempt_count);
        return std::min(duration, config_.max_backoff);
    }

    // Default: exponential backoff
    // backoff = initial * 2^(attempt-1), capped at max_backoff
    long long seconds = config_.initial_backoff.count();
    for (uint32_t i = 1; i < attempt_count; ++i) {
        seconds *= 2;
        if (seconds >= config_.max_backoff.count()) {
            return config_.max_backoff;
        }
    }
    return std::chrono::seconds{seconds};
}

void PluginHealthMonitor::notifyAdministrators(
    const std::string& plugin_name,
    const PluginDiagnostics& diagnostics,
    const std::string& message
) {
    // Emit a dedicated admin-notification event; concrete notification
    // (email, PagerDuty, webhook, etc.) is handled by registered callbacks.
    THEMIS_ERROR("PluginHealthMonitor [ADMIN ALERT]: plugin '{}' requires attention — {}",
                 plugin_name, message);

    emitEvent({
        MonitoringEvent::ADMIN_NOTIFIED, plugin_name,
        std::chrono::system_clock::now(),
        diagnostics.status, message,
        diagnostics.error_message
    });
}

void PluginHealthMonitor::emitEvent(const MonitoringEventData& event) {
    // mutex_ must be held by caller; copy callbacks to avoid deadlock if a
    // callback calls back into the monitor.
    auto callbacks_copy = event_callbacks_;

    for (const auto& cb : callbacks_copy) {
        try {
            cb(event);
        } catch (const std::exception& e) {
            THEMIS_WARN("PluginHealthMonitor: event callback threw: {}", e.what());
        }
    }
}

void PluginHealthMonitor::disablePlugin(MonitoredPlugin& plugin, const std::string& reason) {
    // mutex_ must be held by caller

    plugin.enabled = false;
    THEMIS_ERROR("PluginHealthMonitor: disabling plugin '{}' — {}", plugin.name, reason);

    emitEvent({
        MonitoringEvent::PLUGIN_DISABLED, plugin.name,
        std::chrono::system_clock::now(),
        plugin.last_diagnostics.status, reason, ""
    });

    if (config_.notify_on_critical) {
        notifyAdministrators(plugin.name, plugin.last_diagnostics,
            "Plugin disabled: " + reason);
    }
}

// ============================================================================
// Private: health score computation and Prometheus emission
// ============================================================================

double PluginHealthMonitor::computeHealthScore(const PluginDiagnostics& diag) noexcept {
    const double error_rate = diag.getErrorRate();
    switch (diag.status) {
        case PluginHealthStatus::HEALTHY:
            return std::max(0.0, 1.0 - error_rate * 0.2);
        case PluginHealthStatus::DEGRADED:
            return std::max(0.0, 0.7 - error_rate * 0.2);
        case PluginHealthStatus::UNHEALTHY:
            return std::max(0.0, 0.3 - error_rate * 0.2);
        case PluginHealthStatus::CRITICAL:
        case PluginHealthStatus::RECOVERING:
            return std::max(0.0, 0.1 - error_rate * 0.1);
        default:
            return 0.0;
    }
}

void PluginHealthMonitor::publishHealthScore(const MonitoredPlugin& plugin) noexcept {
    // mutex_ must be held by caller; metrics_sink_ may be null
    if (!metrics_sink_) return;
    try {
        const double score = computeHealthScore(plugin.last_diagnostics);
        metrics_sink_->setGauge("plugin_health_score", score, {{"plugin", plugin.name}});
    } catch (...) {
        // noexcept: swallow any exception from the metrics backend
    }
}

} // namespace plugins
} // namespace themis

