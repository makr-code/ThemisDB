/**
 * @file flash_attention_cuda.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "../flash_attention.h"
#include "../flash_attention_config.h"

#ifdef __CUDACC__
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#endif

namespace themis {
namespace llm {
namespace attention {
namespace cuda {

/**
 * @brief CUDA Flash Attention v3 implementation
 * 
 * Supports:
 * - SM90 (H100, RTX 6000 Ada): Full Flash Attention v3 with TMA, warp specialization
 * - SM86 (A100, RTX 4090): Flash Attention v2 optimizations
 * - SM80 (A100 early): Flash Attention v2
 */
class FlashAttentionCUDA : public IFlashAttention {
public:
    explicit FlashAttentionCUDA(const FlashAttentionConfig& config);
    ~FlashAttentionCUDA() override;
    
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr
    ) override;
    
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    ) override;
    
    std::string getBackendName() const override;
    AttentionMemoryStats getMemoryStats() const override;
    
    /**
     * @brief Get CUDA compute capability
     */
    static int getComputeCapability();
    
    /**
     * @brief Check if CUDA is available
     */
    static bool isAvailable();

private:
    FlashAttentionConfig config_;
    int compute_capability_ = 0;
    
    // CUDA resources
    void* d_workspace_ = nullptr;
    size_t workspace_size_ = 0;
    
    // Helper methods
    Status launchKernelSM90(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O);
    Status launchKernelSM86(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O);
    Status launchKernelSM80(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O);
    
    void allocateWorkspace();
    void freeWorkspace();
};

#ifdef __CUDACC__

/**
 * @brief Flash Attention v3 forward kernel (SM90 - Hopper)
 * 
 * Features:
 * - TMA (Tensor Memory Accelerator) for efficient data movement
 * - Warp specialization (producer/consumer warps)
 * - Async copy with pipeline architecture
 * - FP16/BF16 support with tensor cores
 */
__global__ void flash_attention_fwd_fused_fma_sm90(
    const __half* Q,          // [batch, seq_len, num_heads, head_dim]
    const __half* K,          // [batch, seq_len, num_heads, head_dim]
    const __half* V,          // [batch, seq_len, num_heads, head_dim]
    __half* O,                // [batch, seq_len, num_heads, head_dim]
    const int batch_size,
    const int seq_len,
    const int num_heads,
    const int head_dim,
    const float scale,
    const bool is_causal
);

/**
 * @brief Flash Attention v2 forward kernel (SM86 - Ampere)
 */
__global__ void flash_attention_fwd_sm86(
    const __half* Q,
    const __half* K,
    const __half* V,
    __half* O,
    const int batch_size,
    const int seq_len,
    const int num_heads,
    const int head_dim,
    const float scale,
    const bool is_causal
);

/**
 * @brief Paged attention forward kernel with block table
 * 
 * Supports variable-length sequences with paged KV cache
 */
__global__ void paged_attention_fwd(
    const __half* Q,                    // Query [seq_len, num_heads, head_dim]
    const __half** K_blocks,            // K block table
    const __half** V_blocks,            // V block table
    const int* block_table,             // block_id per token
    __half* O,                          // Output
    const int batch_size,
    const int seq_len,
    const int num_heads,
    const int head_dim,
    const float scale,
    const int block_size
);

/**
 * @brief Flash Attention backward kernel (for training)
 */
__global__ void flash_attention_bwd_fused(
    const __half* O,          // Output
    const __half* dO,         // Gradient of output
    const __half* Q,
    const __half* K,
    const __half* V,
    __half* dQ,
    __half* dK,
    __half* dV,
    const int batch_size,
    const int seq_len,
    const int num_heads,
    const int head_dim,
    const float scale,
    const bool is_causal
);

#endif // __CUDACC__

} // namespace cuda
} // namespace attention
} // namespace llm
} // namespace themis
