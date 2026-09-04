/**
 * @file ai_hardware_dispatcher.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=7; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=1, Debt=0, C=28, H=49, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Acceleration module — AI Hardware Dispatcher
 * ==============================================
 * Routes AI inference workloads to the best available AI accelerator using a
 * deterministic priority chain.  This dispatcher is independent of BackendRegistry
 * and specialises in AI inference rather than general ANN / geospatial / graph
 * compute.
 *
 * Dispatch chain position
 * -----------------------
 *   AiHardwareDispatcher::dispatch(task, model, input)
 *       └─► priority chain (first available wins):
 *               NPU_APPLE  → dispatchAppleANE()          [CoreML, macOS/iOS]
 *               NPU_INTEL  → dispatchIntelNPU()           [OpenVINO ov::Core]
 *               NPU_QUALCOMM → dispatchQualcommQNN()      [graceful not-implemented]
 *               NPU_ARM    → dispatchArmEthos()           [graceful not-implemented]
 *               NNAPI      → dispatchNNAPI()              [graceful not-implemented]
 *               ONNX_RUNTIME → dispatchOnnxRuntime()     [CreateEnv/CreateSession/Run]
 *               GPU        → delegates to BackendRegistry (ANN/matmul path)
 *               CPU        → CPU fallback
 *
 * This is the ONLY file that coordinates the NPU priority chain.
 * BackendRegistry handles GPU/CPU selection for ANN, geo, and graph workloads.
 *
 * Key interfaces implemented / exposed
 * -------------------------------------
 *   AiHardwareDispatcher::instance()   — singleton access
 *   AiHardwareDispatcher::initialize() — probe NPU/GPU/CPU; logs detected capabilities
 *   AiHardwareDispatcher::dispatch()   — route inference task through priority chain
 *   AiHardwareDispatcher::logCapabilities() — log detected CPU/GPU/NPU at startup (AH-31)
 *
 * Build-time feature flags
 * -------------------------
 *   THEMIS_HAS_NPU_APPLE    — enable CoreML / Apple ANE path
 *   THEMIS_HAS_NPU_INTEL    — enable OpenVINO path
 *   THEMIS_HAS_NPU_QUALCOMM — enable QNN path
 *   THEMIS_HAS_NPU_ARM      — enable ARM Ethos path
 *
 * Related files
 * -------------
 *   include/acceleration/ai_hardware_dispatcher.h   — AiHardwareDispatcher declaration
 *   src/acceleration/backend_registry.cpp           — GPU/CPU path consumed by dispatch()→GPU
 *   include/acceleration/compute_backend.h          — BackendType, PrecisionMode, DeviceCapabilityInfo
 *   tests/acceleration/test_ai_hardware_dispatcher.cpp — 30 focused tests (AH-1…AH-30)
 *   src/acceleration/ROADMAP.md                    — "AI Hardware Support (v1.1.0)" section
 */

#include "acceleration/ai_hardware_dispatcher.h"
#include <stdexcept>
#include "acceleration/cpu_backend.h"
#include "acceleration/compute_backend.h"
#include "utils/logger.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstring>
#include <filesystem>
#include <sys/stat.h>
#include <vector>

#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#include <cuda_runtime.h>
#endif

#include "acceleration/compute_backend.h"
#include "utils/logger.h"

// ── Platform-gated includes ───────────────────────────────────────────────────

#if defined(THEMIS_HAS_NPU_APPLE)
#include <TargetConditionals.h>
#if TARGET_OS_OSX || TARGET_OS_IOS
#include <CoreML/CoreML.h> // Obj-C, compiled via .mm if needed
#endif
#endif

#if defined(THEMIS_HAS_NPU_INTEL)
// OpenVINO runtime C API header (opt-in: -DTHEMIS_ENABLE_NPU_INTEL)
#if __has_include(<openvino/openvino.hpp>)
#include <openvino/openvino.hpp>
#define THEMIS_OPENVINO_AVAILABLE 1
#endif
#endif

#if defined(THEMIS_HAS_NPU_QUALCOMM)
// Qualcomm QNN SDK C API header (opt-in: -DTHEMIS_ENABLE_NPU_QUALCOMM)
#if __has_include(<QnnInterface.h>)
#include <QnnInterface.h>
#define THEMIS_QNN_AVAILABLE 1
#endif
#endif

#if defined(THEMIS_HAS_NNAPI)
#include <android/NeuralNetworks.h>
#endif

#if defined(THEMIS_HAS_ONNX_RUNTIME)
// ONNX Runtime C API — available when onnxruntime is linked
#if __has_include(<onnxruntime_c_api.h>)
#include <onnxruntime_c_api.h>
#define THEMIS_ORT_AVAILABLE 1
#elif __has_include(<onnxruntime/core/session/onnxruntime_c_api.h>)
#include <onnxruntime/core/session/onnxruntime_c_api.h>
#define THEMIS_ORT_AVAILABLE 1
#endif
#endif

