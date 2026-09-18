/**
 * @file diff_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "cdc/changefeed.h"
#include "transaction/snapshot_manager.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <unordered_set>
#include <nlohmann/json.hpp>

namespace themis {
namespace analytics {

using json = nlohmann::json;

class DiffEngine {
public:
    enum class ChangeType {
        ADDED,      // Entity was created
        MODIFIED,   // Entity was updated
        DELETED     // Entity was removed
    };

    struct Change {
        ChangeType type;
        std::string key;                         // Affected key
        std::optional<std::string> old_value;    // Value before change (for MODIFIED/DELETED)
        std::optional<std::string> new_value;    // Value after change (for ADDED/MODIFIED)
        uint64_t sequence;                       // Sequence number of change
        int64_t timestamp_ms;                    // Timestamp of change
        json metadata;                           // Additional metadata
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static Change fromJson(const json& j);
    };

    struct DiffStats {
        size_t added_count = 0;
        size_t modified_count = 0;
        size_t deleted_count = 0;
        size_t total_changes = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };

    struct DiffResult {
        std::vector<Change> added;
        std::vector<Change> modified;
        std::vector<Change> deleted;
        DiffStats stats;
        
        uint64_t from_sequence = 0;
        uint64_t to_sequence = 0;
        std::optional<int64_t> from_timestamp_ms;
        std::optional<int64_t> to_timestamp_ms;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
        /**
         * @brief From Json.
         * @param[in] j Input parameter.
         * @return Return value.
         */
        static DiffResult fromJson(const json& j);
    };

    struct DiffOptions {
        // Filtering
        std::optional<std::string> table_filter;      // Filter by table name
        std::optional<std::string> key_prefix;        // Filter by key prefix
        bool include_values = true;                   // Include actual values in result
        
        // Pagination
        size_t limit = 1000;                          // Maximum changes to return (0 = no limit)
        size_t offset = 0;                            // Skip first N changes
        
        // Performance
        bool enable_caching = true;                   // Cache intermediate results
    };

    explicit DiffEngine(Changefeed& changefeed, 
                       transaction::SnapshotManager* snapshot_manager = nullptr);
    
    ~DiffEngine() = default;

    // Disable copy; move is implicitly deleted because std::mutex / std::condition_variable
    // are non-movable — keep = default so the compiler enforces this clearly.
    DiffEngine(const DiffEngine&) = delete;
    DiffEngine& operator=(const DiffEngine&) = delete;
    DiffEngine(DiffEngine&&) = delete;
    DiffEngine& operator=(DiffEngine&&) = delete;

    /**
     * @brief Compute Diff.
     * @param[in] from_sequence Input parameter.
     * @param[in] to_sequence Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    DiffResult computeDiff(
        uint64_t from_sequence,
        uint64_t to_sequence,
        const DiffOptions& options
    );
    
    /**
     * @brief Compute Diff.
     * @param[in] from_sequence Input parameter.
     * @param[in] to_sequence Input parameter.
     * @return Return value.
     * @details Implements computeDiff without additional internal calls.
     */
    DiffResult computeDiff(
        uint64_t from_sequence,
        uint64_t to_sequence
    ) {
        return computeDiff(from_sequence, to_sequence, DiffOptions{});
    }

    /**
     * @brief Compute Diff By Timestamp.
     * @param[in] from_timestamp Input parameter.
     * @param[in] to_timestamp Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    DiffResult computeDiffByTimestamp(
        int64_t from_timestamp,
        int64_t to_timestamp,
        const DiffOptions& options
    );
    
    /**
     * @brief Compute Diff By Timestamp.
     * @param[in] from_timestamp Input parameter.
     * @param[in] to_timestamp Input parameter.
     * @return Return value.
     * @details Implements computeDiffByTimestamp without additional internal calls.
     */
    DiffResult computeDiffByTimestamp(
        int64_t from_timestamp,
        int64_t to_timestamp
    ) {
        return computeDiffByTimestamp(from_timestamp, to_timestamp, DiffOptions{});
    }

    /**
     * @brief Compute Diff By Tag.
     * @param[in] from_tag Input parameter.
     * @param[in] to_tag Input parameter.
     * @param[in] options Input parameter.
     * @return Return value.
     */
    DiffResult computeDiffByTag(
        const std::string& from_tag,
        const std::string& to_tag,
        const DiffOptions& options
    );
    
    /**
     * @brief Compute Diff By Tag.
     * @param[in] from_tag Input parameter.
     * @param[in] to_tag Input parameter.
     * @return Return value.
     * @details Implements computeDiffByTag without additional internal calls.
     */
    DiffResult computeDiffByTag(
        const std::string& from_tag,
        const std::string& to_tag
    ) {
        return computeDiffByTag(from_tag, to_tag, DiffOptions{});
    }

    /**
     * @brief Clear Cache.
     */
    void clearCache();

    /**
     * @brief Get Cache Stats.
     * @return Return value.
     */
    json getCacheStats() const;

    void setComputeHookForTesting(std::function<void()> hook) {
        compute_hook_for_testing_ = std::move(hook);
    }

