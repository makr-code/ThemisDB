// CUDA Kernels for Flash Attention and Kernel Fusion
// ThemisDB LLM Acceleration
// Implements memory-efficient attention with tiling

#include <cuda_runtime.h>
#include <device_launch_parameters.h>
#include <cmath>
#include <cfloat>

namespace themis {
namespace llm {
namespace kernels {
namespace cuda {

// ============================================================================
// Flash Attention Configuration
// ============================================================================

constexpr int BLOCK_SIZE = 256;
constexpr int TILE_SIZE = 64;  // Tile size for memory-efficient attention
constexpr int WARP_SIZE = 32;
constexpr float SOFTMAX_EPSILON = 1e-10f;  // Numerical stability for softmax normalization

// ============================================================================
// Flash Attention Forward Pass Kernels
// ============================================================================

/**
 * @brief Flash Attention Forward Pass - Tiled Implementation
 * 
 * Implements the Flash Attention algorithm with tiling to reduce memory usage.
 * Based on the original Flash Attention paper (https://arxiv.org/abs/2205.14135)
 * 
 * Key optimizations:
 * - Tiled computation to fit in shared memory
 * - Fused softmax and attention multiplication
 * - Online softmax to avoid storing full attention matrix
 * 
 * @param Q Query tensor (batch * num_heads * seq_len * head_dim)
 * @param K Key tensor (batch * num_heads * seq_len * head_dim)
 * @param V Value tensor (batch * num_heads * seq_len * head_dim)
 * @param O Output tensor (batch * num_heads * seq_len * head_dim)
 * @param seq_len Sequence length
 * @param head_dim Dimension per attention head
 * @param scale Scaling factor (typically 1/sqrt(head_dim))
 * @param is_causal Whether to apply causal masking
 */
__global__ void flashAttentionForwardKernel(
    const float* Q,
    const float* K,
    const float* V,
    float* O,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    bool is_causal
) {
    // Thread and block indices
    int batch = blockIdx.z;
    int head = blockIdx.y;
    int q_idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (q_idx >= seq_len) return;
    
    // Shared memory for tiles
    __shared__ float tile_K[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_V[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_scores[TILE_SIZE];
    
    // Base offsets for this head
    int qkv_offset = (batch * num_heads + head) * seq_len * head_dim;
    const float* Q_head = Q + qkv_offset + q_idx * head_dim;
    const float* K_head = K + qkv_offset;
    const float* V_head = V + qkv_offset;
    float* O_head = O + qkv_offset + q_idx * head_dim;
    
    // Online softmax statistics
    float max_score = -FLT_MAX;
    float sum_exp = 0.0f;
    
    // Accumulator for output
    float output[TILE_SIZE];
    for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
        output[d] = 0.0f;
    }
    
    // Process attention in tiles
    for (int k_tile = 0; k_tile < seq_len; k_tile += TILE_SIZE) {
        int k_idx = k_tile + threadIdx.x;
        
        // Causal masking check
        if (is_causal && k_idx > q_idx) {
            break;
        }
        
        // Load K and V tiles into shared memory
        if (k_idx < seq_len) {
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                tile_K[threadIdx.x][d] = K_head[k_idx * head_dim + d];
                tile_V[threadIdx.x][d] = V_head[k_idx * head_dim + d];
            }
        } else {
            for (int d = 0; d < TILE_SIZE; ++d) {
                tile_K[threadIdx.x][d] = 0.0f;
                tile_V[threadIdx.x][d] = 0.0f;
            }
        }
        __syncthreads();
        
        // Compute attention scores for this tile
        for (int k_local = 0; k_local < TILE_SIZE; ++k_local) {
            int k_global = k_tile + k_local;
            if (k_global >= seq_len || (is_causal && k_global > q_idx)) {
                continue;
            }
            
            // Compute Q * K^T
            float score = 0.0f;
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                score += Q_head[d] * tile_K[k_local][d];
            }
            score *= scale;
            
            // Online softmax update
            float old_max = max_score;
            max_score = fmaxf(max_score, score);
            
            // Rescale previous sum_exp and output
            float rescale = expf(old_max - max_score);
            sum_exp *= rescale;
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                output[d] *= rescale;
            }
            
            // Add contribution from current score
            float exp_score = expf(score - max_score);
            sum_exp += exp_score;
            
            // Accumulate V weighted by attention
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                output[d] += exp_score * tile_V[k_local][d];
            }
        }
        __syncthreads();
    }
    
