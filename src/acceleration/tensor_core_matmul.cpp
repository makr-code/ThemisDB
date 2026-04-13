/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tensor_core_matmul.cpp                             ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-04-13 04:23:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     153                                            ║
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

// =============================================================================
// ThemisDB - Tensor Core matmul CPU fallback + unified dispatcher
//
// File:    src/acceleration/tensor_core_matmul.cpp
// =============================================================================

#include "acceleration/tensor_core_matmul.h"
#include <cstring>
#include <iostream>

#ifdef THEMIS_ENABLE_CUDA
#include <cuda_fp16.h>
#include <cuda_bf16.h>
#endif

namespace themis {
namespace acceleration {
namespace tensor_core {

// =============================================================================
// CPU fallback — naive triple-loop FP32 GEMM (always compiled)
// =============================================================================

int launchCPUMatmulKernel(
    const float* A,
    const float* B,
    float*       C,
    int          M,
    int          K,
    int          N,
    float        alpha,
    float        beta
)
{
    if (!A || !B || !C || M <= 0 || K <= 0 || N <= 0) return 1;

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

int dispatchMatmul(const MatrixKernelParams& params, void* opaque_stream)
{
    if (!params.A || !params.B || !params.C) return 1;
    if (params.M == 0 || params.K == 0 || params.N == 0) return 1;

    const int M = static_cast<int>(params.M);
    const int K = static_cast<int>(params.K);
    const int N = static_cast<int>(params.N);

#ifdef THEMIS_ENABLE_CUDA
    cudaStream_t stream = static_cast<cudaStream_t>(opaque_stream);

    switch (params.precision) {
        case MatrixPrecision::FP16:
            return launchFP16MatmulKernel(
                static_cast<const __half*>(params.A),
                static_cast<const __half*>(params.B),
                static_cast<__half*>(params.C),
                M, K, N,
                params.alpha, params.beta,
                stream
            );
        case MatrixPrecision::BF16:
            return launchBF16MatmulKernel(
                static_cast<const __nv_bfloat16*>(params.A),
                static_cast<const __nv_bfloat16*>(params.B),
                static_cast<__nv_bfloat16*>(params.C),
                M, K, N,
                params.alpha, params.beta,
                stream
            );
        case MatrixPrecision::INT8:
            return launchINT8MatmulKernel(
                static_cast<const int8_t*>(params.A),
                static_cast<const int8_t*>(params.B),
                static_cast<int32_t*>(params.C),
                M, K, N,
                params.alpha, params.beta,
                stream
            );
        case MatrixPrecision::FP32:
        default:
            return launchFP32MatmulKernel(
                static_cast<const float*>(params.A),
                static_cast<const float*>(params.B),
                static_cast<float*>(params.C),
                M, K, N,
                params.alpha, params.beta,
                stream
            );
    }
#else
    // No CUDA: always use the CPU FP32 path regardless of requested precision
    return launchCPUMatmulKernel(
        static_cast<const float*>(params.A),
        static_cast<const float*>(params.B),
        static_cast<float*>(params.C),
        M, K, N,
        params.alpha, params.beta
    );
#endif
}

} // namespace tensor_core
} // namespace acceleration
} // namespace themis
