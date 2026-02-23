// =============================================================================
// ThemisDB - Tensor Core FP16 / BF16 Matrix Multiply (CUDA)
//
// File:    src/acceleration/cuda/tensor_core_matmul.cu
// Status:  Production (CUDA) / CPU fallback
//
// Implements the launchers declared in tensor_core_matmul.h.
// Uses cuBLAS for FP16 (cublasHgemm) and BF16 (cublasGemmEx with CUDA_R_16BF).
// Both API calls engage Tensor Core units automatically on SM 7.0+ (FP16) and
// SM 8.0+ (BF16) hardware.
//
// Storage layout note
// -------------------
// The public API uses row-major matrices (C convention).
// cuBLAS is column-major (Fortran convention).  We exploit the identity
//   A_row * B_row = (B_col * A_col)^T
// and compute  C^T = B^T * A^T  by swapping A/B and transposing both,
// which requires no additional memory copies.  Specifically we call:
//   cublasHgemm(handle, CUBLAS_OP_T, CUBLAS_OP_T, N, M, K, alpha, B, K, A, M,
//               beta, C, N)
// This is equivalent to the row-major GEMM C = alpha * A * B + beta * C.
// =============================================================================

#ifdef THEMIS_ENABLE_CUDA

#include "acceleration/tensor_core_matmul.h"
#include <cublas_v2.h>
#include <cuda_fp16.h>
#include <cuda_bf16.h>

// Per-thread-block cuBLAS handle — lightweight wrapper using thread-local
// storage so multiple host threads do not share a handle.
namespace {

static thread_local cublasHandle_t tls_cublas_handle = nullptr;

/// Lazily create a cuBLAS handle for the calling thread.
/// Returns CUBLAS_STATUS_SUCCESS or a cuBLAS error code.
static cublasStatus_t get_cublas_handle(cublasHandle_t* out_handle)
{
    if (tls_cublas_handle == nullptr) {
        cublasStatus_t st = cublasCreate(&tls_cublas_handle);
        if (st != CUBLAS_STATUS_SUCCESS) {
            return st;
        }
        // Request Tensor Core math mode (CUBLAS_TENSOR_OP_MATH deprecated in
        // cuBLAS 11; CUBLAS_DEFAULT_MATH already allows TC on sm_70+).
        cublasSetMathMode(tls_cublas_handle, CUBLAS_DEFAULT_MATH);
    }
    *out_handle = tls_cublas_handle;
    return CUBLAS_STATUS_SUCCESS;
}

} // anonymous namespace

extern "C" {

// ---------------------------------------------------------------------------
// FP16 GEMM
// ---------------------------------------------------------------------------
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
)
{
    cublasHandle_t handle;
    cublasStatus_t st = get_cublas_handle(&handle);
    if (st != CUBLAS_STATUS_SUCCESS) return static_cast<int>(st);

    if (stream) cublasSetStream(handle, stream);

    // Convert alpha/beta to __half for cublasHgemm
    __half h_alpha = __float2half(alpha);
    __half h_beta  = __float2half(beta);

    // Row-major A[M×K] * B[K×N] = C[M×N]
    // Equivalent column-major call: C^T[N×M] = B^T[N×K] * A^T[K×M]
    //   => cublasHgemm(CUBLAS_OP_T, CUBLAS_OP_T, N, M, K, alpha, B, K, A, M,
    //                  beta, C, N)
    st = cublasHgemm(
        handle,
        CUBLAS_OP_T, CUBLAS_OP_T,
        N, M, K,
        &h_alpha,
        d_B, K,
        d_A, M,
        &h_beta,
        d_C, N
    );
    return static_cast<int>(st);
}

// ---------------------------------------------------------------------------
// BF16 GEMM
// ---------------------------------------------------------------------------
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
)
{
    cublasHandle_t handle;
    cublasStatus_t st = get_cublas_handle(&handle);
    if (st != CUBLAS_STATUS_SUCCESS) return static_cast<int>(st);

    if (stream) cublasSetStream(handle, stream);

    // Row-major: C^T = B^T * A^T  (see layout note in file header)
    st = cublasGemmEx(
        handle,
        CUBLAS_OP_T, CUBLAS_OP_T,
        N, M, K,
        &alpha,
        d_B, CUDA_R_16BF, K,
        d_A, CUDA_R_16BF, M,
        &beta,
        d_C, CUDA_R_16BF, N,
        CUBLAS_COMPUTE_32F,          // Accumulate in FP32 for accuracy
        CUBLAS_GEMM_DEFAULT_TENSOR_OP
    );
    return static_cast<int>(st);
}

// ---------------------------------------------------------------------------
// FP32 GEMM (GPU path)
// ---------------------------------------------------------------------------
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
)
{
    cublasHandle_t handle;
    cublasStatus_t st = get_cublas_handle(&handle);
    if (st != CUBLAS_STATUS_SUCCESS) return static_cast<int>(st);

    if (stream) cublasSetStream(handle, stream);

    // Row-major: C^T = B^T * A^T  (see layout note in file header)
    st = cublasSgemm(
        handle,
        CUBLAS_OP_T, CUBLAS_OP_T,
        N, M, K,
        &alpha,
        d_B, K,
        d_A, M,
        &beta,
        d_C, N
    );
    return static_cast<int>(st);
}

} // extern "C"

#endif // THEMIS_ENABLE_CUDA
