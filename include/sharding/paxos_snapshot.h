/**
 * @file paxos_snapshot.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2026 ThemisDB
// Licensed under MIT License

#pragma once

#include "sharding/wal_manager.h"
#include "sharding/consensus_module.h"
#include <string>
#include <map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themisdb {
namespace sharding {
struct PaxosInstance;
struct ConsensusLogEntry;
}
}

namespace themis {
namespace sharding {

using PaxosInstance = themisdb::sharding::PaxosInstance;
using ConsensusLogEntry = themisdb::sharding::ConsensusLogEntry;

/**
 * Paxos Snapshot (Phase 2.1)
 * 
 * Represents a point-in-time snapshot of Paxos consensus state.
 * Used for fast recovery without replaying entire WAL.
 */
struct PaxosSnapshot {
    // Snapshot metadata
    uint64_t snapshot_id;           // Unique snapshot ID (timestamp)
    LSN last_applied_lsn;           // Last WAL entry included in this snapshot
    uint64_t last_committed_slot;   // Last committed Paxos slot
    uint64_t current_round;         // Current Paxos round
    std::string node_id;            // Node that created snapshot
    uint64_t timestamp;             // Creation timestamp (ms since epoch)
    
    // Paxos state
    std::map<uint64_t, nlohmann::json> instances;  // Active Paxos instances (slot -> instance)
    std::map<uint64_t, nlohmann::json> committed_log;  // Committed log entries (index -> entry)
    
    // Checksum for integrity
    std::string checksum;
    
    /**
     * Serialize snapshot to JSON
     * @brief TBD: Describe toJSON.
     * @return Return value.
     */
    nlohmann::json toJSON() const;
    
    /**
     * Deserialize snapshot from JSON
     * @brief TBD: Describe fromJSON.
     * @param[in] json Input parameter.
     * @return Return value.
     */
    static PaxosSnapshot fromJSON(const nlohmann::json& json);
    
    /**
     * Calculate checksum of snapshot data
     * @brief TBD: Describe calculateChecksum.
     * @return Return value.
     */
    std::string calculateChecksum() const;
    
    /**
     * Verify checksum
     * @brief TBD: Describe verifyChecksum.
     * @return True on success.
     */
    bool verifyChecksum() const;

    /**
     * @brief Compress the JSON-serialised snapshot with ZSTD (level 3)
     *
     * Returns the compressed bytes.  The caller is responsible for storing
     * the result alongside the snapshot metadata so that it can be
     * decompressed on load.
     *
     * @param level  ZSTD compression level (default 3)
     * @return Compressed bytes, or empty on failure
     */
    std::vector<uint8_t> compress(int level = 3) const;

    /**
     * @brief Decompress a previously compressed snapshot and reconstruct it
     *
     * @param compressed  Bytes produced by compress()
     * @return Reconstructed snapshot on success, nullopt on failure
     */
    static std::optional<PaxosSnapshot> decompress(
        const std::vector<uint8_t>& compressed);
};

/**
 * Paxos Snapshot Manager (Phase 2.1)
 * 
 * Manages creation, storage, and restoration of Paxos snapshots.
 * 
 * Features:
 * - Periodic snapshot creation
 * - ZSTD snapshot compression (compression_level 3, target >3× ratio)
 * - Snapshot transfer to new replicas
 * - Automatic old snapshot cleanup
 */
class PaxosSnapshotManager {
public:
    explicit PaxosSnapshotManager(const std::string& snapshot_directory,
                                   size_t max_snapshots = 10,
                                   int compression_level = 3);
    ~PaxosSnapshotManager() = default;
    
    /**
     * Create a new snapshot from current Paxos state
     * @param node_id Node creating the snapshot
     * @param last_applied_lsn Last WAL LSN applied
     * @param last_committed_slot Last committed slot
     * @param current_round Current Paxos round
     * @param instances Active Paxos instances
     * @param committed_log Committed log entries
     * @return Snapshot ID on success, nullopt on failure
     */
    std::optional<uint64_t> createSnapshot(
        const std::string& node_id,
        const LSN& last_applied_lsn,
        uint64_t last_committed_slot,
        uint64_t current_round,
        const std::map<uint64_t, PaxosInstance>& instances,
        const std::map<uint64_t, ConsensusLogEntry>& committed_log
    );
    
    /**
     * Load the most recent snapshot
     * @return Snapshot on success, nullopt if no snapshot exists
     * @brief TBD: Describe loadLatestSnapshot.
     */
    std::optional<PaxosSnapshot> loadLatestSnapshot();
    
    /**
     * Load a specific snapshot by ID
     * @param snapshot_id Snapshot ID to load
     * @return Snapshot on success, nullopt if not found
     * @brief TBD: Describe loadSnapshot.
     */
    std::optional<PaxosSnapshot> loadSnapshot(uint64_t snapshot_id);
    
    /**
     * List all available snapshots
     * @return Vector of snapshot IDs (sorted newest first)
     * @brief TBD: Describe listSnapshots.
     */
    std::vector<uint64_t> listSnapshots() const;
    
    /**
     * Delete old snapshots, keeping only the most recent N
     * @param keep_count Number of snapshots to keep
     * @brief TBD: Describe cleanupOldSnapshots.
     */
    void cleanupOldSnapshots(size_t keep_count);
    
    /**
     * Get snapshot file path for a given ID
     * @param snapshot_id Snapshot ID
     * @return Full file path
     * @brief TBD: Describe getSnapshotPath.
     */
    std::string getSnapshotPath(uint64_t snapshot_id) const;
    
private:
    std::string snapshot_directory_;
    size_t max_snapshots_;
    int compression_level_;
    mutable std::mutex mutex_;
    mutable uint64_t last_snapshot_id_{0};
    
    /**
     * @brief Generate unique snapshot ID (timestamp-based)
     * @return Return value.
     */
    uint64_t generateSnapshotId() const;
};

} // namespace sharding
} // namespace themis
