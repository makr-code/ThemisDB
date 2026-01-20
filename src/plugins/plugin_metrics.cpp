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
    
    // Calculate percentiles (requires sorted data)
    std::vector<double> sorted = latency_samples;
    std::sort(sorted.begin(), sorted.end());
    
    // P95: 95th percentile
    size_t p95_idx = static_cast<size_t>(sorted.size() * 0.95);
    if (p95_idx >= sorted.size()) p95_idx = sorted.size() - 1;
    p95_call_latency_ms = sorted[p95_idx];
    
    // P99: 99th percentile
    size_t p99_idx = static_cast<size_t>(sorted.size() * 0.99);
    if (p99_idx >= sorted.size()) p99_idx = sorted.size() - 1;
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
    
    // Add to samples (with circular buffer behavior)
    if (stats.latency_samples.size() >= PluginStats::MAX_SAMPLES) {
        // Remove oldest sample (FIFO)
        stats.latency_samples.erase(stats.latency_samples.begin());
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
    
    static PluginStats empty_stats;
    auto it = stats_.find(plugin);
    if (it != stats_.end()) {
        return it->second;
    }
    return empty_stats;
}

std::map<std::string, PluginMetrics::PluginStats> PluginMetrics::getAllStats() const {
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
    auto it = stats_.find(plugin);
    if (it == stats_.end()) {
        // Create new stats entry
        stats_[plugin] = PluginStats();
        it = stats_.find(plugin);
    }
    return it->second;
}

} // namespace plugins
} // namespace themis
