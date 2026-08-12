/**
 * @file cuda_bf16_kernels.h
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
#include <cuda_bf16.h>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

/**
 * @brief Convert FP32 tensor to BF16 on GPU
 * 
 * @param input Input FP32 tensor (device pointer)
 * @param output Output BF16 tensor (device pointer)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp32_to_bf16_kernel(
    const float* input,
    __nv_bfloat16* output,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief Convert BF16 tensor to FP32 on GPU
 * 
 * @param input Input BF16 tensor (device pointer)
 * @param output Output FP32 tensor (device pointer)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_bf16_to_fp32_kernel(
    const __nv_bfloat16* input,
    float* output,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief BF16 matrix multiplication using Tensor Cores
 * 
 * Computes C = alpha * (A @ B) where:
 * - A: (M, K) in BF16
 * - B: (K, N) in BF16
 * - C: (M, N) in BF16
 * 
 * Uses cuBLAS with Tensor Core acceleration on Ampere+ GPUs
 * 
 * @param A Input matrix A (device pointer, BF16)
 * @param B Input matrix B (device pointer, BF16)
 * @param C Output matrix C (device pointer, BF16)
 * @param M Number of rows in A and C
 * @param K Number of columns in A, rows in B
 * @param N Number of columns in B and C
 * @param alpha Scaling factor
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_bf16_matmul_kernel(
    const __nv_bfloat16* A,
    const __nv_bfloat16* B,
    __nv_bfloat16* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream = nullptr
);

/**
 * @brief BF16 element-wise addition
 * 
 * Computes C = A + B (element-wise) in BF16
 * 
 * @param A Input array A (device pointer, BF16)
 * @param B Input array B (device pointer, BF16)
 * @param C Output array C (device pointer, BF16)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_bf16_add_kernel(
    const __nv_bfloat16* A,
    const __nv_bfloat16* B,
    __nv_bfloat16* C,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief BF16 element-wise multiplication
 * 
 * Computes C = A * B (element-wise) in BF16
 * 
 * @param A Input array A (device pointer, BF16)
 * @param B Input array B (device pointer, BF16)
 * @param C Output array C (device pointer, BF16)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_bf16_multiply_kernel(
    const __nv_bfloat16* A,
    const __nv_bfloat16* B,
    __nv_bfloat16* C,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief BF16 scalar multiplication
 * 
 * Computes C = A * scalar in BF16
 * 
 * @param A Input array A (device pointer, BF16)
 * @param C Output array C (device pointer, BF16)
 * @param scalar Scalar value (FP32, converted to BF16)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_bf16_scalar_multiply_kernel(
    const __nv_bfloat16* A,
    __nv_bfloat16* C,
    float scalar,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief BF16 matrix transpose
 * 
 * @param A Input matrix (device pointer, BF16)
 * @param C Output matrix (device pointer, BF16)
 * @param rows Number of rows in A
 * @param cols Number of columns in A
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_bf16_transpose_kernel(
    const __nv_bfloat16* A,
    __nv_bfloat16* C,
    size_t rows,
    size_t cols,
    cudaStream_t stream = nullptr
);

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
