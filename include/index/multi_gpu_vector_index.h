#pragma once

#include "index/gpu_vector_index.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>
#include <cstdint>

namespace themis {
namespace index {

/**
 * Multi-GPU Vector Index
 * 
 * Extends GPUVectorIndex to support distributed vector search across multiple GPUs.
 * 
 * Features:
 * - Load distribution across 2-8 GPUs
 * - Multiple partitioning strategies (round-robin, hash-based, range-based)
 * - Collective operations (AllReduce, Broadcast, AllGather)
 * - Automatic workload balancing
 * - Fault tolerance with graceful degradation
 * - Support for CUDA (NCCL) and HIP (RCCL) backends
 * 
 * Performance Targets:
 * - 2 GPUs: 1.8x speedup (90% efficiency)
 * - 4 GPUs: 3.4x speedup (85% efficiency)
 * - 8 GPUs: 6.4x speedup (80% efficiency)
 * 
 * @version v2.4
 * @see docs/FUTURE_GPU_SUPPORT.md
 * @see docs/GPU_VECTOR_INDEXING_ARCHITECTURE.md
 */
class MultiGPUVectorIndex {
public:
    /**
     * Partitioning strategy for distributing vectors across GPUs
     */
    enum class PartitionStrategy {
        ROUND_ROBIN,    // Distribute vectors in round-robin fashion
        HASH_BASED,     // Hash vector ID to determine GPU
        RANGE_BASED,    // Assign contiguous ranges to each GPU
        BALANCED        // Balance load considering device capabilities
    };

    /**
     * Load balancing mode
     */
    enum class LoadBalancingMode {
        STATIC,         // Fixed distribution at initialization
        DYNAMIC         // Adaptive rebalancing based on utilization
    };

    /**
     * Multi-GPU configuration
     */
    struct Config {
        // GPU backend configuration
        GPUVectorIndex::Backend backend = GPUVectorIndex::Backend::AUTO;
        GPUVectorIndex::DistanceMetric metric = GPUVectorIndex::DistanceMetric::COSINE;
        
        // Multi-GPU settings
        bool enableMultiGPU = false;
        std::vector<int> deviceIds;  // List of GPU device IDs to use
        PartitionStrategy partitionStrategy = PartitionStrategy::ROUND_ROBIN;
        LoadBalancingMode loadBalancing = LoadBalancingMode::STATIC;
        bool enableP2P = true;  // Enable peer-to-peer transfers
        
        // HNSW parameters (per GPU)
        int M = 16;
        int efConstruction = 200;
        int efSearch = 64;
        
        // Batch processing
        int batchSize = 512;
        
        // Memory and performance
        bool useMixedPrecision = false;
        size_t maxVRAMPerGPU_MB = 0;  // 0 = auto-detect
        
        // Fault tolerance
        bool enableFaultTolerance = true;
        bool allowCPUFallback = true;
    };

    /**
     * Per-GPU statistics
     */
    struct GPUStatistics {
        int deviceId;
        size_t numVectors;
        size_t vramUsageBytes;
        double avgQueryTimeMs;
        double utilizationPercent;
        bool isActive;
        bool hasFailed;
    };

    /**
     * Aggregated multi-GPU statistics
     */
    struct Statistics {
        size_t totalVectors;
        size_t totalDimension;
        size_t numActiveGPUs;
        size_t numFailedGPUs;
        std::vector<GPUStatistics> perGPUStats;
        double avgQueryTimeMs;
        double throughputQPS;
        double scalingEfficiency;  // Actual speedup / ideal speedup
        double loadImbalance;      // Max load / avg load - 1.0
    };

    /**
     * Search result with GPU rank information
     */
    struct SearchResult {
        std::string id;
        float distance;
        int sourceGPU;  // Which GPU found this result
    };

    // Constructor & Destructor
    explicit MultiGPUVectorIndex(const Config& config = Config{});
    ~MultiGPUVectorIndex();

    // Initialization
    bool initialize(int dimension);
    void shutdown();

    // Vector operations
    bool addVector(const std::string& id, const std::vector<float>& vector);
    bool addVectorBatch(const std::vector<std::string>& ids, 
                       const std::vector<std::vector<float>>& vectors);
    bool removeVector(const std::string& id);
    bool updateVector(const std::string& id, const std::vector<float>& vector);

    // Search operations
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k);
    std::vector<std::vector<SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);

    // Multi-GPU control
    bool addGPU(int deviceId);
    bool removeGPU(int deviceId);
    bool rebalance();
    
    // Statistics and monitoring
    Statistics getStatistics() const;
    std::vector<int> getActiveGPUs() const;
    std::vector<int> getFailedGPUs() const;
    
    // Configuration
    void setPartitionStrategy(PartitionStrategy strategy);
    void setLoadBalancingMode(LoadBalancingMode mode);
    void setEfSearch(int ef);
    PartitionStrategy getPartitionStrategy() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis
