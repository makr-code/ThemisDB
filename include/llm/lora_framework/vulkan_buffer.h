/**
 * @file vulkan_buffer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/vulkan_context.h"

#if __has_include(<vulkan/vulkan.h>)
#  include <vulkan/vulkan.h>
#  define THEMIS_HAS_VULKAN_BUFFER 1
#else
#  define THEMIS_HAS_VULKAN_BUFFER 0
#endif

#include <cstddef>
#include <memory>

#if THEMIS_HAS_VULKAN_BUFFER

namespace themis {
namespace lora {
namespace vulkan {

class VulkanBuffer {
public:
    enum class Usage {
        DeviceLocal,  // Device-local buffer for computation (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        Staging,      // Staging buffer for CPU↔GPU transfers (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        Uniform       // Uniform buffer for shader parameters
    };
    
    VulkanBuffer(VulkanContext* context, VkDeviceSize size, Usage usage);
    
    ~VulkanBuffer() noexcept;
    
    // Disable copy, enable move
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;
    
    void upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    
    void download(void* data, VkDeviceSize size, VkDeviceSize offset = 0) const;
    
    /**
     * @brief Map.
     * @return Pointer to the result.
     */
    void* map();
    
    /**
     * @brief Unmap.
     */
    void unmap();
    
    void copy_from(const VulkanBuffer& src, VkDeviceSize size = 0,
                   VkDeviceSize src_offset = 0, VkDeviceSize dst_offset = 0);
    
    // Getters
    VkBuffer buffer() const { return buffer_; }
    VkDeviceMemory memory() const { return memory_; }
    VkDeviceSize size() const { return size_; }
    Usage usage() const { return usage_; }
    bool is_mapped() const { return mapped_ptr_ != nullptr; }
    
private:
    /**
     * @brief Create buffer.
     * @return True when the operation succeeds.
     */
    bool create_buffer();
    
    /**
     * @brief Get usage flags.
     * @return Return value.
     */
    VkBufferUsageFlags get_usage_flags() const;
    
    /**
     * @brief Get memory properties.
     * @return Return value.
     */
    VkMemoryPropertyFlags get_memory_properties() const;
    
    VulkanContext* context_;
    VkBuffer buffer_ = VK_NULL_HANDLE;
    VkDeviceMemory memory_ = VK_NULL_HANDLE;
    VkDeviceSize size_;
    Usage usage_;
    void* mapped_ptr_ = nullptr;
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#else // !THEMIS_HAS_VULKAN_BUFFER

// Stub implementation when Vulkan is not available
namespace themis {
namespace lora {
namespace vulkan {

class VulkanBuffer {
public:
    enum class Usage { DeviceLocal, Staging, Uniform };
    
    VulkanBuffer(VulkanContext*, size_t, Usage) {}
    ~VulkanBuffer() = default;
    
    /**
     * @brief Initialize.
     * @return True when the operation succeeds.
     * @details Implements initialize without additional internal calls.
     */
    bool initialize() { return false; }
    /**
     * @brief Cleanup.
     * @details Implements cleanup without additional internal calls.
     */
    void cleanup() {}
    /**
     * @brief Upload.
     * @param[in] param Input parameter.
     * @param[in] size_t Input parameter.
     * @return True when the operation succeeds.
     * @details Implements upload without additional internal calls.
     */
    bool upload(const void*, size_t) { return false; }
    /**
     * @brief Download.
     * @param[in,out] param Input/output parameter.
     * @param[in] size_t Input parameter.
     * @return True when the operation succeeds.
     * @details Implements download without additional internal calls.
     */
    bool download(void*, size_t) { return false; }
    /**
     * @brief Map.
     * @return Pointer to the result.
     * @details Implements map without additional internal calls.
     */
    void* map() { return nullptr; }
    /**
     * @brief Unmap.
     * @details Implements unmap without additional internal calls.
     */
    void unmap() {}
    bool is_mapped() const { return false; }
    size_t size() const { return 0; }
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_BUFFER