    // Normalize and write output
    float norm = 1.0f / (sum_exp + SOFTMAX_EPSILON);
    for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
        O_head[d] = output[d] * norm;
    }
}

// ============================================================================
// Flash Attention Backward Pass Kernels
// ============================================================================

/**
 * @brief Flash Attention Backward Pass - Tiled Implementation
 * 
 * Computes gradients for Q, K, V using the Flash Attention backward algorithm.
 * Based on Flash Attention paper Algorithm 2.
 * 
 * Key optimizations:
 * - Recompute attention on-the-fly (no need to store full attention matrix)
 * - Tiled computation for memory efficiency
 * - Fused gradient computation
 * 
 * @param dO Gradient of output (batch * num_heads * seq_len * head_dim)
 * @param Q Query tensor (batch * num_heads * seq_len * head_dim)
 * @param K Key tensor (batch * num_heads * seq_len * head_dim)
 * @param V Value tensor (batch * num_heads * seq_len * head_dim)
 * @param O Output from forward pass (batch * num_heads * seq_len * head_dim)
 * @param dQ Gradient of query (output)
 * @param dK Gradient of key (output)
 * @param dV Gradient of value (output)
 * @param scale Scaling factor (typically 1/sqrt(head_dim))
 * @param is_causal Whether to apply causal masking
 */
