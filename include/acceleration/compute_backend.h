/**
 * @file compute_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: compute_backend.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 1119
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4620 feat(acceleration): Backend... (2026-04-13) | #4338 [WIP] Update documentation ... (2026-03-19) | #3555 docs(acceleration): ROADMAP... (2026-03-12) | #2755 feat(acceleration): final p... (2026-03-12) | #2717 feat(acceleration): Impleme... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

/// @brief Enumeration of supported compute backend types.
/// 
/// @details Represents the various hardware accelerators and compute backends
/// available for acceleration operations. Each backend type may support a
/// different subset of operations (vector, graph, geo, matrix) and precision
/// modes. CPU is always available as a fallback; GPU backends may not be present
/// depending on system configuration.
enum class BackendType {
    CPU,            ///< CPU-only fallback (always available)
    CUDA,           ///< NVIDIA CUDA (requires NVIDIA GPU + CUDA toolkit)
    ZLUDA,          ///< AMD ZLUDA (CUDA compatibility layer for AMD GPUs)
    HIP,            ///< AMD HIP (Heterogeneous-computing Interface for Portability)
    ROCM,           ///< AMD ROCm (modern AMD GPU compute platform)
    DIRECTX,        ///< DirectX Compute Shaders (Windows GPU compute)
    VULKAN,         ///< Vulkan Compute (cross-platform GPU compute)
    OPENGL,         ///< OpenGL Compute Shaders (legacy GPU compute support)
    METAL,          ///< Apple Metal (iOS/macOS GPU compute)
    ONEAPI,         ///< Intel OneAPI/SYCL (Intel GPU and CPU compute)
    OPENCL,         ///< OpenCL (generic GPU/CPU compute)
    WEBGPU,         ///< WebGPU (browser-based GPU compute; future support)
    MULTI_GPU,      ///< Multi-GPU sharding (distributes work across N devices)
    // ── AI-specific accelerator backends ─────────────────────────────────────
    // Dedicated AI/inference hardware with dedicated low-power neural engines.
    // All AI backends expose graceful CPU fallback via AiHardwareDispatcher.
    NPU_APPLE,      ///< Apple Neural Engine (Core ML / Metal Performance Shaders)
    NPU_INTEL,      ///< Intel NPU (OpenVINO / iGPU tile)
    NPU_QUALCOMM,   ///< Qualcomm QNN / Hexagon DSP / Snapdragon NPU
    NPU_ARM,        ///< ARM Ethos-N / Mali AI extensions
    NNAPI,          ///< Android Neural Networks API (delegates to best available)
    ONNX_RUNTIME,   ///< ONNX Runtime (universal AI inference, selects EP at runtime)
    AUTO            ///< Auto-detect and select best available backend
};

/// @brief Floating-point and quantization precision modes.
///
/// @details Bitmask flags representing supported numeric precision types.
/// Values are stable and may be combined with bitwise OR to create feature sets.
/// Combine flags with operator| to create composite PrecisionMode values
/// representing multiple supported precisions.
///
/// @note FP32 is always required as a fallback for all compute backends.
/// @note Quantization modes (INT4, INT8, FP4, W4A8, W8A8) are primarily used by
/// AI inference backends (NPU, ONNX Runtime).
enum class PrecisionMode : uint32_t {
    NONE  = 0,          ///< No precision mode specified
    FP32  = 1u << 0,    ///< 32-bit IEEE 754 single precision (always required fallback)
    FP16  = 1u << 1,    ///< 16-bit IEEE 754 half precision (GPU/Tensor Core)
    BF16  = 1u << 2,    ///< Brain float BF16 (Tensor Core, modern NPUs)
    INT8  = 1u << 3,    ///< 8-bit integer quantization (symmetric/asymmetric)
    // ── AI / LLM quantization modes ──────────────────────────────────────────
    // Used by NPU and dedicated AI inference engines.
    INT4  = 1u << 4,    ///< 4-bit integer (GPTQ / AWQ / NF4 schemes)
    FP4   = 1u << 5,    ///< 4-bit float (e.g. NF4, FP4-E2M1)
    W4A8  = 1u << 6,    ///< 4-bit weights, 8-bit activations (Qualcomm AI Engine)
    W8A8  = 1u << 7,    ///< 8-bit weights and 8-bit activations (symmetric INT8)
};

/// @brief Combine two PrecisionMode flags with bitwise OR.
/// @param a First precision mode
/// @param b Second precision mode
/// @return Combined precision mode with both flags set
inline constexpr PrecisionMode operator|(PrecisionMode a, PrecisionMode b) noexcept {
    return static_cast<PrecisionMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

/// @brief Test if precision modes have enabled precision flag.
/// @param modes Precision mode bitmask to test
/// @param flag The specific precision flag to check for
/// @return true if @p flag is set in @p modes, false otherwise
inline constexpr bool hasPrecision(PrecisionMode modes, PrecisionMode flag) noexcept {
    return (static_cast<uint32_t>(modes) & static_cast<uint32_t>(flag)) != 0;
}

/// Returns the bitmask bit position for a DistanceMetric value.
/// Bit i is set when DistanceMetric(i) is supported.
inline constexpr uint32_t metricBit(DistanceMetric m) noexcept {
    return 1u << static_cast<uint32_t>(m);
}

/// @brief PCI vendor IDs for common GPU vendors
///
/// Used by Vulkan and other backends for device selection and capability
/// reporting. These standard PCI vendor ID constants enable backend implementations
/// to identify GPU manufacturers from device enumeration results.
namespace vendor_id {
    static constexpr uint32_t NVIDIA   = 0x10DE;  ///< NVIDIA Corporation
    static constexpr uint32_t AMD      = 0x1002;  ///< Advanced Micro Devices (AMD)
    static constexpr uint32_t INTEL    = 0x8086;  ///< Intel Corporation
    static constexpr uint32_t ARM      = 0x13B5;  ///< ARM Holdings
    static constexpr uint32_t QUALCOMM = 0x5143;  ///< Qualcomm Incorporated
    static constexpr uint32_t IMGTEC   = 0x1010;  ///< Imagination Technologies
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

/// @brief Capability contract for a compute backend.
///
/// Describes the operations and precision modes supported by a backend.
/// Backends report these capabilities during initialization; callers use them
/// for capability-driven backend selection and feature negotiation.
/// 
/// @details Fields are grouped by category: operation support, precision matrix,
/// metric matrix, and device information. All boolean fields default to false,
/// indicating unsupported features. Backends must set flags truthfully to avoid
/// runtime failures from unsupported operations.
struct BackendCapabilities {
    /// @name Operation Support
    /// @{
    bool supportsVectorOps = false;           ///< Backend supports vector similarity (ANN) operations
    bool supportsGraphOps = false;            ///< Backend supports graph traversal (BFS, shortest-path)
    bool supportsGeoOps = false;              ///< Backend supports geospatial operations (distance, containment)
    bool supportsMatrixOps = false;           ///< Backend supports FP16/BF16 matrix multiply via Tensor Core
    bool supportsBatchProcessing = false;     ///< Backend can process multiple independent queries in parallel
    bool supportsAsync = false;               ///< Backend supports asynchronous (non-blocking) execution
    bool supportsAiInference = false;         ///< Backend supports dedicated AI inference (NPU/ONNX Runtime)
    /// @}

    /// @name Precision and Metric Support
    /// @{
    /// @brief Precision feature matrix: OR of PrecisionMode flags.
    /// Must include at least PrecisionMode::FP32 for vector/geo backends.
    /// Bitwise OR of all supported precisions (combine with operator|).
    PrecisionMode supportedPrecisions = PrecisionMode::NONE;

    /// @brief Distance-metric feature matrix: bitmask using metricBit(DistanceMetric).
    /// Set bit i if the backend supports DistanceMetric(i) in its ANN dispatch.
    /// For example: metricBit(L2) | metricBit(COSINE) for L2 and cosine support.
    uint32_t supportedMetrics = 0;
    /// @}

    /// @name Device Information
    /// @{
    size_t maxMemoryBytes = 0;                ///< Available VRAM/host memory in bytes
    int computeUnits = 0;                     ///< Number of compute units/SMs (0 for CPU)
    std::string deviceName;                   ///< Human-readable device name (e.g. "RTX 4090")
    /// @brief Vendor name for GPU/hardware identification
    /// Empty string means unknown or CPU backend.
    /// Examples: "NVIDIA", "AMD", "Intel", "ARM"
    std::string vendorName;
    /// @}

    /// @name AI / NPU-specific
    /// @{
    uint32_t    npuTops           = 0;        ///< Reported peak throughput in TOPS (0 = unknown or CPU)
    std::string preferredOnnxEP;              ///< ONNX Runtime execution provider (empty = CPU fallback)
    /// @}
};

/// @brief Backend health status information and diagnostic data.
///
/// Returned by IComputeBackend::getHealthStatus() to provide comprehensive
/// health, readiness, and liveness information about a backend.
///
/// @details Health states form a hierarchy:
/// - healthy:   Backend is fully operational (ready=true, alive=true)
/// - degraded:  Backend is partially available (ready=false, alive=true)
/// - unhealthy: Backend is non-operational (ready=false, alive=false)
struct BackendHealthStatus {
    /// @brief Overall health status string: "healthy", "degraded", or "unhealthy"
    std::string status;

    /// @brief True when the backend has been successfully initialized and is ready
    /// to accept work (combines liveness probe + readiness probe)
    bool healthy  = false;
    
    /// @brief Backend compute pipelines are loaded and ready to execute kernels
    bool ready    = false;
    
    /// @brief Backend process/driver is reachable and responding
    bool alive    = false;

    /// @brief Human-readable description of the current state (e.g. "healthy")
    std::string message;

    /// @brief List of actionable issue descriptions (empty when healthy)
    /// Contains diagnostic info to help resolve problems (e.g. "CUDA compute capability too low")
    std::vector<std::string> issues;

    /// @brief Device/driver information (populated when alive)
    /// @{
    std::string deviceName;           ///< GPU/device model name (e.g. "RTX 4090")
    std::string driverInfo;           ///< Driver version or runtime info
    /// @}

    /// @brief Memory snapshot (0 when unavailable)
    /// @{
    size_t memoryUsedBytes      = 0;  ///< Memory currently in use (bytes)
    size_t memoryAvailableBytes = 0;  ///< Available free memory (bytes)
    /// @}

    /// @name Builder Helpers
    /// @brief Static factory methods for creating pre-configured health status objects
    /// @{
    
    /// @brief Create a "healthy" status indicating full operational capability.
    /// @param device Optional device name to include in the status
    /// @return BackendHealthStatus with healthy=true, ready=true, alive=true
    static BackendHealthStatus makeHealthy(const std::string& device = "") {
        BackendHealthStatus s;
        s.status  = "healthy";
        s.healthy = s.ready = s.alive = true;
        s.message    = "Backend is operational";
        s.deviceName = device;
        return s;
    }

    /// @brief Create a "degraded" status indicating partial operational capability.
    /// @param issue Description of the degradation issue
    /// @return BackendHealthStatus with healthy=false, ready=false, alive=true
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

    /// @brief Create an "unhealthy" status indicating complete non-operability.
    /// @param issue Description of the failure
    /// @return BackendHealthStatus with healthy=false, ready=false, alive=false
    static BackendHealthStatus makeUnhealthy(const std::string& issue) {
        BackendHealthStatus s;
        s.status  = "unhealthy";
        s.healthy = s.ready = s.alive = false;
        s.message = issue;
        s.issues.push_back(issue);
        return s;
    }
    /// @}
};

/// @brief Result returned by IComputeBackend::submitSimilarityKernel().
///
/// Contains the top-k nearest neighbor results for a batch of queries.
/// Results are guaranteed to match the CPU baseline within FP tolerance.
///
/// @details FP tolerance guarantee: results produced by hardware-accelerated
/// paths must agree with the CPU baseline within <= 1e-6 relative error
/// for FP32 inputs. This ensures deterministic and reproducible results
/// across different backend implementations.
struct SimilarityKernelResult {
    /// @brief Per-query top-k results as (corpus_id, distance) pairs.
    /// Outer index = query index; inner index = rank (0 = closest).
    /// Results are sorted ascending by distance (lower distance = better match).
    std::vector<std::vector<std::pair<uint32_t, float>>> results;

    DistanceMetric metric_used    = DistanceMetric::L2;  ///< Distance metric used for computation
    PrecisionMode  precision_used = PrecisionMode::FP32; ///< Floating-point precision used
    bool           used_hw_path   = false;               ///< True if GPU/hardware path was used
    double         speedup_vs_cpu = 1.0;                 ///< Speedup ratio: CPU time / GPU time
};

/// @brief Input descriptor for a batched similarity kernel invocation.
///
/// Specifies the query and vector data layouts and counts for a single
/// kernel dispatch call. All pointers are host pointers; GPU backends are
/// responsible for host-to-device transfers.
struct BatchDescriptor {
    const float* queries     = nullptr;  ///< Query matrix [numQueries × dim] (row-major)
    size_t       num_queries = 0;        ///< Number of query vectors
    size_t       dim         = 0;        ///< Dimensionality of each vector
    const float* vectors     = nullptr;  ///< Database/corpus matrix [numVectors × dim] (row-major)
    size_t       num_vectors = 0;        ///< Number of database vectors
    size_t       k           = 1;        ///< Number of nearest neighbors to retrieve
};

/// @brief Plain-data runtime configuration for a compute kernel dispatch.
///
/// Contains execution parameters that are backend-independent and can be
/// used by any GPU backend. Backend implementations translate these generic
/// parameters into device-specific launch parameters (e.g. CUDA dim3 blockDim).
///
/// @details This struct must not contain any CUDA/Vulkan/HIP types so that
/// it can be included in any translation unit without requiring GPU SDK headers.
struct KernelConfig {
    uint32_t       block_size = 256;         ///< Thread block/work group size
    uint32_t       grid_size  = 0;           ///< Number of blocks/work groups (0 = auto-calculate)
    uint32_t       shared_mem = 0;           ///< Shared memory per block (bytes; GPU only)
    DistanceMetric metric     = DistanceMetric::L2;  ///< Distance metric to use
    PrecisionMode  precision  = PrecisionMode::FP32; ///< Floating-point precision
    bool           async_exec = false;       ///< True for asynchronous execution (non-blocking)
};

/// @brief Base interface for all compute backends.
///
/// Defines the common contract that all backend implementations must satisfy,
/// including initialization, health reporting, and error handling. This is the
/// parent class for all specialized backend interfaces (IVectorBackend, IGraphBackend,
/// IGeoBackend, IMatrixBackend).
///
/// @details Implementations must be thread-safe unless explicitly documented
/// otherwise. All virtual methods are non-const unless the operation is read-only.
class IComputeBackend {
public:
    virtual ~IComputeBackend() = default;
    
    /// @name Identification
    /// @{
    
    /// @brief Get the human-readable name of this backend.
    /// @return Null-terminated string (e.g., "CUDA Backend", "Vulkan Backend")
    [[nodiscard]] virtual const char* name() const noexcept = 0;
    
    /// @brief Get the backend type enumeration.
    /// @return BackendType enum value identifying this backend
    [[nodiscard]] virtual BackendType type() const noexcept = 0;
    
    /// @brief Check if this backend is available on the current system.
    /// @return true if the backend can be used (hardware/drivers present)
    [[nodiscard]] virtual bool isAvailable() const noexcept = 0;
    /// @}
    
    /// @name Capabilities and Configuration
    /// @{
    
    /// @brief Query the capabilities of this backend.
    /// @return BackendCapabilities describing supported operations and precision modes
    [[nodiscard]] virtual BackendCapabilities getCapabilities() const = 0;
    /// @}
    
    /// @name Lifecycle
    /// @{
    
    /// @brief Initialize the backend and prepare it for work.
    /// @return true on successful initialization, false on failure
    /// @details Must be called before any kernel operations. Safe to call
    /// multiple times; subsequent calls are idempotent.
    [[nodiscard]] virtual bool initialize() = 0;
    
    /// @brief Shut down the backend and release all resources.
    /// @details All pending operations must complete before shutdown.
    /// Safe to call multiple times.
    virtual void shutdown() = 0;
    /// @}
    
    /// @name Error Handling
    /// @{
    
    /// @brief Get the last error that occurred in this backend.
    /// @return ErrorContext containing error code, message, and diagnostics
    virtual ErrorContext getLastError() const {
        return lastError_;
    }
    /// @}

    /// @name Health and Status
    /// @{
    
    /// @brief Get the current health and readiness status of this backend.
    /// @return BackendHealthStatus describing current state and diagnostic info
    /// @details Default implementation derives status from isAvailable() and
    /// getLastError(). Subclasses may override for more detailed diagnostics.
    /// Returns "healthy", "degraded", or "unhealthy" status.
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
    /// @}

    /// @name Kernel Submission (Default CPU Fallback)
    /// @{
    
    /// @brief Submit a similarity search kernel for asynchronous execution.
    ///
    /// Dispatches a batched vector similarity search to the fastest available
    /// execution path. The default implementation runs the search synchronously
    /// on the CPU and returns an already-fulfilled ComputeFuture so that callers
    /// can use a uniform async API regardless of backend type.
    /// 
    /// GPU backends (CUDA, Vulkan, HIP, …) should override this method to
    /// dispatch to their respective device kernels.
    ///
    /// @param batch   Input descriptor: query/corpus pointers, sizes, and k
    /// @param config  Kernel execution configuration: metric, precision, block size
    /// @param token   Optional cancellation token (default-constructed = no cancel)
    /// @return ComputeFuture<SimilarityKernelResult> that becomes ready once the
    ///         kernel completes. The default implementation returns an immediately-ready
    ///         future with CPU-computed results.
    ///
    /// @note FP tolerance guarantee: hardware paths must agree with the CPU baseline
    /// within ≤ 1e-6 relative error for FP32 inputs.
    ///
    /// @throws None (noexcept). Errors are returned via the future.
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
    /// @}
    
protected:
    /// @name Error Management (Protected Helpers)
    /// @{
    
    /// @brief Helper for backends to set error context.
    /// Stores error state for retrieval via getLastError().
    void setError(ErrorContext error) {
        lastError_ = std::move(error);
    }
    
    /// @brief Helper to clear error state (on success).
    void clearError() {
        lastError_ = ErrorContext(
            AccelerationErrorCode::Success,
            name(),
            ""
        );
    }
    /// @}
    
protected:
    ErrorContext lastError_;  ///< Last error context (stored for programmatic access)
};

/// @brief Per-query result with deterministic ordering and partial-failure status.
///
/// Contains nearest neighbors for a single query with optional failure information.
/// Success and failure cases are distinguished via the status field.
///
/// @details On success, neighbors are sorted ascending by distance (lower distance
/// = closer match). When distances are equal, results are sorted by vector index
/// for deterministic ordering. On failure, neighbors is empty and status/errorMessage
/// describe the failure reason.
struct KnnQueryResult {
    /// @brief Nearest neighbors as (corpus_id, distance) pairs, sorted by distance.
    /// Empty on failure; populated with up to k entries on success.
    std::vector<std::pair<uint32_t, float>> neighbors;
    
    /// @brief Error status for this query. Success (default) or failure code.
    AccelerationErrorCode status   = AccelerationErrorCode::Success;
    
    /// @brief Human-readable error message (empty on success).
    /// Examples: "NaN in query vector", "Inf in corpus vector", "dimension mismatch"
    std::string           errorMessage;
};

/// @brief Batch KNN result supporting partial failures.
///
/// Contains results for a batch of queries where some queries may have failed
/// validation while others succeeded. This structure allows callers to process
/// results incrementally without blocking on failed queries.
///
/// @details Queries that fail validation (e.g., NaN/Inf values) receive a
/// non-Success status in queryResults[i].status while other queries that
/// succeed return their neighbors normally. The successCount and failureCount
/// fields summarize the batch result without requiring clients to scan all results.
struct PartialBatchResult {
    /// @brief Per-query results including success/failure status.
    /// Index i corresponds to query i from the original batch.
    std::vector<KnnQueryResult> queryResults;
    
    /// @brief Number of queries that succeeded (status == Success)
    size_t successCount = 0;
    
    /// @brief Number of queries that failed validation (status != Success)
    size_t failureCount = 0;
};

/// @brief Vector operations backend interface.
///
/// Specializes IComputeBackend to provide ANN (approximate nearest neighbor)
/// similarity search and distance computation operations. Backends that support
/// vector operations implement this interface and register themselves with
/// BackendRegistry.
///
/// @details All methods process batches of queries against a fixed corpus.
/// Results are deterministic: when two candidates share the same distance,
/// the one with the lower vector index is placed first.
class IVectorBackend : public IComputeBackend {
public:
    virtual ~IVectorBackend() = default;
    
    /// @brief Compute pairwise distances between all queries and vectors.
    ///
    /// Computes the full [numQueries × numVectors] distance matrix using the
    /// specified distance metric.
    ///
    /// @param queries      Query matrix [numQueries × dim] (row-major)
    /// @param numQueries   Number of queries
    /// @param dim          Vector dimensionality
    /// @param vectors      Database vectors [numVectors × dim] (row-major)
    /// @param numVectors   Database size
    /// @param useL2        If true, use L2 distance; if false, use cosine distance
    /// @return Distance matrix [numQueries × numVectors] linearized (row-major)
    /// @throws std::runtime_error on invalid input or device errors
    [[nodiscard]] virtual std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) = 0;
    
    /// @brief Batch KNN search: find top-k nearest neighbors for each query.
    ///
    /// Computes the top-k nearest neighbors for each query against the corpus.
    /// Results are sorted ascending by distance (best matches first).
    ///
    /// @param queries      Query matrix [numQueries × dim] (row-major)
    /// @param numQueries   Number of queries
    /// @param dim          Vector dimensionality
    /// @param vectors      Database vectors [numVectors × dim] (row-major)
    /// @param numVectors   Database size
    /// @param k            Number of nearest neighbors to retrieve
    /// @param useL2        If true, use L2 distance; if false, use cosine distance
    /// @return Outer vector: one entry per query. Inner vector: up to k
    ///         (corpus_id, distance) pairs sorted by distance.
    /// @throws std::runtime_error on invalid input or device errors
    /// @pre k <= numVectors (enforced by caller)
    /// @note Tie-breaking: when distances are equal, results are sorted by
    ///       vector index for deterministic ordering.
    [[nodiscard]] virtual std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) = 0;

    /// @brief Batch KNN search with per-query partial-failure handling.
    ///
    /// Computes KNN for each query, but handles invalid queries gracefully.
    /// Each query is validated before execution; queries whose input vectors
    /// contain NaN or Inf values receive AccelerationErrorCode::InputRangeViolation
    /// and an empty neighbors list, while remaining valid queries are processed
    /// normally. This default implementation delegates to batchKnnSearch for valid
    /// queries; backends may override for tighter integration.
    ///
    /// @param queries      Query matrix [numQueries × dim] (row-major)
    /// @param numQueries   Number of queries
    /// @param dim          Vector dimensionality
    /// @param vectors      Database vectors [numVectors × dim] (row-major)
    /// @param numVectors   Database size
    /// @param k            Number of nearest neighbors to retrieve
    /// @param useL2        If true, use L2 distance; if false, use cosine distance
    /// @return PartialBatchResult with per-query status and results
    /// @note Invalid queries are skipped without aborting the entire batch
    virtual PartialBatchResult batchKnnSearchSafe(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    );

    /// @brief Populate the frozen kernel dispatch table for this backend.
    ///
    /// Backends override this to expose their kernel function pointers for
    /// direct invocation. Null entries in the returned table indicate unsupported
    /// operations; callers should use ANNKernelFallbackDispatcher for fallback
    /// and retry semantics.
    ///
    /// @return ANNKernelDispatch with function pointers (may contain nullptr entries)
    /// @note This is called once during backend registration; results are cached
    virtual ANNKernelDispatch populateANNDispatch() const { return {}; }
};

/// @brief Graph operations backend interface.
///
/// Specializes IComputeBackend to provide graph traversal and shortest-path
/// algorithms. Backends that support graph operations implement this interface.
///
/// @details Graphs are represented in adjacency format. For BFS and shortest-path,
/// results are per-query vectors of vertex indices describing the traversal path
/// or search results.
class IGraphBackend : public IComputeBackend {
public:
    virtual ~IGraphBackend() = default;
    
    /// @brief Batch breadth-first search (BFS) traversal.
    ///
    /// Performs BFS from a set of start vertices, exploring up to maxDepth levels.
    ///
    /// @param adjacency     Graph adjacency matrix (compressed sparse format or dense)
    /// @param numVertices   Total number of vertices in the graph
    /// @param startVertices Starting vertex indices [numStarts]
    /// @param numStarts     Number of starting vertices
    /// @param maxDepth      Maximum traversal depth
    /// @return Outer vector: one per starting vertex. Inner vector: vertices visited
    ///         in BFS order up to maxDepth levels.
    /// @throws std::runtime_error on invalid graph or device errors
    /// @throws std::invalid_argument if startVertices contains out-of-range indices
    [[nodiscard]] virtual std::vector<std::vector<uint32_t>> batchBFS(
        const uint32_t* adjacency,
        size_t numVertices,
        const uint32_t* startVertices,
        size_t numStarts,
        uint32_t maxDepth
    ) = 0;
    
    /// @brief Batch shortest-path computation (Dijkstra or similar).
    ///
    /// Computes shortest paths between specified source/destination pairs using
    /// the provided edge weights.
    ///
    /// @param adjacency     Graph adjacency matrix
    /// @param weights       Edge weights [numVertices × numVertices] or sparse format
    /// @param numVertices   Total number of vertices
    /// @param startVertices Source vertices [numPairs]
    /// @param endVertices   Destination vertices [numPairs]
    /// @param numPairs      Number of (source, destination) pairs
    /// @return Outer vector: one per pair. Inner vector: vertex indices describing
    ///         the shortest path from startVertices[i] to endVertices[i].
    /// @throws std::runtime_error on invalid graph or device errors
    /// @throws std::invalid_argument if vertex indices are out-of-range
    [[nodiscard]] virtual std::vector<std::vector<uint32_t>> batchShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices,
        const uint32_t* startVertices,
        const uint32_t* endVertices,
        size_t numPairs
    ) = 0;
};

/// @brief Geospatial operations backend interface.
///
/// Specializes IComputeBackend to provide geospatial distance and containment
/// queries. Backends that support geospatial operations implement this interface.
///
/// @details All coordinates are in WGS84 (latitude/longitude) format.
/// Distances are returned in kilometers. Containment uses ray-casting algorithm.
class IGeoBackend : public IComputeBackend {
public:
    virtual ~IGeoBackend() = default;
    
    /// @brief Batch geospatial distance calculations.
    ///
    /// Computes geodesic distances between corresponding latitude/longitude pairs
    /// using Haversine or Vincenty formula.
    ///
    /// @param latitudes1   First set of latitudes (degrees, WGS84) [count]
    /// @param longitudes1  First set of longitudes (degrees, WGS84) [count]
    /// @param latitudes2   Second set of latitudes (degrees, WGS84) [count]
    /// @param longitudes2  Second set of longitudes (degrees, WGS84) [count]
    /// @param count        Number of point pairs to compute
    /// @param useHaversine If true, use Haversine (faster, <0.5% error);
    ///                     if false, use Vincenty (slower, higher precision)
    /// @return Distance vector [count] with distances in kilometers
    /// @throws std::runtime_error on invalid coordinates or device errors
    /// @throws std::invalid_argument if count == 0
    /// @note Coordinates outside [-90, 90] for latitude or [-180, 180] for
    ///       longitude are invalid; behavior is undefined for out-of-range input
    [[nodiscard]] virtual std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) = 0;
    
    /// @brief Batch point-in-polygon containment tests.
    ///
    /// Tests whether each point is inside the given polygon using ray-casting.
    /// The polygon is specified as interleaved vertex coordinates [lat0, lon0, lat1, lon1, …].
    ///
    /// @param pointLats           Test point latitudes (degrees, WGS84) [numPoints]
    /// @param pointLons           Test point longitudes (degrees, WGS84) [numPoints]
    /// @param numPoints           Number of test points
    /// @param polygonCoords       Interleaved polygon vertex coordinates [numPolygonVertices × 2]
    ///                            Format: [lat0, lon0, lat1, lon1, ..., latN, lonN]
    /// @param numPolygonVertices  Number of polygon vertices (must be >= 3)
    /// @return Boolean vector [numPoints]; true if point is inside polygon, false otherwise
    /// @throws std::runtime_error on invalid polygon or device errors
    /// @throws std::invalid_argument if numPolygonVertices < 3 or numPoints == 0
    /// @pre The polygon forms a valid closed loop (implicit edge from last to first vertex)
    /// @note Points on the polygon boundary may return either true or false
    ///       (implementation-dependent); callers should not rely on boundary behavior
    [[nodiscard]] virtual std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) = 0;

    /// @brief Populate the frozen kernel dispatch table for this backend.
    ///
    /// Backends override this to expose their kernel function pointers for
    /// direct invocation. Null entries in the returned table indicate unsupported
    /// operations; callers should use GeoKernelFallbackDispatcher for fallback
    /// and retry semantics.
    ///
    /// @return GeoKernelDispatch with function pointers (may contain nullptr entries)
    /// @note This is called once during backend registration; results are cached
    virtual GeoKernelDispatch populateGeoDispatch() const { return {}; }
};

/// @brief Matrix backend for FP16 / BF16 matrix multiply with Tensor Core acceleration.
///
/// Specializes IComputeBackend to provide efficient batched matrix multiplication
/// (GEMM) using Tensor Cores or equivalent acceleration. Backends that do not support
/// Tensor Cores (e.g. CPU) implement the FP32 path and declare MatrixPrecision::FP32
/// as their supported precision.
///
/// @details Backends must ensure FP tolerance within 1e-5 relative error for FP16
/// operations and 1e-7 for BF16 operations when compared to FP32 CPU reference.
class IMatrixBackend : public IComputeBackend {
public:
    virtual ~IMatrixBackend() = default;

    /// @brief Compute C = alpha * A × B + beta * C (GEMM operation).
    ///
    /// Performs a batched general matrix-multiply (GEMM) operation, computing
    /// the result matrix C from input matrices A and B with optional scaling.
    ///
    /// @param params       Combined parameters: matrices (A, B, C), dimensions (M, K, N),
    ///                     scaling factors (alpha, beta), and precision mode.
    ///                     A is [M × K], B is [K × N], C is [M × N] (all row-major).
    ///                     Inputs/outputs are host pointers for CPU backends and
    ///                     device pointers for GPU backends.
    /// @param opaque_stream Backend-specific stream handle (cudaStream_t for CUDA,
    ///                      VkCommandBuffer for Vulkan, ignored for CPU; pass nullptr)
    /// @return 0 on success, non-zero error code on failure
    ///
    /// @details The @p precision field selects the arithmetic type; implementations
    /// are free to fall back to a wider type if unsupported (e.g. FP32 instead of FP16).
    /// The operation is: C := alpha * (A @ B) + beta * C
    /// When beta=0 (default), existing C values are discarded.
    /// When alpha=1 (default), no scaling is applied to the product.
    ///
    /// @note FP tolerance guarantee: results must agree with FP32 CPU baseline
    ///       within 1e-5 relative error for FP16 and 1e-7 for BF16.
    [[nodiscard]] virtual int matmul(const MatrixKernelParams& params, void* opaque_stream = nullptr) = 0;

    /// @brief Populate the frozen kernel dispatch table for this backend.
    ///
    /// Backends override this to expose their kernel function pointers.
    /// A null entry indicates the backend does not support matrix operations.
    ///
    /// @return MatrixKernelDispatch with function pointers (may contain nullptr)
    /// @note This is called once during backend registration; results are cached
    virtual MatrixKernelDispatch populateMatrixDispatch() const { return {}; }
};

/// @brief Per-type backend aggregation stored in BackendRegistry::typeIndex_.
///
/// Contains typed interface pointers for a single backend. One instance exists
/// per distinct BackendType in the registry. Typed interface pointer fields are
/// set once by registerBackend() via dynamic_cast and are then used in the hot
/// query path without further dynamic_cast overhead.
///
/// @details This structure enables efficient type-specific backend lookup by
/// caching interface pointers at registration time. If a backend does not
/// implement a specific interface (e.g., no vector operations), the corresponding
/// pointer is nullptr.
struct RegisteredBackend {
    IComputeBackend* base      = nullptr;  ///< First registered backend of this type
    IVectorBackend*  vectorPtr = nullptr;  ///< IVectorBackend interface (nullptr if unsupported)
    IGraphBackend*   graphPtr  = nullptr;  ///< IGraphBackend interface (nullptr if unsupported)
    IGeoBackend*     geoPtr    = nullptr;  ///< IGeoBackend interface (nullptr if unsupported)
    IMatrixBackend*  matrixPtr = nullptr;  ///< IMatrixBackend interface (nullptr if unsupported)
};

// ---------------------------------------------------------------------------
// Kernel registry types (forward-declared here so cpp implementations can
// provide method bodies in src/acceleration/kernel_registry.cpp).
// These are intentionally lightweight declarations matching the runtime
// behavior expected by the implementation file.
// ---------------------------------------------------------------------------

/// Per-backend kernel coverage summary used by ValidationReport
struct KernelCoverage {
    BackendType backend = BackendType::CPU;
    bool hasANN    = false;
    bool annComplete = false;
    bool hasGeo    = false;
    bool geoComplete = false;
    bool hasMatrix = false;
    bool matrixComplete = false;
    std::vector<std::string> missingSlots;
};

/// ValidationReport summarises KernelCoverage for all registered backends
struct ValidationReport {
    std::vector<KernelCoverage> entries;

    [[nodiscard]] bool allComplete() const noexcept {
        for (const auto& e : entries) {
            if ((e.hasANN && !e.annComplete) ||
                (e.hasGeo && !e.geoComplete) ||
                (e.hasMatrix && !e.matrixComplete)) return false;
        }
        return true;
    }
    std::string summary() const; // implemented in kernel_registry.cpp
};

/// Central registry holding frozen kernel dispatch tables per BackendType
class KernelRegistry {
public:
    ANNKernelDispatch getANNDispatch(BackendType t) const noexcept {
        auto it = annDispatch_.find(t);
        return (it != annDispatch_.end()) ? it->second : ANNKernelDispatch{};
    }
    GeoKernelDispatch getGeoDispatch(BackendType t) const noexcept {
        auto it = geoDispatch_.find(t);
        return (it != geoDispatch_.end()) ? it->second : GeoKernelDispatch{};
    }

    void registerANNDispatch(BackendType t, const ANNKernelDispatch& d) {
        annDispatch_[t] = d;
    }
    void registerGeoDispatch(BackendType t, const GeoKernelDispatch& d) {
        geoDispatch_[t] = d;
    }
    void registerMatrixDispatch(BackendType t, const MatrixKernelDispatch& d) {
        matrixDispatch_[t] = d;
    }

    ANNKernelDispatch lookupANNWithFallback(BackendType primary) const noexcept;
    GeoKernelDispatch lookupGeoWithFallback(BackendType primary) const noexcept;
    std::vector<BackendType> registeredBackends() const;
    ValidationReport validate() const;

private:
    std::unordered_map<BackendType, ANNKernelDispatch>    annDispatch_;
    std::unordered_map<BackendType, GeoKernelDispatch>    geoDispatch_;
    std::unordered_map<BackendType, MatrixKernelDispatch> matrixDispatch_;
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

/// @brief Capability requirements for capability-driven backend selection.
///
/// Specifies minimum capability thresholds for backend selection. Zero / NONE / false
/// fields are "don't-care" — they impose no constraint. Backends that satisfy all
/// requirements are considered viable candidates.
///
/// @details This structure is used with BackendRegistry::selectBackendFor() and
/// related methods to find a backend that meets specific operational requirements.
struct CapabilityRequirements {
    bool needsVectorOps = false;      ///< Must support vector (ANN) operations
    bool needsGraphOps  = false;      ///< Must support graph traversal (BFS, Dijkstra)
    bool needsGeoOps    = false;      ///< Must support geospatial operations
    bool needsMatrixOps = false;      ///< Must support FP16/BF16 matrix multiply
    bool needsBatch     = false;      ///< Must support batch processing
    bool needsAsync     = false;      ///< Must support asynchronous execution

    /// @brief All listed PrecisionMode flags must be present in backend's supportedPrecisions.
    /// Combine flags with operator| (e.g., PrecisionMode::FP32 | PrecisionMode::FP16).
    PrecisionMode requiredPrecisions = PrecisionMode::NONE;

    /// @brief All listed DistanceMetric bits (via metricBit()) must be present
    /// in backend's supportedMetrics. Use metricBit(METRIC) to construct bitmask.
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
    KernelRegistry kernelRegistry_;

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
    [[nodiscard]] virtual DeviceCapabilityFlags queryCapabilities(int device_index) const noexcept = 0;

    /**
     * @brief Query capabilities for all enumerated devices.
     *
     * @return One `DeviceCapabilityFlags` entry per device in driver
     *         enumeration order.  An empty vector is returned when no GPU
     *         driver is present.
     */
    [[nodiscard]] virtual std::vector<DeviceCapabilityFlags> queryAll() const = 0;
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
    [[nodiscard]] virtual DeviceSet selectDevices(const WorkloadDescriptor& workload) const = 0;

    /**
     * @brief Returns the number of GPU devices visible to this selector.
     *
     * Thread-safe.
     */
    [[nodiscard]] virtual uint32_t deviceCount() const noexcept = 0;
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
    [[nodiscard]] virtual bool registerKernel(std::string name, void* fn_ptr) = 0;

    /**
     * @brief Resolve a named kernel to its function pointer.
     *
     * @param name  Kernel identifier string.
     * @return Function pointer cast to `void*`, or `nullptr` if not registered.
     */
    [[nodiscard]] virtual void* resolveKernel(const std::string& name) const = 0;

    /**
     * @brief Returns true if a kernel with @p name is registered.
     *
     * Thread-safe (read-only lookup).
     */
    [[nodiscard]] virtual bool hasKernel(const std::string& name) const noexcept = 0;

    /**
     * @brief Remove a kernel from the registry.
     *
     * @return true if the kernel was present and removed; false otherwise.
     */
    [[nodiscard]] virtual bool deregisterKernel(const std::string& name) noexcept = 0;
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
    [[nodiscard]] virtual ComputeFuture<SimilarityKernelResult>
    submit(const KernelDescriptor& descriptor,
           CancellationToken        token = {}) = 0;
};

} // namespace acceleration
} // namespace themis
