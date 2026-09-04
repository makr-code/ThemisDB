/**
 * @file metadata_snapshot.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License
// Phase 2.2: Metadata Shard Durability

#pragma once

#include "sharding/wal_manager.h"
#include "sharding/metadata_shard.h"
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

using LSN = themis::sharding::LSN;

/** @brief Immutable metadata snapshot payload used for fast shard recovery. */
struct MetadataSnapshot {
    /** @brief Monotonic snapshot identifier (millisecond timestamp based). */
    uint64_t snapshot_id;              // Unique snapshot ID (timestamp)
    /** @brief Highest WAL LSN fully reflected in this snapshot. */
    LSN last_applied_lsn;              // Last LSN applied in this snapshot
    /** @brief Owning metadata shard identifier. */
    std::string shard_id;              // Shard ID
    /** @brief Snapshot creation timestamp in milliseconds since epoch. */
    uint64_t timestamp;                // Snapshot creation timestamp
    
    // Metadata storage by partition
    std::map<MetadataPartitionKey, std::map<std::string, nlohmann::json>> partitions;
    
    // Metadata for verification
    /** @brief SHA-256 checksum over serialized snapshot payload. */
    std::string checksum;              // SHA-256 checksum
    /** @brief Total logical metadata entries captured across partitions. */
    size_t total_entries;              // Total number of entries
    
    /** @brief Serialize snapshot to JSON object (excluding checksum injection policy). */
    nlohmann::json toJson() const {
        nlohmann::json j = {
            {"snapshot_id", snapshot_id},
            {"last_applied_lsn", {
                {"segment", last_applied_lsn.segment},
                {"offset", last_applied_lsn.offset}
            }},
            {"shard_id", shard_id},
            {"timestamp", timestamp},
            {"total_entries", total_entries}
        };
        
        // Serialize partitions
        nlohmann::json partitions_json;
        for (const auto& [partition_key, entries] : partitions) {
            std::string partition_name = std::to_string(static_cast<int>(partition_key));
            nlohmann::json partition_entries;
            for (const auto& [key, value] : entries) {
                partition_entries[key] = value;
            }
            partitions_json[partition_name] = partition_entries;
        }
        j["partitions"] = partitions_json;
        
        return j;
    }
    
    /** @brief Deserialize snapshot payload from JSON representation. */
    static MetadataSnapshot fromJson(const nlohmann::json& j) {
        MetadataSnapshot snapshot;
        snapshot.snapshot_id = j["snapshot_id"];
        snapshot.last_applied_lsn = LSN(
            j["last_applied_lsn"]["segment"],
            j["last_applied_lsn"]["offset"]
        );
        snapshot.shard_id = j["shard_id"];
        snapshot.timestamp = j["timestamp"];
        snapshot.total_entries = j["total_entries"];
        
        // Deserialize partitions
        if (j.contains("partitions")) {
            for (auto& [partition_name, partition_entries] : j["partitions"].items()) {
                MetadataPartitionKey partition_key = 
                    static_cast<MetadataPartitionKey>(std::stoi(partition_name));
                
                std::map<std::string, nlohmann::json> entries = {};

                for (auto& [key, value] : partition_entries.items()) {
                    entries[key] = value;
                }
                snapshot.partitions[partition_key] = entries;
            }
        }
        
        return snapshot;
    }
    
    /** @brief Compute checksum over current snapshot payload content. */
    std::string calculateChecksum() const;
    
    /** @brief Verify stored checksum against recalculated payload checksum. */
    bool verifyChecksum() const {
        return calculateChecksum() == checksum;
    }
};

/**
 * @brief Metadata Snapshot Manager
 * 
 * Manages creation and loading of metadata snapshots for fast recovery.
 */
class MetadataSnapshotManager {
public:
    /**
     * @brief Construct snapshot manager over one snapshot directory.
     * @param snapshot_directory Filesystem directory storing snapshot JSON files.
     * @param max_snapshots Maximum number of snapshots retained after cleanup.
     */
    MetadataSnapshotManager(
        const std::string& snapshot_directory,
        size_t max_snapshots = 10
    );
    
    /**
     * @brief Create persisted snapshot from in-memory metadata storage.
     * @param shard_id Owning shard identifier.
     * @param last_lsn Highest applied WAL LSN represented by storage.
     * @param storage Metadata storage grouped by partition and key.
     * @return Snapshot ID on success, std::nullopt on failure.
     */
    std::optional<uint64_t> createSnapshot(
        const std::string& shard_id,
        const LSN& last_lsn,
        const std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>>& storage
    );
    
    /** @brief Load newest available snapshot from disk. */
    std::optional<MetadataSnapshot> loadLatestSnapshot();
    
    /** @brief Load specific snapshot by id and verify checksum. */
    std::optional<MetadataSnapshot> loadSnapshot(uint64_t snapshot_id);
    
    /**
     * @brief List all available snapshots
     * @return Vector of snapshot IDs (sorted, newest first)
     */
    std::vector<uint64_t> listSnapshots() const;
    
    /** @brief Delete oldest snapshots when retention exceeds max_snapshots. */
    void cleanupOldSnapshots();
    
    /** @brief Delete one snapshot file by id. */
    bool deleteSnapshot(uint64_t snapshot_id);
    
private:
    /** @brief Build full filesystem path for snapshot id. */
    std::string getSnapshotPath(uint64_t snapshot_id) const;
    
    std::string snapshot_directory_;
    size_t max_snapshots_;
    std::mutex snapshot_mutex_;
};

} // namespace sharding
} // namespace themisdb

