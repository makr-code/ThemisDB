/**
 * @file raft_log.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// Copyright 2025 ThemisDB
// Licensed under MIT License

#pragma once

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>
#include <functional>

namespace themisdb {
namespace sharding {

/**
 * @brief A single entry in the Raft log
 * 
 * Each log entry contains:
 * - term: The term when the entry was created by the leader
 * - index: The position of the entry in the log (1-indexed)
 * - command: The actual command/data to be replicated
 * - timestamp_ns: TrueTime timestamp in nanoseconds since epoch
 */
struct LogEntry {
    uint64_t term = 0;
    uint64_t index;
    std::string command;  // Serialized WAL entry or other command
    uint64_t timestamp_ns;  // TrueTime timestamp for ordering and snapshot isolation
    
    LogEntry() : term(0), index(0), timestamp_ns(0) {}
    LogEntry(uint64_t t, uint64_t i, const std::string& cmd, uint64_t ts = 0)
        : term(t), index(i), command(cmd), timestamp_ns(ts) {}
};

/**
 * @brief AppendEntries RPC request structure
 * 
 * Used for both heartbeats (empty entries) and log replication.
 */
struct AppendEntriesRequest {
    uint64_t term = 0;              // Leader's term
    std::string leader_id;      // So follower can redirect clients
    uint64_t prev_log_index;    // Index of log entry immediately preceding new ones
    uint64_t prev_log_term;     // Term of prevLogIndex entry
    std::vector<LogEntry> entries;  // Log entries to store (empty for heartbeat)
    uint64_t leader_commit;     // Leader's commitIndex
    
    AppendEntriesRequest()
        : term(0), prev_log_index(0), prev_log_term(0), leader_commit(0) {}
};

/**
 * @brief AppendEntries RPC response structure
 */
struct AppendEntriesResponse {
    uint64_t term = 0;          // Current term, for leader to update itself
    bool success;           // True if follower contained entry matching prevLogIndex and prevLogTerm
    uint64_t match_index;   // Highest log index known to match (for leader tracking)
    
    AppendEntriesResponse() : term(0), success(false), match_index(0) {}
    AppendEntriesResponse(uint64_t t, bool s, uint64_t mi)
        : term(t), success(s), match_index(mi) {}
};

/**
 * @brief Manages the replicated log for Raft consensus
 * 
 * The RaftLog maintains:
 * - An ordered sequence of log entries
 * - The commit index (highest index known to be committed)
 * - Thread-safe access to log operations
 * 
 * Properties enforced:
 * - Log Matching: If two logs contain an entry with the same index and term,
 *   then the logs are identical in all entries up through the given index
 * - Leader Completeness: If a log entry is committed in a given term,
 *   that entry will be present in the logs of the leaders for all higher-numbered terms
 */
class RaftLog {
public:
    /** @brief Construct empty Raft log with commit index 0. */
    RaftLog();
    ~RaftLog() = default;
    
    // Prevent copying
    RaftLog(const RaftLog&) = delete;
    RaftLog& operator=(const RaftLog&) = delete;
    
    /**
     * @brief Append a new entry to the log
     * @param entry The log entry to append
     * @return The index at which the entry was appended
     */
    uint64_t append(const LogEntry& entry);
    
    /**
     * @brief Get a single log entry at the specified index
     * @param index The log index (1-indexed)
     * @return The log entry if it exists, std::nullopt otherwise
     */
    std::optional<LogEntry> getEntry(uint64_t index) const;
    
    /**
     * @brief Get a range of log entries
     * @param start_index Start index (inclusive, 1-indexed)
     * @param end_index End index (inclusive)
     * @return Vector of log entries in the range
     */
    std::vector<LogEntry> getEntries(uint64_t start_index, uint64_t end_index) const;
    
    /**
     * @brief Check if the log contains an entry at the given index with the given term
     * @param index The log index to check
     * @param term The expected term
     * @return True if log[index].term == term, false otherwise
     */
    bool hasEntry(uint64_t index, uint64_t term) const;
    
    /**
     * @brief Truncate the log from the given index onward
     * 
     * Used when a follower detects a conflict with the leader's log.
     * All entries from index onward are deleted.
     * 
     * @param index The index from which to truncate (inclusive)
     */
    void truncateFrom(uint64_t index);
    
    /**
     * @brief Set the commit index
     * 
     * The commit index is the highest log entry known to be committed.
     * Committed entries are safe to apply to the state machine.
     * 
     * @param index The new commit index
     */
    void setCommitIndex(uint64_t index);
    
