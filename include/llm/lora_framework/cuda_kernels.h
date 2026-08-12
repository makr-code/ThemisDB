/**
 * @file cuda_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_CUDA

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cstddef>

// Kernel configuration constants for consistency
#define THEMIS_GPU_REDUCTION_BLOCK_SIZE 256
#define THEMIS_GPU_REDUCTION_SHARED_MEM_SIZE 256
#define THEMIS_GPU_MAX_BLOCKS 1024

namespace themis {
namespace llm {
namespace lora {
namespace cuda {

/**
 * @brief CUDA kernel launcher for matrix multiplication
 * 
 * Computes C = alpha * (A @ B) where:
 * - A: (M, K)
 * - B: (K, N)
 * - C: (M, N)
 * 
 * @param A Input matrix A (device pointer)
 * @param B Input matrix B (device pointer)
 * @param C Output matrix C (device pointer)
 * @param M Number of rows in A and C
 * @param K Number of columns in A, rows in B
 * @param N Number of columns in B and C
 * @param alpha Scaling factor
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for element-wise addition
 * 
 * Computes C = A + B (element-wise)
 * 
 * @param A Input array A (device pointer)
 * @param B Input array B (device pointer)
 * @param C Output array C (device pointer)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_add_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for element-wise multiplication
 * 
 * Computes C = A * B (element-wise)
 * 
 * @param A Input array A (device pointer)
 * @param B Input array B (device pointer)
 * @param C Output array C (device pointer)
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_multiply_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for scalar multiplication
 * 
 * Computes C = A * scalar
 * 
 * @param A Input array A (device pointer)
 * @param C Output array C (device pointer)
 * @param scalar Scalar value
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_scalar_multiply_kernel(
    const float* A,
    float* C,
    float scalar,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for in-place scalar multiplication
 * 
 * Computes data = data * scalar (in-place)
 * 
 * @param data Input/output array (device pointer)
 * @param scalar Scalar value
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_scalar_multiply_inplace_kernel(
    float* data,
    float scalar,
    size_t size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for NaN/Inf detection
 * 
 * Checks if any element in the tensor is NaN or Inf
 * 
 * @param data Input array (device pointer)
 * @param size Number of elements
 * @param has_overflow_host Output flag (host pointer)
 * @return cudaSuccess on success
 */
cudaError_t launch_check_inf_nan_kernel(
    const float* data,
    size_t size,
    bool* has_overflow_host
);

