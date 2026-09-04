/**
 * @file infini_attention_cpu.cpp
 * @brief Infini-attention CPU fallback implementation (P2-GATE-02 reference)
 *
 * Provides CPU-based Infini-attention for:
 * 1. Numeric validation against CUDA kernel
 * 2. Testing and debugging
 * 3. Fallback on systems without CUDA
 *
 * Gate P2-GATE-02: Numeric consistency validation CPU ↔ CUDA
 *
 * @author Copilot Coding Agent
 * @date 2026-07-22
 */

#include "llm/attention/cuda/infini_attention_cuda.h"
#include <cmath>
#include <algorithm>

namespace themis {
namespace llm {
namespace attention {

/**
 * @brief CPU implementation of Infini-attention (reference for validation)
 *
 * Used for P2-GATE-02: numeric consistency testing CPU ↔ CUDA kernels.
 */
class InfiniAttentionCPU {
public:
    explicit InfiniAttentionCPU(const InfiniAttentionConfig& config)
        : config_(config),
          memory_(config.memory_dim * config.memory_dim, 0.0f) {
    }
    
    /**
     * @brief CPU forward pass for testing
     */
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O) {
        
        if (!Q.isValid() || !K.isValid() || !V.isValid() || !O.isValid()) {
            return Status::ERROR_INVALID_TENSOR;
        }
        
        // Extract dimensions
        size_t batch_size = Q.shape[0];
        size_t seq_len = Q.shape[1];
        size_t num_heads = Q.shape[2];
        size_t head_dim = Q.shape[3];
        
        // Step 1: Compute local attention (simplified Flash Attention)
        std::vector<float> O_local(O.size, 0.0f);
        computeLocalAttentionCPU(Q, K, V, O_local, batch_size, seq_len, num_heads, head_dim);
        
        // Step 2: Update memory
        updateMemoryCPU(K, V, batch_size, seq_len, head_dim);
        
        // Copy output
        std::copy(O_local.begin(), O_local.end(), (float*)O.data);
        
        return Status::SUCCESS;
    }
    
    /**
     * @brief Get compressive memory (for comparison with CUDA)
     */
    std::vector<float> getMemory() const {
        return memory_;
    }
    
    /**
     * @brief Reset memory
     */
    void resetMemory() {
        std::fill(memory_.begin(), memory_.end(), 0.0f);
    }

private:
    InfiniAttentionConfig config_;
    std::vector<float> memory_;  // [memory_dim x memory_dim]
    
    /**
     * @brief Sigmoid activation function
     */
    static float sigmoid([[maybe_unused]] float x) {
        return 1.0f / (1.0f + std::exp(-x));
    }
    
    /**
     * @brief Compute local attention with simple softmax
     *
     * O[i,j,h,d] = sum_k( softmax(Q[i,j,h,:] @ K[i,k,h,:]^T) * V[i,k,h,:] )
     */
    void computeLocalAttentionCPU(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        std::vector<float>& O,
        size_t batch_size,
        size_t seq_len,
        size_t num_heads,
        size_t head_dim) {
        
        const float* Q_ptr = (const float*)Q.data;
        const float* K_ptr = (const float*)K.data;
        const float* V_ptr = (const float*)V.data;
        
        // For each query position
        for (size_t b = 0; b < batch_size; ++b) {
            for (size_t q_pos = 0; q_pos < seq_len; ++q_pos) {
                for (size_t h = 0; h < num_heads; ++h) {
                    // Compute attention scores for this query
                    std::vector<float> scores(seq_len, 0.0f);
                    float max_score = -std::numeric_limits<float>::infinity();
                    
                    for (size_t k_pos = 0; k_pos < seq_len; ++k_pos) {
                        // Q[b,q_pos,h,:] @ K[b,k_pos,h,:]^T
                        float score = 0.0f;
                        for (size_t d = 0; d < head_dim; ++d) {
                            size_t q_idx = b * seq_len * num_heads * head_dim +
                                          q_pos * num_heads * head_dim + h * head_dim + d;
                            size_t k_idx = b * seq_len * num_heads * head_dim +
                                          k_pos * num_heads * head_dim + h * head_dim + d;
                            score += Q_ptr[q_idx] * K_ptr[k_idx];
                        }
                        
                        // Causal masking: k_pos > q_pos not allowed
                        if (k_pos > q_pos) {
                            score = -std::numeric_limits<float>::infinity();
                        }
                        
                        scores[k_pos] = score;
                        max_score = std::max(max_score, score);
                    }
                    
                    // Compute softmax (with numerical stability)
                    std::vector<float> attn_weights(seq_len);
                    float weight_sum = 0.0f;
                    
                    for (size_t k_pos = 0; k_pos < seq_len; ++k_pos) {
                        if (std::isinf(scores[k_pos]) && scores[k_pos] < 0) {
                            attn_weights[k_pos] = 0.0f;
                        } else {
                            attn_weights[k_pos] = std::exp(scores[k_pos] - max_score);
                            weight_sum += attn_weights[k_pos];
                        }
                    }
                    
                    // Normalize weights
                    if (weight_sum > 1e-6f) {
                        for (auto& w : attn_weights) {
                            w /= weight_sum;
                        }
                    }
                    
                    // Compute output: sum over values
                    for (size_t d = 0; d < head_dim; ++d) {
                        float out_val = 0.0f;
                        
                        for (size_t k_pos = 0; k_pos < seq_len; ++k_pos) {
                            size_t v_idx = b * seq_len * num_heads * head_dim +
                                          k_pos * num_heads * head_dim + h * head_dim + d;
                            out_val += attn_weights[k_pos] * V_ptr[v_idx];
                        }
                        
                        size_t o_idx = b * seq_len * num_heads * head_dim +
                                      q_pos * num_heads * head_dim + h * head_dim + d;
                        O[o_idx] = out_val;
                    }
                }
            }
        }
    }
    
    /**
     * @brief Update compressive memory M' = M + α * σ(K * V^T)
     */
    void updateMemoryCPU(
        const Tensor& K,
        const Tensor& V,
        size_t batch_size,
        size_t seq_len,
        size_t head_dim) {
        
        const float* K_ptr = (const float*)K.data;
        const float* V_ptr = (const float*)V.data;
        
        // Aggregate K, V over sequence (mean pooling)
        std::vector<float> agg_k(head_dim, 0.0f);
        std::vector<float> agg_v(head_dim, 0.0f);
        
        for (size_t b = 0; b < batch_size; ++b) {
            for (size_t t = 0; t < seq_len; ++t) {
                for (size_t d = 0; d < head_dim && d < config_.memory_dim; ++d) {
                    size_t idx = b * seq_len * head_dim + t * head_dim + d;
                    agg_k[d] += K_ptr[idx];
                    agg_v[d] += V_ptr[idx];
                }
            }
        }
        
        // Normalize
        float count = (float)(batch_size * seq_len);
        for (auto& v : agg_k) {
          v /= count;
        }
        for (auto& v : agg_v) {
          v /= count;
        }
        
        // Update memory: M[i,j] += α * σ(agg_k[i]) * σ(agg_v[j])
        for (size_t i = 0; i < config_.memory_dim; ++i) {
            for (size_t j = 0; j < config_.memory_dim; ++j) {
                float k_val = (i < head_dim) ? agg_k[i] : 0.0f;
                float v_val = (j < head_dim) ? agg_v[j] : 0.0f;
                
                float update = config_.update_rate * sigmoid(k_val) * sigmoid(v_val);
                memory_[i * config_.memory_dim + j] += update;
            }
        }
    }
};

/**
 * @brief Validation helper: compare CPU and CUDA outputs numerically
 *
 * Used by tests to verify P2-GATE-02 compliance.
 * Returns MAPE (Mean Absolute Percentage Error) between outputs.
 */
float validateNumericConsistency(
    const Tensor& Q,
    const Tensor& K,
    const Tensor& V,
    const std::vector<float>& cpu_output,
    const std::vector<float>& cuda_output) {
    
    if (static_cast<int>(cpu_output.size()) != cuda_output.size()) {
        return std::numeric_limits<float>::max();
    }
    
    double total_error = 0.0;
    size_t count = 0;
    
    for (size_t i = 0; i <static_cast<int>(cpu_output.size()); ++i) {
        float cpu_val = cpu_output[i];
        float cuda_val = cuda_output[i];
        
        if (std::abs(cpu_val) > 1e-6f) {
            float error = std::abs(cpu_val - cuda_val) / std::abs(cpu_val);
            total_error += error;
            count++;
        }
    }
    
    return (count > 0) ? static_cast<float>(total_error / count) : 0.0f;
}

} // namespace attention
} // namespace llm
} // namespace themis
