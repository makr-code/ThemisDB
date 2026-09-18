/**
 * @file cpu_fused_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace cpu {
namespace fused {

/**
 * @brief Cpu fused lora forward.
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
 * @brief Cpu fused lora backward.
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
 * @brief Cpu fused lora forward parallel.
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
