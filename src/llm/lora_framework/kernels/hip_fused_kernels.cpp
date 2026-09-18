/**
 * @file hip_fused_kernels.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#ifdef THEMIS_ENABLE_HIP

#include "llm/lora_framework/hip_fused_kernels.h"
#include <hip/hip_runtime.h>
#include <algorithm>
#include <limits>

namespace themis {
namespace llm {
namespace lora {
namespace hip {
namespace fused {

// ============================================================================
// Fused Forward Pass Kernel
// ============================================================================

/**
 * @brief Fused lora forward kernel.
 * @param[in] input Input parameter.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @param[in,out] output Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] in_dim Input parameter.
 * @param[in] rank Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] scaling Input parameter.
 * @return Return value.
 * @details Calls: __syncthreads(), min().
 */
__global__ void fused_lora_forward_kernel(
    const float* input,   // [batch_size, in_dim]
    const float* B,       // [in_dim, rank]
    const float* A,       // [rank, out_dim]
    float* output,        // [batch_size, out_dim]
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
) {
    // Shared memory for intermediate results (h = input @ B)
    __shared__ float shared_h[16];
    
    const int TILE_SIZE = 16;
    
    int batch_idx = blockIdx.y;
    int out_idx = blockIdx.x * TILE_SIZE + threadIdx.x;
    
    if (batch_idx >= batch_size || out_idx >= out_dim) {
        return;
    }
    
    // Compute h = input @ B for this batch sample
    // h[rank] = input[in_dim] @ B[in_dim, rank]
    float h_local[32];  // Local storage for h (assumes rank <= 32)
    
    if (rank <= 32) {
        for (int r = 0; r < rank; r++) {
            float sum = 0.0f;
            for (int i = 0; i < in_dim; i++) {
                sum += input[batch_idx * in_dim + i] * B[i * rank + r];
            }
            h_local[r] = sum;
        }
        
        // Compute output = h @ A * scaling
        if (out_idx < out_dim) {
            float result = 0.0f;
            for (int r = 0; r < rank; r++) {
                result += h_local[r] * A[r * out_dim + out_idx];
            }
            output[batch_idx * out_dim + out_idx] = result * scaling;
        }
    } else {
        // For larger ranks, use shared memory tiling
        for (int tile_start = 0; tile_start < rank; tile_start += TILE_SIZE) {
            // Compute partial h in shared memory
            if (threadIdx.x < TILE_SIZE && tile_start + threadIdx.x < rank) {
                int r = tile_start + threadIdx.x;
                float sum = 0.0f;
                for (int i = 0; i < in_dim; i++) {
                    sum += input[batch_idx * in_dim + i] * B[i * rank + r];
                }
                shared_h[threadIdx.x] = sum;
            }
            __syncthreads();
            
            // Accumulate contribution to output
            if (out_idx < out_dim) {
                float partial_result = 0.0f;
                int tile_size = min(TILE_SIZE, static_cast<int>(rank - tile_start));
                for (int i = 0; i < tile_size; i++) {
                    int r = tile_start + i;
                    partial_result += shared_h[i] * A[r * out_dim + out_idx];
                }
                if (tile_start == 0) {
                    output[batch_idx * out_dim + out_idx] = partial_result * scaling;
                } else {
                    output[batch_idx * out_dim + out_idx] += partial_result * scaling;
                }
            }
            __syncthreads();
        }
    }
}

// ============================================================================
// Fused Backward Pass Kernel
// ============================================================================

/**
 * @brief Fused lora backward kernel.
 * @param[in] input Input parameter.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @param[in] grad_output Input parameter.
 * @param[in,out] grad_A Input/output parameter.
 * @param[in,out] grad_B Input/output parameter.
 * @param[in,out] grad_input Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] in_dim Input parameter.
 * @param[in] rank Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] scaling Input parameter.
 * @return Return value.
 * @details Implements fused_lora_backward_kernel without additional internal calls.
 */
