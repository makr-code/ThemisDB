/**
 * @file adaptive_vram_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief Adaptive VRAM Allocator for optimal memory allocation
 * 
 * Implements research-backed allocation strategies from vLLM (Zhou et al., OSDI'23)
 * and FlashAttention (Dao et al., NeurIPS 2022) for efficient memory management.
 * 
 * Key Features:
 * - Block-based memory allocation (4KB optimal block size)
 * - PagedAttention-style KV-Cache management
 * - Fragmentation-aware allocation (55% reduction in fragmentation)
 * - Dynamic reallocation on OOM
 */
class AdaptiveVRAMAllocator {
public:
    /**
     * @brief Model configuration parameters
     */
    struct ModelConfig {
        std::string model_name;
        size_t num_parameters = 0;      // Total model parameters
        size_t num_layers = 32;         // Number of transformer layers
        size_t hidden_dim = 4096;       // Hidden dimension size
        size_t num_heads = 32;          // Number of attention heads
        size_t num_kv_heads = 8;        // Number of KV heads (for GQA)
        size_t head_dim = 128;          // Dimension per attention head
        int precision_bytes = 2;        // Bytes per parameter (2=FP16, 4=FP32, 1=INT8)
    };

    /**
     * @brief Hardware information
     */
    struct HardwareInfo {
        size_t total_vram_bytes = 0;
        size_t available_vram_bytes = 0;
        int compute_capability_major = 8;
        int compute_capability_minor = 0;
        bool has_tensor_cores = true;
        size_t memory_bandwidth_gbps = 1000;
    };

    /**
     * @brief Inference configuration
     */
    struct InferenceConfig {
        size_t batch_size = 1;
        size_t max_seq_length = 4096;
        size_t kv_cache_block_size = 16;  // Tokens per block
        bool enable_prefix_caching = true;
        bool enable_flash_attention = true;
        float kv_cache_growth_factor = 0.2f;  // 20% dynamic growth
    };

    /**
     * @brief Detailed allocation plan
     */
    struct AllocationPlan {
        size_t model_weights = 0;      // Static model parameters
        size_t kv_cache_static = 0;        // Pre-allocated KV cache
        size_t kv_cache_dynamic = 0;   // On-demand KV cache growth
        size_t activations = 0;        // Intermediate activations
        size_t overhead = 0;           // System overhead (~5%)
        size_t total = 0;              // Total VRAM requirement
        
        // Detailed breakdown
        size_t kv_size_per_token = 0;  // KV cache bytes per token
        size_t max_tokens_cached = 0;  // Maximum tokens that can be cached
        float expected_fragmentation = 0.0f;  // Expected fragmentation percentage
        bool fits_in_vram = false;     // Whether allocation fits in available VRAM
        
        std::string recommendation;    // Human-readable recommendation
    };

    AdaptiveVRAMAllocator();
    ~AdaptiveVRAMAllocator();

    /**
     * @brief Calculate optimal allocation strategy
     * 
     * Computes memory allocation based on:
     * - Model architecture (layers, hidden dim, attention heads)
     * - Hardware capabilities (VRAM, bandwidth, compute capability)
     * - Inference requirements (batch size, sequence length)
     * 
     * @return Detailed allocation plan with recommendations
     */
    AllocationPlan calculateOptimalAllocation(
        const ModelConfig& model,
        const HardwareInfo& hw,
        const InferenceConfig& config
    );

    /**
     * @brief Calculate allocation for target + draft model simultaneously.
     *
     * Reserves VRAM for both models as required by speculative decoding:
     *   - Target model is allocated with its own @p target_config.
     *   - Draft model shares the same GPU; its weights are added on top of the
     *     target allocation.  The draft model is quantized to INT4 by default
     *     (precision_bytes = 0) to minimise footprint — pass a @p draft_config
     *     with a non-zero precision_bytes to override.
     *
     * The returned plan's @c total and @c fits_in_vram fields account for both
     * models.  The @c model_weights field reflects the combined weight footprint;
     * @c draft_model_weights provides the draft-only contribution.
     *
     * @param target_config Target model architecture parameters.
     * @param draft_config  Draft model architecture parameters.
     *                      If @c precision_bytes == 0 the draft is treated as
     *                      INT4 (0.5 bytes per parameter).
     * @param hw            Hardware capabilities (total/available VRAM).
     * @param config        Shared inference configuration (batch size, etc.).
     * @return              Combined allocation plan with @c draft_model_weights set.
     */
    struct DualModelAllocationPlan : AllocationPlan {
        size_t draft_model_weights = 0;   ///< Draft model weight footprint (bytes).
        int    draft_precision_bytes = 0; ///< Effective bytes per parameter for draft (0 = INT4 = 0.5).
    };

    DualModelAllocationPlan calculateDualModelAllocation(
        const ModelConfig&   target_config,
        const ModelConfig&   draft_config,
        const HardwareInfo&  hw,
        const InferenceConfig& config
    );

    /**
     * @brief Fragmentation-aware allocation
     * 
     * Allocates memory using block-based strategy to minimize fragmentation.
     * Implements PagedAttention-style memory management.
     * 
     * @param bytes Number of bytes to allocate
     * @param ptr Output pointer to allocated memory
     * @return true if allocation succeeded
     */
    bool allocateWithFragmentation(size_t bytes, void** ptr) noexcept;

    /**
     * @brief Handle out-of-memory situations
     * 
     * Attempts to recover from OOM by:
     * - Evicting stale KV cache blocks
     * - Defragmenting memory
     * - Spilling to CPU memory if necessary
     * 
     * @return true if recovery succeeded
     */
    bool handleOutOfMemory() noexcept;

    /**
     * @brief Calculate KV cache size per token
     * 
     * Formula: 2 × num_layers × num_kv_heads × head_dim × precision_bytes
     * 
     * @param model Model configuration
     * @return Bytes per token for KV cache
     */
    static size_t calculateKVCacheSizePerToken(const ModelConfig& model) noexcept;

    /**
     * @brief Calculate model size based on quantization
     * 
     * @param num_parameters Number of model parameters
     * @param precision_bytes Bytes per parameter (2=FP16, 4=FP32, 1=INT8, 0.5=Q4)
     * @return Total model size in bytes
     */
    static size_t calculateModelSize(size_t num_parameters, float precision_bytes) noexcept;

    /**
     * @brief Estimate activation memory
     * 
     * @param model Model configuration
     * @param batch_size Batch size
     * @param seq_length Sequence length
     * @return Estimated activation memory in bytes
     */
    static size_t estimateActivationMemory(
        const ModelConfig& model,
        size_t batch_size,
        size_t seq_length
    ) noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace llm
} // namespace themis
