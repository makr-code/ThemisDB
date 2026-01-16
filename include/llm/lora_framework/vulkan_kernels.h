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

} // namespace vulkan
} // namespace lora
} // namespace themis
