/**
 * @file vulkan_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <cstdint>

namespace themis {
namespace lora {
namespace vulkan {


bool initialize_vulkan_lora(int device_id = 0);

/**
 * @brief Cleanup vulkan lora.
 */
void cleanup_vulkan_lora();

/**
 * @brief Is vulkan available.
 * @return True when the operation succeeds.
 */
bool is_vulkan_available();

void launch_matmul_shader(
    const float* A, const float* B, float* C,
    int M, int N, int K, float alpha = 1.0f);

/**
 * @brief Launch add shader.
 * @param[in] A Input parameter.
 * @param[in] B Input parameter.
 * @param[in,out] C Input/output parameter.
 * @param[in] size Input parameter.
 */
void launch_add_shader(const float* A, const float* B, float* C, size_t size);

/**
 * @brief Launch multiply shader.
 * @param[in] A Input parameter.
 * @param[in] B Input parameter.
 * @param[in,out] C Input/output parameter.
 * @param[in] size Input parameter.
 */
void launch_multiply_shader(const float* A, const float* B, float* C, size_t size);

/**
 * @brief Launch scalar multiply shader.
 * @param[in] A Input parameter.
 * @param[in,out] B Input/output parameter.
 * @param[in] scalar Input parameter.
 * @param[in] size Input parameter.
 */
void launch_scalar_multiply_shader(const float* A, float* B, float scalar, size_t size);

/**
 * @brief Launch transpose shader.
 * @param[in] input Input parameter.
 * @param[in,out] output Input/output parameter.
 * @param[in] rows Input parameter.
 * @param[in] cols Input parameter.
 */
void launch_transpose_shader(const float* input, float* output, int rows, int cols);

/**
 * @brief Launch lora grad A shader.
 * @param[in] h Input parameter.
 * @param[in] grad_output Input parameter.
 * @param[in,out] grad_A Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] K Input parameter.
 * @param[in] N Input parameter.
 * @param[in] scaling Input parameter.
 */
void launch_lora_grad_A_shader(
    const float* h, const float* grad_output, float* grad_A,
    int M, int K, int N, float scaling);

/**
 * @brief Launch lora grad B shader.
 * @param[in] input Input parameter.
 * @param[in] grad_h Input parameter.
 * @param[in,out] grad_B Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] D Input parameter.
 * @param[in] K Input parameter.
 */
void launch_lora_grad_B_shader(
    const float* input, const float* grad_h, float* grad_B,
    int M, int D, int K);

/**
 * @brief Launch embedding lookup shader.
 * @param[in,out] output Input/output parameter.
 * @param[in] token_ids Input parameter.
 * @param[in] embedding_weights Input parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] seq_len Input parameter.
 * @param[in] hidden_dim Input parameter.
 * @param[in] vocab_size Input parameter.
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
 * @brief Launch sequence mean shader.
 * @param[in,out] output Input/output parameter.
 * @param[in] input Input parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] seq_len Input parameter.
 * @param[in] hidden_dim Input parameter.
 */
void launch_sequence_mean_shader(
    float* output,
    const float* input,
    int batch_size,
    int seq_len,
    int hidden_dim);
/**
 * @brief ============================================================================ Fused LoRA Kernels (Phase 4: Vulkan Backend) ============================================================================
 * @param[in] input Input parameter.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @param[in,out] output Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] in_dim Input parameter.
 * @param[in] rank Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] scaling Input parameter.
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
 * @brief Launch fused lora backward.
 * @param[in] input Input parameter.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @param[in] grad_output Input parameter.
 * @param[in,out] grad_A Input/output parameter.
 * @param[in,out] grad_B Input/output parameter.
 * @param[in,out] grad_input Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] in_dim Input parameter.
 * @param[in] rank Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] scaling Input parameter.
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
