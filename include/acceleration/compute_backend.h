/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compute_backend.h                                  ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-02-21 16:52:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     234                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 234245ceb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <cstdint>
#include "acceleration/error_context.h"
#include "acceleration/kernel_invocation.h"

namespace themis {
namespace acceleration {

// Backend types for hardware acceleration
enum class BackendType {
    CPU,        // CPU-only (fallback)
    CUDA,       // NVIDIA CUDA
    ZLUDA,      // AMD ZLUDA (CUDA compatibility for AMD GPUs)
    HIP,        // AMD HIP (Heterogeneous-computing Interface for Portability)
    ROCM,       // AMD ROCm
    DIRECTX,    // DirectX Compute Shaders (Windows)
    VULKAN,     // Vulkan Compute (cross-platform)
    OPENGL,     // OpenGL Compute Shaders (legacy support)
    METAL,      // Apple Metal
    ONEAPI,     // Intel OneAPI/SYCL (cross-platform)
    OPENCL,     // OpenCL (generic)
    WEBGPU,     // WebGPU (browser-based, future)
    AUTO        // Auto-detect best available
};

// Acceleration capabilities
struct BackendCapabilities {
    bool supportsVectorOps = false;
    bool supportsGraphOps = false;
    bool supportsGeoOps = false;
    bool supportsBatchProcessing = false;
    bool supportsAsync = false;
    size_t maxMemoryBytes = 0;      // Available VRAM/memory
    int computeUnits = 0;            // Number of compute units/SMs
    std::string deviceName;
};

// Base interface for compute backends
class IComputeBackend {
public:
    virtual ~IComputeBackend() = default;
    
    // Backend identification
    virtual const char* name() const noexcept = 0;
    virtual BackendType type() const noexcept = 0;
    virtual bool isAvailable() const noexcept = 0;
    
    // Capabilities
    virtual BackendCapabilities getCapabilities() const = 0;
    
    // Lifecycle
    virtual bool initialize() = 0;
    virtual void shutdown() = 0;
    
    // Error handling (Phase 2.2b)
    // Get the last error that occurred in this backend
    // Returns error context with details, code, and troubleshooting hint
    virtual ErrorContext getLastError() const {
        return lastError_;
    }
    
protected:
    // Helper for backends to set error context
    // Stores error and optionally logs it
    void setError(ErrorContext error) {
        lastError_ = std::move(error);
    }
    
    // Helper to clear error state (on success)
    void clearError() {
        lastError_ = ErrorContext(
            AccelerationErrorCode::Success,
            name(),
            ""
        );
    }
    
    // Last error context (stored for programmatic access)
    ErrorContext lastError_;
};

// Vector operations backend interface
class IVectorBackend : public IComputeBackend {
public:
    virtual ~IVectorBackend() = default;
    
    // Distance computation
    virtual std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) = 0;
    
    // Batch KNN search
    virtual std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) = 0;

    // Populate the frozen kernel dispatch table for this backend.
    // Backends override this to expose their kernel function pointers.
    // Null entries in the returned table indicate unsupported operations.
    virtual ANNKernelDispatch populateANNDispatch() const { return {}; }
};

// Graph operations backend interface
class IGraphBackend : public IComputeBackend {
public:
    virtual ~IGraphBackend() = default;
    
    // Batch BFS traversal
    virtual std::vector<std::vector<uint32_t>> batchBFS(
        const uint32_t* adjacency,
        size_t numVertices,
        const uint32_t* startVertices,
        size_t numStarts,
        uint32_t maxDepth
    ) = 0;
    
    // Batch shortest path
    virtual std::vector<std::vector<uint32_t>> batchShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices,
        const uint32_t* startVertices,
        const uint32_t* endVertices,
        size_t numPairs
    ) = 0;
};

// Geo operations backend interface (extends existing spatial backend concept)
class IGeoBackend : public IComputeBackend {
public:
    virtual ~IGeoBackend() = default;
    
    // Batch distance calculations
    virtual std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) = 0;
    
    // Batch point-in-polygon tests
    virtual std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) = 0;

    // Populate the frozen kernel dispatch table for this backend.
    // Backends override this to expose their kernel function pointers.
    // Null entries in the returned table indicate unsupported operations.
    virtual GeoKernelDispatch populateGeoDispatch() const { return {}; }
};

// Forward declaration
class PluginLoader;

// Backend registry for managing different acceleration backends
class BackendRegistry {
public:
    static BackendRegistry& instance();
    
    // Register a backend (manual registration)
    void registerBackend(std::unique_ptr<IComputeBackend> backend);
    
    // Load plugins from directory (DLL/SO files)
    // Returns number of plugins loaded
    size_t loadPlugins(const std::string& pluginDirectory);
    
    // Load a specific plugin
    bool loadPlugin(const std::string& pluginPath);
    
    // Get backend by type
    IComputeBackend* getBackend(BackendType type) const;
    
    // Get best available backend for a capability
    IVectorBackend* getBestVectorBackend() const;
    IGraphBackend* getBestGraphBackend() const;
    IGeoBackend* getBestGeoBackend() const;
    
    // Auto-detect and initialize all available backends
    void autoDetect();
    
    // List all available backends
    std::vector<BackendType> getAvailableBackends() const;
    
    // Shutdown all backends
    void shutdownAll();
    
private:
    BackendRegistry();
    ~BackendRegistry();
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;
    
    std::vector<std::unique_ptr<IComputeBackend>> backends_;
    std::unique_ptr<PluginLoader> pluginLoader_;
};

} // namespace acceleration
} // namespace themis
