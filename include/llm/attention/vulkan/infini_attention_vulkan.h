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

/**
 * @brief Infini-attention implementation via Vulkan compute shaders
 *
 * Supports all Vulkan 1.2+ capable GPUs (NVIDIA, AMD, Intel, Apple Metal via MoltenVK).
 * Uses compute shaders compiled to SPIR-V.
 *
 * Thread-safe forward pass with configurable memory compression.
 */
class InfiniAttentionVulkan {
public:
    /**
     * @brief Configuration for Vulkan Infini-attention
     */
    struct Config {
        size_t memory_dim = 128;           ///< Compressive memory dimension
        float update_rate = 0.1f;          ///< Contrastive learning rate α
        int low_rank = 4;                  ///< Low-rank projection dimension
        bool enable_fused_ops = true;      ///< Fuse kernels for efficiency
        size_t max_seq_len = 4096;         ///< Maximum sequence length
        int num_heads = 8;                 ///< Number of attention heads
        size_t head_dim = 64;              ///< Dimension per head
    };

    /**
     * @brief Vulkan compute pipeline metadata
     */
    struct VulkanPipeline {
        VkPipeline pipeline = VK_NULL_HANDLE;
        VkPipelineLayout layout = VK_NULL_HANDLE;
        VkDescriptorSetLayout descriptor_set_layout = VK_NULL_HANDLE;
        VkShaderModule shader_module = VK_NULL_HANDLE;
    };

    /**
     * @brief Initialize Vulkan Infini-attention
     *
     * Loads SPIR-V shaders, creates compute pipelines, allocates GPU buffers.
     * Multi-stage: compressive attention, memory update, row-sum, blend.
     *
     * @param config Configuration parameters
     * @throws std::runtime_error if Vulkan unavailable or shader compilation fails
     */
    explicit InfiniAttentionVulkan(const Config& config);

    /**
     * @brief Destructor - releases all Vulkan resources
     */
    ~InfiniAttentionVulkan();

    /**
     * @brief Ensure backend resources are initialized.
     * @return Status::SUCCESS when resources are ready.
     */
    Status initialize();

