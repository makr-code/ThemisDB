#pragma once

#include "llm/lora_framework/vulkan_context.h"
#include <vulkan/vulkan.h>
#include <cstddef>
#include <memory>

namespace themis {
namespace lora {
namespace vulkan {

/**
 * @brief Vulkan buffer for GPU memory management
 * 
 * Manages device-local buffers for computation and staging buffers
 * for CPU↔GPU data transfers.
 */
class VulkanBuffer {
public:
    /**
     * @brief Buffer usage types
     */
    enum class Usage {
        DeviceLocal,  // Device-local buffer for computation (VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT)
        Staging,      // Staging buffer for CPU↔GPU transfers (VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        Uniform       // Uniform buffer for shader parameters
    };
    
    /**
     * @brief Create buffer with specified size and usage
     * @param context Vulkan context
     * @param size Buffer size in bytes
     * @param usage Buffer usage type
     */
    VulkanBuffer(VulkanContext* context, VkDeviceSize size, Usage usage);
    
    ~VulkanBuffer();
    
    // Disable copy, enable move
    VulkanBuffer(const VulkanBuffer&) = delete;
    VulkanBuffer& operator=(const VulkanBuffer&) = delete;
    VulkanBuffer(VulkanBuffer&& other) noexcept;
    VulkanBuffer& operator=(VulkanBuffer&& other) noexcept;
    
    /**
     * @brief Upload data to buffer
     * @param data Source data pointer
     * @param size Data size in bytes
     * @param offset Offset in buffer (default 0)
     */
    void upload(const void* data, VkDeviceSize size, VkDeviceSize offset = 0);
    
    /**
     * @brief Download data from buffer
     * @param data Destination data pointer
     * @param size Data size in bytes
     * @param offset Offset in buffer (default 0)
     */
    void download(void* data, VkDeviceSize size, VkDeviceSize offset = 0) const;
    
    /**
     * @brief Map buffer memory for CPU access
     * @return Mapped memory pointer
     */
    void* map();
    
    /**
     * @brief Unmap buffer memory
     */
    void unmap();
    
    /**
     * @brief Copy data from another buffer
     * @param src Source buffer
     * @param size Size to copy (0 = entire buffer)
     * @param src_offset Source offset
     * @param dst_offset Destination offset
     */
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
     * @brief Create the buffer and allocate memory
     */
    bool create_buffer();
    
    /**
     * @brief Get Vulkan buffer usage flags for our Usage enum
     */
    VkBufferUsageFlags get_usage_flags() const;
    
    /**
     * @brief Get Vulkan memory property flags for our Usage enum
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
