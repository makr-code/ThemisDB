/**
 * @file hnsw_parameter_tuner.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <mutex>
#include <vector>
#include <cstddef>
#include <chrono>
#include <cstdint>

namespace themis {
namespace index {

/**
 * @brief Adaptive HNSW Parameter Tuner
 * 
 * Dynamically adjusts HNSW search parameters based on:
 * - Query latency targets
 * - Recall requirements
 * - Dataset size
 * - Query patterns
 * - Workload type (OLTP, Analytics, Mixed, RAG)
 * 
 * Performance Gains:
 * - Optimal efSearch selection: +15-25% faster queries at same recall
 * - Reduced over-searching: -10-20% CPU usage
 * - Better latency/recall trade-offs
 * - Workload-specific optimization: +20-35% throughput improvement
 * 
 * Sources:
 * - Benchmark Analysis: benchmarks/BENCHMARK_ANALYSIS_20251210.md
 * - Research: docs/de/research/WISSENSCHAFTLICHE_PERFORMANCE_OPTIMIERUNGEN.md
 * - HNSW Paper: Malkov & Yashunin (2018)
 * - Performance Tips: docs/knowledge-base/PERFORMANCE_TIPS.md
 */
class HnswParameterTuner {
public:
    /**
     * @brief Workload types for index optimization
     */
    enum class WorkloadType {
        OLTP,           ///< High-throughput, low-latency, small k values
        ANALYTICS,      ///< Large k values, batch queries, higher latency tolerance
        MIXED,          ///< Balanced workload with varying query patterns
        RAG,            ///< Retrieval-Augmented Generation: medium k, high recall
        BATCH_INSERT    ///< Optimized for bulk indexing operations
    };
    
    struct Config {
        // Fixed parameters (set at index creation)
        int M = 16;                              ///< Connections per node (fixed)
        int ef_construction = 200;               ///< Construction-time search width (fixed)
        
        // Workload optimization
        WorkloadType workload = WorkloadType::MIXED; ///< Workload type for optimization
        
        // Runtime tunable parameters
        int ef_search_min = 32;                  ///< Minimum efSearch
        int ef_search_max = 512;                 ///< Maximum efSearch
        int ef_search_default = 64;              ///< Default efSearch
        
        // Adaptation settings
        bool adaptive = true;                    ///< Enable adaptive tuning
        double target_recall = 0.95;             ///< Target recall (0.0-1.0)
        std::chrono::milliseconds target_latency{10}; ///< Target query latency
        size_t stats_window_size = 1000;        ///< Statistics window for adaptation
        
        // Dataset-based scaling
        bool scale_with_dataset = true;          ///< Scale efSearch with dataset size
    };
    
    explicit HnswParameterTuner(const Config& config);
    ~HnswParameterTuner() = default;
    
    /**
     * @brief Get optimal efSearch for current conditions
     * @param k Number of neighbors requested
     * @param dataset_size Current dataset size (number of vectors)
     * @return Recommended efSearch value
     */
    int getOptimalEfSearch(size_t k, size_t dataset_size) const;
    
    /**
     * @brief Record query result for adaptation
     * @param k Number of neighbors requested
     * @param ef_used efSearch value used
     * @param latency_ms Query latency in milliseconds
     * @param recall Achieved recall (if known, optional)
     */
    void recordQueryResult(size_t k, int ef_used, double latency_ms, double recall = -1.0);
    
    /**
     * @brief Get current configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Update configuration
     */
    void updateConfig(const Config& config);
    
    /**
     * @brief Get performance statistics
     */
    struct Stats {
        size_t queries_processed = 0;
        double avg_latency_ms = 0.0;
        double avg_recall = 0.0;
        int current_ef_search = 0;
        size_t adaptations_count = 0;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Get recommended M parameter for dataset size and workload
     * @param dataset_size Expected dataset size
     * @param workload Workload type for optimization
     * @return Recommended M value (for index creation)
     */
    static int getRecommendedM(size_t dataset_size, WorkloadType workload = WorkloadType::MIXED);
    
    /**
     * @brief Get recommended ef_construction for dataset size, M, and workload
     * @param dataset_size Expected dataset size
     * @param M Connections per node
     * @param workload Workload type for optimization
     * @return Recommended ef_construction value (for index creation)
     */
    static int getRecommendedEfConstruction(size_t dataset_size, int M, WorkloadType workload = WorkloadType::MIXED);
    
    /**
     * @brief Get workload-optimized configuration preset
     * @param dataset_size Expected dataset size
     * @param workload Workload type
     * @return Optimized configuration for the workload
     */
    static Config getWorkloadOptimizedConfig(size_t dataset_size, WorkloadType workload);

