/**
 * @file compute_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

#if defined(__clang__)
#define THEMIS_FLAG_ENUM __attribute__((flag_enum))
#else
#define THEMIS_FLAG_ENUM
#endif

enum class THEMIS_FLAG_ENUM PrecisionMode : uint32_t {
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

#undef THEMIS_FLAG_ENUM

inline constexpr PrecisionMode operator|(PrecisionMode a, PrecisionMode b) noexcept {
    return static_cast<PrecisionMode>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

inline constexpr bool hasPrecision(PrecisionMode modes, PrecisionMode flag) noexcept {
    return (static_cast<uint32_t>(modes) & static_cast<uint32_t>(flag)) != 0;
}

inline constexpr uint32_t metricBit(DistanceMetric m) noexcept {
    return 1u << static_cast<uint32_t>(m);
}

namespace vendor_id {
    static constexpr uint32_t NVIDIA   = 0x10DE;  ///< NVIDIA Corporation
    static constexpr uint32_t AMD      = 0x1002;  ///< Advanced Micro Devices (AMD)
    static constexpr uint32_t INTEL    = 0x8086;  ///< Intel Corporation
    static constexpr uint32_t ARM      = 0x13B5;  ///< ARM Holdings
    static constexpr uint32_t QUALCOMM = 0x5143;  ///< Qualcomm Incorporated
    static constexpr uint32_t IMGTEC   = 0x1010;  ///< Imagination Technologies
} // namespace vendor_id

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
};

struct BackendCapabilities {
    bool supportsVectorOps = false;           ///< Backend supports vector similarity (ANN) operations
    bool supportsGraphOps = false;            ///< Backend supports graph traversal (BFS, shortest-path)
    bool supportsGeoOps = false;              ///< Backend supports geospatial operations (distance, containment)
    bool supportsMatrixOps = false;           ///< Backend supports FP16/BF16 matrix multiply via Tensor Core
    bool supportsBatchProcessing = false;     ///< Backend can process multiple independent queries in parallel
    bool supportsAsync = false;               ///< Backend supports asynchronous (non-blocking) execution
    bool supportsAiInference = false;         ///< Backend supports dedicated AI inference (NPU/ONNX Runtime)

    PrecisionMode supportedPrecisions = PrecisionMode::NONE;

    uint32_t supportedMetrics = 0;

    size_t maxMemoryBytes = 0;                ///< Available VRAM/host memory in bytes
    int computeUnits = 0;                     ///< Number of compute units/SMs (0 for CPU)
    std::string deviceName;                   ///< Human-readable device name (e.g. "RTX 4090")
    std::string vendorName;

    uint32_t    npuTops           = 0;        ///< Reported peak throughput in TOPS (0 = unknown or CPU)
    std::string preferredOnnxEP;              ///< ONNX Runtime execution provider (empty = CPU fallback)
};

struct BackendHealthStatus {
    std::string status;

    bool healthy  = false;
    
    bool ready    = false;
    
    bool alive    = false;

    std::string message;

    std::vector<std::string> issues;

    std::string deviceName;           ///< GPU/device model name (e.g. "RTX 4090")
    std::string driverInfo;           ///< Driver version or runtime info

    size_t memoryUsedBytes      = 0;  ///< Memory currently in use (bytes)
    size_t memoryAvailableBytes = 0;  ///< Available free memory (bytes)

    
    static BackendHealthStatus makeHealthy(const std::string& device = "") {
        BackendHealthStatus s;
        s.status  = "healthy";
        s.healthy = s.ready = s.alive = true;
        s.message    = "Backend is operational";
        s.deviceName = device;
        return s;
    }

    /**
     * @brief Make Degraded.
     * @param[in] issue Input parameter.
     * @return Return value.
     * @details Calls: push_back().
     */
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

    /**
     * @brief Make Unhealthy.
     * @param[in] issue Input parameter.
     * @return Return value.
     * @details Calls: push_back().
     */
    static BackendHealthStatus makeUnhealthy(const std::string& issue) {
        BackendHealthStatus s;
        s.status  = "unhealthy";
        s.healthy = s.ready = s.alive = false;
        s.message = issue;
        s.issues.push_back(issue);
        return s;
    }
};

