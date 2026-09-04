/**
 * @file hip_kernels.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=4; TODO=2, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=13, H=14, M=2, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_HIP

#include "llm/lora_framework/hip_kernels.h"
#include "security/vram_secure_clear.h"
#include <spdlog/spdlog.h>
#include <hip/hip_runtime.h>
#include <rocblas/rocblas.h>

namespace themis {
namespace llm {
namespace lora {
namespace hip {

// ============================================================================
// HIP Kernels (same as CUDA but using HIP API)
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
 * @brief In-place scalar multiplication kernel
 */
__global__ void scalar_multiply_inplace_kernel(float* data, float scalar, size_t size) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        data[idx] *= scalar;
    }
}

/**
 * @brief Check for NaN or Inf in tensor
 * Uses atomic operations to set flag on detection
 */
__device__ inline bool is_inf_or_nan([[maybe_unused]] float val) {
    return isnan(val) || isinf(val);
}

__global__ void check_inf_nan_kernel(const float* data, size_t size, int* has_overflow) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx < size) {
        if (is_inf_or_nan(data[idx])) {
            atomicExch(has_overflow, 1);
        }
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
    size_t r = blockIdx.y * blockDim.y + threadIdx.y;
    size_t o = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (r >= rank || o >= out_dim) {
        return;
    }
    
    float sum = 0.0f;
    
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
    size_t i = blockIdx.y * blockDim.y + threadIdx.y;
    size_t r = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (i >= in_dim || r >= rank) {
        return;
    }
    
    float sum = 0.0f;
    
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

/**
 * @brief MSE loss reduction kernel with shared memory
 * 
 * Computes partial sums of squared differences between predictions and targets.
 * Each block computes a partial sum using parallel reduction in shared memory.
 */
__global__ void mse_loss_reduction_kernel(
    const float* predictions,
    const float* targets,
    float* partial_sums,
    int n
) {
    __shared__ float shared_sum[THEMIS_GPU_REDUCTION_SHARED_MEM_SIZE];
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    // Compute partial sum for this thread
    float local_sum = 0.0f;
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        float diff = predictions[i] - targets[i];
        local_sum += diff * diff;
    }
    
    // Store in shared memory
    shared_sum[tid] = local_sum;
    __syncthreads();
    
    // Tree reduction in shared memory
    for (int s = blockDim.x / 2; s > 64; s >>= 1) {
        if (tid < s) {
            shared_sum[tid] += shared_sum[tid + s];
        }
        __syncthreads();
    }
    
    // Final wavefront reduction (no sync needed, wavefront is implicitly synchronized)
    // HIP wavefront size is 64 threads
    if (tid < 64) {
        // At this point, we have at most 128 values left (s=64 case)
        // Safely reduce the last wavefront without bounds checks since blockDim.x=256
        volatile float* smem = shared_sum;
        if (blockDim.x >= 128) {
          smem[tid] += smem[tid + 64];
        }
        if (blockDim.x >= 64) {
          smem[tid] += smem[tid + 32];
        }
        if (blockDim.x >= 32) {
          smem[tid] += smem[tid + 16];
        }
        if (blockDim.x >= 16) {
          smem[tid] += smem[tid + 8];
        }
        if (blockDim.x >= 8) {
          smem[tid] += smem[tid + 4];
        }
        if (blockDim.x >= 4) {
          smem[tid] += smem[tid + 2];
        }
        if (blockDim.x >= 2) {
          smem[tid] += smem[tid + 1];
        }
    }
    
    // Write block result
    if (tid == 0) {
        partial_sums[blockIdx.x] = shared_sum[0];
    }
}

/**
 * @brief MSE gradient kernel
 * 
 * Computes gradient of MSE loss: grad = (2/n) * (predictions - targets)
 * This is element-wise and fully parallelizable.
 */
__global__ void mse_gradient_kernel(
    float* grad_output,
    const float* predictions,
    const float* targets,
    float scale,
    int n
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int stride = blockDim.x * gridDim.x;
    
    for (int i = idx; i < n; i += stride) {
        grad_output[i] = scale * (predictions[i] - targets[i]);
    }
}

// ============================================================================
// Kernel Launchers
// ============================================================================

