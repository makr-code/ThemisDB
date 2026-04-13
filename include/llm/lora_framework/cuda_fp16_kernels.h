/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cuda_fp16_kernels.h                                ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:16:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     183                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef THEMIS_ENABLE_CUDA

#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

/**
 * @brief Convert FP32 tensor to FP16 on GPU
 * 
 * @param input Input FP32 tensor (device pointer)
 * @param output Output FP16 tensor (device pointer)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp32_to_fp16_kernel(
    const float* input,
    __half* output,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief Convert FP16 tensor to FP32 on GPU
 * 
 * @param input Input FP16 tensor (device pointer)
 * @param output Output FP32 tensor (device pointer)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp16_to_fp32_kernel(
    const __half* input,
    float* output,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief FP16 matrix multiplication using Tensor Cores
 * 
 * Computes C = alpha * (A @ B) where:
 * - A: (M, K) in FP16
 * - B: (K, N) in FP16
 * - C: (M, N) in FP16
 * 
 * Uses cuBLAS with Tensor Core acceleration on Ampere+ GPUs
 * 
 * @param A Input matrix A (device pointer, FP16)
 * @param B Input matrix B (device pointer, FP16)
 * @param C Output matrix C (device pointer, FP16)
 * @param M Number of rows in A and C
 * @param K Number of columns in A, rows in B
 * @param N Number of columns in B and C
 * @param alpha Scaling factor
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp16_matmul_kernel(
    const __half* A,
    const __half* B,
    __half* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream = nullptr
);

/**
 * @brief FP16 element-wise addition
 * 
 * Computes C = A + B (element-wise) in FP16
 * 
 * @param A Input array A (device pointer, FP16)
 * @param B Input array B (device pointer, FP16)
 * @param C Output array C (device pointer, FP16)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp16_add_kernel(
    const __half* A,
    const __half* B,
    __half* C,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief FP16 element-wise multiplication
 * 
 * Computes C = A * B (element-wise) in FP16
 * 
 * @param A Input array A (device pointer, FP16)
 * @param B Input array B (device pointer, FP16)
 * @param C Output array C (device pointer, FP16)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp16_multiply_kernel(
    const __half* A,
    const __half* B,
    __half* C,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief FP16 scalar multiplication
 * 
 * Computes C = A * scalar in FP16
 * 
 * @param A Input array A (device pointer, FP16)
 * @param C Output array C (device pointer, FP16)
 * @param scalar Scalar value (FP32, converted to FP16)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp16_scalar_multiply_kernel(
    const __half* A,
    __half* C,
    float scalar,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief FP16 matrix transpose
 * 
 * @param A Input matrix (device pointer, FP16)
 * @param C Output matrix (device pointer, FP16)
 * @param rows Number of rows in A
 * @param cols Number of columns in A
 * @param stream CUDA stream for async execution
 * @return CUDA error code
 */
cudaError_t launch_fp16_transpose_kernel(
    const __half* A,
    __half* C,
    size_t rows,
    size_t cols,
    cudaStream_t stream = nullptr
);

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
