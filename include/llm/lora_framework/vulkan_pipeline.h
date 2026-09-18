/**
 * @file vulkan_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/lora_framework/vulkan_context.h"
#include "llm/lora_framework/vulkan_buffer.h"

#if __has_include(<vulkan/vulkan.h>)
#  include <vulkan/vulkan.h>
#  define THEMIS_HAS_VULKAN_PIPELINE 1
#else
#  define THEMIS_HAS_VULKAN_PIPELINE 0
#endif

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

#if THEMIS_HAS_VULKAN_PIPELINE

namespace themis {
namespace lora {
namespace vulkan {

class VulkanComputePipeline {
public:
    VulkanComputePipeline(VulkanContext* context, const std::string& shader_path);
    
    VulkanComputePipeline(VulkanContext* context, const std::vector<uint32_t>& shader_code);
    
    ~VulkanComputePipeline() noexcept;
    
    // Disable copy, enable move
    VulkanComputePipeline(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline& operator=(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline(VulkanComputePipeline&& other) noexcept;
    VulkanComputePipeline& operator=(VulkanComputePipeline&& other) noexcept;
    
    bool create(size_t push_constant_size = 0);
    
    /**
     * @brief Bind buffer.
     * @param[in] binding Input parameter.
     * @param[in] buffer Input parameter.
     */
    void bind_buffer(uint32_t binding, const VulkanBuffer& buffer);
    
    void set_push_constants(const void* data, size_t size, size_t offset = 0);
    
    void dispatch(uint32_t group_x, uint32_t group_y = 1, uint32_t group_z = 1);
    
    bool wait(uint64_t timeout_ns = 30'000'000'000ULL);
    
    bool is_ready() const { return pipeline_ != VK_NULL_HANDLE; }
    
    // Getters
    VkPipeline pipeline() const { return pipeline_; }
    VkPipelineLayout pipeline_layout() const { return pipeline_layout_; }
    
private:
    /**
     * @brief Load shader file.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    std::vector<uint32_t> load_shader_file(const std::string& path);
    
    /**
     * @brief Create shader module.
     * @param[in] code Input parameter.
     * @return Return value.
     */
    VkShaderModule create_shader_module(const std::vector<uint32_t>& code);
    
    /**
     * @brief Create descriptor set layout.
     * @return True when the operation succeeds.
     */
    bool create_descriptor_set_layout();
    
    /**
     * @brief Create pipeline layout.
     * @param[in] push_constant_size Input parameter.
     * @return True when the operation succeeds.
     */
    bool create_pipeline_layout(size_t push_constant_size);
    
    /**
     * @brief Create compute pipeline.
     * @return True when the operation succeeds.
     */
    bool create_compute_pipeline();
    
    /**
     * @brief Create descriptor pool.
     * @return True when the operation succeeds.
     */
    bool create_descriptor_pool();
    
    /**
     * @brief Allocate descriptor sets.
     * @return True when the operation succeeds.
     */
    bool allocate_descriptor_sets();
    
    /**
     * @brief Update descriptor sets.
     */
    void update_descriptor_sets();
    
    VulkanContext* context_;
    std::vector<uint32_t> shader_code_;
    
    // Vulkan handles
    VkShaderModule shader_module_ = VK_NULL_HANDLE;
    VkPipeline pipeline_ = VK_NULL_HANDLE;
    VkPipelineLayout pipeline_layout_ = VK_NULL_HANDLE;
    VkDescriptorSetLayout descriptor_set_layout_ = VK_NULL_HANDLE;
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;
    
    // Command buffer and synchronization
    VkCommandBuffer command_buffer_ = VK_NULL_HANDLE;
    VkFence fence_ = VK_NULL_HANDLE;
    
    // Push constants
    size_t push_constant_size_ = 0;
    std::vector<uint8_t> push_constant_data_;
    
    // Buffer bindings
    std::unordered_map<uint32_t, VkBuffer> buffer_bindings_;
    std::unordered_map<uint32_t, VkDeviceSize> buffer_sizes_;
    bool descriptors_dirty_ = false;
    
    // Maximum number of descriptor bindings (storage buffers)
    static constexpr uint32_t MAX_BINDINGS = 16;
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#else // !THEMIS_HAS_VULKAN_PIPELINE

// Stub implementation when Vulkan is not available
namespace themis {
namespace lora {
namespace vulkan {

class VulkanComputePipeline {
public:
    VulkanComputePipeline(VulkanContext*, const std::string&) {}
    VulkanComputePipeline(VulkanContext*, const std::vector<uint32_t>&) {}
    ~VulkanComputePipeline() = default;
    
    VulkanComputePipeline(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline& operator=(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline(VulkanComputePipeline&&) noexcept = default;
    VulkanComputePipeline& operator=(VulkanComputePipeline&&) noexcept = default;
    
    bool create(size_t = 0) { return false; }
    /**
     * @brief Cleanup.
     * @details Implements cleanup without additional internal calls.
     */
    void cleanup() {}
    bool is_ready() const { return false; }
    /**
     * @brief Bind buffer.
     * @param[in] uint32_t Input parameter.
     * @param[in,out] param Input/output parameter.
     * @return True when the operation succeeds.
     * @details Implements bind_buffer without additional internal calls.
     */
    bool bind_buffer(uint32_t, VulkanBuffer*) { return false; }
    /**
     * @brief Set push constants.
     * @param[in] param Input parameter.
     * @param[in] size_t Input parameter.
     * @return True when the operation succeeds.
     * @details Implements set_push_constants without additional internal calls.
     */
    bool set_push_constants(const void*, size_t) { return false; }
    /**
     * @brief Dispatch.
     * @param[in] uint32_t Input parameter.
     * @param[in] uint32_t Input parameter.
     * @param[in] uint32_t Input parameter.
     * @return True when the operation succeeds.
     * @details Implements dispatch without additional internal calls.
     */
    bool dispatch(uint32_t, uint32_t, uint32_t) { return false; }
    bool wait(uint64_t = 0) { return false; }
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_PIPELINE
