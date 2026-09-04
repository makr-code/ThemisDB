/**
 * @file paged_block_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=3, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "llm/paged_block_manager.h"
#include <algorithm>
#include <limits>
#include <stdexcept>

namespace themis {
namespace llm {

PagedBlockManager::PagedBlockManager(const Config& config)
    : config_(config) {
    initializeFreeList();
}

void PagedBlockManager::initializeFreeList() {
    std::lock_guard<std::mutex> lock(free_list_mutex_);
    
    // Resolve number of blocks (support legacy total_blocks alias)
    int num_blocks = config_.max_blocks;
    if (config_.total_blocks > 0) {
        num_blocks = config_.total_blocks;
        // Keep max_blocks consistent for stats
        const_cast<Config&>(config_).max_blocks = num_blocks;
    }

    // Initialize all blocks as free
    for (int i = 0; i < num_blocks; i++) {
        Block block;
        block.block_id = i;
        const std::size_t token_offset = static_cast<std::size_t>(i) * config_.block_size_tokens;
        if (token_offset > static_cast<std::size_t>(std::numeric_limits<int>::max())) {
            throw std::overflow_error("PagedBlockManager: physical_address exceeds int range");
        }
        block.physical_address = static_cast<int>(token_offset);
        block.is_free = true;
        block.memory_bytes = config_.block_size_tokens * config_.token_size_bytes;
        block.ref_count = 0;
        
        // Store in cache (lock-free access)
        blocks_.insert(i, block);
        
        // Add to free list
        free_list_.push(i);
    }
}

std::vector<int> PagedBlockManager::allocateBlocks(int num_blocks) {
    std::vector<int> allocated_ids;
    allocated_ids.reserve(num_blocks);
    
    std::lock_guard<std::mutex> lock(free_list_mutex_);
    
    // Check if enough free blocks
    if (static_cast<int>(free_list_.size()) < num_blocks) {
        return {};  // Allocation failed
    }
    
    // Allocate blocks
    for (int i = 0; i < num_blocks; i++) {
        int block_id = free_list_.front();
        free_list_.pop();
        allocated_ids.push_back(block_id);
        
        // Update block state (lock-free via ConcurrentCache)
        auto block = blocks_.get(block_id);
        if (block) {
            block->is_free = false;
            block->ref_count = 1;
            block->tokens.clear();
            blocks_.insert(block_id, *block);
        }
    }
    
    return allocated_ids;
}

int PagedBlockManager::allocate() {
    auto blocks = allocateBlocks(1);
    return blocks.empty() ? -1 : blocks.front();
}

void PagedBlockManager::freeBlocks(const std::vector<int>& block_ids) {
    std::lock_guard<std::mutex> lock(free_list_mutex_);
    
    for (int block_id : block_ids) {
        // Update block state
        auto block = blocks_.get(block_id);
        if (block && !block->is_free) {
            block->is_free = true;
            block->ref_count = 0;
            block->tokens.clear();
            blocks_.insert(block_id, *block);
            
            // Return to free list
            free_list_.push(block_id);
        }
    }
}

void PagedBlockManager::deallocate(int block_id) {
    freeBlocks({block_id});
}

void PagedBlockManager::withBlock(int block_id, std::function<void(const Block&)> callback) const {
    auto block_opt = blocks_.get(block_id);
    if (block_opt) {
        callback(*block_opt);
    }
}

std::optional<std::reference_wrapper<const PagedBlockManager::Block>> 
PagedBlockManager::getBlockRef(int block_id) const {
    auto block_opt = blocks_.get(block_id);
    if (block_opt) {
        return std::reference_wrapper<const Block>(*block_opt);
    }
    return std::nullopt;
}

PagedBlockManager::Stats PagedBlockManager::getStats() const {
    Stats stats{};
    stats.num_blocks = config_.max_blocks;
    stats.total_memory_bytes = config_.max_blocks * 
                               config_.block_size_tokens * 
                               config_.token_size_bytes;
    
    // Count free blocks
    {
        std::lock_guard<std::mutex> lock(free_list_mutex_);
        stats.num_free_blocks = static_cast<int>(free_list_.size());
    }
    
    stats.num_allocated_blocks = stats.num_blocks - stats.num_free_blocks;
    stats.used_memory_bytes = stats.num_allocated_blocks * 
                              config_.block_size_tokens * 
                              config_.token_size_bytes;
    
    // Calculate fragmentation (simple metric)
    if (stats.num_blocks > 0) {
        stats.fragmentation_ratio = 
            static_cast<double>(stats.num_allocated_blocks) / stats.num_blocks;
    } else {
        stats.fragmentation_ratio = 0.0;
    }
    
    return stats;
}

int PagedBlockManager::getNumFreeBlocks() const {
    std::lock_guard<std::mutex> lock(free_list_mutex_);
    return static_cast<bool>(static_cast<int < static_cast<int>((free_list_.size())));
}

void PagedBlockManager::reset() {
    blocks_.clear();
    
    std::lock_guard<std::mutex> lock(free_list_mutex_);
    while (!free_list_.empty()) {
        free_list_.pop();
    }
    
    initializeFreeList();
}

} // namespace llm
} // namespace themis

