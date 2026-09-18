/**
 * @file vulkan_context.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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

class VulkanContext {
public:
    VulkanContext();
    ~VulkanContext();
    
    // Disable copy, enable move
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&& other) noexcept;
    VulkanContext& operator=(VulkanContext&& other) noexcept;
    
    bool initialize(int device_id = 0, bool enable_validation = false);
    
    /**
     * @brief Cleanup.
     */
    void cleanup();
    
    bool is_initialized() const { return initialized_; }
    
    /**
     * @brief Is available.
     * @return True when the operation succeeds.
     */
    static bool is_available();
    
    // ========== Getters ==========
    
    VkInstance instance() const { return instance_; }
    VkPhysicalDevice physical_device() const { return physical_device_; }
    VkDevice device() const { return device_; }
    VkQueue compute_queue() const { return compute_queue_; }
    VkCommandPool command_pool() const { return command_pool_; }
    uint32_t queue_family_index() const { return queue_family_index_; }
    
    const VkPhysicalDeviceProperties& device_properties() const {
        return device_properties_;
    }
    
    const VkPhysicalDeviceMemoryProperties& memory_properties() const {
        return memory_properties_;
    }
    
    // ========== Command Buffer Allocation ==========
    
    VkCommandBuffer allocate_command_buffer(
        VkCommandBufferLevel level = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    
    /**
     * @brief Free command buffer.
     * @param[in] command_buffer Input parameter.
     */
    void free_command_buffer(VkCommandBuffer command_buffer);
    
    // ========== Synchronization ==========
    
    VkFence create_fence(bool signaled = false);
    
    /**
     * @brief Destroy fence.
     * @param[in] fence Input parameter.
     */
    void destroy_fence(VkFence fence);
    
    bool wait_for_fence(VkFence fence, uint64_t timeout_ns = UINT64_MAX);
    
    /**
     * @brief Reset fence.
     * @param[in] fence Input parameter.
     */
    void reset_fence(VkFence fence);
    
    /**
     * @brief ========== Memory Utilities ==========
     * @param[in] type_filter Input parameter.
     * @param[in] properties Input parameter.
     * @return Return value.
     */
    
    int32_t find_memory_type(uint32_t type_filter,
                              VkMemoryPropertyFlags properties) const;
    
private:
    /**
     * @brief Create instance.
     * @param[in] enable_validation Input parameter.
     * @return True when the operation succeeds.
     */
    bool create_instance(bool enable_validation);
    
    /**
     * @brief Select physical device.
     * @param[in] device_id Identifier of the device.
     * @return True when the operation succeeds.
     */
    bool select_physical_device(int device_id);
    
    /**
     * @brief Find queue family.
     * @return True when the operation succeeds.
     */
    bool find_queue_family();
    
    /**
     * @brief Create device.
     * @return True when the operation succeeds.
     */
    bool create_device();
    
    /**
     * @brief Create command pool.
     * @return True when the operation succeeds.
     */
    bool create_command_pool();
    
    /**
     * @brief Setup debug messenger.
     * @return True when the operation succeeds.
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
     * @brief Check validation layer support.
     * @return True when the operation succeeds.
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

class VulkanContext {
public:
    VulkanContext() = default;
    ~VulkanContext() = default;
    
    VulkanContext(const VulkanContext&) = delete;
    VulkanContext& operator=(const VulkanContext&) = delete;
    VulkanContext(VulkanContext&&) noexcept = default;
    VulkanContext& operator=(VulkanContext&&) noexcept = default;
    
    bool initialize(int = 0, bool = false) { return false; }
    /**
     * @brief Cleanup.
     * @details Implements cleanup without additional internal calls.
     */
    void cleanup() {}
    bool is_initialized() const { return false; }
    /**
     * @brief Is available.
     * @return True when the operation succeeds.
     * @details Implements is_available without additional internal calls.
     */
    static bool is_available() { return false; }
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_HEADER
