/**
 * @file metadata_wal.h
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
#include <mutex>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {

using LSN = themis::sharding::LSN;
using WALEntry = themis::sharding::WALEntry;
using WALEntryType = themis::sharding::WALEntryType;

/** @brief Metadata-specific WAL operation types persisted in shared WAL stream. */
enum class MetadataWALEntryType {
    PUT = 120,     // Put metadata entry
    DELETE_OP = 121,  // Delete metadata entry
    UPDATE = 122   // Update existing entry
};

/** @brief Metadata operation record stored/replayed via WAL manager. */
struct MetadataWALEntry {
    /** @brief LSN assigned by underlying WAL manager. */
    LSN lsn;
    /** @brief Metadata operation kind (PUT/DELETE/UPDATE). */
    MetadataWALEntryType type;
    /** @brief Operation timestamp in milliseconds since epoch. */
    uint64_t timestamp;
    /** @brief Metadata partition owning the key. */
    MetadataPartitionKey partition;
    /** @brief Metadata key affected by the operation. */
    std::string key;
    /** @brief JSON payload for PUT/UPDATE (null for delete). */
    nlohmann::json value;
    /** @brief Version carried for conflict-resolution/order checks. */
    uint64_t version;
    
    /** @brief Convert metadata entry into generic WALEntry for persistence. */
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
    
    /** @brief Reconstruct metadata entry from generic WAL entry payload. */
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

/** @brief Configuration for metadata WAL persistence and snapshot cadence. */
struct MetadataWALConfig {
    /** @brief Directory storing WAL segment files for metadata operations. */
    std::string wal_directory;
    /** @brief Directory storing metadata snapshot files. */
    std::string snapshot_directory;
    
    // WAL settings
    /** @brief Maximum bytes per WAL segment before rotation. */
    size_t segment_size = 16 * 1024 * 1024;  // 16 MB
    /** @brief Buffered write threshold before flush. */
    size_t write_buffer_size = 64 * 1024;    // 64 KB
    /** @brief Flush stream on each write for stronger durability. */
    bool sync_on_write = true;
    
    // Snapshot settings
    /** @brief Operation count threshold triggering snapshot recommendation. */
    uint64_t snapshot_interval = 10000;  // Snapshot every 10K operations
    /** @brief Maximum number of retained snapshot files. */
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
    /** @brief Construct metadata WAL wrapper over shared WAL manager primitives. */
    explicit MetadataWAL(const MetadataWALConfig& config);
    /** @brief Destructor for metadata WAL wrapper. */
    ~MetadataWAL();
    
    /** @brief Initialize directories and underlying WAL manager instance. */
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
    
    /** @brief Flush pending WAL bytes to underlying manager stream. */
    void flush();
    
    /**
     * @brief Check if snapshot should be created
     * @param operations_count Number of operations since last snapshot
     * @return true if snapshot should be created
     */
    bool shouldCreateSnapshot(uint64_t operations_count) const {
        return operations_count >= config_.snapshot_interval;
    }
    
    /** @brief Return effective metadata WAL configuration. */
    const MetadataWALConfig& getConfig() const { return config_; }
    
private:
    /** @brief Internal helper to append one metadata WAL entry. */
    LSN writeEntry(const MetadataWALEntry& entry);
    
    MetadataWALConfig config_;
    std::unique_ptr<themis::sharding::WALManager> wal_manager_;
    std::mutex wal_mutex_;
};

} // namespace sharding
} // namespace themisdb

