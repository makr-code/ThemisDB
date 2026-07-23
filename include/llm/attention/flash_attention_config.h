/**
 * @file flash_attention_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: flash_attention_config.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 107
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #1031 Implement comprehensive res... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <cmath>

namespace themis {
namespace llm {
namespace attention {

/**
 * @brief Configuration for Flash Attention v3
 * 
 * Supports multiple backends (CUDA SM86/SM90, Vulkan, HIP, CPU)
 * and various optimization strategies.
 */
struct FlashAttentionConfig {
    // Tensor dimensions
    int batch_size = 1;
    int seq_len = 2048;
    int num_heads = 32;
    int head_dim = 128;
    
    // Grouped Query Attention (GQA) support
    // num_kv_heads should divide num_heads evenly
    // - Standard MHA: num_kv_heads = num_heads (e.g., 32 = 32)
    // - GQA: num_kv_heads < num_heads (e.g., 8 < 32, ratio 4:1)
    // - MQA: num_kv_heads = 1 (single KV head for all query heads)
    // Llama 2/3 uses GQA with ratio 4:1 (32 query heads, 8 KV heads)
    int num_kv_heads = 8;
    
    // Attention parameters
    float dropout_p = 0.0f;
    bool use_causal_mask = true;
    float scale = 0.0f;  // 1/sqrt(head_dim), auto-computed if 0
    
    // Flash Attention versions
    bool enable_flash_v2 = true;
    bool enable_flash_v3 = true;  // SM90 only (H100, RTX 6000 Ada)
    
    // Optimization flags
    bool enable_kernel_fusion = true;
    bool enable_async_copy = true;
    bool enable_tensor_cores = true;
    bool enable_warp_specialization = true;
    
    // Transformer depth — used by KVCacheManager to compute block byte size
    int num_layers = 32;

    // KV-Cache options
    bool use_paged_kv_cache = true;
    size_t kv_block_size = 16;  // Tokens per block
    size_t num_kv_blocks = 4096;
    bool enable_prefix_caching = true;
    
    // Performance tuning
    int tile_size = 64;
    int block_size = 256;
    int num_warps = 8;
    
    // Quantization
    enum class QuantType {
        FP32,
        FP16,
        BF16,
        INT8,
        Q4
    };
    QuantType quant_type = QuantType::FP16;
    
    // Compute defaults
    FlashAttentionConfig() {
        // Auto-compute scale if not set
        if (scale == 0.0f) {
            scale = 1.0f / std::sqrt(static_cast<float>(head_dim));
        }
    }
};

/**
 * @brief Memory statistics for attention operations
 */
struct AttentionMemoryStats {
    virtual ~AttentionMemoryStats() = default;
    // Total VRAM bytes used by attention (including buffers, KV cache, temps)
    size_t vram_used = 0;
    size_t total_memory_bytes = 0;
    size_t kv_cache_bytes = 0;
    size_t activation_bytes = 0;
    size_t workspace_bytes = 0;
    
    size_t blocks_used = 0;
    size_t blocks_free = 0;
    double fragmentation_rate = 0.0;
    double prefix_sharing_ratio = 0.0;
};

} // namespace attention
} // namespace llm
} // namespace themis
