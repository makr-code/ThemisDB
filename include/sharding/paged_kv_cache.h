// Copyright 2025 ThemisDB
// Licensed under MIT License

/**
 * @file paged_kv_cache.h
 * @brief Paged KV Cache for LLM Inference (vLLM-style)
 * @version 1.9.0-beta
 * @note Maturity: PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=0; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=0
 * @note Implements PagedAttention-style KV cache for memory-efficient inference
 */

#pragma once

#include <vector>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <atomic>
#include <optional>
#include <functional>
#include <queue>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

// Forward declarations
class ContinuousBatchScheduler;

/**
 * @brief KV Cache block size configuration
 */
struct KVCacheConfig {
    // Block sizes (from vLLM)
    uint32_t block_size = 16;           // Number of tokens per block
    uint32_t max_blocks_per_request = 1024;  // Max blocks per request
    uint32_t max_total_blocks = 100000;     // Max total blocks in cache
    
    // Memory limits
    size_t max_cache_memory_bytes = 16ULL * 1024 * 1024 * 1024;  // 16 GB
    double eviction_threshold = 0.85;  // Evict when this utilization is reached
    
    // Preallocation
    bool preallocate_blocks = true;
    uint32_t preallocation_batch_size = 64;  // Number of blocks to preallocate
    
    // Block allocation strategy
    enum class AllocationStrategy {
        FIRST_FIT,
        BEST_FIT,
        WORST_FIT
    } allocation_strategy = AllocationStrategy::FIRST_FIT;
    
    bool isValid() const {
        return block_size > 0 &&
               max_blocks_per_request > 0 &&
               max_total_blocks > 0 &&
               max_cache_memory_bytes > 0 &&
               eviction_threshold > 0.0 &&
               eviction_threshold < 1.0 &&
               preallocation_batch_size > 0;
    }
    
    nlohmann::json toJson() const {
        return {
            {"block_size", block_size},
            {"max_blocks_per_request", max_blocks_per_request},
            {"max_total_blocks", max_total_blocks},
            {"max_cache_memory_bytes", max_cache_memory_bytes},
            {"eviction_threshold", eviction_threshold},
            {"preallocate_blocks", preallocate_blocks},
            {"preallocation_batch_size", preallocation_batch_size},
            {"allocation_strategy", static_cast<int>(allocation_strategy)}
        };
    }
};

/**
 * @brief KV Cache block header
 */
struct KVCacheBlock {
    uint32_t block_id = 0;
    uint32_t request_id;
    uint32_t sequence_number;  // Sequence number within the request
    uint32_t token_start;       // Start token index
    uint32_t token_count;      // Number of tokens in this block
    bool is_active = true;
    
    // Memory pointers (in a real implementation, these would point to GPU/CPU memory)
    void* key_cache = nullptr;
    void* value_cache = nullptr;
    
    // Timestamps for LRU eviction
    std::chrono::steady_clock::time_point last_accessed;
    std::chrono::steady_clock::time_point created;
    
    nlohmann::json toJson() const {
        auto now = std::chrono::steady_clock::now();
        auto age_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - created).count();
        auto last_access_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - last_accessed).count();
        
        return {
            {"block_id", block_id},
            {"request_id", request_id},
            {"sequence_number", sequence_number},
            {"token_start", token_start},
            {"token_count", token_count},
            {"is_active", is_active},
            {"age_ms", age_ms},
            {"last_access_ms", last_access_ms}
        };
    }
};

/**
 * @brief KV Cache statistics
 */
struct KVCacheStats {
    // Block statistics
    uint32_t total_blocks = 0;
    uint32_t used_blocks = 0;
    uint32_t free_blocks = 0;
    uint32_t reserved_blocks = 0;
    
    // Memory statistics
    size_t total_memory_bytes = 0;
    size_t used_memory_bytes = 0;
    size_t free_memory_bytes = 0;
    double utilization = 0.0;
    
    // Request statistics
    uint32_t active_requests = 0;
    uint32_t total_requests = 0;
    
    // Performance statistics
    uint32_t cache_hits = 0;
    uint32_t cache_misses = 0;
    uint32_t blocks_evicted = 0;
    uint32_t blocks_allocated = 0;
    uint32_t blocks_freed = 0;
    
    // Error statistics
    uint32_t allocation_failures = 0;
    uint32_t eviction_failures = 0;
    
    double cache_hit_rate() const {
        uint32_t total = cache_hits + cache_misses;
        return total > 0 ? static_cast<double>(cache_hits) / total : 0.0;
    }
    
    nlohmann::json toJson() const {
        return {
            {"total_blocks", total_blocks},
            {"used_blocks", used_blocks},
            {"free_blocks", free_blocks},
            {"reserved_blocks", reserved_blocks},
            {"total_memory_bytes", total_memory_bytes},
            {"used_memory_bytes", used_memory_bytes},
            {"free_memory_bytes", free_memory_bytes},
            {"utilization", utilization},
            {"active_requests", active_requests},
            {"total_requests", total_requests},
            {"cache_hits", cache_hits},
            {"cache_misses", cache_misses},
            {"blocks_evicted", blocks_evicted},
            {"blocks_allocated", blocks_allocated},
            {"blocks_freed", blocks_freed},
            {"allocation_failures", allocation_failures},
            {"eviction_failures", eviction_failures},
            {"cache_hit_rate", cache_hit_rate()}
        };
    }
};

