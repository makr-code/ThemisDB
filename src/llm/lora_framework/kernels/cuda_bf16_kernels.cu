#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/cuda_bf16_kernels.h"
#include <cuda_runtime.h>
#include <cuda_bf16.h>
#include <cublas_v2.h>
#include <device_launch_parameters.h>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

// ============================================================================
// Type Conversion Kernels
// ============================================================================

/**
 * @brief FP32 to BF16 conversion kernel
 */
__global__ void fp32_to_bf16_kernel(const float* input, __nv_bfloat16* output, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        output[idx] = __float2bfloat16(input[idx]);
    }
}

/**
 * @brief BF16 to FP32 conversion kernel
 */
__global__ void bf16_to_fp32_kernel(const __nv_bfloat16* input, float* output, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        output[idx] = __bfloat162float(input[idx]);
    }
}

// ============================================================================
// BF16 Element-wise Kernels
// ============================================================================

/**
 * @brief BF16 element-wise addition kernel
 */
__global__ void bf16_add_kernel(const __nv_bfloat16* A, const __nv_bfloat16* B, __nv_bfloat16* C, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = __hadd(A[idx], B[idx]);
    }
}

/**
 * @brief BF16 element-wise multiplication kernel
 */
__global__ void bf16_multiply_kernel(const __nv_bfloat16* A, const __nv_bfloat16* B, __nv_bfloat16* C, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = __hmul(A[idx], B[idx]);
    }
}

/**
 * @brief BF16 scalar multiplication kernel
 */
__global__ void bf16_scalar_multiply_kernel(const __nv_bfloat16* A, __nv_bfloat16* C, __nv_bfloat16 scalar, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = __hmul(A[idx], scalar);
    }
}

/**
 * @brief BF16 matrix transpose kernel with shared memory
 */
__global__ void bf16_transpose_kernel(const __nv_bfloat16* A, __nv_bfloat16* C, size_t rows, size_t cols) {
    __shared__ __nv_bfloat16 tile[32][33];  // 33 to avoid bank conflicts
    
    size_t x = blockIdx.x * 32 + threadIdx.x;
    size_t y = blockIdx.y * 32 + threadIdx.y;
    
    // Load tile into shared memory
    if (x < cols && y < rows) {
        tile[threadIdx.y][threadIdx.x] = A[y * cols + x];
    }
    
    __syncthreads();
    
    // Transpose coordinates
    x = blockIdx.y * 32 + threadIdx.x;
    y = blockIdx.x * 32 + threadIdx.y;
    
    // Write transposed tile
    if (x < rows && y < cols) {
        C[y * rows + x] = tile[threadIdx.x][threadIdx.y];
    }
}

// ============================================================================
// Kernel Launchers
// ============================================================================

cudaError_t launch_fp32_to_bf16_kernel(
    const float* input,
    __nv_bfloat16* output,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    fp32_to_bf16_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(input, output, size);
    
    return cudaGetLastError();
}

cudaError_t launch_bf16_to_fp32_kernel(
    const __nv_bfloat16* input,
    float* output,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    bf16_to_fp32_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(input, output, size);
    
    return cudaGetLastError();
}

cudaError_t launch_bf16_matmul_kernel(
    const __nv_bfloat16* A,
    const __nv_bfloat16* B,
    __nv_bfloat16* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream
) {
    // Implement using cuBLAS with Tensor Core support
    // This uses cublasGemmEx with CUDA_R_16BF for BF16 computation
    
    // Create cuBLAS handle
    cublasHandle_t handle;
    cublasStatus_t cublas_status = cublasCreate(&handle);
    if (cublas_status != CUBLAS_STATUS_SUCCESS) {
        return cudaErrorUnknown;
    }
    
    // Set stream if provided
    if (stream != nullptr) {
        cublas_status = cublasSetStream(handle, stream);
        if (cublas_status != CUBLAS_STATUS_SUCCESS) {
            cublasDestroy(handle);
            return cudaErrorUnknown;
        }
    }
    
    // Set math mode to enable Tensor Cores on Ampere+ GPUs
    cublas_status = cublasSetMathMode(handle, CUBLAS_TENSOR_OP_MATH);
    if (cublas_status != CUBLAS_STATUS_SUCCESS) {
        cublasDestroy(handle);
        return cudaErrorUnknown;
    }
    
    // Perform matrix multiplication: C = alpha * A * B + beta * C
    // Note: cuBLAS uses column-major order, so we compute B^T * A^T = (A * B)^T
    // then effectively get C = A * B in row-major
    float beta = 0.0f;  // Don't accumulate into C
    const __nv_bfloat16 alpha_bf16 = __float2bfloat16(alpha);
    const __nv_bfloat16 beta_bf16 = __float2bfloat16(beta);
    
    // Use cublasGemmEx for mixed precision with Tensor Cores
    // Matrix dimensions in cuBLAS (column-major):
    // - op(B): N x K
    // - op(A): K x M  
    // - C: N x M
    cublas_status = cublasGemmEx(
        handle,
        CUBLAS_OP_N,  // B is not transposed
        CUBLAS_OP_N,  // A is not transposed
        static_cast<int>(N),  // Number of rows of matrix op(B) and C
        static_cast<int>(M),  // Number of columns of matrix op(A) and C
        static_cast<int>(K),  // Number of columns of op(B) and rows of op(A)
        &alpha_bf16,
        B, CUDA_R_16BF, static_cast<int>(N),  // Leading dimension of B (BF16)
        A, CUDA_R_16BF, static_cast<int>(K),  // Leading dimension of A (BF16)
        &beta_bf16,
        C, CUDA_R_16BF, static_cast<int>(N),  // Leading dimension of C (BF16)
        CUBLAS_COMPUTE_32F,  // Compute type (FP32 accumulation for BF16 inputs)
        CUBLAS_GEMM_DEFAULT_TENSOR_OP  // Algorithm (uses Tensor Cores when available)
    );
    
    cublasDestroy(handle);
    
    if (cublas_status != CUBLAS_STATUS_SUCCESS) {
        return cudaErrorUnknown;
    }
    
    return cudaSuccess;
}

cudaError_t launch_bf16_add_kernel(
    const __nv_bfloat16* A,
    const __nv_bfloat16* B,
    __nv_bfloat16* C,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    bf16_add_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(A, B, C, size);
    
    return cudaGetLastError();
}

cudaError_t launch_bf16_multiply_kernel(
    const __nv_bfloat16* A,
    const __nv_bfloat16* B,
    __nv_bfloat16* C,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    bf16_multiply_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(A, B, C, size);
    
    return cudaGetLastError();
}

cudaError_t launch_bf16_scalar_multiply_kernel(
    const __nv_bfloat16* A,
    __nv_bfloat16* C,
    float scalar,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    __nv_bfloat16 scalar_bf16 = __float2bfloat16(scalar);
    bf16_scalar_multiply_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(A, C, scalar_bf16, size);
    
    return cudaGetLastError();
}

cudaError_t launch_bf16_transpose_kernel(
    const __nv_bfloat16* A,
    __nv_bfloat16* C,
    size_t rows,
    size_t cols,
    cudaStream_t stream
) {
    dim3 block_size(32, 32);
    dim3 grid_size((cols + 31) / 32, (rows + 31) / 32);
    
    bf16_transpose_kernel<<<grid_size, block_size, 0, stream>>>(A, C, rows, cols);
    
    return cudaGetLastError();
}

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
