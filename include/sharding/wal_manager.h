/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            wal_manager.h                                      ║
  Version:         0.0.46                                             ║
  Last Modified:   2026-04-15 18:05:33                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     301                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

// Prevent/clean Windows macro pollution that can break enum/identifiers
#if defined(_WIN32)
#ifndef NOMINMAX
#define NOMINMAX 1
#endif
#ifdef LSN
#undef LSN
#endif
#ifdef DELETE
#undef DELETE
#endif
#ifdef INSERT
#undef INSERT
#endif
#ifdef UPDATE
#undef UPDATE
#endif
#ifdef BEGIN
#undef BEGIN
#endif
#ifdef COMMIT
#undef COMMIT
#endif
#ifdef ABORT
#undef ABORT
#endif
#ifdef CHECKPOINT
#undef CHECKPOINT
#endif
#endif

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <atomic>
#include <mutex>
#include <fstream>
#include <optional>
#include <cstdint>
#include <nlohmann/json.hpp>

namespace themis::sharding {

/**
 * Write-Ahead Log (WAL) Manager
 * 
 * Manages write-ahead logs for replica synchronization.
 * Inspired by PostgreSQL's WAL architecture.
 * 
 * Features:
 * - Sequential log writing
 * - Log segment rotation
 * - Position tracking (LSN - Log Sequence Number)
 * - Atomic log entries
 * - Crash recovery support
 */

/**
 * Log Sequence Number (LSN)
 * Uniquely identifies a position in the WAL
 */
struct LSN {
    uint64_t segment;  // WAL segment number
    uint64_t offset;   // Offset within segment
    
    LSN() : segment(0), offset(0) {}
    LSN(uint64_t seg, uint64_t off) : segment(seg), offset(off) {}
    
    bool operator<(const LSN& other) const {
        if (segment != other.segment) return segment < other.segment;
        return offset < other.offset;
    }
    
    bool operator==(const LSN& other) const {
        return segment == other.segment && offset == other.offset;
    }
    
    bool operator<=(const LSN& other) const {
        return *this < other || *this == other;
    }

    bool operator>(const LSN& other) const {
        return other < *this;
    }

    bool operator>=(const LSN& other) const {
        return !(*this < other);
    }
    
    std::string toString() const {
        return std::to_string(segment) + "/" + std::to_string(offset);
    }
    
    static LSN fromString(const std::string& str);
};

/**
 * WAL Entry Types
 */
enum class WALEntryType : uint8_t {
    INSERT = 1,
    UPDATE = 2,
    DELETE = 3,
    BEGIN_TX = 4,
    COMMIT_TX = 5,
    ABORT_TX = 6,
    CHECKPOINT = 7,
    PREPARE_TX = 8   // 2PC PREPARE phase log entry (in-doubt recovery)
};

/**
 * WAL Entry
 * Single atomic operation recorded in the log
 */
struct WALEntry {
    LSN lsn;                    // Log sequence number
    WALEntryType type;          // Entry type
    uint64_t timestamp;         // Timestamp (milliseconds since epoch)
    std::string transaction_id; // Transaction ID (optional)
    nlohmann::json data;        // Operation data (JSON for flexibility)
    
    /**
     * Serialize to binary format
     */
    std::vector<uint8_t> serialize() const;
    
    /**
     * Deserialize from binary format
     */
    static WALEntry deserialize(const std::vector<uint8_t>& bytes);
    
    /**
     * Get size in bytes
     */
    size_t size() const;
};

/**
 * WAL Manager Configuration
 */
struct WALManagerConfig {
    std::string wal_directory = "./wal";
    size_t segment_size = 16 * 1024 * 1024;  // 16 MB per segment
    size_t max_segments = 100;                // Max segments to keep
    bool sync_on_write = true;                // fsync after each write
    size_t write_buffer_size = 64 * 1024;     // 64 KB buffer
};

/**
 * WAL Manager
 * 
 * Thread-safe write-ahead log manager
 */
class WALManager {
public:
    explicit WALManager(const WALManagerConfig& config);
    ~WALManager();
    
    /**
     * Append entry to WAL
     * @param entry Entry to append
     * @return LSN of appended entry
     */
    LSN append(const WALEntry& entry);
    
    /**
     * Read entry at specific LSN
     * @param lsn Log sequence number
     * @return Entry at LSN, or nullopt if not found
     */
    std::optional<WALEntry> read(const LSN& lsn);
    
    /**
     * Read entries from start_lsn (inclusive) to end_lsn (exclusive)
     * @param start_lsn Start LSN (inclusive)
     * @param end_lsn End LSN (exclusive), or nullopt for all remaining
     * @return Vector of entries
     */
    std::vector<WALEntry> readRange(const LSN& start_lsn, 
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
     * Create checkpoint at current position
     * @return LSN of checkpoint
     */
    LSN checkpoint();
    
    /**
     * Truncate WAL before given LSN
     * @param lsn LSN to truncate before
     */
    void truncate(const LSN& lsn);
    
    /**
     * Get WAL statistics
     */
    struct Statistics;
    Statistics getStatistics() const;

private:
    WALManagerConfig config_;
    
    mutable std::mutex mutex_;
    
    // Current write position
    LSN current_lsn_;
    LSN oldest_lsn_;
    
    // Current segment file
    std::unique_ptr<std::fstream> current_segment_;
    
    // Write buffer
    std::vector<uint8_t> write_buffer_;
    
    // Statistics
    std::atomic<uint64_t> total_entries_{0};
    std::atomic<uint64_t> total_bytes_{0};
    
    /**
     * Open or create segment file
     */
    void openSegment(uint64_t segment_number);
    
    /**
     * Close current segment
     */
    void closeSegment();
    
    /**
     * Rotate to new segment
     */
    void rotateSegment();
    
    /**
     * Get segment file path
     */
    std::string getSegmentPath(uint64_t segment_number) const;
    
    /**
     * Load existing segments on startup
     */
    void loadExistingSegments();
    
    /**
     * Cleanup old segments
     */
    void cleanupOldSegments();
};

/**
 * WAL Manager Statistics
 */
struct WALManager::Statistics {
    uint64_t total_entries = 0;
    uint64_t total_bytes = 0;
    uint64_t segments = 0;
    LSN current_lsn;
    LSN oldest_lsn;
};

} // namespace themis::sharding
