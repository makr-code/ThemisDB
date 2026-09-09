/**
 * @file ai_hardware_dispatcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <shared_mutex>
#include <optional>
#include <string>
#include <vector>

#include "acceleration/compute_backend.h"
#include "acceleration/kernel_invocation.h"

// ── Compile-time guards ───────────────────────────────────────────────────────
// Each guard can be overridden from the CMake command line:
//   -DTHEMIS_DISABLE_NPU_APPLE=ON   disable Apple Neural Engine
//   -DTHEMIS_DISABLE_NPU_INTEL=ON   disable Intel NPU / OpenVINO
//   -DTHEMIS_DISABLE_NPU_QUALCOMM=ON
//   -DTHEMIS_DISABLE_NPU_ARM=ON
//   -DTHEMIS_DISABLE_NNAPI=ON       disable Android NNAPI
//   -DTHEMIS_DISABLE_ONNX_RUNTIME=ON

#if defined(__APPLE__) && !defined(THEMIS_DISABLE_NPU_APPLE)
#  define THEMIS_HAS_NPU_APPLE 1
#endif

#if defined(__linux__) && defined(THEMIS_ENABLE_NPU_INTEL) && !defined(THEMIS_DISABLE_NPU_INTEL)
#  define THEMIS_HAS_NPU_INTEL 1
#endif

#if (defined(__linux__) || defined(_WIN32)) && defined(THEMIS_ENABLE_NPU_QUALCOMM) && \
    !defined(THEMIS_DISABLE_NPU_QUALCOMM)
#  define THEMIS_HAS_NPU_QUALCOMM 1
#endif

#if defined(__linux__) && defined(THEMIS_ENABLE_NPU_ARM) && !defined(THEMIS_DISABLE_NPU_ARM)
#  define THEMIS_HAS_NPU_ARM 1
#endif

#if defined(__ANDROID__) && !defined(THEMIS_DISABLE_NNAPI)
#  define THEMIS_HAS_NNAPI 1
#endif

#if !defined(THEMIS_DISABLE_ONNX_RUNTIME)
#  define THEMIS_HAS_ONNX_RUNTIME 1
#endif

namespace themis {
namespace acceleration {

/// @brief AI inference request specifying model, input data, and routing preferences.
///
/// This struct describes a single AI inference task to be routed through the dispatcher.
/// The dispatcher fills the `chosen_backend` and `chosen_ep` fields before execution.
struct AiInferenceRequest {
    /// @brief Input tensor on host memory
    const float*  input_data     = nullptr;  ///< Host-side input tensor (FP32 array)
    size_t        input_elements = 0;        ///< Total number of scalar elements in tensor
    std::vector<int64_t> input_shape;        ///< Tensor shape dimensions (e.g., {1, 512})

    /// @brief Model and task identification
    std::string   model_path;                ///< Path to model file (.onnx, .coreml, .dlc, etc.)
    std::string   task_tag;                  ///< Task category: "embedding", "rerank", "classify", "generate", etc.

    /// @brief Precision preference for execution
    /// Dispatcher picks the highest-capability backend that satisfies this constraint.
    /// Defaults to FP32 for broadest compatibility across all backends.
    PrecisionMode preferred_precision = PrecisionMode::FP32;

    /// @brief Routing decision made by dispatcher (output fields)
    BackendType   chosen_backend  = BackendType::CPU;  ///< Backend selected by dispatcher
    std::string   chosen_ep;                           ///< ONNX EP or platform-specific identifier used

    /// @brief Optional vector-similarity request payload.
    ///
    /// If `task_tag` is set to one of:
    /// - `"vector_similarity_l2"`
    /// - `"vector_similarity_cosine"`
    /// - `"vector_similarity_ip"`
    ///
    /// then `input_data` is interpreted as a row-major query matrix
    /// `[similarity_num_queries × similarity_dim]` and `similarity_corpus`
    /// is interpreted as a row-major corpus matrix
    /// `[similarity_num_vectors × similarity_dim]`.
    ///
    /// Failure and edge cases:
    /// - Any null pointer, zero size, or shape mismatch is rejected.
    /// - `similarity_top_k` is clamped to `similarity_num_vectors`.
    const float* similarity_corpus      = nullptr;                  ///< Corpus matrix [numVectors × dim]
    size_t       similarity_num_queries = 1;                        ///< Number of query vectors
    size_t       similarity_num_vectors = 0;                        ///< Number of corpus vectors
    size_t       similarity_dim         = 0;                        ///< Shared vector dimensionality
    size_t       similarity_top_k       = 1;                        ///< Number of neighbours to return
    DistanceMetric similarity_metric    = DistanceMetric::L2;       ///< Metric used for vector-similarity path
};

/// @brief Result of an AI inference operation with output, timing, and backend information.
struct AiInferenceResult {
    std::vector<float>   output;              ///< Host-side output tensor (FP32)
    std::vector<int64_t> output_shape;        ///< Output tensor shape dimensions
    bool                 success      = false; ///< true if inference completed successfully
    std::string          error;               ///< Error message if success is false
    BackendType          backend_used = BackendType::CPU;  ///< Backend that executed the model
    std::string          ep_used;             ///< Execution provider/backend identifier
    double               latency_ms   = 0.0;  ///< Wall-clock inference time in milliseconds

    /// @brief Optional ANN/top-k outputs for vector-similarity requests.
    ///
    /// Layout:
    /// - `topk_indices`: `[numQueries × effectiveK]` row-major
    /// - `topk_distances`: `[numQueries × effectiveK]` row-major
    std::vector<uint32_t> topk_indices;
    std::vector<float>    topk_distances;
};

/// @brief Hardware capability and availability snapshot for an AI acceleration backend.
///
/// Returned by AiHardwareDispatcher::probeCapabilities() to describe the features
/// and availability of each hardware backend.
struct AiHardwareCapability {
    BackendType type             = BackendType::CPU;  ///< Type of acceleration hardware
    std::string name;                                  ///< Human-readable backend identifier
    bool        available        = false;              ///< true if hardware is present and working
    uint32_t    tops             = 0;                  ///< Peak throughput estimate (Tera-OPerations Per Second)
    PrecisionMode supported_precisions = PrecisionMode::FP32;  ///< Bitmask of supported precision modes
    std::string onnx_ep;                               ///< ONNX Runtime execution provider name (empty for non-ONNX)
    std::string error;                                 ///< Error message when available == false
};

// =============================================================================
// AiHardwareDispatcher
// =============================================================================
/**
 * @brief Universal AI-hardware dispatch layer.
 *
 * Singleton; thread-safe.  Probes all supported AI hardware backends at
 * startup, ranks them by throughput potential, and exposes a unified
 * run() method that dispatches inference to the best available backend.
 *
 * Fallback chain (highest priority first):
 *   NPU_APPLE → NPU_INTEL → NPU_QUALCOMM → NPU_ARM → NNAPI
 *   → ONNX_RUNTIME (selects CUDA/DirectML/CoreML/TensorRT/QNN/OpenVINO EP)
 *   → GPU via BackendRegistry (CUDA / HIP / Vulkan / Metal …)
 *   → CPU (AVX-512 / AVX2 / NEON SIMD)
 *
 * All backend probes are lazy and graceful: a backend that fails to
 * initialise (driver absent, hardware not present, library not linked) is
 * transparently skipped without impacting the next candidate.
 *
 * Usage:
 * @code
 *   auto& d = AiHardwareDispatcher::instance();
 *   d.initialize();                          // one-time startup probe
 *
 *   AiInferenceRequest req;
 *   req.model_path        = "model.onnx";
 *   req.input_data        = embeddings.data();
 *   req.input_elements    = embeddings.size();
 *   req.input_shape       = {1, 512};
 *   req.task_tag          = "embedding";
 *   req.preferred_precision = PrecisionMode::FP16;
 *
 *   AiInferenceResult res = d.run(req);
 *   if (res.success) { // use res.output
 *   }
 * @endcode
 */
