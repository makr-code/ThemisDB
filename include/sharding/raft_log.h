/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            raft_log.h                                         ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     204                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

// Copyright 2025 ThemisDB
// Licensed under MIT License

#ifndef THEMISDB_SHARDING_RAFT_LOG_H
#define THEMISDB_SHARDING_RAFT_LOG_H

#include <cstdint>
#include <map>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

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
    uint64_t term;
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
    uint64_t term;              // Leader's term
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
    uint64_t term;          // Current term, for leader to update itself
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
     * @brief Clear the entire log (for testing)
     */
    void clear();
    
private:
    mutable std::mutex mutex_;
    std::map<uint64_t, LogEntry> log_;  // Index -> LogEntry
    uint64_t commit_index_;
};

}  // namespace sharding
}  // namespace themisdb

#endif  // THEMISDB_SHARDING_RAFT_LOG_H
