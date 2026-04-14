/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            paxos_snapshot.h                                   ║
  Version:         0.0.40                                             ║
  Last Modified:   2026-04-14 06:56:38                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     196                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 16db53f833  2026-03-12  feat(sharding): implement Raft snapshot compaction and lo... ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
     */
    nlohmann::json toJSON() const;
    
    /**
     * Deserialize snapshot from JSON
     */
    static PaxosSnapshot fromJSON(const nlohmann::json& json);
    
    /**
     * Calculate checksum of snapshot data
     */
    std::string calculateChecksum() const;
    
    /**
     * Verify checksum
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
     */
    std::optional<PaxosSnapshot> loadLatestSnapshot();
    
    /**
     * Load a specific snapshot by ID
     * @param snapshot_id Snapshot ID to load
     * @return Snapshot on success, nullopt if not found
     */
    std::optional<PaxosSnapshot> loadSnapshot(uint64_t snapshot_id);
    
    /**
     * List all available snapshots
     * @return Vector of snapshot IDs (sorted newest first)
     */
    std::vector<uint64_t> listSnapshots() const;
    
    /**
     * Delete old snapshots, keeping only the most recent N
     * @param keep_count Number of snapshots to keep
     */
    void cleanupOldSnapshots(size_t keep_count);
    
    /**
     * Get snapshot file path for a given ID
     * @param snapshot_id Snapshot ID
     * @return Full file path
     */
    std::string getSnapshotPath(uint64_t snapshot_id) const;
    
private:
    std::string snapshot_directory_;
    size_t max_snapshots_;
    int compression_level_;
    mutable std::mutex mutex_;
    
    // Generate unique snapshot ID (timestamp-based)
    uint64_t generateSnapshotId() const;
};

} // namespace sharding
} // namespace themis
