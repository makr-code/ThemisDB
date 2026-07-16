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
// cuBLAS uses column-major (Fortran convention).  The standard identity is:
//
//   C_row = alpha * A_row * B_row + beta * C_row
//
// is computed as:
//
//   C^T_col = alpha * B^T_col * A^T_col + beta * C^T_col
//
// Memory equivalences (a row-major [m×n] matrix stored in memory is
// identical to the column-major transpose [n×m] with leading dimension n):
//
//   d_A  [M×K] row-major  ==  A^T_col [K×M] col-major,  lda = K
//   d_B  [K×N] row-major  ==  B^T_col [N×K] col-major,  lda = N
//   d_C  [M×N] row-major  ==  C^T_col [N×M] col-major,  ldc = N
//
// Therefore the cuBLAS call is:
//   cublasXgemm(handle, CUBLAS_OP_N, CUBLAS_OP_N,
//               N, M, K, alpha, d_B, N, d_A, K, beta, d_C, N)
//
// No explicit CUBLAS_OP_T is required; the row/column-major reinterpretation
// of the same memory handles the transposition automatically.
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

    // Row-major GEMM via the column-major cuBLAS identity:
    //   C_row[M×N] = A_row[M×K] * B_row[K×N]
    //   ⟺  C^T_col[N×M] = B^T_col[N×K] * A^T_col[K×M]
    //
    // Memory equivalences (row-major X_row == X^T_col):
    //   d_B (B_row[K×N]) as B^T_col[N×K]  → lda = N
    //   d_A (A_row[M×K]) as A^T_col[K×M]  → lda = K
    //   d_C (C_row[M×N]) as C^T_col[N×M]  → ldc = N
    //
    // No explicit transpose is needed (CUBLAS_OP_N); the column/row-major
    // equivalence handles the reinterpretation transparently.
    st = cublasHgemm(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        N, M, K,
        &h_alpha,
        d_B, N,
        d_A, K,
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

    // Row-major: C^T_col[N×M] = B^T_col[N×K] * A^T_col[K×M]
    // d_B (B_row[K×N]) as B^T_col lda=N; d_A (A_row[M×K]) as A^T_col lda=K.
    st = cublasGemmEx(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        N, M, K,
        &alpha,
        d_B, CUDA_R_16BF, N,
        d_A, CUDA_R_16BF, K,
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

    // Row-major: C^T_col[N×M] = B^T_col[N×K] * A^T_col[K×M]
    // d_B (B_row[K×N]) as B^T_col lda=N; d_A (A_row[M×K]) as A^T_col lda=K.
    st = cublasSgemm(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        N, M, K,
        &alpha,
        d_B, N,
        d_A, K,
        &beta,
        d_C, N
    );
    return static_cast<int>(st);
}

// ---------------------------------------------------------------------------
// INT8 GEMM — requires Turing (SM 7.5+)
// Inputs: int8_t A[M×K], int8_t B[K×N], Output: int32_t C[M×N]
// Uses cublasGemmEx with CUDA_R_8I inputs and CUDA_R_32I output.
// alpha/beta are applied to the int32 accumulator cast to float.
// ---------------------------------------------------------------------------
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
)
{
    // Runtime compute-capability guard: INT8 Tensor Cores require SM 7.5+.
    int device = 0;
    cudaError_t ce = cudaGetDevice(&device);
    if (ce != cudaSuccess) return 1;
    int major = 0, minor = 0;
    ce = cudaDeviceGetAttribute(&major, cudaDevAttrComputeCapabilityMajor, device);
    if (ce != cudaSuccess) return 1;
    ce = cudaDeviceGetAttribute(&minor, cudaDevAttrComputeCapabilityMinor, device);
    if (ce != cudaSuccess) return 1;
    const int sm = major * 10 + minor;
    if (sm < 75) {
        // INT8 Tensor Core acceleration requires Turing (SM 7.5+).
        // Fall back silently to CPU — caller should have checked capabilities.
        return 1;
    }

    cublasHandle_t handle;
    cublasStatus_t st = get_cublas_handle(&handle);
    if (st != CUBLAS_STATUS_SUCCESS) return static_cast<int>(st);
    if (stream) cublasSetStream(handle, stream);

    // Row-major reinterpretation (same column-major swap as for FP16/FP32):
    //   C_row[M×N] = A_row[M×K] × B_row[K×N]
    //   ⟺ C^T_col[N×M] = B^T_col[N×K] × A^T_col[K×M]
    //
    // cublasGemmEx signature:
    //   (handle, transa=N, transb=N, n=N, m=M, k=K,
    //    alpha, d_B, CUDA_R_8I, lda=N,
    //           d_A, CUDA_R_8I, ldb=K,
    //    beta,  d_C, CUDA_R_32I, ldc=N,
    //    CUDA_R_32I, CUBLAS_GEMM_DEFAULT_TENSOR_OP)
    //
    // alpha/beta for INT8 GemmEx are passed as int32 values.
    const int32_t ialpha = static_cast<int32_t>(alpha);
    const int32_t ibeta  = static_cast<int32_t>(beta);

    st = cublasGemmEx(
        handle,
        CUBLAS_OP_N, CUBLAS_OP_N,
        N, M, K,
        &ialpha,
        d_B, CUDA_R_8I,  N,
        d_A, CUDA_R_8I,  K,
        &ibeta,
        d_C, CUDA_R_32I, N,
        CUDA_R_32I,
        CUBLAS_GEMM_DEFAULT_TENSOR_OP
    );
    return static_cast<int>(st);
}

} // extern "C"

#endif // THEMIS_ENABLE_CUDA