namespace themis {
namespace acceleration {

namespace {
std::mutex s_apple_ane_dispatch_mutex;
AiHardwareDispatcher::AppleANEDispatchFn s_apple_ane_dispatch_fn;

bool isVectorSimilarityTask(const std::string &task_tag) {
    return task_tag == "vector_similarity_l2" || task_tag == "vector_similarity_cosine"
           || task_tag == "vector_similarity_ip";
}

DistanceMetric metricFromTaskTag(const AiInferenceRequest &req) {
    if (req.task_tag == "vector_similarity_cosine") {
        return DistanceMetric::COSINE;
    }
    if (req.task_tag == "vector_similarity_ip") {
        return DistanceMetric::INNER_PRODUCT;
    }
    return req.similarity_metric;
}

bool validateSimilarityRequest(const AiInferenceRequest &req, std::string &error) {
    if (req.input_data == nullptr) {
        error = "vector similarity request has null input_data";
        return false;
    }
    if (req.similarity_corpus == nullptr) {
        error = "vector similarity request has null similarity_corpus";
        return false;
    }
    if (req.similarity_num_queries == 0 || req.similarity_num_vectors == 0 || req.similarity_dim == 0) {
        error = "vector similarity request requires similarity_num_queries/num_vectors/dim > 0";
        return false;
    }
    const size_t expected_query_elements = req.similarity_num_queries * req.similarity_dim;
    if (req.input_elements != expected_query_elements) {
        error = "vector similarity request input_elements mismatch: expected " + std::to_string(expected_query_elements)
                + ", got " + std::to_string(req.input_elements);
        return false;
    }
    if (req.similarity_top_k == 0) {
        error = "vector similarity request requires similarity_top_k > 0";
        return false;
    }
    return true;
}

AiInferenceResult runVectorSimilarityDispatch(const ANNKernelDispatch &dispatch, BackendType backend_type,
                                              const std::string &ep_used, const AiInferenceRequest &req) {
    AiInferenceResult result;
    result.backend_used = backend_type;
    result.ep_used      = ep_used;

    std::string validation_error = {};
    if (!validateSimilarityRequest(req, validation_error)) {
        result.success = false;
        result.error   = validation_error;
        return result;
    }

    ANNDistanceFn distance_launcher = dispatch.distanceLauncherFor(metricFromTaskTag(req));
    if (distance_launcher == nullptr || dispatch.launchTopK == nullptr) {
        result.success = false;
        result.error   = "ANN dispatch table incomplete for requested metric";
        return result;
    }

    const int num_queries = static_cast<int>(req.similarity_num_queries);
    const int num_vectors = static_cast<int>(req.similarity_num_vectors);
    const int dim         = static_cast<int>(req.similarity_dim);
    const int top_k       = static_cast<int>(std::min(req.similarity_top_k, req.similarity_num_vectors));

    std::vector<float> distance_matrix(req.similarity_num_queries * req.similarity_num_vectors);
    result.topk_indices.resize(req.similarity_num_queries * static_cast<size_t>(top_k));
    result.topk_distances.resize(req.similarity_num_queries * static_cast<size_t>(top_k));

    const auto t0 = std::chrono::steady_clock::now();
    const int distance_rc
        = distance_launcher(req.input_data, req.similarity_corpus, distance_matrix.data(), num_queries, num_vectors, dim, nullptr);
    if (distance_rc != 0) {
        result.success = false;
        result.error   = "distance kernel failed with code " + std::to_string(distance_rc);
        return result;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (backend_type == BackendType::CUDA) {
        const cudaError_t distance_cuda_error = cudaGetLastError();
        if (distance_cuda_error != cudaSuccess) {
            result.success = false;
            result.error   = std::string("distance kernel cudaGetLastError: ") + cudaGetErrorString(distance_cuda_error);
            return result;
        }
    }
#endif

    const int topk_rc = dispatch.launchTopK(distance_matrix.data(), result.topk_indices.data(), result.topk_distances.data(),
                                            num_queries, num_vectors, top_k, nullptr);
    if (topk_rc != 0) {
        result.success = false;
        result.error   = "top-k kernel failed with code " + std::to_string(topk_rc);
        return result;
    }

#ifdef THEMIS_ENABLE_CUDA
    if (backend_type == BackendType::CUDA) {
        const cudaError_t topk_cuda_error = cudaGetLastError();
        if (topk_cuda_error != cudaSuccess) {
            result.success = false;
            result.error   = std::string("top-k kernel cudaGetLastError: ") + cudaGetErrorString(topk_cuda_error);
            return result;
        }
    }
#endif

    result.output      = result.topk_distances;
    result.output_shape = {static_cast<int64_t>(req.similarity_num_queries), static_cast<int64_t>(top_k)};
    result.success     = true;
    const auto t1      = std::chrono::steady_clock::now();
    result.latency_ms  = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return result;
}

AiInferenceResult runCpuVectorSimilarity(const AiInferenceRequest &req) {
    CPUVectorBackend backend;
    (void)backend.initialize();
    return runVectorSimilarityDispatch(backend.populateANNDispatch(), BackendType::CPU, "CPUExecutionProvider", req);
}
} // namespace

void AiHardwareDispatcher::setAppleANEDispatchFn(AppleANEDispatchFn fn) {
    std::lock_guard<std::mutex> lk(s_apple_ane_dispatch_mutex);
    s_apple_ane_dispatch_fn = std::move(fn);
}

// =============================================================================
// Singleton
// =============================================================================

AiHardwareDispatcher &AiHardwareDispatcher::instance() {
    static AiHardwareDispatcher inst;
    return inst;
}

// =============================================================================
// Lifecycle
// =============================================================================

void AiHardwareDispatcher::initialize(bool force) {
    auto now = std::chrono::steady_clock::now();
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        if (!force && initialized_.load(std::memory_order_relaxed)) {
            if (now - last_probe_time_ < kCacheTTL) {
                return;
            }
        }
    }
    // Probe outside the lock to avoid holding it during slow hardware probes.
    // Each probe helper is noexcept and platform-gated.
    std::vector<AiHardwareCapability> caps;
    caps.reserve(8);

