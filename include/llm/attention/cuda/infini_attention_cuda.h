/**
 * @file infini_attention_cuda.h
 * @brief Infini-attention CUDA kernel implementation for SSM-Hybrid Phase 2 (P2-D02)
 *
 * Implements Infini-attention mechanism: unbounded context via compressive memory.
 * - Input: Query (Q), Key (K), Value (V), Compressive Memory (M)
 * - Output: Attention Output (O), Updated Memory (M')
 *
 * Compressive memory matrix M is stored in VRAM and updated via low-rank approximation:
 * M' = M + σ(α * k * v^T) where α controls update rate and k, v are compressed KV.
 *
 * Gate Compliance:
 * - P2-GATE-02: Numeric consistency with CPU fallback (src/llm/infini_attention_cpu.cpp)
 * - Integration: FlashAttentionFactory::create() selects backend automatically
 *
 * @author Copilot Coding Agent
 * @date 2026-07-22
 * @version 1.0.0
 */

#pragma once

#include "llm/attention/flash_attention.h"
#include "llm/attention/kv_cache_manager.h"
#include <memory>
#include <vector>
#include <cstddef>

namespace themis {
namespace llm {
namespace attention {

/**
 * @brief Extended backend types including Infini-attention
 */
enum class ExtendedBackend {
    FLASH_ATTENTION,        // Standard Flash Attention v3
    INFINI_COMPRESSIVE      // Infini-attention with compressive memory
};

/**
 * @brief Configuration for Infini-attention
 */
struct InfiniAttentionConfig {
    /// Compressive memory matrix dimension (typically 128-256)
    size_t memory_dim = 128;
    
    /// Update rate for memory matrix (0.0 to 1.0)
    /// α in M' = M + α * σ(k * v^T)
    float update_rate = 0.1f;
    
    /// Memory compression mode: low-rank or dense
    bool use_low_rank = true;
    
    /// Rank for low-rank decomposition (if use_low_rank = true)
    size_t low_rank_dim = 32;
    
    /// Attention head dimension (inherited from model, typically 64-128)
    size_t head_dim = 128;
    
    /// Number of attention heads
    size_t num_heads = 8;
    
    /// GPU compute capability: SM90 or SM86
    int cuda_sm = 90;
    
    /// Enable kernel fusion with FlashAttention computation
    bool enable_fusion = true;
};

/**
 * @brief Infini-attention CUDA implementation for SM90/SM86
 *
 * Provides:
 * 1. Compressive memory matrix M (size: memory_dim x memory_dim)
 * 2. Query-to-compressive interaction (dot product, sigmoid activation)
 * 3. Linear attention over compressive memory (O = Q @ M / (Q @ 1))
 * 4. Hybrid output: blend of local (Flash Attention) and global (compressive) context
 *
 * Numeric Behavior:
 * - Forward pass: O = softmax(Q @ K^T) @ V + sigmoid(Q @ M) @ M / denom
 * - Memory update: M' = M + α * σ(k * v^T) (causal in-place update)
 * - Gradient flow: Back-propagation through both branches
 */
class InfiniAttentionCUDA {
public:
    /**
     * @brief Construct Infini-attention CUDA backend
     * @param config Infini-attention configuration
     */
    explicit InfiniAttentionCUDA(const InfiniAttentionConfig& config);
    
    /**
     * @brief Destructor: release GPU memory
     */
    ~InfiniAttentionCUDA();
    
    /**
     * @brief Initialize compressive memory (allocate GPU tensors)
     * @return Status::SUCCESS on success, error code otherwise
     */
    Status initialize();
    
