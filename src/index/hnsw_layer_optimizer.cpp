/**
 * @file hnsw_layer_optimizer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// HNSW Layer Optimizer Implementation

#include "index/hnsw_layer_optimizer.h"
#include "utils/logger.h"
#include <algorithm>
#include <map>
#include <numeric>

namespace themis {

HnswLayerOptimizer::HnswLayerOptimizer(const HnswOptimizationConfig& config)
    : config_(config) {
    THEMIS_INFO("HnswLayerOptimizer initialized: enabled={}, layer_pruning={}, adaptive_selection={}",
                config_.enabled, config_.layer_pruning.enabled, 
                config_.adaptive_layer_selection.enabled);
}

void HnswLayerOptimizer::recordLayerAccess(int layer, int64_t candidates_found, double search_time_ms) {
    if (!config_.enabled) return;
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    auto& stats = layer_stats_[layer];
    stats.layer = layer;
    stats.update(candidates_found, search_time_ms);
    
    THEMIS_DEBUG("Layer {} stats: access_count={}, candidates={}, avg_time={:.3f}ms, efficiency={:.2f}",
                 layer, stats.access_count, candidates_found, stats.avg_search_time_ms, 
                 stats.efficiency_score);
}

void HnswLayerOptimizer::recordQueryStats(int entry_layer, int ef_used, int layers_traversed,
                                         size_t k, double total_time_ms) {
    if (!config_.enabled || !config_.adaptive_layer_selection.enabled) return;
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    QueryStats stats;
    stats.entry_layer = entry_layer;
    stats.ef_used = ef_used;
    stats.layers_traversed = layers_traversed;
    stats.k = k;
    stats.total_time_ms = total_time_ms;
    stats.timestamp = std::chrono::steady_clock::now();
    
    recent_queries_.push_back(stats);
    
    // Keep only the most recent queries within the window size
    while (recent_queries_.size() > config_.adaptive_layer_selection.stats_window_size) {
        recent_queries_.pop_front();
    }
    
    THEMIS_DEBUG("Query stats recorded: entry_layer={}, ef={}, layers={}, k={}, time={:.3f}ms",
                 entry_layer, ef_used, layers_traversed, k, total_time_ms);
}

int HnswLayerOptimizer::getOptimalEntryLayer() const {
    if (!config_.enabled || !config_.adaptive_layer_selection.enabled) return -1;
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (recent_queries_.empty() || layer_stats_.empty()) return -1;
    
    // Calculate average efficiency for each entry layer based on recent queries
    std::map<int, std::pair<double, int>> entry_layer_performance;  // layer -> (total_time, count)
    
    for (const auto& query : recent_queries_) {
        auto& perf = entry_layer_performance[query.entry_layer];
        perf.first += query.total_time_ms;
        perf.second++;
    }
    
    // Find entry layer with best average performance
    int best_layer = -1;
    double best_avg_time = std::numeric_limits<double>::max();
    
    for (const auto& [layer, perf] : entry_layer_performance) {
        double avg_time = perf.first / perf.second;
        if (avg_time < best_avg_time && static_cast<size_t>(perf.second) >= config_.adaptive_layer_selection.min_samples) {
            best_avg_time = avg_time;
            best_layer = layer;
        }
    }
    
    if (best_layer >= 0) {
        THEMIS_DEBUG("Optimal entry layer: {} (avg_time={:.3f}ms)", best_layer, best_avg_time);
    }
    
    return best_layer;
}

int HnswLayerOptimizer::getOptimalEf(size_t k) const {
    if (!config_.enabled || !config_.adaptive_layer_selection.enabled) return -1;
    
    std::lock_guard<std::mutex> lock(stats_mutex_);
    
    if (recent_queries_.empty()) return -1;
    
    // Calculate average performance for different ef values for similar k
    std::map<int, std::pair<double, int>> ef_performance;  // ef -> (total_time, count)
    
    for (const auto& query : recent_queries_) {
        // Only consider queries with similar k (within 50%)
        if (query.k >= k / 2 && query.k <= k * 2) {
            auto& perf = ef_performance[query.ef_used];
            perf.first += query.total_time_ms;
            perf.second++;
        }
    }
    
    // Find ef with best average performance
    int best_ef = -1;
    double best_avg_time = std::numeric_limits<double>::max();
    
    for (const auto& [ef, perf] : ef_performance) {
        if (static_cast<size_t>(perf.second) >= config_.adaptive_layer_selection.min_samples) {
            double avg_time = perf.first / perf.second;
            if (avg_time < best_avg_time) {
                best_avg_time = avg_time;
                best_ef = ef;
            }
        }
    }
    
    if (best_ef > 0) {
        THEMIS_DEBUG("Optimal ef for k={}: {} (avg_time={:.3f}ms)", k, best_ef, best_avg_time);
    }
    
    return best_ef;
}

bool HnswLayerOptimizer::shouldPruneLayer(int current_layer, size_t candidate_count, size_t k) const {
    if (!config_.enabled || !config_.layer_pruning.enabled) return false;
    
    // Prune if we have enough candidates (k * threshold_multiplier)
    size_t threshold = static_cast<size_t>(k * config_.layer_pruning.threshold_multiplier);
    bool should_prune = candidate_count > threshold;
    
    if (should_prune) {
        THEMIS_DEBUG("Layer pruning triggered at layer {}: candidates={}, threshold={}", 
                     current_layer, candidate_count, threshold);
    }
    
    return should_prune;
}

std::unordered_map<int, HnswLayerOptimizer::LayerStats> HnswLayerOptimizer::getLayerStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return layer_stats_;
}

std::vector<HnswLayerOptimizer::QueryStats> HnswLayerOptimizer::getRecentQueryStats() const {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    return std::vector<QueryStats>(recent_queries_.begin(), recent_queries_.end());
}

void HnswLayerOptimizer::resetStats() {
    std::lock_guard<std::mutex> lock(stats_mutex_);
    layer_stats_.clear();
    recent_queries_.clear();
    THEMIS_INFO("HnswLayerOptimizer statistics reset");
}

double HnswLayerOptimizer::calculateAdaptiveScore_(int entry_layer, int ef) const {
    // Calculate a score based on historical performance
    // Higher score = better performance
    
    if (recent_queries_.empty()) return 0.0;
    
    {
        double total_score = 0.0;
        int count = 0;
        
        for (const auto& query : recent_queries_) {
            if (query.entry_layer == entry_layer && query.ef_used == ef) {
                // Score inversely proportional to time, proportionally to efficiency
                double time_score = 1000.0 / (query.total_time_ms + 1.0);  // +1 to avoid division by zero
                double layer_score = 10.0 / (query.layers_traversed + 1.0);
                total_score += time_score + layer_score;
                count++;
            }
        }
        
        return count > 0 ? total_score / count : 0.0;
    }
}

} // namespace themis
