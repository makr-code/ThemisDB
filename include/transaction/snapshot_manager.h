/**
 * @file snapshot_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/rocksdb_wrapper.h"
#include "cdc/changefeed.h"
#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themis {
namespace transaction {

using json = nlohmann::json;

/**
 * @brief SnapshotManager provides Git-like named snapshots for ThemisDB's MVCC system
 * 
 * Enables semantic tagging of database states for:
 * - Audit/compliance checkpoints
 * - Pre-deployment safe points
 * - Point-in-time recovery markers
 * - Tag-based diff operations
 * 
 * Features:
 * - Named tags with descriptions
 * - Persistent storage in RocksDB
 * - Tag CRUD operations
 * - Integration with Changefeed for sequence mapping
 * - REST API endpoints
 */
class SnapshotManager {
public:
    /**
     * @brief Snapshot metadata
     */
    struct Snapshot {
        std::string tag_name;           // Unique tag identifier
        uint64_t sequence_number;       // Changefeed sequence at tag creation
        int64_t timestamp_ms;           // Unix timestamp in milliseconds
        std::string description;        // Human-readable description
        std::string created_by;         // User/service that created the tag
        
        json toJson() const;
        static Snapshot fromJson(const json& j);
    };
    
    /**
     * @brief Statistics about snapshots
     */
    struct SnapshotStats {
        size_t total_snapshots = 0;
        int64_t oldest_timestamp_ms = 0;
        int64_t newest_timestamp_ms = 0;
        uint64_t oldest_sequence = 0;
        uint64_t newest_sequence = 0;
        
        json toJson() const;
    };
    
    /**
     * @brief Construct SnapshotManager
     * @param db Reference to RocksDB wrapper
     * @param changefeed Reference to Changefeed for sequence numbers
     */
    explicit SnapshotManager(RocksDBWrapper& db, Changefeed& changefeed);
    
    ~SnapshotManager() = default;

    // Disable copy, allow move
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;
    SnapshotManager(SnapshotManager&&) = default;
    SnapshotManager& operator=(SnapshotManager&&) = default;

    /**
     * @brief Create a new named snapshot/tag
     * @param tag_name Unique tag name (alphanumeric, hyphens, underscores)
     * @param description Human-readable description
     * @param created_by Optional user/service identifier
     * @return Snapshot metadata if successful, nullopt on error
     * 
     * Error conditions:
     * - Tag name already exists
     * - Invalid tag name format
     * - Database write failure
     */
    std::optional<Snapshot> createTag(
        const std::string& tag_name,
        const std::string& description,
        const std::string& created_by = "system"
    );
    
    /**
     * @brief Get snapshot metadata by tag name
     * @param tag_name Tag to retrieve
     * @return Snapshot metadata if found, nullopt otherwise
     */
    std::optional<Snapshot> getTag(const std::string& tag_name) const;
    
    /**
     * @brief List all snapshots
     * @param limit Maximum number of snapshots to return (0 = all)
     * @param sort_by Sort order: "timestamp" (default), "sequence", "name"
     * @param ascending Sort direction (default: false = newest first)
     * @return Vector of snapshot metadata
     */
    std::vector<Snapshot> listTags(
        size_t limit = 0,
        const std::string& sort_by = "timestamp",
        bool ascending = false
    ) const;
    
    /**
     * @brief Delete a snapshot/tag
     * @param tag_name Tag to delete
     * @return true if deleted, false if not found or error
     */
    bool deleteTag(const std::string& tag_name);
    
    /**
     * @brief Check if a tag exists
     * @param tag_name Tag to check
     * @return true if exists, false otherwise
     */
    bool tagExists(const std::string& tag_name) const;
    
    /**
     * @brief Get statistics about all snapshots
     * @return Snapshot statistics
     */
    SnapshotStats getStats() const;
    