    /**
     * @brief Forward pass: compute attention + update compressive memory
     * @param Q Query tensor [batch_size, seq_len, num_heads, head_dim]
     * @param K Key tensor [batch_size, seq_len, num_heads, head_dim]
     * @param V Value tensor [batch_size, seq_len, num_heads, head_dim]
     * @param O Output tensor [batch_size, seq_len, num_heads, head_dim]
     * @param kv_cache Optional KV cache manager for prefix sharing
     * @return Status code
     *
     * Algorithm:
     * 1. Compute local attention: O_local = softmax(Q @ K^T) @ V
     * 2. Compute compressive attention: O_comp = sigmoid(Q @ M^T) @ (M @ M^T) / denom
     * 3. Blend: O = blend(O_local, O_comp) [default: 50/50 or gating]
     * 4. Update memory: M' = M + α * σ(k_compressed * v_compressed^T)
     */
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O,
        const KVCacheManager* kv_cache = nullptr);
    
    /**
     * @brief Backward pass: compute gradients
     * @param dO Gradient w.r.t. output
     * @param dQ Gradient w.r.t. query (output)
     * @param dK Gradient w.r.t. key (output)
     * @param dV Gradient w.r.t. value (output)
     * @return Status code
     */
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV);
    
    /**
     * @brief Get backend identification
     * @return String identifier
     */
    std::string getBackendName() const;
    
    /**
     * @brief Reset compressive memory to initial state
     * @return Status::SUCCESS on success
     */
    Status resetMemory();
    
    /**
     * @brief Get current memory statistics
     * @return AttentionMemoryStats with GPU memory breakdown
     */
    AttentionMemoryStats getMemoryStats() const;
    
    /**
     * @brief Get compressive memory for checkpointing
     * @return Tensor copy (host-side) of current compressive memory
     */
    Tensor getCompressiveMemory() const;
    
    /**
     * @brief Restore compressive memory from checkpoint
     * @param checkpoint Host-side tensor with saved memory
     * @return Status::SUCCESS on success
     */
    Status restoreCompressiveMemory(const Tensor& checkpoint);

private:
    InfiniAttentionConfig config_;
    
    /// GPU-resident compressive memory matrix M [memory_dim x memory_dim]
    float* gpu_memory_ = nullptr;
    
    /// GPU-resident update accumulator (temporary)
    float* gpu_memory_update_ = nullptr;
    
    /// GPU-resident temporary buffers for intermediate computations
    float* gpu_temp_buffer_ = nullptr;
    
    /// Size of allocated GPU memory (bytes)
    size_t gpu_memory_size_ = 0;
    
    /// Whether GPU buffers are initialized
    bool initialized_ = false;
    
    /**
     * @brief Compute local Flash Attention output
     * Uses TensorRT/cuDNN if available, fallback to kernel
     */
    Status computeLocalAttention(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O_local);
    
    /**
     * @brief Compute compressive memory interaction
     * Q @ M^T with causal masking
     */
    Status computeCompressiveAttention(
        const Tensor& Q,
        Tensor& O_comp);
    
    /**
     * @brief Blend local and compressive attention outputs
     * Default: 50/50 averaging
     */
    Status blendOutputs(
        const Tensor& O_local,
        const Tensor& O_comp,
        Tensor& O_final);
    
    /**
     * @brief Update compressive memory M' = M + α * σ(k * v^T)
     * Uses low-rank decomposition if enabled
     */
    Status updateCompressiveMemory(
        const Tensor& K,
        const Tensor& V);
    
    /**
     * @brief Allocate GPU memory
     * @param size_bytes Number of bytes to allocate
     * @return Pointer to GPU memory, nullptr on failure
     */
    float* allocateGPUMemory(size_t size_bytes);
    
    /**
     * @brief Release GPU memory
     */
    void releaseGPUMemory();
};

/**
 * @brief Factory function to create Infini-attention backend
 * @param config Configuration
 * @return Unique pointer to InfiniAttentionCUDA instance
 *
 * Integration point with FlashAttentionFactory.
 * Call as: auto infini = createInfiniAttentionCUDA(config);
 */
std::unique_ptr<InfiniAttentionCUDA> createInfiniAttentionCUDA(
    const InfiniAttentionConfig& config);

} // namespace attention
} // namespace llm
} // namespace themis