struct SimilarityKernelResult {
    std::vector<std::vector<std::pair<uint32_t, float>>> results;

    DistanceMetric metric_used    = DistanceMetric::L2;  ///< Distance metric used for computation
    PrecisionMode  precision_used = PrecisionMode::FP32; ///< Floating-point precision used
    bool           used_hw_path   = false;               ///< True if GPU/hardware path was used
    double         speedup_vs_cpu = 1.0;                 ///< Speedup ratio: CPU time / GPU time
};

struct BatchDescriptor {
    const float* queries     = nullptr;  ///< Query matrix [numQueries × dim] (row-major)
    size_t       num_queries = 0;        ///< Number of query vectors
    size_t       dim         = 0;        ///< Dimensionality of each vector
    const float* vectors     = nullptr;  ///< Database/corpus matrix [numVectors × dim] (row-major)
    size_t       num_vectors = 0;        ///< Number of database vectors
    size_t       k           = 1;        ///< Number of nearest neighbors to retrieve
};

struct KernelConfig {
    uint32_t       block_size = 256;         ///< Thread block/work group size
    uint32_t       grid_size  = 0;           ///< Number of blocks/work groups (0 = auto-calculate)
    uint32_t       shared_mem = 0;           ///< Shared memory per block (bytes; GPU only)
    DistanceMetric metric     = DistanceMetric::L2;  ///< Distance metric to use
    PrecisionMode  precision  = PrecisionMode::FP32; ///< Floating-point precision
    bool           async_exec = false;       ///< True for asynchronous execution (non-blocking)
};

class IComputeBackend {
public:
    /**
     * @brief ICompute Backend.
     * @return Return value.
     */
    virtual ~IComputeBackend() = default;
    
    
    [[nodiscard]] virtual const char* name() const noexcept = 0;
    
    [[nodiscard]] virtual BackendType type() const noexcept = 0;
    
    [[nodiscard]] virtual bool isAvailable() const noexcept = 0;
    
    
    [[nodiscard]] virtual BackendCapabilities getCapabilities() const = 0;
    
    
    [[nodiscard]] virtual bool initialize() = 0;
    
    /**
     * @brief Shutdown.
     */
    virtual void shutdown() = 0;
    
    
    virtual ErrorContext getLastError() const {
        return lastError_;
    }

    
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
                if (token.is_cancelled()) {
                  break;
                }
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
    
    /**
     * @brief Set Error.
     * @param[in] error Input parameter.
     * @details Calls: std::move().
     */
    void setError(ErrorContext error) {
        lastError_ = std::move(error);
    }
    
    /**
     * @brief Clear Error.
     * @details Calls: ErrorContext(), name().
     */
    void clearError() {
        lastError_ = ErrorContext(
            AccelerationErrorCode::Success,
            name(),
            ""
        );
    }
    
protected:
    ErrorContext lastError_;  ///< Last error context (stored for programmatic access)
};

struct KnnQueryResult {
    std::vector<std::pair<uint32_t, float>> neighbors;
    
    AccelerationErrorCode status   = AccelerationErrorCode::Success;
    
    std::string           errorMessage;
};

struct PartialBatchResult {
    std::vector<KnnQueryResult> queryResults;
    
    size_t successCount = 0;
    
    size_t failureCount = 0;
};

class IVectorBackend : public IComputeBackend {
public:
    /**
     * @brief IVector Backend.
     * @return Return value.
     */
    virtual ~IVectorBackend() = default;
    
    [[nodiscard]] virtual std::vector<float> computeDistances(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        bool useL2 = true
    ) = 0;
    
    [[nodiscard]] virtual std::vector<std::vector<std::pair<uint32_t, float>>> batchKnnSearch(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    ) = 0;

    virtual PartialBatchResult batchKnnSearchSafe(
        const float* queries,
        size_t numQueries,
        size_t dim,
        const float* vectors,
        size_t numVectors,
        size_t k,
        bool useL2 = true
    );

    virtual ANNKernelDispatch populateANNDispatch() const { return {}; }
};

class IGraphBackend : public IComputeBackend {
public:
    /**
     * @brief IGraph Backend.
     * @return Return value.
     */
    virtual ~IGraphBackend() = default;
    
