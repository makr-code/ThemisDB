#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/cuda_fp16_kernels.h"
#include <cuda_runtime.h>
#include <cuda_fp16.h>
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
 * @brief FP32 to FP16 conversion kernel
 */
__global__ void fp32_to_fp16_kernel(const float* input, __half* output, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        output[idx] = __float2half(input[idx]);
    }
}

/**
 * @brief FP16 to FP32 conversion kernel
 */
__global__ void fp16_to_fp32_kernel(const __half* input, float* output, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        output[idx] = __half2float(input[idx]);
    }
}

// ============================================================================
// FP16 Element-wise Kernels
// ============================================================================

/**
 * @brief FP16 element-wise addition kernel
 */
__global__ void fp16_add_kernel(const __half* A, const __half* B, __half* C, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = __hadd(A[idx], B[idx]);
    }
}

/**
 * @brief FP16 element-wise multiplication kernel
 */
__global__ void fp16_multiply_kernel(const __half* A, const __half* B, __half* C, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = __hmul(A[idx], B[idx]);
    }
}

/**
 * @brief FP16 scalar multiplication kernel
 */
__global__ void fp16_scalar_multiply_kernel(const __half* A, __half* C, __half scalar, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = __hmul(A[idx], scalar);
    }
}

/**
 * @brief FP16 matrix transpose kernel with shared memory
 */
__global__ void fp16_transpose_kernel(const __half* A, __half* C, size_t rows, size_t cols) {
    __shared__ __half tile[32][33];  // 33 to avoid bank conflicts
    
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

cudaError_t launch_fp32_to_fp16_kernel(
    const float* input,
    __half* output,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    fp32_to_fp16_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(input, output, size);
    
    return cudaGetLastError();
}

cudaError_t launch_fp16_to_fp32_kernel(
    const __half* input,
    float* output,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    fp16_to_fp32_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(input, output, size);
    
    return cudaGetLastError();
}

cudaError_t launch_fp16_matmul_kernel(
    const __half* A,
    const __half* B,
    __half* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream
) {
    // Implement using cuBLAS with Tensor Core support
    // This uses cublasGemmEx with CUDA_R_16F for FP16 computation
    
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
    const __half alpha_fp16 = __float2half(alpha);
    const __half beta_fp16 = __float2half(beta);
    
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
        &alpha_fp16,
        B, CUDA_R_16F, static_cast<int>(N),  // Leading dimension of B
        A, CUDA_R_16F, static_cast<int>(K),  // Leading dimension of A
        &beta_fp16,
        C, CUDA_R_16F, static_cast<int>(N),  // Leading dimension of C
        CUBLAS_COMPUTE_16F,  // Compute type (FP16 with Tensor Cores)
        CUBLAS_GEMM_DEFAULT_TENSOR_OP  // Algorithm (uses Tensor Cores when available)
    );
    
    cublasDestroy(handle);
    
    if (cublas_status != CUBLAS_STATUS_SUCCESS) {
        return cudaErrorUnknown;
    }
    
    return cudaSuccess;
}

cudaError_t launch_fp16_add_kernel(
    const __half* A,
    const __half* B,
    __half* C,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    fp16_add_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(A, B, C, size);
    
    return cudaGetLastError();
}

cudaError_t launch_fp16_multiply_kernel(
    const __half* A,
    const __half* B,
    __half* C,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    fp16_multiply_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(A, B, C, size);
    
    return cudaGetLastError();
}

cudaError_t launch_fp16_scalar_multiply_kernel(
    const __half* A,
    __half* C,
    float scalar,
    size_t size,
    cudaStream_t stream
) {
    constexpr int BLOCK_SIZE = 256;
    int num_blocks = (size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    
    __half scalar_fp16 = __float2half(scalar);
    fp16_scalar_multiply_kernel<<<num_blocks, BLOCK_SIZE, 0, stream>>>(A, C, scalar_fp16, size);
    
    return cudaGetLastError();
}

cudaError_t launch_fp16_transpose_kernel(
    const __half* A,
    __half* C,
    size_t rows,
    size_t cols,
    cudaStream_t stream
) {
    dim3 block_size(32, 32);
    dim3 grid_size((cols + 31) / 32, (rows + 31) / 32);
    
    fp16_transpose_kernel<<<grid_size, block_size, 0, stream>>>(A, C, rows, cols);
    
    return cudaGetLastError();
}

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
