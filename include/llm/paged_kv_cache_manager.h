#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <string>

namespace themis {
namespace llm {

/**
 * @brief Paged KV-Cache Manager with vLLM-inspired architecture
 * 
 * Implements PagedAttention (Zhou et al., OSDI'23) for efficient KV-cache
 * management with block-based allocation and copy-on-write prefix sharing.
 * 
 * Key Features:
 * - Block-based memory allocation (16 tokens per block)
 * - Copy-on-Write for prefix sharing (30-50% memory savings)
 * - Eliminates internal fragmentation
 * - Dynamic block allocation and freeing
 * - Reference counting for shared blocks
 */
class PagedKVCacheManager {
public:
    /**
     * @brief Block size in tokens (optimal: 16)
     */
    static constexpr size_t BLOCK_SIZE = 16;

    /**
     * @brief Configuration for paged KV-cache
     */
    struct Config {
        size_t num_blocks = 4096;          // Total number of blocks
        size_t block_size = BLOCK_SIZE;    // Tokens per block
        size_t num_layers = 32;            // Number of transformer layers
        size_t head_dim = 128;             // Dimension per attention head
        size_t num_kv_heads = 8;           // Number of KV heads
        size_t bytes_per_element = 2;      // FP16 = 2 bytes
        bool enable_prefix_caching = true; // Enable Copy-on-Write
    };

    /**
     * @brief Block metadata
     */
    struct Block {
        int block_id;
        void* device_ptr = nullptr;
        std::atomic<int> ref_count;
        bool is_pinned;
        uint64_t parent_sequence_id;  // For CoW tracking
        
        Block() : ref_count(0), is_pinned(false), parent_sequence_id(0) {}
        
        // Delete copy operations due to atomic
        Block(const Block&) = delete;
        Block& operator=(const Block&) = delete;
        
        // Move operations
        Block(Block&& other) noexcept 
            : block_id(other.block_id)
            , device_ptr(other.device_ptr)
            , ref_count(other.ref_count.load())
            , is_pinned(other.is_pinned)
            , parent_sequence_id(other.parent_sequence_id) {}
        
        Block& operator=(Block&& other) noexcept {
            if (this != &other) {
                block_id = other.block_id;
                device_ptr = other.device_ptr;
                ref_count.store(other.ref_count.load());
                is_pinned = other.is_pinned;
                parent_sequence_id = other.parent_sequence_id;
            }
            return *this;
        }
    };

    /**
     * @brief Block table for a sequence
     */
    struct BlockTable {
        uint64_t sequence_id;
        std::vector<int> block_ids;
        size_t num_tokens;
        bool is_prefix_cached;
    };

    /**
     * @brief Memory statistics
     */
    struct MemoryStats {
        size_t total_blocks;
        size_t used_blocks;
        size_t free_blocks;
        size_t num_sequences;
        double fragmentation_rate;
        double prefix_sharing_ratio;
        size_t bytes_per_block;
        size_t total_memory_bytes;
        size_t used_memory_bytes;
    };

    PagedKVCacheManager(const Config& config);
    ~PagedKVCacheManager();

    /**
     * @brief Allocate blocks for a sequence
     * 
     * @param num_blocks Number of blocks to allocate
     * @return Vector of allocated block IDs
     */
    std::vector<int> allocateBlocks(size_t num_blocks);

    /**
     * @brief Free blocks for a sequence
     * 
     * Decrements reference count and frees blocks when count reaches zero.
     * 
     * @param block_ids Block IDs to free
     */
    void freeBlocks(const std::vector<int>& block_ids);

    /**
     * @brief Enable prefix caching (Copy-on-Write)
     * 
     * Shares prefix blocks between parent and child sequence.
     * Child only allocates new blocks when diverging from parent.
     * 
     * @param seq_id Child sequence ID
     * @param parent_seq_id Parent sequence ID
     * @param prefix_length Length of shared prefix in tokens
     * @return true if prefix caching succeeded
     */
    bool enablePrefixCaching(
        uint64_t seq_id,
        uint64_t parent_seq_id,
        size_t prefix_length
    );

    /**
     * @brief Get block table for a sequence
     * 
     * @param seq_id Sequence ID
     * @return Block table (empty if sequence not found)
     */
    BlockTable getBlockTable(uint64_t seq_id) const;

    /**
     * @brief Add sequence with its block table
     * 
     * @param seq_id Sequence ID
     * @param num_tokens Number of tokens in sequence
     * @return Block table for the sequence
     */
    BlockTable addSequence(uint64_t seq_id, size_t num_tokens);

    /**
     * @brief Remove sequence and free its blocks
     * 
     * @param seq_id Sequence ID
     */
    void removeSequence(uint64_t seq_id);

    /**
     * @brief Get memory statistics
     * 
     * @return Current memory statistics
     */
    MemoryStats getMemoryStats() const;

    /**
     * @brief Check if a block is available
     * 
     * @param block_id Block ID
     * @return true if block is allocated and valid
     */
    bool isBlockAvailable(int block_id) const;

    /**
     * @brief Block information (copy-safe)
     */
    struct BlockInfo {
        int block_id;
        void* device_ptr = nullptr;
        int ref_count;
        bool is_pinned;
        uint64_t parent_sequence_id;
    };
    
    /**
     * @brief Get block information
     * 
     * @param block_id Block ID
     * @return Block information
     */
    BlockInfo getBlockInfo(int block_id) const;

    /**
     * @brief Defragment memory
     * 
     * Compacts allocated blocks to reduce fragmentation.
     * 
     * @return Number of blocks compacted
     */
    size_t defragment();

    /**
     * @brief Calculate memory savings from prefix caching
     * 
     * @return Percentage of memory saved (0.0 - 100.0)
     */
    double calculatePrefixSavings() const;

private:
    Config config_;
    
    // Block management
    std::vector<Block> blocks_;
    std::vector<int> free_block_ids_;
    
    // Sequence to block table mapping
    std::unordered_map<uint64_t, BlockTable> sequence_tables_;
    
    // Prefix caching tracking
    std::unordered_map<uint64_t, uint64_t> parent_map_;  // child -> parent
    
    // Statistics
    std::atomic<size_t> total_blocks_allocated_{0};
    std::atomic<size_t> total_blocks_shared_{0};
    
    // Helper methods
    void initializeBlocks();
    int getFreeBlock();
    void releaseBlock(int block_id);
    size_t calculateBlockMemorySize() const;
};

} // namespace llm
} // namespace themis