    /**
     * @brief Get the current commit index
     * @return The commit index
     */
    uint64_t getCommitIndex() const;
    
    /**
     * @brief Get the index of the last log entry
     * @return The last log index, or 0 if log is empty
     */
    uint64_t getLastLogIndex() const;
    
    /**
     * @brief Get the term of the last log entry
     * @return The last log term, or 0 if log is empty
     */
    uint64_t getLastLogTerm() const;
    
    /**
     * @brief Get the total number of entries in the log
     * @return Number of log entries
     */
    size_t size() const;

    /**
     * @brief Estimate the current in-memory log size in bytes
     *
     * Approximates each entry's size as the command length plus fixed overhead.
     * Used by RaftSnapshotManager to decide when to compact.
     *
     * @return Estimated size in bytes
     */
    size_t estimatedSizeBytes() const;

    /**
     * @brief Clear the entire log (for testing)
        *
        * Resets commit index and snapshot anchor metadata to zero.
     */
    void clear();

    /**
     * @brief Compact (discard) committed entries up to and including snapshot_index
     *
     * Entries at or below snapshot_index are subsumed by the snapshot and can
     * be safely removed from the in-memory map to reclaim memory.  The snapshot
     * index and term recorded via setSnapshotMeta() serve as the virtual "entry
     * 0" anchor so that `hasEntry(snapshot_index, snapshot_term)` returns true.
     *
     * @param snapshot_index  Highest index covered by the snapshot
     * @param snapshot_term   Term of the entry at snapshot_index
     */
    void compactUpTo(uint64_t snapshot_index, uint64_t snapshot_term);

    /**
     * @brief Record the last snapshot's index and term
     *
     * Called after a snapshot has been persisted so that the log can answer
     * prevLogIndex/prevLogTerm queries even after entries have been discarded.
     *
     * @param index Snapshot index
     * @param term  Snapshot term
     */
    void setSnapshotMeta(uint64_t index, uint64_t term);

    /**
     * @brief Get the last snapshot index
     * @return Snapshot index (0 if no snapshot has been taken)
     */
    uint64_t getSnapshotIndex() const;

    /**
     * @brief Get the last snapshot term
     * @return Snapshot term (0 if no snapshot has been taken)
     */
    uint64_t getSnapshotTerm() const;

private:
    mutable std::timed_mutex mutex_;
    std::map<uint64_t, LogEntry> log_;  // Index -> LogEntry
    uint64_t commit_index_;         ///< Highest committed log index.
    uint64_t snapshot_index_{0};    ///< Index of the last installed snapshot.
    uint64_t snapshot_term_{0};     ///< Term of the last installed snapshot.
};

// ============================================================================
// RaftSnapshotManager
// ============================================================================

/**
 * @brief A ZSTD-compressed Raft snapshot ready for storage or transfer
 *
 * Follows the Raft paper §7: the snapshot records the last included index
 * and term so that the log can be safely truncated up to that point.
 */
struct RaftSnapshot {
    uint64_t snapshot_index = 0;          ///< Last Raft log index covered by this snapshot
    uint64_t snapshot_term;           ///< Term of the entry at snapshot_index
    std::vector<uint8_t> data;        ///< ZSTD-compressed state-machine data
    std::string checksum;             ///< SHA-256 of the *uncompressed* state data
    uint64_t uncompressed_size;       ///< Size of the state data before compression
    uint64_t timestamp;               ///< Creation timestamp (ms since epoch)
};

/**
 * @brief A single chunk of a snapshot suitable for wire transfer
 *
 * Large snapshots are split into fixed-size chunks.  Each chunk carries a
 * SHA-256 checksum of its own payload so that the receiver can detect
 * corruption or a truncated transfer without waiting for the final chunk.
 */
struct RaftSnapshotChunk {
    uint64_t snapshot_index = 0;   ///< Identifies which snapshot this chunk belongs to
    uint64_t chunk_index;      ///< 0-based index of this chunk
    uint64_t total_chunks;     ///< Total number of chunks in the snapshot
    std::vector<uint8_t> data; ///< Chunk payload (compressed snapshot bytes)
    std::string checksum;      ///< SHA-256 of this chunk's data
    bool last_chunk;           ///< True if this is the final chunk
};

