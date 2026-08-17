/**
 * @file hypertable.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <optional>
#include <chrono>
#include <memory>
#include <cstdint>

namespace rocksdb {
    class ColumnFamilyHandle;
}

namespace themis {

class RocksDBWrapper;

/**
 * @brief Hypertable implementation for TimescaleDB-compatible time-series storage
 * 
 * v1.2.0 Feature: Uses RocksDB Column Families for automatic partitioning
 * with TTL support for data retention policies.
 * 
 * Architecture:
 * - 1 Chunk = 1 RocksDB Column Family (CF)
 * - Default: 1 Chunk per Day (configurable)
 * - Automatic TTL cleanup via RocksDB (v1.1.0 feature)
 * - Time-based partitioning for efficient queries
 * 
 * Sources:
 * - Concept inspired by: TimescaleDB Hypertables
 * - Project: TimescaleDB - https://github.com/timescale/timescaledb
 * - License: Timescale License (Apache 2.0 compatible for Community Edition)
 * - Paper: Freedman, A., et al. (2017) "TimescaleDB: An Open-Source Time-Series SQL Database"
 * - ThemisDB Implementation: Uses RocksDB Column Families instead of PostgreSQL partitions,
 *   custom chunk management, and integration with ThemisDB's MVCC transaction system
 */
class Hypertable {
public:
    struct Config {
        std::string table_name;
        std::string time_column = "time";         // Time column name
        int64_t chunk_interval_seconds = 86400;   // 1 day default
        int32_t retention_days = 30;              // 30 days retention
        bool auto_create_chunks = true;           // Auto-create chunks on insert
        bool compress_old_chunks = true;          // ZSTD compress chunks > 7 days
    };
    
    struct ChunkInfo {
        std::string chunk_name;
        int64_t start_time;
        int64_t end_time;
        size_t row_count;
        size_t size_bytes;
        bool is_compressed;
        rocksdb::ColumnFamilyHandle* cf_handle;
    };
    
    /**
     * @brief Create or open a hypertable
     */
    explicit Hypertable(RocksDBWrapper* db, const Config& config);
    ~Hypertable();
    
    // Disable copy, allow move
    Hypertable(const Hypertable&) = delete;
    Hypertable& operator=(const Hypertable&) = delete;
    Hypertable(Hypertable&&) noexcept = default;
    Hypertable& operator=(Hypertable&&) noexcept = default;
    
    /**
     * @brief Insert time-series data point
     * 
     * @param timestamp Unix timestamp (seconds or milliseconds)
     * @param data JSON data to store
     * @return true if successful
     */
    bool insert(int64_t timestamp, const std::string& data);
    
    /**
     * @brief Batch insert for high throughput
     */
    bool insertBatch(const std::vector<std::pair<int64_t, std::string>>& batch);
    
    /**
     * @brief Query time range
     * 
     * @param start_time Start timestamp (inclusive)
     * @param end_time End timestamp (exclusive)
     * @return Vector of (timestamp, data) pairs
     */
    std::vector<std::pair<int64_t, std::string>> query(
        int64_t start_time,
        int64_t end_time
    );
    
    /**
     * @brief Chunk health status and lifecycle events
     */
    enum class ChunkStatus {
        Active,      ///< Within retention window, being written to
        Frozen,      ///< Past current interval but within retention window
        Compressible,///< Older than compress_threshold, not yet compressed
        Compressed,  ///< Already compressed
        Expired      ///< Past retention window, ready to drop
    };

    struct ChunkHealth {
        std::string chunk_name;
        ChunkStatus status = ChunkStatus::Active;
        bool is_healthy = true;
        size_t row_count = 0;
        size_t size_bytes = 0;
        int64_t start_time = 0;
        int64_t end_time = 0;
        std::string status_message;
    };

    /**
     * @brief Get health and lifecycle status for all chunks
     *
     * Returns a health report for each tracked chunk, including
     * status (Active / Frozen / Compressible / Compressed / Expired)
     * and diagnostic messages.
     */
    std::vector<ChunkHealth> getChunkHealth();

    /**
     * @brief Get list of chunks
     */
    std::vector<ChunkInfo> listChunks();
    
    /**
     * @brief Compress old chunks (> 7 days) manually
     * 
     * @return Number of chunks compressed
     */
    uint32_t compressOldChunks();
    
    /**
     * @brief Drop expired chunks based on retention policy
     * 
     * @return Number of chunks dropped
     */
    uint32_t dropExpiredChunks();
    
    /**
     * @brief Get hypertable statistics
     */
    struct Stats {
        size_t total_chunks = 0;
        size_t active_chunks = 0;      // Chunks within retention
        size_t compressed_chunks = 0;
        size_t total_rows = 0;
        size_t total_size_bytes = 0;
        int64_t oldest_timestamp = 0;
        int64_t newest_timestamp = 0;
    };
    
    Stats getStats();
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }

private:
    RocksDBWrapper* db_;  // Not owned
    Config config_;
    
    /**
     * @brief Get chunk name for timestamp
     */
    std::string getChunkName(int64_t timestamp);
    
    /**
     * @brief Get or create chunk for timestamp
     */
    rocksdb::ColumnFamilyHandle* getOrCreateChunk(int64_t timestamp);
    
    /**
     * @brief Parse chunk name to get time range
     */
    std::pair<int64_t, int64_t> parseChunkTimeRange(const std::string& chunk_name);
    
    /**
     * @brief Build key for time-series entry
     */
    std::string buildKey(int64_t timestamp, uint64_t sequence_id);
};

} // namespace themis
