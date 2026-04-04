#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/cuda_flash_lora_kernels.h"
#include <cuda_runtime.h>
#include <device_launch_parameters.h>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {
namespace flash {

// ============================================================================
// FlashLoRA Forward Kernel
// ============================================================================

/**
 * @brief Tiled FlashLoRA forward kernel
 * 
 * Key optimizations:
 * 1. Shared memory for input and weight tiles (fast SRAM)
 * 2. Register accumulation for intermediate (fastest)
 * 3. Minimal HBM accesses (only read input/weights, write output)
 * 
 * Memory hierarchy:
 * - Registers: 30 TB/s bandwidth (intermediate values)
 * - Shared Memory: 20 TB/s bandwidth (tiles)
 * - HBM: 1.5 TB/s bandwidth (input/output only)
 */
template<int TILE_M, int TILE_K, int RANK>
__global__ void flash_lora_forward_kernel(
    const float* __restrict__ input,    // [batch_size, seq_len, in_dim]
    const float* __restrict__ B,        // [rank, in_dim]
    const float* __restrict__ A,        // [out_dim, rank]
    float* __restrict__ output,         // [batch_size, seq_len, out_dim]
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim
) {
    // Shared memory for tiles
    __shared__ float smem_input[TILE_M][TILE_K];
    __shared__ float smem_B[RANK][TILE_K];
    
    // Thread indices
    int tx = threadIdx.x;
    int ty = threadIdx.y;
    
    // Global indices
    int batch_idx = blockIdx.y;
    int seq_tile_idx = blockIdx.x;
    int seq_idx = seq_tile_idx * TILE_M + ty;
    
    // Early exit if out of bounds
    if (batch_idx >= batch_size || seq_idx >= seq_len) {
        return;
    }
    
    // Register accumulator for intermediate: intermediate[rank]
    // This is the key optimization - keep intermediate in registers!
    float intermediate[RANK];
    #pragma unroll
    for (int r = 0; r < RANK; ++r) {
        intermediate[r] = 0.0f;
    }
    
    // ========== Phase 1: Compute intermediate = input @ B^T ==========
    // Loop over input dimension in tiles
    for (int k_tile = 0; k_tile < in_dim; k_tile += TILE_K) {
        // Load input tile to shared memory
        // Each thread loads one element
        if (k_tile + tx < in_dim && seq_idx < seq_len) {
            smem_input[ty][tx] = input[batch_idx * seq_len * in_dim + 
                                       seq_idx * in_dim + k_tile + tx];
        } else {
            smem_input[ty][tx] = 0.0f;
        }
        
        // Load B tile to shared memory
        // B is [rank, in_dim], so each thread loads B[ty, k_tile + tx]
        if (ty < RANK && k_tile + tx < in_dim) {
            smem_B[ty][tx] = B[ty * in_dim + k_tile + tx];
        }
        
        __syncthreads();
        
        // Compute tile contribution: intermediate += input_tile @ B_tile^T
        // For each rank dimension, accumulate dot product
        #pragma unroll
        for (int k = 0; k < TILE_K && k_tile + k < in_dim; ++k) {
            float input_val = smem_input[ty][k];
            #pragma unroll
            for (int r = 0; r < RANK; ++r) {
                intermediate[r] += input_val * smem_B[r][k];
            }
        }
        
        __syncthreads();
    }
    
    // ========== Phase 2: Compute output = intermediate @ A^T ==========
    // A is [out_dim, rank]
    // We need to compute output[out_dim] = intermediate[rank] @ A^T[rank, out_dim]
    
    // Process output dimension in chunks (if tx is valid)
    for (int out_idx = tx; out_idx < out_dim; out_idx += blockDim.x) {
        float result = 0.0f;
        
        // Dot product: intermediate @ A[out_idx, :]
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            result += intermediate[r] * A[out_idx * RANK + r];
        }
        
        // Apply scaling and write to output
        output[batch_idx * seq_len * out_dim + seq_idx * out_dim + out_idx] = result * scaling;
    }
}

