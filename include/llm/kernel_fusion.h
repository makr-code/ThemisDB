/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kernel_fusion.h                                    ║
  Version:         0.0.45                                             ║
  Last Modified:   2026-04-15 07:07:17                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     180                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

namespace themis {
namespace llm {
namespace kernels {

/**
 * @brief Fused CUDA Kernels for Performance Optimization
 * 
 * Week 10-12 Implementation: Kernel fusion to reduce memory bandwidth
 * and improve performance by combining multiple operations.
 */

// Fused LayerNorm + Linear + Residual
// Combines 3 operations into 1 kernel to reduce memory passes
void fusedLayerNormLinearResidual(
    float* output,              // Output tensor
    const float* input,         // Input tensor
    const float* weight,        // Linear weight
    const float* bias,          // Linear bias
    const float* residual,      // Residual connection
    const float* ln_weight,     // LayerNorm weight (gamma)
    const float* ln_bias,       // LayerNorm bias (beta)
    int batch_size,
    int seq_len,
    int hidden_dim,
    float epsilon = 1e-5f
);

// Fused Attention QKV projection
// Projects input to Q, K, V in single kernel
void fusedAttentionQKV(
    float* query,               // Output Q
    float* key,                 // Output K
    float* value,               // Output V
    const float* input,         // Input tensor
    const float* qkv_weight,    // Combined QKV weight
    const float* qkv_bias,      // Combined QKV bias
    int batch_size,
    int seq_len,
    int hidden_dim,
    int num_heads
);

// Fused RoPE + Attention Score
// Applies rotary position embedding and computes attention scores
void fusedRoPEAttentionScore(
    float* scores,              // Output attention scores
    const float* query,         // Query tensor
    const float* key,           // Key tensor
    const int* position_ids,    // Position IDs
    int batch_size,
    int num_heads,
    int seq_len,
    int head_dim,
    float scale,
    int rope_base = 10000
);

// Fused SoftMax + Dropout + Attention
// Combines softmax, dropout, and attention multiplication
void fusedSoftmaxDropoutAttention(
    float* output,              // Output tensor
    float* attention_weights,   // Attention weights (for visualization)
    const float* scores,        // Attention scores (pre-softmax)
    const float* values,        // Value tensor
    const float* attention_mask,// Optional attention mask
    int batch_size,
    int num_heads,
    int seq_len_q,
    int seq_len_kv,
    int head_dim,
    float dropout_prob = 0.0f,
    bool is_causal = true
);

// Fused FFN (Feed-Forward Network)
// gate_proj * silu(up_proj) in single kernel
void fusedGatedFFN(
    float* output,              // Output tensor
    const float* input,         // Input tensor
    const float* gate_weight,   // Gate projection weight
    const float* up_weight,     // Up projection weight
    const float* down_weight,   // Down projection weight
    int batch_size,
    int seq_len,
    int hidden_dim,
    int intermediate_dim
);

// Fused RMSNorm + Linear
// Root Mean Square LayerNorm + Linear projection
void fusedRMSNormLinear(
    float* output,              // Output tensor
    const float* input,         // Input tensor
    const float* weight,        // Linear weight
    const float* rms_weight,    // RMSNorm weight
    int batch_size,
    int seq_len,
    int hidden_dim,
    float epsilon = 1e-6f
);

/**
 * @brief Kernel Fusion Manager
 * 
 * Manages kernel fusion decisions and optimizations
 */
class KernelFusionManager {
public:
    struct Config {
        bool enable_fusion = true;
        bool enable_ln_linear_fusion = true;
        bool enable_qkv_fusion = true;
        bool enable_rope_fusion = true;
        bool enable_ffn_fusion = true;
        
        // Auto-tuning
        bool enable_auto_tuning = false;
        size_t auto_tune_iterations = 100;
    };
    
    explicit KernelFusionManager(const Config& config);
    
    // Check if fusion is beneficial for given dimensions
    bool shouldFuseLayerNormLinear(int batch, int seq_len, int hidden_dim) const;
    bool shouldFuseQKV(int batch, int seq_len, int hidden_dim) const;
    bool shouldFuseFFN(int batch, int seq_len, int hidden_dim) const;
    
    // Performance estimation
    double estimateSpeedup(const std::string& fusion_type,
                          int batch, int seq_len, int hidden_dim) const;
    
    // Statistics
    struct FusionStats {
        size_t ln_linear_fusions = 0;
        size_t qkv_fusions = 0;
        size_t ffn_fusions = 0;
        size_t total_fusions = 0;
        
        double total_time_saved_ms = 0.0;
        double avg_speedup = 0.0;
    };
    
    FusionStats getStats() const;
    
private:
    Config config_;
    mutable FusionStats stats_;
};

} // namespace kernels
} // namespace llm
} // namespace themis
