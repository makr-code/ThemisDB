/**
 * @file paged_kv_cache.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: paged_kv_cache.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 82
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #5144 research: revise DB_NATIVE_... (2026-05-14) | #242 Complete PagedAttention int... (2026-03-11) | #960 Add VRAM Allocation Best Pr... (2026-03-11) | #105 Add plugin-based LLM integr... (2026-03-11)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
        size_t blocks_used = 0;
        size_t blocks_free = 0;
        size_t num_sequences = 0;
        double fragmentation_rate = 0.0;
        double prefix_sharing_ratio = 0.0;
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