__global__ void fused_lora_backward_kernel(
    const float* input,        // [batch_size, in_dim]
    const float* B,            // [in_dim, rank]
    const float* A,            // [rank, out_dim]
    const float* grad_output,  // [batch_size, out_dim]
    float* grad_A,             // [rank, out_dim]
    float* grad_B,             // [in_dim, rank]
    float* grad_input,         // [batch_size, in_dim]
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling
) {
    // This kernel is complex, so we'll compute different gradients in different thread blocks
    // grad_type: 0 = grad_A, 1 = grad_B, 2 = grad_input
    int grad_type = blockIdx.z;
    
    if (grad_type == 0) {
        // Compute grad_A = h^T @ (grad_output * scaling)
        // grad_A[rank, out_dim]
        int r = blockIdx.y * blockDim.y + threadIdx.y;
        int o = blockIdx.x * blockDim.x + threadIdx.x;
        
        if (r >= rank || o >= out_dim) {
          return;
        }
        
        float sum = 0.0f;
        for (size_t b = 0; b < batch_size; b++) {
            // Compute h[r] = input[b] @ B[:, r]
            float h_val = 0.0f;
            for (size_t i = 0; i < in_dim; i++) {
                h_val += input[b * in_dim + i] * B[i * rank + r];
            }
            sum += h_val * grad_output[b * out_dim + o] * scaling;
        }
        grad_A[r * out_dim + o] = sum;
        
    } else if (grad_type == 1) {
        // Compute grad_B = input^T @ (grad_output @ A^T * scaling)
        // grad_B[in_dim, rank]
        int i = blockIdx.y * blockDim.y + threadIdx.y;
        int r = blockIdx.x * blockDim.x + threadIdx.x;
        
        if (i >= in_dim || r >= rank) {
          return;
        }
        
        float sum = 0.0f;
        for (size_t b = 0; b < batch_size; b++) {
            // Compute (grad_output @ A^T)[b, r]
            float temp = 0.0f;
            for (size_t o = 0; o < out_dim; o++) {
                temp += grad_output[b * out_dim + o] * A[r * out_dim + o];
            }
            sum += input[b * in_dim + i] * temp * scaling;
        }
        grad_B[i * rank + r] = sum;
        
    } else if (grad_type == 2) {
        // Compute grad_input = (grad_output @ A^T) @ B^T * scaling
        // grad_input[batch_size, in_dim]
        int b = blockIdx.y * blockDim.y + threadIdx.y;
        int i = blockIdx.x * blockDim.x + threadIdx.x;
        
        if (b >= batch_size || i >= in_dim) {
          return;
        }
        
        float sum = 0.0f;
        for (size_t r = 0; r < rank; r++) {
            // Compute (grad_output @ A^T)[b, r]
            float temp = 0.0f;
            for (size_t o = 0; o < out_dim; o++) {
                temp += grad_output[b * out_dim + o] * A[r * out_dim + o];
            }
            sum += temp * B[i * rank + r];
        }
        grad_input[b * in_dim + i] = sum * scaling;
    }
}

// ============================================================================
// Fused SGD Optimizer Kernel
// ============================================================================

/**
 * @brief Fused sgd step kernel.
 * @param[in,out] params Input/output parameter.
 * @param[in] grads Input parameter.
 * @param[in,out] momentum_buffer Input/output parameter.
 * @param[in] size Input parameter.
 * @param[in] learning_rate Input parameter.
 * @param[in] momentum Input parameter.
 * @param[in] weight_decay Input parameter.
 * @return Return value.
 * @details Implements fused_sgd_step_kernel without additional internal calls.
 */
__global__ void fused_sgd_step_kernel(
    float* params,
    const float* grads,
    float* momentum_buffer,
    size_t size,
    float learning_rate,
    float momentum,
    float weight_decay
) {
    size_t idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (idx >= size) {
      return;
    }
    
    float p = params[idx];
    float g = grads[idx];
    
    if (momentum_buffer != nullptr && momentum > 0.0f) {
        // With momentum
        float v = momentum_buffer[idx];
        v = momentum * v + (1.0f - momentum) * g;
        momentum_buffer[idx] = v;
        p = p - learning_rate * (v + weight_decay * p);
    } else {
        // Without momentum
        p = p - learning_rate * (g + weight_decay * p);
    }
    
    params[idx] = p;
}

/**
 * @brief Fused mse loss gradient kernel.
 * @param[in,out] grad_output Input/output parameter.
 * @param[in,out] partial_loss Input/output parameter.
 * @param[in] predictions Input parameter.
 * @param[in] targets Input parameter.
 * @param[in] n Input parameter.
 * @return Return value.
 * @details Calls: __syncthreads().
 */