    /**
     * @brief Get sequence number for a tag
     * @param tag_name Tag to query
     * @return Sequence number if found, nullopt otherwise
     */
    std::optional<uint64_t> getSequenceForTag(const std::string& tag_name) const;
    
    /**
     * @brief Get timestamp for a tag
     * @param tag_name Tag to query
     * @return Timestamp in milliseconds if found, nullopt otherwise
     */
    std::optional<int64_t> getTimestampForTag(const std::string& tag_name) const;
    
    /**
     * @brief Validate tag name format
     * @param tag_name Tag to validate
     * @return true if valid, false otherwise
     * 
     * Valid format: alphanumeric, hyphens, underscores, periods
     * Length: 1-128 characters
     */
    static bool isValidTagName(const std::string& tag_name);

    // ---- Phase 7: GC & Retention Policy ----

    /**
     * @brief Retention policy for automatic snapshot pruning.
     */
    struct RetentionPolicy {
        size_t  max_snapshots{0};          ///< 0 = unlimited
        int64_t max_age_ms{0};             ///< 0 = unlimited; prune older than this
        bool    protect_latest{true};      ///< Never prune the newest snapshot
    };

    /**
     * @brief Set the retention policy applied during pruneOldSnapshots().
     */
    void setRetentionPolicy(const RetentionPolicy& policy);

    /**
     * @brief Prune snapshots that exceed the current retention policy.
     *
     * Removes the oldest snapshots until max_snapshots and max_age_ms
     * constraints are satisfied. The newest snapshot is never deleted when
     * protect_latest is true.
     *
     * @return Number of snapshots deleted.
     */
    size_t pruneOldSnapshots();

    /**
     * @brief Consistency check: verify all stored snapshots are readable.
     * @return Number of corrupted/unreadable snapshots found (0 = healthy).
     */
    size_t checkConsistency() const;

    // ---- Phase 7: Snapshot Restore ----

    /**
     * @brief Result of a restore operation.
     */
    struct RestoreResult {
        bool     success{false};
        std::string tag_name;
        uint64_t target_sequence{0};   ///< Changefeed sequence of the tag
        int64_t  timestamp_ms{0};      ///< Unix timestamp of the tag
        std::string message;           ///< Human-readable status or error

        json toJson() const;
    };

    /**
     * @brief Restore the database view to the state captured by @p tag_name.
     *
     * This method validates that the tag exists and returns its sequence
     * number so callers can reset their changefeed position or RocksDB
     * iterator to replay / re-read data as-of that point in time.
     *
     * Full block-level restore (WAL replay) is a Phase 8 concern; this
     * method covers the Phase 7 "Snapshot-Restore" requirement by:
     *   1. Verifying the tag is present and readable.
     *   2. Returning the exact changefeed sequence to restore to.
     *   3. Creating a "restore-point" tag so the restore is auditable.
     *
     * @param tag_name  Named snapshot / tag to restore to.
     * @param created_by  Optional actor name for audit trail.
     * @return RestoreResult with target sequence and audit info.
     */
    RestoreResult restoreToTag(const std::string& tag_name,
                               const std::string& created_by = "system");

private:
    RocksDBWrapper& db_;
    Changefeed& changefeed_;
    
    mutable std::mutex mutex_;
    RetentionPolicy retention_policy_;

    // Key prefix for snapshot storage in RocksDB
    static constexpr const char* SNAPSHOT_PREFIX = "snapshot:";
    
    /**
     * @brief Make RocksDB key for a tag
     */
    std::string makeKey(const std::string& tag_name) const;
    
    /**
     * @brief Extract tag name from RocksDB key
     */
    std::string extractTagName(const std::string& key) const;
    
    /**
     * @brief Serialize snapshot to bytes
     */
    std::vector<uint8_t> serialize(const Snapshot& snapshot) const;
    
    /**
     * @brief Deserialize snapshot from bytes
     */
    std::optional<Snapshot> deserialize(const std::vector<uint8_t>& data) const;
};

} // namespace transaction
} // namespace themis
