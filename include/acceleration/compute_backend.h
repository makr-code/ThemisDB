/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compute_backend.h                                  ║
  Version:         0.0.32                                             ║
  Last Modified:   2026-02-23 03:57:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     403                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • a629043ab  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
    • b2265b9b9  2026-02-21  feat(acceleration): Phase 3.3 — BackendHealthStatus + Vul... ║
    • bf5228e16  2026-02-21  feat(acceleration): add CapabilityRequirements, satisfies... ║
    • 4255551f1  2026-02-21  feat(acceleration): define backend capability contract wi... ║
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

// PCI vendor IDs for common GPU vendors — used by Vulkan and other backends
// for device selection and capability reporting.
namespace vendor_id {
    static constexpr uint32_t NVIDIA   = 0x10DE;
    static constexpr uint32_t AMD      = 0x1002;
    static constexpr uint32_t INTEL    = 0x8086;
    static constexpr uint32_t ARM      = 0x13B5;
    static constexpr uint32_t QUALCOMM = 0x5143;
    static constexpr uint32_t IMGTEC   = 0x1010;
} // namespace vendor_id

// Capability contract for a compute backend.
// Fields are grouped: operation support, precision matrix, metric matrix, device info.
struct BackendCapabilities {
    // Operation support
    bool supportsVectorOps = false;
    bool supportsGraphOps = false;
    bool supportsGeoOps = false;
    bool supportsMatrixOps = false;    ///< FP16/BF16 matrix multiply via Tensor Core
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
    // Vendor name for GPU/hardware identification (e.g. "NVIDIA", "AMD", "Intel", "ARM")
    // Empty string means unknown or CPU backend.
    std::string vendorName;
};

// Backend health status — returned by IComputeBackend::getHealthStatus()
struct BackendHealthStatus {
    // Overall health: "healthy" | "degraded" | "unhealthy"
    std::string status;

    // True when the backend has been successfully initialized and is ready
    // to accept work (liveness probe + readiness probe combined)
    bool healthy  = false;
    bool ready    = false;  // initialized and compute pipelines loaded
    bool alive    = false;  // backend process/driver is reachable

    // Human-readable description of the current state
    std::string message;

    // List of actionable issue descriptions (empty when healthy)
    std::vector<std::string> issues;

    // Device/driver information (populated when alive)
    std::string deviceName;
    std::string driverInfo;

    // Memory snapshot (0 when unavailable)
    size_t memoryUsedBytes      = 0;
    size_t memoryAvailableBytes = 0;

    // Convenience builder helpers
    static BackendHealthStatus makeHealthy(const std::string& device = "") {
        BackendHealthStatus s;
        s.status  = "healthy";
        s.healthy = s.ready = s.alive = true;
        s.message    = "Backend is operational";
        s.deviceName = device;
        return s;
    }

    static BackendHealthStatus makeDegraded(const std::string& issue) {
        BackendHealthStatus s;
        s.status  = "degraded";
        s.healthy = false;
        s.ready   = false;
        s.alive   = true;
        s.message = issue;
        s.issues.push_back(issue);
        return s;
    }

    static BackendHealthStatus makeUnhealthy(const std::string& issue) {
        BackendHealthStatus s;
        s.status  = "unhealthy";
        s.healthy = s.ready = s.alive = false;
        s.message = issue;
        s.issues.push_back(issue);
        return s;
    }
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

