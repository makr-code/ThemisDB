/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            device_manager.cpp                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:23:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     212                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • bcf21826fe  2026-02-23  feat(acceleration): implement runtime device capability d... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * Acceleration module — Runtime Device Capability Detection
 * ==========================================================
 * Wraps themis::gpu::DeviceDiscovery to expose per-device capability info
 * (BackendType mapping, precision flags, VRAM, compute capability) to the
 * acceleration layer with a 60-second probe cache and startup logging.
 */

#include "acceleration/device_manager.h"
#include "themis/gpu/device_discovery.h"

#include <iostream>

namespace themis {
namespace acceleration {

namespace {

// ---------------------------------------------------------------------------
// File-local helpers
// ---------------------------------------------------------------------------

BackendType mapBackendType(const std::string& backend_str) noexcept {
    if (backend_str == "CUDA")         return BackendType::CUDA;
    if (backend_str == "ROCm")         return BackendType::ROCM;
    if (backend_str == "Vulkan")       return BackendType::VULKAN;
    if (backend_str == "Metal")        return BackendType::METAL;
    if (backend_str == "OpenCL")       return BackendType::OPENCL;
    if (backend_str == "CPU_FALLBACK") return BackendType::CPU;
    return BackendType::CPU;
}

DeviceCapabilityInfo fromGpuDeviceInfo(const themis::gpu::DeviceInfo& d) noexcept {
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
        const int sm = d.compute_major * 10 + d.compute_minor;
        info.supports_fp16 = (sm >= 70);
        info.supports_bf16 = (sm >= 80);
    }

    return info;
}

/// Enumerate devices from DeviceDiscovery and translate to DeviceCapabilityInfo.
/// Returns at least one CPU fallback entry.
std::vector<DeviceCapabilityInfo> enumerateDevices() {
    const auto gpu_devices = themis::gpu::DeviceDiscovery::Enumerate();

    std::vector<DeviceCapabilityInfo> result;
    result.reserve(gpu_devices.size());

    for (const auto& d : gpu_devices) {
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

DeviceManager& DeviceManager::instance() {
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

    cached_     = enumerateDevices();
    cache_time_ = std::chrono::steady_clock::now();
    cache_valid_ = true;
    return cached_;
}

std::vector<DeviceCapabilityInfo> DeviceManager::refresh() {
    std::lock_guard<std::mutex> lock(mutex_);
    cached_     = enumerateDevices();
    cache_time_ = std::chrono::steady_clock::now();
    cache_valid_ = true;
    return cached_;
}

DeviceCapabilityInfo DeviceManager::getBestDevice() {
    const auto devices = probeDevices();

    // Pick the healthy non-CPU device with the highest free VRAM.
    const DeviceCapabilityInfo* best = nullptr;
    for (const auto& d : devices) {
        if (!d.is_healthy)                  continue;
        if (d.backend_type == BackendType::CPU) continue;
        if (best == nullptr || d.free_vram_bytes > best->free_vram_bytes) {
            best = &d;
        }
    }
    if (best != nullptr) {
        return *best;
    }

    // Return CPU fallback.
    for (const auto& d : devices) {
        if (d.backend_type == BackendType::CPU) return d;
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
    for (const auto& d : devices) {
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

    std::cout << "[acceleration] Device capability probe — "
              << devices.size() << " device(s) found:" << std::endl;

    for (const auto& d : devices) {
        std::cout << "  [" << (d.is_healthy ? "OK" : "!!") << "] "
                  << d.name
                  << "  backend=" << static_cast<int>(d.backend_type)
                  << "  vram_free=" << (d.free_vram_bytes / (1024ULL * 1024ULL)) << " MB"
                  << "  sm=" << d.compute_major << "." << d.compute_minor
                  << "  fp16=" << (d.supports_fp16 ? "yes" : "no")
                  << "  bf16=" << (d.supports_bf16 ? "yes" : "no")
                  << std::endl;
    }

    std::cout << "[acceleration] Best device: " << best.name
              << " (backend=" << static_cast<int>(best.backend_type) << ")"
              << std::endl;
}

} // namespace acceleration
} // namespace themis
