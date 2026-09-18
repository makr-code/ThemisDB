/**
 * @file advanced_vector_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstdint>
#include <functional>
#include <mutex>

namespace themis {

class AdvancedVectorIndex {
public:
    enum class WorkloadType {
        OLTP,           ///< High-throughput, low-latency queries
        ANALYTICS,      ///< Large batch queries, maximize recall
        MIXED,          ///< Balanced workload
        RAG,            ///< Retrieval-Augmented Generation
        BATCH_INSERT    ///< Bulk data loading
    };
    
    struct Config {
        // Workload optimization
        WorkloadType workload = WorkloadType::MIXED;
        
        // IVF Parameters
        size_t nlist = 1024;           // Number of clusters (sqrt(N) is good default)
        size_t nprobe = 64;            // Number of clusters to search (tradeoff: speed vs accuracy)
        
        // PQ Parameters
        bool use_pq = true;            // Enable Product Quantization
        size_t pq_m = 8;               // Number of sub-quantizers (d % m == 0)
        size_t pq_nbits = 8;           // Bits per sub-quantizer (8 or 16)
        
        // ADC (Asymmetric Distance Computation) Parameters (v1.5.x)
        bool use_adc_tables = true;    // Enable ADC tables for ~40% faster search
        int polysemous_ht = 0;         // Polysemous codes for early termination (0=disabled)
        
        // GPU Settings
        bool use_gpu = false;          // Use GPU acceleration
        int gpu_device = 0;            // GPU device ID
        
        // Training
        size_t train_size = 100000;    // Training set size (min 30 * nlist)
        
        // Index Type
        enum class Type {
            IVF_FLAT,     // IVF without compression (faster, more memory)
            IVF_PQ,       // IVF + Product Quantization (slower, less memory)
            HNSW_FLAT,    // HNSW without IVF (best accuracy, most memory)
            IVF_HNSW_PQ   // IVF + HNSW + PQ (best tradeoff)
        };
        Type index_type = Type::IVF_PQ;
    };
    
    struct SearchResult {
        std::vector<int64_t> ids;      // Document IDs
        std::vector<float> distances;  // L2 distances
    };
    
    /**
     * @brief Advanced Vector Index.
     * @param[in] dimension Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AdvancedVectorIndex(size_t dimension, const Config& config);
    ~AdvancedVectorIndex() noexcept;
    
    // Disable copy, allow move
    AdvancedVectorIndex(const AdvancedVectorIndex&) = delete;
    AdvancedVectorIndex& operator=(const AdvancedVectorIndex&) = delete;
    AdvancedVectorIndex(AdvancedVectorIndex&&) noexcept = default;
    AdvancedVectorIndex& operator=(AdvancedVectorIndex&&) noexcept = default;
    
    /**
     * @brief Train.
     * @param[in] vectors Input parameter.
     * @param[in] count Input parameter.
     * @return True when the operation succeeds.
     */
    bool train(const float* vectors, size_t count);
    
    /**
     * @brief Add.
     * @param[in] vectors Input parameter.
     * @param[in] count Input parameter.
     * @return True when the operation succeeds.
     */
    bool add(const float* vectors, size_t count);
    
    /**
     * @brief Add With Ids.
     * @param[in] vectors Input parameter.
     * @param[in] ids Input parameter.
     * @param[in] count Input parameter.
     * @return True when the operation succeeds.
     */
    bool addWithIds(const float* vectors, const int64_t* ids, size_t count);
    
    /**
     * @brief Search.
     * @param[in] query Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    SearchResult search(const float* query, size_t k);
    
    /**
     * @brief Search Batch.
     * @param[in] queries Input parameter.
     * @param[in] num_queries Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<SearchResult> searchBatch(const float* queries, size_t num_queries, size_t k);
    
    struct Stats {
        size_t total_vectors = 0;
        size_t index_size_bytes = 0;
        size_t memory_usage_bytes = 0;
        double compression_ratio = 1.0;  // vs Flat index
        bool is_trained = false;
        bool is_gpu = false;
    };
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;

    struct StubCallbacks {
        std::function<bool(size_t dimension, const Config& config)> initialize;
        std::function<bool(const float* vectors, size_t count)> train;
        std::function<bool(const float* vectors, size_t count)> add;
        std::function<bool(const float* vectors, const int64_t* ids, size_t count)> add_with_ids;
        std::function<SearchResult(const float* query, size_t k)> search;
        std::function<std::vector<SearchResult>(const float* queries, size_t num_queries, size_t k)> search_batch;
        std::function<Stats()> stats;
        std::function<bool(const std::string& path)> save;
        std::function<bool(const std::string& path)> load;
    };

    /**
     * @brief Set Stub Callbacks.
     * @param[in] callbacks Input parameter.
     * @details Calls: lk(), stubCallbacksMutex(), stubCallbacksStorage(), std::move().
     */
    static void setStubCallbacks(StubCallbacks callbacks) {
        std::lock_guard<std::mutex> lk(stubCallbacksMutex());
        stubCallbacksStorage() = std::move(callbacks);
    }
    
    /**
     * @brief Save.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool save(const std::string& path);
    
    /**
     * @brief Load.
     * @param[in] path Input parameter.
     * @return True when the operation succeeds.
     */
    bool load(const std::string& path);
    
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Get Workload Optimized Config.
     * @param[in] dataset_size Input parameter.
     * @param[in] dimension Input parameter.
     * @param[in] workload Input parameter.
     * @return Return value.
     */
    static Config getWorkloadOptimizedConfig(
        size_t dataset_size,
        size_t dimension,
        WorkloadType workload);

private:
    /**
     * @brief Stub Callbacks Mutex.
     * @return Return value.
     * @details Implements stubCallbacksMutex without additional internal calls.
     */
    static std::mutex& stubCallbacksMutex() {
        static std::mutex m;
        return m;
    }
    /**
     * @brief Stub Callbacks Storage.
     * @return Return value.
     * @details Implements stubCallbacksStorage without additional internal calls.
     */
    static StubCallbacks& stubCallbacksStorage() {
        static StubCallbacks callbacks;
        return callbacks;
    }
    size_t dimension_;
    Config config_;
    void* index_;  // Opaque FAISS index pointer
    bool is_trained_ = false;
    
    /**
     * @brief Initialize Index.
     * @return True when the operation succeeds.
     */
    bool initializeIndex();
};

} // namespace themis