template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_forward_kernel(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream
) {
    // Thread block configuration
    // TILE_M threads per block in y dimension (one per sequence element)
    // TILE_K threads per block in x dimension (for loading tiles)
    dim3 threads(TILE_K, TILE_M);
    
    // Grid configuration
    // x dimension: number of sequence tiles
    // y dimension: batch size
    dim3 blocks((seq_len + TILE_M - 1) / TILE_M, batch_size);
    
    // Launch kernel
    flash_lora_forward_kernel<TILE_M, TILE_K, RANK><<<blocks, threads, 0, stream>>>(
        input, B, A, output, scaling, batch_size, seq_len, in_dim, out_dim
    );
    
    return cudaGetLastError();
}

// ============================================================================
// FlashLoRA Backward Kernels (Simplified implementations)
// ============================================================================

/**
 * @brief Backward kernel for grad_A
 * 
 * grad_A = (input @ B^T)^T @ grad_output * scaling
 * 
 * NOTE: This implementation uses atomicAdd for gradient accumulation, which
 * can create performance bottlenecks under high parallelism. This is a 
 * simplified implementation suitable for proof-of-concept. Production use
 * should implement reduction trees or warp-level primitives for better
 * performance (e.g., using CUB library or cooperative groups).
 */
template<int TILE_M, int TILE_K, int RANK>
__global__ void flash_lora_backward_A_kernel(
    const float* __restrict__ grad_output,
    const float* __restrict__ input,
    const float* __restrict__ B,
    float* __restrict__ grad_A,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim
) {
    // Simplified: Use atomic adds for gradients
    // Full optimization would use reduction trees
    
    int batch_idx = blockIdx.y;
    int seq_idx = blockIdx.x * TILE_M + threadIdx.y;
    int tx = threadIdx.x;
    
    if (batch_idx >= batch_size || seq_idx >= seq_len) return;
    
    // Compute intermediate = input @ B^T (same as forward)
    float intermediate[RANK];
    #pragma unroll
    for (int r = 0; r < RANK; ++r) {
        intermediate[r] = 0.0f;
    }
    
    for (int i = 0; i < in_dim; ++i) {
        float input_val = input[batch_idx * seq_len * in_dim + seq_idx * in_dim + i];
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            intermediate[r] += input_val * B[r * in_dim + i];
        }
    }
    
    // grad_A[out_dim, rank] = intermediate^T @ grad_output
    for (int out_idx = tx; out_idx < out_dim; out_idx += blockDim.x) {
        float grad_out = grad_output[batch_idx * seq_len * out_dim + seq_idx * out_dim + out_idx];
        
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            atomicAdd(&grad_A[out_idx * RANK + r], intermediate[r] * grad_out * scaling);
        }
    }
}

template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_backward_A_kernel(
    const float* grad_output,
    const float* input,
    const float* B,
    float* grad_A,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream
) {
    dim3 threads(TILE_K, TILE_M);
    dim3 blocks((seq_len + TILE_M - 1) / TILE_M, batch_size);
    
    flash_lora_backward_A_kernel<TILE_M, TILE_K, RANK><<<blocks, threads, 0, stream>>>(
        grad_output, input, B, grad_A, scaling, batch_size, seq_len, in_dim, out_dim
    );
    
    return cudaGetLastError();
}

/**
 * @brief Backward kernel for grad_B
 */
template<int TILE_M, int TILE_K, int RANK>
__global__ void flash_lora_backward_B_kernel(
    const float* __restrict__ grad_output,
    const float* __restrict__ input,
    const float* __restrict__ A,
    float* __restrict__ grad_B,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim
) {
    int batch_idx = blockIdx.y;
    int seq_idx = blockIdx.x * TILE_M + threadIdx.y;
    int tx = threadIdx.x;
    
    if (batch_idx >= batch_size || seq_idx >= seq_len) return;
    
    // Compute grad_h = grad_output @ A^T
    float grad_h[RANK];
    #pragma unroll
    for (int r = 0; r < RANK; ++r) {
        grad_h[r] = 0.0f;
    }
    
    for (int out_idx = 0; out_idx < out_dim; ++out_idx) {
        float grad_out = grad_output[batch_idx * seq_len * out_dim + seq_idx * out_dim + out_idx];
        
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            grad_h[r] += grad_out * A[out_idx * RANK + r];
        }
    }
    
    // grad_B[rank, in_dim] = grad_h^T @ input
    for (int in_idx = tx; in_idx < in_dim; in_idx += blockDim.x) {
        float input_val = input[batch_idx * seq_len * in_dim + seq_idx * in_dim + in_idx];
        
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            atomicAdd(&grad_B[r * in_dim + in_idx], grad_h[r] * input_val * scaling);
        }
    }
}