__global__ void fused_mse_loss_gradient_kernel(
    float* grad_output,
    float* partial_loss,
    const float* predictions,
    const float* targets,
    int n
) {
    __shared__ float shared_loss[256];
    
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int tid = threadIdx.x;
    
    // Compute gradient AND accumulate loss in single pass
    float local_loss = 0.0f;
    float scale = 2.0f / n;
    
    for (int i = idx; i < n; i += blockDim.x * gridDim.x) {
        float diff = predictions[i] - targets[i];
        
        // Write gradient (element-wise operation)
        grad_output[i] = scale * diff;
        
        // Accumulate loss
        local_loss += diff * diff;
    }
    
    // Reduce loss within block using shared memory
    shared_loss[tid] = local_loss;
    __syncthreads();
    
    // Tree reduction in shared memory
    for (int s = blockDim.x / 2; s > 64; s >>= 1) {
        if (tid < s) {
            shared_loss[tid] += shared_loss[tid + s];
        }
        __syncthreads();
    }
    
    // Final wavefront reduction (no sync needed, wavefront size = 64 for AMD)
    if (tid < 64) {
        volatile float* smem = shared_loss;
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
        partial_loss[blockIdx.x] = shared_loss[0];
    }
}

// ============================================================================
// Kernel Launchers
// ============================================================================

/**
 * @brief Launch fused lora forward.
 * @param[in] input Input parameter.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @param[in,out] output Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] in_dim Input parameter.
 * @param[in] rank Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] scaling Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 * @details Calls: max(), blockDim(), gridDim(), hipLaunchKernelGGL(), hipGetLastError().
 */
hipError_t launch_fused_lora_forward(
    const float* input,
    const float* B,
    const float* A,
    float* output,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    hipStream_t stream
) {
    if (input == nullptr || B == nullptr || A == nullptr || output == nullptr) {
        return hipErrorInvalidValue;
    }
    if (batch_size == 0 || in_dim == 0 || rank == 0 || out_dim == 0) {
        return hipErrorInvalidValue;
    }
    if (batch_size > static_cast<size_t>(std::numeric_limits<unsigned int>::max()) ||
        out_dim > static_cast<size_t>(std::numeric_limits<unsigned int>::max())) {
        return hipErrorInvalidValue;
    }

    const int TILE_SIZE = 16;
    dim3 blockDim(TILE_SIZE, 1);
    dim3 gridDim((out_dim + TILE_SIZE - 1) / TILE_SIZE, batch_size);
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(fused_lora_forward_kernel, gridDim, blockDim, 0, stream,
            input, B, A, output, batch_size, in_dim, rank, out_dim, scaling);
    } else {
        hipLaunchKernelGGL(fused_lora_forward_kernel, gridDim, blockDim, 0, 0,
            input, B, A, output, batch_size, in_dim, rank, out_dim, scaling);
    }
    
    return hipGetLastError();
}

/**
 * @brief Launch fused lora backward.
 * @param[in] input Input parameter.
 * @param[in] B Input parameter.
 * @param[in] A Input parameter.
 * @param[in] grad_output Input parameter.
 * @param[in,out] grad_A Input/output parameter.
 * @param[in,out] grad_B Input/output parameter.
 * @param[in,out] grad_input Input/output parameter.
 * @param[in] batch_size Input parameter.
 * @param[in] in_dim Input parameter.
 * @param[in] rank Input parameter.
 * @param[in] out_dim Input parameter.
 * @param[in] scaling Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 * @details Calls: max(), blockDim(), gridDim(), std::max(), hipLaunchKernelGGL(), hipGetLastError().
 */