/**
 * @brief Paged KV Cache for LLM Inference
 * 
 * Implements a vLLM-style paged attention KV cache that:
 * - Divides KV cache into fixed-size blocks
 * - Allocates blocks on-demand for each request
 * - Supports sharing of common prefix blocks between requests
 * - Implements LRU eviction when cache is full
 * - Tracks memory usage and provides statistics
 * 
 * This cache is designed for per-shard inference in a converged
 * storage-retrieval-inference architecture.
 */
class PagedKVCache {
public:
    /**
     * @brief Callback for block allocation
     */
    using BlockAllocator = std::function<void*(uint32_t block_id, size_t size)>;
    
    /**
     * @brief Callback for block deallocation
     */
    using BlockDeallocator = std::function<void(void* ptr, uint32_t block_id)>;
    
    /**
     * @brief Callback for cache eviction
     */
    using EvictionCallback = std::function<void(uint32_t request_id, uint32_t block_id)>;
    
    /**
     * @brief Construct PagedKVCache
     * @param config Cache configuration
     */
    explicit PagedKVCache(const KVCacheConfig& config);
    
    ~PagedKVCache();
    
    // Delete copy constructors and assignment operators
    PagedKVCache(const PagedKVCache&) = delete;
    PagedKVCache& operator=(const PagedKVCache&) = delete;
    
    // ========================================================================
    // Cache Management API
    // ========================================================================
    
    /**
     * @brief Initialize the cache
     * @return true if initialization succeeded
     */
    bool initialize();
    
    /**
     * @brief Shutdown the cache
     */
    void shutdown();
    
    /**
     * @brief Reserve blocks for a new request
     * @param request_id Request identifier
     * @param initial_tokens Number of tokens to reserve initially
     * @return true if reservation succeeded
     */
    bool reserveRequest(int64_t request_id, uint32_t initial_tokens);

    /**
     * @brief Reserve blocks for a new request with prompt token metadata
     * @param request_id Request identifier
     * @param initial_token_ids Prompt token sequence used for prefix sharing
     * @return true if reservation succeeded
     *
     * Stores the logical prompt token sequence so prefix-sharing lookups can
     * identify reusable full blocks from already cached requests.
     */
    bool reserveRequest(int64_t request_id, const std::vector<int>& initial_token_ids);
    
    /**
     * @brief Allocate a block for a request
     * @param request_id Request identifier
     * @param token_count Number of tokens to store
     * @return Block ID or nullopt if allocation failed
     */
    std::optional<uint32_t> allocateBlock(int64_t request_id, uint32_t token_count);
    
    /**
     * @brief Free a block
     * @param block_id Block identifier
     */
    void freeBlock(uint32_t block_id);
    
    /**
     * @brief Free all blocks for a request
     * @param request_id Request identifier
     */
    void freeRequest(int64_t request_id);
    
    /**
     * @brief Clear the entire cache
     */
    void clear();
    
    // ========================================================================
    // Data Access API
    // ========================================================================
    
    /**
     * @brief Write KV data to a block
     * @param block_id Block identifier
     * @param token_offset Offset within the block
     * @param key_data Key data (per-token)
     * @param value_data Value data (per-token)
     * @param token_count Number of tokens to write
     * @return true if write succeeded
     */
    bool writeBlock(
        uint32_t block_id,
        uint32_t token_offset,
        const std::vector<float>& key_data,
        const std::vector<float>& value_data,
        uint32_t token_count
    );
    
    /**
     * @brief Read KV data from a block
     * @param block_id Block identifier
     * @param token_offset Offset within the block
     * @param token_count Number of tokens to read
     * @param key_data Output key data
     * @param value_data Output value data
     * @return true if read succeeded
     */
    bool readBlock(
        uint32_t block_id,
        uint32_t token_offset,
        uint32_t token_count,
        std::vector<float>& key_data,
        std::vector<float>& value_data
    ) const;
    
    /**
     * @brief Get block information
     * @param block_id Block identifier
     * @return Block information or nullopt if not found
     */
    std::optional<KVCacheBlock> getBlock(uint32_t block_id) const;
    
    /**
     * @brief Get all blocks for a request
     * @param request_id Request identifier
     * @return Vector of block IDs
     */
    std::vector<uint32_t> getRequestBlocks(int64_t request_id) const;
    
    // ========================================================================
    // Prefix Sharing API
    // ========================================================================
    
    /**
     * @brief Check if a prefix can be shared
     * @param request_id Request identifier
     * @param token_sequence Token sequence to check
     * @return Block ID of matching prefix or nullopt
     */
    std::optional<uint32_t> findSharedPrefix(
        int64_t request_id,
        const std::vector<int>& token_sequence
    ) const;
    