hipError_t launch_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    hipStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((N + 15) / 16, (M + 15) / 16);
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(matmul_kernel, gridDim, blockDim, 0, stream, A, B, C, M, K, N, alpha);
    } else {
        hipLaunchKernelGGL(matmul_kernel, gridDim, blockDim, 0, 0, A, B, C, M, K, N, alpha);
    }
    
    return hipGetLastError();
}

hipError_t launch_add_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    hipStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(add_kernel, gridSize, blockSize, 0, stream, A, B, C, size);
    } else {
        hipLaunchKernelGGL(add_kernel, gridSize, blockSize, 0, 0, A, B, C, size);
    }
    
    return hipGetLastError();
}

hipError_t launch_multiply_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    hipStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(multiply_kernel, gridSize, blockSize, 0, stream, A, B, C, size);
    } else {
        hipLaunchKernelGGL(multiply_kernel, gridSize, blockSize, 0, 0, A, B, C, size);
    }
    
    return hipGetLastError();
}

hipError_t launch_scalar_multiply_kernel(
    const float* A,
    float* C,
    float scalar,
    size_t size,
    hipStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(scalar_multiply_kernel, gridSize, blockSize, 0, stream, A, C, scalar, size);
    } else {
        hipLaunchKernelGGL(scalar_multiply_kernel, gridSize, blockSize, 0, 0, A, C, scalar, size);
    }
    
    return hipGetLastError();
}

hipError_t launch_scalar_multiply_inplace_kernel(
    float* data,
    float scalar,
    size_t size,
    hipStream_t stream
) {
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(scalar_multiply_inplace_kernel, gridSize, blockSize, 0, stream, data, scalar, size);
    } else {
        hipLaunchKernelGGL(scalar_multiply_inplace_kernel, gridSize, blockSize, 0, 0, data, scalar, size);
    }
    
    return hipGetLastError();
}

hipError_t launch_check_inf_nan_kernel(
    const float* data,
    size_t size,
    bool* has_overflow_host
) {
    // Note: For better performance, consider reusing a pre-allocated device buffer
    // or using unified memory instead of allocating on every call. Currently acceptable
    // as this is called once per training step, not in a tight loop.
    
    // Allocate device flag
    int* d_overflow;
    hipError_t err = hipMalloc(&d_overflow, sizeof(int));
    if (err != hipSuccess) {
        return err;
    }
    
    // Initialize to 0
    err = hipMemset(d_overflow, 0, sizeof(int));
    if (err != hipSuccess) {
        security::VRAMSecureClear::secureClearHIP(d_overflow, sizeof(int));
        const hipError_t free_err = hipFree(d_overflow);
        if (free_err != hipSuccess) {
            spdlog::error("checkInfNanHIP cleanup hipFree failed after hipMemset error: {}",
                          hipGetErrorString(free_err));
        }
        return err;
    }
    
    // Launch kernel
    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    hipLaunchKernelGGL(check_inf_nan_kernel, gridSize, blockSize, 0, 0, data, size, d_overflow);
    
    err = hipGetLastError();
    if (err != hipSuccess) {
        security::VRAMSecureClear::secureClearHIP(d_overflow, sizeof(int));
        const hipError_t free_err = hipFree(d_overflow);
        if (free_err != hipSuccess) {
            spdlog::error("checkInfNanHIP cleanup hipFree failed after kernel launch error: {}",
                          hipGetErrorString(free_err));
        }
        return err;
    }
    
    // Copy result back
    int h_overflow;
    err = hipMemcpy(&h_overflow, d_overflow, sizeof(int), hipMemcpyDeviceToHost);
    
    // Securely clear before freeing
    security::VRAMSecureClear::secureClearHIP(d_overflow, sizeof(int));
    const hipError_t free_err = hipFree(d_overflow);
    if (free_err != hipSuccess) {
        spdlog::error("checkInfNanHIP cleanup hipFree failed after result copy: {}",
                      hipGetErrorString(free_err));
        if (err == hipSuccess) {
            return free_err;
        }
    }
    
    if (err != hipSuccess) {
        return err;
    }
    
    *has_overflow_host = (h_overflow == 1);
    return hipSuccess;
}

hipError_t launch_transpose_kernel(
    const float* A,
    float* C,
    size_t rows,
    size_t cols,
    hipStream_t stream
) {
    dim3 blockDim(32, 32);
    dim3 gridDim((cols + 31) / 32, (rows + 31) / 32);
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(transpose_kernel, gridDim, blockDim, 0, stream, A, C, rows, cols);
    } else {
        hipLaunchKernelGGL(transpose_kernel, gridDim, blockDim, 0, 0, A, C, rows, cols);
    }
    
    return hipGetLastError();
}

hipError_t launch_lora_backward_A_kernel(
    const float* input,
    const float* B,
    const float* grad_output,
    float* grad_A,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    hipStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((out_dim + 15) / 16, (rank + 15) / 16);
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(lora_backward_A_kernel, gridDim, blockDim, 0, stream,
            input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
    } else {
        hipLaunchKernelGGL(lora_backward_A_kernel, gridDim, blockDim, 0, 0,
            input, B, grad_output, grad_A, batch_size, in_dim, rank, out_dim, scaling);
    }
    
    return hipGetLastError();
}

hipError_t launch_lora_backward_B_kernel(
    const float* input,
    const float* A,
    const float* grad_output,
    float* grad_B,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    hipStream_t stream
) {
    dim3 blockDim(16, 16);
    dim3 gridDim((rank + 15) / 16, (in_dim + 15) / 16);
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(lora_backward_B_kernel, gridDim, blockDim, 0, stream,
            input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
    } else {
        hipLaunchKernelGGL(lora_backward_B_kernel, gridDim, blockDim, 0, 0,
            input, A, grad_output, grad_B, batch_size, in_dim, rank, out_dim, scaling);
    }
    
    return hipGetLastError();
}

hipError_t launch_mse_loss_reduction_kernel(
    const float* predictions,
    const float* targets,
    float* partial_sums,
    int n,
    int num_blocks,
    hipStream_t stream
) {
    int threads = THEMIS_GPU_REDUCTION_BLOCK_SIZE;
    int blocks = num_blocks;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(mse_loss_reduction_kernel, dim3(blocks), dim3(threads), 0, stream,
            predictions, targets, partial_sums, n);
    } else {
        hipLaunchKernelGGL(mse_loss_reduction_kernel, dim3(blocks), dim3(threads), 0, 0,
            predictions, targets, partial_sums, n);
    }
    
    return hipGetLastError();
}

hipError_t launch_mse_gradient_kernel(
    float* grad_output,
    const float* predictions,
    const float* targets,
    float scale,
    int n,
    hipStream_t stream
) {
    int threads = THEMIS_GPU_REDUCTION_BLOCK_SIZE;
    int blocks = (n + threads - 1) / threads;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(mse_gradient_kernel, dim3(blocks), dim3(threads), 0, stream,
            grad_output, predictions, targets, scale, n);
    } else {
        hipLaunchKernelGGL(mse_gradient_kernel, dim3(blocks), dim3(threads), 0, 0,
            grad_output, predictions, targets, scale, n);
    }
    
    return hipGetLastError();
}

// ============================================================================
// rocBLAS Wrapper
// ============================================================================

RocblasHandle::RocblasHandle() {
    // REL-86: check rocblas_create_handle return value; leave handle_ null on failure
    // so callers can detect the condition via is_valid() and avoid UB.
    rocblas_status status = rocblas_create_handle(&handle_);
    if (status != rocblas_status_success) {
        spdlog::error("RocblasHandle: rocblas_create_handle failed (status={})", static_cast<int>(status));
        handle_ = nullptr;
    }
}

RocblasHandle::~RocblasHandle() {
    if (handle_ != nullptr) {
        rocblas_destroy_handle(handle_);
    }
}

RocblasHandle::RocblasHandle(RocblasHandle&& other) noexcept
    : handle_(other.handle_) {
    other.handle_ = nullptr;
}

RocblasHandle& RocblasHandle::operator=(RocblasHandle&& other) noexcept {
    if (this != &other) {
        if (handle_ != nullptr) {
            rocblas_destroy_handle(handle_);
        }
        handle_ = other.handle_;
        other.handle_ = nullptr;
    }
    return *this;
}

