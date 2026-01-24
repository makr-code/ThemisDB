#include "index/hnsw_parameter_tuner.h"
#include <algorithm>
#include <cmath>
#include <numeric>

#ifdef __SSE__
#include <xmmintrin.h>
#endif

namespace themis {
namespace index {

HnswParameterTuner::HnswParameterTuner(const Config& config)
    : config_(config)
    , current_ef_search_(config.ef_search_default)
{
    recent_queries_.reserve(config.stats_window_size);
}

int HnswParameterTuner::getOptimalEfSearch(size_t k, size_t dataset_size) const {
    if (!config_.adaptive) {
        return config_.ef_search_default;
    }
    
    int ef = current_ef_search_.load();
    
    // Scale with k (efSearch should be >= k)
    ef = std::max(ef, static_cast<int>(k));
    
    // Scale with dataset size if enabled
    if (config_.scale_with_dataset) {
        ef = calculateEfSearch(k, dataset_size);
    }
    
    // Clamp to configured range
    ef = std::clamp(ef, config_.ef_search_min, config_.ef_search_max);
    
    return ef;
}

void HnswParameterTuner::recordQueryResult(size_t k, int ef_used, double latency_ms, double recall) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    QueryStats stats;
    stats.k = k;
    stats.ef_used = ef_used;
    stats.latency_ms = latency_ms;
    stats.recall = recall;
    
    recent_queries_.push_back(stats);
    
    // Keep only recent queries
    if (recent_queries_.size() > config_.stats_window_size) {
        recent_queries_.erase(recent_queries_.begin());
    }
    
    queries_processed_.fetch_add(1);
    total_latency_.fetch_add(latency_ms);
    
    if (recall >= 0.0) {
        total_recall_.fetch_add(recall);
        recall_count_++;
    }
    
    // Adapt if we have enough samples
    if (recent_queries_.size() >= 100) {
        adapt();
    }
}

void HnswParameterTuner::updateConfig(const Config& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    config_ = config;
    current_ef_search_.store(config.ef_search_default);
}

HnswParameterTuner::Stats HnswParameterTuner::getStats() const {
    Stats stats;
    stats.queries_processed = queries_processed_.load();
    stats.current_ef_search = current_ef_search_.load();
    stats.adaptations_count = adaptations_count_.load();
    
    size_t count = queries_processed_.load();
    if (count > 0) {
        stats.avg_latency_ms = total_latency_.load() / count;
        
        if (recall_count_ > 0) {
            stats.avg_recall = total_recall_.load() / recall_count_;
        }
    }
    
    return stats;
}

void HnswParameterTuner::resetStats() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    recent_queries_.clear();
    queries_processed_.store(0);
    adaptations_count_.store(0);
    total_latency_.store(0.0);
    total_recall_.store(0.0);
    recall_count_ = 0;
}

int HnswParameterTuner::getRecommendedM(size_t dataset_size) {
    // Based on HNSW paper recommendations and empirical results
    
    if (dataset_size < 10000) {
        return 8;  // Small datasets: lower M for faster construction
    } else if (dataset_size < 100000) {
        return 16; // Medium datasets: balanced M
    } else if (dataset_size < 1000000) {
        return 24; // Large datasets: higher M for better connectivity
    } else {
        return 32; // Very large datasets: max M for optimal recall
    }
}

int HnswParameterTuner::getRecommendedEfConstruction(size_t dataset_size, int M) {
    // ef_construction should be roughly 10-20x M for good recall
    // Scale with dataset size
    
    int base_ef = M * 12;
    
    if (dataset_size < 10000) {
        return base_ef;
    } else if (dataset_size < 100000) {
        return static_cast<int>(base_ef * 1.5);
    } else if (dataset_size < 1000000) {
        return base_ef * 2;
    } else {
        return base_ef * 3;
    }
}

void HnswParameterTuner::adapt() {
    if (!config_.adaptive || recent_queries_.empty()) {
        return;
    }
    
    // Calculate average latency
    double avg_latency = 0.0;
    double avg_recall = 0.0;
    size_t recall_samples = 0;
    
    for (const auto& q : recent_queries_) {
        avg_latency += q.latency_ms;
        if (q.recall >= 0.0) {
            avg_recall += q.recall;
            recall_samples++;
        }
    }
    
    avg_latency /= recent_queries_.size();
    if (recall_samples > 0) {
        avg_recall /= recall_samples;
    }
    
    int current_ef = current_ef_search_.load();
    int new_ef = current_ef;
    
    // If latency is too high, decrease efSearch
    if (avg_latency > config_.target_latency.count()) {
        new_ef = std::max(config_.ef_search_min, 
                         static_cast<int>(current_ef * 0.9));
    }
    // If recall is too low (and we have recall data), increase efSearch
    else if (recall_samples > 0 && avg_recall < config_.target_recall) {
        new_ef = std::min(config_.ef_search_max, 
                         static_cast<int>(current_ef * 1.1));
    }
    // If both latency and recall are good, try to optimize (reduce efSearch slightly)
    else if (recall_samples > 0 && avg_recall > config_.target_recall + 0.02) {
        new_ef = std::max(config_.ef_search_min, 
                         static_cast<int>(current_ef * 0.95));
    }
    
    if (new_ef != current_ef) {
        current_ef_search_.store(new_ef);
        adaptations_count_.fetch_add(1);
    }
}

int HnswParameterTuner::calculateEfSearch(size_t k, size_t dataset_size) const {
    // Base efSearch calculation: scale with k and log(dataset_size)
    
    double base_ef = k * 2.0; // Start with 2x k
    
    // Scale with dataset size (logarithmically)
    if (dataset_size > 10000) {
        double scale_factor = 1.0 + std::log10(dataset_size / 10000.0);
        base_ef *= scale_factor;
    }
    
    // Clamp to configured range
    int ef = static_cast<int>(base_ef);
    return std::clamp(ef, config_.ef_search_min, config_.ef_search_max);
}

// HnswMemoryOptimizer implementation

void HnswMemoryOptimizer::prefetchNodes(const std::vector<size_t>& node_ids) {
#ifdef __SSE__
    // Use SIMD prefetch instructions if available
    for (size_t id : node_ids) {
        // Prefetch to L1 cache
        _mm_prefetch(reinterpret_cast<const char*>(&id), _MM_HINT_T0);
    }
#else
    // Fallback: compiler builtin
    #ifdef __GNUC__
    for (size_t id : node_ids) {
        __builtin_prefetch(&id, 0, 3);
    }
    #endif
#endif
}

size_t HnswMemoryOptimizer::getCacheLineSize() {
    // Most modern CPUs use 64-byte cache lines
    // TODO: Could use sysconf(_SC_LEVEL1_DCACHE_LINESIZE) on Linux
    return 64;
}

size_t HnswMemoryOptimizer::alignToCacheLine(size_t size) {
    size_t cache_line = getCacheLineSize();
    return ((size + cache_line - 1) / cache_line) * cache_line;
}

bool HnswMemoryOptimizer::hasSIMDPrefetch() {
#ifdef __SSE__
    return true;
#else
    return false;
#endif
}

} // namespace index
} // namespace themis
