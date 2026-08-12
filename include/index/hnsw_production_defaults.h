/**
 * @file hnsw_production_defaults.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>

namespace themis {
namespace index {

/**
 * @brief Production-ready HNSW default parameters
 * 
 * Provides optimized default parameters based on:
 * - Dataset size
 * - Vector dimensionality
 * - Target recall vs latency trade-off
 * - Hardware capabilities
 * - Workload type (OLTP, Analytics, Mixed, RAG)
 * 
 * Based on:
 * - HNSW paper (Malkov & Yashunin, 2018)
 * - ThemisDB benchmark results (BENCHMARK_RESULTS_COMPLETE_2025.md)
 * - Production deployments and A/B testing
 * - PERFORMANCE_TIPS.md workload optimization guidelines
 */
class HnswProductionDefaults {
public:
    enum class PerformanceProfile {
        LATENCY_OPTIMIZED,    // Min latency, ~95% recall
        BALANCED,             // Balanced latency/recall, ~97% recall
        RECALL_OPTIMIZED      // Max recall, ~99% recall, higher latency
    };
    
    enum class WorkloadType {
        OLTP,           // High-throughput, low-latency point queries
        ANALYTICS,      // Large batch queries, complex aggregations
        MIXED,          // Combination of OLTP and analytics
        RAG,            // Retrieval-Augmented Generation workloads
        BATCH_INSERT    // Bulk data loading optimization
    };
    
    struct HnswParams {
        int M;                    // Connections per node
        int ef_construction;      // Build-time search width
        int ef_search;            // Query-time search width
        
        // Advanced tuning
        int max_M;                // Max connections on layer 0
        int max_M0;               // Max connections on higher layers
        double ml;                // Level multiplier (default: 1/ln(M))
        
        // Memory and performance hints
        size_t initial_capacity;  // Pre-allocate for this many vectors
        bool use_prefetch;        // Enable SIMD prefetching
        bool numa_aware;          // Enable NUMA optimizations
    };
    
    /**
     * @brief Get recommended parameters for dataset and workload
     * @param dataset_size Expected number of vectors
     * @param dimension Vector dimensionality
     * @param profile Performance profile
     * @param workload Workload type for optimization
     * @return Optimized HNSW parameters
     */
    static HnswParams getRecommendedParams(
        size_t dataset_size,
        size_t dimension,
        PerformanceProfile profile = PerformanceProfile::BALANCED,
        WorkloadType workload = WorkloadType::MIXED);
    
    /**
     * @brief Get workload-specific optimized parameters
     * @param dataset_size Expected number of vectors
     * @param dimension Vector dimensionality
     * @param workload Workload type
     * @return Parameters optimized for the specific workload
     */
    static HnswParams getWorkloadOptimizedParams(
        size_t dataset_size,
        size_t dimension,
        WorkloadType workload);
    
    /**
     * @brief Get M parameter based on dataset size
     */
    static int getRecommendedM(size_t dataset_size);
    
    /**
     * @brief Get ef_construction based on M and dataset size
     */
    static int getRecommendedEfConstruction(int M, size_t dataset_size);
    
    /**
     * @brief Get ef_search based on k and performance profile
     */
    static int getRecommendedEfSearch(
        size_t k,
        PerformanceProfile profile = PerformanceProfile::BALANCED);
    
    /**
     * @brief Auto-tune parameters based on initial sample queries
     * @param sample_size Number of sample queries to run
     * @param target_latency_ms Target query latency in milliseconds
     * @param target_recall Target recall (0.0-1.0)
     * @return Tuned parameters
     */
    static HnswParams autoTuneParameters(
        size_t dataset_size,
        size_t dimension,
        size_t sample_size = 100,
        double target_latency_ms = 10.0,
        double target_recall = 0.95);
    
    /**
     * @brief Validate parameters and warn about suboptimal configurations
     */
    static bool validateParams(const HnswParams& params, size_t dataset_size);
    
    /**
     * @brief Get memory estimate for index
     * @return Estimated memory usage in bytes
     */
    static size_t estimateMemoryUsage(
        const HnswParams& params,
        size_t dataset_size,
        size_t dimension);
    
    /**
     * @brief Get build time estimate
     * @return Estimated build time in seconds
     */
    static double estimateBuildTime(
        const HnswParams& params,
        size_t dataset_size,
        size_t dimension);
    
private:
    // Performance profile multipliers
    static constexpr double LATENCY_EF_MULTIPLIER = 0.7;
    static constexpr double BALANCED_EF_MULTIPLIER = 1.0;
    static constexpr double RECALL_EF_MULTIPLIER = 1.5;
    
    // Dataset size thresholds
    static constexpr size_t SMALL_DATASET = 10000;
    static constexpr size_t MEDIUM_DATASET = 100000;
    static constexpr size_t LARGE_DATASET = 1000000;
    static constexpr size_t XLARGE_DATASET = 10000000;
};

/**
 * @brief Runtime HNSW parameter adapter
 * 
 * Adapts parameters during query execution based on actual performance.
 */
class HnswRuntimeAdapter {
public:
    /**
     * @brief Adjust ef_search based on actual vs expected performance
     * @param current_ef Current ef_search value
     * @param actual_latency_ms Measured query latency
     * @param target_latency_ms Target latency
     * @param actual_recall Measured recall (if available)
     * @param target_recall Target recall
     * @return Adjusted ef_search value
     */
    static int adjustEfSearch(
        int current_ef,
        double actual_latency_ms,
        double target_latency_ms,
        double actual_recall = -1.0,
        double target_recall = 0.95);
    
    /**
     * @brief Determine if index rebuild is recommended
     * @param current_size Current index size
     * @param initial_size Size when index was built
     * @return true if rebuild recommended
     */
    static bool shouldRebuildIndex(
        size_t current_size,
        size_t initial_size);
    
    /**
     * @brief Get overfetch multiplier for post-filtering
     * @param filter_selectivity Fraction of vectors passing filter (0.0-1.0)
     * @param k Requested number of results
     * @return Recommended k_initial = k * overfetch
     */
    static double getOverfetchMultiplier(
        double filter_selectivity,
        size_t k);
};

} // namespace index
} // namespace themis
