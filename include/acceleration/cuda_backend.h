/**
 * @file cuda_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/tensor_core_matmul.h"
#include "index/cuda_hnsw_graph_traversal.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/raii/cuda_raii.h"
#include <cuda_runtime.h>
#include <cstdint>
#include <mutex>
#include <unordered_map>
#endif

namespace themis {
namespace acceleration {

#ifdef THEMIS_ENABLE_CUDA

// ============================================================================
// CUDA Graph Capture — QueryShape, CUDAGraphEntry, CUDAGraphCache
//
// CUDAGraphCache stores one captured CUDA graph per unique (numQueries,
// numVectors, dim, topK, metric) tuple.  On the first call for a given shape
// the kernel sequence is recorded into a cudaGraph_t and instantiated as a
// cudaGraphExec_t.  Subsequent calls with the same shape copy input data into
// the pre-allocated device buffers and replay the graph, eliminating repeated
// kernel-launch overhead.  The cache evicts the least-recently-used entry when
// it grows beyond kMaxEntries to bound device-memory usage.
// ============================================================================

struct QueryShape {
    int numQueries = 0;
    int numVectors = 0;
    int dim        = 0;
    int topK       = 0;
    DistanceMetric metric = DistanceMetric::L2;

    bool operator==(const QueryShape& o) const noexcept {
        return numQueries == o.numQueries && numVectors == o.numVectors &&
               dim == o.dim && topK == o.topK && metric == o.metric;
    }
};

struct QueryShapeHash {
    std::size_t operator()(const QueryShape& s) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        auto mix = [&](std::size_t v) {
            h ^= v;
            h *= 1099511628211ULL;
        };
        mix(static_cast<std::size_t>(s.numQueries));
        mix(static_cast<std::size_t>(s.numVectors));
        mix(static_cast<std::size_t>(s.dim));
        mix(static_cast<std::size_t>(s.topK));
        mix(static_cast<std::size_t>(s.metric));
        return h;
    }
};

struct CUDAGraphEntry {
    cudaGraph_t     graph = nullptr;
    cudaGraphExec_t exec  = nullptr;

    // Pre-allocated device buffers — same pointers used at capture time.
    raii::CudaDeviceMemory d_queries;
    raii::CudaDeviceMemory d_vectors;
    raii::CudaDeviceMemory d_distances;
    raii::CudaDeviceMemory d_topkIndices;
    raii::CudaDeviceMemory d_topkDistances;

    // Monotonically-increasing access counter for LRU eviction.
    uint64_t lastAccess = 0;

    CUDAGraphEntry() = default;
    ~CUDAGraphEntry();

    // Non-copyable
    CUDAGraphEntry(const CUDAGraphEntry&) = delete;
    CUDAGraphEntry& operator=(const CUDAGraphEntry&) = delete;

    // Movable — zeros out CUDA handles in the moved-from object.
    CUDAGraphEntry(CUDAGraphEntry&&) noexcept;
    CUDAGraphEntry& operator=(CUDAGraphEntry&&) noexcept;
};

class CUDAGraphCache {
public:
    static constexpr size_t kMaxEntries = 32;

    /**
     * @brief Get.
     * @param[in] shape Input parameter.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    CUDAGraphEntry* get(const QueryShape& shape) noexcept;

    /**
     * @brief Put.
     * @param[in] shape Input parameter.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    CUDAGraphEntry& put(const QueryShape& shape, CUDAGraphEntry entry);

    size_t size() const noexcept { return entries_.size(); }

    /**
     * @brief Clear.
     */
    void clear();

private:
    /**
     * @brief Evict LRU.
     */
    void evictLRU();

    std::unordered_map<QueryShape, CUDAGraphEntry, QueryShapeHash> entries_;
    uint64_t clock_ = 0;
};

// ============================================================================
// Graph BFS — GraphBFSShape, CUDAGraphBFSEntry, CUDAGraphBFSCache
//
// Mirrors the KNN graph-capture design but keyed on (numVertices, numStarts,
// maxDepth).  The captured graph contains: one init kernel + maxDepth BFS
// expand kernels + one gather kernel.  Device buffers are stable across
// replays; only the adjacency and startVertices data are refreshed before
// each cudaGraphLaunch.
// ============================================================================

