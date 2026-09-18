/**
 * @file adaptive_vram_allocator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class AdaptiveVRAMAllocator {
public:
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

    struct HardwareInfo {
        size_t total_vram_bytes = 0;
        size_t available_vram_bytes = 0;
        int compute_capability_major = 8;
        int compute_capability_minor = 0;
        bool has_tensor_cores = true;
        size_t memory_bandwidth_gbps = 1000;
    };

    struct InferenceConfig {
        size_t batch_size = 1;
        size_t max_seq_length = 4096;
        size_t kv_cache_block_size = 16;  // Tokens per block
        bool enable_prefix_caching = true;
        bool enable_flash_attention = true;
        float kv_cache_growth_factor = 0.2f;  // 20% dynamic growth
    };

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
     * @brief Calculate Optimal Allocation.
     * @param[in] model Input parameter.
     * @param[in] hw Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    AllocationPlan calculateOptimalAllocation(
        const ModelConfig& model,
        const HardwareInfo& hw,
        const InferenceConfig& config
    );

    struct DualModelAllocationPlan : AllocationPlan {
        size_t draft_model_weights = 0;   ///< Draft model weight footprint (bytes).
        int    draft_precision_bytes = 0; ///< Effective bytes per parameter for draft (0 = INT4 = 0.5).
    };

    /**
     * @brief Calculate Dual Model Allocation.
     * @param[in] target_config Input parameter.
     * @param[in] draft_config Input parameter.
     * @param[in] hw Input parameter.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    DualModelAllocationPlan calculateDualModelAllocation(
        const ModelConfig&   target_config,
        const ModelConfig&   draft_config,
        const HardwareInfo&  hw,
        const InferenceConfig& config
    );

    /**
     * @brief Allocate With Fragmentation.
     * @param[in] bytes Input parameter.
     * @param[in,out] ptr Input/output parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool allocateWithFragmentation(size_t bytes, void** ptr) noexcept;

    /**
     * @brief Handle Out Of Memory.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool handleOutOfMemory() noexcept;

    /**
     * @brief Calculate KVCache Size Per Token.
     * @param[in] model Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static size_t calculateKVCacheSizePerToken(const ModelConfig& model) noexcept;

    /**
     * @brief Calculate Model Size.
     * @param[in] num_parameters Input parameter.
     * @param[in] precision_bytes Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static size_t calculateModelSize(size_t num_parameters, float precision_bytes) noexcept;

    /**
     * @brief Estimate Activation Memory.
     * @param[in] model Input parameter.
     * @param[in] batch_size Input parameter.
     * @param[in] seq_length Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
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
