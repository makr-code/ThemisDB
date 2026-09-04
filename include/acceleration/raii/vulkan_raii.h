/**
 * @file vulkan_raii.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.24
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/// @brief Template RAII wrapper for Vulkan device-owned resource handles.
///
/// A generic wrapper for any Vulkan resource (buffers, memory, pipelines, etc.)
/// that is destroyed via a device-scoped deleter function. Automatically calls
/// the deleter on scope exit (exception-safe RAII).
///
/// @tparam T The Vulkan handle type (e.g., VkBuffer, VkPipeline).
/// @tparam Deleter A callable type matching the signature:
///         `void(VkDevice, T, const VkAllocationCallbacks*)`
///
/// Features:
/// - Generic: works with any device-owned Vulkan resource.
/// - Move semantics: efficient transfer of handle ownership.
/// - Non-copyable: prevents accidental handle duplication.
/// - Exception-safe: handle is released even during unwinding.
/// - Custom deleters: supports any Vulkan deleter function.
///
/// Example usage:
/// ```cpp
/// VulkanBuffer buf(device, buffer, &vkDestroyBuffer);
/// // Buffer automatically destroyed on scope exit
/// ```
///
/// @note For instance-owned resources (VkInstance, VkDevice), use
///       VulkanInstance or VulkanDevice instead.
///
/// @see VulkanInstance, VulkanDevice for non-device-owned resources.
/// @see convenience aliases like VulkanBuffer, VulkanPipeline below.
template <typename T, typename Deleter>
class VulkanHandle {
public:
    /// @brief Default constructor; creates a null handle.
    VulkanHandle() = default;

    /// @brief Construct with a device, handle, and deleter function.
    /// @param device The Vulkan logical device.
    /// @param handle The Vulkan handle to manage.
    /// @param deleter The deleter function to call on destruction.
    VulkanHandle(VkDevice device, T handle, Deleter deleter)
        : device_(device), handle_(handle), deleter_(std::move(deleter)) {}

    // Non-copyable
    VulkanHandle(const VulkanHandle&) = delete;
    VulkanHandle& operator=(const VulkanHandle&) = delete;

    /// @brief Move constructor; transfers handle ownership.
    VulkanHandle(VulkanHandle&& other) noexcept
        : device_(other.device_), handle_(other.handle_),
          deleter_(std::move(other.deleter_)) {
        other.handle_ = VK_NULL_HANDLE;
    }

    /// @brief Move assignment; transfers handle ownership.
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

    /// @brief Destructor; calls the deleter to destroy the handle.
    ~VulkanHandle() { destroy(); }

    /// @brief Get the underlying Vulkan handle.
    /// @return The wrapped Vulkan handle; VK_NULL_HANDLE if not initialized.
    T get() const noexcept { return handle_; }
    
    /// @brief Check if the handle is valid (non-null).
    /// @return true if handle is VK_NULL_HANDLE, false otherwise.
    explicit operator bool() const noexcept { return handle_ != VK_NULL_HANDLE; }

    /// @brief Release ownership without destroying.
    /// @return The Vulkan handle.
    /// @note After calling release(), the caller is responsible for destroying the handle.
    T release() noexcept {
        T tmp  = handle_;
        handle_ = VK_NULL_HANDLE;
        return tmp;
    }

private:
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

/// @brief RAII wrapper for VkBuffer (device-owned).
using VulkanBuffer              = VulkanHandle<VkBuffer,
    decltype(&vkDestroyBuffer)>;

/// @brief RAII wrapper for VkDeviceMemory (device-owned).
using VulkanDeviceMemory        = VulkanHandle<VkDeviceMemory,
    decltype(&vkFreeMemory)>;

/// @brief RAII wrapper for VkCommandPool (device-owned).
using VulkanCommandPool         = VulkanHandle<VkCommandPool,
    decltype(&vkDestroyCommandPool)>;

/// @brief RAII wrapper for VkDescriptorPool (device-owned).
using VulkanDescriptorPool      = VulkanHandle<VkDescriptorPool,
    decltype(&vkDestroyDescriptorPool)>;

/// @brief RAII wrapper for VkDescriptorSetLayout (device-owned).
using VulkanDescriptorSetLayout = VulkanHandle<VkDescriptorSetLayout,
    decltype(&vkDestroyDescriptorSetLayout)>;

/// @brief RAII wrapper for VkPipelineLayout (device-owned).
using VulkanPipelineLayout      = VulkanHandle<VkPipelineLayout,
    decltype(&vkDestroyPipelineLayout)>;

/// @brief RAII wrapper for VkPipeline (device-owned).
using VulkanPipeline            = VulkanHandle<VkPipeline,
    decltype(&vkDestroyPipeline)>;

/// @brief RAII wrapper for VkShaderModule (device-owned).
using VulkanShaderModule        = VulkanHandle<VkShaderModule,
    decltype(&vkDestroyShaderModule)>;

/// @brief RAII wrapper for VkFence (device-owned).
using VulkanFence               = VulkanHandle<VkFence,
    decltype(&vkDestroyFence)>;

// ============================================================================
// Factory helpers
// ============================================================================

/// @brief Create a VulkanBuffer wrapper from a raw VkBuffer handle.
/// @param device The Vulkan logical device.
/// @param buffer The VkBuffer handle to wrap.
/// @return A VulkanBuffer RAII wrapper.
inline VulkanBuffer makeBuffer(VkDevice device, VkBuffer buffer) {
    return VulkanBuffer(device, buffer, &vkDestroyBuffer);
}

/// @brief Create a VulkanDeviceMemory wrapper from a raw VkDeviceMemory handle.
/// @param device The Vulkan logical device.
/// @param memory The VkDeviceMemory handle to wrap.
/// @return A VulkanDeviceMemory RAII wrapper.
inline VulkanDeviceMemory makeDeviceMemory(VkDevice device, VkDeviceMemory memory) {
    return VulkanDeviceMemory(device, memory, &vkFreeMemory);
}

inline VulkanCommandPool makeCommandPool(VkDevice device, VkCommandPool pool) {
    return VulkanCommandPool(device, pool, &vkDestroyCommandPool);
}

inline VulkanDescriptorPool makeDescriptorPool(VkDevice device, VkDescriptorPool pool) {
    return VulkanDescriptorPool(device, pool, &vkDestroyDescriptorPool);
}

inline VulkanDescriptorSetLayout makeDescriptorSetLayout(VkDevice device,
                                                          VkDescriptorSetLayout layout) {
    return VulkanDescriptorSetLayout(device, layout, &vkDestroyDescriptorSetLayout);
}

inline VulkanPipelineLayout makePipelineLayout(VkDevice device, VkPipelineLayout layout) {
    return VulkanPipelineLayout(device, layout, &vkDestroyPipelineLayout);
}

inline VulkanPipeline makePipeline(VkDevice device, VkPipeline pipeline) {
    return VulkanPipeline(device, pipeline, &vkDestroyPipeline);
}

inline VulkanShaderModule makeShaderModule(VkDevice device, VkShaderModule mod) {
    return VulkanShaderModule(device, mod, &vkDestroyShaderModule);
}

inline VulkanFence makeFence(VkDevice device, VkFence fence) {
    return VulkanFence(device, fence, &vkDestroyFence);
}

// ============================================================================
// Instance RAII (not device-owned, uses a different destruction pattern)
// ============================================================================

/// @brief RAII wrapper for VkInstance (instance-owned, not device-owned).
///
/// Manages the lifetime of a Vulkan instance. Unlike VulkanHandle, instances
/// are destroyed via vkDestroyInstance() rather than a device-scoped deleter.
/// Automatically destroys the instance on scope exit (exception-safe RAII).
///
/// Features:
/// - Move semantics: efficient transfer of instance ownership.
/// - Non-copyable: prevents accidental instance duplication.
/// - Exception-safe: instance is destroyed even during unwinding.
///
/// Example usage:
/// ```cpp
/// VkInstance raw_inst = ...;
/// VulkanInstance inst(raw_inst);
/// // Instance automatically destroyed on scope exit
/// ```
///
/// @see VulkanDevice for logical device RAII wrapper.
/// @see VulkanHandle for device-owned resources.
class VulkanInstance {
public:
    /// @brief Default constructor; does not own an instance.
    VulkanInstance() = default;

    /// @brief Construct with a Vulkan instance handle.
    /// @param instance The VkInstance to manage.
    explicit VulkanInstance(VkInstance instance) : instance_(instance) {}

    VulkanInstance(const VulkanInstance&) = delete;
    VulkanInstance& operator=(const VulkanInstance&) = delete;

    /// @brief Move constructor; transfers instance ownership.
    VulkanInstance(VulkanInstance&& other) noexcept : instance_(other.instance_) {
        other.instance_ = VK_NULL_HANDLE;
    }

    /// @brief Move assignment; transfers instance ownership.
    VulkanInstance& operator=(VulkanInstance&& other) noexcept {
        if (this != &other) {
            destroy();
            instance_       = other.instance_;
            other.instance_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    /// @brief Destructor; destroys the instance.
    ~VulkanInstance() { destroy(); }

    /// @brief Get the underlying Vulkan instance handle.
    /// @return The VkInstance handle; VK_NULL_HANDLE if not initialized.
    VkInstance get() const noexcept { return instance_; }
    
    /// @brief Check if the instance is valid (non-null).
    /// @return true if instance is not VK_NULL_HANDLE; false otherwise.
    explicit operator bool() const noexcept { return instance_ != VK_NULL_HANDLE; }

    /// @brief Release ownership without destroying.
    /// @return The Vulkan instance handle.
    /// @note After calling release(), the caller is responsible for destroying the instance.
    VkInstance release() noexcept {
        VkInstance tmp = instance_;
        instance_      = VK_NULL_HANDLE;
        return tmp;
    }

private:
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

/// @brief RAII wrapper for VkDevice (logical Vulkan device).
///
/// Manages the lifetime of a Vulkan logical device. Automatically waits for
/// device idle and destroys the device on scope exit (exception-safe RAII).
///
/// Features:
/// - Move semantics: efficient transfer of device ownership.
/// - Non-copyable: prevents accidental device duplication.
/// - Exception-safe: device is destroyed even during unwinding.
/// - Synchronization: waitIdle() before destruction ensures all work is complete.
///
/// Example usage:
/// ```cpp
/// VkDevice raw_dev = ...;
/// VulkanDevice dev(raw_dev);
/// // Device automatically waits for idle, then destroys on scope exit
/// ```
///
/// @note A destroyed device cannot be used to manage other Vulkan resources.
///       All resource wrappers (VulkanBuffer, etc.) should be destroyed first.
///
/// @see VulkanHandle for device-owned resource wrappers.
class VulkanDevice {
public:
    /// @brief Default constructor; does not own a device.
    VulkanDevice() = default;

    /// @brief Construct with a Vulkan logical device.
    /// @param device The VkDevice to manage.
    explicit VulkanDevice(VkDevice device) : device_(device) {}

    VulkanDevice(const VulkanDevice&) = delete;
    VulkanDevice& operator=(const VulkanDevice&) = delete;

    /// @brief Move constructor; transfers device ownership.
    VulkanDevice(VulkanDevice&& other) noexcept : device_(other.device_) {
        other.device_ = VK_NULL_HANDLE;
    }

    /// @brief Move assignment; transfers device ownership.
    VulkanDevice& operator=(VulkanDevice&& other) noexcept {
        if (this != &other) {
            destroy();
            device_       = other.device_;
            other.device_ = VK_NULL_HANDLE;
        }
        return *this;
    }

    /// @brief Destructor; waits for idle, then destroys the device.
    ~VulkanDevice() { destroy(); }

    /// @brief Get the underlying Vulkan device handle.
    /// @return The VkDevice handle; VK_NULL_HANDLE if not initialized.
    VkDevice get() const noexcept { return device_; }
    
    /// @brief Check if the device is valid (non-null).
    /// @return true if device is not VK_NULL_HANDLE; false otherwise.
    explicit operator bool() const noexcept { return device_ != VK_NULL_HANDLE; }

    /// @brief Wait for the device to become idle (all commands complete).
    /// @note This is a blocking call; useful before destroying resources.
    /// @note Safe to call when device is invalid (VK_NULL_HANDLE).
    void waitIdle() const {
        if (device_ != VK_NULL_HANDLE)
            vkDeviceWaitIdle(device_);
    }

    /// @brief Release ownership without destroying.
    /// @return The Vulkan device handle.
    /// @note After calling release(), the caller is responsible for destroying the device.
    VkDevice release() noexcept {
        VkDevice tmp = device_;
        device_      = VK_NULL_HANDLE;
        return tmp;
    }

private:
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

/// @brief RAII wrapper for scoped Vulkan device memory mapping.
///
/// Maps device memory on construction, unmaps on destruction. Enables scoped
/// CPU access to device memory without manual unmap() calls.
///
/// Features:
/// - Automatic mapping: vkMapMemory called on construction.
/// - Automatic unmapping: vkUnmapMemory called on destruction.
/// - Exception-safe: memory is unmapped even during unwinding.
/// - Move semantics: transfers mapping ownership.
/// - Non-copyable: prevents accidental mapping duplication.
///
/// Example usage:
/// ```cpp
/// {
///     ScopedMemoryMap mapped(device, memory, 0, size);
///     std::memcpy(mapped.data(), src, size);  // Write to GPU memory
/// }  // Memory automatically unmapped on scope exit
/// ```
///
/// @note The mapped memory is only valid within the scope of this object.
///       Accessing it after scope exit results in undefined behavior.
///
/// @throws std::runtime_error if vkMapMemory fails on construction.
class ScopedMemoryMap {
public:
    /// @brief Map device memory for CPU access.
    /// @param device The Vulkan logical device.
    /// @param memory The VkDeviceMemory to map.
    /// @param offset Memory offset in bytes.
    /// @param size Number of bytes to map.
    /// @param flags Optional memory map flags (default: 0).
    /// @throws std::runtime_error if vkMapMemory fails.
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

    /// @brief Move constructor; transfers mapping ownership.
    ScopedMemoryMap(ScopedMemoryMap&& other) noexcept
        : device_(other.device_), memory_(other.memory_), ptr_(other.ptr_) {
        other.ptr_ = nullptr;
    }

    /// @brief Move assignment; transfers mapping ownership.
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

    /// @brief Get a mutable pointer to the mapped memory.
    /// @return A void* pointer to the CPU-accessible mapped region.
    /// @note Valid only within the scope of this ScopedMemoryMap object.
    void* data() noexcept { return ptr_; }
    
    /// @brief Get a const pointer to the mapped memory.
    /// @return A const void* pointer to the CPU-accessible mapped region.
    /// @note Valid only within the scope of this ScopedMemoryMap object.
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