struct GraphBFSShape {
    int numVertices = 0;
    int numStarts   = 0;
    int maxDepth    = 0;

    bool operator==(const GraphBFSShape& o) const noexcept {
        return numVertices == o.numVertices &&
               numStarts   == o.numStarts   &&
               maxDepth    == o.maxDepth;
    }
};

struct GraphBFSShapeHash {
    std::size_t operator()(const GraphBFSShape& s) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        auto mix = [&](std::size_t v) { h ^= v; h *= 1099511628211ULL; };
        mix(static_cast<std::size_t>(s.numVertices));
        mix(static_cast<std::size_t>(s.numStarts));
        mix(static_cast<std::size_t>(s.maxDepth));
        return h;
    }
};

struct CUDAGraphBFSEntry {
    cudaGraph_t     graph = nullptr;
    cudaGraphExec_t exec  = nullptr;

    // Pre-allocated device buffers (same pointers used at capture and replay time).
    raii::CudaDeviceMemory d_adjacency;      ///< [numVertices × numVertices] uint32_t
    raii::CudaDeviceMemory d_startVertices;  ///< [numStarts] uint32_t
    raii::CudaDeviceMemory d_frontier_a;     ///< [numStarts × numVertices] uint32_t
    raii::CudaDeviceMemory d_frontier_b;     ///< [numStarts × numVertices] uint32_t
    raii::CudaDeviceMemory d_visited;        ///< [numStarts × numVertices] uint32_t
    raii::CudaDeviceMemory d_depths;         ///< [numStarts × numVertices] uint32_t
    raii::CudaDeviceMemory d_result_vertices;///< [numStarts × numVertices] uint32_t
    raii::CudaDeviceMemory d_result_sizes;   ///< [numStarts] int

    uint64_t lastAccess = 0;

    CUDAGraphBFSEntry() = default;
    ~CUDAGraphBFSEntry();

    CUDAGraphBFSEntry(const CUDAGraphBFSEntry&) = delete;
    CUDAGraphBFSEntry& operator=(const CUDAGraphBFSEntry&) = delete;

    CUDAGraphBFSEntry(CUDAGraphBFSEntry&&) noexcept;
    CUDAGraphBFSEntry& operator=(CUDAGraphBFSEntry&&) noexcept;
};

class CUDAGraphBFSCache {
public:
    static constexpr size_t kMaxEntries = 16;

    /**
     * @brief Get.
     * @param[in] shape Input parameter.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    CUDAGraphBFSEntry* get(const GraphBFSShape& shape) noexcept;
    /**
     * @brief Put.
     * @param[in] shape Input parameter.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    CUDAGraphBFSEntry& put(const GraphBFSShape& shape, CUDAGraphBFSEntry entry);
    size_t size() const noexcept { return entries_.size(); }
    /**
     * @brief Clear.
     */
    void clear();

private:
    /**
     * @brief Evict LRU.
     */
    void evictLRU();
    std::unordered_map<GraphBFSShape, CUDAGraphBFSEntry, GraphBFSShapeHash> entries_;
    uint64_t clock_ = 0;
};

// ============================================================================
// Graph Shortest-Path — GraphSPShape, CUDAGraphSPEntry, CUDAGraphSPCache
//
// Captures Bellman-Ford relaxation: one init kernel + (numVertices-1) relax
// kernels, keyed on (numVertices, numPairs).
// ============================================================================

struct GraphSPShape {
    int numVertices = 0;
    int numPairs    = 0;

    bool operator==(const GraphSPShape& o) const noexcept {
        return numVertices == o.numVertices && numPairs == o.numPairs;
    }
};

struct GraphSPShapeHash {
    std::size_t operator()(const GraphSPShape& s) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        auto mix = [&](std::size_t v) { h ^= v; h *= 1099511628211ULL; };
        mix(static_cast<std::size_t>(s.numVertices));
        mix(static_cast<std::size_t>(s.numPairs));
        return h;
    }
};

struct CUDAGraphSPEntry {
    cudaGraph_t     graph = nullptr;
    cudaGraphExec_t exec  = nullptr;

