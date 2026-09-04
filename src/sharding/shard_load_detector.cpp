/**
 * @file shard_load_detector.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=23, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "sharding/shard_load_detector.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <sstream>
#include <unordered_set>

namespace {

/** @brief Append clause to semicolon-delimited imbalance reason string. */
void appendReasonClause(std::string& reason, const std::string& clause) {
    if (!reason.empty()) {
        reason.append("; ");
    }
    reason.append(clause);
}

/** @brief Insert hotspot shard only once while preserving insertion order. */
void addHotspotIfAbsent(std::vector<std::string>& hotspots,
                        std::unordered_set<std::string>& hotspot_index,
                        const std::string& shard_id) {
    if (hotspot_index.insert(shard_id).second) {
        hotspots.push_back(shard_id);
    }
}

} // namespace

namespace themis {
namespace sharding {

/**
 * @brief Construct load detector with default configuration.
 * @param topology Shard topology provider.
 * @param metrics Optional metrics sink.
 */
ShardLoadDetector::ShardLoadDetector(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<PrometheusMetrics> metrics
) : ShardLoadDetector(topology, metrics, Config{}) {}

/**
 * @brief Construct load detector with explicit thresholds and cadence.
 * @param topology Shard topology provider.
 * @param metrics Optional metrics sink.
 * @param config Detection configuration.
 */
ShardLoadDetector::ShardLoadDetector(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<PrometheusMetrics> metrics,
    const Config& config
) : topology_(topology),
    metrics_(metrics),
    config_(config),
    last_rebalance_time_(std::chrono::system_clock::time_point::min()) {
    
    THEMIS_INFO("ShardLoadDetector initialized with storage_threshold={}, request_threshold={}",
               config_.storage_imbalance_threshold, config_.request_imbalance_threshold);
}

/** @brief Update latest shard load snapshot and append history sample. */
void ShardLoadDetector::updateShardLoad(const std::string& shard_id, const ShardLoadMetrics& load) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    shard_loads_[shard_id] = load;

    // Append to per-shard history for forecasting; enforce ring-buffer size
    auto& history = shard_load_history_[shard_id];
    history.push_back(load);
    while (history.size() > kMaxHistorySamples) {
        history.pop_front();
    }
    
    // Update Prometheus metrics
    if (metrics_) {
        metrics_->setGauge("themis_shard_records_total", 
                          static_cast<double>(load.total_records), 
                          {{"shard_id", shard_id}});
        
        metrics_->setGauge("themis_shard_bytes_total", 
                          static_cast<double>(load.total_bytes), 
                          {{"shard_id", shard_id}});
        
        metrics_->setGauge("themis_shard_requests_per_sec", 
                          static_cast<double>(load.requests_per_sec), 
                          {{"shard_id", shard_id}});
        
        metrics_->setGauge("themis_shard_latency_p99_ms", 
                          load.p99_latency_ms, 
                          {{"shard_id", shard_id}});
        
        metrics_->setGauge("themis_shard_cpu_usage_percent", 
                          load.cpu_usage_percent, 
                          {{"shard_id", shard_id}});
        
        metrics_->setGauge("themis_shard_storage_usage_percent", 
                          load.storage_usage_percent, 
                          {{"shard_id", shard_id}});
    }
}

/** @brief Run configured imbalance heuristics and return combined detection result. */
LoadImbalanceResult ShardLoadDetector::detectImbalance() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_detections_++;
    
    LoadImbalanceResult result;
    
    // Check minimum requirements
    if (static_cast<int>(shard_loads_.size()) < config_.min_shards_for_detection) {
        result.reason = "Insufficient shards for detection (min: " + 
                       std::to_string(config_.min_shards_for_detection) + ")";
        return result;
    }
    
    // Check cooldown
    if (isInCooldown()) {
        result.reason = "System in cooldown period after recent rebalance";
        return result;
    }
    
    // Run all detection heuristics
    bool storage_imbalance = detectStorageImbalance(shard_loads_, result);
    bool request_imbalance = detectRequestImbalance(shard_loads_, result);
    bool latency_degradation = detectLatencyDegradation(shard_loads_, result);
    bool resource_exhaustion = detectResourceExhaustion(shard_loads_, result);
    
    result.is_imbalanced = storage_imbalance || request_imbalance || 
                           latency_degradation || resource_exhaustion;
    
    if (result.is_imbalanced) {
        imbalance_detections_++;
        generateRebalanceRecommendations(shard_loads_, result);
        
        THEMIS_WARN("Load imbalance detected: {} (hotspots: {}, cold: {})",
                   result.reason, result.hotspot_shards.size(), result.cold_shards.size());
        
        if (metrics_) {
            metrics_->incrementCounter("themis_load_imbalance_detections_total");
            metrics_->setGauge("themis_cluster_load_variance", result.cluster_load_variance);
        }
    }
    
    return result;
}

/** @brief Evaluate storage-byte skew across shards. */
bool ShardLoadDetector::detectStorageImbalance(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    std::vector<double> storage_values;
    std::vector<std::string> shard_ids;
    
    for (const auto& [shard_id, metrics] : loads) {
        double storage_load = static_cast<double>(metrics.total_bytes);
        storage_values.push_back(storage_load);
        shard_ids.push_back(shard_id);
    }
    
    if (storage_values.empty()) {
      return false;
    }
    
    double max_storage = *std::max_element(storage_values.begin(), storage_values.end());
    double min_storage = *std::min_element(storage_values.begin(), storage_values.end());
    double avg_storage = std::accumulate(storage_values.begin(), storage_values.end(), 0.0) / storage_values.size();
    
    result.cluster_avg_load = avg_storage;
    result.max_shard_load = max_storage;
    result.min_shard_load = min_storage;
    result.cluster_load_variance = calculateVariance(storage_values);
    
    if (avg_storage == 0.0) {
      return false;
    }
    
    double imbalance_ratio = (max_storage - min_storage) / avg_storage;
    
    if (imbalance_ratio > config_.storage_imbalance_threshold) {
        result.reason = "Storage imbalance detected (" + 
                       std::to_string(static_cast<int>(imbalance_ratio * 100)) + "% variance)";
        
        // Identify hotspots (above 120% of average)
        for (size_t i = 0; i < storage_values.size(); i++) {
            if (storage_values[i] > avg_storage * 1.2) {
                result.hotspot_shards.push_back(shard_ids[i]);
            }
            if (storage_values[i] < avg_storage * 0.8) {
                result.cold_shards.push_back(shard_ids[i]);
            }
        }
        
        return true;
    }
    
    return false;
}

/** @brief Evaluate requests/sec skew across shards. */
bool ShardLoadDetector::detectRequestImbalance(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    std::vector<double> request_rates;
    std::vector<std::string> shard_ids = {};

    request_rates.reserve(loads.size());
    shard_ids.reserve(loads.size());

    std::unordered_set<std::string> hotspot_index(
        result.hotspot_shards.begin(), result.hotspot_shards.end());
    
    for (const auto& [shard_id, metrics] : loads) {
        request_rates.push_back(static_cast<double>(metrics.requests_per_sec));
        shard_ids.push_back(shard_id);
    }
    
    if (request_rates.empty()) {
      return false;
    }
    
    double max_rate = *std::max_element(request_rates.begin(), request_rates.end());
    double min_rate = *std::min_element(request_rates.begin(), request_rates.end());
    double avg_rate = std::accumulate(request_rates.begin(), request_rates.end(), 0.0) / request_rates.size();
    
    if (avg_rate == 0.0) {
      return false;
    }
    
    double imbalance_ratio = (max_rate - min_rate) / avg_rate;
    
    if (imbalance_ratio > config_.request_imbalance_threshold) {
        std::ostringstream reason_stream = {};
        reason_stream << "Request imbalance ("
                      << static_cast<int>(imbalance_ratio * 100)
                      << "% variance)";
        appendReasonClause(result.reason, reason_stream.str());
        
        // Identify hotspots (above 150% of average)
        for (size_t i = 0; i < request_rates.size(); i++) {
            if (request_rates[i] > avg_rate * 1.5) {
                addHotspotIfAbsent(result.hotspot_shards, hotspot_index, shard_ids[i]);
            }
        }
        
        return true;
    }
    
    return false;
}

