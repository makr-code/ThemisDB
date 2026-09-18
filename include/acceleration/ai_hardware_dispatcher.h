/**
 * @file ai_hardware_dispatcher.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct AiInferenceRequest {
    const float*  input_data     = nullptr;  ///< Host-side input tensor (FP32 array)
    size_t        input_elements = 0;        ///< Total number of scalar elements in tensor
    std::vector<int64_t> input_shape;        ///< Tensor shape dimensions (e.g., {1, 512})

    std::string   model_path;                ///< Path to model file (.onnx, .coreml, .dlc, etc.)
    std::string   task_tag;                  ///< Task category: "embedding", "rerank", "classify", "generate", etc.

    PrecisionMode preferred_precision = PrecisionMode::FP32;

    BackendType   chosen_backend  = BackendType::CPU;  ///< Backend selected by dispatcher
    std::string   chosen_ep;                           ///< ONNX EP or platform-specific identifier used

    const float* similarity_corpus      = nullptr;                  ///< Corpus matrix [numVectors × dim]
    size_t       similarity_num_queries = 1;                        ///< Number of query vectors
    size_t       similarity_num_vectors = 0;                        ///< Number of corpus vectors
    size_t       similarity_dim         = 0;                        ///< Shared vector dimensionality
    size_t       similarity_top_k       = 1;                        ///< Number of neighbours to return
    DistanceMetric similarity_metric    = DistanceMetric::L2;       ///< Metric used for vector-similarity path
};

struct AiInferenceResult {
    std::vector<float>   output;              ///< Host-side output tensor (FP32)
    std::vector<int64_t> output_shape;        ///< Output tensor shape dimensions
    bool                 success      = false; ///< true if inference completed successfully
    std::string          error;               ///< Error message if success is false
    BackendType          backend_used = BackendType::CPU;  ///< Backend that executed the model
    std::string          ep_used;             ///< Execution provider/backend identifier
    double               latency_ms   = 0.0;  ///< Wall-clock inference time in milliseconds

    std::vector<uint32_t> topk_indices;
    std::vector<float>    topk_distances;
};

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
class AiHardwareDispatcher {
public:
    using AppleANEDispatchFn = std::function<AiInferenceResult(AiInferenceRequest&)>;

    static constexpr auto kCacheTTL = std::chrono::seconds(120);

    /**
     * @brief Instance.
     * @return Return value.
     */
    static AiHardwareDispatcher& instance();

    void initialize(bool force = false);

    /**
     * @brief Probe Capabilities.
     * @return Return value.
     */
    std::vector<AiHardwareCapability> probeCapabilities();

    /**
     * @brief Best Backend.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    BackendType bestBackend() const noexcept;

    /**
     * @brief Best Onnx EP.
     * @return Return value.
     */
    std::string bestOnnxEP() const;

    /**
     * @brief Has Accelerator.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool hasAccelerator() const noexcept;

    /**
     * @brief Has NPU.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool hasNPU() const noexcept;

    /**
     * @brief Run.
     * @param[in,out] req Input/output parameter.
     * @return Return value.
     */
    AiInferenceResult run(AiInferenceRequest& req);

    /**
     * @brief Run On.
     * @param[in] backend Input parameter.
     * @param[in,out] req Input/output parameter.
     * @return Return value.
     */
    AiInferenceResult runOn(BackendType backend, AiInferenceRequest& req);

    /**
     * @brief Log Capabilities.
     */
    void logCapabilities() const;

    /**
     * @brief Set Apple ANEDispatch Fn.
     * @param[in] fn Input parameter.
     */
    static void setAppleANEDispatchFn(AppleANEDispatchFn fn);

private:
    AiHardwareDispatcher() = default;
    ~AiHardwareDispatcher() = default;
    AiHardwareDispatcher(const AiHardwareDispatcher&) = delete;
    AiHardwareDispatcher& operator=(const AiHardwareDispatcher&) = delete;

    /**
     * @brief ── Internal probe helpers ─────────────────────────────────────────────
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeAppleANE() const noexcept;
    
    /**
     * @brief Probe Intel NPU.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeIntelNPU() const noexcept;
    
    /**
     * @brief Probe Qualcomm QNN.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeQualcommQNN() const noexcept;
    
    /**
     * @brief Probe Arm Ethos.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeArmEthos() const noexcept;
    
    /**
     * @brief Probe NNAPI.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeNNAPI() const noexcept;
    
    /**
     * @brief Probe Onnx Runtime.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeOnnxRuntime() const noexcept;
    
    /**
     * @brief Probe Gpu Fallback.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeGpuFallback() const noexcept;
    
    /**
     * @brief Probe Cpu Fallback.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    AiHardwareCapability probeCpuFallback() const noexcept;

    /**
     * @brief ── Internal dispatch helpers ──────────────────────────────────────────
     * @param[in,out] req Input/output parameter.
     * @return Return value.
     */
    AiInferenceResult dispatchAppleANE(AiInferenceRequest& req);
    
    AiInferenceResult dispatchIntelNPU([[maybe_unused]] AiInferenceRequest& req);
    
    AiInferenceResult dispatchQualcommQNN([[maybe_unused]] AiInferenceRequest& req);
    
    AiInferenceResult dispatchArmEthos([[maybe_unused]] AiInferenceRequest& req);
    
    AiInferenceResult dispatchNNAPI([[maybe_unused]] AiInferenceRequest& req);
    
    AiInferenceResult dispatchOnnxRuntime([[maybe_unused]] AiInferenceRequest& req);
    
    /**
     * @brief Dispatch Gpu Fallback.
     * @param[in,out] req Input/output parameter.
     * @return Return value.
     */
    AiInferenceResult dispatchGpuFallback(AiInferenceRequest& req);
    
    /**
     * @brief Dispatch Cpu Fallback.
     * @param[in,out] req Input/output parameter.
     * @return Return value.
     */
    AiInferenceResult dispatchCpuFallback(AiInferenceRequest& req);

    // ── State ─────────────────────────────────────────────────────────────────
    mutable std::shared_mutex            mutex_;                   ///< Protects capabilities_ and last_probe_time_
    std::vector<AiHardwareCapability>    capabilities_;            ///< Hardware capabilities in priority order
    std::chrono::steady_clock::time_point last_probe_time_{};      ///< Timestamp of last hardware probe
    std::atomic<bool>                    initialized_{false};      ///< true if initialize() has been called
};

} // namespace acceleration
} // namespace themis
