#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/cuda_bf16_kernels.h"
#include <cuda_runtime.h>
#include <cuda_bf16.h>
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
    // TODO: Implement using cuBLAS with Tensor Core support
    // For now, this is a placeholder
    // Real implementation should use cublasGemmEx with CUDA_R_16BF
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
