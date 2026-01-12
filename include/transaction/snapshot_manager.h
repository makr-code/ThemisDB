#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <cstdint>
#include <memory>
#include <nlohmann/json.hpp>

// Forward declarations
namespace rocksdb {
    class TransactionDB;
    class ColumnFamilyHandle;
}

namespace themis {

class Changefeed;

/**
 * @brief Snapshot Manager for Point-in-Time Recovery
 * 
 * Provides Git-like tagging functionality for database snapshots:
 * - Create named snapshots (tags) at important points in time
 * - List and retrieve snapshot metadata
 * - Delete snapshots when no longer needed
 * 
 * Use cases:
 * - Disaster Recovery: Tag state before critical operations
 * - Compliance: Tag quarterly/yearly snapshots
 * - Testing: Tag known-good states
 * 
 * Implementation:
 * - Tags stored in dedicated RocksDB Column Family "tags"
 * - Each tag maps to a sequence number from Changefeed
 * - Tags are persistent and survive database restarts
 * 
 * Thread-safety:
 * - All public methods are thread-safe
 * - Uses RocksDB transactions for atomic operations
 */
class SnapshotManager {
public:
    /**
     * @brief Snapshot metadata
     */
    struct Snapshot {
        std::string tag_name;           // Unique tag name
        uint64_t sequence_number;       // Changefeed sequence at snapshot time
        int64_t timestamp_ms;           // Wall clock time when snapshot was created
        std::string description;        // Human-readable description
        std::string created_by;         // User who created the snapshot
        
        // Serialization
        nlohmann::json toJson() const;
        static Snapshot fromJson(const nlohmann::json& j);
    };

    /**
     * @brief Status result for operations
     */
    struct Status {
        bool ok = true;
        std::string message;
        
        static Status OK() { return {true, ""}; }
        static Status Error(std::string msg) { return {false, std::move(msg)}; }
    };

    /**
     * @brief Statistics about snapshots
     */
    struct Stats {
        uint64_t total_snapshots;       // Total number of snapshots
        uint64_t total_size_bytes;      // Approximate storage size
        int64_t oldest_timestamp_ms;    // Timestamp of oldest snapshot
        int64_t newest_timestamp_ms;    // Timestamp of newest snapshot
    };

    /**
     * @brief Construct SnapshotManager
     * @param db RocksDB TransactionDB instance (not owned)
     * @param cf Column family handle for "tags" (not owned)
     * @param changefeed Changefeed instance for sequence numbers (not owned)
     */
    explicit SnapshotManager(rocksdb::TransactionDB* db,
                            rocksdb::ColumnFamilyHandle* cf,
                            Changefeed* changefeed);

    ~SnapshotManager() = default;

    // Disable copy, allow move
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;
    SnapshotManager(SnapshotManager&&) noexcept = default;
    SnapshotManager& operator=(SnapshotManager&&) noexcept = default;

    /**
     * @brief Create a new snapshot tag
     * 
     * @param tag_name Unique identifier for the snapshot (lowercase, alphanumeric, hyphens, underscores)
     * @param description Human-readable description (max 500 characters)
     * @param created_by User identifier (optional, defaults to "system")
     * @return Status indicating success or failure
     * 
     * Errors:
     * - ALREADY_EXISTS: Tag name already exists
     * - INVALID_NAME: Tag name contains invalid characters
     * - INVALID_DESCRIPTION: Description too long
     */
    Status createTag(const std::string& tag_name,
                    const std::string& description,
                    const std::string& created_by = "system");

    /**
     * @brief Get snapshot metadata by tag name
     * 
     * @param tag_name Tag identifier
     * @return Snapshot metadata if exists, nullopt otherwise
     */
    std::optional<Snapshot> getTag(const std::string& tag_name) const;

    /**
     * @brief List all snapshots
     * 
     * @param sort_by_time If true, sort by timestamp (newest first), otherwise by name
     * @return Vector of all snapshots
     */
    std::vector<Snapshot> listTags(bool sort_by_time = true) const;

    /**
     * @brief Delete a snapshot tag
     * 
     * @param tag_name Tag to delete
     * @return Status indicating success or failure
     * 
     * Errors:
     * - NOT_FOUND: Tag does not exist
     */
    Status deleteTag(const std::string& tag_name);

    /**
     * @brief Get statistics about snapshots
     * 
     * @return Stats structure
     */
    Stats getStats() const;

    /**
     * @brief Validate tag name format
     * 
     * Tag names must:
     * - Be 1-64 characters long
     * - Contain only: lowercase letters, digits, hyphens, underscores
     * - Start with a letter
     * 
     * @param tag_name Tag name to validate
     * @return true if valid, false otherwise
     */
    static bool isValidTagName(const std::string& tag_name);

    /**
     * @brief Validate description length
     * 
     * @param description Description to validate
     * @return true if valid (0-500 characters), false otherwise
     */
    static bool isValidDescription(const std::string& description);

private:
    rocksdb::TransactionDB* db_;
    rocksdb::ColumnFamilyHandle* cf_;
    Changefeed* changefeed_;

    static constexpr const char* TAG_PREFIX = "tag:";
    static constexpr size_t MAX_TAG_NAME_LENGTH = 64;
    static constexpr size_t MAX_DESCRIPTION_LENGTH = 500;

    std::string makeKey(const std::string& tag_name) const;
    std::string extractTagName(const std::string& key) const;
};

} // namespace themis
