/**
 * @file paged_kv_cache_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

    /**
     * @brief Block table for a sequence
     */
    struct BlockTable {
        uint64_t sequence_id = 0;
        std::vector<int> block_ids;
        size_t num_tokens = 0;
        bool is_prefix_cached = false;

        /**
         * @brief Tenant identifier for cross-tenant KV-cache isolation.
         *
         * Set from `InferenceRequest::tenant_id` before calling addSequence().
         * The cache manager uses this field to ensure that prefix-sharing
         * (Copy-on-Write) never crosses tenant boundaries: two sequences with
         * different `tenant_id` values MUST NOT share a block, even when the
         * cached prefix tokens are byte-for-byte identical.
         *
         * An empty string means the sequence belongs to the default (untenanted)
         * context and may only share blocks with other default-context sequences.
         */
        std::string tenant_id;
    };

    /**
     * @brief Memory statistics
     */
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

    /**
     * @brief Cache type for workload adaptation
     * 
     * NOTE: Cache type is currently tracked as a metric/hint. Future implementation
     * will wire this into actual allocation/eviction behavior (e.g., block allocation
     * strategies, prefix sharing aggressiveness, eviction policies).
     */
    enum class CacheType {
        STANDARD,           // Standard paged cache
        PREFIX_OPTIMIZED,   // Optimized for high prefix reuse (RAG workloads)
        STREAMING           // Optimized for streaming/generation workloads
    };

    /**
     * @brief Workload pattern detected from access patterns
     */
    enum class WorkloadPattern {
        UNKNOWN,
        HIGH_PREFIX_REUSE,  // Many sequences share prefixes (RAG)
        LOW_PREFIX_REUSE,   // Few sequences share prefixes (generation)
        MIXED               // Mixed workload
    };

    /**
     * @brief Workload metrics for dynamic cache selection
     */
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
        int block_id = 0;
        void* device_ptr = nullptr;
        int ref_count = 0;
        bool is_pinned = false;
        uint64_t parent_sequence_id = 0;
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

    /**
     * @brief Get current cache type
     * @return Current cache type
     */
    CacheType getCacheType() const;

    /**
     * @brief Set cache type (manual override)
     * 
     * Manually sets cache type for testing or explicit control.
     * 
     * @param type Cache type to set
     */
    void setCacheType(CacheType type);

    /**
     * @brief Analyze workload and adapt cache type
     * 
     * Analyzes current access patterns and switches cache type
     * if workload pattern has changed significantly.
     * 
     * @return true if cache type was changed
     */
    bool analyzeAndAdaptCacheType();

    /**
     * @brief Get current workload metrics
     * 
     * @return Workload metrics
     */
    WorkloadMetrics getWorkloadMetrics() const;

    /**
     * @brief Enable automatic cache type adaptation
     * 
     * When enabled, cache manager periodically analyzes workload
     * and switches cache type automatically.
     * 
     * @param enable true to enable automatic adaptation
     * @param check_interval_sequences Analyze after N sequences (default: 100)
     */
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
    void initializeBlocks();
    int getFreeBlock();
    void releaseBlock(int block_id);
    size_t calculateBlockMemorySize() const;
    void updateWorkloadMetrics();
    WorkloadPattern detectWorkloadPattern() const;
    CacheType selectOptimalCacheType(WorkloadPattern pattern) const;
};

} // namespace llm
} // namespace themis

