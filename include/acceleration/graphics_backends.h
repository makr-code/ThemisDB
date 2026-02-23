/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphics_backends.h                                ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     151                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • b2265b9b9  2026-02-21  feat(acceleration): Phase 3.3 — BackendHealthStatus + Vul... ║
    • c2782e741  2026-02-21  feat(acceleration): Phase 3.1 — Vulkan backend metrics in... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "acceleration/compute_backend.h"
#include "acceleration/metrics/backend_metrics.h"
#include <memory>

namespace themis {
namespace acceleration {

// DirectX 12 Compute Shaders backend (Windows only)
class DirectXVectorBackend : public IVectorBackend {
public:
    DirectXVectorBackend() = default;
    ~DirectXVectorBackend() override;
    
    const char* name() const noexcept override { return "DirectX"; }
    BackendType type() const noexcept override { return BackendType::DIRECTX; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
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
    void* device_ = nullptr;  // ID3D12Device*
    void* commandQueue_ = nullptr;  // ID3D12CommandQueue*
};

// Vulkan Compute backend (cross-platform)
class VulkanVectorBackend : public IVectorBackend {
public:
    VulkanVectorBackend();
    ~VulkanVectorBackend() override;
    
    const char* name() const noexcept override { return "Vulkan"; }
    BackendType type() const noexcept override { return BackendType::VULKAN; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    BackendHealthStatus getHealthStatus() const override;
    
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

    // Frozen kernel dispatch table — L2, cosine, inner-product, and top-K
    ANNKernelDispatch populateANNDispatch() const override;

private:
    bool initialized_ = false;
    class VulkanVectorBackendImpl;
    std::unique_ptr<VulkanVectorBackendImpl> impl_;
    metrics::BackendMetrics metrics_{"vulkan"};
};

// Vulkan Geospatial Compute backend (cross-platform)
// Implements the IGeoBackend interface using Vulkan compute shaders for
// Haversine distance and point-in-polygon operations, providing the same
// geospatial compute capabilities as the CUDA geo backend.
class VulkanGeoBackend : public IGeoBackend {
public:
    VulkanGeoBackend();
    ~VulkanGeoBackend() override;

    const char* name() const noexcept override { return "VulkanGeo"; }
    BackendType type() const noexcept override { return BackendType::VULKAN; }
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

    // Frozen kernel dispatch table — haversine distance and point-in-polygon
    GeoKernelDispatch populateGeoDispatch() const override;

private:
    bool initialized_ = false;
};

// OpenGL Compute Shaders backend (legacy support)
class OpenGLVectorBackend : public IVectorBackend {
public:
    OpenGLVectorBackend() = default;
    ~OpenGLVectorBackend() override;
    
    const char* name() const noexcept override { return "OpenGL"; }
    BackendType type() const noexcept override { return BackendType::OPENGL; }
    bool isAvailable() const noexcept override;
    
    BackendCapabilities getCapabilities() const override;
    bool initialize() override;
    void shutdown() override;
    
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
    void* context_ = nullptr;  // OpenGL context
};

} // namespace acceleration
} // namespace themis
