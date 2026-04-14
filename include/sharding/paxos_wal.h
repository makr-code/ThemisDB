/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            paxos_wal.h                                        ║
  Version:         0.0.39                                             ║
  Last Modified:   2026-04-13 20:26:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     263                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 36430300eb  2026-03-13  fix(sharding): address all snapshot compaction PR review ... ║
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
#include <memory>
#include <optional>
#include <vector>

namespace themis {
namespace sharding {

using ConsensusLogEntry = themisdb::sharding::ConsensusLogEntry;

/**
 * Paxos WAL Entry Types (Phase 2.1)
 * 
 * Specific entry types for Paxos consensus operations
 */
enum class PaxosWALEntryType : uint8_t {
    PREPARE = 10,      // Phase 1a: Prepare request
    PROMISE = 11,      // Phase 1b: Promise response
    ACCEPT = 12,       // Phase 2a: Accept request
    ACCEPTED = 13,     // Phase 2b: Accepted response
    COMMIT = 14,       // Value committed to log
    SNAPSHOT = 15,     // Snapshot marker
    CONFIG_CHANGE = 16 // Configuration change
};

/**
 * Paxos WAL Entry (Phase 2.1)
 * 
 * Represents a single Paxos operation that needs to be durably logged
 */
struct PaxosWALEntry {
    LSN lsn;                          // Log sequence number
    PaxosWALEntryType type;           // Entry type
    uint64_t timestamp;                // Timestamp (ms since epoch)
    uint64_t slot;                     // Paxos slot number
    uint64_t round;                    // Proposal round
    std::string node_id;               // Node that created this entry
    nlohmann::json data;               // Operation data
    
    // Serialize to WALEntry for storage
    WALEntry toWALEntry() const;
    
    // Deserialize from WALEntry
    static PaxosWALEntry fromWALEntry(const WALEntry& entry);
    
    // Get size in bytes
    size_t size() const;
};

/**
 * Paxos WAL Configuration (Phase 2.1)
 */
struct PaxosWALConfig {
    std::string wal_directory = "./wal/paxos";
    size_t segment_size = 16 * 1024 * 1024;  // 16 MB per segment
    size_t max_segments = 100;                // Max segments before compaction
    bool sync_on_write = true;                // fsync after each write
    size_t write_buffer_size = 64 * 1024;     // 64 KB buffer
    
    // Snapshot configuration
    size_t snapshot_interval = 10000;         // Snapshot every 10K operations
    std::string snapshot_directory = "./snapshots/paxos";
    size_t max_snapshots = 10;                // Keep last 10 snapshots
};

/**
 * Paxos Write-Ahead Log Manager (Phase 2.1)
 * 
 * Provides durable storage for Paxos consensus operations.
 * Ensures that Paxos state can be recovered after crashes.
 * 
 * Features:
 * - Atomic logging of Paxos operations
 * - Crash recovery via WAL replay
 * - Periodic snapshots for fast recovery
 * - Log compaction after snapshots
 * 
 * Design based on:
 * - PostgreSQL WAL architecture
 * - Raft log replication
 * - Paxos Made Simple (Lamport)
 */
class PaxosWAL {
public:
    explicit PaxosWAL(const PaxosWALConfig& config);
    ~PaxosWAL();
    
    /**
     * Initialize WAL (create directories, load state)
     * @return true on success
     */
    bool initialize();
    
    /**
     * Log a Paxos operation
     * @param entry Paxos WAL entry to log
     * @return LSN of logged entry
     */
    LSN logEntry(const PaxosWALEntry& entry);
    
    /**
     * Log Paxos PREPARE phase
     * @param slot Paxos slot
     * @param round Proposal round
     * @param node_id Proposer node
     * @return LSN of logged entry
     */
    LSN logPrepare(uint64_t slot, uint64_t round, const std::string& node_id);
    
