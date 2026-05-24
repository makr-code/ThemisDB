/*
 * ThemisDB | File: block_table.cpp | Version: 0.0.47 | Last Modified: 2026-05-18 20:49:49
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 122
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=43 | delta=40 | status=divergent
 * External Severity (v3): C=7, H=30, M=6
 * PR: #105 Add plugin-based LLM integration v1.3.0 with llama.cpp, GPU acceler... (2026-03-11T17:03:10Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "llm/block_table.h"
#include <algorithm>

namespace themis {
namespace llm {

BlockTable::BlockTable(std::shared_ptr<PagedBlockManager> block_manager,
                       uint64_t sequence_id,
                       const Config& config)
    : block_manager_(block_manager)
    , sequence_id_(sequence_id)
    , config_(config) {
}

BlockTable::~BlockTable() {
    releaseBlocks();
}

std::vector<int> BlockTable::allocateBlocks(size_t num_blocks) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    std::vector<int> new_blocks;
    new_blocks.reserve(num_blocks);
    
    for (size_t i = 0; i < num_blocks; ++i) {
        // Allocate block from global pool
        int block_id = block_manager_->allocate();
        if (block_id < 0) {
            // Out of blocks, release what we allocated and fail
            for (int bid : new_blocks) {
                block_manager_->deallocate(bid);
            }
            return {};
        }
        
        new_blocks.push_back(block_id);
        block_ids_.push_back(block_id);
        is_shared_.push_back(false);
        ref_counts_[block_id] = 1;
    }
    
    return new_blocks;
}

void BlockTable::releaseBlocks() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    for (size_t i = 0; i < block_ids_.size(); ++i) {
        int block_id = block_ids_[i];
        
        // Decrement reference count
        auto it = ref_counts_.find(block_id);
        if (it != ref_counts_.end()) {
            it->second--;
            
            // Only deallocate if no more references
            if (it->second == 0) {
                block_manager_->deallocate(block_id);
                ref_counts_.erase(it);
            }
        }
    }
    
    block_ids_.clear();
    is_shared_.clear();
}

void BlockTable::sharePrefix(uint64_t /*parent_sequence_id*/, size_t prefix_length) {
    if (!config_.enable_cow) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex_);
    
    // Calculate how many blocks to share
    size_t num_prefix_blocks = (prefix_length + config_.block_size - 1) / config_.block_size;
    
    // For simplicity, assume parent blocks are available
    // In production, would need coordination with parent BlockTable
    for (size_t i = 0; i < num_prefix_blocks && i < block_ids_.size(); ++i) {
        is_shared_[i] = true;
        
        // Increment reference count
        int block_id = block_ids_[i];
        ref_counts_[block_id]++;
    }
}

std::vector<int> BlockTable::getBlockMapping() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return block_ids_;
}

size_t BlockTable::getNumTokens() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return block_ids_.size() * config_.block_size;
}

BlockTable::Stats BlockTable::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);
    
    Stats stats;
    stats.num_blocks = block_ids_.size();
    stats.num_shared_blocks = std::count(is_shared_.begin(), is_shared_.end(), true);
    stats.num_cow_blocks = 0;  // Would track CoW copies separately
    stats.sharing_ratio = block_ids_.empty() ? 0.0 : 
                         static_cast<double>(stats.num_shared_blocks) / block_ids_.size();
    
    return stats;
}

} // namespace llm
} // namespace themis

