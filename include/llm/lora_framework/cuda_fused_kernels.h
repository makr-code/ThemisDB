/**
 * @file cuda_fused_kernels.h
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
namespace fused {

/**
 * @brief Fused LoRA forward pass kernel
 * 
 * Computes output = (input @ B) @ A * scaling in a single kernel
 * Uses shared memory for intermediate results to avoid global memory writes
 * 
 * @param input Input tensor (batch_size, in_dim)
 * @param B LoRA B matrix (in_dim, rank)
 * @param A LoRA A matrix (rank, out_dim)
 * @param output Output tensor (batch_size, out_dim)
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 * @param stream CUDA stream for async execution
 * 
 * Expected speedup: 1.5-1.8x vs unfused
 * Memory bandwidth reduction: ~66%
 */
cudaError_t launch_fused_lora_forward(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    cudaStream_t stream = nullptr
);

/**
 * @brief Fused LoRA backward pass kernel
 * 
 * Computes all gradients in a single kernel:
 * - grad_A = h^T @ grad_output * scaling
 * - grad_B = input^T @ (grad_output @ A^T * scaling)
 * - grad_input = (grad_output @ A^T) @ B^T * scaling
 * 
 * Uses shared memory for intermediate computations
 * Avoids multiple global memory passes
 * 
 * @param input Cached input from forward pass (batch_size, in_dim)
 * @param B LoRA B matrix (in_dim, rank)
 * @param A LoRA A matrix (rank, out_dim)
 * @param grad_output Gradient from next layer (batch_size, out_dim)
 * @param grad_A Output gradient for A (rank, out_dim)
 * @param grad_B Output gradient for B (in_dim, rank)
 * @param grad_input Output gradient for input (batch_size, in_dim)
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 * @param stream CUDA stream for async execution
 * 
 * Expected speedup: 1.7-2.0x vs unfused
 * Memory bandwidth reduction: ~75%
 */
cudaError_t launch_fused_lora_backward(
    const float* input,
    const float* B,
    const float* A,
    const float* grad_output,
    float* grad_A,
    float* grad_B,
    float* grad_input,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    cudaStream_t stream = nullptr
);

/**
 * @brief Fused SGD optimizer update kernel
 * 
 * Fuses gradient accumulation + weight decay + momentum + update into single kernel
 * Computes: p = p - lr * (g + weight_decay * p + momentum * v)
 * 
 * @param params Parameters to update (device pointer)
 * @param grads Gradients (device pointer)
 * @param momentum_buffer Momentum buffer (device pointer, can be nullptr if momentum=0)
 * @param size Number of elements
 * @param learning_rate Learning rate
 * @param momentum Momentum factor
 * @param weight_decay Weight decay factor
 * @param stream CUDA stream for async execution
 * 
 * Expected speedup: 1.3-1.5x vs unfused
 */
cudaError_t launch_fused_sgd_step(
    float* params,
    const float* grads,
    float* momentum_buffer,
    size_t size,
    float learning_rate,
    float momentum,
    float weight_decay,
    cudaStream_t stream = nullptr
);

/**
 * @brief Optimized fused LoRA forward pass with Phase 2 improvements
 * 
 * Phase 2 Optimizations (Issue #36):
 * - Vectorized memory access using float4 (4x bandwidth improvement)
 * - Improved register blocking for better cache reuse
 * - Better memory coalescing patterns
 * 
 * @param input Input tensor (batch_size, in_dim)
 * @param B LoRA B matrix (in_dim, rank)
 * @param A LoRA A matrix (rank, out_dim)
 * @param output Output tensor (batch_size, out_dim)
 * @param batch_size Batch size
 * @param in_dim Input dimension (should be multiple of 4 for best performance)
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 * @param stream CUDA stream for async execution
 * 
 * Expected speedup: 10-20% additional improvement over base fused kernel
 * Total speedup vs unfused: 1.7-2.2x (forward pass)
 */
cudaError_t launch_fused_lora_forward_optimized(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    cudaStream_t stream = nullptr
);

/**
 * @brief Warp-optimized fused LoRA forward with shuffle operations
 * 
 * Phase 2 Advanced Optimizations:
 * - Warp shuffle operations (__shfl_down_sync) for efficient reduction
 * - Multi-level tiling (warp tiles + thread tiles)
 * - Bank conflict avoidance with padding
 * - Cooperative groups for better synchronization
 * 
 * @param input Input tensor (batch_size, in_dim)
 * @param B LoRA B matrix (in_dim, rank)
 * @param A LoRA A matrix (rank, out_dim)
 * @param output Output tensor (batch_size, out_dim)
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 * @param stream CUDA stream for async execution
 * 
 * Expected speedup: 5-10% additional improvement over vectorized version
 * Total speedup vs unfused: 1.8-2.4x (forward pass)
 */
cudaError_t launch_fused_lora_forward_warp_optimized(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    cudaStream_t stream = nullptr
);

/**
 * @brief Batched LoRA forward for multiple adapters (Phase 3)
 * 
 * Process multiple LoRA adapters in single kernel call:
 * - Different ranks per adapter supported
 * - Shared input tensor across all adapters
 * - Separate weights and outputs per adapter
 * - Amortize kernel launch overhead
 * 
 * Use case: Multi-tenant serving with different fine-tuned models
 * 
 * @param input Shared input tensor (batch_size, in_dim)
 * @param B_ptrs Array of B matrix pointers (one per adapter)
 * @param A_ptrs Array of A matrix pointers (one per adapter)
 * @param output_ptrs Array of output tensor pointers (one per adapter)
 * @param ranks Array of ranks (one per adapter)
 * @param scalings Array of scaling factors (one per adapter)
 * @param num_adapters Number of LoRA adapters to process
 * @param batch_size Batch size (shared across adapters)
 * @param in_dim Input dimension (shared across adapters)
 * @param out_dim Output dimension (shared across adapters)
 * @param stream CUDA stream for async execution
 * 
 * Expected benefit: Reduce overhead when serving multiple adapters concurrently
 */
cudaError_t launch_batched_lora_forward(
    const float* input,
    const float** B_ptrs,
    const float** A_ptrs,
    float** output_ptrs,
    const int* ranks,
    const float* scalings,
    int num_adapters,
    size_t batch_size,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream = nullptr
);

} // namespace fused
} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