class AiHardwareDispatcher {
public:
    using AppleANEDispatchFn = std::function<AiInferenceResult(AiInferenceRequest&)>;

    /// Probe TTL: re-probe hardware if the cache is older than this.
    static constexpr auto kCacheTTL = std::chrono::seconds(120);

    /// @brief Singleton accessor
    /// @return Reference to global AiHardwareDispatcher instance
    static AiHardwareDispatcher& instance();

    /// @brief Probe all AI hardware backends and build the priority chain
    ///
    /// Performs automatic discovery of all supported AI acceleration hardware.
    /// Safe to call multiple times — subsequent calls are no-ops unless force == true
    /// or the cache TTL has expired.
    ///
    /// @param force  Re-probe all backends even if cache is still valid; default false
    void initialize(bool force = false);

    /// @brief Get capability snapshot for every probed backend in priority order
    /// @return Vector of AiHardwareCapability structs ordered by priority (best first)
    std::vector<AiHardwareCapability> probeCapabilities();

    /// @brief Get the highest-priority available backend type
    /// @return BackendType of the best available accelerator (CPU if no GPU/NPU available)
    /// @note Does not throw; always returns a valid type
    BackendType bestBackend() const noexcept;

    /// @brief Get ONNX Runtime execution provider for best backend
    /// @return ONNX Runtime EP name (e.g., "CudaExecutionProvider"); empty string if not available
    std::string bestOnnxEP() const;

    /// @brief Check if at least one non-CPU AI accelerator is available
    /// @return true if GPU, NPU, or other dedicated accelerator is present and working
    bool hasAccelerator() const noexcept;