    caps.push_back(probeAppleANE());
    caps.push_back(probeIntelNPU());
    caps.push_back(probeQualcommQNN());
    caps.push_back(probeArmEthos());
    caps.push_back(probeNNAPI());
    caps.push_back(probeOnnxRuntime());
    caps.push_back(probeGpuFallback());
    caps.push_back(probeCpuFallback());

    // Stable-sort: available backends first, then by TOPS descending.
    std::stable_sort(caps.begin(), caps.end(), [](const AiHardwareCapability &a, const AiHardwareCapability &b) {
        if (a.available != b.available) {
            return a.available > b.available;
        }
        return a.tops > b.tops;
    });

    std::unique_lock<std::shared_mutex> lock(mutex_);
    capabilities_    = std::move(caps);
    last_probe_time_ = now;
    initialized_.store(true, std::memory_order_release);

    // Emit a structured log line listing every probed backend so that
    // operators can confirm which accelerator is active at startup.
    // logCapabilities() acquires a shared lock internally; release the
    // exclusive lock first to avoid a self-deadlock.
    lock.unlock();
    logCapabilities();
}

// =============================================================================
// Capability query
// =============================================================================

std::vector<AiHardwareCapability> AiHardwareDispatcher::probeCapabilities() {
    initialize();
    std::unique_lock<std::shared_mutex> lock(mutex_);
    return capabilities_;
}

BackendType AiHardwareDispatcher::bestBackend() const noexcept {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto &c : capabilities_) {
        if (c.available) {
            return c.type;
        }
    }
    return BackendType::CPU;
}

std::string AiHardwareDispatcher::bestOnnxEP() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto &c : capabilities_) {
        if (c.available && !c.onnx_ep.empty()) {
            return c.onnx_ep;
        }
    }
    return "CPUExecutionProvider";
}

bool AiHardwareDispatcher::hasAccelerator() const noexcept {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto &c : capabilities_) {
        if (c.available && c.type != BackendType::CPU) {
            return true;
        }
    }
    return false;
}

bool AiHardwareDispatcher::hasNPU() const noexcept {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    for (const auto &c : capabilities_) {
        if (!c.available) {
            continue;
        }
        if (c.type == BackendType::NPU_APPLE || c.type == BackendType::NPU_INTEL || c.type == BackendType::NPU_QUALCOMM
            || c.type == BackendType::NPU_ARM || c.type == BackendType::NNAPI) {
            return true;
        }
    }
    return false;
}

// =============================================================================
// Inference dispatch
// =============================================================================

AiInferenceResult AiHardwareDispatcher::run(AiInferenceRequest &req) {
    initialize();

    std::vector<AiHardwareCapability> chain;
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        chain = capabilities_;
    }

    for (const auto &cap : chain) {
        if (!cap.available) {
            continue;
        }

        // Check precision compatibility
        if (!hasPrecision(cap.supported_precisions, req.preferred_precision)) {
            // Try FP32 fallback if the preferred mode is unavailable
            if (req.preferred_precision != PrecisionMode::FP32
                && hasPrecision(cap.supported_precisions, PrecisionMode::FP32)) {
                THEMIS_DEBUG("AiHardwareDispatcher: {} does not support requested "
                             "precision, falling back to FP32",
                             cap.name);
            } else {
                continue;
            }
        }

        req.chosen_backend = cap.type;
        req.chosen_ep      = cap.onnx_ep;

        AiInferenceResult result = runOn(cap.type, req);
        if (result.success) {
            return result;
        }

        THEMIS_WARN("AiHardwareDispatcher: {} failed ({}), trying next backend", cap.name, result.error);
    }

    AiInferenceResult err;
    err.success = false;
    err.error   = "All AI hardware backends exhausted — no successful inference path";
    THEMIS_ERROR("AiHardwareDispatcher: {}", err.error);
    return err;
}

AiInferenceResult AiHardwareDispatcher::runOn(BackendType backend, AiInferenceRequest &req) {
    switch (backend) {
        case BackendType::NPU_APPLE:
            return dispatchAppleANE(req);
        case BackendType::NPU_INTEL:
            return dispatchIntelNPU(req);
        case BackendType::NPU_QUALCOMM:
            return dispatchQualcommQNN(req);
        case BackendType::NPU_ARM:
            return dispatchArmEthos(req);
        case BackendType::NNAPI:
            return dispatchNNAPI(req);
        case BackendType::ONNX_RUNTIME:
            return dispatchOnnxRuntime(req);
        case BackendType::CUDA:
        [[fallthrough]];\n        case BackendType::HIP:
        [[fallthrough]];\n        case BackendType::VULKAN:
        [[fallthrough]];\n        case BackendType::METAL:
        [[fallthrough]];\n        case BackendType::OPENCL:
        [[fallthrough]];\n        case BackendType::DIRECTX:
        [[fallthrough]];\n        case BackendType::ONEAPI:
            return dispatchGpuFallback(req);
        default:
            return dispatchCpuFallback(req);
    }
}

// =============================================================================
// Observability
// =============================================================================

void AiHardwareDispatcher::logCapabilities() const {
    std::shared_lock<std::shared_mutex> lock(mutex_);
    THEMIS_INFO("AiHardwareDispatcher — probed backends (priority order):");
    for (const auto &c : capabilities_) {
        if (c.available) {
            THEMIS_INFO("  [+] {} | TOPS: {} | EP: {} | precisions: {:#010x}", c.name, c.tops,
                        c.onnx_ep.empty() ? "native" : c.onnx_ep, static_cast<uint32_t>(c.supported_precisions));
        } else {
            THEMIS_DEBUG("  [-] {} (unavailable: {})", c.name, c.error);
        }
    }
}

