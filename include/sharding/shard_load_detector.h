/**
 * @file shard_load_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <map>
#include <vector>
#include <deque>
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

/** @brief Latest observed load/resource metrics for one shard. */
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

/** @brief Trend-based load forecast for one shard and forecast horizon. */
struct LoadForecast {
    std::string shard_id;

    // Predicted load values (0–100 scale)
    double predicted_cpu_percent = 0.0;
    double predicted_storage_percent = 0.0;
    double predicted_composite_load = 0.0;  // Weighted composite score

    // Confidence interval (±)
    double confidence_interval = 0.0;

    // Forecast horizon used
    std::chrono::minutes horizon{5};

    // Whether the forecast is based on sufficient history (false = best-guess only)
    bool has_sufficient_history = false;
};

/** @brief Result payload emitted by one imbalance detection pass. */
struct LoadImbalanceResult {
    /** @brief True when at least one heuristic signaled imbalance. */
    bool is_imbalanced = false;
    /** @brief Human-readable concatenation of triggered heuristic reasons. */
    std::string reason;
    
    // Hotspots (overloaded shards)
    std::vector<std::string> hotspot_shards;
    
    // Cold shards (underutilized)
    std::vector<std::string> cold_shards;
    
    // Recommended rebalance operations
    /** @brief Suggested movement plan from one hot shard to one cold shard. */
    struct RebalanceRecommendation {
        std::string source_shard;      // Overloaded shard
        std::string target_shard;      // Underloaded shard
        /** @brief Inclusive token-range start suggested for migration. */
        uint64_t token_range_start;
        /** @brief Inclusive token-range end suggested for migration. */
        uint64_t token_range_end;
        /** @brief Estimated source-load reduction achieved by this move. */
        double expected_load_reduction_percent;
        /** @brief Text rationale describing why this movement was recommended. */
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
    /** @brief Runtime thresholds and cadence for imbalance detection logic. */
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
    
    /** @brief Construct detector with default detection configuration. */
    explicit ShardLoadDetector(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<PrometheusMetrics> metrics
    );
    
    /** @brief Construct detector with explicit configuration. */
    ShardLoadDetector(
        std::shared_ptr<ShardTopology> topology,
        std::shared_ptr<PrometheusMetrics> metrics,
        const Config& config
    );
    
    /**
     * @brief Update latest load snapshot for one shard and append history sample.
     * @param shard_id Stable shard identifier.
     * @param metrics Latest observed metrics for that shard.
     */
    void updateShardLoad(const std::string& shard_id, const ShardLoadMetrics& metrics);
    
    /**
     * @brief Run all configured imbalance heuristics over current shard snapshots.
     * @return Detection result with reason string, hotspots/cold-shards and recommendations.
     */
    LoadImbalanceResult detectImbalance() const;
    
    /**
     * @brief Get latest metrics for one shard.
     * @param shard_id Stable shard identifier.
     * @return Metrics snapshot, or std::nullopt if the shard has no tracked sample.
     */
    std::optional<ShardLoadMetrics> getShardLoad(const std::string& shard_id) const;
    
    /**
     * @brief Get latest metrics for all currently tracked shards.
     * @return Map shard_id -> metrics snapshot.
     */
    std::map<std::string, ShardLoadMetrics> getAllShardLoads() const;
    
    /**
     * @brief Record a rebalance trigger and start cooldown suppression window.
     */
    void recordRebalanceTriggered();
    
    /**
     * @brief Check whether rebalance cooldown is currently active.
     * @return true while cooldown window is active; false otherwise.
     */
    bool isInCooldown() const;
    
    /**
     * @brief Export detector counters and freshness diagnostics as JSON.
     * @return JSON object with detection counters, tracked shard count and staleness info.
     */
    nlohmann::json getStatistics() const;

    /**
     * @brief Forecast shard load at a future horizon via linear trend extrapolation.
     *
     * Requires at least Config::min_samples_per_shard history points for a
     * confidence-qualified forecast. With less history, the method returns a
     * best-effort forecast based on the latest sample and marks
     * LoadForecast::has_sufficient_history as false.
     *
     * @param shard_id Shard identifier to forecast.
     * @param horizon Future horizon to project (defaults to 5 minutes).
     * @return Forecast payload or std::nullopt when shard is unknown.
     */
    std::optional<LoadForecast> forecastLoad(
        const std::string& shard_id,
        std::chrono::minutes horizon = std::chrono::minutes{5}
    ) const;

    /** @brief Maximum retained history samples per shard for regression forecast. */
    static constexpr size_t kMaxHistorySamples = 60;
    
private:
    std::shared_ptr<ShardTopology> topology_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    Config config_;
    
    mutable std::mutex mutex_;
    
    // Load metrics per shard (latest snapshot)
    std::map<std::string, ShardLoadMetrics> shard_loads_;

    // Per-shard history for trend-based forecasting (ring-buffer semantics via deque)
    std::map<std::string, std::deque<ShardLoadMetrics>> shard_load_history_;
    
    // Cooldown tracking
    std::chrono::system_clock::time_point last_rebalance_time_;
    
    // Statistics
    mutable std::atomic<uint64_t> total_detections_{0};
    mutable std::atomic<uint64_t> imbalance_detections_{0};
    std::atomic<uint64_t> rebalance_triggers_{0};
    
    // Detection helpers
    /** @brief Detect shard byte-footprint skew beyond configured threshold. */
    bool detectStorageImbalance(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    /** @brief Detect request-rate skew beyond configured threshold. */
    bool detectRequestImbalance(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    /** @brief Detect p99 latency outliers relative to cluster average. */
    bool detectLatencyDegradation(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    /** @brief Detect shards above configured CPU/storage exhaustion thresholds. */
    bool detectResourceExhaustion(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    /** @brief Build rebalance recommendations from hotspot/cold-shard analysis. */
    void generateRebalanceRecommendations(
        const std::map<std::string, ShardLoadMetrics>& loads,
        LoadImbalanceResult& result
    ) const;
    
    /** @brief Collapse multi-signal shard metrics into one weighted load score. */
    double calculateLoad(const ShardLoadMetrics& metrics) const;
    /** @brief Compute standard deviation of value vector. */
    double calculateVariance(const std::vector<double>& values) const;

    /**
     * @brief Fit simple linear regression against sample series.
     * @return Pair (slope_per_sample, intercept).
     */
    static std::pair<double, double> linearRegression(const std::vector<double>& values);
};

} // namespace sharding
} // namespace themis
