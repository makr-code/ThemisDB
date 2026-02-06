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
 * NOTE: This is currently a CPU-only implementation with GPU fallback support.
 * Full GPU acceleration (CUDA, Vulkan, HIP) is planned for v2.x.
 * See docs/FUTURE_GPU_SUPPORT.md for the roadmap.
 * 
 * Features:
 * - CPU-optimized vector search with SIMD acceleration
 * - Multi-threaded batch processing
 * - Production-ready performance (30K+ queries/sec on CPU)
 * - Full API compatibility with CPU VectorIndexManager
 * - Future: GPU acceleration planned for v2.x
 * 
 * @sources
 * - HNSW Algorithm: Malkov & Yashunin (2018) - IEEE TPAMI
 * - FAISS: Johnson et al. (2019) - IEEE Transactions on Big Data
 */
class GPUVectorIndex {
public:
    enum class Backend {
        AUTO,       // Auto-detect best available (currently CPU-only)
        CPU         // CPU-only implementation (default)
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
        bool useMixedPrecision = false; // Reserved for future GPU support
        
        // Fallback (always enabled in current version)
        bool allowCPUFallback = true;  // Always uses CPU in current version
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
    
    // Note: In the current version, only CPU backend is available.
    // GPU backends (CUDA, Vulkan, HIP) are planned for v2.x.
    // See docs/FUTURE_GPU_SUPPORT.md for details.
    
private:
    class Impl;
    std::unique_ptr<Impl> pImpl;
};

} // namespace index
} // namespace themis
