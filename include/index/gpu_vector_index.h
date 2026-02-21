/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            gpu_vector_index.h                                 ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:33:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     157                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7ff413ebb  2026-02-17  Add test certificates for CA and plugin signer ║
    • 37da19d1c  2026-02-10  Refactor code structure for improved readability and main... ║
    • 2890afd14  2026-02-07  Add HIP/ROCm backend for AMD GPU acceleration (v2.3) (#1094) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
 * @sources
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
        size_t maxVRAM_MB = 0;         // Max VRAM usage in MB (0 = auto, not yet implemented)
        uint32_t workgroupSize = 256;  // Compute workgroup size (not yet implemented)
        
        // Memory optimization
        bool useMixedPrecision = false; // Enable FP16/TF32 (GPU backends)
        
        // Fallback
        bool allowCPUFallback = true;  // Fall back to CPU if GPU unavailable
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
    GPUVectorIndex();
    explicit GPUVectorIndex(const Config& config);
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
