/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            shard_load_detector.cpp                            ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:43:09                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     447                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "sharding/shard_load_detector.h"
#include "sharding/shard_topology.h"
#include "sharding/prometheus_metrics.h"
#include "utils/logger.h"
#include <algorithm>
#include <cmath>
#include <numeric>

namespace themis {
namespace sharding {

ShardLoadDetector::ShardLoadDetector(
    std::shared_ptr<ShardTopology> topology,
    std::shared_ptr<PrometheusMetrics> metrics
) : ShardLoadDetector(topology, metrics, Config{}) {}

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

void ShardLoadDetector::updateShardLoad(const std::string& shard_id, const ShardLoadMetrics& load) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    shard_loads_[shard_id] = load;
    
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

LoadImbalanceResult ShardLoadDetector::detectImbalance() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    total_detections_++;
    
    LoadImbalanceResult result;
    
    // Check minimum requirements
    if (shard_loads_.size() < config_.min_shards_for_detection) {
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
    
    if (storage_values.empty()) return false;
    
    double max_storage = *std::max_element(storage_values.begin(), storage_values.end());
    double min_storage = *std::min_element(storage_values.begin(), storage_values.end());
    double avg_storage = std::accumulate(storage_values.begin(), storage_values.end(), 0.0) / storage_values.size();
    
    result.cluster_avg_load = avg_storage;
    result.max_shard_load = max_storage;
    result.min_shard_load = min_storage;
    result.cluster_load_variance = calculateVariance(storage_values);
    
    if (avg_storage == 0.0) return false;
    
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

bool ShardLoadDetector::detectRequestImbalance(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    std::vector<double> request_rates;
    std::vector<std::string> shard_ids;
    
    for (const auto& [shard_id, metrics] : loads) {
        request_rates.push_back(static_cast<double>(metrics.requests_per_sec));
        shard_ids.push_back(shard_id);
    }
    
    if (request_rates.empty()) return false;
    
    double max_rate = *std::max_element(request_rates.begin(), request_rates.end());
    double min_rate = *std::min_element(request_rates.begin(), request_rates.end());
    double avg_rate = std::accumulate(request_rates.begin(), request_rates.end(), 0.0) / request_rates.size();
    
    if (avg_rate == 0.0) return false;
    
    double imbalance_ratio = (max_rate - min_rate) / avg_rate;
    
    if (imbalance_ratio > config_.request_imbalance_threshold) {
        if (!result.reason.empty()) result.reason += "; ";
        result.reason += "Request imbalance (" + 
                        std::to_string(static_cast<int>(imbalance_ratio * 100)) + "% variance)";
        
        // Identify hotspots (above 150% of average)
        for (size_t i = 0; i < request_rates.size(); i++) {
            if (request_rates[i] > avg_rate * 1.5) {
                if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(), 
                             shard_ids[i]) == result.hotspot_shards.end()) {
                    result.hotspot_shards.push_back(shard_ids[i]);
                }
            }
        }
        
        return true;
    }
    
    return false;
}

bool ShardLoadDetector::detectLatencyDegradation(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    std::vector<double> latencies;
    std::vector<std::string> shard_ids;
    
    for (const auto& [shard_id, metrics] : loads) {
        latencies.push_back(metrics.p99_latency_ms);
        shard_ids.push_back(shard_id);
    }
    
    if (latencies.empty()) return false;
    
    double avg_latency = std::accumulate(latencies.begin(), latencies.end(), 0.0) / latencies.size();
    
    if (avg_latency == 0.0) return false;
    
    bool degradation_found = false;
    
    for (size_t i = 0; i < latencies.size(); i++) {
        if (latencies[i] > avg_latency * config_.latency_degradation_threshold) {
            if (!result.reason.empty()) result.reason += "; ";
            result.reason += "Latency degradation on " + shard_ids[i] + 
                           " (" + std::to_string(static_cast<int>(latencies[i])) + "ms p99)";
            
            if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(), 
                         shard_ids[i]) == result.hotspot_shards.end()) {
                result.hotspot_shards.push_back(shard_ids[i]);
            }
            
            degradation_found = true;
        }
    }
    
    return degradation_found;
}

bool ShardLoadDetector::detectResourceExhaustion(
    const std::map<std::string, ShardLoadMetrics>& loads,
    LoadImbalanceResult& result
) const {
    bool exhaustion_found = false;
    
    for (const auto& [shard_id, metrics] : loads) {
        if (metrics.cpu_usage_percent > config_.cpu_exhaustion_threshold * 100.0) {
            if (!result.reason.empty()) result.reason += "; ";
            result.reason += "CPU exhaustion on " + shard_id + 
                           " (" + std::to_string(static_cast<int>(metrics.cpu_usage_percent)) + "%)";
            
            if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(), 
                         shard_id) == result.hotspot_shards.end()) {
                result.hotspot_shards.push_back(shard_id);
            }
            
            exhaustion_found = true;
        }
        
        if (metrics.storage_usage_percent > config_.storage_exhaustion_threshold * 100.0) {
            if (!result.reason.empty()) result.reason += "; ";
            result.reason += "Storage exhaustion on " + shard_id + 
                           " (" + std::to_string(static_cast<int>(metrics.storage_usage_percent)) + "%)";
            
            if (std::find(result.hotspot_shards.begin(), result.hotspot_shards.end(), 
                         shard_id) == result.hotspot_shards.end()) {
                result.hotspot_shards.push_back(shard_id);
            }
            
            exhaustion_found = true;
        }
    }
    
    return exhaustion_found;
}

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
    
    for (size_t i = 0; i < num_recommendations && i < load_rankings.size() / 2; i++) {
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

double ShardLoadDetector::calculateVariance(const std::vector<double>& values) const {
    if (values.empty()) return 0.0;
    
    double mean = std::accumulate(values.begin(), values.end(), 0.0) / values.size();
    
    double variance = 0.0;
    for (double value : values) {
        double diff = value - mean;
        variance += diff * diff;
    }
    
    variance /= values.size();
    
    return std::sqrt(variance);
}

std::optional<ShardLoadMetrics> ShardLoadDetector::getShardLoad(const std::string& shard_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = shard_loads_.find(shard_id);
    if (it != shard_loads_.end()) {
        return it->second;
    }
    
    return std::nullopt;
}

std::map<std::string, ShardLoadMetrics> ShardLoadDetector::getAllShardLoads() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return shard_loads_;
}

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

bool ShardLoadDetector::isInCooldown() const {
    auto now = std::chrono::system_clock::now();
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        now - last_rebalance_time_
    );
    
    return elapsed < config_.rebalance_cooldown;
}

nlohmann::json ShardLoadDetector::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    nlohmann::json stats;
    stats["total_detections"] = total_detections_.load();
    stats["imbalance_detections"] = imbalance_detections_.load();
    stats["rebalance_triggers"] = rebalance_triggers_.load();
    stats["tracked_shards"] = shard_loads_.size();
    stats["in_cooldown"] = isInCooldown();
    
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

} // namespace sharding
} // namespace themis
