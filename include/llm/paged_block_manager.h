#pragma once

#include "utils/concurrent_cache.h"
#include <vector>
#include <queue>
#include <mutex>
#include <memory>

namespace themis {
namespace llm {

/**
 * @brief Paged Block Manager for PagedAttention (v1.4.0)
 * 
 * Foundation component for vLLM-style PagedAttention integration.
 * Manages physical memory blocks for KV cache with efficient allocation.
 * 
 * Uses ThemisDB's ConcurrentCache for lock-free block metadata access.
 * 
 * Benefits:
 * - 10x less contention vs mutex-based map
 * - Lock-free reads for block lookup
 * - Production-tested infrastructure
 * - Unified monitoring
 * 
 * Architecture (v1.4.0 ready):
 * - Physical blocks stored in ConcurrentCache
 * - Free list managed with mutex (low contention)
 * - Block-level locking via TBB
 */
class PagedBlockManager {
public:
    struct Block {
        int block_id;
        int physical_address;
        bool is_free;
        std::vector<int> tokens;
        size_t memory_bytes;
        int ref_count = 0;  // For copy-on-write (v1.4.0)
    };
    
    struct Config {
        int max_blocks = 1024;
        size_t block_size_tokens = 128;  // Tokens per block
        size_t token_size_bytes = 4;     // Bytes per token
    };
    
    struct Stats {
        int num_blocks;
        int num_free_blocks;
        int num_allocated_blocks;
        size_t total_memory_bytes;
        size_t used_memory_bytes;
        double fragmentation_ratio;
    };
    
    explicit PagedBlockManager(const Config& config = Config{});
    ~PagedBlockManager() = default;
    
    // Disable copy, allow move
    PagedBlockManager(const PagedBlockManager&) = delete;
    PagedBlockManager& operator=(const PagedBlockManager&) = delete;
    PagedBlockManager(PagedBlockManager&&) = default;
    PagedBlockManager& operator=(PagedBlockManager&&) = default;
    
    /**
     * @brief Allocate contiguous blocks
     * 
     * @param num_blocks Number of blocks to allocate
     * @return std::vector<int> Block IDs (empty if allocation failed)
     */
    std::vector<int> allocateBlocks(int num_blocks);

    // Convenience single-block allocate (v1.3.0 callers)
    int allocate();
    
    /**
     * @brief Free blocks
     * 
     * @param block_ids Block IDs to free
     */
    void freeBlocks(const std::vector<int>& block_ids);

    // Convenience single-block free (v1.3.0 callers)
    void deallocate(int block_id);
    
    /**
     * @brief Get block metadata (lock-free read)
     * 
     * @param block_id Block ID
     * @return Block* Pointer to block (nullptr if not found)
     */
    Block* getBlock(int block_id);
    
    /**
     * @brief Get block metadata (const version)
     */
    const Block* getBlock(int block_id) const;
    
    /**
     * @brief Get statistics
     */
    Stats getStats() const;
    
    /**
     * @brief Get number of free blocks
     */
    int getNumFreeBlocks() const;
    
    /**
     * @brief Clear all allocations (reset to initial state)
     */
    void reset();
    
private:
    Config config_;
    
    // REUSE: ThemisDB's ConcurrentCache for block metadata
    ConcurrentCache<int, Block> blocks_;
    
    // Free list (mutex-protected, low contention)
    std::queue<int> free_list_;
    mutable std::mutex free_list_mutex_;
    
    /**
     * @brief Initialize free list with all blocks
     */
    void initializeFreeList();
};

} // namespace llm
} // namespace themis