    /**
     * @brief Recommended construction-time parameters returned by the auto-tuner
     */
    struct ConstructionParams {
        int M = 16;                              ///< Recommended connections per node
        int ef_construction = 200;               ///< Recommended construction-time search width
        WorkloadType detected_workload = WorkloadType::MIXED; ///< Detected or inferred workload
    };

    /**
     * @brief Get auto-tuned construction parameters based on workload detection
     *
     * Uses the recently recorded query statistics to infer the workload type and
     * returns the recommended M and ef_construction values for index creation.
     *
     * @param dataset_size Expected number of vectors in the index
     * @param workload_hint If not MIXED, used directly; otherwise workload is auto-detected
     * @return Recommended M and ef_construction together with the detected workload
     */
    ConstructionParams getAutoTunedConstructionParams(
        size_t dataset_size,
        WorkloadType workload_hint = WorkloadType::MIXED) const;

private:
    /**
     * @brief Adapt efSearch based on recent queries
     */
    void adapt();
    
    /**
     * @brief Calculate efSearch based on k and dataset size
     */
    int calculateEfSearch(size_t k, size_t dataset_size) const;
    
    struct QueryStats {
        double latency_ms = 0.0;
        double recall = 0.0;
        int ef_used = 0;
        size_t k = 0;
    };
    
    Config config_;
    mutable std::mutex mutex_;
    
    // Current adapted efSearch
    std::atomic<int> current_ef_search_;
    
    // Statistics (using mutex for double atomicity in C++17)
    std::vector<QueryStats> recent_queries_;
    std::atomic<size_t> queries_processed_{0};
    std::atomic<size_t> adaptations_count_{0};
    double total_latency_{0.0};  // Protected by mutex_
    double total_recall_{0.0};   // Protected by mutex_
    size_t recall_count_{0};     // Protected by mutex_
};

/**
 * @brief Automatic workload classifier for HNSW construction-parameter auto-tuning
 *
 * Observes real-time insert and query events and derives the dominant workload
 * type.  The classification is then fed into HnswParameterTuner so that the
 * best M and ef_construction values can be selected without manual input.
 *
 * Classification rules (evaluated over the observation window):
 *   - BATCH_INSERT  : insert_rate >> query_rate (ratio >= 10:1) AND avg_batch_size >= 100
 *   - OLTP          : insert_rate > query_rate OR avg_k <= 5
 *   - ANALYTICS     : avg_k >= 20 OR query_rate is low but k is large
 *   - RAG           : avg_k in [6, 19] AND query_rate is moderate
 *   - MIXED         : no dominant pattern
 */
class WorkloadClassifier {
public:
    /**
     * @brief Record an insert (or batch-insert) event
     * @param batch_size Number of vectors inserted in this call (defaults to 1)
     */
    void recordInsert(size_t batch_size = 1);

    /**
     * @brief Record a query event
     * @param k Number of nearest neighbors requested
     */
    void recordQuery(size_t k);

    /**
     * @brief Detect the dominant workload type from recorded events
     * @return Detected WorkloadType
     */
    HnswParameterTuner::WorkloadType detectWorkload() const;

    /**
     * @brief Reset all recorded statistics
     */
    void reset();

    /// Statistics snapshot for inspection / testing
    struct Stats {
        uint64_t total_inserts  = 0;
        uint64_t total_queries  = 0;
        double   avg_batch_size = 0.0;
        double   avg_k          = 0.0;
    };

    Stats getStats() const;

private:
    mutable std::mutex mutex_;

    uint64_t total_inserts_  = 0;   ///< Total vectors inserted
    uint64_t insert_events_  = 0;   ///< Number of insert calls
    uint64_t total_k_        = 0;   ///< Sum of k across all query events
    uint64_t query_events_   = 0;   ///< Number of query events
};

/**
 * @brief HNSW Memory Optimizer
 * 
 * Optimizes memory layout and access patterns for HNSW index:
 * - Cache-line alignment for frequently accessed data
 * - Prefetching hints for graph traversal
 * - Memory pool for node allocation
 */
class HnswMemoryOptimizer {
public:
    /**
     * @brief Prefetch memory for HNSW graph traversal
     * @param node_ids Node IDs to prefetch
     */
    static void prefetchNodes([[maybe_unused]] const std::vector<size_t>& node_ids);
    
    /**
     * @brief Get optimal cache-line size for platform
     */
    static size_t getCacheLineSize();
    
    /**
     * @brief Align size to cache line boundary
     */
    static size_t alignToCacheLine(size_t size);
    
    /**
     * @brief Check if SIMD prefetching is available
     */
    static bool hasSIMDPrefetch();
};

} // namespace index
} // namespace themis

