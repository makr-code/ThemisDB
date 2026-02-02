#pragma once

#include "index/vector_index.h"
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
 * Unified interface for GPU-accelerated HNSW vector search across multiple backends:
 * - Vulkan (cross-platform, default)
 * - CUDA (NVIDIA GPUs)
 * - HIP (AMD ROCm)
 * 
 * Features:
 * - Automatic backend selection based on available hardware
 * - Graceful CPU fallback when GPU unavailable
 * - Production-ready performance (50K+ queries/sec)
 * - Full API compatibility with CPU VectorIndexManager
 * - Multi-GPU support with load balancing
 * 
 * @sources
 * - HNSW Algorithm: Malkov & Yashunin (2018) - IEEE TPAMI
 * - FAISS GPU: Johnson et al. (2019) - IEEE Transactions on Big Data
 * - Vulkan Compute: Khronos Vulkan Specification 1.3
 * - Flash Attention: Dao et al. (2022) - NeurIPS
 * - vLLM Paged Attention: Kwon et al. (2023) - SOSP
 */
class GPUVectorIndex {
public:
    enum class Backend {
        AUTO,       // Auto-detect best available
        VULKAN,     // Cross-platform (default)
        CUDA,       // NVIDIA
        HIP,        // AMD ROCm
        CPU         // Fallback
    };
    
    enum class DistanceMetric {
        L2,         // Euclidean distance: ||a - b||²
        COSINE,     // Cosine distance: 1 - (a·b)/(||a|| ||b||)
        INNER_PRODUCT // Inner product: max(0, -a·b)
    };
    
    struct Config {
        Backend backend = Backend::AUTO;
        DistanceMetric metric = DistanceMetric::COSINE;
        
        // HNSW parameters (validated to safe ranges, invalid values auto-corrected)
        int M = 16;                    // Number of connections per layer (range: 1-256)
        int efConstruction = 200;      // Construction time accuracy (range: 1-2000)
        int efSearch = 64;             // Query time accuracy (range: 1-2000)
        
        // GPU-specific (validated, invalid values auto-corrected)
        int batchSize = 512;           // Batch size for parallel search (range: 1-10000)
        size_t maxVRAM_MB = 8192;      // Max VRAM usage in MB (range: 1-1048576)
        int deviceId = 0;              // GPU device ID (range: 0-16)
        bool enableMultiGPU = false;   // Enable multi-GPU support
        
        // Memory optimization
        bool useMixedPrecision = true; // Use FP16/TF32 for better performance
        bool useUnifiedMemory = false; // CUDA unified memory (slower but larger capacity)
        
        // Fallback
        bool allowCPUFallback = true;  // Fall back to CPU if GPU fails
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
    };
    
    // Constructor
    // Config parameters are validated and auto-corrected to safe defaults if invalid
    explicit GPUVectorIndex(const Config& config = Config{});
    ~GPUVectorIndex();
    
    // Initialization
    // @param dimension Vector dimension (must be > 0 and <= 10000)
    // @return true on success, false if dimension is invalid or backend initialization fails
    bool initialize(int dimension);
    void shutdown();
    
    // Vector operations
    // @param id Unique identifier for the vector (cannot be empty)
    // @param vector Vector data (must match initialized dimension, no NaN/Inf values allowed)
    // @return true on success, false if validation fails or index not initialized
    bool addVector(const std::string& id, const std::vector<float>& vector);
    bool addVectorBatch(const std::vector<std::string>& ids, 
                       const std::vector<std::vector<float>>& vectors);
    bool removeVector(const std::string& id);
    bool updateVector(const std::string& id, const std::vector<float>& vector);
    
    // Search operations
    // @param query Query vector (must match initialized dimension, no NaN/Inf values allowed)
    // @param k Number of nearest neighbors to return (must be > 0)
    // @return Search results sorted by distance, empty vector on validation failure
    std::vector<SearchResult> search(const std::vector<float>& query, size_t k);
    std::vector<std::vector<SearchResult>> searchBatch(
        const std::vector<std::vector<float>>& queries, size_t k);
    