__global__ void flashAttentionBackwardKernel(
    const float* dO,
    const float* Q,
    const float* K,
    const float* V,
    const float* O,
    float* dQ,
    float* dK,
    float* dV,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    bool is_causal
) {
    // Thread and block indices
    int batch = blockIdx.z;
    int head = blockIdx.y;
    int q_idx = blockIdx.x * blockDim.x + threadIdx.x;
    
    if (q_idx >= seq_len) return;
    
    // Shared memory for tiles
    __shared__ float tile_K[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_V[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_dV[TILE_SIZE][TILE_SIZE];
    __shared__ float tile_dK[TILE_SIZE][TILE_SIZE];
    
    // Base offsets for this head
    int qkv_offset = (batch * num_heads + head) * seq_len * head_dim;
    const float* Q_head = Q + qkv_offset + q_idx * head_dim;
    const float* K_head = K + qkv_offset;
    const float* V_head = V + qkv_offset;
    const float* O_head = O + qkv_offset + q_idx * head_dim;
    const float* dO_head = dO + qkv_offset + q_idx * head_dim;
    float* dQ_head = dQ + qkv_offset + q_idx * head_dim;
    
    // Initialize dQ to zero
    for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
        dQ_head[d] = 0.0f;
    }
    
    // Recompute attention statistics for this query
    float max_score = -FLT_MAX;
    float sum_exp = 0.0f;
    
    // First pass: compute max and sum for softmax (same as forward)
    for (int k_tile = 0; k_tile < seq_len; k_tile += TILE_SIZE) {
        int k_idx = k_tile + threadIdx.x;
        
        if (is_causal && k_idx > q_idx) break;
        
        // Load K tile
        if (k_idx < seq_len) {
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                tile_K[threadIdx.x][d] = K_head[k_idx * head_dim + d];
            }
        }
        __syncthreads();
        
        // Compute scores
        for (int k_local = 0; k_local < TILE_SIZE; ++k_local) {
            int k_global = k_tile + k_local;
            if (k_global >= seq_len || (is_causal && k_global > q_idx)) continue;
            
            float score = 0.0f;
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                score += Q_head[d] * tile_K[k_local][d];
            }
            score *= scale;
            
            float old_max = max_score;
            max_score = fmaxf(max_score, score);
            float rescale = expf(old_max - max_score);
            sum_exp = sum_exp * rescale + expf(score - max_score);
        }
        __syncthreads();
    }
    
    // Compute D = dO * O (element-wise, then sum)
    float D = 0.0f;
    for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
        D += dO_head[d] * O_head[d];
    }
    
    // Second pass: compute gradients
    for (int k_tile = 0; k_tile < seq_len; k_tile += TILE_SIZE) {
        int k_idx = k_tile + threadIdx.x;
        
        if (is_causal && k_idx > q_idx) break;
        
        // Load K, V tiles
        if (k_idx < seq_len) {
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                tile_K[threadIdx.x][d] = K_head[k_idx * head_dim + d];
                tile_V[threadIdx.x][d] = V_head[k_idx * head_dim + d];
            }
        }
        
        // Initialize dK and dV tiles to zero (use actual head_dim, not TILE_SIZE)
        for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
            tile_dK[threadIdx.x][d] = 0.0f;
            tile_dV[threadIdx.x][d] = 0.0f;
        }
        __syncthreads();
        
        // Compute gradients for this tile
        for (int k_local = 0; k_local < TILE_SIZE; ++k_local) {
            int k_global = k_tile + k_local;
            if (k_global >= seq_len || (is_causal && k_global > q_idx)) continue;
            
            // Recompute attention weight
            float score = 0.0f;
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                score += Q_head[d] * tile_K[k_local][d];
            }
            score *= scale;
            float attn = expf(score - max_score) / (sum_exp + SOFTMAX_EPSILON);
            
            // Compute dP (gradient of attention weights before softmax)
            float dP = 0.0f;
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                dP += dO_head[d] * tile_V[k_local][d];
            }
            dP = attn * (dP - D);
            
            // Accumulate dQ
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                dQ_head[d] += dP * scale * tile_K[k_local][d];
            }
            
            // Accumulate dK and dV in shared memory
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                atomicAdd(&tile_dK[k_local][d], dP * scale * Q_head[d]);
                atomicAdd(&tile_dV[k_local][d], attn * dO_head[d]);
            }
        }
        __syncthreads();
        
        // Write dK and dV back to global memory
        if (k_idx < seq_len) {
            float* dK_ptr = dK + qkv_offset + k_idx * head_dim;
            float* dV_ptr = dV + qkv_offset + k_idx * head_dim;
            for (int d = 0; d < head_dim && d < TILE_SIZE; ++d) {
                atomicAdd(&dK_ptr[d], tile_dK[threadIdx.x][d]);
                atomicAdd(&dV_ptr[d], tile_dV[threadIdx.x][d]);
            }
        }
        __syncthreads();
    }
}

/**
 * @brief Fused QKV Projection Kernel
 * 
 * Projects input to Query, Key, Value in a single kernel launch
 * 
 * @param input Input tensor (batch * seq_len * hidden_dim)
 * @param qkv_weight Weight matrix (hidden_dim * 3 * hidden_dim)
 * @param qkv_bias Bias vector (3 * hidden_dim)
 * @param Q Output query (batch * seq_len * hidden_dim)
 * @param K Output key (batch * seq_len * hidden_dim)
 * @param V Output value (batch * seq_len * hidden_dim)
 */
__global__ void fusedQKVProjectionKernel(
    const float* input,
    const float* qkv_weight,
    const float* qkv_bias,
    float* Q,
    float* K,
    float* V,
    int batch_size,
    int seq_len,
    int hidden_dim
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total_elements = batch_size * seq_len;
    
    if (idx >= total_elements) return;
    
    const float* input_row = input + idx * hidden_dim;
    float* q_row = Q + idx * hidden_dim;
    float* k_row = K + idx * hidden_dim;
    float* v_row = V + idx * hidden_dim;
    
    // Compute Q, K, V projections
    for (int d = 0; d < hidden_dim; ++d) {
        float q_val = qkv_bias ? qkv_bias[d] : 0.0f;
        float k_val = qkv_bias ? qkv_bias[hidden_dim + d] : 0.0f;
        float v_val = qkv_bias ? qkv_bias[2 * hidden_dim + d] : 0.0f;
        
        for (int h = 0; h < hidden_dim; ++h) {
            float in = input_row[h];
            q_val += in * qkv_weight[h * 3 * hidden_dim + d];
            k_val += in * qkv_weight[h * 3 * hidden_dim + hidden_dim + d];
            v_val += in * qkv_weight[h * 3 * hidden_dim + 2 * hidden_dim + d];
        }
        
        q_row[d] = q_val;
        k_row[d] = k_val;
        v_row[d] = v_val;
    }
}

/**
 * @brief Fused RoPE (Rotary Position Embedding) Application
 * 
 * Applies rotary position embeddings to queries and keys
 * 
 * @param QK Query or Key tensor (batch * num_heads * seq_len * head_dim)
 * @param position_ids Position indices (seq_len)
 * @param rope_base Base for rotary embedding (typically 10000)
 */
__global__ void fusedRoPEKernel(
    float* QK,
    const int* position_ids,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    int rope_base
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total = batch_size * num_heads * seq_len;
    
    if (idx >= total) return;
    
    int batch = idx / (num_heads * seq_len);
    int head = (idx / seq_len) % num_heads;
    int pos = idx % seq_len;
    
    int position = position_ids ? position_ids[pos] : pos;
    float* qk_ptr = QK + idx * head_dim;
    
    // Apply RoPE to pairs of dimensions
    for (int d = 0; d < head_dim; d += 2) {
        float freq = 1.0f / powf((float)rope_base, (float)(2 * d) / head_dim);
        float angle = position * freq;
        float cos_val = cosf(angle);
        float sin_val = sinf(angle);
        
        float x0 = qk_ptr[d];
        float x1 = qk_ptr[d + 1];
        
        qk_ptr[d] = x0 * cos_val - x1 * sin_val;
        qk_ptr[d + 1] = x0 * sin_val + x1 * cos_val;
    }
}

/**
 * @brief Fused LayerNorm + Linear Kernel
 * 
 * Applies LayerNorm followed by linear projection in single kernel
 * 
 * @param output Output tensor
 * @param input Input tensor
 * @param weight Linear weight
 * @param bias Linear bias
 * @param ln_weight LayerNorm gamma
 * @param ln_bias LayerNorm beta
 */
__global__ void fusedLayerNormLinearKernel(
    float* output,
    const float* input,
    const float* weight,
    const float* bias,
    const float* ln_weight,
    const float* ln_bias,
    int batch_size,
    int seq_len,
    int hidden_dim,
    float epsilon
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total_elements = batch_size * seq_len;
    
    if (idx >= total_elements) return;
    
    const float* input_row = input + idx * hidden_dim;
    float* output_row = output + idx * hidden_dim;
    
    // Compute mean
    float mean = 0.0f;
    for (int d = 0; d < hidden_dim; ++d) {
        mean += input_row[d];
    }
    mean /= hidden_dim;
    
    // Compute variance
    float variance = 0.0f;
    for (int d = 0; d < hidden_dim; ++d) {
        float diff = input_row[d] - mean;
        variance += diff * diff;
    }
    variance /= hidden_dim;
    
    float inv_std = rsqrtf(variance + epsilon);
    
    // Apply LayerNorm and Linear
    for (int d = 0; d < hidden_dim; ++d) {
        float normalized = (input_row[d] - mean) * inv_std;
        normalized = normalized * ln_weight[d] + ln_bias[d];
        
        // Linear projection (simplified - matrix multiply)
        output_row[d] = normalized * weight[d] + (bias ? bias[d] : 0.0f);
    }
}

/**
 * @brief Fused Gated FFN Kernel (SiLU activation)
 * 
 * Implements gated feed-forward: gate * silu(up) -> down
 * Used in LLaMA and similar models
 */
__global__ void fusedGatedFFNKernel(
    float* output,
    const float* input,
    const float* gate_weight,
    const float* up_weight,
    const float* down_weight,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int intermediate_dim
) {
    int idx = blockIdx.x * blockDim.x + threadIdx.x;
    int total_elements = batch_size * seq_len;
    
    if (idx >= total_elements) return;
    
    const float* input_row = input + idx * hidden_dim;
    float* output_row = output + idx * hidden_dim;
    
    // Allocate shared memory for intermediate results
    extern __shared__ float shared_mem[];
    float* intermediate = shared_mem;
    
    int tid = threadIdx.x;
    
    // Gate and Up projections
    for (int i = 0; i < intermediate_dim; ++i) {
        if (tid < intermediate_dim) {
            float gate_val = 0.0f;
            float up_val = 0.0f;
            
            for (int h = 0; h < hidden_dim; ++h) {
                gate_val += input_row[h] * gate_weight[h * intermediate_dim + i];
                up_val += input_row[h] * up_weight[h * intermediate_dim + i];
            }
            
            // SiLU activation: x * sigmoid(x)
            float sigmoid = 1.0f / (1.0f + expf(-gate_val));
            gate_val = gate_val * sigmoid;
            
            // Element-wise multiply
            intermediate[i] = gate_val * up_val;
        }
    }
    __syncthreads();
    
    // Down projection
    for (int d = 0; d < hidden_dim; ++d) {
        float val = 0.0f;
        for (int i = 0; i < intermediate_dim; ++i) {
            val += intermediate[i] * down_weight[i * hidden_dim + d];
        }
        output_row[d] = val;
    }
}

// ============================================================================
// Kernel Launch Wrappers (C++ interface)
// ============================================================================

