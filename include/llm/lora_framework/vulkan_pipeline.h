/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            vulkan_pipeline.h                                  ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:03:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     224                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Vulkan compute pipeline for shader execution
 * 
 * Manages compute shader compilation, descriptor sets, and dispatch operations.
 */
class VulkanComputePipeline {
public:
    /**
     * @brief Create compute pipeline with shader
     * @param context Vulkan context
     * @param shader_path Path to SPIR-V shader file (.spv)
     */
    VulkanComputePipeline(VulkanContext* context, const std::string& shader_path);
    
    /**
     * @brief Create compute pipeline with shader code
     * @param context Vulkan context
     * @param shader_code SPIR-V shader bytecode
     */
    VulkanComputePipeline(VulkanContext* context, const std::vector<uint32_t>& shader_code);
    
    ~VulkanComputePipeline();
    
    // Disable copy, enable move
    VulkanComputePipeline(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline& operator=(const VulkanComputePipeline&) = delete;
    VulkanComputePipeline(VulkanComputePipeline&& other) noexcept;
    VulkanComputePipeline& operator=(VulkanComputePipeline&& other) noexcept;
    
    /**
     * @brief Create the compute pipeline
     * @param push_constant_size Size of push constants in bytes
     * @return true if successful
     */
    bool create(size_t push_constant_size = 0);
    
    /**
     * @brief Bind buffer to descriptor set
     * @param binding Binding index in shader
     * @param buffer Buffer to bind
     */
    void bind_buffer(uint32_t binding, const VulkanBuffer& buffer);
    
    /**
     * @brief Set push constants
     * @param data Push constant data
     * @param size Data size in bytes
     * @param offset Offset in push constant range
     */
    void set_push_constants(const void* data, size_t size, size_t offset = 0);
    
    /**
     * @brief Dispatch compute shader
     * @param group_x Number of workgroups in X dimension
     * @param group_y Number of workgroups in Y dimension
     * @param group_z Number of workgroups in Z dimension
     */
    void dispatch(uint32_t group_x, uint32_t group_y = 1, uint32_t group_z = 1);
    
    /**
     * @brief Wait for pipeline execution to complete
     */
    void wait();
    
    /**
     * @brief Check if pipeline is ready
     */
    bool is_ready() const { return pipeline_ != VK_NULL_HANDLE; }
    
    // Getters
    VkPipeline pipeline() const { return pipeline_; }
    VkPipelineLayout pipeline_layout() const { return pipeline_layout_; }
    
private:
    /**
     * @brief Load SPIR-V shader from file
     */
    std::vector<uint32_t> load_shader_file(const std::string& path);
    
    /**
     * @brief Create shader module
     */
    VkShaderModule create_shader_module(const std::vector<uint32_t>& code);
    
    /**
     * @brief Create descriptor set layout
     * Analyzes shader and creates appropriate layout
     */
    bool create_descriptor_set_layout();
    
    /**
     * @brief Create pipeline layout
     */
    bool create_pipeline_layout(size_t push_constant_size);
    
    /**
     * @brief Create compute pipeline
     */
    bool create_compute_pipeline();
    
    /**
     * @brief Create descriptor pool
     */
    bool create_descriptor_pool();
    
    /**
     * @brief Allocate descriptor sets
     */
    bool allocate_descriptor_sets();
    
    /**
     * @brief Update descriptor sets with bound buffers
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
    void cleanup() {}
    bool is_ready() const { return false; }
    bool bind_buffer(uint32_t, VulkanBuffer*) { return false; }
    bool set_push_constants(const void*, size_t) { return false; }
    bool dispatch(uint32_t, uint32_t, uint32_t) { return false; }
    bool wait() { return false; }
};

} // namespace vulkan
} // namespace lora
} // namespace themis

#endif // THEMIS_HAS_VULKAN_PIPELINE
