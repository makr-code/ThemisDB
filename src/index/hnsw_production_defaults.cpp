#include "index/hnsw_production_defaults.h"
#include <algorithm>
#include <cmath>
#include <spdlog/spdlog.h>

namespace themis {
namespace index {

// ============================================================================
// HnswProductionDefaults Implementation
// ============================================================================

HnswProductionDefaults::HnswParams HnswProductionDefaults::getRecommendedParams(
    size_t dataset_size,
    size_t dimension,
    PerformanceProfile profile) {
    
    HnswParams params;
    
    // Get M based on dataset size
    params.M = getRecommendedM(dataset_size);
    
    // Set max_M and max_M0 based on M
    params.max_M = params.M;
    params.max_M0 = params.M * 2;  // Layer 0 has more connections
    
    // Set ef_construction
    params.ef_construction = getRecommendedEfConstruction(params.M, dataset_size);
    
    // Set ef_search based on profile
    params.ef_search = getRecommendedEfSearch(10, profile);  // Default k=10
    
    // Set level multiplier (standard HNSW formula)
    params.ml = 1.0 / std::log(static_cast<double>(params.M));
    
    // Set initial capacity (reserve 20% growth headroom)
    params.initial_capacity = static_cast<size_t>(dataset_size * 1.2);
    
    // Enable optimizations based on dataset size and hardware
    params.use_prefetch = (dataset_size > MEDIUM_DATASET);
    params.numa_aware = (dataset_size > LARGE_DATASET);
    
    // Adjust for dimensionality
    if (dimension > 512) {
        // High-dimensional vectors: reduce M slightly to save memory
        params.M = std::max(8, params.M - 4);
        params.max_M = params.M;
        params.max_M0 = params.M * 2;
    }
    
    return params;
}

int HnswProductionDefaults::getRecommendedM(size_t dataset_size) {
    // Based on HNSW paper and production experience
    // Smaller M for small datasets (faster build, less memory)
    // Larger M for large datasets (better connectivity, higher recall)
    
    if (dataset_size < SMALL_DATASET) {
        return 8;
    } else if (dataset_size < MEDIUM_DATASET) {
        return 16;
    } else if (dataset_size < LARGE_DATASET) {
        return 24;
    } else if (dataset_size < XLARGE_DATASET) {
        return 32;
    } else {
        return 48;  // Very large datasets benefit from high connectivity
    }
}

int HnswProductionDefaults::getRecommendedEfConstruction(int M, size_t dataset_size) {
    // ef_construction should be significantly larger than M for good recall
    // Typical recommendation: 10-20x M
    // Scale up for larger datasets to maintain recall
    
    int base_ef = M * 12;
    
    if (dataset_size < SMALL_DATASET) {
        return base_ef;
    } else if (dataset_size < MEDIUM_DATASET) {
        return static_cast<int>(base_ef * 1.25);
    } else if (dataset_size < LARGE_DATASET) {
        return static_cast<int>(base_ef * 1.5);
    } else if (dataset_size < XLARGE_DATASET) {
        return base_ef * 2;
    } else {
        return static_cast<int>(base_ef * 2.5);
    }
}

int HnswProductionDefaults::getRecommendedEfSearch(
    size_t k,
    PerformanceProfile profile) {
    
    // ef_search must be >= k
    // Higher ef_search = better recall but slower queries
    
    int base_ef = static_cast<int>(k);
    
    // Apply profile multiplier
    double multiplier = BALANCED_EF_MULTIPLIER;
    switch (profile) {
        case PerformanceProfile::LATENCY_OPTIMIZED:
            multiplier = LATENCY_EF_MULTIPLIER;
            base_ef = std::max(static_cast<int>(k), static_cast<int>(k * 1.5));
            break;
        case PerformanceProfile::BALANCED:
            multiplier = BALANCED_EF_MULTIPLIER;
            base_ef = static_cast<int>(k * 2.0);
            break;
        case PerformanceProfile::RECALL_OPTIMIZED:
            multiplier = RECALL_EF_MULTIPLIER;
            base_ef = static_cast<int>(k * 3.0);
            break;
    }
    
    int ef = static_cast<int>(base_ef * multiplier);
    
    // Clamp to reasonable range
    return std::clamp(ef, static_cast<int>(k), 512);
}

HnswProductionDefaults::HnswParams HnswProductionDefaults::autoTuneParameters(
    size_t dataset_size,
    size_t dimension,
    size_t sample_size,
    double target_latency_ms,
    double target_recall) {
    
    // Start with balanced defaults
    HnswParams params = getRecommendedParams(
        dataset_size, dimension, PerformanceProfile::BALANCED);
    
    // Note: In a real implementation, this would:
    // 1. Build a small index with sample data
    // 2. Run sample queries with different ef_search values
    // 3. Measure latency and recall
    // 4. Binary search for optimal ef_search
    
    // For now, estimate based on targets
    if (target_latency_ms < 5.0) {
        // Very aggressive latency target
        params.ef_search = getRecommendedEfSearch(10, PerformanceProfile::LATENCY_OPTIMIZED);
    } else if (target_recall > 0.98) {
        // High recall target
        params.ef_search = getRecommendedEfSearch(10, PerformanceProfile::RECALL_OPTIMIZED);
    }
    
    spdlog::info("HNSW auto-tuned parameters: M={}, ef_construction={}, ef_search={}", 
                 params.M, params.ef_construction, params.ef_search);
    
    return params;
}

bool HnswProductionDefaults::validateParams(
    const HnswParams& params, 
    size_t dataset_size) {
    
    bool valid = true;
    
    // Check M is in reasonable range
    if (params.M < 4 || params.M > 64) {
        spdlog::warn("HNSW: M={} is outside recommended range [4, 64]", params.M);
        valid = false;
    }
    
    // Check ef_construction is large enough
    if (params.ef_construction < params.M * 8) {
        spdlog::warn("HNSW: ef_construction={} should be at least 8x M={}", 
                     params.ef_construction, params.M);
        valid = false;
    }
    
    // Check ef_search is reasonable
    if (params.ef_search < 10) {
        spdlog::warn("HNSW: ef_search={} is very low, may result in poor recall", 
                     params.ef_search);
    }
    
    // Warn about memory usage for large datasets with high M
    if (dataset_size > LARGE_DATASET && params.M > 32) {
        size_t estimated_mem = estimateMemoryUsage(params, dataset_size, 768);
        if (estimated_mem > 100ULL * 1024 * 1024 * 1024) {  // 100 GB
            spdlog::warn("HNSW: Estimated memory usage {} GB may be excessive", 
                        estimated_mem / (1024ULL * 1024 * 1024));
        }
    }
    
    return valid;
}

size_t HnswProductionDefaults::estimateMemoryUsage(
    const HnswParams& params,
    size_t dataset_size,
    size_t dimension) {
    
    // Memory per vector:
    // - Vector data: dimension * sizeof(float)
    // - Graph connections: ~M * 1.5 * sizeof(int) average per vector
    // - Metadata: ~100 bytes per vector
    
    size_t vector_size = dimension * sizeof(float);
    size_t graph_size = static_cast<size_t>(params.M * 1.5 * sizeof(int));
    size_t metadata_size = 100;
    
    size_t per_vector = vector_size + graph_size + metadata_size;
    size_t total = per_vector * dataset_size;
    
    // Add overhead (hash tables, bookkeeping, etc.)
    total = static_cast<size_t>(total * 1.2);
    
    return total;
}

double HnswProductionDefaults::estimateBuildTime(
    const HnswParams& params,
    size_t dataset_size,
    size_t dimension) {
    
    // Rough estimate based on:
    // - ~1000 vectors/second for 768-dim with M=16
    // - Scales roughly linearly with ef_construction
    // - Scales with dimension (more distance calculations)
    
    double base_rate = 1000.0;  // vectors per second
    
    // Adjust for M (affects graph construction cost)
    double m_factor = params.M / 16.0;
    
    // Adjust for ef_construction (affects search cost during build)
    double ef_factor = params.ef_construction / 200.0;
    
    // Adjust for dimension (affects distance calculation cost)
    double dim_factor = std::sqrt(dimension / 768.0);
    
    double adjusted_rate = base_rate / (m_factor * ef_factor * dim_factor);
    
    return dataset_size / adjusted_rate;
}

// ============================================================================
// HnswRuntimeAdapter Implementation
// ============================================================================

int HnswRuntimeAdapter::adjustEfSearch(
    int current_ef,
    double actual_latency_ms,
    double target_latency_ms,
    double actual_recall,
    double target_recall) {
    
    int new_ef = current_ef;
    
    // Priority 1: Meet recall target if we have recall data
    if (actual_recall >= 0.0) {
        if (actual_recall < target_recall - 0.01) {
            // Recall too low - increase ef_search
            new_ef = static_cast<int>(current_ef * 1.2);
        } else if (actual_recall > target_recall + 0.02) {
            // Recall higher than needed - can reduce ef_search
            new_ef = static_cast<int>(current_ef * 0.9);
        }
    }
    
    // Priority 2: Meet latency target
    if (actual_latency_ms > target_latency_ms * 1.2) {
        // Latency too high - decrease ef_search
        new_ef = std::min(new_ef, static_cast<int>(current_ef * 0.85));
    } else if (actual_latency_ms < target_latency_ms * 0.5) {
        // Latency very low - can increase ef_search for better recall
        new_ef = std::max(new_ef, static_cast<int>(current_ef * 1.1));
    }
    
    // Ensure ef_search stays in reasonable bounds
    new_ef = std::clamp(new_ef, 10, 512);
    
    if (new_ef != current_ef) {
        spdlog::debug("HNSW: Adapted ef_search {} -> {} (latency: {:.2f}ms, recall: {:.3f})",
                     current_ef, new_ef, actual_latency_ms, actual_recall);
    }
    
    return new_ef;
}

bool HnswRuntimeAdapter::shouldRebuildIndex(
    size_t current_size,
    size_t initial_size) {
    
    // Rebuild if index has grown significantly
    // HNSW performance degrades with many post-construction insertions
    
    constexpr size_t ACCEPTABLE_GROWTH_FACTOR = 2;
    constexpr size_t REBUILD_THRESHOLD_FACTOR = 5;
    
    if (current_size < initial_size * ACCEPTABLE_GROWTH_FACTOR) {
        return false;  // Less than 2x growth is acceptable
    }
    
    if (current_size > initial_size * REBUILD_THRESHOLD_FACTOR) {
        spdlog::info("HNSW: Index size {}x initial - rebuild recommended", 
                    current_size / std::max(initial_size, size_t(1)));
        return true;
    }
    
    return false;
}

double HnswRuntimeAdapter::getOverfetchMultiplier(
    double filter_selectivity,
    size_t k) {
    
    // Protect against division by zero
    if (filter_selectivity <= 0.0) {
        return 20.0;  // Maximum overfetch for invalid/zero selectivity
    }
    
    // When applying post-filters, need to fetch more candidates
    // to ensure we get k results after filtering
    
    if (filter_selectivity >= 0.9) {
        return 1.2;  // High selectivity - minimal overfetch
    } else if (filter_selectivity >= 0.5) {
        return 2.0;  // Medium selectivity
    } else if (filter_selectivity >= 0.1) {
        return 5.0;  // Low selectivity
    } else {
        // Very low selectivity - use heuristic based on k
        return std::min(20.0, std::max(10.0, k / filter_selectivity));
    }
}

} // namespace index
} // namespace themis
