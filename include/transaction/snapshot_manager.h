/**
 * @file snapshot_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class SnapshotManager {
public:
    struct Snapshot {
        std::string tag_name;           // Unique tag identifier
        uint64_t sequence_number;       // Changefeed sequence at tag creation
        int64_t timestamp_ms;           // Unix timestamp in milliseconds
        std::string description;        // Human-readable description
        std::string created_by;         // User/service that created the tag
        
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
        static Snapshot fromJson(const json& j);
    };
    
    struct SnapshotStats {
        size_t total_snapshots = 0;
        int64_t oldest_timestamp_ms = 0;
        int64_t newest_timestamp_ms = 0;
        uint64_t oldest_sequence = 0;
        uint64_t newest_sequence = 0;
        
        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };
    
    /**
     * @brief Snapshot Manager.
     * @param[in,out] db Input/output parameter.
     * @param[in,out] changefeed Input/output parameter.
     * @return Return value.
     */
    explicit SnapshotManager(RocksDBWrapper& db, Changefeed& changefeed);
    
    ~SnapshotManager() = default;

    // Disable copy, allow move
    SnapshotManager(const SnapshotManager&) = delete;
    SnapshotManager& operator=(const SnapshotManager&) = delete;
    SnapshotManager(SnapshotManager&&) noexcept = default;
    SnapshotManager& operator=(SnapshotManager&&) noexcept = default;

    std::optional<Snapshot> createTag(
        const std::string& tag_name,
        const std::string& description,
        const std::string& created_by = "system"
    );
    
    /**
     * @brief Get Tag.
     * @param[in] tag_name Name of the tag.
     * @return Return value.
     */
    std::optional<Snapshot> getTag(const std::string& tag_name) const;
    
    std::vector<Snapshot> listTags(
        size_t limit = 0,
        const std::string& sort_by = "timestamp",
        bool ascending = false
    ) const;
    
    /**
     * @brief Delete Tag.
     * @param[in] tag_name Name of the tag.
     * @return True when the operation succeeds.
     */
    bool deleteTag(const std::string& tag_name);
    
    /**
     * @brief Tag Exists.
     * @param[in] tag_name Name of the tag.
     * @return True when the operation succeeds.
     */
    bool tagExists(const std::string& tag_name) const;
    
    /**
     * @brief Get Stats.
     * @return Return value.
     */
    SnapshotStats getStats() const;
    
    /**
     * @brief Get Sequence For Tag.
     * @param[in] tag_name Name of the tag.
     * @return Return value.
     */
    std::optional<uint64_t> getSequenceForTag(const std::string& tag_name) const;
    
    /**
     * @brief Get Timestamp For Tag.
     * @param[in] tag_name Name of the tag.
     * @return Return value.
     */
    std::optional<int64_t> getTimestampForTag(const std::string& tag_name) const;
    
    /**
     * @brief Is Valid Tag Name.
     * @param[in] tag_name Name of the tag.
     * @return True when the operation succeeds.
     */
    static bool isValidTagName(const std::string& tag_name);

    // ---- Phase 7: GC & Retention Policy ----

    struct RetentionPolicy {
        size_t  max_snapshots{0};          ///< 0 = unlimited
        int64_t max_age_ms{0};             ///< 0 = unlimited; prune older than this
        bool    protect_latest{true};      ///< Never prune the newest snapshot
    };

    /**
     * @brief Set Retention Policy.
     * @param[in] policy Input parameter.
     */
    void setRetentionPolicy(const RetentionPolicy& policy);

    /**
     * @brief Prune Old Snapshots.
     * @return Return value.
     */
    size_t pruneOldSnapshots();

    /**
     * @brief Check Consistency.
     * @return Return value.
     */
    size_t checkConsistency() const;

    // ---- Phase 7: Snapshot Restore ----

    struct RestoreResult {
        bool     success{false};
        std::string tag_name = {};
        uint64_t target_sequence{0};   ///< Changefeed sequence of the tag
        int64_t  timestamp_ms{0};      ///< Unix timestamp of the tag
        std::string message;           ///< Human-readable status or error

        /**
         * @brief To Json.
         * @return Return value.
         */
        json toJson() const;
    };

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
     * @brief Make Key.
     * @param[in] tag_name Name of the tag.
     * @return Return value.
     */
    std::string makeKey(const std::string& tag_name) const;
    
    /**
     * @brief Extract Tag Name.
     * @param[in] key Input parameter.
     * @return Return value.
     */
    std::string extractTagName(const std::string& key) const;
    
    /**
     * @brief Serialize.
     * @param[in] snapshot Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> serialize(const Snapshot& snapshot) const;
    
    /**
     * @brief Deserialize.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    std::optional<Snapshot> deserialize(const std::vector<uint8_t>& data) const;
};

} // namespace transaction
} // namespace themis