    // Index management
    bool buildIndex();
    bool saveIndex(const std::string& path);
    bool loadIndex(const std::string& path);
    
    // Configuration
    // @param ef efSearch parameter (must be > 0, recommended range: 1-2000)
    void setEfSearch(int ef);
    // @param size Batch size (must be > 0, recommended range: 1-10000)
    void setBatchSize(int size);
    Backend getActiveBackend() const;
    Statistics getStatistics() const;
    
    // Backend control
    bool switchBackend(Backend backend);
    std::vector<Backend> getAvailableBackends() const;
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * Vulkan GPU Vector Index Backend
 * Cross-platform GPU acceleration using Vulkan Compute Shaders
 */
class VulkanVectorIndexBackend {
public:
    VulkanVectorIndexBackend();
    ~VulkanVectorIndexBackend();
    
    bool initialize(int dimension, const GPUVectorIndex::Config& config);
    void shutdown();
    
    // Distance computation kernels
    std::vector<float> computeL2Distance(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors, size_t dim);
    
    std::vector<float> computeCosineDistance(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors, size_t dim);
    
    std::vector<float> computeInnerProduct(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors, size_t dim);
    
    // Batch search
    std::vector<std::vector<std::pair<uint32_t, float>>> batchSearch(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dim, size_t k, GPUVectorIndex::DistanceMetric metric);
    
    // Multi-GPU support
    bool enableMultiGPU(int numDevices);
    void distributeLoad(const std::vector<float>& vectors, int deviceId);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * CUDA GPU Vector Index Backend
 * NVIDIA GPU acceleration with advanced optimizations
 */
class CUDAVectorIndexBackend {
public:
    CUDAVectorIndexBackend();
    ~CUDAVectorIndexBackend();
    
    bool initialize(int dimension, const GPUVectorIndex::Config& config);
    void shutdown();
    
    // Mixed-precision support
    bool enableMixedPrecision(bool useFP16, bool useTF32, bool useINT8);
    
    // Flash Attention-style optimizations
    void enableFlashAttentionOptimization(bool enable);
    
    // Tensor Core support
    bool hasTensorCoreSupport() const;
    void enableTensorCores(bool enable);
    
    // Memory coalescing
    void optimizeMemoryCoalescing();
    
    // Unified memory
    bool enableUnifiedMemory(bool enable);
    
    // Graph execution for kernel fusion
    bool createComputeGraph();
    void executeComputeGraph();
    
    // Distance computation
    std::vector<float> computeDistances(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dim, GPUVectorIndex::DistanceMetric metric);
    
    // Batch search
    std::vector<std::vector<std::pair<uint32_t, float>>> batchSearch(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dim, size_t k, GPUVectorIndex::DistanceMetric metric);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

/**
 * HIP GPU Vector Index Backend
 * AMD ROCm acceleration with AMD-specific optimizations
 */
class HIPVectorIndexBackend {
public:
    HIPVectorIndexBackend();
    ~HIPVectorIndexBackend();
    
    bool initialize(int dimension, const GPUVectorIndex::Config& config);
    void shutdown();
    
    // rocBLAS integration
    bool enableRocBLAS(bool enable);
    
    // AMD-specific optimizations
    void optimizeForRDNA2();
    void optimizeForRDNA3();
    
    // Wave size tuning (Wave64 vs Wave32)
    void setWaveSize(int waveSize);
    
    // Multi-GPU collective operations (RCCL)
    bool enableRCCL(int numDevices);
    void ringAllReduce(float* data, size_t size);
    void collectiveBroadcast(const float* src, float* dst, size_t size, int rootDevice);
    
    // Distance computation
    std::vector<float> computeDistances(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dim, GPUVectorIndex::DistanceMetric metric);
    
    // Batch search
    std::vector<std::vector<std::pair<uint32_t, float>>> batchSearch(
        const float* queries, size_t numQueries,
        const float* vectors, size_t numVectors,
        size_t dim, size_t k, GPUVectorIndex::DistanceMetric metric);
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis
