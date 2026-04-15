/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            paged_kv_cache.h                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:35:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     95                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "llm/block_table.h"
#include "llm/paged_block_manager.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace themis {
namespace llm {

/**
 * PagedKVCache manages Key-Value cache storage using block-based memory allocation.
 * Integrates with llama.cpp KV cache format for efficient memory usage.
 */
class PagedKVCache {
public:
    struct Config {
        size_t block_size = 16;           // Tokens per block
        size_t num_blocks = 4096;         // Total blocks available
        size_t num_layers = 32;           // Number of transformer layers
        size_t head_dim = 128;            // Dimension per attention head
        size_t num_kv_heads = 8;          // Number of KV heads (GQA)
        bool enable_prefix_caching = true; // Enable prefix sharing
    };

    PagedKVCache(const Config& config, std::shared_ptr<PagedBlockManager> block_manager);
    
    ~PagedKVCache();

    // Store KV cache for a sequence
    void store(uint64_t sequence_id, size_t layer_id, const std::vector<float>& kv_data);
    
    // Retrieve KV cache for a sequence
    std::vector<float> retrieve(uint64_t sequence_id, size_t layer_id) const;
    
    // Share prefix between sequences (Copy-on-Write)
    void sharePrefix(uint64_t new_sequence_id, uint64_t parent_sequence_id, size_t prefix_length);
    
    // Get block table for sequence
    std::shared_ptr<BlockTable> getBlockTable(uint64_t sequence_id);
    
    // Remove sequence and free blocks
    void removeSequence(uint64_t sequence_id);
    
    // Get statistics
    struct Stats {
        size_t blocks_used;
        size_t blocks_free;
        size_t num_sequences;
        double fragmentation_rate;
        double prefix_sharing_ratio;
    };
    Stats getStats() const;

private:
    Config config_;
    std::shared_ptr<PagedBlockManager> block_manager_;
    
    // Map sequence_id -> BlockTable
    std::unordered_map<uint64_t, std::shared_ptr<BlockTable>> block_tables_;
    
    // KV cache storage: block_id -> layer_id -> kv_data
    std::unordered_map<int, std::unordered_map<size_t, std::vector<float>>> kv_storage_;
    
    mutable std::mutex mutex_;
    
    size_t calculateKVSize() const;
};

} // namespace llm
} // namespace themis
