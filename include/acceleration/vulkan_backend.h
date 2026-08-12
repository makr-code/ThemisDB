/**
 * @file vulkan_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// VulkanBackend — public alias for the Vulkan compute shader pipeline backend.
// Tests and callers use `VulkanBackend`; the class is implemented as
// `VulkanVectorBackend` inside graphics_backends.h / graphics_backends.cpp.

#include "acceleration/graphics_backends.h"
#include "acceleration/compute_backend.h"
#include <cstdint>
#include <vector>

namespace themis {
namespace acceleration {

// Alias so that `#include "acceleration/vulkan_backend.h"` gives a
// `VulkanBackend` type that is identical to `VulkanVectorBackend`.
using VulkanBackend = VulkanVectorBackend;

// =============================================================================
// IVulkanComputeBackend
// =============================================================================

/**
 * @brief Opaque Vulkan pipeline handle.
 *
 * No raw `VkPipeline` is exposed in this header; the backend implementation
 * maps @p id to an internal `VkPipeline` handle.
 */
struct VulkanPipelineHandle {
    uint64_t id = 0;  ///< Opaque backend-assigned pipeline identifier

    /// Returns true when this handle refers to a valid, created pipeline.
    bool valid() const noexcept { return id != 0; }

    bool operator==(const VulkanPipelineHandle& o) const noexcept { return id == o.id; }
    bool operator!=(const VulkanPipelineHandle& o) const noexcept { return id != o.id; }
};

/**
 * @brief Plain-data pipeline configuration for a Vulkan compute shader.
 *
 * This struct does **not** contain any Vulkan types; the backend translates
 * it to `VkComputePipelineCreateInfo` internally.  Safe to include in any
 * translation unit without the Vulkan SDK.
 */
struct VulkanPipelineConfig {
    uint32_t local_size_x    = 64;    ///< Workgroup local X dimension (threads per block)
    uint32_t local_size_y    = 1;     ///< Workgroup local Y dimension
    uint32_t local_size_z    = 1;     ///< Workgroup local Z dimension
    uint32_t push_const_size = 0;     ///< Push constant block size in bytes (0 = none)
    bool     enable_fp16     = false; ///< Request VK_KHR_shader_float16_int8 (FP16)
    bool     enable_int8     = false; ///< Request 8-bit integer arithmetic
};

/**
 * @brief POD device info queryable **without** a live Vulkan instance.
 *
 * Populated by `IVulkanComputeBackend::enumerateDevices()` via a static
 * enumeration pass at startup.  Safe to read at any time after the first
 * call to `enumerateDevices()`.
 */
struct VulkanDeviceInfo {
    uint32_t device_index      = 0;         ///< Vulkan physical device index
    uint32_t vendor_id         = 0;         ///< PCI vendor ID (see vendor_id::*)
    uint32_t device_id         = 0;         ///< PCI device ID
    char     device_name[256]  = {};        ///< Human-readable name (null-terminated)
    uint64_t vram_bytes        = 0;         ///< Dedicated GPU VRAM in bytes
    bool     supports_fp16     = false;     ///< VK_KHR_shader_float16_int8 available
    bool     supports_int8     = false;     ///< 8-bit integer arithmetic available
    bool     is_discrete       = false;     ///< Discrete GPU (dGPU) vs. integrated
};

/**
 * @brief Vulkan-specific compute backend interface.
 *
 * Extends `IVectorBackend` with compute pipeline lifecycle management for
 * SPIR-V shaders.  Intended for backends that operate via Vulkan Compute
 * (cross-platform: NVIDIA, AMD, Intel, Apple, Qualcomm).
 *
 * ## Compile-time guard
 * This interface is nested within `THEMIS_ENABLE_VULKAN` (which in turn
 * implies `THEMIS_ENABLE_GPU`).  Source files that include this header
 * without defining `THEMIS_ENABLE_VULKAN` still get the type alias
 * `VulkanBackend` and the POD structs above; only the class body is guarded.
 *
 * ## Thread safety
 * `enumerateDevices()` is safe to call concurrently.  `createPipeline()` and
 * `destroyPipeline()` require the caller to ensure that no other thread is
 * using the same pipeline handle simultaneously.
 */
#ifdef THEMIS_ENABLE_VULKAN
class IVulkanComputeBackend : public IVectorBackend {
public:
    virtual ~IVulkanComputeBackend() = default;

    /**
     * @brief Create or retrieve a cached compute pipeline from a SPIR-V blob.
     *
     * @param spirv       Pointer to SPIR-V bytecode; must be 4-byte aligned
     *                    and have a valid SPIR-V magic number.
     * @param spirv_size  Size in bytes of the SPIR-V blob.
     * @param config      Pipeline configuration (workgroup dimensions, etc.).
     * @return Opaque handle; `handle.valid() == false` on failure.
     */
    [[nodiscard]] virtual VulkanPipelineHandle createPipeline(
            const uint32_t*            spirv,
            size_t                     spirv_size,
            const VulkanPipelineConfig& config) = 0;

    /**
     * @brief Destroy a pipeline created by `createPipeline()`.
     *
     * No-op if the handle is already invalid or was previously destroyed.
     */
    virtual void destroyPipeline(VulkanPipelineHandle handle) noexcept = 0;

    /**
     * @brief Enumerate Vulkan-capable devices without requiring a live
     *        `VkInstance`.
     *
     * May be called before `initialize()` to query hardware for device
     * selection purposes.  Implementations should use
     * `vkEnumeratePhysicalDevices` on first call and cache results.
     *
     * @return One `VulkanDeviceInfo` entry per discovered physical device.
     */
    [[nodiscard]] virtual std::vector<VulkanDeviceInfo> enumerateDevices() const = 0;

    /**
     * @brief Returns true when a Vulkan 1.1+ instance with compute support
     *        is available on this system.
     *
     * Thread-safe; may be called at any time.
     */
    [[nodiscard]] virtual bool isVulkanAvailable() const noexcept = 0;
};
#endif // THEMIS_ENABLE_VULKAN

} // namespace acceleration
} // namespace themis
