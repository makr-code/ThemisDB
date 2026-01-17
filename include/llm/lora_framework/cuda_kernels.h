#pragma once

#ifdef THEMIS_ENABLE_CUDA

#include <cuda_runtime.h>
#include <cublas_v2.h>
#include <cstddef>

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

} // namespace cuda
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_CUDA