template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_backward_B_kernel(
    const float* grad_output,
    const float* input,
    const float* A,
    float* grad_B,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream
) {
    dim3 threads(TILE_K, TILE_M);
    dim3 blocks((seq_len + TILE_M - 1) / TILE_M, batch_size);
    
    flash_lora_backward_B_kernel<TILE_M, TILE_K, RANK><<<blocks, threads, 0, stream>>>(
        grad_output, input, A, grad_B, scaling, batch_size, seq_len, in_dim, out_dim
    );
    
    return cudaGetLastError();
}

/**
 * @brief Backward kernel for grad_input
 */
template<int TILE_M, int TILE_K, int RANK>
__global__ void flash_lora_backward_input_kernel(
    const float* __restrict__ grad_output,
    const float* __restrict__ B,
    const float* __restrict__ A,
    float* __restrict__ grad_input,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim
) {
    int batch_idx = blockIdx.y;
    int seq_idx = blockIdx.x * TILE_M + threadIdx.y;
    int tx = threadIdx.x;
    
    if (batch_idx >= batch_size || seq_idx >= seq_len) return;
    
    // Compute grad_h = grad_output @ A^T
    float grad_h[RANK];
    #pragma unroll
    for (int r = 0; r < RANK; ++r) {
        grad_h[r] = 0.0f;
    }
    
    for (int out_idx = 0; out_idx < out_dim; ++out_idx) {
        float grad_out = grad_output[batch_idx * seq_len * out_dim + seq_idx * out_dim + out_idx];
        
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            grad_h[r] += grad_out * A[out_idx * RANK + r] * scaling;
        }
    }
    
    // grad_input = grad_h @ B
    for (int in_idx = tx; in_idx < in_dim; in_idx += blockDim.x) {
        float result = 0.0f;
        
        #pragma unroll
        for (int r = 0; r < RANK; ++r) {
            result += grad_h[r] * B[r * in_dim + in_idx];
        }
        
        grad_input[batch_idx * seq_len * in_dim + seq_idx * in_dim + in_idx] = result;
    }
}

template<int TILE_M, int TILE_K, int RANK>
cudaError_t launch_flash_lora_backward_input_kernel(
    const float* grad_output,
    const float* B,
    const float* A,
    float* grad_input,
    float scaling,
    size_t batch_size,
    size_t seq_len,
    size_t in_dim,
    size_t out_dim,
    cudaStream_t stream
) {
    dim3 threads(TILE_K, TILE_M);
    dim3 blocks((seq_len + TILE_M - 1) / TILE_M, batch_size);
    
    flash_lora_backward_input_kernel<TILE_M, TILE_K, RANK><<<blocks, threads, 0, stream>>>(
        grad_output, B, A, grad_input, scaling, batch_size, seq_len, in_dim, out_dim
    );
    
    return cudaGetLastError();
}

// ============================================================================
// Explicit Template Instantiations
// ============================================================================

// Forward kernel instantiations
template cudaError_t launch_flash_lora_forward_kernel<128, 64, 4>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_forward_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_forward_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_forward_kernel<128, 64, 32>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_forward_kernel<128, 64, 64>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

// Backward A kernel instantiations
template cudaError_t launch_flash_lora_backward_A_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_backward_A_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

// Backward B kernel instantiations
template cudaError_t launch_flash_lora_backward_B_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_backward_B_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

// Backward input kernel instantiations
template cudaError_t launch_flash_lora_backward_input_kernel<128, 64, 8>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

template cudaError_t launch_flash_lora_backward_input_kernel<128, 64, 16>(
    const float*, const float*, const float*, float*, float,
    size_t, size_t, size_t, size_t, cudaStream_t);

} // namespace flash
} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
