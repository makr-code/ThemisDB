/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            plugin_metrics.h                                   ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     159                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

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

} // namespace plugins
} // namespace themis
