/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            advanced_vector_index.h                            ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:52:24                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     198                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <vector>
#include <memory>
#include <string>
#include <cstdint>

namespace themis {

/**
 * @brief Advanced FAISS Vector Index with IVF+PQ for production-scale search
 * 
 * v1.2.0 Feature: IVF (Inverted File) + PQ (Product Quantization)
 * 
 * Memory Reduction: 10-100x vs Flat index
 * Search Speed: 2-10x faster on large datasets (> 1M vectors)
 * 
 * Use Cases:
 * - Large-scale RAG (> 10M documents)
 * - Multi-tenant vector search
 * - Memory-constrained deployments
 * - Workload-optimized configurations (OLTP, Analytics, RAG)
 * 
 * @sources
 * - Based on: FAISS (Facebook AI Similarity Search)
 * - Library: https://github.com/facebookresearch/faiss
 * - Paper: Johnson, J., Douze, M., & Jégou, H. (2019). 
 *          "Billion-scale similarity search with GPUs." IEEE Transactions on Big Data.
 * - License: MIT
 * - ThemisDB Integration: Transactional wrapper with ACID guarantees,
 *   multi-backend GPU support, and RocksDB persistence layer
 * - Workload Optimization: PERFORMANCE_TIPS.md
 */
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
     * @brief Create advanced vector index
     */
    explicit AdvancedVectorIndex(size_t dimension, const Config& config);
    ~AdvancedVectorIndex();
    
    // Disable copy, allow move
    AdvancedVectorIndex(const AdvancedVectorIndex&) = delete;
    AdvancedVectorIndex& operator=(const AdvancedVectorIndex&) = delete;
    AdvancedVectorIndex(AdvancedVectorIndex&&) = default;
    AdvancedVectorIndex& operator=(AdvancedVectorIndex&&) = default;
    
    /**
     * @brief Train index on sample data
     * 
     * Required for IVF-based indexes before adding vectors
     */
    bool train(const float* vectors, size_t count);
    
    /**
     * @brief Add vectors to index
     */
    bool add(const float* vectors, size_t count);
    
    /**
     * @brief Add vectors with IDs
     */
    bool addWithIds(const float* vectors, const int64_t* ids, size_t count);
    
    /**
     * @brief Search for k nearest neighbors
     */
    SearchResult search(const float* query, size_t k);
    
    /**
     * @brief Batch search for multiple queries
     */
    std::vector<SearchResult> searchBatch(const float* queries, size_t num_queries, size_t k);
    
    /**
     * @brief Get index statistics
     */
    struct Stats {
        size_t total_vectors = 0;
        size_t index_size_bytes = 0;
        size_t memory_usage_bytes = 0;
        double compression_ratio = 1.0;  // vs Flat index
        bool is_trained = false;
        bool is_gpu = false;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Save index to disk
     */
    bool save(const std::string& path);
    
    /**
     * @brief Load index from disk
     */
    bool load(const std::string& path);
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Get workload-optimized configuration
     * @param dataset_size Expected dataset size
     * @param dimension Vector dimensionality
     * @param workload Workload type
     * @return Optimized configuration for workload
     */
    static Config getWorkloadOptimizedConfig(
        size_t dataset_size,
        size_t dimension,
        WorkloadType workload);

private:
    size_t dimension_;
    Config config_;
    void* index_;  // Opaque FAISS index pointer
    bool is_trained_ = false;
    
    /**
     * @brief Initialize FAISS index based on config
     */
    bool initializeIndex();
};

} // namespace themis
