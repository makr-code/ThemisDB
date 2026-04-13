/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            metadata_wal.h                                     ║
  Version:         0.0.38                                             ║
  Last Modified:   2026-04-13 04:20:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     217                                            ║
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

#ifndef THEMISDB_SHARDING_METADATA_WAL_H
#define THEMISDB_SHARDING_METADATA_WAL_H

#include "sharding/wal_manager.h"
#include "sharding/metadata_shard.h"
#include <string>
#include <memory>
#include <optional>
#include <vector>
#include <mutex>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

using LSN = themis::sharding::LSN;
using WALEntry = themis::sharding::WALEntry;
using WALEntryType = themis::sharding::WALEntryType;

/**
 * @brief Metadata WAL entry types
 */
enum class MetadataWALEntryType {
    PUT = 120,     // Put metadata entry
    DELETE = 121,  // Delete metadata entry
    UPDATE = 122   // Update existing entry
};

/**
 * @brief Metadata WAL entry
 */
struct MetadataWALEntry {
    LSN lsn;
    MetadataWALEntryType type;
    uint64_t timestamp;
    MetadataPartitionKey partition;
    std::string key;
    nlohmann::json value;
    uint64_t version;
    
    /**
     * @brief Convert to WALEntry for storage
     */
    WALEntry toWALEntry() const {
        WALEntry entry;
        entry.type = static_cast<WALEntryType>(type);
        entry.timestamp = timestamp;
        entry.data = {
            {"partition", static_cast<int>(partition)},
            {"key", key},
            {"value", value},
            {"version", version}
        };
        return entry;
    }
    
    /**
     * @brief Create from WALEntry
     */
    static MetadataWALEntry fromWALEntry(const WALEntry& entry) {
        MetadataWALEntry metadata_entry;
        metadata_entry.lsn = entry.lsn;
        metadata_entry.type = static_cast<MetadataWALEntryType>(entry.type);
        metadata_entry.timestamp = entry.timestamp;
        metadata_entry.partition = static_cast<MetadataPartitionKey>(
            entry.data["partition"].get<int>());
        metadata_entry.key = entry.data["key"].get<std::string>();
        metadata_entry.value = entry.data["value"];
        metadata_entry.version = entry.data["version"].get<uint64_t>();
        return metadata_entry;
    }
};

/**
 * @brief Metadata WAL configuration
 */
struct MetadataWALConfig {
    std::string wal_directory;
    std::string snapshot_directory;
    
    // WAL settings
    size_t segment_size = 16 * 1024 * 1024;  // 16 MB
    size_t write_buffer_size = 64 * 1024;    // 64 KB
    bool sync_on_write = true;
    
    // Snapshot settings
    uint64_t snapshot_interval = 10000;  // Snapshot every 10K operations
    size_t max_snapshots = 10;           // Keep last 10 snapshots
};

/**
 * @brief Metadata Write-Ahead Log
 * 
 * Provides durable logging for metadata operations to enable crash recovery.
 * Similar to PaxosWAL but optimized for metadata workloads.
 */
class MetadataWAL {
public:
    explicit MetadataWAL(const MetadataWALConfig& config);
    ~MetadataWAL();
    
    /**
     * @brief Initialize the WAL
     * @return true if successful
     */
    bool initialize();
    
    /**
     * @brief Log a PUT operation
     * @param partition Metadata partition
     * @param key Metadata key
     * @param value Metadata value
     * @param version Version number
     * @return LSN of the log entry
     */
    LSN logPut(
        MetadataPartitionKey partition,
        const std::string& key,
        const nlohmann::json& value,
        uint64_t version
    );
    
    /**
     * @brief Log a DELETE operation
     * @param partition Metadata partition
     * @param key Metadata key
     * @param version Version number
     * @return LSN of the log entry
     */
    LSN logDelete(
        MetadataPartitionKey partition,
        const std::string& key,
        uint64_t version
    );
    
    /**
     * @brief Log an UPDATE operation
     * @param partition Metadata partition
     * @param key Metadata key
     * @param value New metadata value
     * @param version Version number
     * @return LSN of the log entry
     */
    LSN logUpdate(
        MetadataPartitionKey partition,
        const std::string& key,
        const nlohmann::json& value,
        uint64_t version
    );
    
    /**
     * @brief Read WAL entries starting from a given LSN
     * @param start_lsn Starting LSN (inclusive)
     * @return Vector of metadata WAL entries
     */
    std::vector<MetadataWALEntry> readEntries(const LSN& start_lsn);
    
    /**
     * @brief Flush WAL to disk
     */
    void flush();
    
    /**
     * @brief Check if snapshot should be created
     * @param operations_count Number of operations since last snapshot
     * @return true if snapshot should be created
     */
    bool shouldCreateSnapshot(uint64_t operations_count) const {
        return operations_count >= config_.snapshot_interval;
    }
    
    /**
     * @brief Get WAL configuration
     */
    const MetadataWALConfig& getConfig() const { return config_; }
    
private:
    /**
     * @brief Write a metadata entry to WAL
     */
    LSN writeEntry(const MetadataWALEntry& entry);
    
    MetadataWALConfig config_;
    std::unique_ptr<themis::sharding::WALManager> wal_manager_;
    std::mutex wal_mutex_;
};

} // namespace sharding
} // namespace themisdb

#endif // THEMISDB_SHARDING_METADATA_WAL_H
