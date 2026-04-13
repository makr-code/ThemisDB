/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            faiss_gpu_backend.h                                ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:13:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     195                                            ║
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

#include "acceleration/compute_backend.h"
#include <memory>
#include <vector>
#include <string>

#ifdef THEMIS_ENABLE_CUDA
// Forward declarations to avoid including Faiss headers in this header
namespace faiss {
    class IndexFlatL2;
    class IndexFlatIP;
    class IndexIVFFlat;
    struct IndexIVFPQ;
    class gpu::GpuResources;
    class gpu::StandardGpuResources;
    namespace gpu {
        class GpuIndexFlatL2;
        class GpuIndexFlatIP;
        class GpuIndexIVFFlat;
        class GpuIndexIVFPQ;
    }
}
#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_CUDA

/**
 * FAISS GPU Vector Backend
 * Production-grade GPU vector search using Facebook's FAISS library
 * 
 * @sources
 * - Library: FAISS (Facebook AI Similarity Search)
 * - Repository: https://github.com/facebookresearch/faiss
 * - License: MIT
 * - Paper: Johnson, J., Douze, M., & Jégou, H. (2019)
 *          "Billion-scale similarity search with GPUs"
 *          IEEE Transactions on Big Data, 7(3), 535-547
 * - arXiv: https://arxiv.org/abs/1702.08734
 * - Index Types Used: IndexFlatL2, IndexFlatIP, IndexIVFFlat, IndexIVFPQ
 * - ThemisDB Integration: Multi-backend GPU support wrapper, integrated with
 *   RocksDB persistence and ACID transaction system
 */
class FaissGPUVectorBackend : public IVectorBackend {
public:
    enum class IndexType {
        FLAT_L2,        // Exact search, L2 distance
        FLAT_IP,        // Exact search, Inner Product
        IVF_FLAT,       // Inverted file with flat quantizer (fast approx)
        IVF_PQ          // Inverted file with product quantizer (memory efficient)
    };
    
    struct Config {
        IndexType indexType = IndexType::IVF_FLAT;
        int dimension = 128;
        int nlist = 100;           // Number of clusters for IVF
        int nprobe = 10;           // Number of clusters to search
        int m = 8;                 // Number of sub-quantizers (PQ)
        int nbits = 8;             // Bits per sub-quantizer (PQ)
        size_t maxMemoryMB = 8192; // Max GPU memory in MB
        int deviceId = 0;          // CUDA device ID
    };
    
    FaissGPUVectorBackend();
    ~FaissGPUVectorBackend() override;
    
    // IComputeBackend interface
    const char* name() const noexcept override { return "Faiss GPU"; }
    BackendType type() const noexcept override { return BackendType::CUDA; }
    bool isAvailable() const noexcept override;
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
    // IVectorBackend interface
    std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) override;
    
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) override;
    
    // Faiss-specific methods
    
    /**
     * Initialize index with configuration
     */
    bool initializeIndex(const Config& config);
    
    /**
     * Add vectors to the index (for persistent indices)
     */
    bool addVectors(const float* vectors, size_t numVectors);
    
    /**
     * Train index (required for IVF indices before adding vectors)
     */
    bool trainIndex(const float* vectors, size_t numVectors);
    
    /**
     * Search in pre-built index
     */
    std::vector<std::vector<std::pair<uint32_t, float>>> search(
        const float* queries,
        size_t numQueries,
        size_t k
    );
    
    /**
     * Save index to disk
     */
    bool saveIndex(const std::string& filepath);
    
    /**
     * Load index from disk
     */
    bool loadIndex(const std::string& filepath);
    
    /**
     * Get index statistics
     */
    struct IndexStats {
        size_t numVectors = 0;
        size_t dimension = 0;
        size_t memoryUsageBytes = 0;
        bool isTrained = false;
        IndexType type;
    };
    
    IndexStats getIndexStats() const;
    
    /**
     * Reset index (clear all vectors)
     */
    void resetIndex();
    
private:
    bool initialized_ = false;
    Config config_;
    
    // GPU resources
    std::unique_ptr<faiss::gpu::StandardGpuResources> gpuResources_;
    
    // Faiss index (one of the types)
    void* index_ = nullptr;  // Actual type depends on config
    IndexType currentIndexType_;
    
    // Helper methods
    void* createIndex(IndexType type, int dimension);
    void destroyIndex();
    bool transferIndexToGPU();
};

#endif // THEMIS_ENABLE_CUDA

} // namespace acceleration
} // namespace themis