/** @brief Evaluate per-shard p99 latency outliers against cluster average. */
bool ShardLoadDetector::detectLatencyDegradation(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    std::vector<double> latencies;
    std::vector<std::string> shard_ids = {};

    latencies.reserve(loads.size());
    shard_ids.reserve(loads.size());

    std::unordered_set<std::string> hotspot_index(
        result.hotspot_shards.begin(), result.hotspot_shards.end());
    
    for (const auto& [shard_id, metrics] : loads) {
        latencies.push_back(metrics.p99_latency_ms);
        shard_ids.push_back(shard_id);
    }
    
    if (latencies.empty()) {
      return false;
    }
    
    double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    
    if (avg_latency == 0.0) {
      return false;
    }
    
    bool degradation_found = false;
    
    for (size_t i = 0; i < latencies.size(); i++) {
        if (latencies[i] > avg_latency * config_.latency_degradation_threshold) {
            std::ostringstream reason_stream = {};
            reason_stream << "Latency degradation on " << shard_ids[i]
                          << " (" << static_cast<int>(latencies[i]) << "ms p99)";
            appendReasonClause(result.reason, reason_stream.str());

            addHotspotIfAbsent(result.hotspot_shards, hotspot_index, shard_ids[i]);
            
            degradation_found = true;
        }
    }
    
    return degradation_found;
}

/** @brief Detect shards above CPU or storage exhaustion thresholds. */
bool ShardLoadDetector::detectResourceExhaustion(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    bool exhaustion_found = false;
    std::unordered_set<std::string> hotspot_index(
        result.hotspot_shards.begin(), result.hotspot_shards.end());
    
    for (const auto& [shard_id, metrics] : loads) {
        if (metrics.cpu_usage_percent > config_.cpu_exhaustion_threshold * 100.0) {
            std::ostringstream reason_stream = {};
            reason_stream << "CPU exhaustion on " << shard_id
                          << " (" << static_cast<int>(metrics.cpu_usage_percent) << "%)";
            appendReasonClause(result.reason, reason_stream.str());

            addHotspotIfAbsent(result.hotspot_shards, hotspot_index, shard_id);
            
            exhaustion_found = true;
        }
        
        if (metrics.storage_usage_percent > config_.storage_exhaustion_threshold * 100.0) {
            std::ostringstream reason_stream = {};
            reason_stream << "Storage exhaustion on " << shard_id
                          << " (" << static_cast<int>(metrics.storage_usage_percent) << "%)";
            appendReasonClause(result.reason, reason_stream.str());

            addHotspotIfAbsent(result.hotspot_shards, hotspot_index, shard_id);
            
            exhaustion_found = true;
        }
    }
    
    return exhaustion_found;
}

