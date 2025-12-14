#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <functional>
#include <string>

namespace rocksdb {
    class TransactionDB;
    class Transaction;
    class WriteBatch;
    class Iterator;
    class Options;
    class ReadOptions;
    class WriteOptions;
    class TransactionDBOptions;
    class TransactionOptions;
    class Snapshot;
    class DB;
    class ColumnFamilyHandle;
}

namespace themis {

class BaseEntity;

/// High-level wrapper around RocksDB TransactionDB for MVCC support
/// Manages LSM-Tree configuration, WAL, Transactions, and BlobDB
class RocksDBWrapper {
public:
    struct Config {
        std::string db_path = "./data/rocksdb";
        // Optional separates
        std::string wal_dir; // wenn leer -> Standard unter db_path
        struct DbPath { std::string path; uint64_t target_size_bytes; };
        std::vector<DbPath> db_paths; // für SSTables auf mehreren NVMe-Mounts

        size_t memtable_size_mb = 256;
        size_t block_cache_size_mb = 1024;
        bool cache_index_and_filter_blocks = true;
        bool pin_l0_filter_and_index_blocks_in_cache = true;
        bool partition_filters = true;
        double high_pri_pool_ratio = 0.5; // Anteil für Index/Filter im Cache
        int bloom_bits_per_key = 10;
        bool enable_wal = true;
        bool enable_blobdb = true;
        size_t blob_size_threshold = 4096;  // Files > 4KB go to BlobDB
        int max_background_jobs = 4;
        
        // Compaction
        bool use_universal_compaction = false;
        bool dynamic_level_bytes = true;
        uint64_t target_file_size_base_mb = 64;
        uint64_t max_bytes_for_level_base_mb = 256;

        // Write buffer tuning
        int max_write_buffer_number = 3;
        int min_write_buffer_number_to_merge = 1;

        // I/O
        bool use_direct_reads = false;
        bool use_direct_io_for_flush_and_compaction = false;

        // Compression (best-effort; depends on RocksDB build)
        // Values: "none", "lz4", "zstd", "snappy", "zlib", "bzip2", "lz4hc"
        std::string compression_default = "none";
        std::string compression_bottommost = "none";
        // Legacy compatibility flags used by some tests
        bool wal_enabled = true;         // maps to enable_wal
        bool create_if_missing = true;   // respected in options configuration
        
        // v1.1.0: TTL (Time-To-Live) support
        bool enable_ttl = false;         // Enable TTL for automatic data expiration
        int32_t ttl_seconds = 0;         // TTL in seconds (0 = disabled)
    };
    
    explicit RocksDBWrapper(const Config& config);
    ~RocksDBWrapper();
    
    // Disable copy, allow move
    RocksDBWrapper(const RocksDBWrapper&) = delete;
    RocksDBWrapper& operator=(const RocksDBWrapper&) = delete;
    RocksDBWrapper(RocksDBWrapper&&) noexcept;
    RocksDBWrapper& operator=(RocksDBWrapper&&) noexcept;
    
    /// Open the database
    bool open();
    
    /// Close the database
    void close();
    
    /// Check if database is open
    bool isOpen() const;
    
    // ===== CRUD Operations =====
    
    /// Get value by key
    std::optional<std::vector<uint8_t>> get(std::string_view key);
    
    /// Convenience: Get as string (legacy test compatibility). Returns true if found.
    bool get(std::string_view key, std::string& out);
    
    /// Put key-value pair
    bool put(std::string_view key, const std::vector<uint8_t>& value);
    
    /// Convenience: Put string value (legacy test compatibility)
    bool put(std::string_view key, std::string_view value);
    
    /// Delete key
    bool del(std::string_view key);
    
    /// Multi-get (batch read)
    std::vector<std::optional<std::vector<uint8_t>>> multiGet(
        const std::vector<std::string>& keys
    );
    
    // ===== Atomic Batch Operations =====
    
    /// Create a new write batch for atomic multi-index updates (legacy compatibility)
    class WriteBatchWrapper {
    public:
        explicit WriteBatchWrapper(RocksDBWrapper* db);
        ~WriteBatchWrapper();
        
        void put(std::string_view key, const std::vector<uint8_t>& value);
        void del(std::string_view key);
        
        /// Commit the batch atomically
        bool commit();
        
        /// Rollback (discard) the batch
        void rollback();
        
    private:
        RocksDBWrapper* db_;
        std::unique_ptr<rocksdb::WriteBatch> batch_;
        friend class RocksDBWrapper;
    };
    
    std::unique_ptr<WriteBatchWrapper> createWriteBatch();
    
    // ===== MVCC Transaction Operations =====
    
