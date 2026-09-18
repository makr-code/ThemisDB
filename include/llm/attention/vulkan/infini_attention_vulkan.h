/**
 * @file infini_attention_vulkan.h
 * @brief Infini-attention Vulkan compute pipeline API (P2-D02)
 *
 * Provides host-side interface for Vulkan compute shader execution on multi-vendor GPUs.
 *
 * @author Copilot Coding Agent (Vulkan Port)
 * @date 2026-07-22
 */

#pragma once

#include "../flash_attention.h"
#include "../flash_attention_config.h"
#include <vulkan/vulkan.h>
#include <memory>
#include <vector>

namespace themis {
namespace llm {
namespace attention {
namespace vulkan {

class InfiniAttentionVulkan {
public:
    struct Config {
        size_t memory_dim = 128;           ///< Compressive memory dimension
        float update_rate = 0.1f;          ///< Contrastive learning rate α
        int low_rank = 4;                  ///< Low-rank projection dimension
        bool enable_fused_ops = true;      ///< Fuse kernels for efficiency
        size_t max_seq_len = 4096;         ///< Maximum sequence length
        int num_heads = 8;                 ///< Number of attention heads
        size_t head_dim = 64;              ///< Dimension per head
    };

    struct VulkanPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        VkShaderModule shader_module = VK_NULL_HANDLE;
    };

    /**
     * @brief Infini Attention Vulkan.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit InfiniAttentionVulkan(const Config& config);

    ~InfiniAttentionVulkan();

    /**
     * @brief Initialize.
     * @return Return value.
     */
    Status initialize();

    /**
     * @brief Forward.
     * @param[in] Q Input parameter.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @param[in,out] O Input/output parameter.
     * @return Return value.
     */
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O
    );

    /**
     * @brief Backward.
     * @param[in] dO Input parameter.
     * @param[in,out] dQ Input/output parameter.
     * @param[in,out] dK Input/output parameter.
     * @param[in,out] dV Input/output parameter.
     * @return Return value.
     */
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    );

    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    AttentionMemoryStats getMemoryStats() const;

    std::string getBackendName() const { return "vulkan"; }

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

    /**
     * @brief Initialize Vulkan Runtime.
     * @return Return value.
     */
    static Status initializeVulkanRuntime();

    /**
     * @brief Reset Memory.
     * @return Return value.
     */
    Status resetMemory();

    /**
     * @brief Get Compressive Memory.
     * @return Return value.
     */
    std::vector<float> getCompressiveMemory() const;

    /**
     * @brief Restore Compressive Memory.
     * @param[in] checkpoint Input parameter.
     * @return Return value.
     */
    Status restoreCompressiveMemory(const std::vector<float>& checkpoint);

private:
    Config config_;
    bool initialized_ = false;

    // Vulkan instance and device handles
    static VkInstance vulkan_instance_;
    static VkPhysicalDevice physical_device_;
    static VkDevice logical_device_;
    static VkQueue compute_queue_;
    static VkCommandPool command_pool_;

    // Compute pipelines (one per stage)
    VulkanPipeline pipeline_compressive_attention_;
    VulkanPipeline pipeline_memory_update_;
    VulkanPipeline pipeline_row_sums_;
    VulkanPipeline pipeline_blend_;

    // GPU memory buffers
    VkBuffer buffer_memory_ = VK_NULL_HANDLE;           ///< M [memory_dim × memory_dim]
    VkBuffer buffer_memory_update_ = VK_NULL_HANDLE;    ///< M_update [memory_dim × memory_dim]
    VkBuffer buffer_rowsums_ = VK_NULL_HANDLE;          ///< M_rowsum [memory_dim]
    VkDeviceMemory memory_gpu_;

    // Descriptor sets for kernel dispatch
    VkDescriptorPool descriptor_pool_ = VK_NULL_HANDLE;
    VkDescriptorSet descriptor_set_ = VK_NULL_HANDLE;

    /**
     * @brief Allocate GPUBuffer.
     * @param[in] size Input parameter.
     * @param[in] usage Input parameter.
     * @return Return value.
     */
    VkBuffer allocateGPUBuffer(size_t size, VkBufferUsageFlags usage);

    /**
     * @brief Release GPUBuffer.
     * @param[in] buffer Input parameter.
     * @return Return value.
     */
    Status releaseGPUBuffer(VkBuffer buffer);

    /**
     * @brief Load Shader Module.
     * @param[in] path Input parameter.
     * @return Return value.
     */
    VkShaderModule loadShaderModule(const char* path) const;

    /**
     * @brief Create Compute Pipeline.
     * @param[in] shader_module Input parameter.
     * @param[in,out] pipeline Input/output parameter.
     * @return Return value.
     */
    Status createComputePipeline(
        VkShaderModule shader_module,
        VulkanPipeline& pipeline
    );

    /**
     * @brief Dispatch Kernel.
     * @param[in] pipeline Input parameter.
     * @param[in] grid_x Input parameter.
     * @param[in] grid_y Input parameter.
     * @param[in] grid_z Input parameter.
     * @return Return value.
     */
    Status dispatchKernel(
        const VulkanPipeline& pipeline,
        uint32_t grid_x,
        uint32_t grid_y,
        uint32_t grid_z
    );

    /**
     * @brief Compute Local Attention.
     * @param[in] Q Input parameter.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @param[in,out] O Input/output parameter.
     * @return Return value.
     */
    Status computeLocalAttention(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O
    );

    /**
     * @brief Compute Compressive Attention.
     * @param[in] Q Input parameter.
     * @param[in,out] O Input/output parameter.
     * @return Return value.
     */
    Status computeCompressiveAttention(
        const Tensor& Q,
        Tensor& O
    );

    /**
     * @brief Update Compressive Memory.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @return Return value.
     */
    Status updateCompressiveMemory(
        const Tensor& K,
        const Tensor& V
    );

    /**
     * @brief Blend Outputs.
     * @param[in] O_local Input parameter.
     * @param[in] O_comp Input parameter.
     * @param[in,out] O_final Input/output parameter.
     * @return Return value.
     */
    Status blendOutputs(
        const Tensor& O_local,
        const Tensor& O_comp,
        Tensor& O_final
    );

    /**
     * @brief Copy Device To Host.
     * @param[in] device_buffer Input parameter.
     * @param[in,out] host_data Input/output parameter.
     * @param[in] size Input parameter.
     * @return Return value.
     */
    Status copyDeviceToHost(
        VkBuffer device_buffer,
        void* host_data,
        size_t size
    ) const;

    /**
     * @brief Copy Host To Device.
     * @param[in] host_data Input parameter.
     * @param[in] device_buffer Input parameter.
     * @param[in] size Input parameter.
     * @return Return value.
     */
    Status copyHostToDevice(
        const void* host_data,
        VkBuffer device_buffer,
        size_t size
    );
};

} // namespace vulkan
} // namespace attention
} // namespace llm
} // namespace themis