    [[nodiscard]] virtual std::vector<std::vector<uint32_t>> batchBFS(
        const uint32_t* adjacency,
        size_t numVertices,
        const uint32_t* startVertices,
        size_t numStarts,
        uint32_t maxDepth
    ) = 0;
    
    [[nodiscard]] virtual std::vector<std::vector<uint32_t>> batchShortestPath(
        const uint32_t* adjacency,
        const float* weights,
        size_t numVertices,
        const uint32_t* startVertices,
        const uint32_t* endVertices,
        size_t numPairs
    ) = 0;
};

class IGeoBackend : public IComputeBackend {
public:
    /**
     * @brief IGeo Backend.
     * @return Return value.
     */
    virtual ~IGeoBackend() = default;
    
    [[nodiscard]] virtual std::vector<float> batchDistances(
        const double* latitudes1,
        const double* longitudes1,
        const double* latitudes2,
        const double* longitudes2,
        size_t count,
        bool useHaversine = true
    ) = 0;
    
    [[nodiscard]] virtual std::vector<bool> batchPointInPolygon(
        const double* pointLats,
        const double* pointLons,
        size_t numPoints,
        const double* polygonCoords,
        size_t numPolygonVertices
    ) = 0;

    virtual GeoKernelDispatch populateGeoDispatch() const { return {}; }
};

class IMatrixBackend : public IComputeBackend {
public:
    /**
     * @brief IMatrix Backend.
     * @return Return value.
     */
    virtual ~IMatrixBackend() = default;

    [[nodiscard]] virtual int matmul(const MatrixKernelParams& params, void* opaque_stream = nullptr) = 0;

    virtual MatrixKernelDispatch populateMatrixDispatch() const { return {}; }
};

struct RegisteredBackend {
    IComputeBackend* base      = nullptr;  ///< First registered backend of this type
    IVectorBackend*  vectorPtr = nullptr;  ///< IVectorBackend interface (nullptr if unsupported)
    IGraphBackend*   graphPtr  = nullptr;  ///< IGraphBackend interface (nullptr if unsupported)
    IGeoBackend*     geoPtr    = nullptr;  ///< IGeoBackend interface (nullptr if unsupported)
    IMatrixBackend*  matrixPtr = nullptr;  ///< IMatrixBackend interface (nullptr if unsupported)
};

// =============================================================================
// KernelRegistry — dispatch table management for compute kernels
// =============================================================================

struct KernelCoverage {
    BackendType backend;              ///< Backend type being tracked
    bool hasANN = false;              ///< Has ANN (vector) kernels registered
    bool hasGeo = false;              ///< Has geo kernels registered
    bool hasMatrix = false;           ///< Has matrix kernels registered
    bool annComplete = false;         ///< All ANN kernels implemented
    bool geoComplete = false;         ///< All geo kernels implemented
    bool matrixComplete = false;      ///< All matrix kernels implemented
    std::vector<std::string> missingSlots;  ///< Missing or incomplete kernel implementations
};

struct ValidationReport {
    std::vector<KernelCoverage> entries;

    [[nodiscard]] bool allComplete() const noexcept {
        if (entries.empty()) {
          return true;
        }
        for (const auto& e : entries) {
            if (e.hasANN && !e.annComplete) {
              return false;
            }
            if (e.hasGeo && !e.geoComplete) {
              return false;
            }
            if (e.hasMatrix && !e.matrixComplete) {
              return false;
            }
        }
        return true;
    }

    /**
     * @brief Summary.
     * @return Return value.
     */
    std::string summary() const;
};

class KernelRegistry {
public:
    KernelRegistry() = default;
    ~KernelRegistry() = default;

    /**
     * @brief Register ANNDispatch.
     * @param[in] type Input parameter.
     * @param[in] dispatch Input parameter.
     * @details Implements registerANNDispatch without additional internal calls.
     */
    void registerANNDispatch(BackendType type, const ANNKernelDispatch& dispatch) {
        annDispatch_[type] = dispatch;
    }

    /**
     * @brief Register Geo Dispatch.
     * @param[in] type Input parameter.
     * @param[in] dispatch Input parameter.
     * @details Implements registerGeoDispatch without additional internal calls.
     */
    void registerGeoDispatch(BackendType type, const GeoKernelDispatch& dispatch) {
        geoDispatch_[type] = dispatch;
    }

