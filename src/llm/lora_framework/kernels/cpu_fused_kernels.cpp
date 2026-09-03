/**
 * @file cpu_fused_kernels.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=8, H=19, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "llm/lora_framework/cpu_fused_kernels.h"
#include <cmath>
#include <algorithm>
#include <vector>

namespace themis {
namespace llm {
namespace lora {
namespace cpu {
namespace fused {

/*
 * @brief CPU fused LoRA forward pass: Y = (X @ B^T @ A^T) * scaling
 * 
 * This is the reference implementation that computes the entire LoRA path
 * in a single function, keeping the intermediate result in memory.
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
) {
    // Allocate intermediate result: h = input @ B^T
    std::vector<float> intermediate(batch_size * rank);
    
    // Step 1: Compute h = input @ B^T
    // h[batch_idx, r] = sum_i(input[batch_idx, i] * B[i, r])
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float sum = 0.0f;
            for (size_t i = 0; i < in_dim; ++i) {
                sum += input[b * in_dim + i] * B[i * rank + r];
            }
            intermediate[b * rank + r] = sum;
        }
    }
    
    // Step 2: Compute output = h @ A^T * scaling
    // output[batch_idx, o] = sum_r(intermediate[batch_idx, r] * A[r, o]) * scaling
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t o = 0; o < out_dim; ++o) {
            float sum = 0.0f;
            for (size_t r = 0; r < rank; ++r) {
                sum += intermediate[b * rank + r] * A[r * out_dim + o];
            }
            output[b * out_dim + o] = sum * scaling;
        }
    }
}

/*
 * @brief CPU fused LoRA backward pass
 * 
 * Computes all gradients in a single function:
 * - grad_input = (grad_output @ A) @ B * scaling
 * - grad_A = intermediate^T @ grad_output * scaling
 * - grad_B = input^T @ (grad_output @ A) * scaling
 * 
 * @param input Input tensor [batch_size, in_dim] (from forward)
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
) {
    // Recompute intermediate from forward pass: h = input @ B^T
    std::vector<float> intermediate(batch_size * rank);
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float sum = 0.0f;
            for (size_t i = 0; i < in_dim; ++i) {
                sum += input[b * in_dim + i] * B[i * rank + r];
            }
            intermediate[b * rank + r] = sum;
        }
    }
    
    // Initialize gradients to zero
    std::fill(grad_A, grad_A + rank * out_dim, 0.0f);
    std::fill(grad_B, grad_B + in_dim * rank, 0.0f);
    std::fill(grad_input, grad_input + batch_size * in_dim, 0.0f);
    
    // Compute grad_A = intermediate^T @ (grad_output * scaling)
    // grad_A[r, o] = sum_b(intermediate[b, r] * grad_output[b, o]) * scaling
    for (size_t r = 0; r < rank; ++r) {
        for (size_t o = 0; o < out_dim; ++o) {
            float sum = 0.0f;
            for (size_t b = 0; b < batch_size; ++b) {
                sum += intermediate[b * rank + r] * grad_output[b * out_dim + o];
            }
            grad_A[r * out_dim + o] = sum * scaling;
        }
    }
    
    // Compute intermediate gradient: grad_h = grad_output @ A^T
    std::vector<float> grad_intermediate(batch_size * rank);
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float sum = 0.0f;
            for (size_t o = 0; o < out_dim; ++o) {
                sum += grad_output[b * out_dim + o] * A[r * out_dim + o];
            }
            grad_intermediate[b * rank + r] = sum * scaling;
        }
    }
    
    // Compute grad_B = input^T @ grad_intermediate
    // grad_B[i, r] = sum_b(input[b, i] * grad_intermediate[b, r])
    for (size_t i = 0; i < in_dim; ++i) {
        for (size_t r = 0; r < rank; ++r) {
            float sum = 0.0f;
            for (size_t b = 0; b < batch_size; ++b) {
                sum += input[b * in_dim + i] * grad_intermediate[b * rank + r];
            }
            grad_B[i * rank + r] = sum;
        }
    }
    
    // Compute grad_input = grad_intermediate @ B^T
    // grad_input[b, i] = sum_r(grad_intermediate[b, r] * B[i, r])
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t i = 0; i < in_dim; ++i) {
            float sum = 0.0f;
            for (size_t r = 0; r < rank; ++r) {
                sum += grad_intermediate[b * rank + r] * B[i * rank + r];
            }
            grad_input[b * in_dim + i] = sum;
        }
    }
}

/**
 * @brief CPU fused LoRA forward pass with OpenMP parallelization
 * 
 * Parallelized version using OpenMP for better CPU performance
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
) {
    // Allocate intermediate result
    std::vector<float> intermediate(batch_size * rank);
    
    // Step 1: Compute h = input @ B^T (parallelized over batch)
    #ifdef _OPENMP
    #pragma omp parallel for if(batch_size > 4)
    #endif
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t r = 0; r < rank; ++r) {
            float sum = 0.0f;
            for (size_t i = 0; i < in_dim; ++i) {
                sum += input[b * in_dim + i] * B[i * rank + r];
            }
            intermediate[b * rank + r] = sum;
        }
    }
    
    // Step 2: Compute output = h @ A^T * scaling (parallelized over batch)
    #ifdef _OPENMP
    #pragma omp parallel for if(batch_size > 4)
    #endif
    for (size_t b = 0; b < batch_size; ++b) {
        for (size_t o = 0; o < out_dim; ++o) {
            float sum = 0.0f;
            for (size_t r = 0; r < rank; ++r) {
                sum += intermediate[b * rank + r] * A[r * out_dim + o];
            }
            output[b * out_dim + o] = sum * scaling;
        }
    }
}

} // namespace fused
} // namespace cpu
} // namespace lora
} // namespace llm
} // namespace themis
