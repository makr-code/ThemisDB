/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            paged_kv_cache.cpp                                 ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:49:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     197                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "llm/paged_kv_cache.h"
#include <algorithm>

namespace themis {
namespace llm {

PagedKVCache::PagedKVCache(const Config& config, std::shared_ptr<PagedBlockManager> block_manager)
    : config_(config)
    , block_manager_(block_manager) {
}

PagedKVCache::~PagedKVCache() {
    // Clean up all sequences
    std::lock_guard<std::mutex> lock(mutex_);
    block_tables_.clear();
    kv_storage_.clear();
}

void PagedKVCache::store(uint64_t sequence_id, size_t layer_id, const std::vector<float>& kv_data) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Get or create block table for this sequence
    auto it = block_tables_.find(sequence_id);
    if (it == block_tables_.end()) {
        BlockTable::Config bt_config;
        bt_config.block_size = config_.block_size;
        bt_config.enable_cow = config_.enable_prefix_caching;
        
        block_tables_[sequence_id] = std::make_shared<BlockTable>(
            block_manager_, sequence_id, bt_config);
        it = block_tables_.find(sequence_id);
    }
    
    auto block_table = it->second;
    
    // Calculate how many blocks we need
    size_t kv_size_per_token = calculateKVSize();
    size_t num_tokens = kv_data.size() / kv_size_per_token;
    size_t num_blocks_needed = (num_tokens + config_.block_size - 1) / config_.block_size;
    
    // Allocate blocks if needed
    auto current_blocks = block_table->getBlockMapping();
    if (current_blocks.size() < num_blocks_needed) {
        size_t blocks_to_allocate = num_blocks_needed - current_blocks.size();
        block_table->allocateBlocks(blocks_to_allocate);
        current_blocks = block_table->getBlockMapping();
    }
    
    // Store KV data in blocks
    for (size_t i = 0; i < current_blocks.size(); ++i) {
        int block_id = current_blocks[i];
        
        // Calculate offset for this block
        size_t start_token = i * config_.block_size;
        size_t end_token = std::min(start_token + config_.block_size, num_tokens);
        size_t start_idx = start_token * kv_size_per_token;
        size_t end_idx = end_token * kv_size_per_token;
        
        // Copy KV data for this block
        if (end_idx <= kv_data.size()) {
            kv_storage_[block_id][layer_id] = std::vector<float>(
                kv_data.begin() + start_idx,
                kv_data.begin() + end_idx
            );
        }
    }
}

std::vector<float> PagedKVCache::retrieve(uint64_t sequence_id, size_t layer_id) const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = block_tables_.find(sequence_id);
    if (it == block_tables_.end()) {
        return {};
    }
    
    auto block_table = it->second;
    auto block_ids = block_table->getBlockMapping();
    
    std::vector<float> result;
    
    // Retrieve KV data from all blocks
    for (int block_id : block_ids) {
        auto block_it = kv_storage_.find(block_id);
        if (block_it != kv_storage_.end()) {
            auto layer_it = block_it->second.find(layer_id);
            if (layer_it != block_it->second.end()) {
                const auto& block_kv = layer_it->second;
                result.insert(result.end(), block_kv.begin(), block_kv.end());
            }
        }
    }
    
    return result;
}

void PagedKVCache::sharePrefix(uint64_t new_sequence_id, uint64_t parent_sequence_id, size_t prefix_length) {
    if (!config_.enable_prefix_caching) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto parent_it = block_tables_.find(parent_sequence_id);
    if (parent_it == block_tables_.end()) {
        return;
    }
    
    // Create new block table
    BlockTable::Config bt_config;
    bt_config.block_size = config_.block_size;
    bt_config.enable_cow = true;
    
    auto new_block_table = std::make_shared<BlockTable>(
        block_manager_, new_sequence_id, bt_config);
    
    // Share prefix blocks
    new_block_table->sharePrefix(parent_sequence_id, prefix_length);
    
    block_tables_[new_sequence_id] = new_block_table;
}

std::shared_ptr<BlockTable> PagedKVCache::getBlockTable(uint64_t sequence_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = block_tables_.find(sequence_id);
    if (it != block_tables_.end()) {
        return it->second;
    }
    
    return nullptr;
}

void PagedKVCache::removeSequence(uint64_t sequence_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    auto it = block_tables_.find(sequence_id);
    if (it != block_tables_.end()) {
        // Blocks will be released by BlockTable destructor
        block_tables_.erase(it);
    }
}

PagedKVCache::Stats PagedKVCache::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.num_sequences = block_tables_.size();
    
    size_t total_blocks = 0;
    size_t shared_blocks = 0;
    
    for (const auto& [seq_id, block_table] : block_tables_) {
        auto bt_stats = block_table->getStats();
        total_blocks += bt_stats.num_blocks;
        shared_blocks += bt_stats.num_shared_blocks;
    }
    
    stats.blocks_used = total_blocks;
    stats.blocks_free = config_.num_blocks > total_blocks ? 
                       config_.num_blocks - total_blocks : 0;
    stats.fragmentation_rate = 0.0;  // Would calculate based on allocation pattern
    stats.prefix_sharing_ratio = total_blocks > 0 ? 
                                static_cast<double>(shared_blocks) / total_blocks : 0.0;
    
    return stats;
}

size_t PagedKVCache::calculateKVSize() const {
    // KV cache size per token: 2 (K and V) * num_kv_heads * head_dim
    return 2 * config_.num_kv_heads * config_.head_dim;
}

} // namespace llm
} // namespace themis
