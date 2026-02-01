#include "llm/paged_kv_cache_manager.h"
#include <algorithm>
#include <stdexcept>

namespace themis {
namespace llm {

PagedKVCacheManager::PagedKVCacheManager(const Config& config)
    : config_(config) {
    initializeBlocks();
}

PagedKVCacheManager::~PagedKVCacheManager() = default;

void PagedKVCacheManager::initializeBlocks() {
    blocks_.resize(config_.num_blocks);
    free_block_ids_.reserve(config_.num_blocks);
    
    for (size_t i = 0; i < config_.num_blocks; ++i) {
        blocks_[i].block_id = static_cast<int>(i);
        blocks_[i].ref_count = 0;
        blocks_[i].is_pinned = false;
        blocks_[i].parent_sequence_id = 0;
        blocks_[i].device_ptr = nullptr;  // Would allocate GPU memory here
        
        free_block_ids_.push_back(static_cast<int>(i));
    }
}

std::vector<int> PagedKVCacheManager::allocateBlocks(size_t num_blocks) {
    std::vector<int> allocated;
    allocated.reserve(num_blocks);
    
    for (size_t i = 0; i < num_blocks && !free_block_ids_.empty(); ++i) {
        int block_id = getFreeBlock();
        if (block_id >= 0) {
            allocated.push_back(block_id);
            blocks_[block_id].ref_count++;
            total_blocks_allocated_++;
        }
    }
    
    return allocated;
}

void PagedKVCacheManager::freeBlocks(const std::vector<int>& block_ids) {
    for (int block_id : block_ids) {
        if (block_id >= 0 && block_id < static_cast<int>(blocks_.size())) {
            releaseBlock(block_id);
        }
    }
}

bool PagedKVCacheManager::enablePrefixCaching(
    uint64_t seq_id,
    uint64_t parent_seq_id,
    size_t prefix_length
) {
    if (!config_.enable_prefix_caching) {
        return false;
    }
    
    // Find parent sequence
    auto parent_it = sequence_tables_.find(parent_seq_id);
    if (parent_it == sequence_tables_.end()) {
        return false;
    }
    
    // Calculate number of blocks to share
    size_t blocks_to_share = (prefix_length + config_.block_size - 1) / config_.block_size;
    blocks_to_share = std::min(blocks_to_share, parent_it->second.block_ids.size());
    
    // Create new sequence with shared blocks
    BlockTable child_table;
    child_table.sequence_id = seq_id;
    child_table.num_tokens = prefix_length;
    child_table.is_prefix_cached = true;
    
    // Share prefix blocks (increment ref count)
    for (size_t i = 0; i < blocks_to_share; ++i) {
        int block_id = parent_it->second.block_ids[i];
        child_table.block_ids.push_back(block_id);
        blocks_[block_id].ref_count++;
        total_blocks_shared_++;
    }
    
    sequence_tables_[seq_id] = child_table;
    parent_map_[seq_id] = parent_seq_id;
    
    return true;
}

PagedKVCacheManager::BlockTable 
PagedKVCacheManager::getBlockTable(uint64_t seq_id) const {
    auto it = sequence_tables_.find(seq_id);
    if (it != sequence_tables_.end()) {
        return it->second;
    }
    
    BlockTable empty;
    empty.sequence_id = seq_id;
    empty.num_tokens = 0;
    empty.is_prefix_cached = false;
    return empty;
}

PagedKVCacheManager::BlockTable 
PagedKVCacheManager::addSequence(uint64_t seq_id, size_t num_tokens) {
    // Calculate number of blocks needed
    size_t num_blocks_needed = (num_tokens + config_.block_size - 1) / config_.block_size;
    
    // Allocate blocks
    std::vector<int> block_ids = allocateBlocks(num_blocks_needed);
    
    BlockTable table;
    table.sequence_id = seq_id;
    table.block_ids = block_ids;
    table.num_tokens = num_tokens;
    table.is_prefix_cached = false;
    
    sequence_tables_[seq_id] = table;
    
    return table;
}

void PagedKVCacheManager::removeSequence(uint64_t seq_id) {
    auto it = sequence_tables_.find(seq_id);
    if (it != sequence_tables_.end()) {
        freeBlocks(it->second.block_ids);
        sequence_tables_.erase(it);
    }
    
    // Remove from parent map if exists
    parent_map_.erase(seq_id);
}

PagedKVCacheManager::MemoryStats 
PagedKVCacheManager::getMemoryStats() const {
    MemoryStats stats;
    stats.total_blocks = config_.num_blocks;
    stats.free_blocks = free_block_ids_.size();
    stats.used_blocks = stats.total_blocks - stats.free_blocks;
    stats.num_sequences = sequence_tables_.size();
    
    // Calculate fragmentation rate
    size_t allocated_blocks = 0;
    size_t total_tokens = 0;
    for (const auto& [seq_id, table] : sequence_tables_) {
        allocated_blocks += table.block_ids.size();
        total_tokens += table.num_tokens;
    }
    
    size_t theoretical_blocks = (total_tokens + config_.block_size - 1) / config_.block_size;
    if (theoretical_blocks > 0) {
        stats.fragmentation_rate = static_cast<double>(allocated_blocks - theoretical_blocks) / 
                                   theoretical_blocks;
    } else {
        stats.fragmentation_rate = 0.0;
    }
    
    // Calculate prefix sharing ratio
    stats.prefix_sharing_ratio = calculatePrefixSavings() / 100.0;
    
    // Calculate memory usage
    stats.bytes_per_block = calculateBlockMemorySize();
    stats.total_memory_bytes = stats.total_blocks * stats.bytes_per_block;
    stats.used_memory_bytes = stats.used_blocks * stats.bytes_per_block;
    
    return stats;
}

bool PagedKVCacheManager::isBlockAvailable(int block_id) const {
    return block_id >= 0 && 
           block_id < static_cast<int>(blocks_.size()) && 
           blocks_[block_id].ref_count > 0;
}

PagedKVCacheManager::BlockInfo 
PagedKVCacheManager::getBlockInfo(int block_id) const {
    if (block_id >= 0 && block_id < static_cast<int>(blocks_.size())) {
        const auto& block = blocks_[block_id];
        BlockInfo info;
        info.block_id = block.block_id;
        info.device_ptr = block.device_ptr;
        info.ref_count = block.ref_count.load();
        info.is_pinned = block.is_pinned;
        info.parent_sequence_id = block.parent_sequence_id;
        return info;
    }
    
    BlockInfo invalid;
    invalid.block_id = -1;
    invalid.device_ptr = nullptr;
    invalid.ref_count = 0;
    invalid.is_pinned = false;
    invalid.parent_sequence_id = 0;
    return invalid;
}

size_t PagedKVCacheManager::defragment() {
    // Stub implementation - would compact memory
    // In production, would reorganize blocks to reduce fragmentation
    return 0;
}

double PagedKVCacheManager::calculatePrefixSavings() const {
    if (total_blocks_allocated_ == 0) {
        return 0.0;
    }
    
    double savings = (static_cast<double>(total_blocks_shared_) / 
                     static_cast<double>(total_blocks_allocated_)) * 100.0;
    return savings;
}

int PagedKVCacheManager::getFreeBlock() {
    if (free_block_ids_.empty()) {
        return -1;
    }
    
    int block_id = free_block_ids_.back();
    free_block_ids_.pop_back();
    return block_id;
}

void PagedKVCacheManager::releaseBlock(int block_id) {
    if (block_id < 0 || block_id >= static_cast<int>(blocks_.size())) {
        return;
    }
    
    int prev_count = blocks_[block_id].ref_count.fetch_sub(1);
    
    // Only free when ref count reaches zero
    if (prev_count == 1) {
        blocks_[block_id].parent_sequence_id = 0;
        blocks_[block_id].is_pinned = false;
        free_block_ids_.push_back(block_id);
    }
}

size_t PagedKVCacheManager::calculateBlockMemorySize() const {
    // Memory per block = block_size × num_layers × 2 (K+V) × 
    //                    num_kv_heads × head_dim × bytes_per_element
    return config_.block_size * config_.num_layers * 2 * 
           config_.num_kv_heads * config_.head_dim * config_.bytes_per_element;
}

} // namespace llm
} // namespace themis
