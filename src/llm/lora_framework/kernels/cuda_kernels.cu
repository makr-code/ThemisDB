#ifdef THEMIS_ENABLE_CUDA

#include "llm/lora_framework/cuda_kernels.h"
#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <device_launch_parameters.h>

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

// ============================================================================
// CUDA Kernels
// ============================================================================

/**
 * @brief Element-wise addition kernel
 */
__global__ void add_kernel(const float* A, const float* B, float* C, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = A[idx] + B[idx];
    }
}

/**
 * @brief Element-wise multiplication kernel
 */
__global__ void multiply_kernel(const float* A, const float* B, float* C, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = A[idx] * B[idx];
    }
}

/**
 * @brief Scalar multiplication kernel
 */
__global__ void scalar_multiply_kernel(const float* A, float* C, float scalar, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        C[idx] = A[idx] * scalar;
    }
}

/**
 * @brief Matrix transpose kernel with shared memory
 */
__global__ void transpose_kernel(const float* A, float* C, size_t rows, size_t cols) {
    __shared__ float tile[32][33];  // 33 to avoid bank conflicts
    
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

/**
 * @brief Tile-based matrix multiplication kernel
 * Uses shared memory for better cache utilization
 */
__global__ void matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha
) {
    __shared__ float tileA[16][16];
    __shared__ float tileB[16][16];
    
    size_t row = blockIdx.y * 16 + threadIdx.y;
    size_t col = blockIdx.x * 16 + threadIdx.x;
    
    float sum = 0.0f;
    
    // Compute number of tiles
    size_t numTiles = (K + 15) / 16;
    
    for (size_t tile = 0; tile < numTiles; tile++) {
        // Load tile of A
        size_t aCol = tile * 16 + threadIdx.x;
        if (row < M && aCol < K) {
            tileA[threadIdx.y][threadIdx.x] = A[row * K + aCol];
        } else {
            tileA[threadIdx.y][threadIdx.x] = 0.0f;
        }
        
        // Load tile of B
        size_t bRow = tile * 16 + threadIdx.y;
        if (bRow < K && col < N) {
            tileB[threadIdx.y][threadIdx.x] = B[bRow * N + col];
        } else {
            tileB[threadIdx.y][threadIdx.x] = 0.0f;
        }
        
        __syncthreads();
        
        // Compute partial dot product
        #pragma unroll 16
        for (int k = 0; k < 16; k++) {
            sum += tileA[threadIdx.y][k] * tileB[k][threadIdx.x];
        }
        
        __syncthreads();
    }
    
    // Write result
    if (row < M && col < N) {
        C[row * N + col] = sum * alpha;
    }
}

/**
 * @brief LoRA backward pass - compute grad_A
 */
__global__ void lora_backward_A_kernel(
    const float* input,
    const float* B,
    const float* grad_output,
    float* grad_A,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
) {
    size_t r = blockIdx.y * blockDim.y + threadIdx.y;  // rank dimension
    size_t o = blockIdx.x * blockDim.x + threadIdx.x;  // out_dim dimension
    
    if (r >= rank || o >= out_dim) {
        return;
    }
    
    float sum = 0.0f;
    
    // Accumulate over batch and input dimensions
    for (size_t b = 0; b < batch_size; b++) {
        for (size_t i = 0; i < in_dim; i++) {
            float input_val = input[b * in_dim + i];
            float grad_val = grad_output[b * out_dim + o] * scaling;
            float b_val = B[i * rank + r];
            
            sum += b_val * input_val * grad_val;
        }
    }
    
    grad_A[r * out_dim + o] = sum;
}

/**
 * @brief LoRA backward pass - compute grad_B
 */
__global__ void lora_backward_B_kernel(
    const float* input,
    const float* A,
    const float* grad_output,
    float* grad_B,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
) {
    size_t i = blockIdx.y * blockDim.y + threadIdx.y;  // in_dim dimension
    size_t r = blockIdx.x * blockDim.x + threadIdx.x;  // rank dimension
    
    if (i >= in_dim || r >= rank) {
        return;
    }
    
    float sum = 0.0f;
    
    // Accumulate over batch and output dimensions
    for (size_t b = 0; b < batch_size; b++) {
        for (size_t o = 0; o < out_dim; o++) {
            float grad_val = grad_output[b * out_dim + o] * scaling;
            float a_val = A[r * out_dim + o];
            float input_val = input[b * in_dim + i];
            
            sum += grad_val * a_val * input_val;
        }
    }
    
    grad_B[i * rank + r] = sum;
}