    /**
     * @brief Register Matrix Dispatch.
     * @param[in] type Input parameter.
     * @param[in] dispatch Input parameter.
     * @details Implements registerMatrixDispatch without additional internal calls.
     */
    void registerMatrixDispatch(BackendType type, const MatrixKernelDispatch& dispatch) {
        matrixDispatch_[type] = dispatch;
    }

    [[nodiscard]] ANNKernelDispatch getANNDispatch(BackendType type) const noexcept {
        auto it = annDispatch_.find(type);
        return (it != annDispatch_.end()) ? it->second : ANNKernelDispatch{};
    }

    [[nodiscard]] GeoKernelDispatch getGeoDispatch(BackendType type) const noexcept {
        auto it = geoDispatch_.find(type);
        return (it != geoDispatch_.end()) ? it->second : GeoKernelDispatch{};
    }

    [[nodiscard]] MatrixKernelDispatch getMatrixDispatch(BackendType type) const noexcept {
        auto it = matrixDispatch_.find(type);
        return (it != matrixDispatch_.end()) ? it->second : MatrixKernelDispatch{};
    }

    [[nodiscard]] ANNKernelDispatch lookupANNWithFallback(BackendType primary) const noexcept;

    [[nodiscard]] GeoKernelDispatch lookupGeoWithFallback(BackendType primary) const noexcept;

    [[nodiscard]] std::vector<BackendType> registeredBackends() const;

    [[nodiscard]] ValidationReport validate() const;

    [[nodiscard]] bool hasANNDispatch(BackendType type) const noexcept {
        return annDispatch_.find(type) != annDispatch_.end();
    }

    /**
     * @brief Clear.
     * @details Implements clear without additional internal calls.
     */
    void clear() {
        annDispatch_.clear();
        geoDispatch_.clear();
        matrixDispatch_.clear();
    }

private:
    std::unordered_map<BackendType, ANNKernelDispatch> annDispatch_;
    std::unordered_map<BackendType, GeoKernelDispatch> geoDispatch_;
    std::unordered_map<BackendType, MatrixKernelDispatch> matrixDispatch_;
};

// Forward declaration
class PluginLoader;

// Backend registry for managing different acceleration backends
class BackendRegistry {
public:
    /**
     * @brief Instance.
     * @return Return value.
     */
    static BackendRegistry& instance();
    
    /**
     * @brief Register a backend (manual registration)
     * @param[in] backend Input parameter.
     */
    void registerBackend(std::unique_ptr<IComputeBackend> backend);
    
    /**
     * @brief Load plugins from directory (DLL/SO files) Returns number of plugins loaded
     * @param[in] pluginDirectory Input parameter.
     * @return Return value.
     */
    size_t loadPlugins(const std::string& pluginDirectory);
    
    /**
     * @brief Load a specific plugin
     * @param[in] pluginPath Input parameter.
     * @return True when the operation succeeds.
     */
    bool loadPlugin(const std::string& pluginPath);
    
    /**
     * @brief Get backend by type
     * @param[in] type Input parameter.
     * @return Pointer to the result.
     */
    IComputeBackend* getBackend(BackendType type) const;
    
    /**
     * @brief Get best available backend for a capability
     * @return Pointer to the result.
     */
    IVectorBackend* getBestVectorBackend() const;
    /**
     * @brief Get Best Graph Backend.
     * @return Pointer to the result.
     */
    IGraphBackend* getBestGraphBackend() const;
    /**
     * @brief Get Best Geo Backend.
     * @return Pointer to the result.
     */
    IGeoBackend* getBestGeoBackend() const;
    /**
     * @brief Get Best Matrix Backend.
     * @return Pointer to the result.
     */
    IMatrixBackend* getBestMatrixBackend() const;
    
    /**
     * @brief Auto-detect and initialize all available backends
     */
    void autoDetect();
    
    /**
     * @brief List all available backends
     * @return Return value.
     */
    std::vector<BackendType> getAvailableBackends() const;

    /**
     * @brief Returns the ordered fallback chain used when selecting the best backend.
     * @return Return value.
     * @note Exception safety: noexcept.
     * @details The first element has the highest priority; BackendType::CPU is always last. All getBestXBackend() methods traverse this chain in order.
     */
    static const std::vector<BackendType>& getFallbackOrder() noexcept;

