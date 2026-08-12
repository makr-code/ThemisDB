/**
 * @file vulkan_buffer.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/lora_framework/vulkan_buffer.h"
#include <stdexcept>
#include <cstring>
#include <algorithm>

#if THEMIS_HAS_VULKAN_HEADER

namespace themis {
namespace lora {
namespace vulkan {

VulkanBuffer::VulkanBuffer(VulkanContext* context, VkDeviceSize size, Usage usage)
    : context_(context)
    , size_(size)
    , usage_(usage) {
    
    if (!context_ || !context_->is_initialized()) {
        throw std::runtime_error("VulkanContext not initialized");
    }
    
    if (!create_buffer()) {
        throw std::runtime_error("Failed to create Vulkan buffer");
    }
}

VulkanBuffer::~VulkanBuffer() {
    if (mapped_ptr_) {
        unmap();
    }
    
    if (buffer_ != VK_NULL_HANDLE) {
        vkDestroyBuffer(context_->device(), buffer_, nullptr);
    }
    
    if (memory_ != VK_NULL_HANDLE) {
        vkFreeMemory(context_->device(), memory_, nullptr);
    }
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : context_(other.context_)
    , buffer_(other.buffer_)
    , memory_(other.memory_)
    , size_(other.size_)
    , usage_(other.usage_)
    , mapped_ptr_(other.mapped_ptr_) {
    
    other.buffer_ = VK_NULL_HANDLE;
    other.memory_ = VK_NULL_HANDLE;
    other.mapped_ptr_ = nullptr;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    if (this != &other) {
        // Cleanup current resources
        if (mapped_ptr_) {
            unmap();
        }
        if (buffer_ != VK_NULL_HANDLE) {
            vkDestroyBuffer(context_->device(), buffer_, nullptr);
        }
        if (memory_ != VK_NULL_HANDLE) {
            vkFreeMemory(context_->device(), memory_, nullptr);
        }
        
        // Move from other
        context_ = other.context_;
        buffer_ = other.buffer_;
        memory_ = other.memory_;
        size_ = other.size_;
        usage_ = other.usage_;
        mapped_ptr_ = other.mapped_ptr_;
        
        other.buffer_ = VK_NULL_HANDLE;
        other.memory_ = VK_NULL_HANDLE;
        other.mapped_ptr_ = nullptr;
    }
    return *this;
}

bool VulkanBuffer::create_buffer() {
    // Create buffer
    VkBufferCreateInfo buffer_info = {};
    buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    buffer_info.size = size_;
    buffer_info.usage = get_usage_flags();
    buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VkResult result = vkCreateBuffer(context_->device(), &buffer_info, nullptr, &buffer_);
    if (result != VK_SUCCESS) {
        return false;
    }
    
    // Get memory requirements
    VkMemoryRequirements mem_requirements = {};
    vkGetBufferMemoryRequirements(context_->device(), buffer_, &mem_requirements);
    
    // Allocate memory
    VkMemoryAllocateInfo alloc_info = {};
    alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc_info.allocationSize = mem_requirements.size;
    
    int32_t memory_type = context_->find_memory_type(
        mem_requirements.memoryTypeBits,
        get_memory_properties()
    );
    
    if (memory_type < 0) {
        vkDestroyBuffer(context_->device(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        return false;
    }
    
    alloc_info.memoryTypeIndex = static_cast<uint32_t>(memory_type);
    
    result = vkAllocateMemory(context_->device(), &alloc_info, nullptr, &memory_);
    if (result != VK_SUCCESS) {
        vkDestroyBuffer(context_->device(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        return false;
    }
    
    // Bind buffer to memory
    result = vkBindBufferMemory(context_->device(), buffer_, memory_, 0);
    if (result != VK_SUCCESS) {
        vkFreeMemory(context_->device(), memory_, nullptr);
        memory_ = VK_NULL_HANDLE;
        vkDestroyBuffer(context_->device(), buffer_, nullptr);
        buffer_ = VK_NULL_HANDLE;
        return false;
    }
    
    return true;
}

VkBufferUsageFlags VulkanBuffer::get_usage_flags() const {
    VkBufferUsageFlags flags = 0;
    
    switch (usage_) {
        case Usage::DeviceLocal:
            flags = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT |
                    VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            break;
        case Usage::Staging:
            flags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
        case Usage::Uniform:
            flags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT |
                    VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            break;
    }
    
    return flags;
}

VkMemoryPropertyFlags VulkanBuffer::get_memory_properties() const {
    switch (usage_) {
        case Usage::DeviceLocal:
            return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        case Usage::Staging:
        case Usage::Uniform:
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                   VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        default:
            return 0;
    }
}

void VulkanBuffer::upload(const void* data, VkDeviceSize size, VkDeviceSize offset) {
    if (offset + size > size_) {
        throw std::runtime_error("Upload size exceeds buffer size");
    }
    
    if (usage_ == Usage::DeviceLocal) {
        // For device-local buffers, use staging buffer
        VulkanBuffer staging(context_, size, Usage::Staging);
        staging.upload(data, size, 0);
        copy_from(staging, size, 0, offset);
    } else {
        // For host-visible buffers, map and copy directly
        void* mapped = map();
        std::memcpy(static_cast<char*>(mapped) + offset, data, size);
        unmap();
    }
}

void VulkanBuffer::download(void* data, VkDeviceSize size, VkDeviceSize offset) const {
    if (offset + size > size_) {
        throw std::runtime_error("Download size exceeds buffer size");
    }
    
    if (usage_ == Usage::DeviceLocal) {
        // For device-local buffers, use staging buffer
        VulkanBuffer staging(context_, size, Usage::Staging);
        staging.copy_from(*this, size, offset, 0);
        staging.download(data, size, 0);
    } else {
        // For host-visible buffers, map and copy directly
        void* mapped_memory = nullptr;
        VkResult map_result = vkMapMemory(context_->device(), memory_, offset, size, 0, &mapped_memory);
        if (map_result != VK_SUCCESS) {
            throw std::runtime_error("Failed to map buffer memory for download");
        }
        std::memcpy(data, mapped_memory, size);
        vkUnmapMemory(context_->device(), memory_);
    }
}

void* VulkanBuffer::map() {
    if (mapped_ptr_) {
        return mapped_ptr_;
    }
    
    if (usage_ == Usage::DeviceLocal) {
        throw std::runtime_error("Cannot map device-local buffer");
    }
    
    VkResult result = vkMapMemory(context_->device(), memory_, 0, size_, 0, &mapped_ptr_);
    if (result != VK_SUCCESS) {
        throw std::runtime_error("Failed to map buffer memory");
    }
    
    return mapped_ptr_;
}

void VulkanBuffer::unmap() {
    if (!mapped_ptr_) {
        return;
    }
    
    vkUnmapMemory(context_->device(), memory_);
    mapped_ptr_ = nullptr;
}

void VulkanBuffer::copy_from(const VulkanBuffer& src, VkDeviceSize size,
                              VkDeviceSize src_offset, VkDeviceSize dst_offset) {
    if (size == 0) {
        size = std::min(src.size_ - src_offset, size_ - dst_offset);
    }
    
    // Create command buffer for copy operation
    VkCommandBuffer cmd_buffer = context_->allocate_command_buffer();
    
    VkCommandBufferBeginInfo begin_info = {};
    begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    
    VkResult begin_result = vkBeginCommandBuffer(cmd_buffer, &begin_info);
    if (begin_result != VK_SUCCESS) {
        context_->free_command_buffer(cmd_buffer);
        throw std::runtime_error("Failed to begin command buffer for buffer copy");
    }
    
    VkBufferCopy copy_region = {};
    copy_region.srcOffset = src_offset;
    copy_region.dstOffset = dst_offset;
    copy_region.size = size;
    
    vkCmdCopyBuffer(cmd_buffer, src.buffer_, buffer_, 1, &copy_region);
    
    VkResult end_result = vkEndCommandBuffer(cmd_buffer);
    if (end_result != VK_SUCCESS) {
        context_->free_command_buffer(cmd_buffer);
        throw std::runtime_error("Failed to end command buffer for buffer copy");
    }
    
    // Submit command buffer
    VkSubmitInfo submit_info = {};
    submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;
    
    VkFence fence = context_->create_fence(false);
    VkResult submit_result = vkQueueSubmit(context_->compute_queue(), 1, &submit_info, fence);
    if (submit_result != VK_SUCCESS) {
        context_->destroy_fence(fence);
        context_->free_command_buffer(cmd_buffer);
        throw std::runtime_error("Failed to submit buffer copy command");
    }

    if (!context_->wait_for_fence(fence)) {
        context_->destroy_fence(fence);
        context_->free_command_buffer(cmd_buffer);
        throw std::runtime_error("Failed while waiting for buffer copy completion");
    }
    
    context_->destroy_fence(fence);
    context_->free_command_buffer(cmd_buffer);
}

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_HEADER
