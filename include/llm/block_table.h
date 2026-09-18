/**
 * @file block_table.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

    /**
     * @brief Allocate blocks for this sequence
     * @param[in] num_blocks Input parameter.
     * @return Return value.
     */
    std::vector<int> allocateBlocks(size_t num_blocks);
    
    /**
     * @brief Release all blocks for this sequence
     */
    void releaseBlocks();
    
    /**
     * @brief Share prefix blocks from parent sequence (Copy-on-Write)
     * @param[in] parent_sequence_id Identifier of the parent sequence.
     * @param[in] prefix_length Input parameter.
     */
    void sharePrefix(uint64_t parent_sequence_id, size_t prefix_length);
    
    /**
     * @brief Get block mapping for attention computation
     * @return Return value.
     */
    std::vector<int> getBlockMapping() const;
    
    /**
     * @brief Get number of tokens stored
     * @return Return value.
     */
    size_t getNumTokens() const;
    
    // Get statistics
    struct Stats {
        size_t num_blocks = 0;
        size_t num_shared_blocks = 0;
        size_t num_cow_blocks = 0;
        double sharing_ratio = 0.0;
    };
    /**
     * @brief Get Stats.
     * @return Return value.
     */
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

