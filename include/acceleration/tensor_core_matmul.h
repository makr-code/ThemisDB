/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_core_matmul.h                               ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:13:08                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     166                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • e627c556bd  2026-03-15  feat(acceleration): BackendRegistry thread-safety, VLLMRe... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 57747c2d64  2026-02-23  feat(acceleration): Tensor Core FP16/BF16 matrix ops via ... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/// INT8 GEMM using cuBLAS GemmEx: INT8 inputs × INT8 weights → INT32 accumulator.
/// Requires compute capability SM 7.5+ (Turing or later).
/// d_A [M×K] int8_t, d_B [K×N] int8_t, d_C [M×N] int32_t — all device pointers.
/// alpha and beta are applied to the INT32 output (cast to float for scaling).
/// Returns 0 on success, cuBLAS status code on failure, or 1 when the device
/// compute capability is < 7.5 (INT8 Tensor Core not supported).
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
/// Routes to the CUDA FP16/BF16/INT8/FP32 launcher or the CPU fallback depending on
/// params.precision and whether THEMIS_ENABLE_CUDA is defined.
/// Returns 0 on success, non-zero on failure.
int dispatchMatmul(const MatrixKernelParams& params, void* opaque_stream);

// =============================================================================
// FP32 ↔ INT8 quantisation helpers (CPU, always available)
// =============================================================================

/// Quantise a FP32 array to INT8 using symmetric per-tensor quantisation.
///
/// Each element is converted as:
///   dst[i] = clamp(round(src[i] / scale), -128, 127)
///
/// @param src   Input array of @p n float values (host pointer).
/// @param dst   Output array of @p n int8_t values (host pointer).
/// @param n     Number of elements to convert.
/// @param scale Quantisation scale factor (must be > 0).  A typical value is
///              max(|src|) / 127.  Passing scale <= 0 has no effect and leaves
///              @p dst unchanged.
void quantize(const float* src, int8_t* dst, size_t n, float scale);

/// Dequantise an INT8 array back to FP32 using the same symmetric scale factor.
///
/// Each element is converted as:
///   dst[i] = src[i] * scale
///
/// @param src   Input array of @p n int8_t values (host pointer).
/// @param dst   Output array of @p n float values (host pointer).
/// @param n     Number of elements to convert.
/// @param scale Quantisation scale factor used during the forward quantize() call.
///              Must match the scale used in quantize() to recover the original values.
void dequantize(const int8_t* src, float* dst, size_t n, float scale);

} // namespace tensor_core
} // namespace acceleration
} // namespace themis