    // ---------------------------------------------------------------------------
    // Capability-driven selection
    // ---------------------------------------------------------------------------

struct CapabilityRequirements {
    bool needsVectorOps = false;      ///< Must support vector (ANN) operations
    bool needsGraphOps  = false;      ///< Must support graph traversal (BFS, Dijkstra)
    bool needsGeoOps    = false;      ///< Must support geospatial operations
    bool needsMatrixOps = false;      ///< Must support FP16/BF16 matrix multiply
    bool needsBatch     = false;      ///< Must support batch processing
    bool needsAsync     = false;      ///< Must support asynchronous execution

    PrecisionMode requiredPrecisions = PrecisionMode::NONE;

    uint32_t requiredMetrics = 0;
};

    static inline bool satisfies(const BackendCapabilities& caps,
                                  const CapabilityRequirements& reqs) noexcept {
        if (reqs.needsVectorOps && !caps.supportsVectorOps) {
          return false;
        }
        if (reqs.needsGraphOps  && !caps.supportsGraphOps) {
          return false;
        }
        if (reqs.needsGeoOps    && !caps.supportsGeoOps) {
          return false;
        }
        if (reqs.needsMatrixOps && !caps.supportsMatrixOps) {
          return false;
        }
        if (reqs.needsBatch     && !caps.supportsBatchProcessing) {
          return false;
        }
        if (reqs.needsAsync     && !caps.supportsAsync) {
          return false;
        }
        const auto reqP = static_cast<uint32_t>(reqs.requiredPrecisions);
        const auto hasP = static_cast<uint32_t>(caps.supportedPrecisions);
        if ((reqP & hasP) != reqP) {
          return false;
        }
        if ((reqs.requiredMetrics & caps.supportedMetrics) != reqs.requiredMetrics) {
          return false;
        }
        return true;
    }

    /**
     * @brief Select Backend For.
     * @param[in] reqs Input parameter.
     * @return Pointer to the result.
     */
    IComputeBackend* selectBackendFor(const CapabilityRequirements& reqs) const;

    /**
     * @brief Select Vector Backend For.
     * @param[in] reqs Input parameter.
     * @return Pointer to the result.
     */
    IVectorBackend* selectVectorBackendFor(const CapabilityRequirements& reqs) const;

    /**
     * @brief Select Graph Backend For.
     * @param[in] reqs Input parameter.
     * @return Pointer to the result.
     */
    IGraphBackend* selectGraphBackendFor(const CapabilityRequirements& reqs) const;

    /**
     * @brief Select Geo Backend For.
     * @param[in] reqs Input parameter.
     * @return Pointer to the result.
     */
    IGeoBackend* selectGeoBackendFor(const CapabilityRequirements& reqs) const;

    /**
     * @brief Select Matrix Backend For.
     * @param[in] reqs Input parameter.
     * @return Pointer to the result.
     */
    IMatrixBackend* selectMatrixBackendFor(const CapabilityRequirements& reqs) const;

    // ---------------------------------------------------------------------------
    // Runtime startup initialization
    // ---------------------------------------------------------------------------

    void initializeRuntime(
        const CapabilityRequirements& vectorReqs = defaultVectorRequirements(),
        const CapabilityRequirements& graphReqs  = defaultGraphRequirements(),
        const CapabilityRequirements& geoReqs    = defaultGeoRequirements());

    /**
     * @brief Get Selected Vector Backend.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    IVectorBackend* getSelectedVectorBackend() const noexcept;

    /**
     * @brief Get Selected Graph Backend.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    IGraphBackend* getSelectedGraphBackend() const noexcept;

    /**
     * @brief Get Selected Geo Backend.
     * @return Pointer to the result.
     * @note Exception safety: noexcept.
     */
    IGeoBackend* getSelectedGeoBackend() const noexcept;

