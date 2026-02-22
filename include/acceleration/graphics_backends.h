/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            graphics_backends.h                                ║
  Version:         0.0.24                                             ║
  Last Modified:   2026-02-22 08:12:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     155                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 00c723d27  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b2265b9b9  2026-02-21  feat(acceleration): Phase 3.3 — BackendHealthStatus + Vul... ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

private:
    bool initialized_ = false;
    class VulkanVectorBackendImpl;
    std::unique_ptr<VulkanVectorBackendImpl> impl_;
    metrics::BackendMetrics metrics_{"vulkan"};
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
