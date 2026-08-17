/**
 * @file hnsw_layer_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// HNSW Layer Optimizer for Vector Index Optimization
// Implements predictive layer pruning and adaptive layer selection
// to reduce layer traversal from O(log²N) to ~O(log N)

#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <mutex>
#include <deque>
#include <chrono>

namespace themis {

/// Configuration for HNSW layer optimization
struct HnswOptimizationConfig {
    bool enabled = false;
    
    // Layer pruning configuration
    struct LayerPruning {
        bool enabled = false;
        double threshold_multiplier = 5.0;  // candidate_count > k * multiplier
        size_t min_samples = 5;              // Minimum samples for ef optimization
    } layer_pruning;
    
    // Adaptive layer selection configuration
    struct AdaptiveLayerSelection {
        bool enabled = false;
        size_t stats_window_size = 1000;  // Moving window for statistics
        size_t min_samples = 10;          // Minimum samples for statistical confidence
    } adaptive_layer_selection;
    
    // Batch insert optimization configuration
    struct BatchInsert {
        bool enabled = false;
        size_t batch_size = 100;
    } batch_insert;
};

/// HNSW Layer Optimizer
/// Implements layer pruning, adaptive layer selection, and batch insert optimization
/// for HNSW vector index to improve performance at scale (1B+ vectors)
///
/// Sources:
/// - HNSW Algorithm: Malkov, Y. A., & Yashunin, D. A. (2018).
///   "Efficient and robust approximate nearest neighbor search using Hierarchical Navigable Small World graphs"
///   IEEE Transactions on Pattern Analysis and Machine Intelligence
/// - Design: docs/performance/phase4_hnsw_layer_optimization.md
class HnswLayerOptimizer {
public:
    /// Statistics for a single layer
    struct LayerStats {
        int layer = 0;
        int64_t access_count = 0;
        int64_t candidates_found = 0;
        double total_search_time_ms = 0.0;
        double avg_search_time_ms = 0.0;
        double efficiency_score = 0.0;  // candidates_found / avg_search_time_ms
        
        void update(int64_t candidates, double search_time_ms) {
            access_count++;
            candidates_found += candidates;
            total_search_time_ms += search_time_ms;
            avg_search_time_ms = total_search_time_ms / access_count;
            efficiency_score = (avg_search_time_ms > 0) ? 
                (static_cast<double>(candidates_found) / avg_search_time_ms) : 0.0;
        }
    };
    
    /// Recent query statistics for adaptive optimization
    struct QueryStats {
        int entry_layer = 0;
        int ef_used = 0;
        int layers_traversed = 0;
        size_t k = 0;
        double total_time_ms = 0.0;
        std::chrono::steady_clock::time_point timestamp;
    };

    explicit HnswLayerOptimizer(const HnswOptimizationConfig& config);
    ~HnswLayerOptimizer() = default;
    
    // Disable copy, allow move
    HnswLayerOptimizer(const HnswLayerOptimizer&) = delete;
    HnswLayerOptimizer& operator=(const HnswLayerOptimizer&) = delete;
    HnswLayerOptimizer(HnswLayerOptimizer&&) noexcept = default;
    HnswLayerOptimizer& operator=(HnswLayerOptimizer&&) noexcept = default;
    
    /// Check if optimization is enabled
    bool isEnabled() const { return config_.enabled; }
    
    /// Record layer access for statistics
    /// @param layer Layer number (0 = bottom layer)
    /// @param candidates_found Number of candidates found in this layer
    /// @param search_time_ms Time spent searching this layer (milliseconds)
    void recordLayerAccess(int layer, int64_t candidates_found, double search_time_ms);
    
    /// Record query statistics for adaptive optimization
    /// @param entry_layer Entry layer used for this query
    /// @param ef_used EF parameter used for this query
    /// @param layers_traversed Number of layers actually traversed
    /// @param k Number of neighbors requested
    /// @param total_time_ms Total query time (milliseconds)
    void recordQueryStats(int entry_layer, int ef_used, int layers_traversed, 
                         size_t k, double total_time_ms);
    
    /// Get optimal entry layer based on statistics
    /// @return Recommended entry layer, or -1 to use default
    int getOptimalEntryLayer() const;
    
    /// Get optimal ef parameter for given k
    /// @param k Number of neighbors to search for
    /// @return Recommended ef parameter, or -1 to use default
    int getOptimalEf(size_t k) const;
    
    /// Check if layer should be pruned (skip deeper layers)
    /// @param current_layer Current layer being searched
    /// @param candidate_count Number of candidates found so far
    /// @param k Number of neighbors requested
    /// @return true if deeper layers can be skipped
    bool shouldPruneLayer(int current_layer, size_t candidate_count, size_t k) const;
    
    /// Get layer statistics for monitoring
    std::unordered_map<int, LayerStats> getLayerStats() const;
    
    /// Get recent query statistics
    std::vector<QueryStats> getRecentQueryStats() const;
    
    /// Reset statistics
    void resetStats();
    
    /// Get configuration
    const HnswOptimizationConfig& getConfig() const { return config_; }

private:
    HnswOptimizationConfig config_;
    
    // Layer statistics tracking
    mutable std::mutex stats_mutex_;
    std::unordered_map<int, LayerStats> layer_stats_;
    
    // Recent query statistics for adaptive optimization
    std::deque<QueryStats> recent_queries_;
    
    // Helper: Calculate efficiency score for adaptive selection
    double calculateAdaptiveScore_(int entry_layer, int ef) const;
};

} // namespace themis