    /**
     * Log Paxos PROMISE phase
     * @param slot Paxos slot
     * @param round Promised round
     * @param node_id Acceptor node
     * @param accepted_round Previously accepted round (0 if none)
     * @param accepted_value Previously accepted value (empty if none)
     * @return LSN of logged entry
     */
    LSN logPromise(uint64_t slot, uint64_t round, const std::string& node_id,
                   uint64_t accepted_round, const nlohmann::json& accepted_value);
    
    /**
     * Log Paxos ACCEPT phase
     * @param slot Paxos slot
     * @param round Proposal round
     * @param node_id Proposer node
     * @param value Value to accept
     * @return LSN of logged entry
     */
    LSN logAccept(uint64_t slot, uint64_t round, const std::string& node_id,
                  const ConsensusLogEntry& value);
    
    /**
     * Log Paxos ACCEPTED phase
     * @param slot Paxos slot
     * @param round Accepted round
     * @param node_id Acceptor node
     * @return LSN of logged entry
     */
    LSN logAccepted(uint64_t slot, uint64_t round, const std::string& node_id);
    
    /**
     * Log Paxos COMMIT
     * @param slot Paxos slot
     * @param value Committed value
     * @return LSN of logged entry
     */
    LSN logCommit(uint64_t slot, const ConsensusLogEntry& value);
    
    /**
     * Get all entries from a starting LSN
     * @param start_lsn Starting LSN (inclusive)
     * @param end_lsn Ending LSN (exclusive), or nullopt for all
     * @return Vector of Paxos WAL entries
     */
    std::vector<PaxosWALEntry> readEntries(const LSN& start_lsn,
                                           const std::optional<LSN>& end_lsn = std::nullopt);
    
    /**
     * Get current LSN (position of next write)
     */
    LSN getCurrentLSN() const;
    
    /**
     * Get oldest available LSN
     */
    LSN getOldestLSN() const;
    
    /**
     * Force flush buffered entries to disk
     */
    void flush();
    
    /**
     * Check if snapshot should be created
     * @param operations_since_last Number of operations since last snapshot
     * @return true if snapshot should be created
     */
    bool shouldCreateSnapshot(size_t operations_since_last) const;

    /**
     * @brief Compact (truncate) the WAL at the next segment boundary before `up_to_lsn`.
     *
     * Called after a Paxos snapshot has been successfully persisted.  WAL
     * entries are discarded on a segment-granularity basis: all segments whose
     * segment number is strictly less than `up_to_lsn.segment` are deleted by
     * `WALManager::truncate()`.  The oldest retained data is therefore the
     * beginning of the segment that contains `up_to_lsn`, not necessarily the
     * exact record at `up_to_lsn`.  Callers should pass the LSN of the last
     * committed entry in the snapshot so that recovery can always start from a
     * valid segment boundary.
     *
     * The operation first writes a SNAPSHOT marker WAL entry so that a reader
     * replaying from an older checkpoint can detect the compaction boundary.
     *
     * @param up_to_lsn  LSN whose preceding segments will be discarded
     * @param node_id    Node performing the compaction (used in the marker)
     * @return true on success
     */
    bool compact(const LSN& up_to_lsn, const std::string& node_id);

    /**
     * Get snapshot directory path
     */
    std::string getSnapshotDirectory() const { return config_.snapshot_directory; }
    
    /**
     * Get WAL directory path
     */
    std::string getWALDirectory() const { return config_.wal_directory; }
    
    /**
     * Get configuration
     */
    const PaxosWALConfig& getConfig() const { return config_; }
    
private:
    PaxosWALConfig config_;
    std::unique_ptr<WALManager> wal_manager_;
    mutable std::mutex mutex_;
    
    // Helper to create WAL entry with timestamp and LSN
    PaxosWALEntry createEntry(PaxosWALEntryType type, uint64_t slot,
                              uint64_t round, const std::string& node_id,
                              const nlohmann::json& data);
};

} // namespace sharding
} // namespace themis