/**
 * @brief CUDA kernel launcher for matrix transpose
 * 
 * Computes C = A^T where A: (rows, cols), C: (cols, rows)
 * 
 * @param A Input matrix A (device pointer)
 * @param C Output matrix C (device pointer)
 * @param rows Number of rows in A
 * @param cols Number of columns in A
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_transpose_kernel(
    const float* A,
    float* C,
    size_t rows,
    size_t cols,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for LoRA gradient computation (grad_A)
 * 
 * Computes gradient w.r.t. A matrix in LoRA backward pass
 * grad_A = B.T @ (input.T @ (grad_output * scaling))
 * 
 * @param input Cached input from forward pass (batch_size, in_dim)
 * @param B LoRA B matrix (in_dim, rank)
 * @param grad_output Gradient from next layer (batch_size, out_dim)
 * @param grad_A Output gradient (rank, out_dim)
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 * @param stream CUDA stream for async execution
 */
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
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for LoRA gradient computation (grad_B)
 * 
 * Computes gradient w.r.t. B matrix in LoRA backward pass
 * grad_B = (grad_output * scaling @ A.T) @ input.T
 * 
 * @param input Cached input from forward pass (batch_size, in_dim)
 * @param A LoRA A matrix (rank, out_dim)
 * @param grad_output Gradient from next layer (batch_size, out_dim)
 * @param grad_B Output gradient (in_dim, rank)
 * @param batch_size Batch size
 * @param in_dim Input dimension
 * @param rank LoRA rank
 * @param out_dim Output dimension
 * @param scaling Scaling factor
 * @param stream CUDA stream for async execution
 */
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
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for MSE loss reduction
 * 
 * Computes partial sums of squared differences for MSE loss calculation.
 * Uses parallel reduction with shared memory for efficiency.
 * 
 * @param predictions Predictions tensor (device pointer)
 * @param targets Target tensor (device pointer)
 * @param partial_sums Output partial sums (device pointer, size = num_blocks)
 * @param n Number of elements
 * @param num_blocks Number of blocks to use for reduction
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_mse_loss_reduction_kernel(
    const float* predictions,
    const float* targets,
    float* partial_sums,
    int n,
    int num_blocks,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for MSE gradient computation
 * 
 * Computes gradient of MSE loss: grad = (2/n) * (predictions - targets)
 * 
 * @param grad_output Output gradient tensor (device pointer)
 * @param predictions Predictions tensor (device pointer)
 * @param targets Target tensor (device pointer)
 * @param scale Scaling factor (2.0 / n)
 * @param n Number of elements
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_mse_gradient_kernel(
    float* grad_output,
    const float* predictions,
    const float* targets,
    float scale,
    int n,
    cudaStream_t stream = nullptr
);

/**
 * @brief cuBLAS handle manager
 */
class CublasHandle {
public:
    CublasHandle();
    ~CublasHandle();
    
    // Disable copy, allow move
    CublasHandle(const CublasHandle&) = delete;
    CublasHandle& operator=(const CublasHandle&) = delete;
    CublasHandle(CublasHandle&&) noexcept;
    CublasHandle& operator=(CublasHandle&&) noexcept;
    
    cublasHandle_t get() const { return handle_; }
    bool is_valid() const { return handle_ != nullptr; }

private:
    cublasHandle_t handle_ = nullptr;
};

/**
 * @brief Matrix multiplication using cuBLAS
 * 
 * Higher-level wrapper that uses cuBLAS for optimal GEMM performance
 * Supports tensor cores on Ampere+ GPUs
 */
cudaError_t cublas_matmul(
    cublasHandle_t handle,
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha = 1.0f,
    float beta = 0.0f
);

/**
 * @brief CUDA kernel launcher for embedding lookup
 * 
 * Looks up embeddings for given token IDs from embedding matrix.
 * Input: token_ids [batch_size, seq_len] (float tensor, will be cast to int)
 * Output: embeddings [batch_size, seq_len, hidden_dim]
 * 
 * @param output Output embeddings tensor (device pointer)
 * @param token_ids Input token IDs (device pointer, stored as floats)
 * @param embedding_weights Embedding weight matrix [vocab_size, hidden_dim] (device pointer)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden/embedding dimension
 * @param vocab_size Vocabulary size (for bounds checking)
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_embedding_lookup_kernel(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    size_t vocab_size,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for sequence mean reduction
 * 
 * Computes mean over sequence dimension:
 * Input: [batch_size, seq_len, hidden_dim]
 * Output: [batch_size, hidden_dim]
 * 
 * @param output Output tensor (device pointer)
 * @param input Input tensor (device pointer)
 * @param batch_size Batch size
 * @param seq_len Sequence length
 * @param hidden_dim Hidden dimension
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_sequence_mean_kernel(
    float* output,
    const float* input,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    cudaStream_t stream = nullptr
);

/**
 * @brief CUDA kernel launcher for SGD parameter update
 * 
 * Performs in-place SGD parameter update on GPU:
 * param = param - learning_rate * grad
 * 
 * This avoids CPU roundtrip for efficient parameter updates.
 * 
 * @param params Parameter tensor (device pointer, in/out)
 * @param grads Gradient tensor (device pointer)
 * @param learning_rate Learning rate
 * @param size Number of elements
 * @param stream CUDA stream for async execution
 */
cudaError_t launch_sgd_update_kernel(
    float* params,
    const float* grads,
    float learning_rate,
    size_t size,
    cudaStream_t stream = nullptr
);

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
