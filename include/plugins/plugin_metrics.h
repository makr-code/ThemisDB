/**
 * @file plugin_metrics.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "core/concerns/i_metrics.h"
#include <string>
#include <deque>
#include <map>
#include <vector>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <numeric>

namespace themis {
namespace plugins {

/**
 * @brief Plugin Metrics Collector
 * 
 * Tracks comprehensive metrics for each plugin:
 * - Timing: load time, reload time, call latency
 * - Counts: reload count, function calls, errors
 * - Resource: memory usage
 * - Performance: latency percentiles (P95, P99)
 * 
 * Thread-safe for concurrent access from multiple threads.
 */
class PluginMetrics {
public:
    /**
     * @brief Statistics for a single plugin
     */
    struct PluginStats {
        // Timing metrics
        std::chrono::milliseconds load_time{0};
        std::chrono::milliseconds last_reload_time{0};
        std::chrono::system_clock::time_point loaded_at;
        
        // Count metrics
        uint64_t reload_count = 0;
        uint64_t function_calls = 0;
        uint64_t errors = 0;
        
        // Resource usage
        size_t memory_bytes = 0;
        
        // Performance metrics (latency in milliseconds)
        double avg_call_latency_ms = 0.0;
        double p95_call_latency_ms = 0.0;
        double p99_call_latency_ms = 0.0;
        double sum_call_latency_ms = 0.0;  // Actual sum for accurate reporting
        
        // Internal: latency samples for percentile calculation.
        // Uses deque for O(1) front eviction instead of O(n) vector::erase.
        std::deque<double> latency_samples;
        static constexpr size_t MAX_SAMPLES = 1000;
        
        void updatePercentiles();
    };
    
    PluginMetrics() = default;
    ~PluginMetrics() = default;
    
    // Prevent copying
    PluginMetrics(const PluginMetrics&) = delete;
    PluginMetrics& operator=(const PluginMetrics&) = delete;
    
    /**
     * @brief Record plugin load time
     * @param plugin Plugin name
     * @param duration Load duration
     */
    void recordLoad(const std::string& plugin, std::chrono::milliseconds duration);
    
    /**
     * @brief Record plugin reload time
     * @param plugin Plugin name
     * @param duration Reload duration
     */
    void recordReload(const std::string& plugin, std::chrono::milliseconds duration);
    
    /**
     * @brief Record function call latency
     * @param plugin Plugin name
     * @param latency Call latency
     */
    void recordCall(const std::string& plugin, std::chrono::microseconds latency);
    
    /**
     * @brief Record plugin error
     * @param plugin Plugin name
     */
    void recordError(const std::string& plugin);
    
    /**
     * @brief Update memory usage for plugin
     * @param plugin Plugin name
     * @param bytes Memory usage in bytes
     */
    void updateMemoryUsage(const std::string& plugin, size_t bytes);
    
    /**
     * @brief Get statistics for a plugin
     * @param plugin Plugin name
     * @return Plugin statistics
     */
    const PluginStats& getStats(const std::string& plugin) const;
    
    /**
     * @brief Get all plugin statistics
     * @return Map of plugin name to statistics
     */
    std::map<std::string, PluginStats> getAllStats() const;
    
    /**
     * @brief Reset statistics for a plugin
     * @param plugin Plugin name
     */
    void resetStats(const std::string& plugin);
    
    /**
     * @brief Reset all statistics
     */
    void resetAll();
    
private:
    std::map<std::string, PluginStats> stats_;
    mutable std::mutex mutex_;
    
    // Helper to ensure plugin stats exist
    PluginStats& getOrCreateStats(const std::string& plugin);
};

/**
 * @brief Bridges PluginMetrics snapshots to an IMetrics sink (e.g. Prometheus).
 *
 * Call collect() periodically to push all per-plugin counters, latency gauges,
 * and resource metrics into the observability backend so they appear on the
 * Prometheus scrape endpoint and in the Grafana plugin dashboard.
 *
 * Metric names and labels emitted:
 *   plugin_function_calls      {plugin="<name>"}  gauge
 *   plugin_errors              {plugin="<name>"}  gauge
 *   plugin_reload_count        {plugin="<name>"}  gauge
 *   plugin_load_time_ms        {plugin="<name>"}  gauge
 *   plugin_memory_bytes        {plugin="<name>"}  gauge
 *   plugin_call_latency_avg_ms {plugin="<name>"}  gauge
 *   plugin_call_latency_p95_ms {plugin="<name>"}  gauge
 *   plugin_call_latency_p99_ms {plugin="<name>"}  gauge
 */
class PluginMetricsCollector {
public:
    /**
     * @brief Construct a collector bound to the given PluginMetrics store.
     * @param metrics  The PluginMetrics instance to read from.
     */
    explicit PluginMetricsCollector(const PluginMetrics& metrics) : metrics_(metrics) {}

    /**
     * @brief Publish a point-in-time snapshot of all per-plugin metrics to sink.
     *
     * Iterate all plugins tracked by PluginMetrics and emit each stat as a
     * labelled gauge into @p sink.  All values are absolute snapshots; the
     * caller is responsible for invoking this at a suitable scrape interval.
     *
     * @param sink  IMetrics backend to write into (e.g. PrometheusMetricsAdapter).
     */
    void collect(themis::core::concerns::IMetrics& sink) const;

private:
    const PluginMetrics& metrics_;
};

} // namespace plugins
} // namespace themis