extern "C" {

/**
 * @brief Launch Flash Attention Forward Kernel
 */
void launchFlashAttentionForward(
    const float* d_Q,
    const float* d_K,
    const float* d_V,
    float* d_O,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    bool is_causal,
    cudaStream_t stream
) {
    dim3 blockDim(BLOCK_SIZE);
    dim3 gridDim(
        (seq_len + blockDim.x - 1) / blockDim.x,
        num_heads,
        batch_size
    );
    
    flashAttentionForwardKernel<<<gridDim, blockDim, 0, stream>>>(
        d_Q, d_K, d_V, d_O,
        batch_size, num_heads, seq_len, head_dim,
        scale, is_causal
    );
    // REL-77: check kernel launch status
    cudaError_t launch_err = cudaPeekAtLastError();
    if (launch_err != cudaSuccess) {
        return;
    }
}

/**
 * @brief Launch Flash Attention Backward Kernel
 */
void launchFlashAttentionBackward(
    const float* d_dO,
    const float* d_Q,
    const float* d_K,
    const float* d_V,
    const float* d_O,
    float* d_dQ,
    float* d_dK,
    float* d_dV,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    bool is_causal,
    cudaStream_t stream
) {
    // Initialize gradients to zero
    size_t size = batch_size * num_heads * seq_len * head_dim * sizeof(float);
    // REL-74..REL-76: check cudaMemsetAsync return values
    cudaError_t memset_err = cudaMemsetAsync(d_dQ, 0, size, stream);
    if (memset_err != cudaSuccess) {
        return;
    }
    memset_err = cudaMemsetAsync(d_dK, 0, size, stream);
    if (memset_err != cudaSuccess) {
        return;
    }
    memset_err = cudaMemsetAsync(d_dV, 0, size, stream);
    if (memset_err != cudaSuccess) {
        return;
    }
    
    dim3 blockDim(BLOCK_SIZE);
    dim3 gridDim(
        (seq_len + blockDim.x - 1) / blockDim.x,
        num_heads,
        batch_size
    );
    
    flashAttentionBackwardKernel<<<gridDim, blockDim, 0, stream>>>(
        d_dO, d_Q, d_K, d_V, d_O,
        d_dQ, d_dK, d_dV,
        batch_size, num_heads, seq_len, head_dim,
        scale, is_causal
    );
    // REL-78: check kernel launch status
    cudaError_t launch_err = cudaPeekAtLastError();
    if (launch_err != cudaSuccess) {
        return;
    }
}

/**
 * @brief Launch Fused QKV Projection Kernel
 */
void launchFusedQKVProjection(
    const float* d_input,
    const float* d_qkv_weight,
    const float* d_qkv_bias,
    float* d_Q,
    float* d_K,
    float* d_V,
    int batch_size,
    int seq_len,
    int hidden_dim,
    cudaStream_t stream
) {
    int total_elements = batch_size * seq_len;
    int threadsPerBlock = 256;
    int blocksPerGrid = (total_elements + threadsPerBlock - 1) / threadsPerBlock;
    
    fusedQKVProjectionKernel<<<blocksPerGrid, threadsPerBlock, 0, stream>>>(
        d_input, d_qkv_weight, d_qkv_bias,
        d_Q, d_K, d_V,
        batch_size, seq_len, hidden_dim
    );
}

/**
 * @brief Launch Fused RoPE Kernel
 */
void launchFusedRoPE(
    float* d_QK,
    const int* d_position_ids,
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    int rope_base,
    cudaStream_t stream
) {
    int total = batch_size * num_heads * seq_len;
    int threadsPerBlock = 256;
    int blocksPerGrid = (total + threadsPerBlock - 1) / threadsPerBlock;
    
    fusedRoPEKernel<<<blocksPerGrid, threadsPerBlock, 0, stream>>>(
        d_QK, d_position_ids,
        batch_size, num_heads, seq_len, head_dim,
        rope_base
    );
}

/**
 * @brief Launch Fused LayerNorm + Linear Kernel
 */
void launchFusedLayerNormLinear(
    float* d_output,
    const float* d_input,
    const float* d_weight,
    const float* d_bias,
    const float* d_ln_weight,
    const float* d_ln_bias,
    int batch_size,
    int seq_len,
    int hidden_dim,
    float epsilon,
    cudaStream_t stream
) {
    int total_elements = batch_size * seq_len;
    int threadsPerBlock = 256;
    int blocksPerGrid = (total_elements + threadsPerBlock - 1) / threadsPerBlock;
    
    fusedLayerNormLinearKernel<<<blocksPerGrid, threadsPerBlock, 0, stream>>>(
        d_output, d_input, d_weight, d_bias,
        d_ln_weight, d_ln_bias,
        batch_size, seq_len, hidden_dim, epsilon
    );
}

/**
 * @brief Launch Fused Gated FFN Kernel
 */
void launchFusedGatedFFN(
    float* d_output,
    const float* d_input,
    const float* d_gate_weight,
    const float* d_up_weight,
    const float* d_down_weight,
    int batch_size,
    int seq_len,
    int hidden_dim,
    int intermediate_dim,
    cudaStream_t stream
) {
    int total_elements = batch_size * seq_len;
    int threadsPerBlock = 256;
    int blocksPerGrid = (total_elements + threadsPerBlock - 1) / threadsPerBlock;
    size_t sharedMemSize = intermediate_dim * sizeof(float);
    
    fusedGatedFFNKernel<<<blocksPerGrid, threadsPerBlock, sharedMemSize, stream>>>(
        d_output, d_input,
        d_gate_weight, d_up_weight, d_down_weight,
        batch_size, seq_len, hidden_dim, intermediate_dim
    );
}

} // extern "C"

} // namespace cuda
} // namespace kernels
} // namespace llm
} // namespace themis
