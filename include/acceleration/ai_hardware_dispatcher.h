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

/*
 * ThemisDB | File: ai_hardware_dispatcher.h | Version: 0.0.10 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 252
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

// =============================================================================
// AiInferenceRequest
// =============================================================================
// Describes a single AI inference task routed through the dispatcher.
// The dispatcher fills `chosen_backend` and `chosen_ep` before execution.
// =============================================================================
struct AiInferenceRequest {
    // ── Input ─────────────────────────────────────────────────────────────────
    const float*  input_data     = nullptr;  ///< Host-side input tensor (FP32)
    size_t        input_elements = 0;        ///< Total number of scalar elements
    std::vector<int64_t> input_shape;        ///< Tensor shape (e.g. {1, 512})

    // ── Model / task ──────────────────────────────────────────────────────────
    std::string   model_path;                ///< Path to .onnx / .coreml / .dlc model
    std::string   task_tag;                  ///< "embedding" | "rerank" | "classify" | "generate"

    // ── Precision preference ──────────────────────────────────────────────────
    // Dispatcher picks the highest-capable backend that satisfies this constraint.
    // Defaults to FP32 (broadest compatibility).
    PrecisionMode preferred_precision = PrecisionMode::FP32;

    // ── Routing hint (set automatically by dispatcher) ─────────────────────
    BackendType   chosen_backend  = BackendType::CPU;
    std::string   chosen_ep;                 ///< ONNX EP or platform-specific identifier
};

// =============================================================================
// AiInferenceResult
// =============================================================================
struct AiInferenceResult {
    std::vector<float>   output;             ///< Host-side output tensor (FP32)
    std::vector<int64_t> output_shape;
    bool                 success      = false;
    std::string          error;
    BackendType          backend_used = BackendType::CPU;
    std::string          ep_used;            ///< Execution provider that ran the model
    double               latency_ms   = 0.0; ///< Wall-clock inference time
};

// =============================================================================
// AiHardwareCapability
// =============================================================================
// Snapshot returned by AiHardwareDispatcher::probeCapabilities().
// =============================================================================
struct AiHardwareCapability {
    BackendType type             = BackendType::CPU;
    std::string name;                            ///< Human-readable identifier
    bool        available        = false;        ///< Probe succeeded at runtime
    uint32_t    tops             = 0;            ///< Peak throughput estimate (TOPS)
    PrecisionMode supported_precisions = PrecisionMode::FP32;
    std::string onnx_ep;                         ///< ONNX EP name (empty for non-ONNX paths)
    std::string error;                           ///< Non-empty when available == false
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

    /// Singleton accessor.
    static AiHardwareDispatcher& instance();

    // ── Lifecycle ─────────────────────────────────────────────────────────────
    /**
     * @brief Probe all AI hardware backends and build the priority chain.
     *
     * Safe to call multiple times — subsequent calls are no-ops unless
     * force == true or the cache TTL has expired.
     *
     * @param force  Re-probe even if the cache is still valid.
     */
    void initialize(bool force = false);

    // ── Capability query ──────────────────────────────────────────────────────
    /// Return the capability snapshot for every probed backend (in priority order).
    std::vector<AiHardwareCapability> probeCapabilities();

    /// Return the highest-priority available backend type.
    BackendType bestBackend() const noexcept;

    /// Return the ONNX Runtime execution provider name for the best backend.
    /// Empty string when ONNX Runtime is not available or not the best path.
    std::string bestOnnxEP() const;

    /// True when at least one non-CPU AI accelerator is available.
    bool hasAccelerator() const noexcept;

    /// True when a dedicated NPU (Apple ANE / Intel / Qualcomm / ARM) is present.
    bool hasNPU() const noexcept;

    // ── Inference dispatch ────────────────────────────────────────────────────
    /**
     * @brief Run AI inference via the best available backend.
     *
     * Routes the request through the priority chain, falling back
     * automatically on any error.  The `chosen_backend` and `chosen_ep`
     * fields of @p req are filled before dispatch.
     *
     * @param req  Inference request (input data, model path, task tag).
     * @return     Result including output tensor, latency, and backend used.
     */
    AiInferenceResult run(AiInferenceRequest& req);

    /**
     * @brief Attempt inference on a specific backend without fallback.
     *
     * Returns an error result when the requested backend is unavailable.
     */
    AiInferenceResult runOn(BackendType backend, AiInferenceRequest& req);

    // ── Observability ─────────────────────────────────────────────────────────
    /// Log a structured summary of all probed backends to the ThemisDB logger.
    void logCapabilities() const;

    static void setAppleANEDispatchFn(AppleANEDispatchFn fn);

private:
    AiHardwareDispatcher() = default;
    ~AiHardwareDispatcher() = default;
    AiHardwareDispatcher(const AiHardwareDispatcher&) = delete;
    AiHardwareDispatcher& operator=(const AiHardwareDispatcher&) = delete;

    // ── Internal probe helpers ─────────────────────────────────────────────
    AiHardwareCapability probeAppleANE() const noexcept;
    AiHardwareCapability probeIntelNPU() const noexcept;
    AiHardwareCapability probeQualcommQNN() const noexcept;
    AiHardwareCapability probeArmEthos() const noexcept;
    AiHardwareCapability probeNNAPI() const noexcept;
    AiHardwareCapability probeOnnxRuntime() const noexcept;
    AiHardwareCapability probeGpuFallback() const noexcept;
    AiHardwareCapability probeCpuFallback() const noexcept;

    // ── Internal dispatch helpers ──────────────────────────────────────────
    AiInferenceResult dispatchAppleANE(AiInferenceRequest& req);
    AiInferenceResult dispatchIntelNPU(AiInferenceRequest& req);
    AiInferenceResult dispatchQualcommQNN(AiInferenceRequest& req);
    AiInferenceResult dispatchArmEthos(AiInferenceRequest& req);
    AiInferenceResult dispatchNNAPI(AiInferenceRequest& req);
    AiInferenceResult dispatchOnnxRuntime([[maybe_unused]] AiInferenceRequest& req);
    AiInferenceResult dispatchGpuFallback(AiInferenceRequest& req);
    AiInferenceResult dispatchCpuFallback(AiInferenceRequest& req);

    // ── State ─────────────────────────────────────────────────────────────────
    mutable std::shared_mutex            mutex_;
    std::vector<AiHardwareCapability>    capabilities_;   ///< Ordered priority chain
    std::chrono::steady_clock::time_point last_probe_time_{};
    std::atomic<bool>                    initialized_{false};
};

} // namespace acceleration
} // namespace themis
