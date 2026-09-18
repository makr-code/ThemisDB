/**
 * @file multi_gpu_vector_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


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
 * Multi-GPU Vector Index API (v2.4)
 * 
 * Provides a multi-device API and scaffolding for distributed vector search.
 * 
 * **Current Status (v2.4)**: This implementation provides the API surface and
 * partitioning/merge logic for multi-GPU vector indexing. The underlying
 * GPUVectorIndex currently uses CPU-only execution. Full GPU acceleration with
 * NCCL/RCCL collectives and device-to-device transfers will be available in v2.5+.
 * 
 * Features (v2.4):
 * - Multi-device API with partition strategies (round-robin, hash-based, range-based, balanced)
 * - Query fan-out and top-k result merging
 * - Runtime device management (add/remove)
 * - Per-partition statistics and monitoring
 * - Fault tolerance with graceful degradation
 * - CPU fallback support
 * 
 * Planned (v2.5+):
 * - Actual GPU execution on multiple devices
 * - NCCL (NVIDIA) and RCCL (AMD) collective operations
 * - Peer-to-peer GPU transfers
 * - Dynamic load balancing with data migration
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
     * Communication backend for multi-GPU operations
     */
    enum class CommBackend {
        AUTO,       // Auto-detect (NCCL for NVIDIA, RCCL for AMD, fallback to CPU)
        NCCL,       // NVIDIA NCCL (CUDA only)
        RCCL,       // AMD RCCL (HIP only)
        CPU         // CPU-based communication (no GPU collectives)
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
        
        // Communication backend (v2.5+)
        CommBackend commBackend = CommBackend::AUTO;
        bool enableNVLink = true;   // Use NVIDIA NVLink if available
        bool enableXGMI = true;     // Use AMD Infinity Fabric if available
        size_t commBufferSizeMB = 256;  // Communication buffer size
        
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
        int deviceId = 0;
        size_t numVectors = 0;
        size_t vramUsageBytes = 0;
        double avgQueryTimeMs = 0.0;
        double utilizationPercent = 0.0;
        bool isActive = false;
        bool hasFailed = false;
    };

    /**
     * Aggregated multi-GPU statistics
     */
    struct Statistics {
        size_t totalVectors = 0;
        size_t totalDimension = 0;
        size_t numActiveGPUs = 0;
        size_t numFailedGPUs = 0;
        std::vector<GPUStatistics> perGPUStats;
        double avgQueryTimeMs = 0.0;
        double throughputQPS = 0.0;
        double scalingEfficiency = 0.0;  // Actual speedup / ideal speedup
        double loadImbalance = 0.0;      // Max load / avg load - 1.0
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
    MultiGPUVectorIndex();
    /**
     * @brief TBD: Describe MultiGPUVectorIndex.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit MultiGPUVectorIndex(const Config& config);
    ~MultiGPUVectorIndex();

    /**
     * @brief Initialization
     * @param[in] dimension Input parameter.
     * @return True on success.
     */
    bool initialize(int dimension);
    /**
     * @brief TBD: Describe shutdown.
     */
    void shutdown();

    /**
     * @brief Vector operations
     * @param[in] id Input parameter.
     * @param[in] vector Input parameter.
     * @return True on success.
     */
    bool addVector(const std::string& id, const std::vector<float>& vector);
    /**
     * @brief TBD: Describe addVectorBatch.
     * @param[in] ids Input parameter.
     * @param[in] vectors Input parameter.
     * @return True on success.
     */
    bool addVectorBatch(const std::vector<std::string>& ids, 
                       const std::vector<std::vector<float>>& vectors);
    /**
     * @brief TBD: Describe removeVector.
     * @param[in] id Input parameter.
     * @return True on success.
     */
    bool removeVector(const std::string& id);
    /**
     * @brief TBD: Describe updateVector.
     * @param[in] id Input parameter.
     * @param[in] vector Input parameter.
     * @return True on success.
     */
    bool updateVector(const std::string& id, const std::vector<float>& vector);

    /**
     * @brief Search operations
     * @param[in] query Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k);
    /**
     * @brief TBD: Describe searchBatch.
     * @param[in] queries Input parameter.
     * @param[in] k Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);

    /**
     * @brief Multi-GPU control
     * @param[in] deviceId Input parameter.
     * @return True on success.
     */
    bool addGPU(int deviceId);
    /**
     * @brief TBD: Describe removeGPU.
     * @param[in] deviceId Input parameter.
     * @return True on success.
     */
    bool removeGPU(int deviceId);
    /**
     * @brief TBD: Describe rebalance.
     * @return True on success.
     */
    bool rebalance();
    
    /**
     * @brief Statistics and monitoring
     * @return Return value.
     */
    Statistics getStatistics() const;
    /**
     * @brief TBD: Describe getActiveGPUs.
     * @return Return value.
     */
    std::vector<int> getActiveGPUs() const;
    /**
     * @brief TBD: Describe getFailedGPUs.
     * @return Return value.
     */
    std::vector<int> getFailedGPUs() const;
    
    /**
     * @brief Configuration
     * @param[in] strategy Input parameter.
     */
    void setPartitionStrategy(PartitionStrategy strategy);
    /**
     * @brief TBD: Describe setLoadBalancingMode.
     * @param[in] mode Input parameter.
     */
    void setLoadBalancingMode(LoadBalancingMode mode);
    /**
     * @brief TBD: Describe setEfSearch.
     * @param[in] ef Input parameter.
     */
    void setEfSearch(int ef);
    /**
     * @brief TBD: Describe getPartitionStrategy.
     * @return Return value.
     */
    PartitionStrategy getPartitionStrategy() const;
    
    /**
     * @brief Communication backend control (v2.
     * @return Return value.
     * @details 5+)
     */
    CommBackend getCommBackend() const;
    /**
     * @brief TBD: Describe isCollectiveOpsAvailable.
     * @return True on success.
     */
    bool isCollectiveOpsAvailable() const;
    /**
     * @brief TBD: Describe isP2PTransferAvailable.
     * @return True on success.
     */
    bool isP2PTransferAvailable() const;
    /**
     * @brief TBD: Describe isNVLinkAvailable.
     * @return True on success.
     */
    bool isNVLinkAvailable() const;
    /**
     * @brief TBD: Describe isXGMIAvailable.
     * @return True on success.
     */
    bool isXGMIAvailable() const;

private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis

