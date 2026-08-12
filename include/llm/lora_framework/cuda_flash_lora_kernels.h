/**
 * @file cuda_flash_lora_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_CUDA

#include <cuda_runtime.h>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {
namespace flash {

/**
 * @brief FlashLoRA forward kernel launcher
 * 
 * Fused tiled LoRA computation: output = (input @ B^T @ A^T) * scaling
 * 
 * Tiling strategy:
 * - Input tiles loaded into shared memory (TILE_M x TILE_K)
 * - Intermediate (input @ B^T) computed in registers
 * - Final output (intermediate @ A^T) computed and written to HBM
 * 
 * Memory bandwidth analysis:
 * Standard LoRA:
 *   - Read input: batch * seq_len * in_dim * 4 bytes
 *   - Write intermediate: batch * seq_len * rank * 4 bytes (HBM)
 *   - Read intermediate: batch * seq_len * rank * 4 bytes (HBM)
 *   - Write output: batch * seq_len * out_dim * 4 bytes
 *   Total: ~4x data movement
 * 
 * FlashLoRA:
 *   - Read input: batch * seq_len * in_dim * 4 bytes (tiled)
 *   - Write output: batch * seq_len * out_dim * 4 bytes
 *   - Intermediate in SRAM (100x faster, not counted)
 *   Total: ~2x data movement (50% reduction)
 * 
 * @param input Input tensor [batch_size, seq_len, in_dim]
 * @param B LoRA down-projection [rank, in_dim]
 * @param A LoRA up-projection [out_dim, rank]
 * @param output Output tensor [batch_size, seq_len, out_dim]
 * @param scaling Scaling factor (lora_alpha / rank)
 * @param batch_size Number of samples in batch
 * @param seq_len Sequence length
 * @param in_dim Input dimension
 * @param out_dim Output dimension
 * @param stream CUDA stream for async execution
 * 
 * @note RANK is a template parameter, not a function parameter
 */
template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_forward_kernel(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream = nullptr
);

/**
 * @brief FlashLoRA backward kernel for grad_A computation
 * 
 * Computes: grad_A = intermediate^T @ grad_output
 * where intermediate = input @ B^T
 * 
 * Uses tiling to avoid materializing intermediate in HBM
 * 
 * @param grad_output Gradient from next layer [batch_size, seq_len, out_dim]
 * @param input Cached input [batch_size, seq_len, in_dim]
 * @param B LoRA down-projection [rank, in_dim]
 * @param grad_A Output gradient [out_dim, rank]
 * @param scaling Scaling factor
 * @param batch_size Number of samples
 * @param seq_len Sequence length
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param stream CUDA stream
 */
template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_backward_A_kernel(
    const float* grad_output,
    const float* input,
    const float* B,
    float* grad_A,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream = nullptr
);

/**
 * @brief FlashLoRA backward kernel for grad_B computation
 * 
 * Computes: grad_B = input^T @ (grad_output @ A^T)
 * 
 * Uses tiling to avoid materializing (grad_output @ A^T) in HBM
 * 
 * @param grad_output Gradient from next layer [batch_size, seq_len, out_dim]
 * @param input Cached input [batch_size, seq_len, in_dim]
 * @param A LoRA up-projection [out_dim, rank]
 * @param grad_B Output gradient [rank, in_dim]
 * @param scaling Scaling factor
 * @param batch_size Number of samples
 * @param seq_len Sequence length
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param stream CUDA stream
 */
template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_backward_B_kernel(
    const float* grad_output,
    const float* input,
    const float* A,
    float* grad_B,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream = nullptr
);

/**
 * @brief FlashLoRA backward kernel for grad_input computation
 * 
 * Computes: grad_input = (grad_output @ A^T) @ B
 * 
 * Uses tiling for memory efficiency
 * 
 * @param grad_output Gradient from next layer [batch_size, seq_len, out_dim]
 * @param B LoRA down-projection [rank, in_dim]
 * @param A LoRA up-projection [out_dim, rank]
 * @param grad_input Output gradient [batch_size, seq_len, in_dim]
 * @param scaling Scaling factor
 * @param batch_size Number of samples
 * @param seq_len Sequence length
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param stream CUDA stream
 */
template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_backward_input_kernel(
    const float* grad_output,
    const float* B,
    const float* A,
    float* grad_input,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream = nullptr
);

// Explicit template instantiations for common rank values
// This allows faster compilation and smaller binary size

// Forward kernel instantiations
extern template cudaError_t launch_flash_lora_forward_kernel<128, 64, 4>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_forward_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_forward_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_forward_kernel<128, 64, 32>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_forward_kernel<128, 64, 64>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

// Backward A kernel instantiations
extern template cudaError_t launch_flash_lora_backward_A_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_backward_A_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

// Backward B kernel instantiations
extern template cudaError_t launch_flash_lora_backward_B_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_backward_B_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

// Backward input kernel instantiations
extern template cudaError_t launch_flash_lora_backward_input_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

extern template cudaError_t launch_flash_lora_backward_input_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

} // namespace flash
} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