hipError_t rocblas_matmul(
    rocblas_handle handle,
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    float beta
) {
    rocblas_status status = rocblas_sgemm(
        handle,
        rocblas_operation_none,
        rocblas_operation_none,
        N, M, K,
        &alpha,
        B, N,
        A, K,
        &beta,
        C, N
    );
    
    if (status != rocblas_status_success) {
        return hipErrorUnknown;
    }
    
    return hipSuccess;
}

/**
 * @brief HIP kernel for embedding lookup
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

hipError_t launch_embedding_lookup_kernel(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    size_t vocab_size,
    hipStream_t stream
) {
    size_t total_tokens = batch_size * seq_len;
    
    // Configure kernel launch
    const int threads_per_block = 256;
    const int num_blocks = (total_tokens + threads_per_block - 1) / threads_per_block;
    
    // Launch kernel
    if (stream) {
        hipLaunchKernelGGL(embedding_lookup_kernel, 
                          dim3(num_blocks), dim3(threads_per_block), 0, stream,
                          output, token_ids, embedding_weights,
                          batch_size, seq_len, hidden_dim, vocab_size);
    } else {
        hipLaunchKernelGGL(embedding_lookup_kernel, 
                          dim3(num_blocks), dim3(threads_per_block), 0, 0,
                          output, token_ids, embedding_weights,
                          batch_size, seq_len, hidden_dim, vocab_size);
    }
    
    // Check for launch errors
    hipError_t err = hipGetLastError();
    if (err != hipSuccess) {
        return err;
    }
    
    // Synchronize if no stream
    if (!stream) {
        return hipDeviceSynchronize();
    }
    
    return hipSuccess;
}

/**
 * @brief HIP kernel for computing mean over sequence dimension
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

hipError_t launch_sequence_mean_kernel(
    float* output,
    const float* input,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    hipStream_t stream
) {
    size_t total_outputs = batch_size * hidden_dim;
    
    // Configure kernel launch
    const int threads_per_block = 256;
    const int num_blocks = (total_outputs + threads_per_block - 1) / threads_per_block;
    
    // Launch kernel
    if (stream) {
        hipLaunchKernelGGL(sequence_mean_kernel, 
                          dim3(num_blocks), dim3(threads_per_block), 0, stream,
                          output, input, batch_size, seq_len, hidden_dim);
    } else {
        hipLaunchKernelGGL(sequence_mean_kernel, 
                          dim3(num_blocks), dim3(threads_per_block), 0, 0,
                          output, input, batch_size, seq_len, hidden_dim);
    }
    
    // Check for launch errors
    hipError_t err = hipGetLastError();
    if (err != hipSuccess) {
        return err;
    }
    
    // Synchronize if no stream
    if (!stream) {
        return hipDeviceSynchronize();
    }
    
    return hipSuccess;
}

/**
 * @brief SGD parameter update kernel
 * 
 * Performs in-place SGD update: param = param - learning_rate * grad
 */
__global__ void sgd_update_kernel(
    float* params,
    const float* grads,
    float learning_rate,
    size_t size
) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    if (idx < size) {
        params[idx] -= learning_rate * grads[idx];
    }
}

hipError_t launch_sgd_update_kernel(
    float* params,
    const float* grads,
    float learning_rate,
    size_t size,
    hipStream_t stream
) {
    if (!params || !grads || size == 0) {
        return hipErrorInvalidValue;
    }
    
    // Launch configuration
    const int threads_per_block = 256;
    const int num_blocks = (size + threads_per_block - 1) / threads_per_block;
    
    // Launch kernel
    if (stream) {
        hipLaunchKernelGGL(sgd_update_kernel, 
            dim3(num_blocks), dim3(threads_per_block), 0, stream,
            params, grads, learning_rate, size);
    } else {
        hipLaunchKernelGGL(sgd_update_kernel, 
            dim3(num_blocks), dim3(threads_per_block), 0, 0,
            params, grads, learning_rate, size);
    }
    
    // Check for launch errors
    hipError_t err = hipGetLastError();
    if (err != hipSuccess) {
        return err;
    }
    
    // Synchronize if no stream
    if (!stream) {
        return hipDeviceSynchronize();
    }
    
    return hipSuccess;
}

} // namespace hip
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_HIP