    raii::CudaDeviceMemory d_adjacency;     ///< [numVertices × numVertices] uint32_t
    raii::CudaDeviceMemory d_weights;       ///< [numVertices × numVertices] float
    raii::CudaDeviceMemory d_startVertices; ///< [numPairs] uint32_t
    raii::CudaDeviceMemory d_distances;     ///< [numPairs × numVertices] float
    raii::CudaDeviceMemory d_predecessors;  ///< [numPairs × numVertices] int

    uint64_t lastAccess = 0;

    CUDAGraphSPEntry() = default;
    ~CUDAGraphSPEntry();

    CUDAGraphSPEntry(const CUDAGraphSPEntry&) = delete;
    CUDAGraphSPEntry& operator=(const CUDAGraphSPEntry&) = delete;

    CUDAGraphSPEntry(CUDAGraphSPEntry&&) noexcept;
    CUDAGraphSPEntry& operator=(CUDAGraphSPEntry&&) noexcept;
};

class CUDAGraphSPCache {
public:
    static constexpr size_t kMaxEntries = 16;

    /**
     * @brief Get.
     * @param[in] shape Input parameter.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    CUDAGraphSPEntry* get(const GraphSPShape& shape) noexcept;
    /**
     * @brief Put.
     * @param[in] shape Input parameter.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    CUDAGraphSPEntry& put(const GraphSPShape& shape, CUDAGraphSPEntry entry);
    size_t size() const noexcept { return entries_.size(); }
    /**
     * @brief Clear.
     */
    void clear();

private:
    /**
     * @brief Evict LRU.
     */
    void evictLRU();
    std::unordered_map<GraphSPShape, CUDAGraphSPEntry, GraphSPShapeHash> entries_;
    uint64_t clock_ = 0;
};

#endif // THEMIS_ENABLE_CUDA

// CUDA backend for GPU acceleration (NVIDIA)
// Uses RAII wrappers for automatic resource management and exception safety
class CUDAVectorBackend : public IVectorBackend {
public:
    CUDAVectorBackend() = default;
    ~CUDAVectorBackend() override;
    
    // IComputeBackend interface
    const char* name() const noexcept override { return "CUDA"; }
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

    // Frozen kernel dispatch — wires CUDA launchers to the interface contract
    ANNKernelDispatch populateANNDispatch() const override;

    /**
     * @brief Build Hnsw Ann Index.
     * @param[in] layers Input parameter.
     * @param[in] vectors Input parameter.
     * @param[in] numVectors Input parameter.
     * @param[in] dim Input parameter.
     * @return True when the operation succeeds.
     */
    bool buildHnswAnnIndex(const std::vector<HnswLayerGraph>& layers,
                           const float* vectors,
                           size_t numVectors,
                           uint32_t dim);

    // -------------------------------------------------------------------------
    // HNSW-based batch ANN search
    //
    // Requires buildHnswAnnIndex() to have been called first.  If no index is
    // built the method returns an empty vector.
    //
    // Parameters:
    //   queries    — Row-major float array [numQueries × dim].
    //   numQueries — Number of query vectors.
    //   k          — Nearest neighbours to return per query.
    //   ef         — Search-time ef override (0 = use index default).
    //
    // Returns one inner vector per query, sorted ascending by distance.
    // -------------------------------------------------------------------------
    std::vector<std::vector<std::pair<uint32_t, float>>> annBatchSearch(
        const float* queries,
        size_t numQueries,
        size_t k,
        uint32_t ef = 0);

    /**
     * @brief Is Hnsw Index Built.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isHnswIndexBuilt() const noexcept;

    /**
     * @brief Set Max Batch Size.
     * @param[in] n Input parameter.
     */
    void setMaxBatchSize(size_t n);

    size_t maxBatchSize() const noexcept { return maxBatchSize_; }