/**
 * @brief Manages Raft snapshot lifecycle: creation, persistence, compaction,
 *        and chunked transfer to lagging replicas.
 *
 * Design notes
 * - Snapshots are compressed with ZSTD level 3 (>3× ratio for typical JSON
 *   metadata payloads).
 * - Compaction is triggered automatically when the estimated in-memory log
 *   size exceeds `compaction_threshold_bytes` (default 512 MB).
 * - Snapshot files are named `raft_snapshot_<snapshot_index>.bin`.
 * - At most `max_snapshots` snapshot files are retained on disk; older ones
 *   are deleted after a new snapshot is successfully written.
 * - The chunked-transfer API (`getChunk`) exposes fixed-size pieces of the
 *   on-disk compressed data together with per-chunk SHA-256 checksums so that
 *   the receiver can verify each piece independently (tolerating network
 *   interruptions during lagging-replica catch-up).
 */
class RaftSnapshotManager {
public:
    /// Default chunk size for snapshot transfer (4 MB).
    static constexpr size_t kDefaultChunkSizeBytes = 4 * 1024 * 1024;

    struct Config {
        std::string snapshot_directory = "./snapshots/raft";
        /// Compact the log once its estimated size exceeds this threshold
        size_t compaction_threshold_bytes = 512ULL * 1024 * 1024;  // 512 MB
        int compression_level = 3;      ///< ZSTD compression level
        size_t max_snapshots = 5;       ///< Maximum on-disk snapshots to retain
        size_t chunk_size_bytes = kDefaultChunkSizeBytes;  ///< Per-chunk transfer size
    };

    explicit RaftSnapshotManager(const Config& config);
    ~RaftSnapshotManager() = default;

    // Prevent copying
    RaftSnapshotManager(const RaftSnapshotManager&) = delete;
    RaftSnapshotManager& operator=(const RaftSnapshotManager&) = delete;

    /**
     * @brief Create and persist a snapshot then compact the Raft log
     *
     * @param log           The Raft log to compact
     * @param snapshot_index The last log index to include in the snapshot
     * @param snapshot_term  The term of the entry at snapshot_index
     * @param state_data     Raw (uncompressed) state-machine data to snapshot
     * @return true on success
     */
    bool createAndInstall(RaftLog& log,
                          uint64_t snapshot_index,
                          uint64_t snapshot_term,
                          const std::vector<uint8_t>& state_data);

    /**
     * @brief Load the most recently persisted snapshot from disk
     * @return The snapshot, or std::nullopt if none exists
     */
    std::optional<RaftSnapshot> loadLatestSnapshot() const;

    /**
     * @brief Load a specific snapshot by its index
     * @param snapshot_index The snapshot index to load
     * @return The snapshot, or std::nullopt if not found
     */
    std::optional<RaftSnapshot> loadSnapshot(uint64_t snapshot_index) const;

    /**
     * @brief Decide whether the log is large enough to warrant compaction
     * @param log  The Raft log to evaluate
     * @return true if compaction should be triggered
     */
    bool shouldCompact(const RaftLog& log) const;

    /**
     * @brief Return the total number of transfer chunks for a snapshot
     * @param snapshot_index Snapshot to query
     * @return Chunk count, or 0 if the snapshot does not exist
     */
    size_t getChunkCount(uint64_t snapshot_index) const;

    /**
     * @brief Retrieve a single transfer chunk of a snapshot
     *
     * The chunk contains a slice of the on-disk compressed snapshot data plus
     * a SHA-256 checksum of that slice.  The receiver must verify the checksum
     * before accumulating data; if any chunk fails verification the transfer
     * should be restarted from that chunk index.
     *
     * @param snapshot_index Snapshot to read from
     * @param chunk_index    0-based chunk index
     * @return The chunk, or std::nullopt if the snapshot or chunk is not found
     */
    std::optional<RaftSnapshotChunk> getChunk(uint64_t snapshot_index,
                                               uint64_t chunk_index) const;

    /**
     * @brief List all persisted snapshot indices (newest first)
      * @return Snapshot indices sorted descending by index.
     */
    std::vector<uint64_t> listSnapshots() const;

    /**
     * @brief Get the snapshot directory path
      * @return Filesystem path used for snapshot storage.
     */
    const std::string& getSnapshotDirectory() const { return config_.snapshot_directory; }

private:
    Config config_;
    mutable std::mutex mutex_;

    /** @brief Build file path for a snapshot index. */
    std::string snapshotPath(uint64_t snapshot_index) const;

    /** @brief Remove snapshots beyond configured retention. */
    void cleanupOldSnapshots();

    /** @brief Compute lowercase SHA-256 checksum for input bytes. */
    static std::string computeChecksum(const uint8_t* data, size_t size);
};

}  // namespace sharding
}  // namespace themisdb
