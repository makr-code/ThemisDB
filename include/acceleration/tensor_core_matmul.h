#pragma once

// =============================================================================
// ThemisDB - Tensor Core FP16 / BF16 Matrix Multiply
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
//
// When THEMIS_ENABLE_CUDA is defined the launchers call cuBLAS cublasHgemm
// (FP16) or cublasGemmEx with CUDA_R_16BF (BF16), both of which automatically
// engage Tensor Core units on supported hardware.  Without CUDA the calls
// transparently fall through to the FP32 CPU implementation.
// =============================================================================

#include "acceleration/kernel_invocation.h"

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <cuda_bf16.h>
#endif

#include <cstddef>

namespace themis {
namespace acceleration {
namespace tensor_core {

// =============================================================================
// CUDA kernel launchers (extern "C" linkage, defined in tensor_core_matmul.cu)
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

extern "C" {

/// FP16 GEMM: C = alpha * A × B + beta * C using cuBLAS Hgemm.
/// A [M×K], B [K×N], C [M×N] — all device pointers, row-major storage.
/// Returns 0 on success, cuBLAS status code on failure.
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

/// BF16 GEMM: C = alpha * A × B + beta * C using cuBLAS GemmEx.
/// A [M×K], B [K×N], C [M×N] — all device pointers, row-major storage.
/// Returns 0 on success, cuBLAS status code on failure.
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

/// FP32 GEMM: C = alpha * A × B + beta * C using cuBLAS Sgemm.
/// A [M×K], B [K×N], C [M×N] — all device pointers, row-major storage.
/// Returns 0 on success, cuBLAS status code on failure.
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

} // extern "C"

#endif // THEMIS_ENABLE_CUDA

// =============================================================================
// CPU fallback launcher (always available, no GPU required)
// =============================================================================

/// FP32 GEMM on CPU: C = alpha * A × B + beta * C.
/// A [M×K], B [K×N], C [M×N] — all host pointers, row-major storage.
/// Returns 0 on success.
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

// =============================================================================
// Unified dispatch adapter (backend-agnostic entry point)
// =============================================================================

/// General matmul dispatcher conforming to MatrixKernelFn.
/// Routes to the CUDA FP16/BF16/FP32 launcher or the CPU fallback depending on
/// params.precision and whether THEMIS_ENABLE_CUDA is defined.
/// Returns 0 on success, non-zero on failure.
int dispatchMatmul(const MatrixKernelParams& params, void* opaque_stream);

} // namespace tensor_core
} // namespace acceleration
} // namespace themis
