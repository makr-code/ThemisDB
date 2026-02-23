/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_backend.h                                     ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     158                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/compute_backend.h"

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
    bool initialized_ = false;

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
    void* deviceContext_ = nullptr;
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
    void* deviceContext_ = nullptr;
};

} // namespace acceleration
} // namespace themis
