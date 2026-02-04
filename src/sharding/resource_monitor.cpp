// Copyright 2025 ThemisDB
// Licensed under MIT License

#include "sharding/resource_monitor.h"

namespace themisdb {
namespace sharding {

ResourceMonitor::ResourceMonitor() {}

void ResourceMonitor::registerAlertHandler(
    const std::string& metric,
    AlertCallback handler
) {
    std::lock_guard<std::mutex> lock(mutex_);
    alert_handlers_[metric] = handler;
}

void ResourceMonitor::updateMetric(
    const std::string& component,
    const std::string& metric,
    double value,
    double threshold,
    ResourceAlert::Severity severity
) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        metrics_[component][metric] = value;
    }
    
    // Update global stats
    if (metric == "memory_bytes") {
        total_memory_bytes_ = static_cast<size_t>(value);
        if (value > peak_memory_bytes_) {
            peak_memory_bytes_ = static_cast<size_t>(value);
        }
    } else if (metric == "connection_count") {
        total_connections_ = static_cast<size_t>(value);
    } else if (metric == "active_threads") {
        active_threads_ = static_cast<size_t>(value);
    } else if (metric == "pending_transactions") {
        pending_transactions_ = static_cast<size_t>(value);
    }
    
    // Check if alert should be triggered
    checkAndTriggerAlert(component, metric, value, threshold, severity);
}

void ResourceMonitor::monitor() {
    // Periodic monitoring logic can be implemented here
    // For now, alerts are triggered immediately in updateMetric
}

ResourceMonitor::GlobalStats ResourceMonitor::getGlobalStats() const {
    return GlobalStats{
        total_memory_bytes_,
        peak_memory_bytes_,
        total_connections_,
        active_threads_,
        pending_transactions_
    };
}

std::optional<double> ResourceMonitor::getMetric(
    const std::string& component,
    const std::string& metric
) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto comp_it = metrics_.find(component);
    if (comp_it == metrics_.end()) {
        return std::nullopt;
    }
    
    auto metric_it = comp_it->second.find(metric);
    if (metric_it == comp_it->second.end()) {
        return std::nullopt;
    }
    
    return metric_it->second;
}

void ResourceMonitor::checkAndTriggerAlert(
    const std::string& component,
    const std::string& metric,
    double value,
    double threshold,
    ResourceAlert::Severity severity
) {
    if (value <= threshold) {
        return;
    }
    
    // Find handler for this metric
    AlertCallback handler;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = alert_handlers_.find(metric);
        if (it != alert_handlers_.end()) {
            handler = it->second;
        }
    }
    
    // Trigger alert
    if (handler) {
        ResourceAlert alert;
        alert.component = component;
        alert.metric = metric;
        alert.current_value = value;
        alert.threshold = threshold;
        alert.severity = severity;
        alert.timestamp = std::chrono::system_clock::now();
        
        handler(alert);
    }
}

} // namespace sharding
} // namespace themisdb