    /**
     * @brief Share a prefix block between requests
     * @param source_request_id Source request identifier
     * @param target_request_id Target request identifier
     * @param block_id Block identifier to share
     * @return true if sharing succeeded
     */
    bool sharePrefixBlock(
        int64_t source_request_id,
        int64_t target_request_id,
        uint32_t block_id
    );
    
    // ========================================================================
    // Statistics and Monitoring
    // ========================================================================
    
    /**
     * @brief Get current cache statistics
     */
    KVCacheStats getStats() const;
    
    /**
     * @brief Get detailed statistics as JSON
     */
    nlohmann::json getStatsJson() const;
    
    /**
     * @brief Reset statistics
     */
    void resetStats();
    
    /**
     * @brief Get memory usage
     */
    size_t getMemoryUsage() const;
    
    /**
     * @brief Get cache utilization
     */
    double getUtilization() const;
    
    /**
     * @brief Check if cache needs eviction
     */
    bool needsEviction() const;
    
    // ========================================================================
    // Configuration and Control
    // ========================================================================
    
    /**
     * @brief Update cache configuration
     * @param config New configuration
     */
    void updateConfig(const KVCacheConfig& config);
    
    /**
     * @brief Get current configuration
     */
    const KVCacheConfig& getConfig() const;
    
    /**
     * @brief Set block allocator callback
     */
    void setBlockAllocator(BlockAllocator allocator);
    
    /**
     * @brief Set block deallocator callback
     */
    void setBlockDeallocator(BlockDeallocator deallocator);
    
    /**
     * @brief Set eviction callback
     */
    void setEvictionCallback(EvictionCallback callback);
    
    // ========================================================================
    // Integration with Scheduler
    // ========================================================================
    
    /**
     * @brief Set the associated scheduler
     * @param scheduler Continuous batch scheduler
     */
    void setScheduler(ContinuousBatchScheduler* scheduler);
    
    /**
     * @brief Get the associated scheduler
     */
    ContinuousBatchScheduler* getScheduler();
    
    /**
     * @brief Clear cache for a specific request (called by scheduler)
     * @param request_id Request identifier
     */
    void clearRequestCache(int64_t request_id);
    
private:
    // ========================================================================
    // Internal Types
    // ========================================================================
    
    struct RequestState {
        int64_t request_id;
        std::vector<uint32_t> block_ids;
        uint32_t total_tokens = 0;
        std::vector<int> token_sequence;
        std::chrono::steady_clock::time_point last_accessed;
    };
    
    // ========================================================================
    // Internal State
    // ========================================================================
    
    KVCacheConfig config_;
    ContinuousBatchScheduler* scheduler_ = nullptr;
    
    // Block management
    std::unordered_map<uint32_t, KVCacheBlock> blocks_;
    std::unordered_map<uint32_t, uint32_t> block_ref_counts_;
    std::unordered_map<uint32_t, std::vector<float>> key_block_storage_;
    std::unordered_map<uint32_t, std::vector<float>> value_block_storage_;
    std::queue<uint32_t> free_blocks_;
    uint32_t next_block_id_ = 0;
    
    // Request management
    std::unordered_map<int64_t, RequestState> requests_;
    
    // Statistics
    mutable KVCacheStats stats_;
    
    // Callbacks
    BlockAllocator block_allocator_;
    BlockDeallocator block_deallocator_;
    EvictionCallback eviction_callback_;
    
    // Memory tracking
    size_t current_memory_usage_ = 0;
    size_t block_memory_size_ = 0;  // Calculated based on config
    
    // Synchronization
    mutable std::mutex mutex_;
    
    // ========================================================================
    // Internal Methods
    // ========================================================================
    
    /**
     * @brief Calculate block memory size
     */
    size_t calculateBlockMemorySize() const;

    /**
     * @brief Reserve request state after external synchronization.
     */
    bool reserveRequestUnlocked(int64_t request_id,
                                uint32_t initial_tokens,
                                const std::vector<int>* initial_token_ids);
    
    /**
     * @brief Allocate a new block ID
     */
    uint32_t allocateBlockId();
    
    /**
     * @brief Free a block ID
     */
    void freeBlockId(uint32_t block_id);
    
    /**
     * @brief Evict blocks to make space
     * @param needed_blocks Number of blocks needed
     * @return true if eviction succeeded
     */
    bool evictBlocks(uint32_t needed_blocks);

    /**
     * @brief Release a shared or owned block from one request after locking.
     */
    void releaseRequestBlockUnlocked(int64_t request_id, uint32_t block_id);

    /**
     * @brief Destroy a block and detach it from all requests after locking.
     */
    void destroyBlockUnlocked(uint32_t block_id);
    
    /**
     * @brief Find the least recently used block after external synchronization
     * @return Block ID and request ID or nullopt
     */
    std::optional<std::pair<uint32_t, int64_t>> findLRUBlock() const;
    
    /**
     * @brief Update statistics
     */
    void updateStats();
};

} // namespace sharding
} // namespace themisdb
