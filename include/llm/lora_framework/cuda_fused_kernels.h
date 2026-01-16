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

} // namespace fused
} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