    // -------------------------------------------------------------------------
    // CUDA Graph-accelerated KNN search
    //
    // Identical semantics to batchKnnSearch() but caches a captured CUDA graph
    // keyed on {numQueries, numVectors, dim, k, metric}.  On the first call for
    // a given shape the kernel sequence is recorded; subsequent calls replay the
    // graph, eliminating per-kernel launch overhead.
    //
    // Use this method for recurring fixed-shape query batches.  For
    // variable-shaped batches use the standard batchKnnSearch() instead.
    // -------------------------------------------------------------------------
    std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearchWithGraph(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        DistanceMetric metric = DistanceMetric::L2
    );

#ifdef THEMIS_ENABLE_CUDA
    CUDAGraphCache& graphCache() noexcept { return graphCache_; }
    const CUDAGraphCache& graphCache() const noexcept { return graphCache_; }
#endif

private:
    bool   initialized_  = false;
    size_t maxBatchSize_ = 512;  ///< Max queries per HNSW kernel launch (pool size)

    // HNSW-based ANN engine — present in both CUDA and non-CUDA builds;
    // CudaHnswTraversalEngine transparently falls back to CPU when no GPU is
    // available.  Populated by buildHnswAnnIndex(); null until that call.
    std::unique_ptr<CudaHnswTraversalEngine> hnswEngine_;

#ifdef THEMIS_ENABLE_CUDA
    // RAII-managed CUDA resources (automatic cleanup)
    raii::CudaStream stream_;
    // Graph cache for recurring query workloads (mutex-protected)
    CUDAGraphCache graphCache_;
    std::mutex     graphCacheMutex_;
#else
    void* deviceContext_ = nullptr;  // Fallback for non-CUDA builds
#endif
};

class CUDAGraphBackend : public IGraphBackend {
public:
    CUDAGraphBackend() = default;
    ~CUDAGraphBackend() override;
    
    const char* name() const noexcept override { return "CUDA"; }
    BackendType type() const noexcept override { return BackendType::CUDA; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
    std::vector<std::vector<uint32_t>> batchBFS(
        const uint32_t* adjacency,
        size_t numVertices,
        const uint32_t* startVertices,
        size_t numStarts,
        uint32_t maxDepth
    ) override;
    
    std::vector<std::vector<uint32_t>> batchShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices,
        const uint32_t* startVertices,
        const uint32_t* endVertices,
        size_t numPairs
    ) override;

private:
    bool initialized_ = false;

#ifdef THEMIS_ENABLE_CUDA
    raii::CudaStream   stream_;
    CUDAGraphBFSCache  bfsCache_;
    CUDAGraphSPCache   spCache_;
    std::mutex         cacheMutex_;
#else
    void* deviceContext_ = nullptr;
#endif
};

class CUDAGeoBackend : public IGeoBackend {
public:
    CUDAGeoBackend() = default;
    ~CUDAGeoBackend() override;
    
    const char* name() const noexcept override { return "CUDA"; }
    BackendType type() const noexcept override { return BackendType::CUDA; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
    std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) override;
    
    std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) override;

    // Frozen kernel dispatch — wires CUDA geo launchers to the interface contract
    GeoKernelDispatch populateGeoDispatch() const override;

private:
    bool initialized_ = false;

#ifdef THEMIS_ENABLE_CUDA
    raii::CudaStream stream_;
#else
    void* deviceContext_ = nullptr;
#endif
};

// CUDA backend for FP16/BF16/FP32 matrix multiply with Tensor Core acceleration.
// Uses cuBLAS cublasHgemm (FP16) and cublasGemmEx (BF16) which automatically
// engage Tensor Core units on SM 7.0+ (FP16) and SM 8.0+ (BF16) hardware.
// Falls back to returning an error when CUDA is not available.
class CUDAMatrixBackend : public IMatrixBackend {
public:
    CUDAMatrixBackend() = default;
    ~CUDAMatrixBackend() override;

    const char* name() const noexcept override { return "CUDA"; }
    BackendType type() const noexcept override { return BackendType::CUDA; }
    bool isAvailable() const noexcept override;

    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;

    // IMatrixBackend interface
    int matmul(const MatrixKernelParams& params, void* opaque_stream = nullptr) override;

    // Frozen kernel dispatch — wires CUDA matmul launcher to the interface contract
    MatrixKernelDispatch populateMatrixDispatch() const override;

private:
    bool initialized_ = false;

#ifdef THEMIS_ENABLE_CUDA
    raii::CudaStream stream_;
#else
    void* deviceContext_ = nullptr;
#endif
};

} // namespace acceleration
} // namespace themis
