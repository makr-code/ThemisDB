/**
 * @file backend_registry.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Acceleration module — Backend Registry (Central Dispatch Coordinator)
 * ======================================================================
 * BackendRegistry is the process-wide singleton that wires together all
 * acceleration backends.  It is the only component that knows about every
 * available backend and selects the best one per operation category at
 * startup.
 *
 * Dispatch chain position
 * -----------------------
 *   Callers (AQL executor, vector index, geo module)
 *       └─► BackendRegistry::getSelected{Vector,Graph,Geo}Backend()
 *               └─► concrete backend (CUDA / HIP / Vulkan / OpenCL / CPU)
 *                       └─► ANNKernelFallbackDispatcher / GeoKernelFallbackDispatcher
 *                               └─► retry + CPU fallback
 *
 * Key interfaces implemented / exposed
 * -------------------------------------
 *   initializeRuntime(vectorReqs, graphReqs, geoReqs)  — call once at startup
 *   getSelectedVectorBackend()                          — returns best IVectorBackend*
 *   getSelectedGraphBackend()                           — returns best IGraphBackend*
 *   getSelectedGeoBackend()                             — returns best IGeoBackend*
 *   registerBackend(backend)                            — register additional backends
 *   defaultVector/Graph/GeoRequirements()               — factory helpers for CapabilityRequirements
 *
 * Related files
 * -------------
 *   include/acceleration/compute_backend.h     — IComputeBackend interface + RegisteredBackend
 *   src/acceleration/device_manager.cpp        — device enumeration called during autoDetect()
 *   src/acceleration/plugin_loader.cpp         — plugin loading extends the registry
 *   include/acceleration/kernel_fallback_dispatcher.h — downstream retry/fallback dispatchers
 *   src/acceleration/ARCHITECTURE.md           — component diagram and startup flow
 */
#include "acceleration/compute_backend.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/device_manager.h"
#include "acceleration/multi_gpu_backend.h"
#include "acceleration/plugin_loader.h"
#include "utils/logger.h"
#ifdef THEMIS_ENABLE_VULKAN
#include "acceleration/graphics_backends.h"
#endif
#if defined(THEMIS_ENABLE_OPENGL) && !defined(THEMIS_ENABLE_VULKAN)
// graphics_backends.h declares both VulkanVectorBackend and OpenGLVectorBackend.
// Include it here when Vulkan is disabled so OpenGLVectorBackend is reachable.
#include "acceleration/graphics_backends.h"
#endif
#ifdef THEMIS_ENABLE_HIP
#include "acceleration/hip_backend.h"
#endif
#ifdef THEMIS_ENABLE_CUDA
#include "acceleration/cuda_backend.h"
#endif
#ifdef THEMIS_ENABLE_OPENCL
#include "acceleration/opencl_backend.h"
#endif
#include <algorithm>
#include <mutex>
#include <shared_mutex>
#include <unordered_map>