hipError_t launch_fused_lora_backward(
    const float* input,
    const float* B,
    const float* A,
    const float* grad_output,
    float* grad_A,
    float* grad_B,
    float* grad_input,
    size_t batch_size,
    size_t in_dim,
    size_t rank,
    size_t out_dim,
    float scaling,
    hipStream_t stream
) {
    if (input == nullptr || B == nullptr || A == nullptr || grad_output == nullptr ||
        grad_A == nullptr || grad_B == nullptr || grad_input == nullptr) {
        return hipErrorInvalidValue;
    }
    if (batch_size == 0 || in_dim == 0 || rank == 0 || out_dim == 0) {
        return hipErrorInvalidValue;
    }
    if (batch_size > static_cast<size_t>(std::numeric_limits<unsigned int>::max()) ||
        in_dim > static_cast<size_t>(std::numeric_limits<unsigned int>::max()) ||
        rank > static_cast<size_t>(std::numeric_limits<unsigned int>::max()) ||
        out_dim > static_cast<size_t>(std::numeric_limits<unsigned int>::max())) {
        return hipErrorInvalidValue;
    }

    dim3 blockDim(16, 16);
    
    // Launch with 3D grid for different gradient types
    dim3 gridDim((std::max({out_dim, rank, in_dim}) + 15) / 16,
                 (std::max({rank, in_dim, batch_size}) + 15) / 16,
                 3);  // 3 gradient types
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(fused_lora_backward_kernel, gridDim, blockDim, 0, stream,
            input, B, A, grad_output, grad_A, grad_B, grad_input,
            batch_size, in_dim, rank, out_dim, scaling);
    } else {
        hipLaunchKernelGGL(fused_lora_backward_kernel, gridDim, blockDim, 0, 0,
            input, B, A, grad_output, grad_A, grad_B, grad_input,
            batch_size, in_dim, rank, out_dim, scaling);
    }
    
    return hipGetLastError();
}

/**
 * @brief Launch fused sgd step.
 * @param[in,out] params Input/output parameter.
 * @param[in] grads Input parameter.
 * @param[in,out] momentum_buffer Input/output parameter.
 * @param[in] size Input parameter.
 * @param[in] learning_rate Input parameter.
 * @param[in] momentum Input parameter.
 * @param[in] weight_decay Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 * @details Calls: max(), hipLaunchKernelGGL(), dim3(), hipGetLastError().
 */
hipError_t launch_fused_sgd_step(
    float* params,
    const float* grads,
    float* momentum_buffer,
    size_t size,
    float learning_rate,
    float momentum,
    float weight_decay,
    hipStream_t stream
) {
    if (params == nullptr || grads == nullptr || size == 0) {
        return hipErrorInvalidValue;
    }
    if (size > static_cast<size_t>(std::numeric_limits<int>::max())) {
        return hipErrorInvalidValue;
    }

    int blockSize = 256;
    int gridSize = (size + blockSize - 1) / blockSize;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(fused_sgd_step_kernel, dim3(gridSize), dim3(blockSize), 0, stream,
            params, grads, momentum_buffer, size, learning_rate, momentum, weight_decay);
    } else {
        hipLaunchKernelGGL(fused_sgd_step_kernel, dim3(gridSize), dim3(blockSize), 0, 0,
            params, grads, momentum_buffer, size, learning_rate, momentum, weight_decay);
    }
    
    return hipGetLastError();
}

/**
 * @brief Launch fused mse loss gradient.
 * @param[in,out] grad_output Input/output parameter.
 * @param[in,out] partial_loss Input/output parameter.
 * @param[in] predictions Input parameter.
 * @param[in] targets Input parameter.
 * @param[in] n Input parameter.
 * @param[in] num_blocks Input parameter.
 * @param[in] stream Input parameter.
 * @return Return value.
 * @details Calls: hipLaunchKernelGGL(), dim3(), hipGetLastError().
 */
hipError_t launch_fused_mse_loss_gradient(
    float* grad_output,
    float* partial_loss,
    const float* predictions,
    const float* targets,
    int n,
    int num_blocks,
    hipStream_t stream
) {
    if (grad_output == nullptr || partial_loss == nullptr || predictions == nullptr || targets == nullptr) {
        return hipErrorInvalidValue;
    }
    if (n <= 0 || num_blocks <= 0) {
        return hipErrorInvalidValue;
    }

    int threads = 256;
    int blocks = num_blocks;
    
    if (stream != nullptr) {
        hipLaunchKernelGGL(fused_mse_loss_gradient_kernel, dim3(blocks), dim3(threads), 0, stream,
            grad_output, partial_loss, predictions, targets, n);
    } else {
        hipLaunchKernelGGL(fused_mse_loss_gradient_kernel, dim3(blocks), dim3(threads), 0, 0,
            grad_output, partial_loss, predictions, targets, n);
    }
    
    return hipGetLastError();
}

} // namespace fused
} // namespace hip
} // namespace lora
} // namespace llm
} // namespace themis

#endif // THEMIS_ENABLE_HIP
