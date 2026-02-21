/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            device_discovery.cpp                               ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-02-21 16:34:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     229                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • b8b369411  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 8efb1d2fe  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31ccce9fb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ea0163e87  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 171dcc258  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/*
 * GPU Device Discovery
 * ====================
 * Enumerates physical GPU devices.  Real CUDA/ROCm calls are gated behind
 * THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP.  When neither is available a
 * single CPU_FALLBACK sentinel is returned so that the rest of the code
 * never has to null-check.
 */

#include "themis/gpu/device_discovery.h"
#include "themis/edition.h"

#include <algorithm>

#ifdef THEMIS_ENABLE_CUDA
#  include <cuda_runtime.h>
#endif

#ifdef THEMIS_ENABLE_HIP
#  include <hip/hip_runtime.h>
#endif

namespace themis {
namespace gpu {

namespace {

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

DeviceInfo MakeCPUFallback() {
    DeviceInfo d;
    d.index            = -1;
    d.device_index     = -1;
    d.name             = "CPU Fallback";
    d.backend          = "CPU_FALLBACK";
    d.total_vram_bytes = edition::GPU_MAX_VRAM_GB > 0
                             ? static_cast<uint64_t>(edition::GPU_MAX_VRAM_GB) * 1024ULL * 1024ULL * 1024ULL
                             : 0;
    d.free_vram_bytes  = d.total_vram_bytes;
    d.compute_major    = 0;
    d.compute_minor    = 0;
    d.is_healthy       = true;
    return d;
}

#ifdef THEMIS_ENABLE_CUDA
std::vector<DeviceInfo> EnumerateCUDA() {
    std::vector<DeviceInfo> result;
    int count = 0;
    if (cudaGetDeviceCount(&count) != cudaSuccess || count == 0) {
        return result;
    }
    for (int i = 0; i < count; ++i) {
        cudaDeviceProp props{};
        DeviceInfo d;
        d.index   = i;
        d.device_index = i;
        d.backend = "CUDA";
        if (cudaGetDeviceProperties(&props, i) != cudaSuccess) {
            d.is_healthy    = false;
            d.error_message = "cudaGetDeviceProperties failed for device " + std::to_string(i);
            result.push_back(d);
            continue;
        }
        d.name          = props.name;
        d.total_vram_bytes = static_cast<uint64_t>(props.totalGlobalMem);
        d.compute_major = props.major;
        d.compute_minor = props.minor;
        // Query free memory.
        size_t free_mem = 0, total_mem = 0;
        cudaSetDevice(i);
        if (cudaMemGetInfo(&free_mem, &total_mem) == cudaSuccess) {
            d.free_vram_bytes = free_mem;
        } else {
            d.free_vram_bytes = d.total_vram_bytes;  // best-effort fallback
        }
        d.is_healthy = true;
        result.push_back(d);
    }
    return result;
}
#endif  // THEMIS_ENABLE_CUDA

#ifdef THEMIS_ENABLE_HIP
std::vector<DeviceInfo> EnumerateROCm() {
    std::vector<DeviceInfo> result;
    int count = 0;
    if (hipGetDeviceCount(&count) != hipSuccess || count == 0) {
        return result;
    }
    for (int i = 0; i < count; ++i) {
        hipDeviceProp_t props{};
        DeviceInfo d;
        d.index   = i;
        d.device_index = i;
        d.backend = "ROCm";
        if (hipGetDeviceProperties(&props, i) != hipSuccess) {
            d.is_healthy    = false;
            d.error_message = "hipGetDeviceProperties failed for device " + std::to_string(i);
            result.push_back(d);
            continue;
        }
        d.name             = props.name;
        d.total_vram_bytes = static_cast<uint64_t>(props.totalGlobalMem);
        d.compute_major    = props.major;
        d.compute_minor    = props.minor;
        size_t free_mem = 0, total_mem = 0;
        hipSetDevice(i);
        if (hipMemGetInfo(&free_mem, &total_mem) == hipSuccess) {
            d.free_vram_bytes = free_mem;
        } else {
            d.free_vram_bytes = d.total_vram_bytes;
        }
        d.is_healthy = true;
        result.push_back(d);
    }
    return result;
}
#endif  // THEMIS_ENABLE_HIP

}  // namespace

// ============================================================================
// DeviceDiscovery — static method implementations
// ============================================================================

std::vector<DeviceInfo> DeviceDiscovery::Enumerate() {
    std::vector<DeviceInfo> devices;

#ifdef THEMIS_ENABLE_CUDA
    devices = EnumerateCUDA();
#elif defined(THEMIS_ENABLE_HIP)
    devices = EnumerateROCm();
#endif

    // If no real GPU was found, provide the CPU-fallback sentinel so that
    // callers (memory manager, safe-fail) always get a non-empty list.
    if (devices.empty()) {
        devices.push_back(MakeCPUFallback());
    }

    return devices;
}

DeviceInfo DeviceDiscovery::GetBestDevice(const std::vector<DeviceInfo>& devices) {
    if (devices.empty()) {
        return MakeCPUFallback();
    }

    // Prefer healthy GPU devices; fall back to CPU sentinel if none.
    const DeviceInfo* best = nullptr;
    for (const auto& d : devices) {
        if (!d.is_healthy) continue;
        if (d.backend == "CPU_FALLBACK") continue;
        if (best == nullptr || d.free_vram_bytes > best->free_vram_bytes) {
            best = &d;
        }
    }

    if (best != nullptr) {
        return *best;
    }

    // All GPUs unhealthy or only CPU fallback present — return the sentinel.
    for (const auto& d : devices) {
        if (d.backend == "CPU_FALLBACK") return d;
    }

    return MakeCPUFallback();
}

DeviceInfo DeviceDiscovery::GetBestDevice() {
    return GetBestDevice(Enumerate());
}

std::vector<DeviceInfo> DeviceDiscovery::GetHealthyDevices(
    const std::vector<DeviceInfo>& devices) {
    std::vector<DeviceInfo> result;
    for (const auto& d : devices) {
        if (d.is_healthy) {
            result.push_back(d);
        }
    }
    return result;
}

bool DeviceDiscovery::HasGPU(const std::vector<DeviceInfo>& devices) {
    for (const auto& d : devices) {
        if (d.is_healthy && d.backend != "CPU_FALLBACK") {
            return true;
        }
    }
    return false;
}

bool DeviceDiscovery::HasGPU() {
    return HasGPU(Enumerate());
}

} // namespace gpu
} // namespace themis