    /**
     * @brief Is Runtime Initialized.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isRuntimeInitialized() const noexcept;

    /**
     * @brief Default capability requirements used by initializeRuntime() when the caller does not supply explicit requirements.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static CapabilityRequirements defaultVectorRequirements() noexcept;
    /**
     * @brief Default Graph Requirements.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static CapabilityRequirements defaultGraphRequirements() noexcept;
    /**
     * @brief Default Geo Requirements.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static CapabilityRequirements defaultGeoRequirements() noexcept;

    /**
     * @brief Device Info.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::vector<DeviceCapabilityInfo> deviceInfo() const noexcept;

    // ---------------------------------------------------------------------------
    // Kernel registry access
    // ---------------------------------------------------------------------------

    [[nodiscard]] ValidationReport validateKernels() const;

    [[nodiscard]] const KernelRegistry& getKernelRegistry() const noexcept;

    // Shutdown all backends
    /**
     * @brief Shutdown All.
     */
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

struct DeviceSet {
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

    bool push(uint32_t device) noexcept {
        if (count >= kMaxDevices) {
          return false;
        }
        devices[count++] = device;
        return true;
    }
};

// =============================================================================
// WorkloadDescriptor — hints for IMultiGPUSelector::selectDevices()
// =============================================================================

enum class LatencyClass : uint8_t {
    INTERACTIVE = 0,  ///< Latency-critical (< 1 ms); minimise queue depth
    BATCH       = 1,  ///< Throughput-oriented; minutes; GPU saturation preferred
    BACKGROUND  = 2,  ///< Non-interactive; accept long queue wait times
};

struct WorkloadDescriptor {
    size_t       byte_size     = 0;                     ///< Total input data size in bytes
    uint64_t     flop_estimate = 0;                     ///< Estimated FLOPs (0 = unknown)
    LatencyClass latency_class = LatencyClass::BATCH;   ///< Scheduling priority hint
    PrecisionMode precision    = PrecisionMode::FP32;   ///< Required precision mode
};

// =============================================================================
// BatchDescriptor — input/output shape for kernel dispatch
// =============================================================================

// BatchDescriptor is defined before IComputeBackend.

// =============================================================================
// KernelConfig — plain-data runtime parameters for a compute kernel
// =============================================================================

// KernelConfig is defined before IComputeBackend.

// =============================================================================
// KernelDescriptor — combined batch + config + optional named kernel
// =============================================================================

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

class IDeviceCapabilityQuery {
public:
    /**
     * @brief IDevice Capability Query.
     * @return Return value.
     */
    virtual ~IDeviceCapabilityQuery() = default;

    [[nodiscard]] virtual DeviceCapabilityFlags queryCapabilities(int device_index) const noexcept = 0;

    [[nodiscard]] virtual std::vector<DeviceCapabilityFlags> queryAll() const = 0;
};

// =============================================================================
// IMultiGPUSelector — thread-safe workload-to-device mapping
// =============================================================================

class IMultiGPUSelector {
public:
    /**
     * @brief IMulti GPUSelector.
     * @return Return value.
     */
    virtual ~IMultiGPUSelector() = default;

    [[nodiscard]] virtual DeviceSet selectDevices(const WorkloadDescriptor& workload) const = 0;

    [[nodiscard]] virtual uint32_t deviceCount() const noexcept = 0;
};

// =============================================================================
// IKernelRegistry — named compute kernel lookup table
// =============================================================================

class IKernelRegistry {
public:
    /**
     * @brief IKernel Registry.
     * @return Return value.
     */
    virtual ~IKernelRegistry() = default;

    [[nodiscard]] virtual bool registerKernel(std::string name, void* fn_ptr) = 0;

    [[nodiscard]] virtual void* resolveKernel(const std::string& name) const = 0;

    [[nodiscard]] virtual bool hasKernel(const std::string& name) const noexcept = 0;

    [[nodiscard]] virtual bool deregisterKernel(const std::string& name) noexcept = 0;
};

// =============================================================================
// IAsyncComputeDispatch — non-blocking kernel submission
// =============================================================================

class IAsyncComputeDispatch {
public:
    /**
     * @brief IAsync Compute Dispatch.
     * @return Return value.
     */
    virtual ~IAsyncComputeDispatch() = default;

    [[nodiscard]] virtual ComputeFuture<SimilarityKernelResult>
    submit(const KernelDescriptor& descriptor,
           CancellationToken        token = {}) = 0;
};

} // namespace acceleration
} // namespace themis
