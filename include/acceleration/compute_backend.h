/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compute_backend.h                                  ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     334                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bf5228e16  2026-02-21  feat(acceleration): add CapabilityRequirements, satisfies... ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

// Floating-point and quantisation precision modes.
// Values are stable bitmask flags; combine with bitwise OR.
enum class PrecisionMode : uint32_t {
    NONE = 0,
    FP32 = 1u << 0,   ///< 32-bit IEEE 754 single precision (always required)
    FP16 = 1u << 1,   ///< 16-bit IEEE 754 half precision
    BF16 = 1u << 2,   ///< bfloat16
    INT8 = 1u << 3,   ///< 8-bit integer quantisation
};

inline constexpr PrecisionMode operator|(PrecisionMode a, PrecisionMode b) noexcept {
    return static_cast<PrecisionMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr bool hasPrecision(PrecisionMode modes, PrecisionMode flag) noexcept {
    return (static_cast<uint32_t>(modes) & static_cast<uint32_t>(flag)) != 0;
}

/// Returns the bitmask bit position for a DistanceMetric value.
/// Bit i is set when DistanceMetric(i) is supported.
inline constexpr uint32_t metricBit(DistanceMetric m) noexcept {
    return 1u << static_cast<uint32_t>(m);
}

// Capability contract for a compute backend.
// Fields are grouped: operation support, precision matrix, metric matrix, device info.
struct BackendCapabilities {
    // Operation support
    bool supportsVectorOps = false;
    bool supportsGraphOps = false;
    bool supportsGeoOps = false;
    bool supportsBatchProcessing = false;
    bool supportsAsync = false;

    // Precision feature matrix: OR of PrecisionMode flags.
    // Must include at least PrecisionMode::FP32 for any vector or geo backend.
    PrecisionMode supportedPrecisions = PrecisionMode::NONE;

    // Distance-metric feature matrix: bitmask using metricBit(DistanceMetric).
    // Set bit i if the backend supports DistanceMetric(i) in its ANN dispatch.
    uint32_t supportedMetrics = 0;

    // Device info
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

    // Returns the ordered fallback chain used when selecting the best backend.
    // The first element has the highest priority; BackendType::CPU is always last.
    // All getBestXBackend() methods traverse this chain in order.
    static const std::vector<BackendType>& getFallbackOrder() noexcept;

    // ---------------------------------------------------------------------------
    // Capability-driven selection
    // ---------------------------------------------------------------------------

    /// Minimum capability requirements for capability-driven backend selection.
    /// Zero / NONE / false fields are "don't-care" — they impose no constraint.
    struct CapabilityRequirements {
        bool needsVectorOps = false;        ///< Must support vector (ANN) operations
        bool needsGraphOps  = false;        ///< Must support graph traversal operations
        bool needsGeoOps    = false;        ///< Must support geospatial operations
        bool needsBatch     = false;        ///< Must support batch processing
        bool needsAsync     = false;        ///< Must support asynchronous execution

        /// All listed PrecisionMode flags must be present in supportedPrecisions.
        PrecisionMode requiredPrecisions = PrecisionMode::NONE;

        /// All listed DistanceMetric bits (via metricBit()) must be present in
        /// supportedMetrics.
        uint32_t requiredMetrics = 0;
    };

    /// Returns true if @p caps satisfies every field in @p reqs.
    static inline bool satisfies(const BackendCapabilities& caps,
                                  const CapabilityRequirements& reqs) noexcept {
        if (reqs.needsVectorOps && !caps.supportsVectorOps) return false;
        if (reqs.needsGraphOps  && !caps.supportsGraphOps)  return false;
        if (reqs.needsGeoOps    && !caps.supportsGeoOps)    return false;
        if (reqs.needsBatch     && !caps.supportsBatchProcessing) return false;
        if (reqs.needsAsync     && !caps.supportsAsync)     return false;
        const auto reqP = static_cast<uint32_t>(reqs.requiredPrecisions);
        const auto hasP = static_cast<uint32_t>(caps.supportedPrecisions);
        if ((reqP & hasP) != reqP) return false;
        if ((reqs.requiredMetrics & caps.supportedMetrics) != reqs.requiredMetrics) return false;
        return true;
    }

    /// Returns the highest-priority available backend (per getFallbackOrder())
    /// whose capabilities satisfy @p reqs, or nullptr if none do.
    IComputeBackend* selectBackendFor(const CapabilityRequirements& reqs) const;

    /// Like selectBackendFor() but restricted to IVectorBackend instances.
    IVectorBackend* selectVectorBackendFor(const CapabilityRequirements& reqs) const;

    /// Like selectBackendFor() but restricted to IGraphBackend instances.
    IGraphBackend* selectGraphBackendFor(const CapabilityRequirements& reqs) const;

    /// Like selectBackendFor() but restricted to IGeoBackend instances.
    IGeoBackend* selectGeoBackendFor(const CapabilityRequirements& reqs) const;
    
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