// =============================================================================
// Platform probe helpers
// =============================================================================

AiHardwareCapability AiHardwareDispatcher::probeAppleANE() const noexcept {
    AiHardwareCapability cap;
    cap.type = BackendType::NPU_APPLE;
    cap.name = "Apple Neural Engine (Core ML)";

#if defined(THEMIS_HAS_NPU_APPLE)
    // Core ML is always present on Apple silicon; availability is determined
    // by linking CoreML.framework and the presence of the ANE in the SoC.
    // We probe by querying MLModel's available compute units.
    // On x86 Macs the ANE is absent; we still report CoreML CPU/GPU EP.
    cap.available            = true;
    cap.onnx_ep              = "CoreMLExecutionProvider";
    cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8;
#if defined(__aarch64__)
    // Apple Silicon (M1/M2/M3/M4 series)
    cap.tops                 = 38; // Conservative M2 estimate; overridden by metal-device query
    cap.supported_precisions = cap.supported_precisions | PrecisionMode::INT4;
#else
    cap.tops = 0; // Intel Mac: no discrete ANE
#endif
#else
    cap.available = false;
    cap.error     = "Not an Apple platform or THEMIS_DISABLE_NPU_APPLE set";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeIntelNPU() const noexcept {
    AiHardwareCapability cap;
    cap.type = BackendType::NPU_INTEL;
    cap.name = "Intel NPU (OpenVINO)";

#if defined(THEMIS_HAS_NPU_INTEL) && defined(THEMIS_OPENVINO_AVAILABLE)
    try {
        ov::Core core;
        auto devices   = core.get_available_devices();
        bool npu_found = false;
        for (const auto &d : devices) {
            if (d.find("NPU") != std::string::npos) {
                npu_found = true;
                break;
            }
        }
        if (npu_found) {
            cap.available = true;
            cap.onnx_ep   = "OpenVINOExecutionProvider";
            cap.tops      = 11; // Intel Core Ultra series NPU estimate
            cap.supported_precisions
                = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8 | PrecisionMode::INT4;
        } else {
            cap.available = false;
            cap.error     = "OpenVINO found but no NPU device enumerated";
        }
    } catch (const std::exception &e) {
        cap.available = false;
        cap.error     = std::string("OpenVINO probe exception: ") + e.what();
    }
#elif defined(THEMIS_HAS_NPU_INTEL)
    cap.available = false;
    cap.error     = "THEMIS_ENABLE_NPU_INTEL set but OpenVINO headers not found at compile time";
#else
    cap.available = false;
    cap.error     = "Not enabled (set -DTHEMIS_ENABLE_NPU_INTEL=ON)";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeQualcommQNN() const noexcept {
    AiHardwareCapability cap;
    cap.type = BackendType::NPU_QUALCOMM;
    cap.name = "Qualcomm AI Engine (QNN)";

#if defined(THEMIS_HAS_NPU_QUALCOMM) && defined(THEMIS_QNN_AVAILABLE)
    // QNN SDK probe: attempt to get the interface provider.
    // The QNN runtime is a shared library; its absence is a non-fatal probe failure.
    cap.available = true;
    cap.onnx_ep   = "QNNExecutionProvider";
    cap.tops      = 45; // Snapdragon X Elite NPU estimate
    cap.supported_precisions
        = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8 | PrecisionMode::W4A8 | PrecisionMode::INT4;
#elif defined(THEMIS_HAS_NPU_QUALCOMM)
    cap.available = false;
    cap.error     = "THEMIS_ENABLE_NPU_QUALCOMM set but QNN SDK headers not found";
#else
    cap.available = false;
    cap.error     = "Not enabled (set -DTHEMIS_ENABLE_NPU_QUALCOMM=ON)";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeArmEthos() const noexcept {
    AiHardwareCapability cap;
    cap.type = BackendType::NPU_ARM;
    cap.name = "ARM Ethos-N NPU";

#if defined(THEMIS_HAS_NPU_ARM)
    // Ethos-N is probed via the Arm NN delegate or a sysfs device node.
    // We check for the kernel driver node /dev/ethosu0.
    struct stat st{};
    int ret = ::stat("/dev/ethosu0", &st);
    if (ret == 0) {
        cap.available            = true;
        cap.tops                 = 4; // Ethos-N78 estimate (scales with config)
        cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::INT8;
    } else {
        int saved_errno = errno;
        if (saved_errno == ENOENT) {
            cap.available = false;
            cap.error     = "Ethos-N kernel driver not found (/dev/ethosu0 absent)";
        } else {
            cap.available = false;
            cap.error     = std::string("Ethos-N probe failed: stat() errno=") + std::to_string(saved_errno) + " ("
                            + ::strerror(saved_errno) + ")";
            THEMIS_WARN("AiHardwareDispatcher: probeArmEthos: unexpected stat error: {}", cap.error);
        }
    }
#else
    cap.available = false;
    cap.error     = "Not enabled (set -DTHEMIS_ENABLE_NPU_ARM=ON)";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeNNAPI() const noexcept {
    AiHardwareCapability cap;
    cap.type = BackendType::NNAPI;
    cap.name = "Android NNAPI";

#if defined(THEMIS_HAS_NNAPI)
    // ANeuralNetworks_getRuntimeFeatureLevel requires API 31+.
    // We check availability via the C API without linking statically.
    int64_t feature_level = 0;
    if (ANeuralNetworks_getRuntimeFeatureLevel(&feature_level) == ANEURALNETWORKS_NO_ERROR) {
        cap.available            = true;
        cap.onnx_ep              = "NnapiExecutionProvider";
        cap.tops                 = 10; // Conservative estimate; delegate dispatches to DSP/NPU
        cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8;
    } else {
        cap.available = false;
        cap.error     = "ANeuralNetworks_getRuntimeFeatureLevel returned error";
    }
#else
    cap.available = false;
    cap.error     = "Not an Android platform or THEMIS_DISABLE_NNAPI set";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeOnnxRuntime() const noexcept {
    AiHardwareCapability cap;
    cap.type = BackendType::ONNX_RUNTIME;
    cap.name = "ONNX Runtime";

#if defined(THEMIS_ORT_AVAILABLE)
    // Probe by fetching the ORT API — this is always available when the library
    // is linked and confirms the header/library version match.
    const OrtApi *api = OrtGetApiBase()->GetApi(ORT_API_VERSION);
    if (api) {
        cap.available = true;
        // TOPS is left at 0 — ONNX Runtime dispatches to the best EP at session
        // creation time, so the effective TOPS depends on the selected EP.
        // We rank it below dedicated NPU probes but above plain GPU fallback.
        cap.tops = 1;
        cap.supported_precisions
            = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8 | PrecisionMode::INT4;

        // Determine best EP based on available compile-time guards.
#if defined(THEMIS_ENABLE_CUDA)
        cap.onnx_ep = "CUDAExecutionProvider";
#elif defined(_WIN32)
        cap.onnx_ep = "DmlExecutionProvider"; // DirectML
#elif defined(THEMIS_HAS_NPU_INTEL) && defined(THEMIS_OPENVINO_AVAILABLE)
        cap.onnx_ep = "OpenVINOExecutionProvider";
#else
        cap.onnx_ep = "CPUExecutionProvider";
#endif
    } else {
        cap.available = false;
        cap.error     = "OrtGetApiBase returned null — library version mismatch?";
    }
#else
    cap.available = false;
    cap.error     = "ONNX Runtime headers not found at compile time "
                    "(set -DTHEMIS_DISABLE_ONNX_RUNTIME=ON to silence)";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeGpuFallback() const noexcept {
    AiHardwareCapability cap;
    cap.name = "GPU (via BackendRegistry)";
    // Determine the best GPU backend available from what was compiled in.
#if defined(THEMIS_ENABLE_CUDA)
    cap.type                 = BackendType::CUDA;
    cap.available            = true;
    cap.tops                 = 0; // Depends on GPU model; DeviceManager provides exact value
    cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::BF16 | PrecisionMode::INT8;
#elif defined(THEMIS_ENABLE_HIP)
    cap.type                 = BackendType::HIP;
    cap.available            = true;
    cap.tops                 = 0;
    cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8;
#elif defined(THEMIS_ENABLE_VULKAN)
    cap.type                 = BackendType::VULKAN;
    cap.available            = true;
    cap.tops                 = 0;
    cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::FP16;
#else
    cap.type      = BackendType::CPU;
    cap.available = false;
    cap.error     = "No GPU backend compiled in";
#endif
    return cap;
}

AiHardwareCapability AiHardwareDispatcher::probeCpuFallback() const noexcept {
    AiHardwareCapability cap;
    cap.type                 = BackendType::CPU;
    cap.name                 = "CPU (SIMD fallback)";
    cap.available            = true;
    cap.tops                 = 0;
    cap.supported_precisions = PrecisionMode::FP32 | PrecisionMode::FP16 | PrecisionMode::INT8;
    return cap;
}

// =============================================================================
// Dispatch helpers
// =============================================================================
// Each dispatch helper:
//   1. Validates the request (non-null data, valid shape).
//   2. Executes via the platform-native API.
//   3. Returns a filled AiInferenceResult (success or error).
//
// Platform-conditional sections are gated identically to the probe helpers.
// =============================================================================

static AiInferenceResult makeError(BackendType bt, const std::string &msg) {
    AiInferenceResult r;
    r.success      = false;
    r.backend_used = bt;
    r.error        = msg;
    return r;
}

AiInferenceResult AiHardwareDispatcher::dispatchAppleANE([[maybe_unused]] AiInferenceRequest &req) {
    AppleANEDispatchFn fn;
    {
        std::lock_guard<std::mutex> lk(s_apple_ane_dispatch_mutex);
        fn = s_apple_ane_dispatch_fn;
    }
    if (fn) {
        try {
            return fn(req);
        } catch (const std::exception& e) {
            return makeError(BackendType::NPU_APPLE,
                             std::string("Injected Apple ANE dispatch failed: ") + e.what());
        } catch (const std::string& e) {
            return makeError(BackendType::NPU_APPLE,
                             std::string("Injected Apple ANE dispatch failed: ") + e);
        } catch (const char* e) {
            return makeError(BackendType::NPU_APPLE,
                             std::string("Injected Apple ANE dispatch failed: ") + (e ? e : "<null>"));
            return makeError(BackendType::NPU_APPLE, "Injected Apple ANE dispatch failed");
        }
    }
#if defined(THEMIS_HAS_NPU_APPLE)
    if (!req.input_data || req.input_elements == 0) {
        return makeError(BackendType::NPU_APPLE, "Invalid input: null or empty");
    }
    auto t0 = std::chrono::steady_clock::now();

    // Apple Neural Engine dispatch via Core ML.
    // Full Core ML session management is in metal_backend.mm (Objective-C++).
    //
    // STUB/SIMULATION NOTE:
    // Purpose: Allow the AI Hardware Dispatcher to compile on non-macOS
    //   platforms or Apple builds without the Objective-C++ runtime.  The
    //   dispatch succeeds structurally (timing measured) but always returns
    //   `success = false` with an informative error message.
    // Activation: `THEMIS_HAS_NPU_APPLE` defined but `metal_backend.mm` is not
    //   linked as an Objective-C++ TU, or Core ML headers are absent.
    // Production Delta: Apple Neural Engine (ANE) / Core ML inference is
    //   unavailable.  Workloads that could run at ANE speeds (≥ 10 TOPS) route
    //   to CPU/GPU fallback.  The dispatcher returns an error result; callers
    //   must re-route to CPU/CUDA.
    // Removal Plan: Link `metal_backend.mm` with Objective-C++ and ensure Core
    //   ML framework is available (`-framework CoreML -framework Metal`).
    //   Implement the full `MLModel` session, `MLMultiArray` preparation, and
    //   result extraction in `metal_backend.mm`.  Remove this stub and delegate
    //   directly to the Obj-C++ implementation.
    // Roadmap ref: src/acceleration/FUTURE_ENHANCEMENTS.md §"Apple ANE Core ML Activation"

    // This stub delegates to the Metal backend which has Core ML integration.
    // A real implementation would create an MLModel session, prepare an
    // MLMultiArray from req.input_data, run prediction, and extract results.
    AiInferenceResult result;
    result.success      = false;
    result.backend_used = BackendType::NPU_APPLE;
    result.error        = "Core ML dispatch requires Objective-C++ compilation "
                          "(link metal_backend.mm and set THEMIS_HAS_NPU_APPLE)";

    auto t1           = std::chrono::steady_clock::now();
    result.latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    result.ep_used    = "CoreMLExecutionProvider";
    return result;
#else
    return makeError(BackendType::NPU_APPLE, "Apple ANE not available on this platform");
#endif
}

AiInferenceResult AiHardwareDispatcher::dispatchIntelNPU([[maybe_unused]] AiInferenceRequest &req) {
#if defined(THEMIS_HAS_NPU_INTEL) && defined(THEMIS_OPENVINO_AVAILABLE)
    if (!req.input_data || req.input_elements == 0) {
        return makeError(BackendType::NPU_INTEL, "Invalid input: null or empty");
    }
    auto t0 = std::chrono::steady_clock::now();

    try {
        ov::Core core;
        auto model = core.read_model(req.model_path);

        // Compile to NPU execution device
        auto compiled  = core.compile_model(model, "NPU");
        auto infer_req = compiled.create_infer_request();

        // Bind input tensor
        ov::Shape shape(req.input_shape.begin(), req.input_shape.end());
        ov::Tensor input_tensor(ov::element::f32, shape, const_cast<float *>(req.input_data));
        infer_req.set_input_tensor(input_tensor);

        infer_req.infer();

        auto output_tensor   = infer_req.get_output_tensor();
        const float *out_ptr = output_tensor.data<float>();
        size_t out_size      = output_tensor.get_size();

        AiInferenceResult result;
        result.success      = true;
        result.backend_used = BackendType::NPU_INTEL;
        result.ep_used      = "OpenVINOExecutionProvider/NPU";
        result.output.assign(out_ptr, out_ptr + out_size);
        for (auto dim : output_tensor.get_shape()) {
            result.output_shape.push_back(static_cast<int64_t>(dim));
        }
        auto t1           = std::chrono::steady_clock::now();
        result.latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
        return result;

    } catch (const ov::Exception &e) {
        return makeError(BackendType::NPU_INTEL, std::string("OpenVINO NPU inference failed: ") + e.what());
    }
#else
    return makeError(BackendType::NPU_INTEL, "Intel NPU (OpenVINO) not available");
#endif
}

AiInferenceResult AiHardwareDispatcher::dispatchQualcommQNN([[maybe_unused]] AiInferenceRequest &req) {
#if defined(THEMIS_HAS_NPU_QUALCOMM) && defined(THEMIS_QNN_AVAILABLE)
    if (!req.input_data || req.input_elements == 0) {
        return makeError(BackendType::NPU_QUALCOMM, "Invalid input: null or empty");
    }
    // QNN session management follows the Qualcomm AI Engine Direct SDK pattern:
    // QnnInterface_getProviders → QnnBackend_create → QnnContext_create →
    // QnnGraph_create → populate graph → QnnGraph_finalize → execute.
    // Full implementation requires QNN SDK linkage (-DTHEMIS_ENABLE_NPU_QUALCOMM=ON).
    AiInferenceResult result;
    result.success      = false;
    result.backend_used = BackendType::NPU_QUALCOMM;
    result.error        = "QNN full dispatch requires QNN SDK linkage. "
                          "Set -DTHEMIS_ENABLE_NPU_QUALCOMM=ON and link QNN libraries.";
    result.ep_used      = "QNNExecutionProvider";
    return result;
#else
    return makeError(BackendType::NPU_QUALCOMM, "Qualcomm QNN not available");
#endif
}

AiInferenceResult AiHardwareDispatcher::dispatchArmEthos([[maybe_unused]] AiInferenceRequest &req) {
#if defined(THEMIS_HAS_NPU_ARM)
    if (!req.input_data || req.input_elements == 0) {
        return makeError(BackendType::NPU_ARM, "Invalid input: null or empty");
    }
    // ARM Ethos-N dispatch via the Ethos-N kernel driver ioctl interface or
    // via the Arm NN TfLite delegate.  Requires linking libEthosN.so.
    AiInferenceResult result;
    result.success      = false;
    result.backend_used = BackendType::NPU_ARM;
    result.error        = "ARM Ethos-N dispatch requires libEthosN.so linkage. "
                          "Set -DTHEMIS_ENABLE_NPU_ARM=ON and link Ethos-N runtime.";
    return result;
#else
    return makeError(BackendType::NPU_ARM, "ARM Ethos-N not available");
#endif
}

AiInferenceResult AiHardwareDispatcher::dispatchNNAPI([[maybe_unused]] AiInferenceRequest &req) {
#if defined(THEMIS_HAS_NNAPI)
    if (!req.input_data || req.input_elements == 0) {
        return makeError(BackendType::NNAPI, "Invalid input: null or empty");
    }
    // Android NNAPI is most reliably consumed via the ONNX Runtime NNAPI
    // Execution Provider, which handles model compilation and execution
    // scheduling through the Android NeuralNetworks API internally.
    // Route through dispatchOnnxRuntime with NnapiExecutionProvider selected.
#if defined(THEMIS_ORT_AVAILABLE)
    req.chosen_ep   = "NnapiExecutionProvider";
    auto ort_result = dispatchOnnxRuntime(req);
    // Re-stamp backend as NNAPI so callers see the correct backend type.
    ort_result.backend_used = BackendType::NNAPI;
    if (ort_result.success) {
        ort_result.ep_used = "NnapiExecutionProvider";
    }
    return ort_result;
#else
    AiInferenceResult result;
    result.success      = false;
    result.backend_used = BackendType::NNAPI;
    result.error        = "NNAPI dispatch requires ONNX Runtime "
                          "(build with THEMIS_HAS_ONNX_RUNTIME)";
    result.ep_used      = "NnapiExecutionProvider";
    return result;
#endif // THEMIS_ORT_AVAILABLE
#else
    return makeError(BackendType::NNAPI, "NNAPI not available (non-Android platform)");
#endif // THEMIS_HAS_NNAPI
}

AiInferenceResult AiHardwareDispatcher::dispatchOnnxRuntime([[maybe_unused]] AiInferenceRequest &req) {
#if defined(THEMIS_ORT_AVAILABLE)
    if (req.input_data == nullptr || req.input_elements == 0) {
        return makeError(BackendType::ONNX_RUNTIME, "Invalid input: null or empty");
    }
    if (req.model_path.empty()) {
        return makeError(BackendType::ONNX_RUNTIME, "model_path is empty");
    }

    auto t0 = std::chrono::steady_clock::now();
    AiInferenceResult result;
    result.backend_used = BackendType::ONNX_RUNTIME;
    result.ep_used      = req.chosen_ep.empty() ? "CPUExecutionProvider" : req.chosen_ep;

    try {
        const OrtApi *ort = OrtGetApiBase()->GetApi(ORT_API_VERSION);
        if (ort == nullptr) {
            return makeError(BackendType::ONNX_RUNTIME, "OrtGetApiBase returned null");
        }

        // Environment (one per process)
        OrtEnv *env       = nullptr;
        OrtStatus *status = ort->CreateEnv(ORT_LOGGING_LEVEL_WARNING, "ThemisAiDispatcher", &env);
        if (status != nullptr) {
            std::string msg = ort->GetErrorMessage(status);
            ort->ReleaseStatus(status);
            return makeError(BackendType::ONNX_RUNTIME, "ORT CreateEnv failed: " + msg);
        }

        // Session options
        OrtSessionOptions *opts = nullptr;
        ort->CreateSessionOptions(&opts);
        // EP selection: append the preferred provider
        if (result.ep_used == "CUDAExecutionProvider") {
            OrtCUDAProviderOptions cuda_opts{};
            ort->SessionOptionsAppendExecutionProvider_CUDA(opts, &cuda_opts);
        }
        // CoreML, QNN, OpenVINO EPs require runtime library linkage; they are
        // appended here via the generic string EP API when available.

        // Create session
        OrtSession *session = nullptr;
#if defined(_WIN32)
        const std::wstring model_path_w = std::filesystem::path(req.model_path).wstring();
        status                          = ort->CreateSession(env, model_path_w.c_str(), opts, &session);
#else
        status = ort->CreateSession(env, req.model_path.c_str(), opts, &session);
#endif
        if (status != nullptr) {
            std::string msg = ort->GetErrorMessage(status);
            ort->ReleaseStatus(status);
            ort->ReleaseSessionOptions(opts);
            ort->ReleaseEnv(env);
            return makeError(BackendType::ONNX_RUNTIME, "ORT CreateSession failed: " + msg);
        }

        // Build input shape
        std::vector<int64_t> shape(req.input_shape.begin(), req.input_shape.end());

        // Memory info
        OrtMemoryInfo *mem_info = nullptr;
        ort->CreateCpuMemoryInfo(OrtArenaAllocator, OrtMemTypeDefault, &mem_info);

        // Input tensor
        OrtValue *input_tensor = nullptr;
        ort->CreateTensorWithDataAsOrtValue(mem_info, const_cast<float *>(req.input_data),
                                            req.input_elements * sizeof(float), shape.data(), shape.size(),
                                            ONNX_TENSOR_ELEMENT_DATA_TYPE_FLOAT, &input_tensor);

        // Query input/output names
        OrtAllocator *alloc = nullptr;
        ort->GetAllocatorWithDefaultOptions(&alloc);
        size_t input_count = 0, output_count = 0;
        ort->SessionGetInputCount(session, &input_count);
        ort->SessionGetOutputCount(session, &output_count);

        std::vector<char *> input_names_raw(input_count);
        std::vector<char *> output_names_raw(output_count);
        for (size_t i = 0; i < input_count; ++i) {
            ort->SessionGetInputName(session, i, alloc, &input_names_raw[i]);
        }
        for (size_t i = 0; i < output_count; ++i) {
            ort->SessionGetOutputName(session, i, alloc, &output_names_raw[i]);
        }

        const char *const *in_names  = const_cast<const char *const *>(input_names_raw.data());
        const char *const *out_names = const_cast<const char *const *>(output_names_raw.data());

        // Run inference
        std::vector<OrtValue *> output_tensors(output_count, nullptr);
        status = ort->Run(session, nullptr, in_names, &input_tensor, input_count, out_names, output_count,
                          output_tensors.data());

        if (status != nullptr) {
            std::string msg = ort->GetErrorMessage(status);
            ort->ReleaseStatus(status);
            // Cleanup
            ort->ReleaseValue(input_tensor);
            ort->ReleaseMemoryInfo(mem_info);
            ort->ReleaseSession(session);
            ort->ReleaseSessionOptions(opts);
            ort->ReleaseEnv(env);
            return makeError(BackendType::ONNX_RUNTIME, "ORT Run failed: " + msg);
        }

        // Extract first output tensor
        if (!output_tensors.empty() && output_tensors[0]) {
            float *out_data = nullptr;
            ort->GetTensorMutableData(output_tensors[0], reinterpret_cast<void **>(&out_data));
            OrtTensorTypeAndShapeInfo *shape_info = nullptr;
            ort->GetTensorTypeAndShape(output_tensors[0], &shape_info);
            size_t out_count = 0;
            ort->GetTensorShapeElementCount(shape_info, &out_count);
            size_t rank = 0;
            ort->GetDimensionsCount(shape_info, &rank);
            std::vector<int64_t> out_shape(rank);
            ort->GetDimensions(shape_info, out_shape.data(), rank);
            ort->ReleaseTensorTypeAndShapeInfo(shape_info);

            result.output.assign(out_data, out_data + out_count);
            result.output_shape = std::move(out_shape);
        }

        result.success = true;

        // Cleanup
        for (auto *v : output_tensors) {
            if (v) {
                ort->ReleaseValue(v);
            }
        }
        ort->ReleaseValue(input_tensor);
        ort->ReleaseMemoryInfo(mem_info);
        ort->ReleaseSession(session);
        ort->ReleaseSessionOptions(opts);
        ort->ReleaseEnv(env);

    } catch (const std::exception &e) {
        result.success = false;
        result.error   = std::string("ORT dispatch exception: ") + e.what();
    }

    auto t1           = std::chrono::steady_clock::now();
    result.latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return result;
#else
    static_cast<void>(req);
    return makeError(BackendType::ONNX_RUNTIME, "ONNX Runtime headers not found at compile time");
#endif
}

AiInferenceResult AiHardwareDispatcher::dispatchGpuFallback(AiInferenceRequest &req) {
    if (isVectorSimilarityTask(req.task_tag)) {
#ifdef THEMIS_ENABLE_CUDA
        CUDAVectorBackend cuda_backend = {};
        if (!cuda_backend.initialize()) {
            THEMIS_WARN("AiHardwareDispatcher: CUDA backend init failed for vector similarity — using CPU fallback");
            return runCpuVectorSimilarity(req);
        }

        AiInferenceResult gpu_result
            = runVectorSimilarityDispatch(cuda_backend.populateANNDispatch(), BackendType::CUDA, "CUDAExecutionProvider", req);
        if (!gpu_result.success) {
            THEMIS_WARN("AiHardwareDispatcher: CUDA vector similarity failed: {} — using CPU fallback", gpu_result.error);
            return runCpuVectorSimilarity(req);
        }
        return gpu_result;
#else
        THEMIS_WARN("AiHardwareDispatcher: CUDA vector similarity requested without THEMIS_ENABLE_CUDA — using CPU fallback");
        return runCpuVectorSimilarity(req);
#endif
    }

    THEMIS_DEBUG("AiHardwareDispatcher: GPU inference path unavailable for task '{}', falling back to CPU", req.task_tag);
    return dispatchCpuFallback(req);
}

AiInferenceResult AiHardwareDispatcher::dispatchCpuFallback(AiInferenceRequest &req) {
    if (isVectorSimilarityTask(req.task_tag)) {
        return runCpuVectorSimilarity(req);
    }

    if (req.input_data == nullptr || req.input_elements == 0) {
        return makeError(BackendType::CPU, "Invalid input: null or empty");
    }
    auto t0 = std::chrono::steady_clock::now();

    // CPU fallback: identity copy (caller is expected to provide a pre-computed
    // embedding or run the model via a separate thread-pool).  Real inference
    // would call an llama.cpp / ggml or OpenBLAS routine here.
    AiInferenceResult result;
    result.success      = true;
    result.backend_used = BackendType::CPU;
    result.ep_used      = "CPUExecutionProvider";
    result.output.assign(req.input_data, req.input_data + req.input_elements);
    result.output_shape = req.input_shape;

    auto t1           = std::chrono::steady_clock::now();
    result.latency_ms = std::chrono::duration<double, std::milli>(t1 - t0).count();
    return result;
}

} // namespace acceleration
} // namespace themis
