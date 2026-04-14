/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metadata_snapshot.h                                ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:37                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     209                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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

/**
 * @brief Metadata snapshot structure
 */
struct MetadataSnapshot {
    uint64_t snapshot_id;              // Unique snapshot ID (timestamp)
    LSN last_applied_lsn;              // Last LSN applied in this snapshot
    std::string shard_id;              // Shard ID
    uint64_t timestamp;                // Snapshot creation timestamp
    
    // Metadata storage by partition
    std::map<MetadataPartitionKey, std::map<std::string, nlohmann::json>> partitions;
    
    // Metadata for verification
    std::string checksum;              // SHA-256 checksum
    size_t total_entries;              // Total number of entries
    
    /**
     * @brief Serialize to JSON
     */
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
    
    /**
     * @brief Deserialize from JSON
     */
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
                
                std::map<std::string, nlohmann::json> entries;
                for (auto& [key, value] : partition_entries.items()) {
                    entries[key] = value;
                }
                snapshot.partitions[partition_key] = entries;
            }
        }
        
        return snapshot;
    }
    
    /**
     * @brief Calculate checksum for this snapshot
     */
    std::string calculateChecksum() const;
    
    /**
     * @brief Verify checksum
     */
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
     * @brief Constructor
     * @param snapshot_directory Directory to store snapshots
     * @param max_snapshots Maximum number of snapshots to keep
     */
    MetadataSnapshotManager(
        const std::string& snapshot_directory,
        size_t max_snapshots = 10
    );
    
    /**
     * @brief Create a snapshot from metadata storage
     * @param shard_id Shard ID
     * @param last_lsn Last applied LSN
     * @param storage Metadata storage by partition
     * @return Snapshot ID if successful
     */
    std::optional<uint64_t> createSnapshot(
        const std::string& shard_id,
        const LSN& last_lsn,
        const std::map<MetadataPartitionKey, std::map<std::string, MetadataEntry>>& storage
    );
    
    /**
     * @brief Load the latest snapshot
     * @return Snapshot if found
     */
    std::optional<MetadataSnapshot> loadLatestSnapshot();
    
    /**
     * @brief Load a specific snapshot
     * @param snapshot_id Snapshot ID to load
     * @return Snapshot if found
     */
    std::optional<MetadataSnapshot> loadSnapshot(uint64_t snapshot_id);
    
    /**
     * @brief List all available snapshots
     * @return Vector of snapshot IDs (sorted, newest first)
     */
    std::vector<uint64_t> listSnapshots() const;
    
    /**
     * @brief Delete old snapshots beyond max_snapshots limit
     */
    void cleanupOldSnapshots();
    
    /**
     * @brief Delete a specific snapshot
     * @param snapshot_id Snapshot ID to delete
     * @return true if successful
     */
    bool deleteSnapshot(uint64_t snapshot_id);
    
private:
    /**
     * @brief Get snapshot file path
     */
    std::string getSnapshotPath(uint64_t snapshot_id) const;
    
    std::string snapshot_directory_;
    size_t max_snapshots_;
    std::mutex snapshot_mutex_;
};

} // namespace sharding
} // namespace themisdb
