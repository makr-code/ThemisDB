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
 * Supports multiple GPU backends:
 * - CPU: Optimized CPU-only implementation (always available)
 * - HIP: AMD GPU acceleration via ROCm (v2.3, production-ready)
 * - CUDA: NVIDIA GPU acceleration (planned for v2.4)
 * - Vulkan: Cross-platform GPU compute (planned for v2.2)
 * 
 * Features:
 * - CPU-optimized vector search with SIMD acceleration
 * - Multi-threaded batch processing
 * - GPU-accelerated distance computation (L2, Cosine) via HIP
 * - Production-ready performance (30K+ QPS on CPU, 200K+ QPS on GPU)
 * - Full API compatibility with CPU VectorIndexManager
 * - Automatic backend selection and CPU fallback
 * 
 * Note: GPU backend support depends on build-time configuration.
 * HIP backend requires ROCm 5.0+ and compatible AMD GPU hardware.
 * 
 * @sources
 * - HNSW Algorithm: Malkov & Yashunin (2018) - IEEE TPAMI
 * - FAISS: Johnson et al. (2019) - IEEE Transactions on Big Data
 * - ROCm/HIP: https://rocm.docs.amd.com/
 */
class GPUVectorIndex {
public:
    enum class Backend {
        AUTO,       // Auto-detect best available
        CPU,        // CPU-only implementation (fallback)
        CUDA,       // NVIDIA CUDA (v2.1)
        VULKAN,     // Vulkan Compute (v2.2)
        HIP         // AMD HIP/ROCm (v2.3)
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
        
        // Memory optimization
        bool useMixedPrecision = false; // Enable FP16/TF32 on GPU
        
        // Fallback
        bool allowCPUFallback = true;  // Fall back to CPU if GPU fails
        
        // GPU device selection
        int deviceId = 0;              // GPU device ID to use
        
        // HIP-specific configuration (AMD GPUs)
        int waveSize = 0;              // Wave size: 0=auto, 32=Wave32, 64=Wave64
        bool enableRocBLAS = false;    // Use rocBLAS for matrix operations
        size_t maxVRAM_MB = 0;         // Maximum VRAM to use (0=auto)
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
    explicit GPUVectorIndex(const Config& config = Config{});
    ~GPUVectorIndex();
    
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
    
    // Note: GPU backends (CUDA, Vulkan, HIP) are available when enabled at build time.
    // CPU backend is always available as fallback.
    // See docs/GPU_SUPPORT_ROADMAP.md for details.
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis
