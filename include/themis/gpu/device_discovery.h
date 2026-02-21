/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            device_discovery.h                                 ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 10:58:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     121                                            ║
    • Open Issues:     TODOs: 1, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 818af750c  2026-02-20  feat: Erweiterung der Metriken und Verbesserung der Valid... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include "themis/edition.h"

namespace themis {
namespace gpu {

/**
 * @brief Capabilities and identity of a single GPU device.
 *
 * Populated by DeviceDiscovery::Enumerate().  On systems without a real
 * CUDA/ROCm runtime the fields reflect the compile-time edition limits so
 * that the rest of the code can always query device information without
 * conditional compilation.
 */
struct DeviceInfo {
    int         index          = 0;      ///< Driver-assigned device index (0-based)
    int         device_index   = 0;      ///< Backward-compatible alias for index
    std::string name;                    ///< Human-readable device name
    std::string backend;                 ///< "CUDA", "ROCm", "Vulkan", "CPU_FALLBACK"
    uint64_t    total_vram_bytes  = 0;   ///< Total VRAM reported by driver
    uint64_t    free_vram_bytes   = 0;   ///< Available VRAM at discovery time
    int         compute_major     = 0;   ///< Compute capability major (CUDA) / wave size (ROCm)
    int         compute_minor     = 0;   ///< Compute capability minor
    bool        is_healthy        = true;///< false if the device reported an error
    std::string error_message;           ///< Non-empty when is_healthy == false
};

/**
 * @brief GPU device discovery.
 *
 * Enumerates physical GPU devices available to the process.  When no real
 * GPU runtime is present (or when the edition forbids GPU use) an empty list
 * or a single CPU-fallback sentinel is returned so that callers never have
 * to null-check.
 *
 * Design notes
 * ------------
 * - All methods are static; there is no device handle or runtime state kept
 *   inside this class.
 * - Real CUDA/ROCm integration is a future TODO guarded by
 *   THEMIS_ENABLE_CUDA / THEMIS_ENABLE_HIP compile-time defines.
 * - Currently returns a CPU-fallback sentinel device when no GPU runtime is
 *   present, which lets the memory manager and safe-fail logic operate
 *   without hardware.
 */
class DeviceDiscovery {
public:
    /**
     * @brief Enumerate all available GPU devices.
     *
     * @return List of discovered devices.  Empty only when the edition has
     *         disabled GPU and no CPU-fallback sentinel is appropriate.
     *         On CI / no-GPU machines returns a single CPU_FALLBACK entry.
     */
    static std::vector<DeviceInfo> Enumerate();

    /**
     * @brief Return the device best suited for a new allocation.
     *
     * Currently returns the device with the most free VRAM.  Returns the
     * CPU-fallback sentinel when no GPU is available.
     *
     * @param devices  List produced by Enumerate() (avoids re-enumeration).
     */
    static DeviceInfo GetBestDevice(const std::vector<DeviceInfo>& devices);

    /**
     * @brief Convenience overload: enumerate then pick best.
     */
    static DeviceInfo GetBestDevice();

    /**
     * @brief Return every device that reported is_healthy == true.
     */
    static std::vector<DeviceInfo> GetHealthyDevices(
        const std::vector<DeviceInfo>& devices);

    /**
     * @brief True when at least one real (non-CPU-fallback) device is present
     *        and healthy.
     */
    static bool HasGPU(const std::vector<DeviceInfo>& devices);

    /**
     * @brief Convenience overload: enumerate then check.
     */
    static bool HasGPU();
};

} // namespace gpu
} // namespace themis
