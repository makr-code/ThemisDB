/**
 * @file paged_block_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "utils/concurrent_cache.h"
#include <vector>
#include <queue>
#include <mutex>
#include <memory>
#include <functional>
#include <optional>

namespace themis {
namespace llm {

class PagedBlockManager {
public:
    struct Block {
        int block_id = 0;
        int physical_address = 0;
        bool is_free = true;
        std::vector<int> tokens;
        size_t memory_bytes = 0;
        int ref_count = 0;  // For copy-on-write (v1.4.0)
    };
    
    struct Config {
        int max_blocks = 1024;
        // Backward-compat alias expected by tests
        int total_blocks = 0;
        size_t block_size_tokens = 128;  // Tokens per block
        size_t token_size_bytes = 4;     // Bytes per token
    };
    
    struct Stats {
        int num_blocks = 0;
        int num_free_blocks = 0;
        int num_allocated_blocks = 0;
        size_t total_memory_bytes = 0;
        size_t used_memory_bytes = 0;
        double fragmentation_ratio = 0.0;
    };
    
    /**
     * @brief Paged Block Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit PagedBlockManager(const Config& config);
    ~PagedBlockManager() = default;
    
    // Disable copy, allow move
    PagedBlockManager(const PagedBlockManager&) = delete;
    PagedBlockManager& operator=(const PagedBlockManager&) = delete;
    PagedBlockManager(PagedBlockManager&&) noexcept = default;
    PagedBlockManager& operator=(PagedBlockManager&&) noexcept = default;
    
    /**
     * @brief Allocate Blocks.
     * @param[in] num_blocks Input parameter.
     * @return Return value.
     */
    std::vector<int> allocateBlocks(int num_blocks);

    /**
     * @brief Convenience single-block allocate (v1.
     * @return Return value.
     * @details 3.0 callers)
     */
    int allocate();
    
    /**
     * @brief Free Blocks.
     * @param[in] block_ids Input parameter.
     */
    void freeBlocks(const std::vector<int>& block_ids);

    /**
     * @brief Convenience single-block free (v1.
     * @param[in] block_id Identifier of the block.
     * @details 3.0 callers)
     */
    void deallocate(int block_id);
    
    void withBlock(int block_id, std::function<void(const Block&)> callback) const;
    
    /**
     * @brief Get Block Ref.
     * @param[in] block_id Identifier of the block.
     * @return Return value.
     */
    std::optional<std::reference_wrapper<const Block>> getBlockRef(int block_id) const;
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    Stats getStats() const;
    
    /**
     * @brief Get Num Free Blocks.
     * @return Return value.
     */
    int getNumFreeBlocks() const;
    
    /**
     * @brief Reset the modification detection flag.
     */
    void reset();
    
private:
    Config config_;
    
    // Stable storage for block metadata so reference-based accessors stay valid
    // across updates and rehashes.
    std::unordered_map<int, std::shared_ptr<Block>> block_store_;
    mutable std::mutex block_store_mutex_;
    
    // REUSE: ThemisDB's ConcurrentCache for block metadata
    ConcurrentCache<int, Block> blocks_;
    
    // Free list (mutex-protected, low contention)
    std::queue<int> free_list_;
    mutable std::mutex free_list_mutex_;
    
    /**
     * @brief Initialize Free List.
     */
    void initializeFreeList();
};

} // namespace llm
} // namespace themis

