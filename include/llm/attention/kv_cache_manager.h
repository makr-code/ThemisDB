/**
 * @file kv_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "flash_attention_config.h"
#include <vector>
#include <unordered_map>
#include <queue>
#include <memory>
#include <mutex>
#include <cstdint>

namespace themis {
namespace llm {
namespace attention {

/**
 * @brief Tensor wrapper for KV cache data
 */
struct KVTensor {
    virtual ~KVTensor() = default;
    std::vector<float> data;
    size_t layer_id = 0;
    size_t head_id = 0;
    size_t token_pos = 0;
};

/**
 * @brief Block table for mapping logical to physical blocks
 */
struct BlockTable {
    virtual ~BlockTable() = default;
    std::vector<int> block_ids;     // Physical block IDs
    int num_tokens = 0;             // Total tokens in this sequence
    uint64_t sequence_id = 0;       // Sequence identifier
    
    // Prefix sharing support
    bool has_shared_prefix = false;
    uint64_t parent_sequence_id = 0;
    int shared_prefix_length = 0;
};

/**
 * @brief Physical block in KV cache
 */
struct Block {
    virtual ~Block() = default;
    int block_id = -1;
    bool is_free = true;
    int ref_count = 0;              // For prefix sharing (Copy-on-Write)
    std::vector<float> data;        // Actual KV data
};

/**
 * @brief KV-Cache Manager with paged memory allocation
 * 
 * Implements block-based memory management for efficient KV cache:
 * - Paged allocation reduces memory fragmentation
 * - Prefix sharing via Copy-on-Write
 * - Integrates with Flash Attention v3 kernels
 */
class KVCacheManager {
public:
    explicit KVCacheManager(const FlashAttentionConfig& config);
    ~KVCacheManager();
    
    /**
     * @brief Allocate blocks for a new sequence
     * @param seq_id Unique sequence identifier
     * @param expected_tokens Expected number of tokens
     * @return Block table for the sequence
     */
    BlockTable allocateSequence(uint64_t seq_id, int expected_tokens);
    
    /**
     * @brief Free all blocks for a sequence
     * @param seq_id Sequence identifier
     */
    void freeSequence(uint64_t seq_id);
    
    /**
     * @brief Append token KV to cache
     * @param seq_id Sequence identifier
     * @param kv KV tensor data
     */
    void appendToken(uint64_t seq_id, const KVTensor& kv);
    
    /**
     * @brief Share prefix between sequences (Copy-on-Write)
     * @param new_seq_id New sequence ID
     * @param parent_seq_id Parent sequence ID
     * @param prefix_length Length of shared prefix
     */
    void sharePrefix(uint64_t new_seq_id, uint64_t parent_seq_id, int prefix_length);
    
    /**
     * @brief Get block table for sequence
     * @param seq_id Sequence identifier
     * @return Pointer to block table, nullptr if not found
     */
    const BlockTable* getBlockTable(uint64_t seq_id) const;
    
    /**
     * @brief Get physical block data
     * @param block_id Physical block ID
     * @return Pointer to block, nullptr if invalid
     */
    const Block* getBlock(int block_id) const;
    
    /**
     * @brief Get memory statistics
     * @return Memory usage statistics
     */
    AttentionMemoryStats getStats() const;
    
    /**
     * @brief Get number of free blocks
     */
    size_t getFreeBlockCount() const;
    
    /**
     * @brief Get total number of blocks
     */
    size_t getTotalBlockCount() const { return blocks_.size(); }
    
private:
    FlashAttentionConfig config_;
    
    // Physical blocks
    std::vector<Block> blocks_;
    
    // Free block management
    std::queue<int> free_blocks_;
    
    // Sequence to block table mapping
    std::unordered_map<uint64_t, BlockTable> sequences_;
    
    // Thread safety
    mutable std::mutex mutex_;
    
    // Helper methods
    int allocateBlock();
    void freeBlock(int block_id);
    size_t calculateBlockSize() const;
};

} // namespace attention
} // namespace llm
} // namespace themis

