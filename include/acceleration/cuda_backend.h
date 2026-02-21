/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_backend.h                                     ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:57:00                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     156                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • d3d236ab7  2026-02-20  Production hardening: Consistency, RAII resource manageme... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/compute_backend.h"

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/raii/cuda_raii.h"
#endif

namespace themis {
namespace acceleration {

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

private:
    bool initialized_ = false;
    
#ifdef THEMIS_ENABLE_CUDA
    // RAII-managed CUDA resources (automatic cleanup)
    raii::CudaStream stream_;
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

private:
    bool initialized_ = false;
    void* deviceContext_ = nullptr;
};

} // namespace acceleration
} // namespace themis
