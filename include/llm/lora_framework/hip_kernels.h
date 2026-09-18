/**
 * @file hip_kernels.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#ifdef THEMIS_ENABLE_HIP

#include <hip/hip_runtime.h>
#include <rocblas/rocblas.h>
#include <cstddef>

// Kernel configuration constants for consistency
#define THEMIS_GPU_REDUCTION_BLOCK_SIZE 256
#define THEMIS_GPU_REDUCTION_SHARED_MEM_SIZE 256
#define THEMIS_GPU_MAX_BLOCKS 1024

namespace themis {
namespace llm {
namespace lora {
namespace hip {

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

hipError_t launch_add_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    hipStream_t stream = nullptr
);

hipError_t launch_multiply_kernel(
    const float* A,
    const float* B,
    float* C,
    size_t size,
    hipStream_t stream = nullptr
);

hipError_t launch_scalar_multiply_kernel(
    const float* A,
    float* C,
    float scalar,
    size_t size,
    hipStream_t stream = nullptr
);

hipError_t launch_scalar_multiply_inplace_kernel(
    float* data,
    float scalar,
    size_t size,
    hipStream_t stream = nullptr
);

/**
 * @brief Launch check inf nan kernel.
 * @param[in] data Input parameter.
 * @param[in] size Input parameter.
 * @param[in,out] has_overflow_host Input/output parameter.
 * @return Return value.
 */
hipError_t launch_check_inf_nan_kernel(
    const float* data,
    size_t size,
    bool* has_overflow_host
);

hipError_t launch_transpose_kernel(
    const float* A,
    float* C,
    size_t rows,
    size_t cols,
    hipStream_t stream = nullptr
);

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

hipError_t launch_mse_loss_reduction_kernel(
    const float* predictions,
    const float* targets,
    float* partial_sums,
    int n,
    int num_blocks,
    hipStream_t stream = nullptr
);

hipError_t launch_mse_gradient_kernel(
    float* grad_output,
    const float* predictions,
    const float* targets,
    float scale,
    int n,
    hipStream_t stream = nullptr
);

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

hipError_t launch_embedding_lookup_kernel(
    float* output,
    const float* token_ids,
    const float* embedding_weights,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    size_t vocab_size,
    hipStream_t stream = nullptr
);

hipError_t launch_sequence_mean_kernel(
    float* output,
    const float* input,
    size_t batch_size,
    size_t seq_len,
    size_t hidden_dim,
    hipStream_t stream = nullptr
);

hipError_t launch_sgd_update_kernel(
    float* params,
    const float* grads,
    float learning_rate,
    size_t size,
    hipStream_t stream = nullptr
);

} // namespace hip
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_HIP
