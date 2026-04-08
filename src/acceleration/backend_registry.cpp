/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            backend_registry.cpp                               ║
  Version:         0.0.37                                             ║
  Last Modified:   2026-04-06 04:13:58                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     509                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e627c556bd  2026-03-15  feat(acceleration): BackendRegistry thread-safety, VLLMRe... ║
    • 6afb158447  2026-03-15  feat(acceleration): replace std::cout with structured log... ║
    • 4b2fdfa0e1  2026-03-11  fix(acceleration): Wire OpenGLVectorBackend into BackendR... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • f2fa0c5eb7  2026-02-23  fix(acceleration): address code-audit gaps — deviceInfo()... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "acceleration/compute_backend.h"
#include "acceleration/plugin_loader.h"
#include "acceleration/cpu_backend.h"
#include "acceleration/multi_gpu_backend.h"
#include "acceleration/device_manager.h"
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

namespace themis {
namespace acceleration {

// The canonical fallback chain: highest priority first, CPU always last.
// All getBestXBackend() methods traverse this list in order.
static const std::vector<BackendType> kFallbackOrder = {
    BackendType::MULTI_GPU,
    BackendType::CUDA,
    BackendType::HIP,
    BackendType::ZLUDA,
    BackendType::VULKAN,
    BackendType::DIRECTX,
    BackendType::ROCM,
    BackendType::ONEAPI,
    BackendType::METAL,
    BackendType::OPENCL,
    BackendType::OPENGL,
    BackendType::WEBGPU,
    BackendType::CPU,
};

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

BackendRegistry& BackendRegistry::instance() {
    static BackendRegistry instance;
    return instance;
}

void BackendRegistry::registerBackend(std::unique_ptr<IComputeBackend> backend) {
    if (backend && backend->isAvailable()) {
        std::unique_lock<std::shared_mutex> lock(registryMutex_);
        THEMIS_INFO("Registered backend: {} (type={})", backend->name(), static_cast<int>(backend->type()));
        backends_.push_back(std::move(backend));
    }
}

size_t BackendRegistry::loadPlugins(const std::string& pluginDirectory) {
    THEMIS_INFO("Loading acceleration plugins from: {}", pluginDirectory);
    
    size_t count = pluginLoader_->loadPluginsFromDirectory(pluginDirectory);
    
    // Register backends from loaded plugins
    for (auto* plugin : pluginLoader_->getLoadedPlugins()) {
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

bool BackendRegistry::loadPlugin(const std::string& pluginPath) {
    THEMIS_INFO("Loading acceleration plugin: {}", pluginPath);
    
    if (!pluginLoader_->loadPlugin(pluginPath)) {
        return false;
    }
    
    // Get the newly loaded plugin
    auto plugins = pluginLoader_->getLoadedPlugins();
    if (!plugins.empty()) {
        auto* plugin = plugins.back();
        
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

IComputeBackend* BackendRegistry::getBackend(BackendType type) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    // Use the O(1) index when it has been built by initializeRuntime().
    if (!backendTypeIndex_.empty()) {
        auto it = backendTypeIndex_.find(static_cast<int>(type));
        if (it != backendTypeIndex_.end()) return it->second;
        return nullptr;
    }
    // Fallback linear scan before initializeRuntime() is called.
    for (const auto& backend : backends_) {
        if (backend->type() == type) {
            return backend.get();
        }
    }
    return nullptr;
}

// static
const std::vector<BackendType>& BackendRegistry::getFallbackOrder() noexcept {
    return kFallbackOrder;
}

// Internal helper: traverse kFallbackOrder and return the first backend that
// satisfies reqs and can be downcast to T*. T must be IComputeBackend or a
// subtype (IVectorBackend, IGraphBackend, IGeoBackend).
//
// When backendTypeIndex_ has been built (after initializeRuntime()) each
// priority-level lookup is a single O(1) map lookup rather than a linear scan
// over all backends_.  The map stores the *first* registered backend of each
// type; for the common case of one backend per type this is equivalent to the
// original nested-loop and avoids repeated dynamic_cast calls in the hot path.
//
// NOTE: callers must hold at least a shared lock on BackendRegistry::registryMutex_
// before calling this function.  The lock is NOT acquired here to allow both
// shared (read-only) and exclusive (write) callers to reuse the same helper.
template <typename T>
static T* selectTyped(const std::vector<std::unique_ptr<IComputeBackend>>& backends,
                      const BackendRegistry::CapabilityRequirements& reqs,
                      const std::unordered_map<int, IComputeBackend*>& index) {
    if (!index.empty()) {
        // O(|kFallbackOrder|) path: one map lookup per priority level.
        for (auto btype : kFallbackOrder) {
            auto it = index.find(static_cast<int>(btype));
            if (it == index.end()) continue;
            IComputeBackend* raw = it->second;
            if (!BackendRegistry::satisfies(raw->getCapabilities(), reqs)) continue;
            T* typed = dynamic_cast<T*>(raw);
            if (typed) return typed;
        }
        return nullptr;
    }
    // O(|kFallbackOrder| × |backends|) fallback before initializeRuntime().
    for (auto btype : kFallbackOrder) {
        for (const auto& backend : backends) {
            if (backend->type() == btype && BackendRegistry::satisfies(backend->getCapabilities(), reqs)) {
                T* typed = dynamic_cast<T*>(backend.get());
                if (typed) return typed;
            }
        }
    }
    return nullptr;
}

IComputeBackend* BackendRegistry::selectBackendFor(const CapabilityRequirements& reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectTyped<IComputeBackend>(backends_, reqs, backendTypeIndex_);
}

IVectorBackend* BackendRegistry::selectVectorBackendFor(const CapabilityRequirements& reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectTyped<IVectorBackend>(backends_, reqs, backendTypeIndex_);
}

IGraphBackend* BackendRegistry::selectGraphBackendFor(const CapabilityRequirements& reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectTyped<IGraphBackend>(backends_, reqs, backendTypeIndex_);
}

IGeoBackend* BackendRegistry::selectGeoBackendFor(const CapabilityRequirements& reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectTyped<IGeoBackend>(backends_, reqs, backendTypeIndex_);
}

IMatrixBackend* BackendRegistry::selectMatrixBackendFor(const CapabilityRequirements& reqs) const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectTyped<IMatrixBackend>(backends_, reqs, backendTypeIndex_);
}

IVectorBackend* BackendRegistry::getBestVectorBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsVectorOps) {
                auto* vectorBackend = dynamic_cast<IVectorBackend*>(backend.get());
                if (vectorBackend) {
                    return vectorBackend;
                }
            }
        }
    }
    return nullptr;
}

IGraphBackend* BackendRegistry::getBestGraphBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsGraphOps) {
                auto* graphBackend = dynamic_cast<IGraphBackend*>(backend.get());
                if (graphBackend) {
                    return graphBackend;
                }
            }
        }
    }
    return nullptr;
}

IGeoBackend* BackendRegistry::getBestGeoBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsGeoOps) {
                auto* geoBackend = dynamic_cast<IGeoBackend*>(backend.get());
                if (geoBackend) {
                    return geoBackend;
                }
            }
        }
    }
    return nullptr;
}

IMatrixBackend* BackendRegistry::getBestMatrixBackend() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    for (auto type : kFallbackOrder) {
        for (const auto& backend : backends_) {
            if (backend->type() == type && backend->getCapabilities().supportsMatrixOps) {
                auto* matrixBackend = dynamic_cast<IMatrixBackend*>(backend.get());
                if (matrixBackend) {
                    return matrixBackend;
                }
            }
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
        "./plugins",                    // Current directory
        "./lib/themis/plugins",         // Relative to binary
        "/usr/local/lib/themis/plugins", // System-wide (Linux)
        "/opt/themis/plugins",          // Alternative system location
#ifdef _WIN32
        "C:/Program Files/ThemisDB/plugins",
#endif
    };
    
    for (const auto& path : pluginPaths) {
        loadPlugins(path);
    }
    
    // Acquire shared lock only for the read-only logging pass.
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    THEMIS_INFO("Total backends available: {}", backends_.size());
    
    // Print available backends
    for (const auto& backend : backends_) {
        auto caps = backend->getCapabilities();
        THEMIS_DEBUG("  - {} (Vector:{} Graph:{} Geo:{})", backend->name(),
                     (caps.supportsVectorOps ? "Yes" : "No"),
                     (caps.supportsGraphOps  ? "Yes" : "No"),
                     (caps.supportsGeoOps    ? "Yes" : "No"));
    }
}

std::vector<BackendType> BackendRegistry::getAvailableBackends() const {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    std::vector<BackendType> types;
    types.reserve(backends_.size());
    
    for (const auto& backend : backends_) {
        types.push_back(backend->type());
    }
    
    return types;
}

void BackendRegistry::shutdownAll() {
    THEMIS_INFO("Shutting down all acceleration backends...");

    // Acquire exclusive lock for the full shutdown + clear sequence.
    std::unique_lock<std::shared_mutex> lock(registryMutex_);
    
    for (auto& backend : backends_) {
        backend->shutdown();
    }
    backends_.clear();

    // Clear cached startup selections; the backends they pointed to are gone.
    selectedVectorBackend_ = nullptr;
    selectedGraphBackend_  = nullptr;
    selectedGeoBackend_    = nullptr;
    runtimeInitialized_.store(false, std::memory_order_release);
    cachedDeviceInfo_.clear();
    backendTypeIndex_.clear();
    
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
    reqs.needsVectorOps      = true;
    reqs.requiredPrecisions  = PrecisionMode::FP32;
    reqs.requiredMetrics     = metricBit(DistanceMetric::L2)
                             | metricBit(DistanceMetric::COSINE)
                             | metricBit(DistanceMetric::INNER_PRODUCT);
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
    reqs.needsGeoOps         = true;
    reqs.requiredPrecisions  = PrecisionMode::FP32;
    return reqs;
}

// ---------------------------------------------------------------------------
// Runtime startup initialization
// ---------------------------------------------------------------------------

void BackendRegistry::initializeRuntime(
    const CapabilityRequirements& vectorReqs,
    const CapabilityRequirements& graphReqs,
    const CapabilityRequirements& geoReqs)
{
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

        // Build the O(1) BackendType → IComputeBackend* lookup index.
        // For each type we keep only the *first* registered backend (highest priority
        // within the type, as backends_ is appended to in registration order).
        backendTypeIndex_.clear();
        backendTypeIndex_.reserve(backends_.size());
        for (const auto& b : backends_) {
            backendTypeIndex_.emplace(static_cast<int>(b->type()), b.get());
        }

        // selectTyped() requires the lock to be held (shared is sufficient; we
        // hold exclusive here so it is already satisfied).
        selectedVectorBackend_ = selectTyped<IVectorBackend>(backends_, vectorReqs, backendTypeIndex_);
        selectedGraphBackend_  = selectTyped<IGraphBackend>(backends_, graphReqs, backendTypeIndex_);
        selectedGeoBackend_    = selectTyped<IGeoBackend>(backends_, geoReqs, backendTypeIndex_);
        runtimeInitialized_.store(true, std::memory_order_release);
    }

    // Log the selected backends so operators can confirm the startup choice.
    // Acquire a shared lock for the read-only log pass.
    std::shared_lock<std::shared_mutex> rlock(registryMutex_);
    auto logSelection = [](const char* category, const IComputeBackend* b) {
        if (b) {
            THEMIS_INFO("[acceleration] Selected {} backend: {} (type={})",
                        category, b->name(), static_cast<int>(b->type()));
        } else {
            THEMIS_WARN("[acceleration] No suitable {} backend found — operation category unavailable.",
                        category);
        }
    };

    logSelection("vector", selectedVectorBackend_);
    logSelection("graph",  selectedGraphBackend_);
    logSelection("geo",    selectedGeoBackend_);
}

IVectorBackend* BackendRegistry::getSelectedVectorBackend() const noexcept {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectedVectorBackend_;
}

IGraphBackend* BackendRegistry::getSelectedGraphBackend() const noexcept {
    std::shared_lock<std::shared_mutex> lock(registryMutex_);
    return selectedGraphBackend_;
}

IGeoBackend* BackendRegistry::getSelectedGeoBackend() const noexcept {
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
