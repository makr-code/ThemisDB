/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            compute_backend.h                                  ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:19:42                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     1137                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b5921c3ced  2026-04-13  feat(acceleration): BackendRegistry O(n²) → O(k) backend ... ║
    • 040083b025  2026-04-12  feat: StreamingIngestManager, TsStreamCursor, LZ4 compres... ║
    • 3b792a6ae0  2026-03-20  Refactor saga orchestrator, add compute types ║
    • fe44926901  2026-03-19  Changes before error encountered        ║
    • e627c556bd  2026-03-15  feat(acceleration): BackendRegistry thread-safety, VLLMRe... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <memory>
#include <cstdint>
#include <atomic>
#include <algorithm>
#include <shared_mutex>
#include "acceleration/error_context.h"
#include "acceleration/kernel_invocation.h"
#include "acceleration/compute_future.h"

namespace themis {
namespace acceleration {

// =============================================================================
// Backend contract version
//
// Monotonically increasing integer encoding major*100 + minor.
// Callers may compare this at runtime to detect mismatched shared libraries.
// This value is bumped ONLY on breaking changes to the public backend API
// (IComputeBackend, IVectorBackend, IGeoBackend, IGraphBackend, IMatrixBackend,
// BackendCapabilities, BackendHealthStatus, PartialBatchResult, KnnQueryResult,
// and BackendRegistry::CapabilityRequirements).
//
// Compatibility guarantees:
//  - Additive changes (new fields, new enum values) increment the minor part.
//  - Breaking changes (removed/renamed symbols, changed signatures) increment
//    the major part and invalidate binary compatibility.
//  - BACKEND_CONTRACT_VERSION and KERNEL_INVOCATION_INTERFACE_VERSION must be
//    queried together; a shared library is compatible only when both match.
// =============================================================================
inline constexpr uint32_t BACKEND_CONTRACT_VERSION = 100; // v1.0

// Backend types for hardware acceleration
enum class BackendType {
    CPU,            // CPU-only (fallback)
    CUDA,           // NVIDIA CUDA
    ZLUDA,          // AMD ZLUDA (CUDA compatibility for AMD GPUs)
    HIP,            // AMD HIP (Heterogeneous-computing Interface for Portability)
    ROCM,           // AMD ROCm
    DIRECTX,        // DirectX Compute Shaders (Windows)
    VULKAN,         // Vulkan Compute (cross-platform)
    OPENGL,         // OpenGL Compute Shaders (legacy support)
    METAL,          // Apple Metal (GPU)
    ONEAPI,         // Intel OneAPI/SYCL (cross-platform)
    OPENCL,         // OpenCL (generic)
    WEBGPU,         // WebGPU (browser-based, future)
    MULTI_GPU,      // Multi-GPU sharding (distributes across N devices)
    // ── AI-specific accelerator backends ─────────────────────────────────────
    // Dedicated AI/inference hardware with dedicated low-power neural engines.
    // All AI backends expose graceful CPU fallback via AiHardwareDispatcher.
    NPU_APPLE,      // Apple Neural Engine (Core ML / Metal Performance Shaders)
    NPU_INTEL,      // Intel NPU (OpenVINO / iGPU tile)
    NPU_QUALCOMM,   // Qualcomm QNN / Hexagon DSP / Snapdragon NPU
    NPU_ARM,        // ARM Ethos-N / Mali AI extensions
    NNAPI,          // Android Neural Networks API (delegates to best available)
    ONNX_RUNTIME,   // ONNX Runtime (universal AI inference, selects EP at runtime)
    AUTO            // Auto-detect best available
};

// Floating-point and quantisation precision modes.
// Values are stable bitmask flags; combine with bitwise OR.
enum class PrecisionMode : uint32_t {
    NONE  = 0,
    FP32  = 1u << 0,   ///< 32-bit IEEE 754 single precision (always required)
    FP16  = 1u << 1,   ///< 16-bit IEEE 754 half precision
    BF16  = 1u << 2,   ///< bfloat16
    INT8  = 1u << 3,   ///< 8-bit integer quantisation (symmetric / asymmetric)
    // ── AI / LLM quantisation modes ─────────────────────────────────────────
    // Used by NPU and dedicated AI inference engines.
    INT4  = 1u << 4,   ///< 4-bit integer (GPTQ / AWQ / NF4 schemes)
    FP4   = 1u << 5,   ///< 4-bit float (e.g. NF4, FP4-E2M1)
    W4A8  = 1u << 6,   ///< 4-bit weights, 8-bit activations (Qualcomm AI Engine)
    W8A8  = 1u << 7,   ///< 8-bit weights and 8-bit activations (symmetric INT8)
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

/**
 * @brief Per-device capability snapshot produced by DeviceManager::probeDevices().
 *
 * Fields are translated from themis::gpu::DeviceInfo and augmented with
 * acceleration-specific precision support flags derived from the device's
 * compute capability.  CPU-fallback sentinels use index == -1.
 */
struct DeviceCapabilityInfo {
    int         index             = -1;      ///< Driver device index (-1 for CPU fallback)
    std::string name;                        ///< Human-readable device name
    BackendType backend_type      = BackendType::CPU;
    uint64_t    total_vram_bytes  = 0;       ///< Total VRAM reported by driver
    uint64_t    free_vram_bytes   = 0;       ///< Free VRAM at probe time
    int         compute_major     = 0;       ///< Compute capability major (CUDA/ROCm)
    int         compute_minor     = 0;       ///< Compute capability minor
    bool        is_healthy        = true;    ///< false when the device reported an error
    std::string error_message;              ///< Non-empty when is_healthy == false

