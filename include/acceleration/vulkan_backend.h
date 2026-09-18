/**
 * @file vulkan_backend.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct VulkanPipelineHandle {
    uint64_t id = 0;  ///< Opaque backend-assigned pipeline identifier

    bool valid() const noexcept { return id != 0; }

    bool operator==(const VulkanPipelineHandle& o) const noexcept { return id == o.id; }
    bool operator!=(const VulkanPipelineHandle& o) const noexcept { return id != o.id; }
};

struct VulkanPipelineConfig {
    uint32_t local_size_x    = 64;    ///< Workgroup local X dimension (threads per block)
    uint32_t local_size_y    = 1;     ///< Workgroup local Y dimension
    uint32_t local_size_z    = 1;     ///< Workgroup local Z dimension
    uint32_t push_const_size = 0;     ///< Push constant block size in bytes (0 = none)
    bool     enable_fp16     = false; ///< Request VK_KHR_shader_float16_int8 (FP16)
    bool     enable_int8     = false; ///< Request 8-bit integer arithmetic
};

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

#ifdef THEMIS_ENABLE_VULKAN
class IVulkanComputeBackend : public IVectorBackend {
public:
    /**
     * @brief IVulkan Compute Backend.
     * @return Return value.
     */
    virtual ~IVulkanComputeBackend() = default;

    [[nodiscard]] virtual VulkanPipelineHandle createPipeline(
            const uint32_t*            spirv,
            size_t                     spirv_size,
            const VulkanPipelineConfig& config) = 0;

    /**
     * @brief Destroy Pipeline.
     * @param[in] handle Input parameter.
     * @note Exception safety: noexcept.
     */
    virtual void destroyPipeline(VulkanPipelineHandle handle) noexcept = 0;

    [[nodiscard]] virtual std::vector<VulkanDeviceInfo> enumerateDevices() const = 0;

    [[nodiscard]] virtual bool isVulkanAvailable() const noexcept = 0;
};
#endif // THEMIS_ENABLE_VULKAN

} // namespace acceleration
} // namespace themis
