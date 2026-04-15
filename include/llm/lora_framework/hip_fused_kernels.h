/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            hip_fused_kernels.h                                ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:11:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     181                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef THEMIS_ENABLE_HIP

#include <hip/hip_runtime.h>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace hip {
namespace fused {

/**
 * @brief Fused LoRA forward pass kernel (HIP)
 * 
 * Computes output = (input @ B) @ A * scaling in a single kernel
 * Uses shared memory for intermediate results to avoid global memory writes
 * Optimized for AMD GPU architecture (RDNA2/RDNA3)
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
 * @param stream HIP stream for async execution
 * 
 * Expected speedup: 1.5-1.8x vs unfused
 * Memory bandwidth reduction: ~66%
 */
hipError_t launch_fused_lora_forward(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    hipStream_t stream = nullptr
);

/**
 * @brief Fused LoRA backward pass kernel (HIP)
 * 
 * Computes all gradients in a single kernel:
 * - grad_A = h^T @ grad_output * scaling
 * - grad_B = input^T @ (grad_output @ A^T * scaling)
 * - grad_input = (grad_output @ A^T) @ B^T * scaling
 * 
 * Uses shared memory for intermediate computations
 * Avoids multiple global memory passes
 * Optimized for AMD GPU architecture
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
 * @param stream HIP stream for async execution
 * 
 * Expected speedup: 1.7-2.0x vs unfused
 * Memory bandwidth reduction: ~75%
 */
hipError_t launch_fused_lora_backward(
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
    hipStream_t stream = nullptr
);

/**
 * @brief Fused SGD optimizer update kernel (HIP)
 * 
 * Fuses gradient accumulation + weight decay + momentum + update into single kernel
 * Computes: p = p - lr * (g + weight_decay * p + momentum * v)
 * Optimized for AMD GPU architecture
 * 
 * @param params Parameters to update (device pointer)
 * @param grads Gradients (device pointer)
 * @param momentum_buffer Momentum buffer (device pointer, can be nullptr if momentum=0)
 * @param size Number of elements
 * @param learning_rate Learning rate
 * @param momentum Momentum factor
 * @param weight_decay Weight decay factor
 * @param stream HIP stream for async execution
 * 
 * Expected speedup: 1.3-1.5x vs unfused
 */
hipError_t launch_fused_sgd_step(
    float* params,
    const float* grads,
    float* momentum_buffer,
    size_t size,
    float learning_rate,
    float momentum,
    float weight_decay,
    hipStream_t stream = nullptr
);

/**
 * @brief Fused MSE loss and gradient computation kernel
 * 
 * Computes both MSE loss and gradient in a single kernel pass:
 * - Loss: sum((predictions - targets)^2) / n
 * - Gradient: (2/n) * (predictions - targets)
 * 
 * Saves memory bandwidth by reading predictions/targets only once.
 * More efficient than calling separate loss and gradient kernels.
 * 
 * @param grad_output Output gradient tensor (device pointer, size = n)
 * @param partial_loss Partial loss sums (device pointer, size = num_blocks)
 * @param predictions Predictions tensor (device pointer, size = n)
 * @param targets Target tensor (device pointer, size = n)
 * @param n Number of elements
 * @param num_blocks Number of blocks for reduction
 * @param stream HIP stream for async execution
 * 
 * Expected speedup: 1.3-1.5x vs separate loss+gradient kernels
 * Memory bandwidth reduction: ~50% (single read pass instead of two)
 */
hipError_t launch_fused_mse_loss_gradient(
    float* grad_output,
    float* partial_loss,
    const float* predictions,
    const float* targets,
    int n,
    int num_blocks,
    hipStream_t stream = nullptr
);

} // namespace fused
} // namespace hip
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_HIP
