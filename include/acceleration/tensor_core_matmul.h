/**
 * @file tensor_core_matmul.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

// =============================================================================
// ThemisDB - Tensor Core FP16 / BF16 / INT8 Matrix Multiply
//
// File:    include/acceleration/tensor_core_matmul.h
// Status:  Production (CUDA path) / CPU fallback always available
//
// Public API for Tensor Core-accelerated matrix multiply (GEMM):
//   C = alpha * A × B + beta * C
//
// Precision modes:
//   FP16  — half precision  (CUDA SM 7.0+, Volta / Turing / Ampere / Hopper)
//   BF16  — bfloat16        (CUDA SM 8.0+, Ampere / Hopper)
//   FP32  — single precision (CPU fallback, always available)
//   INT8  — 8-bit integer with INT32 accumulator (CUDA SM 7.5+, Turing+)
//
// When THEMIS_ENABLE_CUDA is defined the launchers call cuBLAS cublasHgemm
// (FP16) or cublasGemmEx with CUDA_R_16BF (BF16) or CUDA_R_8I (INT8), all of
// which automatically engage Tensor Core units on supported hardware.  Without
// CUDA the calls transparently fall through to the FP32 CPU implementation.
//
// FP32 ↔ INT8 quantisation helpers:
//   quantize()   — converts a FP32 array to INT8 using symmetric per-tensor
//                  quantisation: dst[i] = clamp(round(src[i] / scale), -128, 127)
//   dequantize() — inverse: dst[i] = src[i] * scale
// =============================================================================

#include "acceleration/kernel_invocation.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cuda_bf16.h>
#endif

#include <cstddef>
#include <cstdint>

namespace themis {
namespace acceleration {
namespace tensor_core {

// =============================================================================
// CUDA kernel launchers (extern "C" linkage, defined in tensor_core_matmul.cu)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

extern "C" {

/**
 * @brief Launch FP16 Matmul Kernel.
 * @param[in] d_A Input parameter.
 * @param[in] d_B Input parameter.
 * @param[in,out] d_C Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] K Input parameter.
 * @param[in] N Input parameter.
 * @param[in] alpha Input parameter.
 * @param[in] beta Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 */
int launchFP16MatmulKernel(
    const __half* d_A,
    const __half* d_B,
    __half*       d_C,
    int           M,
    int           K,
    int           N,
    float         alpha,
    float         beta,
    cudaStream_t  stream
);

/**
 * @brief Launch BF16 Matmul Kernel.
 * @param[in] d_A Input parameter.
 * @param[in] d_B Input parameter.
 * @param[in,out] d_C Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] K Input parameter.
 * @param[in] N Input parameter.
 * @param[in] alpha Input parameter.
 * @param[in] beta Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 */
int launchBF16MatmulKernel(
    const __nv_bfloat16* d_A,
    const __nv_bfloat16* d_B,
    __nv_bfloat16*       d_C,
    int                  M,
    int                  K,
    int                  N,
    float                alpha,
    float                beta,
    cudaStream_t         stream
);

/**
 * @brief Launch FP32 Matmul Kernel.
 * @param[in] d_A Input parameter.
 * @param[in] d_B Input parameter.
 * @param[in,out] d_C Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] K Input parameter.
 * @param[in] N Input parameter.
 * @param[in] alpha Input parameter.
 * @param[in] beta Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 */
int launchFP32MatmulKernel(
    const float* d_A,
    const float* d_B,
    float*       d_C,
    int          M,
    int          K,
    int          N,
    float        alpha,
    float        beta,
    cudaStream_t stream
);

/**
 * @brief Launch INT8 Matmul Kernel.
 * @param[in] d_A Input parameter.
 * @param[in] d_B Input parameter.
 * @param[in,out] d_C Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] K Input parameter.
 * @param[in] N Input parameter.
 * @param[in] alpha Input parameter.
 * @param[in] beta Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 */
int launchINT8MatmulKernel(
    const int8_t*  d_A,
    const int8_t*  d_B,
    int32_t*       d_C,
    int            M,
    int            K,
    int            N,
    float          alpha,
    float          beta,
    cudaStream_t   stream
);

} // extern "C"

#endif // THEMIS_ENABLE_CUDA

/**
 * @brief ============================================================================= CPU fallback launcher (always available, no GPU required) =============================================================================
 * @param[in] A Input parameter.
 * @param[in] B Input parameter.
 * @param[in,out] C Input/output parameter.
 * @param[in] M Input parameter.
 * @param[in] K Input parameter.
 * @param[in] N Input parameter.
 * @param[in] alpha Input parameter.
 * @param[in] beta Input parameter.
 * @return Return value.
 */

int launchCPUMatmulKernel(
    const float* A,
    const float* B,
    float*       C,
    int          M,
    int          K,
    int          N,
    float        alpha,
    float        beta
);

/**
 * @brief ============================================================================= Unified dispatch adapter (backend-agnostic entry point) =============================================================================
 * @param[in] params Input parameter.
 * @param[in,out] opaque_stream Input/output parameter.
 * @return Return value.
 */

int dispatchMatmul(const MatrixKernelParams& params, void* opaque_stream);

/**
 * @brief ============================================================================= FP32 ↔ INT8 quantisation helpers (CPU, always available) =============================================================================
 * @param[in] src Input parameter.
 * @param[in,out] dst Input/output parameter.
 * @param[in] n Input parameter.
 * @param[in] scale Input parameter.
 */

void quantize(const float* src, int8_t* dst, size_t n, float scale);

/**
 * @brief Dequantize.
 * @param[in] src Input parameter.
 * @param[in,out] dst Input/output parameter.
 * @param[in] n Input parameter.
 * @param[in] scale Input parameter.
 */
void dequantize(const int8_t* src, float* dst, size_t n, float scale);

} // namespace tensor_core
} // namespace acceleration
} // namespace themis
