#pragma once

#ifdef THEMIS_ENABLE_HIP

#include <hip/hip_runtime.h>
#include <rocblas/rocblas.h>
#include <cstddef>

namespace themis {
namespace llm {
namespace lora {
namespace hip {

/**
 * @brief HIP kernel launcher for matrix multiplication
 * 
 * Computes C = alpha * (A @ B) where:
 * - A: (M, K)
 * - B: (K, N)
 * - C: (M, N)
 */
hipError_t launch_matmul_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha,
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for element-wise addition
 */
hipError_t launch_add_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for element-wise multiplication
 */
hipError_t launch_multiply_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for scalar multiplication
 */
hipError_t launch_scalar_multiply_kernel(
    const float* A,
    float* C,
    float scalar,
    size_t size,
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for matrix transpose
 */
hipError_t launch_transpose_kernel(
    const float* A,
    float* C,
    size_t rows,
    size_t cols,
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for LoRA gradient computation (grad_A)
 */
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
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for LoRA gradient computation (grad_B)
 */
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
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for MSE loss reduction
 * 
 * Computes partial sums of squared differences for MSE loss calculation.
 * Uses parallel reduction with shared memory for efficiency.
 * 
 * @param predictions Predictions tensor (device pointer)
 * @param targets Target tensor (device pointer)
 * @param partial_sums Output partial sums (device pointer, size = num_blocks)
 * @param n Number of elements
 * @param num_blocks Number of blocks to use for reduction
 * @param stream HIP stream for async execution
 */
hipError_t launch_mse_loss_reduction_kernel(
    const float* predictions,
    const float* targets,
    float* partial_sums,
    int n,
    int num_blocks,
    hipStream_t stream = nullptr
);

/**
 * @brief HIP kernel launcher for MSE gradient computation
 * 
 * Computes gradient of MSE loss: grad = (2/n) * (predictions - targets)
 * 
 * @param grad_output Output gradient tensor (device pointer)
 * @param predictions Predictions tensor (device pointer)
 * @param targets Target tensor (device pointer)
 * @param scale Scaling factor (2.0 / n)
 * @param n Number of elements
 * @param stream HIP stream for async execution
 */
hipError_t launch_mse_gradient_kernel(
    float* grad_output,
    const float* predictions,
    const float* targets,
    float scale,
    int n,
    hipStream_t stream = nullptr
);

/**
 * @brief rocBLAS handle manager
 */
class RocblasHandle {
public:
    RocblasHandle();
    ~RocblasHandle();
    
    // Disable copy, allow move
    RocblasHandle(const RocblasHandle&) = delete;
    RocblasHandle& operator=(const RocblasHandle&) = delete;
    RocblasHandle(RocblasHandle&&) noexcept;
    RocblasHandle& operator=(RocblasHandle&&) noexcept;
    
    rocblas_handle get() const { return handle_; }
    bool is_valid() const { return handle_ != nullptr; }

private:
    rocblas_handle handle_ = nullptr;
};

/**
 * @brief Matrix multiplication using rocBLAS
 */
hipError_t rocblas_matmul(
    rocblas_handle handle,
    const float* A,
    const float* B,
    float* C,
    size_t M,
    size_t K,
    size_t N,
    float alpha = 1.0f,
    float beta = 0.0f
);

} // namespace hip
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_HIP
