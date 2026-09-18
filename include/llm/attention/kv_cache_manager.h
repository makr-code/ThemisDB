/**
 * @file kv_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct KVTensor {
    /**
     * @brief KVTensor.
     * @return Return value.
     */
    virtual ~KVTensor() = default;
    std::vector<float> data;
    size_t layer_id = 0;
    size_t head_id = 0;
    size_t token_pos = 0;
};

struct BlockTable {
    /**
     * @brief Block Table.
     * @return Return value.
     */
    virtual ~BlockTable() = default;
    std::vector<int> block_ids;     // Physical block IDs
    int num_tokens = 0;             // Total tokens in this sequence
    uint64_t sequence_id = 0;       // Sequence identifier
    
    // Prefix sharing support
    bool has_shared_prefix = false;
    uint64_t parent_sequence_id = 0;
    int shared_prefix_length = 0;
};

struct Block {
    /**
     * @brief Block.
     * @return Return value.
     */
    virtual ~Block() = default;
    int block_id = -1;
    bool is_free = true;
    int ref_count = 0;              // For prefix sharing (Copy-on-Write)
    std::vector<float> data;        // Actual KV data
};

class KVCacheManager {
public:
    /**
     * @brief KVCache Manager.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit KVCacheManager(const FlashAttentionConfig& config);
    ~KVCacheManager();
    
    /**
     * @brief Allocate Sequence.
     * @param[in] seq_id Identifier of the seq.
     * @param[in] expected_tokens Input parameter.
     * @return Return value.
     */
    BlockTable allocateSequence(uint64_t seq_id, int expected_tokens);
    
    /**
     * @brief Free Sequence.
     * @param[in] seq_id Identifier of the seq.
     */
    void freeSequence(uint64_t seq_id);
    
    /**
     * @brief Append Token.
     * @param[in] seq_id Identifier of the seq.
     * @param[in] kv Input parameter.
     */
    void appendToken(uint64_t seq_id, const KVTensor& kv);
    
    /**
     * @brief Share Prefix.
     * @param[in] new_seq_id Identifier of the new seq.
     * @param[in] parent_seq_id Identifier of the parent seq.
     * @param[in] prefix_length Input parameter.
     */
    void sharePrefix(uint64_t new_seq_id, uint64_t parent_seq_id, int prefix_length);
    
    /**
     * @brief Get Block Table.
     * @param[in] seq_id Identifier of the seq.
     * @return Pointer to the result.
     */
    const BlockTable* getBlockTable(uint64_t seq_id) const;
    
    /**
     * @brief Get Block.
     * @param[in] block_id Identifier of the block.
     * @return Pointer to the result.
     */
    const Block* getBlock(int block_id) const;
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    AttentionMemoryStats getStats() const;
    
    /**
     * @brief Get Free Block Count.
     * @return Return value.
     */
    size_t getFreeBlockCount() const;
    
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
    /**
     * @brief Allocate Block.
     * @return Return value.
     */
    int allocateBlock();
    /**
     * @brief Free Block.
     * @param[in] block_id Identifier of the block.
     */
    void freeBlock(int block_id);
    /**
     * @brief Calculate Block Size.
     * @return Return value.
     */
    size_t calculateBlockSize() const;
};

} // namespace attention
} // namespace llm
} // namespace themis