    /// @brief Check if a dedicated NPU (Apple ANE / Intel / Qualcomm / ARM) is available
    /// @return true if specialized neural processing unit is detected and functional
    bool hasNPU() const noexcept;

    /// @brief Run AI inference via the best available backend
    ///
    /// Routes the request through the priority chain and falls back automatically
    /// on any error. Sets the `chosen_backend` and `chosen_ep` fields of the request
    /// before dispatch.
    ///
    /// @param req Inference request (input data, model path, task tag)
    /// @return Result struct containing output, latency, backend used, and success status
    AiInferenceResult run(AiInferenceRequest& req);

    /// @brief Attempt inference on specific backend without fallback
    ///
    /// Does not attempt fallback if the requested backend fails.
    ///
    /// @param backend Specific backend to execute on (GPU, NPU, CPU, etc.)
    /// @param req Inference request to execute
    /// @return Result with success == false and error message if backend unavailable or fails
    AiInferenceResult runOn(BackendType backend, AiInferenceRequest& req);

    /// @brief Log a structured summary of all probed backends
    /// @note Output is written to ThemisDB logger at INFO level
    void logCapabilities() const;

    /// @brief Register custom dispatch function for Apple Neural Engine
    /// @param fn Function to call for Apple ANE inference dispatch
    static void setAppleANEDispatchFn(AppleANEDispatchFn fn);

private:
    AiHardwareDispatcher() = default;
    ~AiHardwareDispatcher() = default;
    AiHardwareDispatcher(const AiHardwareDispatcher&) = delete;
    AiHardwareDispatcher& operator=(const AiHardwareDispatcher&) = delete;

    // ── Internal probe helpers ─────────────────────────────────────────────
    /// @brief Probe Apple Neural Engine availability and capabilities
    AiHardwareCapability probeAppleANE() const noexcept;
    
    /// @brief Probe Intel NPU / OpenVINO availability and capabilities
    AiHardwareCapability probeIntelNPU() const noexcept;
    
    /// @brief Probe Qualcomm QNN / Hexagon availability and capabilities
    AiHardwareCapability probeQualcommQNN() const noexcept;
    
    /// @brief Probe ARM Mali / Ethos availability and capabilities
    AiHardwareCapability probeArmEthos() const noexcept;
    
    /// @brief Probe Android NNAPI availability and capabilities
    AiHardwareCapability probeNNAPI() const noexcept;
    
    /// @brief Probe ONNX Runtime EP availability and capabilities
    AiHardwareCapability probeOnnxRuntime() const noexcept;
    
    /// @brief Probe GPU backend (CUDA/HIP/Metal/Vulkan) availability
    AiHardwareCapability probeGpuFallback() const noexcept;
    
    /// @brief Probe CPU backend (AVX-512/AVX2/NEON) capabilities
    AiHardwareCapability probeCpuFallback() const noexcept;

    // ── Internal dispatch helpers ──────────────────────────────────────────
    /// @brief Dispatch inference to Apple Neural Engine
    AiInferenceResult dispatchAppleANE(AiInferenceRequest& req);
    
    /// @brief Dispatch inference to Intel NPU / OpenVINO
    AiInferenceResult dispatchIntelNPU([[maybe_unused]] AiInferenceRequest& req);
    
    /// @brief Dispatch inference to Qualcomm QNN / Hexagon
    AiInferenceResult dispatchQualcommQNN([[maybe_unused]] AiInferenceRequest& req);
    
    /// @brief Dispatch inference to ARM Mali / Ethos
    AiInferenceResult dispatchArmEthos([[maybe_unused]] AiInferenceRequest& req);
    
    /// @brief Dispatch inference to Android NNAPI
    AiInferenceResult dispatchNNAPI([[maybe_unused]] AiInferenceRequest& req);
    
    /// @brief Dispatch inference to ONNX Runtime selected EP
    AiInferenceResult dispatchOnnxRuntime([[maybe_unused]] AiInferenceRequest& req);
    
    /// @brief Dispatch inference to GPU backend (CUDA/HIP/Metal/Vulkan)
    AiInferenceResult dispatchGpuFallback(AiInferenceRequest& req);
    
    /// @brief Dispatch inference to CPU backend (AVX-512/AVX2/NEON)
    AiInferenceResult dispatchCpuFallback(AiInferenceRequest& req);

    // ── State ─────────────────────────────────────────────────────────────────
    mutable std::shared_mutex            mutex_;                   ///< Protects capabilities_ and last_probe_time_
    std::vector<AiHardwareCapability>    capabilities_;            ///< Hardware capabilities in priority order
    std::chrono::steady_clock::time_point last_probe_time_{};      ///< Timestamp of last hardware probe
    std::atomic<bool>                    initialized_{false};      ///< true if initialize() has been called
};

} // namespace acceleration
} // namespace themis
