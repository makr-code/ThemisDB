/**
 * @file vulkan_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// RAII wrappers for Vulkan resources
// Provides automatic resource cleanup and exception safety
// Header-only implementation for ease of use

#ifdef THEMIS_ENABLE_VULKAN
#include <vulkan/vulkan.h>
#include <functional>
#include <stdexcept>
#include <string>
#include <utility>

namespace themis {
namespace acceleration {
namespace raii {

// ============================================================================
// Generic Vulkan handle RAII wrapper (device-owned resources)
// ============================================================================

template <typename T, typename Deleter>
class VulkanHandle {
public:
    VulkanHandle() = default;

    VulkanHandle(VkDevice device, T handle, Deleter deleter)
        : device_(device), handle_(handle), deleter_(std::move(deleter)) {}

    // Non-copyable
    VulkanHandle(const VulkanHandle&) = delete;
    VulkanHandle& operator=(const VulkanHandle&) = delete;

    VulkanHandle(VulkanHandle&& other) noexcept
        : device_(other.device_), handle_(other.handle_),
          deleter_(std::move(other.deleter_)) {
        other.handle_ = VK_NULL_HANDLE;
    }

    VulkanHandle& operator=(VulkanHandle&& other) noexcept {
        if (this != &other) {
            destroy();
            device_  = other.device_;
            handle_  = other.handle_;
            deleter_ = std::move(other.deleter_);
            other.handle_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    ~VulkanHandle() { destroy(); }

    T get() const noexcept { return handle_; }
    
    explicit operator bool() const noexcept { return handle_ != VK_NULL_HANDLE; }

    T release() noexcept {
        T tmp  = handle_;
        handle_ = VK_NULL_HANDLE;
        return tmp;
    }

private:
    /**
     * @brief Destroy.
     * @details Calls: deleter_().
     */
    void destroy() {
        if (handle_ != VK_NULL_HANDLE && device_ != VK_NULL_HANDLE) {
            deleter_(device_, handle_, nullptr);
            handle_ = VK_NULL_HANDLE;
        }
    }

    VkDevice device_  = VK_NULL_HANDLE;
    T        handle_  = VK_NULL_HANDLE;
    Deleter  deleter_{};
};

// ============================================================================
// Convenience type aliases for common resources
// ============================================================================

using VulkanBuffer              = VulkanHandle<VkBuffer,
    decltype(&vkDestroyBuffer)>;

using VulkanDeviceMemory        = VulkanHandle<VkDeviceMemory,
    decltype(&vkFreeMemory)>;

using VulkanCommandPool         = VulkanHandle<VkCommandPool,
    decltype(&vkDestroyCommandPool)>;

using VulkanDescriptorPool      = VulkanHandle<VkDescriptorPool,
    decltype(&vkDestroyDescriptorPool)>;

using VulkanDescriptorSetLayout = VulkanHandle<VkDescriptorSetLayout,
    decltype(&vkDestroyDescriptorSetLayout)>;

using VulkanPipelineLayout      = VulkanHandle<VkPipelineLayout,
    decltype(&vkDestroyPipelineLayout)>;

using VulkanPipeline            = VulkanHandle<VkPipeline,
    decltype(&vkDestroyPipeline)>;

using VulkanShaderModule        = VulkanHandle<VkShaderModule,
    decltype(&vkDestroyShaderModule)>;

using VulkanFence               = VulkanHandle<VkFence,
    decltype(&vkDestroyFence)>;

// ============================================================================
// Factory helpers
// ============================================================================

/**
 * @brief Make Buffer.
 * @param[in] device Input parameter.
 * @param[in] buffer Input parameter.
 * @return Return value.
 * @details Calls: VulkanBuffer().
 */
inline VulkanBuffer makeBuffer(VkDevice device, VkBuffer buffer) {
    return VulkanBuffer(device, buffer, &vkDestroyBuffer);
}

/**
 * @brief Make Device Memory.
 * @param[in] device Input parameter.
 * @param[in] memory Input parameter.
 * @return Return value.
 * @details Calls: VulkanDeviceMemory().
 */
inline VulkanDeviceMemory makeDeviceMemory(VkDevice device, VkDeviceMemory memory) {
    return VulkanDeviceMemory(device, memory, &vkFreeMemory);
}

/**
 * @brief Make Command Pool.
 * @param[in] device Input parameter.
 * @param[in] pool Input parameter.
 * @return Return value.
 * @details Calls: VulkanCommandPool().
 */
inline VulkanCommandPool makeCommandPool(VkDevice device, VkCommandPool pool) {
    return VulkanCommandPool(device, pool, &vkDestroyCommandPool);
}

/**
 * @brief Make Descriptor Pool.
 * @param[in] device Input parameter.
 * @param[in] pool Input parameter.
 * @return Return value.
 * @details Calls: VulkanDescriptorPool().
 */
inline VulkanDescriptorPool makeDescriptorPool(VkDevice device, VkDescriptorPool pool) {
    return VulkanDescriptorPool(device, pool, &vkDestroyDescriptorPool);
}

/**
 * @brief Make Descriptor Set Layout.
 * @param[in] device Input parameter.
 * @param[in] layout Input parameter.
 * @return Return value.
 * @details Calls: VulkanDescriptorSetLayout().
 */
inline VulkanDescriptorSetLayout makeDescriptorSetLayout(VkDevice device,
                                                          VkDescriptorSetLayout layout) {
    return VulkanDescriptorSetLayout(device, layout, &vkDestroyDescriptorSetLayout);
}

/**
 * @brief Make Pipeline Layout.
 * @param[in] device Input parameter.
 * @param[in] layout Input parameter.
 * @return Return value.
 * @details Calls: VulkanPipelineLayout().
 */
