/**
 * @file infini_attention_hip.h
 * @brief Infini-attention HIP kernel API for AMD GPUs (P2-D02)
 *
 * Provides host-side interface for Infini-attention GPU kernels on AMD RDNA/CDNA architectures.
 *
 * @author Copilot Coding Agent (HIP Port)
 * @date 2026-07-22
 */

#pragma once

#include "../flash_attention.h"
#include "../flash_attention_config.h"
#include <memory>
#include <vector>

namespace themis {
namespace llm {
namespace attention {
namespace hip {

/**
 * @brief Infini-attention implementation for AMD GPUs via HIP
 *
 * Supports:
 * - MI300 (CDNA 3): Wave64 optimization
 * - MI100 (CDNA 1): Wave64 optimization
 * - RDNA 2/3: Wave32 optimization
 *
 * Thread-safe forward pass with configurable memory compression.
 */
class InfiniAttentionHIP {
public:
    /**
     * @brief Configuration for HIP Infini-attention
     */
    struct Config {
        size_t memory_dim = 128;           ///< Compressive memory dimension
        float update_rate = 0.1f;          ///< Contrastive learning rate α
        int low_rank = 4;                  ///< Low-rank projection dimension
        bool enable_fused_ops = true;      ///< Fuse kernels for efficiency
        size_t max_seq_len = 4096;         ///< Maximum sequence length
        int num_heads = 8;                 ///< Number of attention heads
        size_t head_dim = 64;              ///< Dimension per head
    };

    /**
     * @brief Initialize HIP Infini-attention
     *
     * Allocates GPU memory for compressive matrix and temporary buffers.
     * Deterministic allocation pattern: seed=42 for memory layout reproducibility.
     *
     * @param config Configuration parameters
     * @throws std::runtime_error if HIP device unavailable or memory allocation fails
     */
    explicit InfiniAttentionHIP(const Config& config);

    /**
     * @brief Destructor - releases all GPU resources
     */
    ~InfiniAttentionHIP();

    /**
     * @brief Ensure backend resources are initialized.
     * @return Status::SUCCESS when resources are ready.
     */
    Status initialize();

    /**
     * @brief Forward pass: compute attention with compressive memory
     *
     * Computes:
     * 1. Local attention: softmax(Q @ K^T) @ V via kernel dispatch
     * 2. Compressive attention: sigmoid(Q @ M^T) @ m_v
     * 3. Blend: α * local + (1 - α) * compressive
     *
     * @param Q Query tensor [batch*seq_len, num_heads, head_dim]
     * @param K Key tensor [batch*seq_len, num_heads, head_dim]
     * @param V Value tensor [batch*seq_len, num_heads, head_dim]
     * @param[out] O Output tensor [batch*seq_len, num_heads, head_dim]
     * @return Status code (SUCCESS on completion)
     *
     * @note Numerically stable: sigmoid clamped ±50, ε=1e-6
     * @note VRAM: ~256KB for 128×128 memory (0.006% of 4.4GB)
     */
    Status forward(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O
    );

    /**
     * @brief Backward pass - compute gradients (Phase 2.2)
     *
     * @param dO Gradient w.r.t. output
     * @param[out] dQ Gradient w.r.t. query
     * @param[out] dK Gradient w.r.t. key
     * @param[out] dV Gradient w.r.t. value
     * @return Status code
     *
     * @note Currently returns STATUS_NOT_IMPLEMENTED (deferred to Phase 2.2)
     */
    Status backward(
        const Tensor& dO,
        Tensor& dQ,
        Tensor& dK,
        Tensor& dV
    );

    /**
     * @brief Get memory statistics
     *
     * @return AttentionMemoryStats with total VRAM usage breakdown
     */
    AttentionMemoryStats getMemoryStats() const;

    /**
     * @brief Get backend name identifier
     *
     * @return "hip" identifying this as HIP backend
     */
    std::string getBackendName() const { return "hip"; }

    /**
     * @brief Check HIP availability on this system
     *
     * @return true if at least one HIP-capable GPU is available
     */
    static bool isAvailable();

    /**
     * @brief Initialize HIP device (first-time setup)
     *
     * Called automatically by constructor if HIP not yet initialized.
     * Safe to call multiple times.
     *
     * @return Status code
     */
    static Status initializeHIPDevice();

    /**
     * @brief Reset compressive memory to zeros
     *
     * Clears M matrix for fresh attention computation.
     *
     * @return Status code
     */
    Status resetMemory();

    /**
     * @brief Get checkpoint of compressive memory
     *
     * @return Copy of current M matrix for serialization/debugging
     * @throws std::runtime_error if GPU memory not allocated
     */
    std::vector<float> getCompressiveMemory() const;

    /**
     * @brief Restore compressive memory from checkpoint
     *
     * @param checkpoint Memory matrix to restore
     * @return Status code
     * @throws std::invalid_argument if checkpoint size mismatch
     */
    Status restoreCompressiveMemory(const std::vector<float>& checkpoint);

private:
    Config config_;
    bool initialized_ = false;

    // GPU memory pointers (managed via HIP)
    void* gpu_memory_ = nullptr;              ///< Compressive memory M [memory_dim × memory_dim]
    void* gpu_memory_update_ = nullptr;       ///< Update buffer for M'
    void* gpu_temp_buffer_ = nullptr;         ///< Temporary computation buffer

    /**
     * @brief Allocate GPU memory via hipMalloc
     *
     * @param bytes Number of bytes to allocate
     * @return Device pointer on success, nullptr on failure
     */
    void* allocateGPUMemory(size_t bytes) const;

    /**
     * @brief Release GPU memory via hipFree
     *
     * @param ptr Device pointer (nullptr-safe)
     * @return Status code
     */
    Status releaseGPUMemory();

    /**
     * @brief Compute local attention via Flash Attention
     *
     * @param Q Query tensor
     * @param K Key tensor
     * @param V Value tensor
     * @param[out] O Output tensor
     * @return Status code
     *
     * @note Placeholder (Phase 2.2): calls CPU fallback currently
     */
    Status computeLocalAttention(
        const Tensor& Q,
        const Tensor& K,
        const Tensor& V,
        Tensor& O
    );

    /**
     * @brief Compute compressive attention: sigmoid(Q @ M^T) @ m_v
     *
     * @param Q Query tensor
     * @param[out] O Output tensor [batch*seq_len, num_heads, memory_dim]
     * @return Status code
     *
     * @note Calls kernelCompressiveAttention kernel
     */
    Status computeCompressiveAttention(
        const Tensor& Q,
        Tensor& O
    );

    /**
     * @brief Update compressive memory M via low-rank approximation
     *
     * M' = M + α * sigmoid(K_compressed) ⊗ sigmoid(V_compressed)
     *
     * @param K Key tensor
     * @param V Value tensor
     * @return Status code
     *
     * @note Calls kernelUpdateMemory with atomic operations
     */
    Status updateCompressiveMemory(
        const Tensor& K,
        const Tensor& V
    );

    /**
     * @brief Blend local and compressive outputs
     *
     * O_final = α_blend * O_local + (1 - α_blend) * O_comp
     *
     * @param O_local Output from Flash Attention
     * @param O_comp Output from compressive attention
     * @param[out] O_final Blended output
     * @return Status code
     *
     * @note Placeholder (Phase 2.2): simple 50/50 blend currently
     */
    Status blendOutputs(
        const Tensor& O_local,
        const Tensor& O_comp,
        Tensor& O_final
    );
};

} // namespace hip
} // namespace attention
} // namespace llm
} // namespace themis
