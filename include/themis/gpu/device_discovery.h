/**
 * @file device_discovery.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

    // MIG (Multi-Instance GPU) fields — populated for NVIDIA A/H series devices.
    bool        mig_enabled          = false; ///< true when MIG mode is active on this device
    int         mig_max_instances    = 0;     ///< Maximum MIG instances supported (0 = MIG not supported)
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