    /// Create a new MVCC transaction with snapshot isolation
    class TransactionWrapper {
    public:
        explicit TransactionWrapper(RocksDBWrapper* db);
        ~TransactionWrapper();
        
        /// Get value with snapshot isolation
        std::optional<std::vector<uint8_t>> get(std::string_view key);
        
        /// Put key-value pair (visible only after commit)
        bool put(std::string_view key, const std::vector<uint8_t>& value);
        
        /// Delete key (effective only after commit)
        bool del(std::string_view key);
        
        /// Commit the transaction (may fail with conflict)
        bool commit();
        
        /// Rollback the transaction
        void rollback();
        
        /// Check if transaction is still active
        bool isActive() const { return active_; }
        
        /// Get the snapshot (for debugging)
        const rocksdb::Snapshot* getSnapshot() const;
        
    private:
        RocksDBWrapper* db_;
        std::unique_ptr<rocksdb::Transaction> txn_;
        bool active_ = true;
        friend class RocksDBWrapper;
    };
    
    std::unique_ptr<TransactionWrapper> beginTransaction();
    
    // ===== Iteration / Scanning =====
    
    /// Scan with prefix (for index scans)
    using ScanCallback = std::function<bool(std::string_view key, std::string_view value)>;
    void scanPrefix(std::string_view prefix, ScanCallback callback);
    
    /// Scan range [start_key, end_key)
    void scanRange(std::string_view start_key, std::string_view end_key, ScanCallback callback);
    
    /// Full scan (use sparingly!)
    void scanAll(ScanCallback callback);
    
    // ===== Statistics & Maintenance =====
    
    /// Get database statistics
    std::string getStats() const;
    
    /// Get active compression type (runtime query)
    std::string getCompressionType() const;
    
    /// Trigger manual compaction
    void compactRange(std::string_view start_key, std::string_view end_key);
    
    /// Flush memtable to disk
    void flush();
    
    /// Get approximate database size in bytes
    uint64_t getApproximateSize() const;
    
    /// Get current configuration
    const Config& getConfig() const { return config_; }

    // ===== Backup & Recovery (Checkpoints) =====
    /// Create a RocksDB checkpoint (filesystem-level snapshot) at the given directory.
    /// Returns true on success. Directory will be created if it doesn't exist.
    bool createCheckpoint(const std::string& checkpoint_dir);

    /// Restore the database from a previously created checkpoint directory.
    /// This will close the current DB, replace the DB path contents with the checkpoint,
    /// and reopen the DB. Returns true on success.
    bool restoreFromCheckpoint(const std::string& checkpoint_dir);

    // ===== v1.1.0: Advanced RocksDB Features =====
    
    /// Create an incremental backup (only delta since last backup)
    /// @param backup_dir Directory to store backups
    /// @param flush_before_backup Flush memtables before backup
    /// @return true on success
    bool createIncrementalBackup(const std::string& backup_dir, bool flush_before_backup = true);
    
    /// Restore from the latest backup
    /// @param backup_dir Directory containing backups
    /// @return true on success
    bool restoreFromBackup(const std::string& backup_dir);
    
    /// Get number of backups available
    /// @param backup_dir Directory containing backups
    /// @return Number of backups
    uint32_t getBackupCount(const std::string& backup_dir) const;
    
    /// Export RocksDB statistics as JSON (for OpenTelemetry integration)
    /// @return JSON object with statistics
    std::string exportStatisticsJSON() const;
    
    /// Get specific statistic value by ticker type
    /// @param ticker_name Name of the ticker (e.g., "BYTES_WRITTEN", "BYTES_READ")
    /// @return Ticker value
    uint64_t getStatistic(const std::string& ticker_name) const;

    // ===== Column Family Management =====
    
    /// Create or open a column family
    /// @return Column family handle (owned by DB, don't delete)
    rocksdb::ColumnFamilyHandle* getOrCreateColumnFamily(const std::string& cf_name);
    
    /// Get raw RocksDB pointer for advanced operations
    rocksdb::TransactionDB* getRawDB() { return db_.get(); }
    const rocksdb::TransactionDB* getRawDB() const { return db_.get(); }

private:
    Config config_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::unique_ptr<rocksdb::Options> options_;
    std::unique_ptr<rocksdb::TransactionDBOptions> txn_db_options_;
    std::unique_ptr<rocksdb::TransactionOptions> txn_options_;
    std::unique_ptr<rocksdb::ReadOptions> read_options_;
    std::unique_ptr<rocksdb::WriteOptions> write_options_;
    // Track created column family handles so they can be destroyed before DB close
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
    
    void configureOptions();
    bool commitBatch(rocksdb::WriteBatch* batch);
};

} // namespace themis
