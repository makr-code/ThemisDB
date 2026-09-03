/**
 * @file gpu_vector_index.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "index/vector_index.h"
#include "index/gpu_memory_oversubscription.h"
#include "acceleration/compute_backend.h"
#include <memory>
#include <vector>
#include <string>
#include <optional>

namespace themis {
namespace index {

/**
 * GPU-Accelerated Vector Index
 * 
 * Provides GPU-accelerated vector similarity search with multiple backend support:
 * - Vulkan: Cross-platform GPU compute (NVIDIA, AMD, Intel, Apple via MoltenVK) - v2.2
 * - CUDA: NVIDIA GPUs (planned)
 * - HIP: AMD GPUs (planned)
 * - CPU: Fallback with SIMD acceleration
 * 
 * Features:
 * - Cross-platform GPU acceleration via Vulkan
 * - CPU-optimized SIMD fallback
 * - Multi-threaded batch processing
 * - Automatic backend selection
 * - Production-ready performance (200K+ queries/sec on GPU)
 * 
 * Sources:
 * - HNSW Algorithm: Malkov & Yashunin (2018) - IEEE TPAMI
 * - FAISS: Johnson et al. (2019) - IEEE Transactions on Big Data
 * - ROCm/HIP: https://rocm.docs.amd.com/
 */
class GPUVectorIndex {
public:
    enum class Backend {
        AUTO,       // Auto-detect best available backend
        CPU,        // CPU-only implementation (default)
        VULKAN,     // Vulkan compute backend (cross-platform GPU) - v2.2
        CUDA,       // CUDA backend (NVIDIA GPUs, planned)
        HIP         // HIP backend (AMD GPUs, planned)
    };
    
    enum class DistanceMetric {
        L2,         // Euclidean distance: ||a - b||²
        COSINE,     // Cosine distance: 1 - (a·b)/(||a|| ||b||)
        INNER_PRODUCT // Inner product: max(0, -a·b)
    };
    
    struct Config {
        Backend backend = Backend::AUTO;
        DistanceMetric metric = DistanceMetric::COSINE;
        
        // HNSW parameters
        int M = 16;                    // Number of connections per layer
        int efConstruction = 200;      // Construction time accuracy
        int efSearch = 64;             // Query time accuracy
        
        // Batch processing
        int batchSize = 512;           // Batch size for parallel search
        
        // GPU-specific options
        int deviceId = 0;              // GPU device ID (0 = default)
        bool enableValidation = false; // Enable GPU validation layers (debug)
        size_t maxVRAM_MB = 0;         // Max VRAM usage in MB (0 = no limit); enforced via GPUMemoryManager
        uint32_t workgroupSize = 256;  // Compute workgroup size (not yet implemented)
        
        // Memory optimization
        bool useMixedPrecision = false; // Enable FP16/TF32 (GPU backends)
        
        // GPU Memory Oversubscription (v1.7.0)
        // When enabled, vectors are partitioned and streamed from host RAM into
        // VRAM as needed.  Allows indexes larger than available GPU VRAM.
        bool enable_oversubscription = false;          // Enable paging/streaming
        size_t vram_budget_mb = 0;                     // VRAM cap in MB (0 = no limit)
        PrefetchStrategy prefetch_strategy = PrefetchStrategy::LRU; // Eviction/prefetch policy
        size_t oversubscription_partition_vectors = 65536; // Vectors per partition chunk
        
        // Fallback
        // Production contract:
        // - true: explicit failover to CPU is allowed on backend gate or runtime failure
        // - false: fail closed (initialize/search return failure/empty result)
        bool allowCPUFallback = true;
    };
    
    struct SearchResult {
        std::string id;
        float distance;
    };
    
    struct Statistics {
        size_t numVectors = 0;
        size_t dimension = 0;
        Backend activeBackend = Backend::CPU;
        size_t vramUsageBytes = 0;
        double avgQueryTimeMs = 0.0;
        double throughputQPS = 0.0;
        bool isGPUActive = false;
        // Oversubscription statistics (populated when enable_oversubscription = true).
        bool oversubscriptionActive = false;
        size_t oversubHotPartitions  = 0;  ///< Partitions currently in VRAM.
        size_t oversubColdPartitions = 0;  ///< Partitions in host RAM only.
        size_t oversubEvictions      = 0;  ///< Total LRU evictions.
        size_t oversubLoads          = 0;  ///< Total partition loads into VRAM.
        double oversubPrefetchHitRate = 0.0; ///< Prefetch hit rate [0,1].
    };
    
    // Constructor
    GPUVectorIndex();
    explicit GPUVectorIndex(const Config& config);
    ~GPUVectorIndex() noexcept;
    
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
    
    // Index management
    bool buildIndex();
    bool saveIndex(const std::string& path);
    bool loadIndex(const std::string& path);
    
    // Configuration
    void setEfSearch(int ef);
    void setBatchSize(int size);
    Backend getActiveBackend() const;
    Statistics getStatistics() const;
    
    // Backend control
    bool switchBackend(Backend backend);
    std::vector<Backend> getAvailableBackends() const;

    // Oversubscription control (v1.7.0)
    // Returns the oversubscription stats; the returned Stats::oversubscriptionActive
    // field is false when oversubscription is disabled.
    GPUMemoryOversubscriptionManager::Stats getOversubscriptionStats() const;
    
    // Backend availability:
    // - CPU: Always available (fallback)
    // - VULKAN: Available if Vulkan SDK installed and GPU present
    // - CUDA: Planned for v2.1 (NVIDIA GPUs)
    // - HIP: Planned for v2.3 (AMD GPUs)
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis
