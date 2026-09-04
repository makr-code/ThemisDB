/**
 * @file device_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/*
 * Acceleration module — Runtime Device Capability Detection
 * ==========================================================
 * Wraps themis::gpu::DeviceDiscovery to expose per-device capability info
 * (BackendType mapping, precision flags, VRAM, compute capability) to the
 * acceleration layer with a 60-second probe cache and startup logging.
 *
 * Dispatch chain position
 * -----------------------
 *   BackendRegistry::initializeRuntime()
 *       └─► DeviceManager::instance().probeDevices()   ← this file
 *               └─► themis::gpu::DeviceDiscovery::Enumerate()
 *                       └─► CUDA / ROCm / Vulkan / CPU device enumeration
 *       └─► BackendRegistry scores capabilities per device
 *       └─► best backend selected per operation category (vector/graph/geo)
 *
 * Key interfaces implemented / exposed
 * -------------------------------------
 *   DeviceManager::instance()      — singleton access
 *   DeviceManager::probeDevices()  — enumerate and cache device capabilities
 *   DeviceManager::refresh()       — force re-probe (invalidates 60 s TTL cache)
 *   DeviceManager::getBestDevice() — return highest-scoring DeviceCapabilityInfo
 *   DeviceManager::deviceInfo()    — immutable snapshot captured at last probe
 *
 * Related files
 * -------------
 *   include/acceleration/device_manager.h      — DeviceManager / DeviceCapabilityInfo declarations
 *   include/themis/gpu/device_discovery.h      — underlying GPU discovery layer
 *   src/acceleration/backend_registry.cpp      — consumes DeviceManager during initializeRuntime()
 *   src/acceleration/ARCHITECTURE.md           — startup flow (Section 4.1)
 */
#include "acceleration/device_manager.h"

#include <iostream>

#include "themis/gpu/device_discovery.h"

namespace themis {
namespace acceleration {

namespace {

std::mutex &enumerateFnMutex() {
    static std::mutex mutex;
    return mutex;
}

DeviceManager::EnumerateFn &enumerateFnStorage() {
    static DeviceManager::EnumerateFn enumerate_fn;
    return enumerate_fn;
}

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------

BackendType mapBackendType(const std::string &backend_str) noexcept {
    if (backend_str == "CUDA") {
        return BackendType::CUDA;
    }
    if (backend_str == "ROCm") {
        return BackendType::ROCM;
    }
    if (backend_str == "Vulkan") {
        return BackendType::VULKAN;
    }
    if (backend_str == "Metal") {
        return BackendType::METAL;
    }
    if (backend_str == "OpenCL") {
        return BackendType::OPENCL;
    }
    if (backend_str == "CPU_FALLBACK") {
        return BackendType::CPU;
    }
    return BackendType::CPU;
}

DeviceCapabilityInfo fromGpuDeviceInfo(const themis::gpu::DeviceInfo &d) noexcept {
    DeviceCapabilityInfo info;
    info.index            = d.index;
    info.name             = d.name;
    info.backend_type     = mapBackendType(d.backend);
    info.total_vram_bytes = d.total_vram_bytes;
    info.free_vram_bytes  = d.free_vram_bytes;
    info.compute_major    = d.compute_major;
    info.compute_minor    = d.compute_minor;
    info.is_healthy       = d.is_healthy;
    info.error_message    = d.error_message;

    // Derive precision support from compute capability.
    // FP16 (half precision) requires sm_70+ for Tensor Cores (CUDA Volta+).
    // BF16 requires sm_80+ (CUDA Ampere+).
    // For ROCm / CPU, leave the flags false (conservative default).
    if (d.backend == "CUDA" && d.is_healthy) {
        const int sm       = d.compute_major * 10 + d.compute_minor;
        info.supports_fp16 = (sm >= 70);
        info.supports_bf16 = (sm >= 80);
    }

    return info;
}

/// Enumerate devices from DeviceDiscovery and translate to DeviceCapabilityInfo.
/// Returns at least one CPU fallback entry.
std::vector<DeviceCapabilityInfo> enumerateDevices() {
    DeviceManager::EnumerateFn enumerate_fn;
    {
        std::lock_guard<std::mutex> lock(enumerateFnMutex());
        enumerate_fn = enumerateFnStorage();
    }

    if (enumerate_fn) {
        auto injected = enumerate_fn();
        if (injected.empty()) {
            DeviceCapabilityInfo cpu;
            cpu.index        = -1;
            cpu.name         = "CPU Fallback";
            cpu.backend_type = BackendType::CPU;
            cpu.is_healthy   = true;
            injected.push_back(cpu);
        }
        return injected;
    }

    const auto gpu_devices = themis::gpu::DeviceDiscovery::Enumerate();

    std::vector<DeviceCapabilityInfo> result = {};

    result.reserve(gpu_devices.size());

    for (const auto &d : gpu_devices) {
        result.push_back(fromGpuDeviceInfo(d));
    }

    if (result.empty()) {
        DeviceCapabilityInfo cpu;
        cpu.index        = -1;
        cpu.name         = "CPU Fallback";
        cpu.backend_type = BackendType::CPU;
        cpu.is_healthy   = true;
        result.push_back(cpu);
    }

    return result;
}

} // namespace

// ============================================================================
// Singleton
// ============================================================================

DeviceManager &DeviceManager::instance() {
    static DeviceManager inst;
    return inst;
}

// ============================================================================
// Public API
// ============================================================================

std::vector<DeviceCapabilityInfo> DeviceManager::probeDevices() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (cache_valid_) {
        const auto now = std::chrono::steady_clock::now();
        if (now - cache_time_ < kCacheTTL) {
            return cached_;
        }
    }

    cached_      = enumerateDevices();
    cache_time_  = std::chrono::steady_clock::now();
    cache_valid_ = true;
    return cached_;
}

std::vector<DeviceCapabilityInfo> DeviceManager::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);
    cached_      = enumerateDevices();
    cache_time_  = std::chrono::steady_clock::now();
    cache_valid_ = true;
    return cached_;
}

DeviceCapabilityInfo DeviceManager::getBestDevice() {
    const auto devices = probeDevices();

    // Pick the healthy non-CPU device with the highest free VRAM.
    const DeviceCapabilityInfo *best = nullptr;
    for (const auto &d : devices) {
        if (!d.is_healthy) {
            continue;
        }
        if (d.backend_type == BackendType::CPU) {
            continue;
        }
        if (best == nullptr || d.free_vram_bytes > best->free_vram_bytes) {
            best = &d;
        }
    }
    if (best != nullptr) {
        return *best;
    }

    // Return CPU fallback.
    for (const auto &d : devices) {
        if (d.backend_type == BackendType::CPU) {
            return d;
        }
    }

    // Synthesise a safe default.
    DeviceCapabilityInfo cpu;
    cpu.index        = -1;
    cpu.name         = "CPU Fallback";
    cpu.backend_type = BackendType::CPU;
    cpu.is_healthy   = true;
    return cpu;
}

bool DeviceManager::hasGPU() {
    const auto devices = probeDevices();
    for (const auto &d : devices) {
        if (d.is_healthy && d.backend_type != BackendType::CPU) {
            return true;
        }
    }
    return false;
}

BackendType DeviceManager::bestBackendType() {
    return getBestDevice().backend_type;
}

void DeviceManager::logDeviceInfo() {
    const auto devices = probeDevices();
    const auto best    = getBestDevice();

    std::cout << "[acceleration] Device capability probe — " << devices.size() << " device(s) found:" << std::endl;

    for (const auto &d : devices) {
        std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] " << d.name
                  << "  backend=" << static_cast<int>(d.backend_type)
                  << "  vram_free=" << (d.free_vram_bytes / (1024 * 1024)) << " MB"
                  << "  sm=" << d.compute_major << "." << d.compute_minor
                  << "  fp16=" << (d.supports_fp16 ? "yes" : "no") << "  bf16=" << (d.supports_bf16 ? "yes" : "no")
                  << std::endl;
    }

    std::cout << "[acceleration] Best device: " << best.name << " (backend=" << static_cast<int>(best.backend_type)
              << ")" << std::endl;
}

void DeviceManager::setEnumerateFn(EnumerateFn fn) {
    auto &manager = DeviceManager::instance();
    std::lock_guard<std::mutex> cache_lock(manager.mutex_);
    std::lock_guard<std::mutex> lock(::themis::acceleration::enumerateFnMutex());
    enumerateFnStorage() = std::move(fn);
    manager.cache_valid_ = false;
    manager.cached_.clear();
}

} // namespace acceleration
} // namespace themis