    // Derived precision support flags (GPU / CPU backends)
    bool        supports_fp16     = false;   ///< true for CUDA sm_70+ / ROCm gfx900+
    bool        supports_bf16     = false;   ///< true for CUDA sm_80+ (Ampere and newer)

    // ── AI / NPU-specific fields ──────────────────────────────────────────────
    bool        is_npu            = false;   ///< true when this is a dedicated neural engine
    uint32_t    npu_tops          = 0;       ///< Reported NPU peak throughput in TOPS (0 = unknown)
    bool        supports_int4     = false;   ///< 4-bit inference (GPTQ/AWQ/NF4)
    bool        supports_w4a8     = false;   ///< W4A8 mixed-precision (Qualcomm AI Engine, etc.)
    std::string onnx_ep;                     ///< Preferred ONNX Runtime execution provider name
                                             ///<  ("CUDAExecutionProvider", "CoreMLExecutionProvider",
                                             ///<   "QNNExecutionProvider", "OpenVINOExecutionProvider",
                                             ///<   "CPUExecutionProvider" …)
};

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
    bool supportsAiInference = false;  ///< Dedicated AI inference path (NPU / ONNX Runtime)

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

    // ── AI / NPU-specific ────────────────────────────────────────────────────
    uint32_t    npuTops           = 0;    ///< Reported peak throughput in TOPS (0 = unknown)
    std::string preferredOnnxEP;          ///< ONNX Runtime execution provider (empty = CPU)
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

/**
 * @brief Result returned by `IComputeBackend::submitSimilarityKernel()`.
 *
 * FP tolerance guarantee: results produced by hardware-accelerated paths must
 * agree with the CPU baseline within <= 1e-6 relative error for FP32 inputs.
 */
struct SimilarityKernelResult {
    // Per-query top-k results as (corpus_id, distance) pairs, sorted ascending
    // by distance. Outer index = query index; inner index = rank.
    std::vector<std::vector<std::pair<uint32_t, float>>> results;

    DistanceMetric metric_used    = DistanceMetric::L2;
    PrecisionMode  precision_used = PrecisionMode::FP32;
    bool           used_hw_path   = false;
    double         speedup_vs_cpu = 1.0;
};

// Input descriptor for a batched similarity kernel invocation.
struct BatchDescriptor {
    const float* queries     = nullptr;
    size_t       num_queries = 0;
    size_t       dim         = 0;
    const float* vectors     = nullptr;
    size_t       num_vectors = 0;
    size_t       k           = 1;
};

// Plain-data runtime configuration for a compute kernel dispatch.
struct KernelConfig {
    uint32_t       block_size = 256;
    uint32_t       grid_size  = 0;
    uint32_t       shared_mem = 0;
    DistanceMetric metric     = DistanceMetric::L2;
    PrecisionMode  precision  = PrecisionMode::FP32;
    bool           async_exec = false;
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

