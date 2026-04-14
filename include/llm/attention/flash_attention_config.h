/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            flash_attention_config.h                           ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:52:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     120                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • a629043ab2  2026-02-22  Audit: document gaps found - benchmarks and stale annotat... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
