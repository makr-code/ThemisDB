/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cpu_fused_kernels.h                                ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-02-21 12:08:43                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     126                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3b2027fce  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file cpu_fused_kernels.h
 * @brief CPU reference implementation of fused LoRA kernels
 */

#ifndef THEMIS_LLM_LORA_CPU_FUSED_KERNELS_H
#define THEMIS_LLM_LORA_CPU_FUSED_KERNELS_H

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

#endif // THEMIS_LLM_LORA_CPU_FUSED_KERNELS_H