    /**
     * @brief Forward pass: compute attention with compressive memory
     *
     * Pipeline:
     * 1. Local attention: dispatch compute shader for Flash Attention
     * 2. Compressive attention: sigmoid(Q @ M^T) @ m_v
     * 3. Memory update: M' = M + α * σ(K) ⊗ σ(V)
     * 4. Blend: α_blend * local + (1 - α_blend) * compressive
     *
     * @param Q Query tensor [batch*seq_len, num_heads, head_dim]
     * @param K Key tensor [batch*seq_len, num_heads, head_dim]
     * @param V Value tensor [batch*seq_len, num_heads, head_dim]
     * @param[out] O Output tensor [batch*seq_len, num_heads, head_dim]
     * @return Status code (SUCCESS on completion)
     *
     * @note Numerically stable: sigmoid clamped ±50, ε=1e-6
     * @note VRAM: ~256KB for 128×128 memory (0.006% of typical 4GB)
     */
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O
    );

    /**
     * @brief Backward pass - compute gradients (Phase 2.2)
     *
     * @param dO Gradient w.r.t. output
     * @param[out] dQ Gradient w.r.t. query
     * @param[out] dK Gradient w.r.t. key
     * @param[out] dV Gradient w.r.t. value
     * @return Status code
     *
     * @note Currently returns STATUS_NOT_IMPLEMENTED (deferred to Phase 2.2)
     */
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    );

    /**
     * @brief Get memory statistics
     *
     * @return AttentionMemoryStats with total VRAM usage breakdown
     */
    AttentionMemoryStats getMemoryStats() const;

    /**
     * @brief Get backend name identifier
     *
     * @return "vulkan" identifying this as Vulkan backend
     */
    std::string getBackendName() const { return "vulkan"; }

    /**
     * @brief Check Vulkan availability on this system
     *
     * @return true if Vulkan 1.2+ is available with a capable GPU
     */
    static bool isAvailable();

    /**
     * @brief Initialize Vulkan runtime (first-time setup)
     *
     * Creates Vulkan instance, selects GPU device, initializes command buffers.
     * Called automatically by constructor if Vulkan not yet initialized.
     *
     * @return Status code
     */
    static Status initializeVulkanRuntime();

    /**
     * @brief Reset compressive memory to zeros
     *
     * Issues GPU command to clear M matrix for fresh attention computation.
     *
     * @return Status code
     */
    Status resetMemory();

    /**
     * @brief Get checkpoint of compressive memory
     *
     * Transfers M from GPU to host memory for serialization/debugging.
     *
     * @return Copy of current M matrix
     * @throws std::runtime_error if GPU memory not allocated
     */
    std::vector<float> getCompressiveMemory() const;

    /**
     * @brief Restore compressive memory from checkpoint
     *
     * Transfers checkpoint data from host to GPU memory.
     *
     * @param checkpoint Memory matrix to restore
     * @return Status code
     * @throws std::invalid_argument if checkpoint size mismatch
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
     * @brief Allocate GPU buffer memory via vkCreateBuffer + vkAllocateMemory
     *
     * @param size Buffer size in bytes
     * @param usage Vulkan buffer usage flags
     * @return VkBuffer on success, VK_NULL_HANDLE on failure
     */
    VkBuffer allocateGPUBuffer(size_t size, VkBufferUsageFlags usage);

    /**
     * @brief Release GPU buffer memory
     *
     * @param buffer Vulkan buffer handle
     * @return Status code
     */
    Status releaseGPUBuffer(VkBuffer buffer);

    /**
     * @brief Load SPIR-V shader module from file
     *
     * @param path File path to .spv bytecode
     * @return VkShaderModule on success, VK_NULL_HANDLE on failure
     */
    VkShaderModule loadShaderModule(const char* path) const;

    /**
     * @brief Create compute pipeline from shader module
     *
     * @param shader_module Compiled SPIR-V module
     * @param[out] pipeline Resulting compute pipeline
     * @return Status code
     */
    Status createComputePipeline(
        VkShaderModule shader_module,
        VulkanPipeline& pipeline
    );

    /**
     * @brief Dispatch compute kernel
     *
     * Issues vkCmdDispatch to compute queue with proper synchronization.
     *
     * @param pipeline Pipeline to dispatch
     * @param grid_x Groups in X dimension
     * @param grid_y Groups in Y dimension
     * @param grid_z Groups in Z dimension
     * @return Status code
     */
    Status dispatchKernel(
        const VulkanPipeline& pipeline,
        uint32_t grid_x,
        uint32_t grid_y,
        uint32_t grid_z
    );

    /**
     * @brief Compute local attention via Flash Attention
     *
     * @param Q Query tensor
     * @param K Key tensor
     * @param V Value tensor
     * @param[out] O Output tensor
     * @return Status code
     *
     * @note Placeholder (Phase 2.2): calls CPU fallback currently
     */
    Status computeLocalAttention(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O
    );

    /**
     * @brief Compute compressive attention: sigmoid(Q @ M^T) @ m_v
     *
     * @param Q Query tensor
     * @param[out] O Output tensor [batch*seq_len, num_heads, memory_dim]
     * @return Status code
     *
     * @note Dispatches pipeline_compressive_attention_ compute shader
     */
    Status computeCompressiveAttention(
        const Tensor& Q,
        Tensor& O
    );

    /**
     * @brief Update compressive memory M via low-rank approximation
     *
     * M' = M + α * sigmoid(K_compressed) ⊗ sigmoid(V_compressed)
     *
     * @param K Key tensor
     * @param V Value tensor
     * @return Status code
     *
     * @note Dispatches pipeline_memory_update_ compute shader
     */
    Status updateCompressiveMemory(
        const Tensor& K,
        const Tensor& V
    );

    /**
     * @brief Blend local and compressive outputs
     *
     * O_final = α_blend * O_local + (1 - α_blend) * O_comp
     *
     * @param O_local Output from Flash Attention
     * @param O_comp Output from compressive attention
     * @param[out] O_final Blended output
     * @return Status code
     *
     * @note Dispatches pipeline_blend_ compute shader
     */
    Status blendOutputs(
        const Tensor& O_local,
        const Tensor& O_comp,
        Tensor& O_final
    );

    /**
     * @brief Copy buffer from device to host
     *
     * @param device_buffer Device-side buffer handle
     * @param[out] host_data Host memory destination
     * @param size Number of bytes to copy
     * @return Status code
     */
    Status copyDeviceToHost(
        VkBuffer device_buffer,
        void* host_data,
        size_t size
    ) const;

    /**
     * @brief Copy buffer from host to device
     *
     * @param host_data Host memory source
     * @param[out] device_buffer Device-side buffer handle
     * @param size Number of bytes to copy
     * @return Status code
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
