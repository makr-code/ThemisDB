/**
 * @file plugin_metrics.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "plugins/plugin_metrics.h"
#include <algorithm>
#include <numeric>

namespace themis {
namespace plugins {

// ============================================================================
// PluginStats
// ============================================================================

void PluginMetrics::PluginStats::updatePercentiles() {
    if (latency_samples.empty()) {
        avg_call_latency_ms = 0.0;
        p95_call_latency_ms = 0.0;
        p99_call_latency_ms = 0.0;
        return;
    }
    
    // Calculate average
    double sum = std::accumulate(latency_samples.begin(), latency_samples.end(), 0.0);
    avg_call_latency_ms = sum / latency_samples.size();
    
    // Calculate percentiles (requires sorted copy of samples)
    std::vector<double> sorted(latency_samples.begin(), latency_samples.end());
    std::sort(sorted.begin(), sorted.end());
    
    // P95: 95th percentile
    size_t p95_idx = static_cast<size_t>(sorted.size() * 0.95);
    if (p95_idx >= static_cast<int>(sorted.size())) {
      p95_idx = sorted.size() - 1;
    }
    p95_call_latency_ms = sorted[p95_idx];
    
    // P99: 99th percentile
    size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
    if (p99_idx >= static_cast<int>(sorted.size())) {
      p99_idx = sorted.size() - 1;
    }
    p99_call_latency_ms = sorted[p99_idx];
}

// ============================================================================
// PluginMetrics
// ============================================================================

void PluginMetrics::recordLoad(const std::string& plugin, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stats = getOrCreateStats(plugin);
    stats.load_time = duration;
    stats.loaded_at = std::chrono::system_clock::now();
}

void PluginMetrics::recordReload(const std::string& plugin, std::chrono::milliseconds duration) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stats = getOrCreateStats(plugin);
    stats.last_reload_time = duration;
    stats.reload_count++;
    stats.loaded_at = std::chrono::system_clock::now(); // Update loaded timestamp
}

void PluginMetrics::recordCall(const std::string& plugin, std::chrono::microseconds latency) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stats = getOrCreateStats(plugin);
    stats.function_calls++;
    
    // Convert microseconds to milliseconds
    double latency_ms = latency.count() / 1000.0;
    
    // Add to sum for accurate reporting
    stats.sum_call_latency_ms += latency_ms;
    
    // Add to samples (FIFO circular buffer, O(1) eviction via deque::pop_front)
    if (stats.latency_samples.size() >= PluginStats::MAX_SAMPLES) {
        stats.latency_samples.pop_front();
    }
    stats.latency_samples.push_back(latency_ms);
    
    // Update percentiles
    stats.updatePercentiles();
}

void PluginMetrics::recordError(const std::string& plugin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stats = getOrCreateStats(plugin);
    stats.errors++;
}

void PluginMetrics::updateMemoryUsage(const std::string& plugin, size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto& stats = getOrCreateStats(plugin);
    stats.memory_bytes = bytes;
}

const PluginMetrics::PluginStats& PluginMetrics::getStats(const std::string& plugin) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = stats_.find(plugin);
    if (it != stats_.end()) {
        return it->second;
    }
    
    // Return thread-local empty stats to avoid data races
    thread_local static PluginStats empty_stats;
    return empty_stats;
}

std::unordered_map<std::string, PluginMetrics::PluginStats> PluginMetrics::getAllStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    return stats_;
}

void PluginMetrics::resetStats(const std::string& plugin) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = stats_.find(plugin);
    if (it != stats_.end()) {
        stats_.erase(it);
    }
}

void PluginMetrics::resetAll() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    stats_.clear();
}

PluginMetrics::PluginStats& PluginMetrics::getOrCreateStats(const std::string& plugin) {
    // Note: mutex_ must be held by caller
    auto result = stats_.emplace(plugin, PluginStats());
    return result.first->second;
}

// ============================================================================
// PluginMetricsCollector
// ============================================================================

void PluginMetricsCollector::collect(themis::core::concerns::IMetrics& sink) const {
    using Labels = themis::core::concerns::IMetrics::Labels;

    const auto all_stats = metrics_.getAllStats();

    for (const auto& [name, stats] : all_stats) {
        const Labels labels = {{"plugin", name}};

        // Call / error counters exposed as gauges (absolute snapshots)
        sink.setGauge("plugin_function_calls",
                      static_cast<double>(stats.function_calls), labels);
        sink.setGauge("plugin_errors",
                      static_cast<double>(stats.errors), labels);
        sink.setGauge("plugin_reload_count",
                      static_cast<double>(stats.reload_count), labels);

        // Timing gauges (milliseconds)
        sink.setGauge("plugin_load_time_ms",
                      static_cast<double>(stats.load_time.count()), labels);
        sink.setGauge("plugin_call_latency_avg_ms",
                      stats.avg_call_latency_ms, labels);
        sink.setGauge("plugin_call_latency_p95_ms",
                      stats.p95_call_latency_ms, labels);
        sink.setGauge("plugin_call_latency_p99_ms",
                      stats.p99_call_latency_ms, labels);

        // Resource gauge
        sink.setGauge("plugin_memory_bytes",
                      static_cast<double>(stats.memory_bytes), labels);
    }
}

} // namespace plugins
} // namespace themis