private:
    Changefeed& changefeed_;
    transaction::SnapshotManager* snapshot_manager_;  // Optional, for tag-based diff
    
    // Cache for frequently requested diffs (simple LRU-like cache)
    // Implementation: map of (from,to) -> DiffResult with timestamp
    struct CachedDiff {
        DiffResult result;
        std::chrono::system_clock::time_point cached_at;
    };

    // Cache key type: (from_sequence, to_sequence)
    using CacheKey = std::pair<uint64_t, uint64_t>;

    // Hash functor for CacheKey (std::pair has no default hash)
    struct CacheKeyHash {
        std::size_t operator()(const CacheKey& k) const noexcept {
            std::size_t h1 = std::hash<uint64_t>{}(k.first);
            std::size_t h2 = std::hash<uint64_t>{}(k.second);
            // Mix with a large prime to reduce collisions
            return h1 ^ (h2 * 0x9e3779b97f4a7c15ULL + (h1 << 6) + (h1 >> 2));
        }
    };

    mutable std::map<CacheKey, CachedDiff> diff_cache_;
    mutable std::mutex cache_mutex_;
    mutable std::condition_variable inflight_cv_;
    mutable std::unordered_set<CacheKey, CacheKeyHash> inflight_keys_;
    static constexpr std::chrono::seconds CACHE_TTL{300}; // 5 minutes
    static constexpr size_t MAX_CACHE_SIZE = 100;
    static constexpr size_t MAX_DIFF_LIMIT = 1000000; // Maximum allowed limit

    // Testing hook — see setComputeHookForTesting()
    std::function<void()> compute_hook_for_testing_;
    
    /**
     * @brief Process Events.
     * @param[in] events Input parameter.
     * @param[in] options Input parameter.
     * @param[in] from_sequence Input parameter.
     * @return Return value.
     */
    DiffResult processEvents(
        const std::vector<Changefeed::ChangeEvent>& events,
        const DiffOptions& options,
        uint64_t from_sequence
    );
    
    /**
     * @brief Should Include Event.
     * @param[in] event Input parameter.
     * @param[in] options Input parameter.
     * @return True when the operation succeeds.
     */
    bool shouldIncludeEvent(
        const Changefeed::ChangeEvent& event,
        const DiffOptions& options
    ) const;
    
    Change categorizeChange(
        const Changefeed::ChangeEvent& event,
        const std::map<std::string, Changefeed::ChangeEvent>& key_history
    ) const;
    
    std::map<std::string, Changefeed::ChangeEvent> buildKeyHistory(
        const std::vector<Changefeed::ChangeEvent>& events
    ) const;
    
    std::pair<uint64_t, uint64_t> findSequenceRange(
        int64_t from_timestamp,
        int64_t to_timestamp
    ) const;
    
    /**
     * @brief Is Cache Valid.
     * @param[in] cached Input parameter.
     * @return True when the operation succeeds.
     */
    bool isCacheValid(const CachedDiff& cached) const;
    
    /**
     * @brief Evict Old Cache Entries.
     */
    void evictOldCacheEntries();
};

} // namespace analytics
} // namespace themis