/** @brief Build simple hotspot-to-cold-shard migration recommendations. */
void ShardLoadDetector::generateRebalanceRecommendations(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    // Simple algorithm: pair each hotspot with coldest shard
    std::vector<std::pair<std::string, double>> load_rankings;
    
    for (const auto& [shard_id, metrics] : loads) {
        double load = calculateLoad(metrics);
        load_rankings.push_back({shard_id, load});
    }
    
    // Sort by load (descending)
    std::sort(load_rankings.begin(), load_rankings.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Generate recommendations: move data from hottest to coldest
    size_t num_recommendations = std::min(result.hotspot_shards.size(), result.cold_shards.size());
    
    for (size_t i = 0; i < num_recommendations  && static_cast<size_t>(i) < load_rankings.size() / 2; i++) {
        LoadImbalanceResult::RebalanceRecommendation rec;
        rec.source_shard = load_rankings[i].first;
        rec.target_shard = load_rankings[load_rankings.size() - 1 - i].first;
        
        // Calculate token range (simplified - move ~20% of source's range)
        // In reality, this would analyze actual token distribution
        rec.token_range_start = i * (UINT64_MAX / load_rankings.size());
        rec.token_range_end = rec.token_range_start + (UINT64_MAX / (load_rankings.size() * 5));
        
        double source_load = load_rankings[i].second;
        double target_load = load_rankings[load_rankings.size() - 1 - i].second;
        
        rec.expected_load_reduction_percent = ((source_load - target_load) / source_load) * 0.2 * 100.0;
        
        rec.justification = "Move data from overloaded shard (" + 
                           std::to_string(static_cast<int>(source_load)) + 
                           " load units) to underutilized shard (" + 
                           std::to_string(static_cast<int>(target_load)) + " load units)";
        
        result.recommendations.push_back(rec);
    }
    
    THEMIS_INFO("Generated {} rebalance recommendations", result.recommendations.size());
}

/** @brief Compute weighted composite shard load score. */
double ShardLoadDetector::calculateLoad(const ShardLoadMetrics& metrics) const {
    // Weighted load score
    double storage_weight = 0.4;
    double request_weight = 0.3;
    double latency_weight = 0.2;
    double cpu_weight = 0.1;
    
    // Normalize metrics (0-100 scale)
    double storage_score = std::min(metrics.storage_usage_percent, 100.0);
    double request_score = std::min(static_cast<double>(metrics.requests_per_sec) / 10.0, 100.0);
    double latency_score = std::min(metrics.p99_latency_ms / 10.0, 100.0);
    double cpu_score = metrics.cpu_usage_percent;
    
    return (storage_score * storage_weight) +
           (request_score * request_weight) +
           (latency_score * latency_weight) +
           (cpu_score * cpu_weight);
}

/** @brief Compute standard deviation across numeric vector. */
double ShardLoadDetector::calculateVariance(const std::vector<double>& values) const {
    if (values.empty()) {
      return 0.0;
    }
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    
    double variance = 0.0;
    for (double value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    
    variance /= values.size();
    
    return std::sqrt(variance);
}

/** @brief Return latest load metrics for shard id if present. */
std::optional<ShardLoadMetrics> ShardLoadDetector::getShardLoad(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_loads_.find(shard_id);
    if (it != shard_loads_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

/** @brief Return copy of latest load metrics map for all tracked shards. */
std::map<std::string, ShardLoadMetrics> ShardLoadDetector::getAllShardLoads() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_loads_;
}

/** @brief Record rebalance trigger and start cooldown interval. */
void ShardLoadDetector::recordRebalanceTriggered() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    last_rebalance_time_ = std::chrono::system_clock::now();
    rebalance_triggers_++;
    
    if (metrics_) {
        metrics_->incrementCounter("themis_rebalance_triggers_total");
    }
    
    THEMIS_INFO("Rebalance triggered, cooldown period started ({}s)", 
               config_.rebalance_cooldown.count() / 1000);
}

/** @brief Return whether detector currently suppresses actions during cooldown window. */
bool ShardLoadDetector::isInCooldown() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (last_rebalance_time_ == std::chrono::system_clock::time_point::min()) {
        return false;
    }

    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_rebalance_time_
    );
    
    return elapsed < config_.rebalance_cooldown;
}

/** @brief Return detector counters, tracking state and metric staleness diagnostics. */
nlohmann::json ShardLoadDetector::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Check cooldown without re-acquiring lock (we already hold it)
    bool in_cooldown = false;
    if (last_rebalance_time_ != std::chrono::system_clock::time_point::min()) {
        auto now = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_rebalance_time_
        );
        in_cooldown = elapsed < config_.rebalance_cooldown;
    }
    
    nlohmann::json stats;
    stats["total_detections"] = total_detections_.load();
    stats["imbalance_detections"] = imbalance_detections_.load();
    stats["rebalance_triggers"] = rebalance_triggers_.load();
    stats["tracked_shards"] = shard_loads_.size();
    stats["in_cooldown"] = in_cooldown;
    
    if (!shard_loads_.empty()) {
        auto now = std::chrono::system_clock::now();
        auto oldest_update = now;
        
        for (const auto& [shard_id, metrics] : shard_loads_) {
            if (metrics.last_update < oldest_update) {
                oldest_update = metrics.last_update;
            }
        }
        
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - oldest_update);
        stats["oldest_metric_age_sec"] = age.count();
    }
    
    return stats;
}

// ──────────────────────────────────────────────────────────────────────────────
// Load Forecasting
// ──────────────────────────────────────────────────────────────────────────────

/** @brief Fit linear trend model over value series (index-based x-axis). */
std::pair<double, double> ShardLoadDetector::linearRegression(const std::vector<double>& values) {
    if (static_cast<int>(values.size()) < 2) {
        return {0.0, values.empty() ? 0.0 : values[0]};
    }

    const double n = static_cast<double>(values.size());
    double sum_x = 0.0, sum_y = 0.0, sum_xy = 0.0, sum_xx = 0.0;

    for (size_t i = 0; i < values.size(); ++i) {
        const double x = static_cast<double>(i);
        sum_x  += x;
        sum_y  += values[i];
        sum_xy += x * values[i];
        sum_xx += x * x;
    }

    const double denom = n * sum_xx - sum_x * sum_x;
    if (std::abs(denom) < 1e-12) {
        return {0.0, sum_y / n};
    }

    const double slope     = (n * sum_xy - sum_x * sum_y) / denom;
    const double intercept = (sum_y - slope * sum_x) / n;
    return {slope, intercept};
}

