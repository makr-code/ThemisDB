/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_load_detector.h                              ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:38:10                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     258                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 4f8278e29  2025-11-30  Release 1.0.0: C++ header fixes, Docker build complete, P... ║
    • 35e35b6dc  2025-11-30  Sharding Phase 2-3: Auto-Rebalancing komplett (Load Detec... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_SHARD_LOAD_DETECTOR_H
#define THEMIS_SHARD_LOAD_DETECTOR_H

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <mutex>
#include <atomic>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declarations
class ShardTopology;
class PrometheusMetrics;

/**
 * Load metrics for a single shard
 */
struct ShardLoadMetrics {
    std::string shard_id;
    
    // Storage metrics
    uint64_t total_records = 0;
    uint64_t total_bytes = 0;
    double storage_usage_percent = 0.0;
    
    // Request metrics
    uint64_t requests_per_sec = 0;
    uint64_t read_requests_per_sec = 0;
    uint64_t write_requests_per_sec = 0;
    
    // Performance metrics
    double avg_latency_ms = 0.0;
    double p95_latency_ms = 0.0;
    double p99_latency_ms = 0.0;
    
    // Resource usage
    double cpu_usage_percent = 0.0;
    double memory_usage_mb = 0.0;
    
    // Token range coverage
    double token_range_coverage = 0.0;  // Percentage of total ring
    
    // Timestamp
    std::chrono::system_clock::time_point last_update;
};

/**
 * Load imbalance detection result
 */
struct LoadImbalanceResult {
    bool is_imbalanced = false;
    std::string reason;
    
    // Hotspots (overloaded shards)
    std::vector<std::string> hotspot_shards;
    
    // Cold shards (underutilized)
    std::vector<std::string> cold_shards;
    
    // Recommended rebalance operations
    struct RebalanceRecommendation {
        std::string source_shard;      // Overloaded shard
        std::string target_shard;      // Underloaded shard
        uint64_t token_range_start;
        uint64_t token_range_end;
        double expected_load_reduction_percent;
        std::string justification;
    };
    std::vector<RebalanceRecommendation> recommendations;
    
    // Overall cluster metrics
    double cluster_load_variance = 0.0;
    double cluster_avg_load = 0.0;
    double max_shard_load = 0.0;
    double min_shard_load = 0.0;
};

/**
 * Shard Load Detector
 * 
 * Monitors shard load metrics and detects imbalances that require rebalancing.
 * 
 * Detection criteria:
 * - Storage imbalance: >30% difference between shards
 * - Request imbalance: >50% difference in request rates
 * - Latency degradation: p99 > 2x cluster average
 * - Resource exhaustion: CPU >80% or storage >85%
 * 
 * Example:
 *   auto detector = std::make_unique<ShardLoadDetector>(topology, metrics);
 *   detector->updateShardLoad("shard001", load_metrics);
 *   
 *   auto imbalance = detector->detectImbalance();
 *   if (imbalance.is_imbalanced) {
 *       for (const auto& rec : imbalance.recommendations) {
 *           rebalancer->scheduleRebalance(rec);
 *       }
 *   }
 */
class ShardLoadDetector {
public:
    struct Config {
        // Detection thresholds
        double storage_imbalance_threshold = 0.30;      // 30% difference
        double request_imbalance_threshold = 0.50;      // 50% difference
        double latency_degradation_threshold = 2.0;     // 2x average
        double cpu_exhaustion_threshold = 0.80;         // 80% CPU
        double storage_exhaustion_threshold = 0.85;     // 85% storage
        
        // Minimum data for detection
        size_t min_shards_for_detection = 2;
        size_t min_samples_per_shard = 10;
        
        // Detection interval
        std::chrono::milliseconds detection_interval{std::chrono::minutes(5)};
        
        // Cooldown after rebalancing
        std::chrono::milliseconds rebalance_cooldown{std::chrono::hours(1)};
    };
    
    explicit ShardLoadDetector(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<PrometheusMetrics> metrics
    );
    
    ShardLoadDetector(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<PrometheusMetrics> metrics,
        const Config& config
    );
    
    /**
     * Update load metrics for a shard
     * @param shard_id Shard identifier
     * @param metrics Current load metrics
     */
    void updateShardLoad(const std::string& shard_id, const ShardLoadMetrics& metrics);
    
    /**
     * Detect load imbalance across cluster
     * @return Imbalance detection result with recommendations
     */
    LoadImbalanceResult detectImbalance() const;
    
    /**
     * Get load metrics for specific shard
     * @param shard_id Shard identifier
     * @return Load metrics (nullopt if shard not found)
     */
    std::optional<ShardLoadMetrics> getShardLoad(const std::string& shard_id) const;
    
    /**
     * Get load metrics for all shards
     * @return Map of shard_id -> load metrics
     */
    std::map<std::string, ShardLoadMetrics> getAllShardLoads() const;
    
    /**
     * Record that rebalancing was triggered
     * Starts cooldown period
     */
    void recordRebalanceTriggered();
    
    /**
     * Check if system is in cooldown period
     * @return true if cooldown active
     */
    bool isInCooldown() const;
    
    /**
     * Get detector statistics
     * @return JSON statistics (detections, triggers, etc.)
     */
    nlohmann::json getStatistics() const;
    
private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    Config config_;
    
    mutable std::mutex mutex_;
    
    // Load metrics per shard
    std::map<std::string, ShardLoadMetrics> shard_loads_;
    
    // Cooldown tracking
    std::chrono::system_clock::time_point last_rebalance_time_;
    
    // Statistics
    mutable std::atomic<uint64_t> total_detections_{0};
    mutable std::atomic<uint64_t> imbalance_detections_{0};
    std::atomic<uint64_t> rebalance_triggers_{0};
    
    // Detection helpers
    bool detectStorageImbalance(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    bool detectRequestImbalance(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    bool detectLatencyDegradation(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    bool detectResourceExhaustion(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    void generateRebalanceRecommendations(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    double calculateLoad(const ShardLoadMetrics& metrics) const;
    double calculateVariance(const std::vector<double>& values) const;
};

} // namespace sharding
} // namespace themis

#endif // THEMIS_SHARD_LOAD_DETECTOR_H
