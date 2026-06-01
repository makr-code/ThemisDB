/*
 * ThemisDB | File: cpu_fused_kernels.h | Version: 0.0.47 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 105
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #607 Complete implementation of ... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file cpu_fused_kernels.h
 * @brief CPU reference implementation of fused LoRA kernels
 */

#pragma once

#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace cpu {
namespace fused {

/**
 * @brief CPU fused LoRA forward pass: Y = (X @ B^T @ A^T) * scaling
 * 
 * Reference implementation for numerical correctness validation
 * 
 * @param input Input tensor [batch_size, in_dim]
 * @param B LoRA B matrix [in_dim, rank]
 * @param A LoRA A matrix [rank, out_dim]
 * @param output Output tensor [batch_size, out_dim]
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 */
void cpu_fused_lora_forward(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
);

/**
 * @brief CPU fused LoRA backward pass
 * 
 * Computes all gradients in a single function
 * 
 * @param input Input tensor [batch_size, in_dim]
 * @param B LoRA B matrix [in_dim, rank]
 * @param A LoRA A matrix [rank, out_dim]
 * @param grad_output Gradient w.r.t output [batch_size, out_dim]
 * @param grad_A Gradient w.r.t A [rank, out_dim]
 * @param grad_B Gradient w.r.t B [in_dim, rank]
 * @param grad_input Gradient w.r.t input [batch_size, in_dim]
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 */
void cpu_fused_lora_backward(
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
    float scaling
);

/**
 * @brief CPU fused LoRA forward pass with OpenMP parallelization
 * 
 * Parallelized version for better CPU performance
 */
void cpu_fused_lora_forward_parallel(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
);

} // namespace fused
} // namespace cpu
} // namespace lora
} // namespace llm
} // namespace themis
