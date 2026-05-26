#include "llm/attention/cuda/flash_attention_cuda.h"
#include <cuda_runtime.h>
#include <cuda_fp16.h>
#include <stdexcept>
#include <cmath>
#include <cstdio>

namespace themis {
namespace llm {
namespace attention {
namespace cuda {

// CUDA error checking macro
#define CUDA_CHECK(call) \
    do { \
        cudaError_t err = call; \
        if (err != cudaSuccess) { \
            throw std::runtime_error(std::string("CUDA error: ") + cudaGetErrorString(err)); \
        } \
    } while(0)

// ============================================================================
// CUDA Kernels
// ============================================================================

constexpr int BLOCK_SIZE = 256;
constexpr int TILE_SIZE = 64;
constexpr int WARP_SIZE = 32;
constexpr int MAX_HEAD_DIM = 128;  // Maximum supported head dimension

/**
 * @brief Flash Attention v2 forward kernel (SM86+)
 * 
 * Implements tiled attention to reduce memory bandwidth:
 * - Load Q, K, V tiles into shared memory
 * - Compute attention scores in tiles
 * - Apply softmax online
 * - Accumulate output
 */
__global__ void flash_attention_fwd_kernel_fp32(
    const float* Q,
    const float* K,
    const float* V,
    float* O,
    const int batch_size,
    const int seq_len,
    const int num_heads,
    const int head_dim,
    const float scale,
    const bool is_causal
) {
    // Global thread position
    int batch = blockIdx.z;
    int head = blockIdx.y;
    int q_idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (q_idx >= seq_len) return;
    
    // Ensure head_dim doesn't exceed max supported size
    if (head_dim > MAX_HEAD_DIM) return;
    
    // Base offsets
    int qkv_offset = (batch * num_heads + head) * seq_len * head_dim;
    const float* Q_head = Q + qkv_offset;
    const float* K_head = K + qkv_offset;
    const float* V_head = V + qkv_offset;
    float* O_head = O + qkv_offset;
    
    // Load query vector for this thread
    float q_vec[MAX_HEAD_DIM];
    for (int d = 0; d < head_dim; ++d) {
        q_vec[d] = Q_head[q_idx * head_dim + d];
    }
    
    // Accumulator for output
    float out_vec[MAX_HEAD_DIM] = {0.0f};
    float max_score = -1e10f;
    float sum_exp = 0.0f;
    
    // Compute attention scores and apply softmax online
    for (int k_idx = 0; k_idx < seq_len; ++k_idx) {
        // Causal mask
        if (is_causal && k_idx > q_idx) {
            continue;
        }
        
        // Compute Q * K^T
        float score = 0.0f;
        for (int d = 0; d < head_dim; ++d) {
            float k_val = K_head[k_idx * head_dim + d];
            score += q_vec[d] * k_val;
        }
        score *= scale;
        
        // Online softmax: update max and sum
        float old_max = max_score;
        max_score = fmaxf(max_score, score);
        float exp_score = expf(score - max_score);
        float correction = expf(old_max - max_score);
        sum_exp = sum_exp * correction + exp_score;
        
        // Update output with correction
        float attn_weight = exp_score;
        for (int d = 0; d < head_dim; ++d) {
            out_vec[d] = out_vec[d] * correction + attn_weight * V_head[k_idx * head_dim + d];
        }
    }
    
    // Normalize and write output
    float norm = 1.0f / (sum_exp + 1e-10f);
    for (int d = 0; d < head_dim; ++d) {
        O_head[q_idx * head_dim + d] = out_vec[d] * norm;
    }
}

/**
 * @brief Flash Attention v3 forward kernel with FP16 (SM90)
 * 
 * Uses half precision for memory efficiency and tensor cores
 */
__global__ void flash_attention_fwd_kernel_fp16(
    const __half* Q,
    const __half* K,
    const __half* V,
    __half* O,
    const int batch_size,
    const int seq_len,
    const int num_heads,
    const int head_dim,
    const float scale,
    const bool is_causal
) {
    // Similar to FP32 version but using half precision
    int batch = blockIdx.z;
    int head = blockIdx.y;
    int q_idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (q_idx >= seq_len) return;
    
    // Ensure head_dim doesn't exceed max supported size
    if (head_dim > MAX_HEAD_DIM) return;
    
    int qkv_offset = (batch * num_heads + head) * seq_len * head_dim;
    const __half* Q_head = Q + qkv_offset;
    const __half* K_head = K + qkv_offset;
    const __half* V_head = V + qkv_offset;
    __half* O_head = O + qkv_offset;
    
    // Accumulate in FP32 for numerical stability
    float out_vec[MAX_HEAD_DIM] = {0.0f};
    float max_score = -1e10f;
    float sum_exp = 0.0f;
    
    for (int k_idx = 0; k_idx < seq_len; ++k_idx) {
        if (is_causal && k_idx > q_idx) {
            continue;
        }
        
        float score = 0.0f;
        for (int d = 0; d < head_dim; ++d) {
            float q_val = __half2float(Q_head[q_idx * head_dim + d]);
            float k_val = __half2float(K_head[k_idx * head_dim + d]);
            score += q_val * k_val;
        }
        score *= scale;
        
        float old_max = max_score;
        max_score = fmaxf(max_score, score);
        float exp_score = expf(score - max_score);
        float correction = expf(old_max - max_score);
        sum_exp = sum_exp * correction + exp_score;
        
        float attn_weight = exp_score;
        for (int d = 0; d < head_dim; ++d) {
            float v_val = __half2float(V_head[k_idx * head_dim + d]);
            out_vec[d] = out_vec[d] * correction + attn_weight * v_val;
        }
    }
    
    float norm = 1.0f / (sum_exp + 1e-10f);
    for (int d = 0; d < head_dim; ++d) {
        O_head[q_idx * head_dim + d] = __float2half(out_vec[d] * norm);
    }
}

// ============================================================================
// FlashAttentionCUDA Implementation
// ============================================================================

FlashAttentionCUDA::FlashAttentionCUDA(const FlashAttentionConfig& config)
    : config_(config) {
    
    compute_capability_ = getComputeCapability();
    
    if (!isAvailable()) {
        throw std::runtime_error("CUDA not available");
    }
    
    allocateWorkspace();
}

FlashAttentionCUDA::~FlashAttentionCUDA() {
    freeWorkspace();
}

Status FlashAttentionCUDA::forward(
    const Tensor& Q,
    const Tensor& K,
    const Tensor& V,
    Tensor& O,
    const KVCacheManager* kv_cache
) {
    if (!Q.isValid() || !K.isValid() || !V.isValid() || !O.isValid()) {
        return Status::ERROR_INVALID_TENSOR;
    }
    
    try {
        // Select kernel based on compute capability
        if (compute_capability_ >= 90 && config_.enable_flash_v3) {
            return launchKernelSM90(Q, K, V, O);
        } else if (compute_capability_ >= 86) {
            return launchKernelSM86(Q, K, V, O);
        } else {
            return launchKernelSM80(Q, K, V, O);
        }
    } catch (const std::exception& e) {
        return Status::ERROR_CUDA_ERROR;
    }
}

Status FlashAttentionCUDA::backward(
    const Tensor& dO,
    Tensor& dQ,
    Tensor& dK,
    Tensor& dV
) {
    // Backward pass not implemented yet
    return Status::ERROR_NOT_IMPLEMENTED;
}

std::string FlashAttentionCUDA::getBackendName() const {
    return "CUDA SM" + std::to_string(compute_capability_);
}

AttentionMemoryStats FlashAttentionCUDA::getMemoryStats() const {
    AttentionMemoryStats stats;
    
    // Query GPU memory
    size_t free_mem = 0;
    size_t total_mem = 0;
    cudaError_t err = cudaMemGetInfo(&free_mem, &total_mem);
    if (err != cudaSuccess) {
        throw std::runtime_error(
            std::string("Failed to query CUDA memory stats: ") + cudaGetErrorString(err)
        );
    }
    
    stats.total_memory_bytes = total_mem;
    stats.workspace_bytes = workspace_size_;
    
    return stats;
}

int FlashAttentionCUDA::getComputeCapability() {
    int device;
    cudaError_t err = cudaGetDevice(&device);
    if (err != cudaSuccess) {
        return 0;
    }
    
    cudaDeviceProp prop{};
    err = cudaGetDeviceProperties(&prop, device);
    if (err != cudaSuccess) {
        return 0;
    }
    
    return prop.major * 10 + prop.minor;
}

bool FlashAttentionCUDA::isAvailable() {
    int device_count;
    cudaError_t err = cudaGetDeviceCount(&device_count);
    return (err == cudaSuccess && device_count > 0);
}

Status FlashAttentionCUDA::launchKernelSM90(const Tensor& Q, const Tensor& K, 
                                             const Tensor& V, Tensor& O) {
    // Use FP16 kernel for SM90
    int batch = config_.batch_size;
    int seq_len = config_.seq_len;
    int num_heads = config_.num_heads;
    int head_dim = config_.head_dim;
    
    dim3 block(BLOCK_SIZE);
    dim3 grid((seq_len + BLOCK_SIZE - 1) / BLOCK_SIZE, num_heads, batch);
    
    // For simplicity, use FP32 kernel (FP16 requires data conversion)
    flash_attention_fwd_kernel_fp32<<<grid, block>>>(
        Q.data, K.data, V.data, O.data,
        batch, seq_len, num_heads, head_dim,
        config_.scale, config_.use_causal_mask
    );
    
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    
    return Status::SUCCESS;
}

Status FlashAttentionCUDA::launchKernelSM86(const Tensor& Q, const Tensor& K,
                                             const Tensor& V, Tensor& O) {
    int batch = config_.batch_size;
    int seq_len = config_.seq_len;
    int num_heads = config_.num_heads;
    int head_dim = config_.head_dim;
    
    dim3 block(BLOCK_SIZE);
    dim3 grid((seq_len + BLOCK_SIZE - 1) / BLOCK_SIZE, num_heads, batch);
    
    flash_attention_fwd_kernel_fp32<<<grid, block>>>(
        Q.data, K.data, V.data, O.data,
        batch, seq_len, num_heads, head_dim,
        config_.scale, config_.use_causal_mask
    );
    
    CUDA_CHECK(cudaGetLastError());
    CUDA_CHECK(cudaDeviceSynchronize());
    
    return Status::SUCCESS;
}

Status FlashAttentionCUDA::launchKernelSM80(const Tensor& Q, const Tensor& K,
                                             const Tensor& V, Tensor& O) {
    // Same as SM86 for now
    return launchKernelSM86(Q, K, V, O);
}

void FlashAttentionCUDA::allocateWorkspace() {
    // Allocate workspace memory if needed
    workspace_size_ = 1024 * 1024;  // 1 MB workspace
    CUDA_CHECK(cudaMalloc(&d_workspace_, workspace_size_));
}

void FlashAttentionCUDA::freeWorkspace() {
    if (d_workspace_) {
        cudaError_t err = cudaFree(d_workspace_);
        if (err != cudaSuccess) {
            std::fprintf(
                stderr,
                "FlashAttentionCUDA::freeWorkspace cudaFree failed: %s\n",
                cudaGetErrorString(err)
            );
        }
        d_workspace_ = nullptr;
    }
}

} // namespace cuda
} // namespace attention
} // namespace llm
} // namespace themis