namespace themis {
namespace acceleration {

// The canonical fallback chain: highest priority first, CPU always last.
// All getBestXBackend() methods traverse this list in order.
static const std::vector<BackendType> kFallbackOrder = {
    BackendType::MULTI_GPU, BackendType::CUDA,   BackendType::HIP,    BackendType::ZLUDA, BackendType::VULKAN,
    BackendType::DIRECTX,   BackendType::ROCM,   BackendType::ONEAPI, BackendType::METAL, BackendType::OPENCL,
    BackendType::OPENGL,    BackendType::WEBGPU, BackendType::CPU,
};

static bool hasAcceleratorDevice(const std::vector<DeviceCapabilityInfo> &devices) noexcept {
    return std::any_of(devices.begin(), devices.end(), [](const DeviceCapabilityInfo &device) {
        return device.is_healthy && device.backend_type != BackendType::CPU;
    });
}

// Singleton instance
BackendRegistry::BackendRegistry() : pluginLoader_(std::make_unique<PluginLoader>()) {
    // Always register CPU backends (fallback)
    registerBackend(std::make_unique<CPUVectorBackend>());
    registerBackend(std::make_unique<CPUGraphBackend>());
    registerBackend(std::make_unique<CPUGeoBackend>());
    registerBackend(std::make_unique<CPUMatrixBackend>());

    // Register CUDA matrix backend for Tensor Core FP16/BF16 acceleration.
    // registerBackend() checks isAvailable() at runtime; silently skipped
    // when no CUDA-capable GPU is detected.
#ifdef THEMIS_ENABLE_CUDA
    registerBackend(std::make_unique<CUDAMatrixBackend>());
#endif

    // Register Vulkan backend when compiled with Vulkan support.
    // registerBackend() checks isAvailable() at runtime, so if no Vulkan
    // ICD is present the backend is silently skipped and CPU remains the
    // fallback. This is the primary fallback path for non-NVIDIA hardware
    // (AMD, Intel, ARM, Qualcomm) that has no CUDA but supports Vulkan 1.x.
#ifdef THEMIS_ENABLE_VULKAN
    registerBackend(std::make_unique<VulkanVectorBackend>());
#endif

    // Register HIP vector and geo backends for AMD GPUs.
    // registerBackend() checks isAvailable() at runtime; both are silently
    // skipped when no ROCm-capable GPU is present.
#ifdef THEMIS_ENABLE_HIP
    registerBackend(std::make_unique<HIPVectorBackend>());
    registerBackend(std::make_unique<HIPGeoBackend>());
#endif

    // Register OpenCL backend for broad hardware compatibility.
    // Supports any OpenCL 1.2+ capable device: NVIDIA, AMD, Intel, ARM, Qualcomm.
    // registerBackend() checks isAvailable() at runtime; silently skipped when
    // no OpenCL ICD is present.
#ifdef THEMIS_ENABLE_OPENCL
    registerBackend(std::make_unique<OpenCLVectorBackend>());
#endif

    // Register OpenGL Compute Shader backend for platform-wide GPU acceleration.
    // Uses EGL headless context (no display required); falls back to CPU kernels
    // when EGL/GL 4.3+ is unavailable so initialize() always succeeds.
    // registerBackend() checks isAvailable() at runtime; on systems without a
    // compatible EGL driver the backend still initialises in CPU-fallback mode.
#ifdef THEMIS_ENABLE_OPENGL
    registerBackend(std::make_unique<OpenGLVectorBackend>());
#endif
}

BackendRegistry::~BackendRegistry() {
    shutdownAll();
}

BackendRegistry &BackendRegistry::instance() {
    static BackendRegistry instance;
    return instance;
}

void BackendRegistry::registerBackend(std::unique_ptr<IComputeBackend> backend) {
    if (backend && backend->isAvailable()) {
        std::unique_lock<std::shared_mutex> lock(registryMutex_);
        THEMIS_INFO("Registered backend: {} (type={})", backend->name(), static_cast<int>(backend->type()));

        // Perform all dynamic_casts once at registration time so the hot
        // query path (selectTyped / getBestXBackend) never needs to cast.
        const BackendType bt  = backend->type();
        RegisteredBackend &rb = typeIndex_[bt]; // creates default entry if absent
        if (!rb.base) {
            rb.base = backend.get();
        }
        if (!rb.vectorPtr) {
            rb.vectorPtr = dynamic_cast<IVectorBackend *>(backend.get());
        }
        if (!rb.graphPtr) {
            rb.graphPtr = dynamic_cast<IGraphBackend *>(backend.get());
        }
        if (!rb.geoPtr) {
            rb.geoPtr = dynamic_cast<IGeoBackend *>(backend.get());
        }
        if (!rb.matrixPtr) {
            rb.matrixPtr = dynamic_cast<IMatrixBackend *>(backend.get());
        }

        backends_.push_back(std::move(backend));
    }
}

size_t BackendRegistry::loadPlugins(const std::string &pluginDirectory) {
    THEMIS_INFO("Loading acceleration plugins from: {}", pluginDirectory);

    size_t count = pluginLoader_->loadPluginsFromDirectory(pluginDirectory);

    // Register backends from loaded plugins
    for (auto *plugin : pluginLoader_->getLoadedPlugins()) {
        // Try to create each type of backend
        if (auto vectorBackend = plugin->createVectorBackend()) {
            registerBackend(std::move(vectorBackend));
        }

        if (auto graphBackend = plugin->createGraphBackend()) {
            registerBackend(std::move(graphBackend));
        }

        if (auto geoBackend = plugin->createGeoBackend()) {
            registerBackend(std::move(geoBackend));
        }
    }

    return count;
}

bool BackendRegistry::loadPlugin(const std::string &pluginPath) {
    THEMIS_INFO("Loading acceleration plugin: {}", pluginPath);

    if (!pluginLoader_->loadPlugin(pluginPath)) {
        return false;
    }

    // Get the newly loaded plugin
    auto plugins = pluginLoader_->getLoadedPlugins();
    if (!plugins.empty()) {
        auto *plugin = plugins.back();

        // Null-safety: Validate plugin pointer before use
        if (!plugin) {
            THEMIS_ERROR("Plugin pointer is null after loading");
            return false;
        }

        // Register backends
        if (auto vectorBackend = plugin->createVectorBackend()) {
            registerBackend(std::move(vectorBackend));
        }

        if (auto graphBackend = plugin->createGraphBackend()) {
            registerBackend(std::move(graphBackend));
        }

        if (auto geoBackend = plugin->createGeoBackend()) {
            registerBackend(std::move(geoBackend));
        }

        return true;
    }

    return false;
}

IComputeBackend *BackendRegistry::getBackend(BackendType type) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    auto it = typeIndex_.find(type);
    return (it != typeIndex_.end()) ? it->second.base : nullptr;
}

// static
const std::vector<BackendType> &BackendRegistry::getFallbackOrder() noexcept {
    return kFallbackOrder;
}

// Internal helper: extract the typed interface pointer for template type T from
// a RegisteredBackend entry.  Specialisations cover all four typed interfaces
// plus the base IComputeBackend (for selectBackendFor).  The primary template
// returns nullptr so that an unknown T compiles but produces no result.
template <typename T> static T *getTypedPtr(const RegisteredBackend &) noexcept = delete;
template <> IComputeBackend *getTypedPtr<IComputeBackend>(const RegisteredBackend &rb) noexcept {
    return rb.base;
}
template <> IVectorBackend *getTypedPtr<IVectorBackend>(const RegisteredBackend &rb) noexcept {
    return rb.vectorPtr;
}
template <> IGraphBackend *getTypedPtr<IGraphBackend>(const RegisteredBackend &rb) noexcept {
    return rb.graphPtr;
}
template <> IGeoBackend *getTypedPtr<IGeoBackend>(const RegisteredBackend &rb) noexcept {
    return rb.geoPtr;
}
template <> IMatrixBackend *getTypedPtr<IMatrixBackend>(const RegisteredBackend &rb) noexcept {
    return rb.matrixPtr;
}

// Internal helper: traverse kFallbackOrder and return the first backend that
// satisfies reqs and exposes the typed interface T.  Uses the pre-built
// typeIndex for O(|kFallbackOrder|) lookup; each priority level costs a
// single hash-map lookup rather than an O(|backends|) linear scan.
//
// T must be one of IVectorBackend, IGraphBackend, IGeoBackend, or IMatrixBackend.
//
// NOTE: callers must hold at least a shared lock on BackendRegistry::registryMutex_
// before calling this function.  The lock is NOT acquired here to allow both
// shared (read-only) and exclusive (write) callers to reuse the same helper.
template <typename T>
static T *selectTyped(const std::unordered_map<BackendType, RegisteredBackend> &index,
                      const BackendRegistry::CapabilityRequirements &reqs,
                      bool cpuOnlyEnvironment = false) noexcept {
    for (auto type : kFallbackOrder) {
        if (cpuOnlyEnvironment && type != BackendType::CPU) {
            continue;
        }
        auto it = index.find(type);
        if (it == index.end()) {
            continue;
        }
        T *typed = getTypedPtr<T>(it->second);
        if (!typed) {
            continue;
        }
        if (!BackendRegistry::satisfies(typed->getCapabilities(), reqs)) {
            continue;
        }
        return typed;
    }
    return nullptr;
}

IComputeBackend *BackendRegistry::selectBackendFor(const CapabilityRequirements &reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    // Uses the full backends_ list so that all registered IComputeBackend
    // instances (including specialised backends that share a BackendType) are
    // considered.  No dynamic_cast is needed — all backends are IComputeBackend.
    for (auto type : kFallbackOrder) {
        if (cpuOnlyEnvironment && type != BackendType::CPU) {
            continue;
        }
        for (const auto &backend : backends_) {
            if (backend->type() == type && satisfies(backend->getCapabilities(), reqs)) {
                return backend.get();
            }
        }
    }
    return nullptr;
}

IVectorBackend *BackendRegistry::selectVectorBackendFor(const CapabilityRequirements &reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    return selectTyped<IVectorBackend>(typeIndex_, reqs, cpuOnlyEnvironment);
}

IGraphBackend *BackendRegistry::selectGraphBackendFor(const CapabilityRequirements &reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    return selectTyped<IGraphBackend>(typeIndex_, reqs, cpuOnlyEnvironment);
}

IGeoBackend *BackendRegistry::selectGeoBackendFor(const CapabilityRequirements &reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    return selectTyped<IGeoBackend>(typeIndex_, reqs, cpuOnlyEnvironment);
}

IMatrixBackend *BackendRegistry::selectMatrixBackendFor(const CapabilityRequirements &reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    return selectTyped<IMatrixBackend>(typeIndex_, reqs, cpuOnlyEnvironment);
}

IVectorBackend *BackendRegistry::getBestVectorBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    for (auto type : kFallbackOrder) {
        if (cpuOnlyEnvironment && type != BackendType::CPU) {
            continue;
        }
        auto it = typeIndex_.find(type);
        if (it != typeIndex_.end() && it->second.vectorPtr) {
            return it->second.vectorPtr;
        }
    }
    return nullptr;
}

IGraphBackend *BackendRegistry::getBestGraphBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    for (auto type : kFallbackOrder) {
        if (cpuOnlyEnvironment && type != BackendType::CPU) {
            continue;
        }
        auto it = typeIndex_.find(type);
        if (it != typeIndex_.end() && it->second.graphPtr) {
            return it->second.graphPtr;
        }
    }
    return nullptr;
}

IGeoBackend *BackendRegistry::getBestGeoBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    for (auto type : kFallbackOrder) {
        if (cpuOnlyEnvironment && type != BackendType::CPU) {
            continue;
        }
        auto it = typeIndex_.find(type);
        if (it != typeIndex_.end() && it->second.geoPtr) {
            return it->second.geoPtr;
        }
    }
    return nullptr;
}

IMatrixBackend *BackendRegistry::getBestMatrixBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    const bool cpuOnlyEnvironment = runtimeInitialized_.load(std::memory_order_acquire)
                                 && !hasAcceleratorDevice(cachedDeviceInfo_);
    for (auto type : kFallbackOrder) {
        if (cpuOnlyEnvironment && type != BackendType::CPU) {
            continue;
        }
        auto it = typeIndex_.find(type);
        if (it != typeIndex_.end() && it->second.matrixPtr) {
            return it->second.matrixPtr;
        }
    }
    return nullptr;
}

void BackendRegistry::autoDetect() {
    THEMIS_INFO("Auto-detecting acceleration backends...");

    // Register multi-GPU sharding backend when multiple GPUs are visible.
    // This is done before plugin loading so it appears at the head of the
    // fallback chain and getBestVectorBackend() returns it first.
    // registerBackend() acquires the exclusive lock internally — no outer lock here.
    if (MultiGPUVectorBackend::detectGPUCount() >= 2) {
        registerBackend(std::make_unique<MultiGPUVectorBackend>());
    }

    // Try to load plugins from standard locations
    std::vector<std::string> pluginPaths = {
        "./plugins",                     // Current directory
        "./lib/themis/plugins",          // Relative to binary
        "/usr/local/lib/themis/plugins", // System-wide (Linux)
        "/opt/themis/plugins",           // Alternative system location
#ifdef _WIN32
        "C:/Program Files/ThemisDB/plugins",
#endif
    };

    for (const auto &path : pluginPaths) {
        loadPlugins(path);
    }

    // Acquire shared lock only for the read-only logging pass.
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    THEMIS_INFO("Total backends available: {}",static_cast<int>(backends_.size()));

    // Print available backends
    for (const auto &backend : backends_) {
        auto caps = backend->getCapabilities();
        THEMIS_DEBUG("  - {} (Vector:{} Graph:{} Geo:{})", backend->name(), (caps.supportsVectorOps ? "Yes" : "No"),
                     (caps.supportsGraphOps ? "Yes" : "No"), (caps.supportsGeoOps ? "Yes" : "No"));
    }
}

std::vector<BackendType> BackendRegistry::getAvailableBackends() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    std::vector<BackendType> types = {};

    types.reserve(backends_.size());

    for (const auto &backend : backends_) {
        types.push_back(backend->type());
    }

    return types;
}

void BackendRegistry::shutdownAll() {
    THEMIS_INFO("Shutting down all acceleration backends...");

    // Acquire exclusive lock for the full shutdown + clear sequence.
    std::unique_lock<std::shared_mutex> lock(registryMutex_);

    for (auto &backend : backends_) {
        backend->shutdown();
    }
    backends_.clear();
    typeIndex_.clear();

    // Clear cached startup selections; the backends they pointed to are gone.
    selectedVectorBackend_ = nullptr;
    selectedGraphBackend_  = nullptr;
    selectedGeoBackend_    = nullptr;
    runtimeInitialized_.store(false, std::memory_order_release);
    cachedDeviceInfo_.clear();

    if (pluginLoader_) {
        // Unlock before calling into pluginLoader_ to prevent potential
        // deadlock if a plugin destructor calls back into the registry.
        lock.unlock();
        pluginLoader_->unloadAllPlugins();
    }
}

// ---------------------------------------------------------------------------
// Default capability requirements
// ---------------------------------------------------------------------------

// static
BackendRegistry::CapabilityRequirements BackendRegistry::defaultVectorRequirements() noexcept {
    CapabilityRequirements reqs;
    reqs.needsVectorOps     = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;
    reqs.requiredMetrics
        = metricBit(DistanceMetric::L2) | metricBit(DistanceMetric::COSINE) | metricBit(DistanceMetric::INNER_PRODUCT);
    return reqs;
}

// static
BackendRegistry::CapabilityRequirements BackendRegistry::defaultGraphRequirements() noexcept {
    CapabilityRequirements reqs;
    reqs.needsGraphOps = true;
    return reqs;
}

// static
BackendRegistry::CapabilityRequirements BackendRegistry::defaultGeoRequirements() noexcept {
    CapabilityRequirements reqs;
    reqs.needsGeoOps        = true;
    reqs.requiredPrecisions = PrecisionMode::FP32;
    return reqs;
}

// ---------------------------------------------------------------------------
// Runtime startup initialization
// ---------------------------------------------------------------------------

void BackendRegistry::initializeRuntime(const CapabilityRequirements &vectorReqs,
                                        const CapabilityRequirements &graphReqs,
                                        const CapabilityRequirements &geoReqs) {
    THEMIS_INFO("Initializing acceleration runtime with capability-driven backend selection...");

    // Hardware probing and plugin loading are done without the registry lock
    // because autoDetect()/loadPlugins() call registerBackend() which acquires
    // the exclusive lock themselves.
    //
    // Step 1: Probe hardware (lock-free — DeviceManager has its own concurrency).
    std::vector<DeviceCapabilityInfo> deviceSnapshot = DeviceManager::instance().refresh();
    DeviceManager::instance().logDeviceInfo();

    // Step 2: Discover and load all available backends (registerBackend locks internally).
    autoDetect();

    // Step 3: Capability-driven selection + write results under exclusive lock.
    {
        std::unique_lock<std::shared_mutex> lock(registryMutex_);
        cachedDeviceInfo_ = std::move(deviceSnapshot);
        const bool cpuOnlyEnvironment = !hasAcceleratorDevice(cachedDeviceInfo_);
        // selectTyped() requires the lock to be held (shared is sufficient; we
        // hold exclusive here so it is already satisfied).
        selectedVectorBackend_ = selectTyped<IVectorBackend>(typeIndex_, vectorReqs, cpuOnlyEnvironment);
        selectedGraphBackend_  = selectTyped<IGraphBackend>(typeIndex_, graphReqs, cpuOnlyEnvironment);
        selectedGeoBackend_    = selectTyped<IGeoBackend>(typeIndex_, geoReqs, cpuOnlyEnvironment);
        runtimeInitialized_.store(true, std::memory_order_release);
    }

    // Log the selected backends so operators can confirm the startup choice.
    // Acquire a shared lock for the read-only log pass.
    std::shared_lock<std::shared_mutex> rlock(registryMutex_);
    auto logSelection = [](const char *category, const IComputeBackend *b) {
        if (b) {
            THEMIS_INFO("[acceleration] Selected {} backend: {} (type={})", category, b->name(),
                        static_cast<int>(b->type()));
        } else {
            THEMIS_WARN("[acceleration] No suitable {} backend found — operation category unavailable.", category);
        }
    };

    logSelection("vector", selectedVectorBackend_);
    logSelection("graph", selectedGraphBackend_);
    logSelection("geo", selectedGeoBackend_);
}

IVectorBackend *BackendRegistry::getSelectedVectorBackend() const noexcept {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectedVectorBackend_;
}

IGraphBackend *BackendRegistry::getSelectedGraphBackend() const noexcept {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectedGraphBackend_;
}

IGeoBackend *BackendRegistry::getSelectedGeoBackend() const noexcept {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectedGeoBackend_;
}

bool BackendRegistry::isRuntimeInitialized() const noexcept {
    return runtimeInitialized_.load(std::memory_order_acquire);
}

std::vector<DeviceCapabilityInfo> BackendRegistry::deviceInfo() const noexcept {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return cachedDeviceInfo_;
}

} // namespace acceleration
} // namespace themis
