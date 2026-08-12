/**
 * @file tensor_core_matmul.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// =============================================================================
// ThemisDB - Tensor Core matmul CPU fallback + unified dispatcher
//
// File:    src/acceleration/tensor_core_matmul.cpp
// =============================================================================

#include "acceleration/tensor_core_matmul.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_bf16.h>
#include <cuda_fp16.h>
#endif

namespace themis {
namespace acceleration {
namespace tensor_core {

// =============================================================================
// CPU fallback — naive triple-loop FP32 GEMM (always compiled)
// =============================================================================

int launchCPUMatmulKernel(const float *A, const float *B, float *C, int M, int K, int N, float alpha, float beta) {
    if (!A || !B || !C || M <= 0 || K <= 0 || N <= 0) {
        return 1;
    }

    // Scale existing C by beta first
    if (beta == 0.0f) {
        std::memset(C, 0, static_cast<size_t>(M) * static_cast<size_t>(N) * sizeof(float));
    } else if (beta != 1.0f) {
        const size_t total = static_cast<size_t>(M) * static_cast<size_t>(N);
        for (size_t i = 0; i < total; ++i) {
            C[i] *= beta;
        }
    }

    // Compute alpha * A × B and accumulate into C
    for (int m = 0; m < M; ++m) {
        for (int n = 0; n < N; ++n) {
            float acc = 0.0f;
            for (int k = 0; k < K; ++k) {
                acc += A[m * K + k] * B[k * N + n];
            }
            C[m * N + n] += alpha * acc;
        }
    }
    return 0;
}

// =============================================================================
// Unified dispatcher
// =============================================================================

int dispatchMatmul(const MatrixKernelParams &params, void *opaque_stream) {
#ifndef THEMIS_ENABLE_CUDA
    (void)opaque_stream;
#endif
    if (!params.A || !params.B || !params.C) {
        return 1;
    }
    if (params.M == 0 || params.K == 0 || params.N == 0) {
        return 1;
    }

    const int M = static_cast<int>(params.M);
    const int K = static_cast<int>(params.K);
    const int N = static_cast<int>(params.N);

#ifdef THEMIS_ENABLE_CUDA
    cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    switch (params.precision) {
        case MatrixPrecision::FP16:
            return launchFP16MatmulKernel(static_cast<const __half *>(params.A), static_cast<const __half *>(params.B),
                                          static_cast<__half *>(params.C), M, K, N, params.alpha, params.beta, stream);
        case MatrixPrecision::BF16:
            return launchBF16MatmulKernel(
                static_cast<const __nv_bfloat16 *>(params.A), static_cast<const __nv_bfloat16 *>(params.B),
                static_cast<__nv_bfloat16 *>(params.C), M, K, N, params.alpha, params.beta, stream);
        case MatrixPrecision::INT8:
            return launchINT8MatmulKernel(static_cast<const int8_t *>(params.A), static_cast<const int8_t *>(params.B),
                                          static_cast<int32_t *>(params.C), M, K, N, params.alpha, params.beta, stream);
        case MatrixPrecision::FP32:
        default:
            return launchFP32MatmulKernel(static_cast<const float *>(params.A), static_cast<const float *>(params.B),
                                          static_cast<float *>(params.C), M, K, N, params.alpha, params.beta, stream);
    }
#else
    // No CUDA: always use the CPU FP32 path regardless of requested precision
    return launchCPUMatmulKernel(static_cast<const float *>(params.A), static_cast<const float *>(params.B),
                                 static_cast<float *>(params.C), M, K, N, params.alpha, params.beta);
#endif
}

// =============================================================================
// FP32 ↔ INT8 quantisation helpers
// =============================================================================

void quantize(const float *src, int8_t *dst, size_t n, float scale) {
    if (!src || !dst || n == 0 || scale <= 0.0f) {
        return;
    }
    const float inv_scale = 1.0f / scale;
    for (size_t i = 0; i < n; ++i) {
        float val = std::round(src[i] * inv_scale);
        val       = std::max(val, -128.0f);
        val       = std::min(val, 127.0f);
        dst[i]    = static_cast<int8_t>(val);
    }
}

void dequantize(const int8_t *src, float *dst, size_t n, float scale) {
    if (!src || !dst || n == 0) {
        return;
    }
    for (size_t i = 0; i < n; ++i) {
        dst[i] = static_cast<float>(src[i]) * scale;
    }
}

} // namespace tensor_core
} // namespace acceleration
} // namespace themis
