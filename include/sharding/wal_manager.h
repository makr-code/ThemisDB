/**
 * @file wal_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: wal_manager.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    
    /** @brief Parse LSN from segment/offset text representation. */
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
    
    /** @brief Serialize WAL entry into binary wire/storage format. */
    std::vector<uint8_t> serialize() const;
    
    /** @brief Deserialize WAL entry from binary bytes. */
    static WALEntry deserialize(const std::vector<uint8_t>& bytes);
    
    /** @brief Return serialized entry size in bytes. */
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
    /** @brief Construct WAL manager with configured directory/segment policy. */
    explicit WALManager(const WALManagerConfig& config);

    /** @brief Destructor flushes buffered writes and closes segment file. */
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
    
    /** @brief Return current LSN (position of next append). */
    LSN getCurrentLSN() const;
    
    /** @brief Return oldest retained LSN after segment retention cleanup. */
    LSN getOldestLSN() const;
    
    /** @brief Flush buffered WAL bytes to active segment stream. */
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
    
    /** @brief Return WAL statistics snapshot. */
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
    
    /** @brief Open or create WAL segment file by segment number. */
    void openSegment(uint64_t segment_number);
    
    /** @brief Close active WAL segment file when open. */
    void closeSegment();
    
    /** @brief Rotate active segment and apply retention cleanup. */
    void rotateSegment();
    
    /** @brief Build filesystem path for a WAL segment number. */
    std::string getSegmentPath(uint64_t segment_number) const;
    
    /** @brief Discover existing segments and recover current/oldest LSN. */
    void loadExistingSegments();
    
    /** @brief Delete old segments beyond max_segments retention limit. */
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
