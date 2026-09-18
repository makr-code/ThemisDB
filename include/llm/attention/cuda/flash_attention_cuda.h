/**
 * @file flash_attention_cuda.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class FlashAttentionCUDA : public IFlashAttention {
public:
    /**
     * @brief Flash Attention CUDA.
     * @param[in] config Input parameter.
     * @return Return value.
     */
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
     * @brief Get Compute Capability.
     * @return Return value.
     */
    static int getComputeCapability();
    
    /**
     * @brief Is Available.
     * @return True when the operation succeeds.
     */
    static bool isAvailable();

private:
    FlashAttentionConfig config_;
    int compute_capability_ = 0;
    
    // CUDA resources
    void* d_workspace_ = nullptr;
    size_t workspace_size_ = 0;
    
    // Helper methods
    /**
     * @brief Launch Kernel SM90.
     * @param[in] Q Input parameter.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @param[in,out] O Input/output parameter.
     * @return Return value.
     */
    Status launchKernelSM90(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O);
    /**
     * @brief Launch Kernel SM86.
     * @param[in] Q Input parameter.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @param[in,out] O Input/output parameter.
     * @return Return value.
     */
    Status launchKernelSM86(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O);
    /**
     * @brief Launch Kernel SM80.
     * @param[in] Q Input parameter.
     * @param[in] K Input parameter.
     * @param[in] V Input parameter.
     * @param[in,out] O Input/output parameter.
     * @return Return value.
     */
    Status launchKernelSM80(const Tensor& Q, const Tensor& K, const Tensor& V, Tensor& O);
    
    /**
     * @brief Allocate Workspace.
     */
    void allocateWorkspace();
    /**
     * @brief Free Workspace.
     */
    void freeWorkspace();
};

#ifdef __CUDACC__

/**
 * @brief Flash attention fwd fused fma sm90.
 * @param[in] Q Input parameter.
 * @param[in] K Input parameter.
 * @param[in] V Input parameter.
 * @param[in,out] O Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] seq_len Input parameter.
 * @param[in] num_heads Input parameter.
 * @param[in] head_dim Input parameter.
 * @param[in] scale Input parameter.
 * @param[in] is_causal Input parameter.
 * @return Return value.
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
 * @brief Flash attention fwd sm86.
 * @param[in] Q Input parameter.
 * @param[in] K Input parameter.
 * @param[in] V Input parameter.
 * @param[in,out] O Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] seq_len Input parameter.
 * @param[in] num_heads Input parameter.
 * @param[in] head_dim Input parameter.
 * @param[in] scale Input parameter.
 * @param[in] is_causal Input parameter.
 * @return Return value.
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
 * @brief Paged attention fwd.
 * @param[in] Q Input parameter.
 * @param[in] K_blocks Input parameter.
 * @param[in] V_blocks Input parameter.
 * @param[in] block_table Input parameter.
 * @param[in,out] O Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] seq_len Input parameter.
 * @param[in] num_heads Input parameter.
 * @param[in] head_dim Input parameter.
 * @param[in] scale Input parameter.
 * @param[in] block_size Input parameter.
 * @return Return value.
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
 * @brief Flash attention bwd fused.
 * @param[in] O Input parameter.
 * @param[in] dO Input parameter.
 * @param[in] Q Input parameter.
 * @param[in] K Input parameter.
 * @param[in] V Input parameter.
 * @param[in,out] dQ Input/output parameter.
 * @param[in,out] dK Input/output parameter.
 * @param[in,out] dV Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] seq_len Input parameter.
 * @param[in] num_heads Input parameter.
 * @param[in] head_dim Input parameter.
 * @param[in] scale Input parameter.
 * @param[in] is_causal Input parameter.
 * @return Return value.
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
