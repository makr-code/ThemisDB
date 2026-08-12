/**
 * @file block_table.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "llm/paged_block_manager.h"
#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace themis {
namespace llm {

/**
 * BlockTable manages the mapping between logical sequence positions and physical blocks
 * for a single inference sequence. Supports Copy-on-Write (CoW) for prefix sharing.
 */
class BlockTable {
public:
    struct Config {
        size_t block_size = 16;  // Tokens per block
        bool enable_cow = true;   // Enable Copy-on-Write for prefix sharing
    };

    BlockTable(std::shared_ptr<PagedBlockManager> block_manager, 
               uint64_t sequence_id, 
               const Config& config);
    
    ~BlockTable();

    // Allocate blocks for this sequence
    std::vector<int> allocateBlocks(size_t num_blocks);
    
    // Release all blocks for this sequence
    void releaseBlocks();
    
    // Share prefix blocks from parent sequence (Copy-on-Write)
    void sharePrefix(uint64_t parent_sequence_id, size_t prefix_length);
    
    // Get block mapping for attention computation
    std::vector<int> getBlockMapping() const;
    
    // Get number of tokens stored
    size_t getNumTokens() const;
    
    // Get statistics
    struct Stats {
        size_t num_blocks = 0;
        size_t num_shared_blocks = 0;
        size_t num_cow_blocks = 0;
        double sharing_ratio = 0.0;
    };
    Stats getStats() const;

private:
    std::shared_ptr<PagedBlockManager> block_manager_;
    uint64_t sequence_id_ = 0;
    Config config_;
    
    std::vector<int> block_ids_;          // Physical block IDs
    std::vector<bool> is_shared_;         // Which blocks are shared via CoW
    std::unordered_map<int, int> ref_counts_;  // Reference counts for shared blocks
    
    mutable std::mutex mutex_;
};

} // namespace llm
} // namespace themis