// ============================================================================
// Kernel Launchers
// ============================================================================

cudaError_t launch_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((N + 15) / 16, (M + 15) / 16);
    
    if (stream != nullptr) {
        matmul_kernel<<<gridDim, blockDim, 0, stream>>>(A, B, C, M, K, N, alpha);
    } else {
        matmul_kernel<<<gridDim, blockDim>>>(A, B, C, M, K, N, alpha);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_add_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    cudaStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        add_kernel<<<gridSize, blockSize, 0, stream>>>(A, B, C, size);
    } else {
        add_kernel<<<gridSize, blockSize>>>(A, B, C, size);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_multiply_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    cudaStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        multiply_kernel<<<gridSize, blockSize, 0, stream>>>(A, B, C, size);
    } else {
        multiply_kernel<<<gridSize, blockSize>>>(A, B, C, size);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_scalar_multiply_kernel(
    const float* A,
    float* C,
    float scalar,
    size_t size,
    cudaStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        scalar_multiply_kernel<<<gridSize, blockSize, 0, stream>>>(A, C, scalar, size);
    } else {
        scalar_multiply_kernel<<<gridSize, blockSize>>>(A, C, scalar, size);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_transpose_kernel(
    const float* A,
    float* C,
    size_t rows,
    size_t cols,
    cudaStream_t stream
) {
    dim3 blockDim(32, 32);
    dim3 gridDim((cols + 31) / 32, (rows + 31) / 32);
    
    if (stream != nullptr) {
        transpose_kernel<<<gridDim, blockDim, 0, stream>>>(A, C, rows, cols);
    } else {
        transpose_kernel<<<gridDim, blockDim>>>(A, C, rows, cols);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_lora_backward_A_kernel(
    const float* input,
    const float* B,
    const float* grad_output,
    float* grad_A,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    cudaStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((out_dim + 15) / 16, (rank + 15) / 16);
    
    if (stream != nullptr) {
        lora_backward_A_kernel<<<gridDim, blockDim, 0, stream>>>(
            input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
    } else {
        lora_backward_A_kernel<<<gridDim, blockDim>>>(
            input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
    }
    
    return cudaGetLastError();
}

cudaError_t launch_lora_backward_B_kernel(
    const float* input,
    const float* A,
    const float* grad_output,
    float* grad_B,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    cudaStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((rank + 15) / 16, (in_dim + 15) / 16);
    
    if (stream != nullptr) {
        lora_backward_B_kernel<<<gridDim, blockDim, 0, stream>>>(
            input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
    } else {
        lora_backward_B_kernel<<<gridDim, blockDim>>>(
            input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
    }
    
    return cudaGetLastError();
}

// ============================================================================
// cuBLAS Wrapper
// ============================================================================

CublasHandle::CublasHandle() {
    cublasCreate(&handle_);
}

CublasHandle::~CublasHandle() {
    if (handle_ != nullptr) {
        cublasDestroy(handle_);
    }
}

CublasHandle::CublasHandle(CublasHandle&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = nullptr;
}

CublasHandle& CublasHandle::operator=(CublasHandle&& other) noexcept {
    if (this != &other) {
        if (handle_ != nullptr) {
            cublasDestroy(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

cudaError_t cublas_matmul(
    cublasHandle_t handle,
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    float beta
) {
    // cuBLAS uses column-major order, so we compute: C = B^T @ A^T = (A @ B)^T
    // Then transpose back to row-major
    
    cublasStatus_t status = cublasSgemm(
        handle,
        CUBLAS_OP_N,  // Don't transpose B
        CUBLAS_OP_N,  // Don't transpose A
        N, M, K,      // Dimensions (swapped for column-major)
        &alpha,
        B, N,         // B matrix
        A, K,         // A matrix
        &beta,
        C, N          // C matrix
    );
    
    if (status != CUBLAS_STATUS_SUCCESS) {
        return cudaErrorUnknown;
    }
    
    return cudaSuccess;
}

/**
 * @brief CUDA kernel for embedding lookup
 * 
 * Each thread processes one token ID and copies its embedding vector
 */
__global__ void embedding_lookup_kernel(
    float* output,              // [batch_size, seq_len, hidden_dim]
    const float* token_ids,     // [batch_size, seq_len]
    const float* embedding_weights,  // [vocab_size, hidden_dim]
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    size_t vocab_size
) {
    // Each thread processes one token
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    size_t total_tokens = batch_size * seq_len;
    
    if (idx < total_tokens) {
        // Convert float token ID to int (with rounding)
        int token_id = __float2int_rn(token_ids[idx]);
        
        // Bounds check
        if (token_id >= 0 && token_id < static_cast<int>(vocab_size)) {
            // Calculate pointers
            const float* src = embedding_weights + token_id * hidden_dim;
            float* dst = output + idx * hidden_dim;
            
            // Copy embedding vector
            for (size_t i = 0; i < hidden_dim; ++i) {
                dst[i] = src[i];
            }
        } else {
            // Out of bounds - fill with zeros
            float* dst = output + idx * hidden_dim;
            for (size_t i = 0; i < hidden_dim; ++i) {
                dst[i] = 0.0f;
            }
        }
    }
}

cudaError_t launch_embedding_lookup_kernel(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    size_t vocab_size,
    cudaStream_t stream
) {
    size_t total_tokens = batch_size * seq_len;
    
    // Configure kernel launch
    const int threads_per_block = 256;
    const int num_blocks = (total_tokens + threads_per_block - 1) / threads_per_block;
    
    // Launch kernel
    if (stream) {
        embedding_lookup_kernel<<<num_blocks, threads_per_block, 0, stream>>>(
            output, token_ids, embedding_weights,
            batch_size, seq_len, hidden_dim, vocab_size
        );
    } else {
        embedding_lookup_kernel<<<num_blocks, threads_per_block>>>(
            output, token_ids, embedding_weights,
            batch_size, seq_len, hidden_dim, vocab_size
        );
    }
    
    // Check for launch errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        return err;
    }
    
    // Synchronize if no stream
    if (!stream) {
        return cudaDeviceSynchronize();
    }
    
    return cudaSuccess;
}

/**
 * @brief CUDA kernel for computing mean over sequence dimension
 * 
 * Each thread processes one element in the output [batch_size, hidden_dim]
 * and computes the mean of corresponding sequence elements
 */
__global__ void sequence_mean_kernel(
    float* output,          // [batch_size, hidden_dim]
    const float* input,     // [batch_size, seq_len, hidden_dim]
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim
) {
    // Each thread handles one output element
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    size_t total_outputs = batch_size * hidden_dim;
    
    if (idx < total_outputs) {
        size_t batch_idx = idx / hidden_dim;
        size_t hidden_idx = idx % hidden_dim;
        
        // Compute mean over sequence dimension
        float sum = 0.0f;
        for (size_t seq_idx = 0; seq_idx < seq_len; ++seq_idx) {
            size_t input_idx = batch_idx * seq_len * hidden_dim + 
                              seq_idx * hidden_dim + 
                              hidden_idx;
            sum += input[input_idx];
        }
        
        output[idx] = sum / static_cast<float>(seq_len);
    }
}

cudaError_t launch_sequence_mean_kernel(
    float* output,
    const float* input,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    cudaStream_t stream
) {
    size_t total_outputs = batch_size * hidden_dim;
    
    // Configure kernel launch
    const int threads_per_block = 256;
    const int num_blocks = (total_outputs + threads_per_block - 1) / threads_per_block;
    
    // Launch kernel
    if (stream) {
        sequence_mean_kernel<<<num_blocks, threads_per_block, 0, stream>>>(
            output, input, batch_size, seq_len, hidden_dim
        );
    } else {
        sequence_mean_kernel<<<num_blocks, threads_per_block>>>(
            output, input, batch_size, seq_len, hidden_dim
        );
    }
    
    // Check for launch errors
    cudaError_t err = cudaGetLastError();
    if (err != cudaSuccess) {
        return err;
    }
    
    // Synchronize if no stream
    if (!stream) {
        return cudaDeviceSynchronize();
    }
    
    return cudaSuccess;
}

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
