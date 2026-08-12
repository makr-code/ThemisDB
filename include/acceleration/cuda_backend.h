/**
 * @file cuda_backend.h
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

/// Identifies a fixed-shape KNN workload for graph cache lookup.
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

/// FNV-1a–inspired hash for QueryShape.
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

/// Holds a captured CUDA graph and its pre-allocated device memory buffers for
/// one specific query shape.  Non-copyable; move-constructible.
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

/// Thread-safety: external mutex required — caller must hold the lock.
class CUDAGraphCache {
public:
    /// Maximum number of cached graphs before LRU eviction.
    static constexpr size_t kMaxEntries = 32;

    /// Returns a pointer to the entry matching @p shape, or nullptr.
    CUDAGraphEntry* get(const QueryShape& shape) noexcept;

    /// Inserts (or replaces) an entry for @p shape.
    /// Returns a reference to the stored entry.
    CUDAGraphEntry& put(const QueryShape& shape, CUDAGraphEntry entry);

    /// Number of currently cached graphs.
    size_t size() const noexcept { return entries_.size(); }

    /// Destroy all cached graphs and free device memory.
    void clear();

private:
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

/// Identifies a fixed-shape BFS workload for graph cache lookup.
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

/// FNV-1a–inspired hash for GraphBFSShape.
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

/// Captured CUDA graph entry for one fixed-shape BFS workload.  Non-copyable.
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

/// LRU cache of captured BFS graphs.  Thread-safety: external mutex required.
class CUDAGraphBFSCache {
public:
    static constexpr size_t kMaxEntries = 16;

    CUDAGraphBFSEntry* get(const GraphBFSShape& shape) noexcept;
    CUDAGraphBFSEntry& put(const GraphBFSShape& shape, CUDAGraphBFSEntry entry);
    size_t size() const noexcept { return entries_.size(); }
    void clear();

private:
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

/// Identifies a fixed-shape Bellman-Ford SP workload for graph cache lookup.
struct GraphSPShape {
    int numVertices = 0;
    int numPairs    = 0;

    bool operator==(const GraphSPShape& o) const noexcept {
        return numVertices == o.numVertices && numPairs == o.numPairs;
    }
};

/// FNV-1a–inspired hash for GraphSPShape.
struct GraphSPShapeHash {
    std::size_t operator()(const GraphSPShape& s) const noexcept {
        std::size_t h = 14695981039346656037ULL;
        auto mix = [&](std::size_t v) { h ^= v; h *= 1099511628211ULL; };
        mix(static_cast<std::size_t>(s.numVertices));
        mix(static_cast<std::size_t>(s.numPairs));
        return h;
    }
};

/// Captured CUDA graph entry for one fixed-shape SP workload.  Non-copyable.
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

/// LRU cache of captured Bellman-Ford graphs.  Thread-safety: external mutex required.
class CUDAGraphSPCache {
public:
    static constexpr size_t kMaxEntries = 16;

    CUDAGraphSPEntry* get(const GraphSPShape& shape) noexcept;
    CUDAGraphSPEntry& put(const GraphSPShape& shape, CUDAGraphSPEntry entry);
    size_t size() const noexcept { return entries_.size(); }
    void clear();

private:
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

    // -------------------------------------------------------------------------
    // HNSW Graph-based ANN index management
    //
    // buildHnswAnnIndex() uploads a pre-built multi-layer HNSW graph and the
    // associated flat vector store to the GPU (or falls back to CPU if no CUDA
    // device is available).  Once built, subsequent calls to batchKnnSearch()
    // and annBatchSearch() use the HNSW traversal path instead of the brute-
    // force flat-search kernel.
    //
    // Parameters:
    //   layers     — Multi-layer HNSW graph in CSR format (index 0 = bottom).
    //   vectors    — Row-major flat float array [numVectors × dim].
    //   numVectors — Number of vectors indexed.
    //   dim        — Vector dimensionality.
    //
    // Returns true on success; false if the engine could not upload the data
    // (the backend remains usable in brute-force fallback mode).
    // -------------------------------------------------------------------------
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

    /** True when buildHnswAnnIndex() has been called successfully. */
    bool isHnswIndexBuilt() const noexcept;

    // -------------------------------------------------------------------------
    // Visited bitset pool tuning
    //
    // setMaxBatchSize() controls the size of the persistent visited bitset
    // pool allocated in the HNSW engine during buildHnswAnnIndex().  The pool
    // is sized as maxBatchSize × ceil(numNodes / 8) bytes and lives for the
    // lifetime of the index.  Calling setMaxBatchSize() before
    // buildHnswAnnIndex() is the recommended usage pattern; calling it after
    // the index has been built has no effect until the next buildHnswAnnIndex().
    //
    // Default: 512 queries.
    //
    // Pool allocation must not exceed BackendCapabilities::maxMemoryBytes.
    // If the computed pool size would exceed that limit, the effective
    // maxBatchSize is clamped automatically during buildHnswAnnIndex().
    // -------------------------------------------------------------------------
    void setMaxBatchSize(size_t n);

    /** Return the current maxBatchSize setting (default: 512). */
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
    /// Direct access to the graph cache — exposed for testing.
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
