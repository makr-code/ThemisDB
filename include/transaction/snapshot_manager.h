#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <memory>
#include <mutex>
#include <chrono>
#include <nlohmann/json.hpp>

// Forward declarations
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

class RocksDBWrapper;
class Changefeed;

/**
 * @brief Named Snapshots Manager for MVCC System
 * 
 * Enables semantic tagging of important database states for:
 * - Disaster recovery (before critical operations)
 * - Compliance (quarterly backups, audit points)
 * - Schema migrations (rollback points)
 * - Testing (repeatable test states)
 * 
 * Tags are stored persistently in RocksDB column family "tags"
 * Key format: "tags:{tag_name}"
 * Value format: JSON with {sequence, timestamp, description, created_by}
 * 
 * Thread-Safety: All methods are thread-safe
 */
class SnapshotManager {
public:
    struct Status {
        bool ok = true;
        std::string message;
        
        static Status OK() { return Status{true, ""}; }
        static Status Error(std::string msg) { return Status{false, std::move(msg)}; }
    };

    struct Snapshot {
        std::string tag_name;           // Unique tag name (e.g., "before_migration_2026_q1")
        uint64_t sequence_number;       // Changefeed sequence at snapshot time
        int64_t timestamp_ms;           // Unix timestamp in milliseconds
        std::string description;        // Human-readable description
        std::string created_by;         // User or system that created the snapshot
        
        // Serialization
        nlohmann::json toJson() const;
        static Snapshot fromJson(const nlohmann::json& j);
    };

    struct SnapshotStats {
        size_t total_tags;              // Total number of tags
        uint64_t oldest_sequence;       // Sequence of oldest snapshot
        uint64_t newest_sequence;       // Sequence of newest snapshot
        int64_t oldest_timestamp_ms;    // Timestamp of oldest snapshot
        int64_t newest_timestamp_ms;    // Timestamp of newest snapshot
    };

    /**
     * @brief Construct SnapshotManager
     * @param db RocksDB wrapper instance
     * @param changefeed Changefeed for sequence tracking
     */
    explicit SnapshotManager(RocksDBWrapper& db, Changefeed& changefeed);
    
    ~SnapshotManager() = default;

    // Prevent copying, allow moving
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;
    SnapshotManager(SnapshotManager&&) noexcept = default;
    SnapshotManager& operator=(SnapshotManager&&) noexcept = default;

    /**
     * @brief Create a named snapshot at current sequence
     * @param tag_name Unique tag name (alphanumeric, hyphens, underscores only)
     * @param description Human-readable description (optional)
     * @param created_by User identifier (optional, defaults to "system")
     * @return Status indicating success or failure
     */
    Status createTag(
        const std::string& tag_name,
        const std::string& description = "",
        const std::string& created_by = "system"
    );

    /**
     * @brief Get a specific snapshot by tag name
     * @param tag_name Tag name to retrieve
     * @return Snapshot if exists, nullopt otherwise
     */
    std::optional<Snapshot> getTag(const std::string& tag_name) const;

    /**
     * @brief List all snapshots
     * @return Vector of all snapshots, sorted by timestamp (newest first)
     */
    std::vector<Snapshot> listTags() const;

    /**
     * @brief Delete a snapshot tag
     * @param tag_name Tag name to delete
     * @return Status indicating success or failure
     */
    Status deleteTag(const std::string& tag_name);

    /**
     * @brief Check if a tag exists
     * @param tag_name Tag name to check
     * @return true if tag exists, false otherwise
     */
    bool tagExists(const std::string& tag_name) const;

    /**
     * @brief Get snapshot statistics
     * @return Stats about all snapshots
     */
    SnapshotStats getStats() const;

    /**
     * @brief Validate tag name format
     * @param tag_name Tag name to validate
     * @return Status indicating if name is valid
     */
    static Status validateTagName(const std::string& tag_name);

    /**
     * @brief Get sequence number for a tag
     * @param tag_name Tag name
     * @return Sequence number if tag exists, nullopt otherwise
     */
    std::optional<uint64_t> getSequenceForTag(const std::string& tag_name) const;

private:
    RocksDBWrapper& db_;
    Changefeed& changefeed_;
    mutable std::mutex mutex_;  // For thread-safe access

    static constexpr const char* KEY_PREFIX = "tags:";
    static constexpr size_t MAX_TAG_NAME_LENGTH = 128;
    static constexpr size_t MAX_DESCRIPTION_LENGTH = 1024;

    std::string makeKey(const std::string& tag_name) const;
    std::string extractTagName(const std::string& key) const;
    
    // Helper to get current timestamp
    static int64_t getCurrentTimestampMs();
};

} // namespace themis
