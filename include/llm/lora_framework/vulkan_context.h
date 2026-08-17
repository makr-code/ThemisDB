/**
 * @file vulkan_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// Check if Vulkan header is available
#if defined(__has_include)
#  if __has_include(<vulkan/vulkan.h>)
#    include <vulkan/vulkan.h>
#    define THEMIS_HAS_VULKAN_HEADER 1
#  else
#    define THEMIS_HAS_VULKAN_HEADER 0
#  endif
#else
// Compiler doesn't support __has_include, assume Vulkan is available if THEMIS_ENABLE_VULKAN is defined
#  if defined(THEMIS_ENABLE_VULKAN)
#    include <vulkan/vulkan.h>
#    define THEMIS_HAS_VULKAN_HEADER 1
#  else
#    define THEMIS_HAS_VULKAN_HEADER 0
#  endif
#endif

#if !THEMIS_HAS_VULKAN_HEADER && defined(THEMIS_ENABLE_VULKAN)
#  error "Vulkan SDK not found. Please install Vulkan SDK or disable THEMIS_ENABLE_VULKAN in CMake configuration."
#endif

#include <vector>
#include <array>
#include <string>
#include <memory>
#include <cstdint>

#if THEMIS_HAS_VULKAN_HEADER

namespace themis {
namespace lora {
namespace vulkan {

/**
 * @brief Vulkan context for compute pipeline operations
 * 
 * Manages Vulkan instance, device, queue, and command pool for LoRA training.
 * Provides resource management and synchronization primitives.
 */
class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();
    
    // Disable copy, enable move
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&& other) noexcept;
    VulkanContext& operator=(VulkanContext&& other) noexcept;
    
    /**
     * @brief Initialize Vulkan context
     * @param device_id Physical device index (0 for default)
     * @param enable_validation Enable validation layers for debugging
     * @return true if initialization successful
     */
    bool initialize(int device_id = 0, bool enable_validation = false);
    
    /**
     * @brief Cleanup all Vulkan resources
     */
    void cleanup();
    
    /**
     * @brief Check if context is initialized
     */
    bool is_initialized() const { return initialized_; }
    
    /**
     * @brief Check if Vulkan is available on this system
     */
    static bool is_available();
    
    // ========== Getters ==========
    
    VkInstance instance() const { return instance_; }
    VkPhysicalDevice physical_device() const { return physical_device_; }
    VkDevice device() const { return device_; }
    VkQueue compute_queue() const { return compute_queue_; }
    VkCommandPool command_pool() const { return command_pool_; }
    uint32_t queue_family_index() const { return queue_family_index_; }
    
    /**
     * @brief Get device properties
     */
    const VkPhysicalDeviceProperties& device_properties() const {
        return device_properties_;
    }
    
    /**
     * @brief Get device memory properties
     */
    const VkPhysicalDeviceMemoryProperties& memory_properties() const {
        return memory_properties_;
    }
    
    // ========== Command Buffer Allocation ==========
    
    /**
     * @brief Allocate a command buffer
     * @param level Command buffer level (primary or secondary)
     * @return Allocated command buffer
     */
    VkCommandBuffer allocate_command_buffer(
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
    /**
     * @brief Free a command buffer
     *
     * No-op if the command buffer handle is null.
     */
    void free_command_buffer(VkCommandBuffer command_buffer);
    
    // ========== Synchronization ==========
    
    /**
     * @brief Create a fence
     * @param signaled Create fence in signaled state
     */
    VkFence create_fence(bool signaled = false);
    
    /**
     * @brief Destroy a fence
     *
     * No-op if the fence handle is null.
     */
    void destroy_fence(VkFence fence);
    
    /**
     * @brief Wait for fence to be signaled
     * @param fence Fence to wait on
     * @param timeout_ns Timeout in nanoseconds (UINT64_MAX for infinite)
     * @return false if waiting fails or the fence/context handle is invalid
     */
    bool wait_for_fence(VkFence fence, uint64_t timeout_ns = UINT64_MAX);
    
    /**
     * @brief Reset a fence
     * @throws std::runtime_error if fence/context handle is invalid
     * @throws std::runtime_error if Vulkan fails to reset the fence
     */
    void reset_fence(VkFence fence);
    
    // ========== Memory Utilities ==========
    
    /**
     * @brief Find suitable memory type for allocation
     * @param type_filter Type filter from buffer/image requirements
     * @param properties Required memory properties
     * @return Memory type index, or -1 if not found
     */
    int32_t find_memory_type(uint32_t type_filter,
                              VkMemoryPropertyFlags properties) const;
    
private:
    /**
     * @brief Create Vulkan instance
     */
    bool create_instance(bool enable_validation);
    
    /**
     * @brief Select physical device (GPU)
     */
    bool select_physical_device(int device_id);
    
    /**
     * @brief Find compute queue family
     */
    bool find_queue_family();
    
    /**
     * @brief Create logical device
     */
    bool create_device();
    
    /**
     * @brief Create command pool
     */
    bool create_command_pool();
    
    /**
     * @brief Setup debug messenger (if validation enabled)
     */
    bool setup_debug_messenger();
    
    // Vulkan handles
    VkInstance instance_ = VK_NULL_HANDLE;
    VkPhysicalDevice physical_device_ = VK_NULL_HANDLE;
    VkDevice device_ = VK_NULL_HANDLE;
    VkQueue compute_queue_ = VK_NULL_HANDLE;
    VkCommandPool command_pool_ = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT debug_messenger_ = VK_NULL_HANDLE;
    
    // Device info
    uint32_t queue_family_index_ = 0;
    VkPhysicalDeviceProperties device_properties_ = {};
    VkPhysicalDeviceMemoryProperties memory_properties_ = {};
    
    // State
    bool initialized_ = false;
    bool validation_enabled_ = false;
    
    // Validation layers (returned by value to avoid static storage initialization concerns)
    static constexpr std::array<const char*, 1> validation_layers() noexcept {
        return {"VK_LAYER_KHRONOS_validation"};
    }
    
    /**
     * @brief Check if validation layers are available
     */
    static bool check_validation_layer_support();
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#else // !THEMIS_HAS_VULKAN_HEADER

// Stub implementation when Vulkan is not available
namespace themis {
namespace lora {
namespace vulkan {

/** @brief Vulkan context object for. */
class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext() = default;
    
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) noexcept noexcept = default;
    VulkanContext& operator=(VulkanContext&&) noexcept noexcept = default;
    
    bool initialize(int = 0, bool = false) { return false; }
    void cleanup() {}
    bool is_initialized() const { return false; }
    static bool is_available() { return false; }
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_HEADER
