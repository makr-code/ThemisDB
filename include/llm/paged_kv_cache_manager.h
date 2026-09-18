/**
 * @file paged_kv_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <cstddef>
#include <memory>
#include <vector>
#include <unordered_map>
#include <atomic>
#include <string>

namespace themis {
namespace llm {

class PagedKVCacheManager {
public:
    static constexpr size_t BLOCK_SIZE = 16;

    struct Config {
        size_t num_blocks = 4096;          // Total number of blocks
        size_t block_size = BLOCK_SIZE;    // Tokens per block
        size_t num_layers = 32;            // Number of transformer layers
        size_t head_dim = 128;             // Dimension per attention head
        size_t num_kv_heads = 8;           // Number of KV heads
        size_t bytes_per_element = 2;      // FP16 = 2 bytes
        bool enable_prefix_caching = true; // Enable Copy-on-Write
    };

    struct Block {
        int block_id = 0;
        void* device_ptr = nullptr;
        std::atomic<int> ref_count;
        bool is_pinned = false;
        uint64_t parent_sequence_id = 0;  // For CoW tracking
        
        Block() : block_id(0), ref_count(0), is_pinned(false), parent_sequence_id(0) {}
        
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

    struct BlockTable {
        uint64_t sequence_id = 0;
        std::vector<int> block_ids;
        size_t num_tokens = 0;
        bool is_prefix_cached = false;

        std::string tenant_id;
    };

    struct MemoryStats {
        size_t total_blocks = 0;
        size_t used_blocks = 0;
        size_t free_blocks = 0;
        size_t num_sequences = 0;
        double fragmentation_rate = 0.0;
        size_t shared_blocks = 0;
        double prefix_sharing_ratio = 0.0;
        size_t bytes_per_block = 0;
        size_t total_memory_bytes = 0;
        size_t used_memory_bytes = 0;
    };

    enum class CacheType {
        STANDARD,           // Standard paged cache
        PREFIX_OPTIMIZED,   // Optimized for high prefix reuse (RAG workloads)
        STREAMING           // Optimized for streaming/generation workloads
    };

    enum class WorkloadPattern {
        UNKNOWN,
        HIGH_PREFIX_REUSE,  // Many sequences share prefixes (RAG)
        LOW_PREFIX_REUSE,   // Few sequences share prefixes (generation)
        MIXED               // Mixed workload
    };

    struct WorkloadMetrics {
        size_t total_sequences = 0;
        size_t sequences_with_shared_prefix = 0;
        double avg_prefix_length = 0.0;
        double prefix_reuse_ratio = 0.0;
        WorkloadPattern detected_pattern = WorkloadPattern::UNKNOWN;
    };

    PagedKVCacheManager(const Config& config);
    ~PagedKVCacheManager() noexcept;

    /**
     * @brief Allocate Blocks.
     * @param[in] num_blocks Input parameter.
     * @return Return value.
     */
    std::vector<int> allocateBlocks(size_t num_blocks);

    /**
     * @brief Free Blocks.
     * @param[in] block_ids Input parameter.
     */
    void freeBlocks(const std::vector<int>& block_ids);

    /**
     * @brief Enable Prefix Caching.
     * @param[in] seq_id Identifier of the seq.
     * @param[in] parent_seq_id Identifier of the parent seq.
     * @param[in] prefix_length Input parameter.
     * @return True when the operation succeeds.
     */
    bool enablePrefixCaching(
        uint64_t seq_id,
        uint64_t parent_seq_id,
        size_t prefix_length
    );

    /**
     * @brief Get Block Table.
     * @param[in] seq_id Identifier of the seq.
     * @return Return value.
     */
    BlockTable getBlockTable(uint64_t seq_id) const;

    /**
     * @brief Add Sequence.
     * @param[in] seq_id Identifier of the seq.
     * @param[in] num_tokens Input parameter.
     * @return Return value.
     */
    BlockTable addSequence(uint64_t seq_id, size_t num_tokens);

    /**
     * @brief Remove Sequence.
     * @param[in] seq_id Identifier of the seq.
     */
    void removeSequence(uint64_t seq_id);

    /**
     * @brief Get Memory Stats.
     * @return Return value.
     */
    MemoryStats getMemoryStats() const;

    /**
     * @brief Is Block Available.
     * @param[in] block_id Identifier of the block.
     * @return True when the operation succeeds.
     */
    bool isBlockAvailable(int block_id) const;

    struct BlockInfo {
        int block_id = 0;
        void* device_ptr = nullptr;
        int ref_count = 0;
        bool is_pinned = false;
        uint64_t parent_sequence_id = 0;
    };
    
    /**
     * @brief Get Block Info.
     * @param[in] block_id Identifier of the block.
     * @return Return value.
     */
    BlockInfo getBlockInfo(int block_id) const;

    /**
     * @brief Defragment.
     * @return Return value.
     */
    size_t defragment();

    /**
     * @brief Calculate Prefix Savings.
     * @return Return value.
     */
    double calculatePrefixSavings() const;

    /**
     * @brief Get Cache Type.
     * @return Return value.
     */
    CacheType getCacheType() const;

    /**
     * @brief Set Cache Type.
     * @param[in] type Input parameter.
     */
    void setCacheType(CacheType type);

    /**
     * @brief Analyze And Adapt Cache Type.
     * @return True when the operation succeeds.
     */
    bool analyzeAndAdaptCacheType();

    /**
     * @brief Get Workload Metrics.
     * @return Return value.
     */
    WorkloadMetrics getWorkloadMetrics() const;

    void setAutomaticAdaptation(bool enable, size_t check_interval_sequences = 100);

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
    
    // Workload adaptation
    CacheType current_cache_type_{CacheType::STANDARD};
    bool auto_adaptation_enabled_{false};
    size_t adaptation_check_interval_{100};
    size_t sequences_since_last_check_{0};
    WorkloadMetrics workload_metrics_;
    
    // Helper methods
    /**
     * @brief Initialize Blocks.
     */
    void initializeBlocks();
    /**
     * @brief Get Free Block.
     * @return Return value.
     */
    int getFreeBlock();
    /**
     * @brief Release Block.
     * @param[in] block_id Identifier of the block.
     */
    void releaseBlock(int block_id);
    /**
     * @brief Calculate Block Memory Size.
     * @return Return value.
     */
    size_t calculateBlockMemorySize() const;
    /**
     * @brief Update Workload Metrics.
     */
    void updateWorkloadMetrics();
    /**
     * @brief Detect Workload Pattern.
     * @return Return value.
     */
    WorkloadPattern detectWorkloadPattern() const;
    /**
     * @brief Select Optimal Cache Type.
     * @param[in] pattern Input parameter.
     * @return Return value.
     */
    CacheType selectOptimalCacheType(WorkloadPattern pattern) const;
};

} // namespace llm
} // namespace themis