    // -------------------------------------------------------------------------
    // submitSimilarityKernel() — async hardware-accelerated similarity search.
    //
    // Dispatches a batched vector similarity search to the fastest available
    // execution path.  The default implementation runs the search synchronously
    // on the CPU and returns an already-fulfilled ComputeFuture so that callers
    // can use a uniform async API regardless of backend type.
    //
    // GPU backends (CUDA, Vulkan, HIP, …) should override this method to
    // dispatch to their respective device kernels.
    //
    // FP tolerance guarantee: hardware paths must agree with the CPU baseline
    // within ≤ 1e-6 relative error for FP32 inputs.
    //
    // Parameters:
    //   batch  — Input descriptor: query/corpus pointers, sizes, and k.
    //   config — Kernel execution configuration: metric, precision, block size.
    //   token  — Optional cancellation token (default-constructed = no cancel).
    //
    // Returns a ComputeFuture<SimilarityKernelResult> that will be ready once
    // the kernel completes.  The default implementation sets the future ready
    // immediately (synchronous CPU fallback).
    // -------------------------------------------------------------------------
    virtual ComputeFuture<SimilarityKernelResult>
    submitSimilarityKernel(const BatchDescriptor& batch,
                           [[maybe_unused]] const KernelConfig&    config,
                           CancellationToken       token = {}) {
        // Default CPU fallback: brute-force L2 / cosine / inner-product search.
        SimilarityKernelResult result;
        result.metric_used    = config.metric;
        result.precision_used = config.precision;
        result.used_hw_path   = false;
        result.speedup_vs_cpu = 1.0;

        if (batch.queries && batch.vectors && batch.num_queries > 0
                && batch.num_vectors > 0 && batch.dim > 0 && batch.k > 0) {
            result.results.resize(batch.num_queries);
            for (size_t qi = 0; qi < batch.num_queries; ++qi) {
                if (token.is_cancelled()) break;
                const float* q = batch.queries + qi * batch.dim;
                std::vector<std::pair<uint32_t, float>> row;
                row.reserve(batch.num_vectors);
                for (size_t vi = 0; vi < batch.num_vectors; ++vi) {
                    const float* v = batch.vectors + vi * batch.dim;
                    float dist = 0.0f;
                    if (config.metric == DistanceMetric::L2 ||
                        config.metric == DistanceMetric::COSINE) {
                        for (size_t d = 0; d < batch.dim; ++d) {
                            float diff = q[d] - v[d];
                            dist += diff * diff;
                        }
                    } else {
                        // Inner-product (negative dot product for min-heap)
                        for (size_t d = 0; d < batch.dim; ++d) {
                            dist -= q[d] * v[d];
                        }
                    }
                    row.emplace_back(static_cast<uint32_t>(vi), dist);
                }
                const size_t k = std::min(batch.k, row.size());
                std::partial_sort(row.begin(),
                                  row.begin() + static_cast<ptrdiff_t>(k),
                                  row.end(),
                                  [](const std::pair<uint32_t, float>& a,
                                     const std::pair<uint32_t, float>& b) {
                                      return a.second < b.second;
                                  });
                row.resize(k);
                result.results[qi] = std::move(row);
            }
        }

        return ComputeFuture<SimilarityKernelResult>::make_ready(
            std::move(result));
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

/// Per-type backend aggregation stored in BackendRegistry::typeIndex_.
/// One instance exists per distinct BackendType in the registry.  The typed
/// interface pointer fields are set once by registerBackend() (one
/// dynamic_cast per interface per registration) and are then used in the hot
/// query path without further dynamic_cast calls.
struct RegisteredBackend {
    IComputeBackend* base      = nullptr;  ///< First registered backend of this type
    IVectorBackend*  vectorPtr = nullptr;  ///< First IVectorBackend of this type, or nullptr
    IGraphBackend*   graphPtr  = nullptr;  ///< First IGraphBackend of this type, or nullptr
    IGeoBackend*     geoPtr    = nullptr;  ///< First IGeoBackend of this type, or nullptr
    IMatrixBackend*  matrixPtr = nullptr;  ///< First IMatrixBackend of this type, or nullptr
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

    /// Like selectBackendFor() but restricted to IMatrixBackend instances.
    IMatrixBackend* selectMatrixBackendFor(const CapabilityRequirements& reqs) const;

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

    /// Return the list of compute devices probed at the last initializeRuntime()
    /// call.  Each entry contains the device name, BackendType, VRAM, compute
    /// capability, and derived precision support flags.  Returns an empty vector
    /// if initializeRuntime() has not been called yet.
    ///
    /// The returned snapshot is immutable; call initializeRuntime() again to
    /// refresh the device list.
    std::vector<DeviceCapabilityInfo> deviceInfo() const noexcept;

    // Shutdown all backends
    void shutdownAll();
    
private:
    BackendRegistry();
    ~BackendRegistry();
    BackendRegistry(const BackendRegistry&) = delete;
    BackendRegistry& operator=(const BackendRegistry&) = delete;
    
    // Protects all mutable state below.
    // Read-only operations (getBackend, getBestXBackend, selectXBackendFor,
    // getAvailableBackends, deviceInfo) acquire a shared lock.
    // Write operations (registerBackend, shutdownAll, initializeRuntime,
    // autoDetect, loadPlugins, loadPlugin) acquire an exclusive lock.
    mutable std::shared_mutex registryMutex_;

    std::vector<std::unique_ptr<IComputeBackend>> backends_;
    std::unordered_map<BackendType, RegisteredBackend> typeIndex_;
    std::unique_ptr<PluginLoader> pluginLoader_;

    // Backends selected at the last initializeRuntime() call (nullptr until
    // initializeRuntime() has been called).
    IVectorBackend* selectedVectorBackend_ = nullptr;
    IGraphBackend*  selectedGraphBackend_  = nullptr;
    IGeoBackend*    selectedGeoBackend_    = nullptr;
    std::atomic<bool> runtimeInitialized_{false};

    // Device info snapshot captured at the last initializeRuntime() call.
    std::vector<DeviceCapabilityInfo> cachedDeviceInfo_;
};

// =============================================================================
// DeviceCapabilityFlags — strongly-typed bitmask for per-device features
// =============================================================================

/**
 * @brief Strongly-typed bitmask of hardware capability flags.
 *
 * Each flag corresponds to a discrete hardware feature that may or may not be
 * present on a given GPU.  Combine flags with bitwise OR; test with
 * `hasCapability()`.
 *
 * ## Validity
 * The known-valid mask is `DeviceCapabilityFlags::KNOWN_VALID_MASK`.  Bitmasks
 * that have bits set outside of this mask are rejected by
 * `IDeviceCapabilityQuery::queryCapabilities()` (returns `NONE` on error).
 */
enum class DeviceCapabilityFlags : uint32_t {
    NONE                 = 0,
    FLOAT32              = 1u << 0,  ///< IEEE 754 single-precision (always present on FP-capable devices)
    FLOAT16              = 1u << 1,  ///< IEEE 754 half-precision compute (native fp16)
    BFLOAT16             = 1u << 2,  ///< Brain float BF16 arithmetic
    INT8                 = 1u << 3,  ///< 8-bit integer arithmetic (including VNNI)
    TENSOR_CORES         = 1u << 4,  ///< Tensor Core acceleration (sm_70+, RDNA3+)
    WARP_PRIMITIVES      = 1u << 5,  ///< Warp shuffle / ballot / vote intrinsics
    DYNAMIC_PARALLELISM  = 1u << 6,  ///< CUDA dynamic parallelism (sm_35+)
    UNIFIED_MEMORY       = 1u << 7,  ///< CUDA/HIP unified virtual address space
    PEER_ACCESS          = 1u << 8,  ///< Device-to-device peer memory access (NVLink / PCIe BAR)
    COMPUTE_PREEMPTION   = 1u << 9,  ///< Fine-grained thread-level compute preemption
    COOPERATIVE_GROUPS   = 1u << 10, ///< CUDA cooperative group launches
    GRAPH_CAPTURE        = 1u << 11, ///< CUDA/HIP graph capture and replay

    /// Mask of all valid capability bits.  Queries that return flags with bits
    /// outside this mask are considered an error (forward-compat guard).
    KNOWN_VALID_MASK     = (1u << 12) - 1u,
};

inline constexpr DeviceCapabilityFlags operator|(DeviceCapabilityFlags a,
                                                  DeviceCapabilityFlags b) noexcept {
    return static_cast<DeviceCapabilityFlags>(
        static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr DeviceCapabilityFlags operator&(DeviceCapabilityFlags a,
                                                  DeviceCapabilityFlags b) noexcept {
    return static_cast<DeviceCapabilityFlags>(
        static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline constexpr DeviceCapabilityFlags operator~(DeviceCapabilityFlags a) noexcept {
    return static_cast<DeviceCapabilityFlags>(~static_cast<uint32_t>(a));
}
inline constexpr bool hasCapability(DeviceCapabilityFlags flags,
                                     DeviceCapabilityFlags flag) noexcept {
    return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(flag)) != 0;
}

// =============================================================================
// DeviceSet — small-vector of up to 8 device indices (stack allocated)
// =============================================================================

/**
 * @brief Compact set of GPU device indices (max 8 devices).
 *
 * Stack-allocated to avoid heap allocation in the hot path of
 * `IMultiGPUSelector::selectDevices()`.  Construction time for an 8-device
 * selection is O(1) with no allocator involvement.
 *
 * Thread safety: individual `DeviceSet` instances are not thread-safe; callers
 * must synchronise access.
 */
struct DeviceSet {
    /// Maximum number of GPU devices that can be held in a single set.
    static constexpr size_t kMaxDevices = 8;

    uint32_t devices[kMaxDevices] = {};  ///< Device indices (in priority order)
    uint32_t count                = 0;   ///< Number of valid entries in @p devices

    // ── Accessors ─────────────────────────────────────────────────────────

    bool     empty()                    const noexcept { return count == 0; }
    size_t   size()                     const noexcept { return static_cast<size_t>(count); }
    uint32_t operator[](size_t i)       const noexcept { return devices[i]; }
    uint32_t front()                    const noexcept { return devices[0]; }
    uint32_t back()                     const noexcept { return devices[count > 0 ? count - 1u : 0u]; }

    const uint32_t* begin()             const noexcept { return devices; }
    const uint32_t* end()               const noexcept { return devices + count; }

    // ── Mutation ──────────────────────────────────────────────────────────

    /**
     * @brief Append a device index if capacity allows.
     * @return true on success; false when the set is already full.
     */
    bool push(uint32_t device) noexcept {
        if (count >= kMaxDevices) return false;
        devices[count++] = device;
        return true;
    }
};

// =============================================================================
// WorkloadDescriptor — hints for IMultiGPUSelector::selectDevices()
// =============================================================================

/**
 * @brief Latency class for a compute workload.
 *
 * Used by `IMultiGPUSelector` to bias device selection toward low-latency
 * (interactive) or high-throughput (batch) scheduling strategies.
 */
enum class LatencyClass : uint8_t {
    INTERACTIVE = 0,  ///< Latency-critical (< 1 ms); minimise queue depth
    BATCH       = 1,  ///< Throughput-oriented; minutes; GPU saturation preferred
    BACKGROUND  = 2,  ///< Non-interactive; accept long queue wait times
};

/**
 * @brief Descriptor used by `IMultiGPUSelector::selectDevices()` to
 *        communicate the resource characteristics of an upcoming workload.
 */
struct WorkloadDescriptor {
    size_t       byte_size     = 0;                     ///< Total input data size in bytes
    uint64_t     flop_estimate = 0;                     ///< Estimated FLOPs (0 = unknown)
    LatencyClass latency_class = LatencyClass::BATCH;   ///< Scheduling priority hint
    PrecisionMode precision    = PrecisionMode::FP32;   ///< Required precision mode
};

// =============================================================================
// BatchDescriptor — input/output shape for kernel dispatch
// =============================================================================

/**
 * @brief Shape and pointer descriptor for a batched similarity kernel call.
 *
 * All pointers are host pointers for CPU backends; GPU backends are
 * responsible for any required host-to-device transfers.
 */
// BatchDescriptor is defined before IComputeBackend.

// =============================================================================
// KernelConfig — plain-data runtime parameters for a compute kernel
// =============================================================================

/**
 * @brief Plain-data runtime configuration for a compute kernel dispatch.
 *
 * This struct must not contain any CUDA/Vulkan/HIP types so that it can be
 * included in any translation unit without GPU SDK headers.
 *
 * Backend implementations translate this struct into the corresponding
 * device-specific launch parameters (e.g. `dim3 blockDim` for CUDA).
 */
// KernelConfig is defined before IComputeBackend.

// =============================================================================
// KernelDescriptor — combined batch + config + optional named kernel
// =============================================================================

/**
 * @brief Full descriptor for a kernel submission to IAsyncComputeDispatch.
 *
 * Combines the data-shape information (`BatchDescriptor`) with the execution
 * configuration (`KernelConfig`) and an optional named kernel identifier that
 * can be looked up via `IKernelRegistry`.
 */
struct KernelDescriptor {
    BatchDescriptor batch;            ///< Input / output shapes and host pointers
    KernelConfig    config;           ///< Execution parameters
    std::string     kernel_name;      ///< Optional: resolved via IKernelRegistry (empty = auto)
};

// =============================================================================
// SimilarityKernelResult — return value of submitSimilarityKernel()
// =============================================================================

// SimilarityKernelResult is defined before IComputeBackend.

// =============================================================================
// IComputeBackend::submitSimilarityKernel() — default virtual method
// =============================================================================
// The method is added to IComputeBackend below via a non-pure virtual with a
// default CPU-fallback implementation.  Backends that support hardware-
// accelerated similarity search should override it.
//
// Note: IComputeBackend is defined earlier in this header; we add the new
// method by providing a standalone free function + default in a derived helper.
// To avoid breaking ABI for existing IComputeBackend subclasses the method is
// non-pure virtual with a full default body.
// =============================================================================

// =============================================================================
// IDeviceCapabilityQuery — query hardware feature flags without a GPU context
// =============================================================================

/**
 * @brief Interface for querying device capability flags.
 *
 * All methods must be callable **before** CUDA/Vulkan context creation so
 * that device selection can happen at startup without side effects.
 *
 * Implementations must be thread-safe: any method may be called concurrently
 * from multiple threads without external locking.
 */
class IDeviceCapabilityQuery {
public:
    virtual ~IDeviceCapabilityQuery() = default;

    /**
     * @brief Query capability flags for a specific device.
     *
     * @param device_index  Driver device index (0-based).
     * @return Detected `DeviceCapabilityFlags`.  Returns `NONE` when the
     *         device index is invalid, the driver is not loaded, or the
     *         returned flags contain bits outside `KNOWN_VALID_MASK`.
     */
    virtual DeviceCapabilityFlags queryCapabilities(int device_index) const noexcept = 0;

    /**
     * @brief Query capabilities for all enumerated devices.
     *
     * @return One `DeviceCapabilityFlags` entry per device in driver
     *         enumeration order.  An empty vector is returned when no GPU
     *         driver is present.
     */
    virtual std::vector<DeviceCapabilityFlags> queryAll() const = 0;
};

// =============================================================================
// IMultiGPUSelector — thread-safe workload-to-device mapping
// =============================================================================

/**
 * @brief Interface for selecting the best set of GPU devices for a workload.
 *
 * ## Thread safety
 * `selectDevices()` is guaranteed to be safe to call concurrently from N
 * threads without external locking.  Implementations must document any
 * internal synchronisation used.
 *
 * ## Typical usage
 * @code
 *   auto& selector = registry.getMultiGPUSelector();
 *   WorkloadDescriptor wl;
 *   wl.byte_size     = num_vectors * dim * sizeof(float);
 *   wl.flop_estimate = num_vectors * dim * 2; // multiply-add per element
 *   wl.latency_class = LatencyClass::INTERACTIVE;
 *   wl.precision     = PrecisionMode::FP32;
 *
 *   DeviceSet devices = selector.selectDevices(wl);
 *   // devices[0] is the highest-priority recommended GPU.
 * @endcode
 */
class IMultiGPUSelector {
public:
    virtual ~IMultiGPUSelector() = default;

    /**
     * @brief Select the best set of GPU devices for the given workload.
     *
     * Thread-safe: safe to call concurrently from N threads.
     *
     * @param workload  Descriptor with size, FLOP estimate, and latency class.
     * @return Set of selected device indices in priority order (highest
     *         priority first).  Returns an empty `DeviceSet` when no suitable
     *         GPU device is available.
     */
    virtual DeviceSet selectDevices(const WorkloadDescriptor& workload) const = 0;

    /**
     * @brief Returns the number of GPU devices visible to this selector.
     *
     * Thread-safe.
     */
    virtual uint32_t deviceCount() const noexcept = 0;
};

// =============================================================================
// IKernelRegistry — named compute kernel lookup table
// =============================================================================

/**
 * @brief Registry for named compute kernel function pointers.
 *
 * Allows `IAsyncComputeDispatch` to resolve named kernels at dispatch time
 * without hard-coding symbol names.  Intended for use by plugin backends that
 * expose custom kernel implementations at runtime.
 *
 * Kernel names are arbitrary strings; by convention they use the format
 * `"<backend>/<operation>"` (e.g. `"cuda/l2_distance"`, `"vulkan/topk"`).
 *
 * ## Thread safety
 * Implementations must document their thread-safety guarantees.  The default
 * expectation is that `resolveKernel()` and `hasKernel()` are safe to call
 * concurrently, while `registerKernel()` and `deregisterKernel()` require
 * exclusive access.
 */
class IKernelRegistry {
public:
    virtual ~IKernelRegistry() = default;

    /**
     * @brief Register a named kernel function pointer.
     *
     * @param name    Kernel identifier string (e.g. `"cuda/cosine_distance"`).
     * @param fn_ptr  Opaque function pointer to the kernel launcher.
     * @return true on success; false if @p name is already registered.
     */
    virtual bool registerKernel(std::string name, void* fn_ptr) = 0;

    /**
     * @brief Resolve a named kernel to its function pointer.
     *
     * @param name  Kernel identifier string.
     * @return Function pointer cast to `void*`, or `nullptr` if not registered.
     */
    virtual void* resolveKernel(const std::string& name) const = 0;

    /**
     * @brief Returns true if a kernel with @p name is registered.
     *
     * Thread-safe (read-only lookup).
     */
    virtual bool hasKernel(const std::string& name) const noexcept = 0;

    /**
     * @brief Remove a kernel from the registry.
     *
     * @return true if the kernel was present and removed; false otherwise.
     */
    virtual bool deregisterKernel(const std::string& name) noexcept = 0;
};

// =============================================================================
// IAsyncComputeDispatch — non-blocking kernel submission
// =============================================================================

/**
 * @brief Interface for non-blocking kernel submission and result collection.
 *
 * Callers submit a `KernelDescriptor` (containing the input data shape and
 * execution parameters) and immediately receive a `ComputeFuture<T>`.  The
 * kernel executes asynchronously; `ComputeFuture::get()` blocks until it
 * completes.
 *
 * ## Cancellation
 * The caller may request early termination via `CancellationToken::cancel()`.
 * The implementation is responsible for observing the token and aborting
 * (best-effort) before full completion.  After cancellation, `get()` may
 * still return a valid result if the kernel finished before the token was
 * observed.
 *
 * ## Performance target
 * The `submit()` call overhead on the calling thread must be ≤ 2 µs
 * (measured on x86-64, GCC -O2) regardless of queue depth.
 */
class IAsyncComputeDispatch {
public:
    virtual ~IAsyncComputeDispatch() = default;

    /**
     * @brief Submit a similarity kernel for asynchronous execution.
     *
     * @param descriptor  Combined batch shape + kernel config + optional
     *                    named kernel identifier.
     * @param token       Cancellation token (default-constructed = no
     *                    cancellation support).
     * @return A `ComputeFuture<SimilarityKernelResult>` that will carry the
     *         result once the kernel completes.
     */
    virtual ComputeFuture<SimilarityKernelResult>
    submit(const KernelDescriptor& descriptor,
           CancellationToken        token = {}) = 0;
};

} // namespace acceleration
} // namespace themis