/** @brief Forecast shard load at future horizon using linear-trend extrapolation. */
std::optional<LoadForecast> ShardLoadDetector::forecastLoad(
    const std::string& shard_id,
    std::chrono::minutes horizon
) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it_current = shard_loads_.find(shard_id);
    if (it_current == shard_loads_.end()) {
        return std::nullopt;
    }

    LoadForecast forecast;
    forecast.shard_id = shard_id;
    forecast.horizon  = horizon;

    auto it_hist = shard_load_history_.find(shard_id);
    const bool has_history = (it_hist != shard_load_history_.end()) &&
                             (it_hist->second.size() >= config_.min_samples_per_shard);
    forecast.has_sufficient_history = has_history;

    if (!has_history) {
        // Fall back to current snapshot (no extrapolation)
        const auto& current = it_current->second;
        forecast.predicted_cpu_percent      = current.cpu_usage_percent;
        forecast.predicted_storage_percent  = current.storage_usage_percent;
        forecast.predicted_composite_load   = calculateLoad(current);
        forecast.confidence_interval        = 0.0;
        return forecast;
    }

    const auto& history = it_hist->second;
    const size_t n = history.size();

    // Build per-metric time-series (sample index as proxy for time)
    std::vector<double> cpu_series, storage_series, composite_series;
    cpu_series.reserve(n);
    storage_series.reserve(n);
    composite_series.reserve(n);

    for (const auto& sample : history) {
        cpu_series.push_back(sample.cpu_usage_percent);
        storage_series.push_back(sample.storage_usage_percent);
        composite_series.push_back(calculateLoad(sample));
    }

    // Determine how many additional samples the horizon corresponds to.
    // We estimate the inter-sample interval from the history timestamps.
    double steps_ahead = 1.0;
    if (static_cast<int>(history.size()) > = 2) {
        const auto& first = history.front();
        const auto& last  = history.back();
        auto total_duration = std::chrono::duration_cast<std::chrono::milliseconds>(
            last.last_update - first.last_update
        ).count();
        if (total_duration > 0) {
            const double interval_ms = static_cast<double>(total_duration) / (n - 1);
            const double horizon_ms  =
                static_cast<double>(std::chrono::duration_cast<std::chrono::milliseconds>(horizon).count());
            steps_ahead = horizon_ms / interval_ms;
        }
    }

    // Apply linear regression and extrapolate
    auto [cpu_slope,     cpu_intercept]     = linearRegression(cpu_series);
    auto [storage_slope, storage_intercept] = linearRegression(storage_series);
    auto [comp_slope,    comp_intercept]    = linearRegression(composite_series);

    const double future_x = static_cast<double>(n - 1) + steps_ahead;

    forecast.predicted_cpu_percent     = std::clamp(cpu_slope * future_x + cpu_intercept, 0.0, 100.0);
    forecast.predicted_storage_percent = std::clamp(storage_slope * future_x + storage_intercept, 0.0, 100.0);
    forecast.predicted_composite_load  = std::clamp(comp_slope * future_x + comp_intercept, 0.0, 100.0);

    // Residual standard deviation as a proxy for confidence interval
    double residual_sum = 0.0;
    for (size_t i = 0; i < composite_series.size(); ++i) {
        const double predicted = comp_slope * static_cast<double>(i) + comp_intercept;
        const double diff = composite_series[i] - predicted;
        residual_sum += diff * diff;
    }
    forecast.confidence_interval = (n > 2)
        ? std::sqrt(residual_sum / static_cast<double>(n - 2))
        : 0.0;

    THEMIS_DEBUG("ShardLoadDetector::forecastLoad shard={} horizon={}min "
                 "predicted_cpu={:.1f}% predicted_storage={:.1f}% composite={:.1f}±{:.1f}",
                 shard_id, horizon.count(),
                 forecast.predicted_cpu_percent,
                 forecast.predicted_storage_percent,
                 forecast.predicted_composite_load,
                 forecast.confidence_interval);

    return forecast;
}

} // namespace sharding
} // namespace themis

