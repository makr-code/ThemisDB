/**
 * @file infini_attention_cuda.h
 * @brief Infini-attention CUDA kernel implementation for SSM-Hybrid Phase 2 (P2-D02)
 *
 * Implements Infini-attention mechanism: unbounded context via compressive memory.
 * - Input: Query (Q), Key (K), Value (V), Compressive Memory (M)
 * - Output: Attention Output (O), Updated Memory (M')
 *
 * Compressive memory matrix M is stored in VRAM and updated via low-rank approximation:
 * M' = M + σ(α * k * v^T) where α controls update rate and k, v are compressed KV.
 *
 * Gate Compliance:
 * - P2-GATE-02: Numeric consistency with CPU fallback (src/llm/infini_attention_cpu.cpp)
 * - Integration: FlashAttentionFactory::create() selects backend automatically
 *
 * @author Copilot Coding Agent
 * @date 2026-07-22
 * @version 1.0.0
 */

#pragma once

#include "llm/attention/flash_attention.h"
#include "llm/attention/kv_cache_manager.h"
#include <memory>
#include <vector>
#include <cstddef>

namespace themis {
namespace llm {
namespace attention {

enum class ExtendedBackend {
    FLASH_ATTENTION,        // Standard Flash Attention v3
    INFINI_COMPRESSIVE      // Infini-attention with compressive memory
};

struct InfiniAttentionConfig {
    size_t memory_dim = 128;
    
    float update_rate = 0.1f;
    
    bool use_low_rank = true;
    
    size_t low_rank_dim = 32;
    
    size_t head_dim = 128;
    
    size_t num_heads = 8;
    
    int cuda_sm = 90;
    
    bool enable_fusion = true;
};

class InfiniAttentionCUDA {
public:
    /**
     * @brief Infini Attention CUDA.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit InfiniAttentionCUDA(const InfiniAttentionConfig& config);
    
    ~InfiniAttentionCUDA();
    
    /**
     * @brief Initialize.
     * @return Return value.
     */
    Status initialize();
    
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr);
    
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
        Tensor& dV);
    
    /**
     * @brief Get Backend Name.
     * @return Return value.
     */
    std::string getBackendName() const;
    
    /**
     * @brief Reset Memory.
     * @return Return value.
     */
    Status resetMemory();
    
    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    AttentionMemoryStats getMemoryStats() const;
    
    /**
     * @brief Get Compressive Memory.
     * @return Return value.
     */
    Tensor getCompressiveMemory() const;
    
    /**
     * @brief Restore Compressive Memory.
     * @param[in] checkpoint Input parameter.
     * @return Return value.
     */
    Status restoreCompressiveMemory(const Tensor& checkpoint);

private:
    InfiniAttentionConfig config_;
    
    float* gpu_memory_ = nullptr;
    
    float* gpu_memory_update_ = nullptr;
    
    float* gpu_temp_buffer_ = nullptr;
    
    size_t gpu_memory_size_ = 0;
    
    bool initialized_ = false;
    
    /**
     * @brief Compute Local Attention.
     * @param[in] Q Input parameter.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @param[in,out] O_local Input/output parameter.
     * @return Return value.
     */
    Status computeLocalAttention(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O_local);
    
    /**
     * @brief Compute Compressive Attention.
     * @param[in] Q Input parameter.
     * @param[in,out] O_comp Input/output parameter.
     * @return Return value.
     */
    Status computeCompressiveAttention(
        const Tensor& Q,
        Tensor& O_comp);
    
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
        Tensor& O_final);
    
    /**
     * @brief Update Compressive Memory.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @return Return value.
     */
    Status updateCompressiveMemory(
        const Tensor& K,
        const Tensor& V);
    
    /**
     * @brief Allocate GPUMemory.
     * @param[in] size_bytes Input parameter.
     * @return Pointer to the result.
     */
    float* allocateGPUMemory(size_t size_bytes);
    
    /**
     * @brief Release GPUMemory.
     */
    void releaseGPUMemory();
};

/**
 * @brief Create Infini Attention CUDA.
 * @param[in] config Input parameter.
 * @return Return value.
 */
std::unique_ptr<InfiniAttentionCUDA> createInfiniAttentionCUDA(
    const InfiniAttentionConfig& config);

} // namespace attention
} // namespace llm
} // namespace themis
