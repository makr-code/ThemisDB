/**
 * @file vulkan_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace lora {
namespace vulkan {

/**
 * @brief Vulkan compute shader dispatch for LoRA operations
 * 
 * This module provides Vulkan compute shader dispatching for GPU-accelerated
 * LoRA training operations. Shaders are located in:
 * - src/acceleration/vulkan/shaders/lora/matmul.comp
 * - src/acceleration/vulkan/shaders/lora/elementwise.comp
 * - src/acceleration/vulkan/shaders/lora/gradient.comp
 *
 * Error behavior:
 * - Throws std::runtime_error when backend state cannot be acquired/initialized.
 * - Throws std::invalid_argument on null pointers or invalid dimensions.
 * - Throws std::overflow_error when workload byte-size calculations overflow.
 */

/**
 * @brief Initialize Vulkan compute pipeline for LoRA operations
 * @param device_id Vulkan device ID (0 for default)
 * @return true if initialization successful
 */
bool initialize_vulkan_lora(int device_id = 0);

/**
 * @brief Cleanup Vulkan resources
 */
void cleanup_vulkan_lora();

/**
 * @brief Check if Vulkan backend is available
 */
bool is_vulkan_available();

/**
 * @brief Launch Vulkan matrix multiplication shader
 * @param A Input matrix A (device pointer)
 * @param B Input matrix B (device pointer)
 * @param C Output matrix C (device pointer)
 * @param M Rows of A
 * @param N Columns of B
 * @param K Columns of A / Rows of B
 * @param alpha Scaling factor
 */
void launch_matmul_shader(
    const float* A, const float* B, float* C,
    int M, int N, int K, float alpha = 1.0f);

/**
 * @brief Launch Vulkan element-wise addition shader
 * @param A Input array A (device pointer)
 * @param B Input array B (device pointer)
 * @param C Output array C (device pointer)
 * @param size Number of elements
 */
void launch_add_shader(const float* A, const float* B, float* C, size_t size);

/**
 * @brief Launch Vulkan element-wise multiplication shader
 * @param A Input array A (device pointer)
 * @param B Input array B (device pointer)
 * @param C Output array C (device pointer)
 * @param size Number of elements
 */
void launch_multiply_shader(const float* A, const float* B, float* C, size_t size);

/**
 * @brief Launch Vulkan scalar multiplication shader
 * @param A Input array (device pointer)
 * @param B Output array (device pointer)
 * @param scalar Scalar value
 * @param size Number of elements
 */
void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size);

/**
 * @brief Launch Vulkan transpose shader
 * @param input Input matrix (device pointer)
 * @param output Output matrix (device pointer)
 * @param rows Number of rows
 * @param cols Number of columns
 */
void launch_transpose_shader(const float* input, float* output, int rows, int cols);

/**
 * @brief Launch Vulkan LoRA gradient computation (grad_A)
 * @param h Cached activation h = input @ B (device pointer)
 * @param grad_output Gradient from next layer (device pointer)
 * @param grad_A Output gradient for A (device pointer)
 * @param M Batch size
 * @param K LoRA rank
 * @param N Output dimension
 * @param scaling LoRA scaling factor
 */
void launch_lora_grad_A_shader(
    const float* h, const float* grad_output, float* grad_A,
    int M, int K, int N, float scaling);

/**
 * @brief Launch Vulkan LoRA gradient computation (grad_B)
 * @param input Input to LoRA layer (device pointer)
 * @param grad_h Gradient w.r.t h (device pointer)
 * @param grad_B Output gradient for B (device pointer)
 * @param M Batch size
 * @param D Input dimension
 * @param K LoRA rank
 */
void launch_lora_grad_B_shader(
    const float* input, const float* grad_h, float* grad_B,
    int M, int D, int K);

/**
 * @brief Launch Vulkan embedding lookup shader
 * 
 * Looks up embeddings for given token IDs from embedding matrix.
 * Input: token_ids [batch_size, seq_len] (float tensor, will be cast to int)
 * Output: embeddings [batch_size, seq_len, hidden_dim]
 * 
 * @param output Output embeddings tensor (device pointer)
 * @param token_ids Input token IDs (device pointer, stored as floats)
 * @param embedding_weights Embedding weight matrix [vocab_size, hidden_dim] (device pointer)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden/embedding dimension
 * @param vocab_size Vocabulary size (for bounds checking)
 */
void launch_embedding_lookup_shader(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int vocab_size);

/**
 * @brief Launch Vulkan sequence mean reduction shader
 * 
 * Computes mean over sequence dimension:
 * Input: [batch_size, seq_len, hidden_dim]
 * Output: [batch_size, hidden_dim]
 * 
 * @param output Output tensor (device pointer)
 * @param input Input tensor (device pointer)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden dimension
 */
void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim);
// ============================================================================
// Fused LoRA Kernels (Phase 4: Vulkan Backend)
// ============================================================================

/**
 * @brief Vulkan fused LoRA forward pass: Y = (X @ B^T @ A^T) * scaling
 * 
 * Implements complete LoRA forward path in single compute shader:
 * - Computes intermediate h = X @ B^T in workgroup local memory
 * - Computes output = h @ A^T * scaling
 * - Reduces global memory traffic by 33-75%
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
 * 
 * Expected performance: 1.5-3x speedup vs unfused
 */
void launch_fused_lora_forward(
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
 * @brief Vulkan fused LoRA backward pass
 * 
 * Computes all gradients in single compute shader:
 * - grad_A = intermediate^T @ grad_output * scaling
 * - grad_B = input^T @ (grad_output @ A) * scaling
 * - grad_input = (grad_output @ A) @ B * scaling
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
 * 
 * Expected performance: 1.7-3x speedup vs unfused
 */
void launch_fused_lora_backward(
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

} // namespace vulkan
} // namespace lora
} // namespace themis
