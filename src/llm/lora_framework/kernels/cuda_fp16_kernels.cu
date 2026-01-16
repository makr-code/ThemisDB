#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/cuda_fp16_kernels.h"
#include <cuda_runtime.h>
#include <cuda_fp16.h>
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
    // TODO: Implement using cuBLAS with Tensor Core support
    // For now, this is a placeholder
    // Real implementation should use cublasGemmEx with CUDA_R_16F
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