    // Health check (Phase 3.3)
    // Returns the current health and readiness status of this backend.
    // Default implementation derives status from isAvailable() + getLastError().
    virtual BackendHealthStatus getHealthStatus() const {
        if (!isAvailable()) {
            return BackendHealthStatus::makeUnhealthy(
                std::string(name()) + " is not available on this system");
        }
        const auto& err = lastError_;
        if (!err.isSuccess()) {
            return BackendHealthStatus::makeDegraded(
                std::string(name()) + " error: " + err.message);
        }
        return BackendHealthStatus::makeHealthy(
            getCapabilities().deviceName);
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

// Per-query result with deterministic ordering and partial-failure status.
// On success: neighbors is sorted ascending by distance (lower index breaks ties).
// On failure: neighbors is empty; status holds the error code; errorMessage describes
//             the failure (e.g. NaN in input vector, Inf in input vector).
struct KnnQueryResult {
    std::vector<std::pair<uint32_t, float>> neighbors;
    AccelerationErrorCode status   = AccelerationErrorCode::Success;
    std::string           errorMessage;
};

// Batch KNN result supporting partial failures.
// Queries that fail validation receive a non-Success status in queryResults[i].status
// while other queries that succeed return their neighbors normally.
struct PartialBatchResult {
    std::vector<KnnQueryResult> queryResults;
    size_t successCount = 0;
    size_t failureCount = 0;
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
    
    // Batch KNN search — results sorted ascending by distance.
    // Tie-breaking rule: when two candidates share the same distance the one
    // with the lower vector index is placed first (deterministic ordering).
    virtual std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) = 0;

    // Batch KNN search with per-query partial-failure handling.
    // Each query is validated before execution; queries whose input vectors
    // contain NaN or Inf values receive AccelerationErrorCode::InputRangeViolation
    // and an empty neighbors list, while the remaining valid queries are processed
    // normally.  This default implementation delegates to batchKnnSearch for valid
    // queries; backends may override for tighter integration.
    virtual PartialBatchResult batchKnnSearchSafe(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    );

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

// Matrix backend — FP16 / BF16 matrix multiply with Tensor Core acceleration.
// Backends that do not support Tensor Cores (e.g. CPUMatrixBackend) implement
// the FP32 path and declare MatrixPrecision::FP32 as their supported precision.
class IMatrixBackend : public IComputeBackend {
public:
    virtual ~IMatrixBackend() = default;

    /// Compute C = alpha * A × B + beta * C.
    /// A is [M × K], B is [K × N], C is [M × N] (row-major).
    /// Inputs/outputs are host pointers for CPU backends and device pointers
    /// for GPU backends.  @p precision selects the arithmetic type; the
    /// implementation is free to fall back to a wider type if unsupported.
    /// Returns 0 on success, non-zero on failure.
    virtual int matmul(const MatrixKernelParams& params, void* opaque_stream = nullptr) = 0;

    /// Populate the frozen kernel dispatch table for this backend.
    virtual MatrixKernelDispatch populateMatrixDispatch() const { return {}; }
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
    IMatrixBackend* getBestMatrixBackend() const;
    
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
        bool needsMatrixOps = false;        ///< Must support FP16/BF16 matrix multiply
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
        if (reqs.needsMatrixOps && !caps.supportsMatrixOps) return false;
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

    // ---------------------------------------------------------------------------
    // Runtime startup initialization
    // ---------------------------------------------------------------------------

    /// Perform capability-driven backend selection at runtime startup.
    ///
    /// Calls autoDetect() to discover all available backends, then runs
    /// selectVectorBackendFor(), selectGraphBackendFor(), and
    /// selectGeoBackendFor() with the provided requirements (defaulting to
    /// FP32 vector+ANN metrics when no requirements are specified).  The
    /// selected backends are cached and returned by the getSelected*Backend()
    /// accessors below.
    ///
    /// Safe to call multiple times; subsequent calls re-run detection and
    /// overwrite the cached selections.
    ///
    /// @param vectorReqs  Capability requirements for the vector backend.
    ///                    Defaults to { needsVectorOps=true, FP32, L2|COSINE|IP }.
    /// @param graphReqs   Capability requirements for the graph backend.
    ///                    Defaults to { needsGraphOps=true }.
    /// @param geoReqs     Capability requirements for the geo backend.
    ///                    Defaults to { needsGeoOps=true, FP32 }.
    void initializeRuntime(
        const CapabilityRequirements& vectorReqs = defaultVectorRequirements(),
        const CapabilityRequirements& graphReqs  = defaultGraphRequirements(),
        const CapabilityRequirements& geoReqs    = defaultGeoRequirements());

    /// Returns the vector backend selected by the last initializeRuntime() call,
    /// or nullptr if initializeRuntime() has not been called yet.
    IVectorBackend* getSelectedVectorBackend() const noexcept;

    /// Returns the graph backend selected by the last initializeRuntime() call,
    /// or nullptr if initializeRuntime() has not been called yet.
    IGraphBackend* getSelectedGraphBackend() const noexcept;

    /// Returns the geo backend selected by the last initializeRuntime() call,
    /// or nullptr if initializeRuntime() has not been called yet.
    IGeoBackend* getSelectedGeoBackend() const noexcept;

    /// Returns true if initializeRuntime() has been called at least once.
    bool isRuntimeInitialized() const noexcept;

    // Default capability requirements used by initializeRuntime() when the
    // caller does not supply explicit requirements.
    static CapabilityRequirements defaultVectorRequirements() noexcept;
    static CapabilityRequirements defaultGraphRequirements() noexcept;
    static CapabilityRequirements defaultGeoRequirements() noexcept;

    // Shutdown all backends
    void shutdownAll();
    
private:
    BackendRegistry();
    ~BackendRegistry();
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;
    
    std::vector<std::unique_ptr<IComputeBackend>> backends_;
    std::unique_ptr<PluginLoader> pluginLoader_;

    // Backends selected at the last initializeRuntime() call (nullptr until
    // initializeRuntime() has been called).
    IVectorBackend* selectedVectorBackend_ = nullptr;
    IGraphBackend*  selectedGraphBackend_  = nullptr;
    IGeoBackend*    selectedGeoBackend_    = nullptr;
    bool            runtimeInitialized_    = false;
};

} // namespace acceleration
} // namespace themis
