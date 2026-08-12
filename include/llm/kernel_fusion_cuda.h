/**
 * @file kernel_fusion_cuda.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef __CUDACC__
#include <cuda_runtime.h>
#else
// For non-CUDA builds, define cudaStream_t as void*
typedef void* cudaStream_t;
#define cudaStream_t void*
#endif

namespace themis {
namespace llm {
namespace kernels {
namespace cuda {

/**
 * @brief CUDA Kernel Launch Functions for Flash Attention
 * 
 * These functions provide C++ interface to CUDA kernels.
 * They are only available when THEMIS_ENABLE_CUDA is defined.
 */

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Launch Flash Attention Forward Kernel
 * 
 * Implements memory-efficient attention using tiling strategy.
 * Based on Flash Attention paper (https://arxiv.org/abs/2205.14135)
 * 
 * @param d_Q Query tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_K Key tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_V Value tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_O Output tensor on device (batch * num_heads * seq_len * head_dim)
 * @param batch_size Batch size
 * @param num_heads Number of attention heads
 * @param seq_len Sequence length
 * @param head_dim Dimension per head
 * @param scale Scaling factor (typically 1/sqrt(head_dim))
 * @param is_causal Whether to apply causal masking
 * @param stream CUDA stream for async execution
 */
void launchFlashAttentionForward(
    const float* d_Q,
    const float* d_K,
    const float* d_V,
    float* d_O,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    bool is_causal,
    cudaStream_t stream = 0
);

/**
 * @brief Launch Flash Attention Backward Kernel
 * 
 * Computes gradients for Q, K, V using Flash Attention backward algorithm.
 * Based on Flash Attention paper Algorithm 2.
 * 
 * @param d_dO Gradient of output on device (batch * num_heads * seq_len * head_dim)
 * @param d_Q Query tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_K Key tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_V Value tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_O Output from forward pass on device (batch * num_heads * seq_len * head_dim)
 * @param d_dQ Gradient of query on device (output)
 * @param d_dK Gradient of key on device (output)
 * @param d_dV Gradient of value on device (output)
 * @param batch_size Batch size
 * @param num_heads Number of attention heads
 * @param seq_len Sequence length
 * @param head_dim Dimension per head
 * @param scale Scaling factor (typically 1/sqrt(head_dim))
 * @param is_causal Whether to apply causal masking
 * @param stream CUDA stream for async execution
 */
void launchFlashAttentionBackward(
    const float* d_dO,
    const float* d_Q,
    const float* d_K,
    const float* d_V,
    const float* d_O,
    float* d_dQ,
    float* d_dK,
    float* d_dV,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    bool is_causal,
    cudaStream_t stream = 0
);

/**
 * @brief Launch Fused QKV Projection Kernel
 * 
 * Projects input to Query, Key, Value in single kernel launch
 * 
 * @param d_input Input tensor on device (batch * seq_len * hidden_dim)
 * @param d_qkv_weight Weight matrix on device (hidden_dim * 3 * hidden_dim)
 * @param d_qkv_bias Bias vector on device (3 * hidden_dim), can be nullptr
 * @param d_Q Output query on device (batch * seq_len * hidden_dim)
 * @param d_K Output key on device (batch * seq_len * hidden_dim)
 * @param d_V Output value on device (batch * seq_len * hidden_dim)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden dimension
 * @param stream CUDA stream for async execution
 */
void launchFusedQKVProjection(
    const float* d_input,
    const float* d_qkv_weight,
    const float* d_qkv_bias,
    float* d_Q,
    float* d_K,
    float* d_V,
    int batch_size,
    int seq_len,
    int hidden_dim,
    cudaStream_t stream = 0
);

/**
 * @brief Launch Fused RoPE (Rotary Position Embedding) Kernel
 * 
 * Applies rotary position embeddings to queries or keys
 * 
 * @param d_QK Query or Key tensor on device (batch * num_heads * seq_len * head_dim)
 * @param d_position_ids Position indices on device (seq_len), can be nullptr for 0..seq_len-1
 * @param batch_size Batch size
 * @param num_heads Number of attention heads
 * @param seq_len Sequence length
 * @param head_dim Dimension per head
 * @param rope_base Base for rotary embedding (typically 10000)
 * @param stream CUDA stream for async execution
 */
void launchFusedRoPE(
    float* d_QK,
    const int* d_position_ids,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    int rope_base,
    cudaStream_t stream = 0
);

/**
 * @brief Launch Fused LayerNorm + Linear Kernel
 * 
 * Applies LayerNorm followed by linear projection in single kernel
 * 
 * @param d_output Output tensor on device (batch * seq_len * hidden_dim)
 * @param d_input Input tensor on device (batch * seq_len * hidden_dim)
 * @param d_weight Linear weight on device (hidden_dim * hidden_dim)
 * @param d_bias Linear bias on device (hidden_dim), can be nullptr
 * @param d_ln_weight LayerNorm gamma on device (hidden_dim)
 * @param d_ln_bias LayerNorm beta on device (hidden_dim)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden dimension
 * @param epsilon Epsilon for numerical stability (typically 1e-5)
 * @param stream CUDA stream for async execution
 */
void launchFusedLayerNormLinear(
    float* d_output,
    const float* d_input,
    const float* d_weight,
    const float* d_bias,
    const float* d_ln_weight,
    const float* d_ln_bias,
    int batch_size,
    int seq_len,
    int hidden_dim,
    float epsilon,
    cudaStream_t stream = 0
);

/**
 * @brief Launch Fused Gated FFN Kernel
 * 
 * Implements gated feed-forward: gate * silu(up) -> down
 * Used in LLaMA and similar transformer models
 * 
 * @param d_output Output tensor on device (batch * seq_len * hidden_dim)
 * @param d_input Input tensor on device (batch * seq_len * hidden_dim)
 * @param d_gate_weight Gate projection weight on device (hidden_dim * intermediate_dim)
 * @param d_up_weight Up projection weight on device (hidden_dim * intermediate_dim)
 * @param d_down_weight Down projection weight on device (intermediate_dim * hidden_dim)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden dimension
 * @param intermediate_dim Intermediate FFN dimension (typically 4 * hidden_dim)
 * @param stream CUDA stream for async execution
 */
void launchFusedGatedFFN(
    float* d_output,
    const float* d_input,
    const float* d_gate_weight,
    const float* d_up_weight,
    const float* d_down_weight,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int intermediate_dim,
    cudaStream_t stream = 0
);

#ifdef __cplusplus
} // extern "C"
#endif

} // namespace cuda
} // namespace kernels
} // namespace llm
} // namespace themis
