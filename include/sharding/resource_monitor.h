// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_RESOURCE_MONITOR_H
#define THEMISDB_SHARDING_RESOURCE_MONITOR_H

#include <string>
#include <map>
#include <functional>
#include <mutex>
#include <atomic>

namespace themisdb {
namespace sharding {

/**
 * @brief Resource alert information
 */
struct ResourceAlert {
    enum class Severity { WARNING, CRITICAL };
    
    std::string component;
    std::string metric;
    double current_value;
    double threshold;
    Severity severity;
    std::chrono::system_clock::time_point timestamp;
};

/**
 * @brief Resource Monitor
 * 
 * Monitors resource usage across all components and triggers alerts
 * when thresholds are exceeded.
 */
class ResourceMonitor {
public:
    using AlertCallback = std::function<void(const ResourceAlert&)>;
    
    struct GlobalStats {
        size_t total_memory_bytes;
        size_t peak_memory_bytes;
        size_t total_connections;
        size_t active_threads;
        size_t pending_transactions;
    };
    
    ResourceMonitor();
    
    /**
     * @brief Register alert handler for specific metric
     * @param metric Metric name (e.g., "memory_usage", "connection_count")
     * @param handler Callback to invoke when alert is triggered
     */
    void registerAlertHandler(const std::string& metric, AlertCallback handler);
    
    /**
     * @brief Update metric value
     * @param component Component name
     * @param metric Metric name
     * @param value Current value
     * @param threshold Alert threshold
     * @param severity Alert severity if threshold exceeded
     */
    void updateMetric(
        const std::string& component,
        const std::string& metric,
        double value,
        double threshold,
        ResourceAlert::Severity severity
    );
    
    /**
     * @brief Perform periodic monitoring
     */
    void monitor();
    
    /**
     * @brief Get global resource statistics
     * @return Current global stats
     */
    GlobalStats getGlobalStats() const;
    
    /**
     * @brief Get metric value
     * @param component Component name
     * @param metric Metric name
     * @return Current value if found
     */
    std::optional<double> getMetric(
        const std::string& component,
        const std::string& metric
    ) const;

private:
    mutable std::mutex mutex_;
    std::map<std::string, AlertCallback> alert_handlers_;
    std::map<std::string, std::map<std::string, double>> metrics_;
    
    std::atomic<size_t> total_memory_bytes_{0};
    std::atomic<size_t> peak_memory_bytes_{0};
    std::atomic<size_t> total_connections_{0};
    std::atomic<size_t> active_threads_{0};
    std::atomic<size_t> pending_transactions_{0};
    
    /**
     * @brief Trigger alert if threshold exceeded
     */
    void checkAndTriggerAlert(
        const std::string& component,
        const std::string& metric,
        double value,
        double threshold,
        ResourceAlert::Severity severity
    );
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_RESOURCE_MONITOR_H
