/**
 * @file infini_attention_hip.h
 * @brief Infini-attention HIP kernel API for AMD GPUs (P2-D02)
 *
 * Provides host-side interface for Infini-attention GPU kernels on AMD RDNA/CDNA architectures.
 *
 * @author Copilot Coding Agent (HIP Port)
 * @date 2026-07-22
 */

#pragma once

#include "../flash_attention.h"
#include "../flash_attention_config.h"
#include <memory>
#include <vector>

namespace themis {
namespace llm {
namespace attention {
namespace hip {

class InfiniAttentionHIP {
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

    /**
     * @brief Infini Attention HIP.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit InfiniAttentionHIP(const Config& config);

    ~InfiniAttentionHIP();

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

    std::string getBackendName() const { return "hip"; }

    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

    /**
     * @brief Initialize HIPDevice.
     * @return Return value.
     */
    static Status initializeHIPDevice();

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

    // GPU memory pointers (managed via HIP)
    void* gpu_memory_ = nullptr;              ///< Compressive memory M [memory_dim × memory_dim]
    void* gpu_memory_update_ = nullptr;       ///< Update buffer for M'
    void* gpu_temp_buffer_ = nullptr;         ///< Temporary computation buffer

    /**
     * @brief Allocate GPUMemory.
     * @param[in] bytes Input parameter.
     * @return Pointer to the result.
     */
    void* allocateGPUMemory(size_t bytes) const;

    /**
     * @brief Release GPUMemory.
     * @return Return value.
     */
    Status releaseGPUMemory();

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
};

} // namespace hip
} // namespace attention
} // namespace llm
} // namespace themis