inline VulkanPipelineLayout makePipelineLayout(VkDevice device, VkPipelineLayout layout) {
    return VulkanPipelineLayout(device, layout, &vkDestroyPipelineLayout);
}

/**
 * @brief Make Pipeline.
 * @param[in] device Input parameter.
 * @param[in] pipeline Input parameter.
 * @return Return value.
 * @details Calls: VulkanPipeline().
 */
inline VulkanPipeline makePipeline(VkDevice device, VkPipeline pipeline) {
    return VulkanPipeline(device, pipeline, &vkDestroyPipeline);
}

/**
 * @brief Make Shader Module.
 * @param[in] device Input parameter.
 * @param[in] mod Input parameter.
 * @return Return value.
 * @details Calls: VulkanShaderModule().
 */
inline VulkanShaderModule makeShaderModule(VkDevice device, VkShaderModule mod) {
    return VulkanShaderModule(device, mod, &vkDestroyShaderModule);
}

/**
 * @brief Make Fence.
 * @param[in] device Input parameter.
 * @param[in] fence Input parameter.
 * @return Return value.
 * @details Calls: VulkanFence().
 */
inline VulkanFence makeFence(VkDevice device, VkFence fence) {
    return VulkanFence(device, fence, &vkDestroyFence);
}

// ============================================================================
// Instance RAII (not device-owned, uses a different destruction pattern)
// ============================================================================

class VulkanInstance {
public:
    VulkanInstance() = default;

    explicit VulkanInstance(VkInstance instance) : instance_(instance) {}

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    VulkanInstance(VulkanInstance&& other) noexcept : instance_(other.instance_) {
        other.instance_ = VK_NULL_HANDLE;
    }

    VulkanInstance& operator=(VulkanInstance&& other) noexcept {
        if (this != &other) {
            destroy();
            instance_       = other.instance_;
            other.instance_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    ~VulkanInstance() { destroy(); }

    VkInstance get() const noexcept { return instance_; }
    
    explicit operator bool() const noexcept { return instance_ != VK_NULL_HANDLE; }

    VkInstance release() noexcept {
        VkInstance tmp = instance_;
        instance_      = VK_NULL_HANDLE;
        return tmp;
    }

private:
    /**
     * @brief Destroy.
     * @details Calls: vkDestroyInstance().
     */
    void destroy() {
        if (instance_ != VK_NULL_HANDLE) {
            vkDestroyInstance(instance_, nullptr);
            instance_ = VK_NULL_HANDLE;
        }
    }

    VkInstance instance_ = VK_NULL_HANDLE;
};

// ============================================================================
// Device RAII
// ============================================================================

class VulkanDevice {
public:
    VulkanDevice() = default;

    explicit VulkanDevice(VkDevice device) : device_(device) {}

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    VulkanDevice(VulkanDevice&& other) noexcept : device_(other.device_) {
        other.device_ = VK_NULL_HANDLE;
    }

    VulkanDevice& operator=(VulkanDevice&& other) noexcept {
        if (this != &other) {
            destroy();
            device_       = other.device_;
            other.device_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    ~VulkanDevice() { destroy(); }

    VkDevice get() const noexcept { return device_; }
    
    explicit operator bool() const noexcept { return device_ != VK_NULL_HANDLE; }

    void waitIdle() const {
        if (device_ != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device_);
    }

    VkDevice release() noexcept {
        VkDevice tmp = device_;
        device_      = VK_NULL_HANDLE;
        return tmp;
    }

private:
    /**
     * @brief Destroy.
     * @details Calls: vkDeviceWaitIdle(), vkDestroyDevice().
     */
    void destroy() {
        if (device_ != VK_NULL_HANDLE) {
            vkDeviceWaitIdle(device_);
            vkDestroyDevice(device_, nullptr);
            device_ = VK_NULL_HANDLE;
        }
    }

    VkDevice device_ = VK_NULL_HANDLE;
};

// ============================================================================
// Scoped buffer mapping (host-visible memory)
// ============================================================================

class ScopedMemoryMap {
public:
    ScopedMemoryMap(VkDevice device, VkDeviceMemory memory,
                    VkDeviceSize offset, VkDeviceSize size,
                    VkMemoryMapFlags flags = 0)
        : device_(device), memory_(memory) {
        VkResult r = vkMapMemory(device_, memory_, offset, size, flags, &ptr_);
        if (r != VK_SUCCESS)
            throw std::runtime_error("vkMapMemory failed");
    }

    ~ScopedMemoryMap() {
        if (ptr_) {
            vkUnmapMemory(device_, memory_);
            ptr_ = nullptr;
        }
    }

    // Non-copyable; movable (transfers the mapping)
    ScopedMemoryMap(const ScopedMemoryMap&) = delete;
    ScopedMemoryMap& operator=(const ScopedMemoryMap&) = delete;

    ScopedMemoryMap(ScopedMemoryMap&& other) noexcept
        : device_(other.device_), memory_(other.memory_), ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    ScopedMemoryMap& operator=(ScopedMemoryMap&& other) noexcept {
        if (this != &other) {
            if (ptr_) {
              vkUnmapMemory(device_, memory_);
            }
            device_    = other.device_;
            memory_    = other.memory_;
            ptr_       = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    void* data() noexcept { return ptr_; }
    
    const void* data() const noexcept { return ptr_; }

private:
    VkDevice       device_;
    VkDeviceMemory memory_;
    void*          ptr_ = nullptr;
};

} // namespace raii
} // namespace acceleration
} // namespace themis

#endif // THEMIS_ENABLE_VULKAN
