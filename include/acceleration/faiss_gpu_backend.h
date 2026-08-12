/**
 * @file faiss_gpu_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/error_context.h"
#include <memory>
#include <mutex>
#include <vector>
#include <string>

#ifdef THEMIS_ENABLE_CUDA
// Forward declarations to avoid including Faiss headers in this header
namespace faiss {
    class Index;
    class IndexFlatL2;
    class IndexFlatIP;
    class IndexIVFFlat;
    struct IndexIVFPQ;
    class IndexHNSWFlat;
    namespace gpu {
        class GpuResources;
        class StandardGpuResources;
        class GpuIndexFlatL2;
        class GpuIndexFlatIP;
        class GpuIndexIVFFlat;
        class GpuIndexIVFPQ;
        class GpuIndexIVFScalarQuantizer;
    }
}
#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_CUDA

/**
 * FAISS GPU Vector Backend
 * Production-grade GPU vector search using Facebook's FAISS library.
 *
 * Supported index types:
 *   FLAT_L2  – Exact brute-force L2 (GPU)
 *   FLAT_IP  – Exact brute-force inner-product (GPU)
 *   IVF_FLAT – Inverted-file with flat quantizer (GPU, fast approximate)
 *   IVF_PQ   – Inverted-file + product quantizer (GPU, memory-efficient approx)
 *   IVF_SQ8  – Inverted-file + 8-bit scalar quantizer (GPU, higher recall than PQ
 *               at equivalent memory)
 *   HNSW_FLAT– Hierarchical Navigable Small World flat graph (CPU-side FAISS HNSW;
 *               use when low-latency single-query search is needed without a GPU
 *               at query time — same IVectorBackend interface)
 *
 * Sources:
 * - Library: FAISS (Facebook AI Similarity Search)
 * - Repository: https://github.com/facebookresearch/faiss
 * - License: MIT
 * - Paper: Johnson, J., Douze, M., & Jégou, H. (2019)
 *          "Billion-scale similarity search with GPUs"
 *          IEEE Transactions on Big Data, 7(3), 535-547
 * - arXiv: https://arxiv.org/abs/1702.08734
 * - ThemisDB Integration: Multi-backend GPU support wrapper, integrated with
 *   RocksDB persistence and ACID transaction system
 */
class FaissGPUVectorBackend : public IVectorBackend {
public:
    enum class IndexType {
        FLAT_L2,        // Exact search, L2 distance (GPU)
        FLAT_IP,        // Exact search, Inner Product (GPU)
        IVF_FLAT,       // Inverted file with flat quantizer (GPU, fast approx)
        IVF_PQ,         // Inverted file with product quantizer (GPU, memory efficient)
        IVF_SQ8,        // Inverted file with 8-bit scalar quantizer (GPU, better recall than PQ)
        HNSW_FLAT,      // HNSW graph with flat storage (CPU-side FAISS HNSW)
    };

    struct Config {
        IndexType indexType = IndexType::IVF_FLAT;
        int dimension = 128;
        int nlist = 100;           // Number of clusters for IVF indices
        int nprobe = 10;           // Number of clusters to search at query time
        int m = 8;                 // Sub-quantizers for IVF_PQ
        int nbits = 8;             // Bits per sub-quantizer for IVF_PQ
        int hnswM = 32;            // Connections per HNSW node (HNSW_FLAT only)
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
    ErrorContext getLastError() const override { return lastError_; }
    
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
    mutable ErrorContext lastError_;

    // GPU resources (null for CPU-only index types such as HNSW_FLAT)
    std::unique_ptr<faiss::gpu::StandardGpuResources> gpuResources_;

    // Faiss index (concrete type depends on config_.indexType).
    // Owned via unique_ptr with a type-dispatching custom deleter.
    struct IndexDeleter {
        IndexType type = IndexType::IVF_FLAT;
        void operator()(faiss::Index* p) const noexcept;
    };
    std::unique_ptr<faiss::Index, IndexDeleter> index_;
    IndexType currentIndexType_ = IndexType::IVF_FLAT;
    mutable std::mutex indexMutex_; // guards index_ and config_.nprobe accesses

    // Helper methods
    std::unique_ptr<faiss::Index, IndexDeleter> createIndex(IndexType type, int dimension);
    bool transferIndexToGPU();

    void setError(AccelerationErrorCode code, const std::string& msg,
                  const std::string& hint = "") const;
};

#endif // THEMIS_ENABLE_CUDA

} // namespace acceleration
} // namespace themis
