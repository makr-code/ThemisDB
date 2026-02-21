/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            diff_engine.h                                      ║
  Version:         0.0.19                                             ║
  Last Modified:   2026-02-21 18:59:31                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     290                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • e178371a5  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#ifndef THEMIS_DIFF_ENGINE_H
#define THEMIS_DIFF_ENGINE_H

#include "cdc/changefeed.h"
#include "transaction/snapshot_manager.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace analytics {

using json = nlohmann::json;

/**
 * @brief DiffEngine computes structured differences between two points in time
 * 
 * Provides Git-like diff functionality for ThemisDB's MVCC system by analyzing
 * changefeed events between two sequence numbers, timestamps, or tags (future).
 * 
 * Features:
 * - Diff by sequence range
 * - Diff by timestamp range
 * - Filtering by table, key prefix, event type
 * - Pagination for large result sets
 * - Structured output with Add/Modify/Delete categorization
 * 
 * Performance target: <100ms for 10K changes, <1s for 100K changes
 */
class DiffEngine {
public:
    /**
     * @brief Type of change detected in diff
     */
    enum class ChangeType {
        ADDED,      // Entity was created
        MODIFIED,   // Entity was updated
        DELETED     // Entity was removed
    };

    /**
     * @brief A single change in the diff result
     */
    struct Change {
        ChangeType type;
        std::string key;                         // Affected key
        std::optional<std::string> old_value;    // Value before change (for MODIFIED/DELETED)
        std::optional<std::string> new_value;    // Value after change (for ADDED/MODIFIED)
        uint64_t sequence;                       // Sequence number of change
        int64_t timestamp_ms;                    // Timestamp of change
        json metadata;                           // Additional metadata
        
        json toJson() const;
        static Change fromJson(const json& j);
    };

    /**
     * @brief Statistics about the diff result
     */
    struct DiffStats {
        size_t added_count = 0;
        size_t modified_count = 0;
        size_t deleted_count = 0;
        size_t total_changes = 0;
        
        json toJson() const;
    };

    /**
     * @brief Result of a diff computation
     */
    struct DiffResult {
        std::vector<Change> added;
        std::vector<Change> modified;
        std::vector<Change> deleted;
        DiffStats stats;
        
        uint64_t from_sequence = 0;
        uint64_t to_sequence = 0;
        std::optional<int64_t> from_timestamp_ms;
        std::optional<int64_t> to_timestamp_ms;
        
        json toJson() const;
        static DiffResult fromJson(const json& j);
    };

    /**
     * @brief Options for diff computation
     */
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

    /**
     * @brief Construct DiffEngine
     * @param changefeed Reference to Changefeed instance
     * @param snapshot_manager Optional reference to SnapshotManager for tag-based diff
     */
    explicit DiffEngine(Changefeed& changefeed, 
                       transaction::SnapshotManager* snapshot_manager = nullptr);
    
    ~DiffEngine() = default;

    // Disable copy, allow move
    DiffEngine(const DiffEngine&) = delete;
    DiffEngine& operator=(const DiffEngine&) = delete;
    DiffEngine(DiffEngine&&) = default;
    DiffEngine& operator=(DiffEngine&&) = default;

    /**
     * @brief Compute diff between two sequence numbers
     * @param from_sequence Start sequence (exclusive)
     * @param to_sequence End sequence (inclusive)
     * @param options Diff options
     * @return DiffResult containing all changes
     */
    DiffResult computeDiff(
        uint64_t from_sequence,
        uint64_t to_sequence,
        const DiffOptions& options
    );
    
    DiffResult computeDiff(
        uint64_t from_sequence,
        uint64_t to_sequence
    ) {
        return computeDiff(from_sequence, to_sequence, DiffOptions{});
    }

    /**
     * @brief Compute diff between two timestamps
     * @param from_timestamp Start timestamp in milliseconds (exclusive)
     * @param to_timestamp End timestamp in milliseconds (inclusive)
     * @param options Diff options
     * @return DiffResult containing all changes
     */
    DiffResult computeDiffByTimestamp(
        int64_t from_timestamp,
        int64_t to_timestamp,
        const DiffOptions& options
    );
    
    DiffResult computeDiffByTimestamp(
        int64_t from_timestamp,
        int64_t to_timestamp
    ) {
        return computeDiffByTimestamp(from_timestamp, to_timestamp, DiffOptions{});
    }

    /**
     * @brief Compute diff between two tags
     * @param from_tag Source tag name
     * @param to_tag Target tag name
     * @param options Diff options
     * @return DiffResult containing all changes
     * 
     * Note: Requires SnapshotManager to be provided in constructor.
     * Tags must exist or an error will be thrown.
     */
    DiffResult computeDiffByTag(
        const std::string& from_tag,
        const std::string& to_tag,
        const DiffOptions& options
    );
    
    DiffResult computeDiffByTag(
        const std::string& from_tag,
        const std::string& to_tag
    ) {
        return computeDiffByTag(from_tag, to_tag, DiffOptions{});
    }

    /**
     * @brief Clear any cached diff results
     */
    void clearCache();

    /**
     * @brief Get cache statistics
     */
    json getCacheStats() const;

private:
    Changefeed& changefeed_;
    transaction::SnapshotManager* snapshot_manager_;  // Optional, for tag-based diff
    
    // Cache for frequently requested diffs (simple LRU-like cache)
    // Implementation: map of (from,to) -> DiffResult with timestamp
    struct CachedDiff {
        DiffResult result;
        std::chrono::system_clock::time_point cached_at;
    };
    mutable std::map<std::pair<uint64_t, uint64_t>, CachedDiff> diff_cache_;
    mutable std::mutex cache_mutex_;
    static constexpr std::chrono::seconds CACHE_TTL{300}; // 5 minutes
    static constexpr size_t MAX_CACHE_SIZE = 100;
    static constexpr size_t MAX_DIFF_LIMIT = 1000000; // Maximum allowed limit
    
    /**
     * @brief Process changefeed events and categorize them
     */
    DiffResult processEvents(
        const std::vector<Changefeed::ChangeEvent>& events,
        const DiffOptions& options
    );
    
    /**
     * @brief Apply filters to events
     */
    bool shouldIncludeEvent(
        const Changefeed::ChangeEvent& event,
        const DiffOptions& options
    ) const;
    
    /**
     * @brief Convert ChangeEvent to Change with type determination
     */
    Change categorizeChange(
        const Changefeed::ChangeEvent& event,
        const std::map<std::string, Changefeed::ChangeEvent>& key_history
    ) const;
    
    /**
     * @brief Build key history from events to detect modifications
     */
    std::map<std::string, Changefeed::ChangeEvent> buildKeyHistory(
        const std::vector<Changefeed::ChangeEvent>& events
    ) const;
    
    /**
     * @brief Find sequence numbers for a timestamp range
     */
    std::pair<uint64_t, uint64_t> findSequenceRange(
        int64_t from_timestamp,
        int64_t to_timestamp
    ) const;
    
    /**
     * @brief Check if cached result is still valid
     */
    bool isCacheValid(const CachedDiff& cached) const;
    
    /**
     * @brief Evict oldest cache entries if cache is full
     */
    void evictOldCacheEntries();
};

} // namespace analytics
} // namespace themis

#endif // THEMIS_DIFF_ENGINE_H
