/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphics_backends.h                                ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:13:03                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2c7ea935e7  2026-03-14  fix(acceleration): address Vulkan compute shader pipeline... ║
    • f52f9b7eaa  2026-03-14  feat(acceleration): implement Vulkan compute shader pipel... ║
    • 9b3ffd6f0a  2026-03-11  feat(acceleration): implement DirectX 12 compute shader b... ║
    • f6207665d0  2026-03-11  feat(acceleration): Implement full OpenGL 4.3+ Compute Sh... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
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
    DirectXVectorBackend();
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
    class DirectXVectorBackendImpl;
    std::unique_ptr<DirectXVectorBackendImpl> impl_;
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

    // ---- Vulkan-specific introspection --------------------------------

    // Returns true when the selected physical device advertises
    // VK_KHR_buffer_device_address (required for advanced buffer aliasing
    // and bindless GPU pointer operations).  On Apple Silicon via MoltenVK
    // this may return false even if Vulkan is otherwise functional.
    // Only meaningful after a successful initialize().
    bool hasBufferDeviceAddress() const noexcept;

    // Tunable workgroup dimensions for SPIR-V specialization constants.
    // Must be called before initialize() to take effect.
    // Calls after initialize() are silently ignored; zero values are rejected.
    // setWorkgroupSizeBatchSearch() additionally rejects values > 256 because
    // batch_search.comp declares shared float sharedQuery[256].
    void setWorkgroupSizeL2(uint32_t wgX, uint32_t wgY) noexcept;
    void setWorkgroupSizeBatchSearch(uint32_t wgX) noexcept;

    // Inspect current (pending or baked) workgroup sizes for testing/debugging.
    // Returns {wgX, wgY} for the L2 pipeline; {batchX, 1} for batch-search.
    std::pair<uint32_t, uint32_t> getWorkgroupSizeL2() const noexcept;
    uint32_t getWorkgroupSizeBatchSearch() const noexcept;

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

// OpenGL Compute Shaders backend (OpenGL 4.3+ compute shader acceleration)
class OpenGLVectorBackend : public IVectorBackend {
public:
    OpenGLVectorBackend();
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
    class OpenGLVectorBackendImpl;
    std::unique_ptr<OpenGLVectorBackendImpl> impl_;
};

} // namespace acceleration
} // namespace themis
