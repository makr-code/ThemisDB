/**
 * @file paged_block_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
    
    explicit PagedBlockManager(const Config& config);
    ~PagedBlockManager() = default;
    
    // Disable copy, allow move
    PagedBlockManager(const PagedBlockManager&) = delete;
    PagedBlockManager& operator=(const PagedBlockManager&) = delete;
    PagedBlockManager(PagedBlockManager&&) noexcept = default;
    PagedBlockManager& operator=(PagedBlockManager&&) noexcept = default;
    
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
     * @brief Safe accessor pattern - executes callback with block reference
     * 
     * Prevents dangling pointer issues by passing block as reference to callback.
     * Callback is executed only if block exists.
     * 
     * @param block_id Block ID
     * @param callback Function to execute with block reference
     * 
     * Example usage:
     *   mgr.withBlock(block_id, [](const Block& block) {
     *       std::cout << "Block ID: " << block.block_id << std::endl;
     *   });
     */
    void withBlock(int block_id, std::function<void(const Block&)> callback) const;
    
    /**
     * @brief Get read-only reference to block (safe alternative to pointer)
     * 
     * Returns std::nullopt if block not found.
     * 
     * @param block_id Block ID
     * @return std::optional<std::reference_wrapper<const Block>> Reference to block or nullopt
     * 
     * Example usage:
     *   if (auto block_ref = mgr.getBlockRef(block_id)) {
     *       const Block& block = block_ref->get();
     *       std::cout << "Block ID: " << block.block_id << std::endl;
     *   }
     */
    std::optional<std::reference_wrapper<const Block>> getBlockRef(int block_id) const;
    
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

