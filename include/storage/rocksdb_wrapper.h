/**
 * @file rocksdb_wrapper.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <string_view>
#include <optional>
#include <vector>
#include <functional>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <string>
#include "utils/expected.h"
#include "storage/nvme_manager.h"

// RocksDB is a hard requirement for this wrapper API.
#ifndef THEMIS_ROCKSDB_AVAILABLE
#error "RocksDBWrapper requires THEMIS_ROCKSDB_AVAILABLE with real RocksDB headers/libraries."
#endif

// Note: rocksdb/iterator.h is required for full Iterator definition used by std::unique_ptr.
#include <rocksdb/iterator.h>

namespace rocksdb {
    class TransactionDB;
    class Transaction;
    class WriteBatch;
    class WriteBatchWithIndex;
    struct Options;
    struct ReadOptions;
    struct WriteOptions;
    struct TransactionDBOptions;
    struct TransactionOptions;
    class Snapshot;
    class DB;
    class ColumnFamilyHandle;
    class EventListener;
}

namespace themis {

class BaseEntity;

/// High-level wrapper around RocksDB TransactionDB for MVCC support
/// Manages LSM-Tree configuration, WAL, Transactions, and BlobDB
/// 
/// Thread-safety:
/// - **Read-safe**: Multiple threads can call read operations (get, scan, etc) concurrently
/// - **Write-safe**: Write operations (put, delete) are thread-safe (use internal locking)
/// - **Config-safe**: All RocksDB Options/ReadOptions/WriteOptions objects are guarded by
///   options_mutex_ and are thread-safe for concurrent reads (all member calls use immutable
///   object views passed to RocksDB engine). Configuration changes during construction only.
/// - **NOT move-safe**: Move constructor and assignment should NOT be called during concurrent access
///   - Only safe during initialization/teardown when no other threads are accessing the object
///   - Never move during active operation
///   - Debug mode (THEMIS_DEBUG_THREADING) will detect and log concurrent move operations
/// - **NOT copyable**: Copy operations are deleted
/// - **Iterator-safe**: Iterator operations use reference counting to prevent use-after-free
/// - **close() waits**: close() waits for active operations before shutdown
/// 
/// @code
/// // ✅ OK: Move during initialization
/// std::unique_ptr<RocksDBWrapper> db = std::make_unique<RocksDBWrapper>(config);
/// 
/// // ❌ WRONG: Move while other threads are accessing
/// db = std::make_unique<RocksDBWrapper>(other_config);  // Race condition!
/// @endcode
/// 
/// @warning Move constructor and assignment should only be called when no other
///          threads are accessing the object. Typically done during initialization
///          or teardown, not during active operation.
class RocksDBWrapper {
public:
    struct Config {
        std::string db_path = "./data/rocksdb";
        // Optional separates
        std::string wal_dir; // wenn leer -> Standard unter db_path
        struct DbPath { std::string path; uint64_t target_size_bytes; };
        std::vector<DbPath> db_paths; // für SSTables auf mehreren NVMe-Mounts

        // Read-Only Mode (v1.4.0+)
        // Öffnet Datenbank im Read-Only-Modus - keine Schreiboperationen möglich
        // Verhindert WAL-Updates und Compactions
        // Ideal für: Dokumentations-Datenbanken, Archiv-Daten, Backup-Verification
        bool read_only = false;

        // Write-Amplification Optimization (v1.5.0+):
        // Larger memtables reduce write-amplification by reducing flush frequency
        // 512MB memtable → ~50% fewer L0 files → ~30-40% less write-amp
        // Trade-off: Higher memory usage, longer recovery time
        size_t memtable_size_mb = 512;
        size_t block_cache_size_mb = 1024;
        int block_cache_shard_bits = -1;  // -1 = auto, 4 = 16 shards, 6 = 64 shards (better for 8+ threads)
        bool cache_index_and_filter_blocks = true;
        bool pin_l0_filter_and_index_blocks_in_cache = true;
        bool partition_filters = true;
        double high_pri_pool_ratio = 0.5; // Anteil für Index/Filter im Cache
        int bloom_bits_per_key = 10;
        bool enable_wal = true;
        bool enable_blobdb = true;
        bool enable_statistics = true;          // allow disabling stats in microbenchmarks
        size_t blob_size_threshold = 4096;  // Files > 4KB go to BlobDB
        int max_background_jobs = 4;
        
        // Phase 2H: Granular background thread control for high parallelism
        int max_background_compactions = -1;  // -1 = use max_background_jobs (auto)
        int max_background_flushes = -1;      // -1 = use max_background_jobs (auto)
        int max_subcompactions = 1;           // Parallel sub-compactions per compaction
        int background_threads_high = 2;      // Flush thread pool size
        int background_threads_low = 2;       // Compaction thread pool size
        bool enable_high_parallel_tuning = false;  // Hybrid flag: apply Phase 2H presets automatically
        int high_parallel_thread_threshold = 16;   // Turn on tuning at/above this concurrency
        
        // Compaction
        bool use_universal_compaction = false;
        bool dynamic_level_bytes = true;
        uint64_t target_file_size_base_mb = 64;
        uint64_t max_bytes_for_level_base_mb = 256;

        // Write buffer tuning (Write-Amplification Optimization v1.5.0+)
        // More write buffers allow writes to continue while flushing
        // 6 buffers = optimal for high-throughput OLTP (PERFORMANCE_TIPS.md)
        int max_write_buffer_number = 6;
        int min_write_buffer_number_to_merge = 1;
        // Total write buffer across all CFs: 2GB default for write-heavy workloads
        // Limits total memory used by all memtables (prevents OOM on many CFs)
        size_t db_write_buffer_size_mb = 2048;  // 2GB total (was 0/unlimited)
        
        // Phase 2H: Level0 file control to prevent write stalls
        int level0_file_num_compaction_trigger = 4;  // Start L0->L1 compaction
        int level0_slowdown_writes_trigger = 20;     // Slow down writes
        int level0_stop_writes_trigger = 36;         // Stop writes completely
        
        bool allow_concurrent_memtable_write = true;   // v1.3.0: Allow parallel writes to different memtables
        bool enable_pipelined_write = false;           // v1.3.0: Disabled for TransactionDB - pipelined_writes incompatible with concurrent prepares
        bool allow_unordered_write = false;            // Allow unordered writes (better concurrency)
        bool disable_wal_for_benchmark = false;        // WriteOptions::disableWAL for benchmark mode (NO fsync on writes!)

        // I/O
        bool use_direct_reads = false;
        bool use_direct_io_for_flush_and_compaction = false;

        // v1.6.0 NVMe Optimizations
        // Enable NVMe-specific I/O features via NVMeManager.  When enabled,
        // RocksDBWrapper constructs an NVMeManager, calls initialize(), and
        // applies its recommended Direct I/O flags and background-thread counts.
        // All sub-features (io_uring, ZNS, multi-queue) are controlled via the
        // nvme_enable_* and nvme_* fields below.
        bool enable_nvme_optimizations = false;
        // Block-device path forwarded to NVMeManager for capability detection
        // (e.g. "/dev/nvme0n1").  Leave empty to skip sysfs probing.
        std::string nvme_device_path;
        // io_uring queue depth (entries per ring); ignored when enable_nvme_optimizations=false.
        uint32_t nvme_io_uring_queue_depth = 128;
        // Enable io_uring Linux async I/O (requires THEMIS_ENABLE_IO_URING and Linux ≥ 5.1).
        bool nvme_enable_io_uring = false;
        // Enable ZNS zone-namespace placement (requires a ZNS NVMe device).
        bool nvme_enable_zns = false;

        // v1.3.0 Phase 2: Async I/O with Prefetching (Enhanced v1.5.0)
        // Async I/O improves scan performance by 2-5x through prefetching
        // Enable for workloads with sequential scans, range queries
        bool enable_async_io = true;                     // Enable async I/O by default
        size_t async_io_readahead_size_mb = 128;        // Increased from 64MB for better throughput
        int async_io_multiget_batch_size = 100;         // MultiGet batch size
        int async_io_num_threads = 4;                   // Async I/O thread pool size
        
        // v1.4.1: CPU-level Prefetch Hints for Random Access Performance
        // Software prefetch hints to improve cache hit rates for random access patterns
        // Based on research: Chen, T-F., Baer, J-L. (1995) "Effective Hardware-Based Data Prefetching for 
        // High-Performance Processors", IEEE Transactions on Computers, vol. 44, no. 5, pp. 609-623.
        // DOI: 10.1109/12.381947
        bool enable_cpu_prefetch = true;                // Enable CPU prefetch hints
        size_t prefetch_distance = 2;                   // Items to prefetch ahead (1-8, default: 2)
        size_t prefetch_min_batch_size = 4;             // Minimum batch size to enable prefetch

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

        // TransactionDB write policy (performance tuning)
        enum class WritePolicy {
            WriteCommitted,
            WritePrepared,
            WriteUnprepared
        };
        WritePolicy write_policy = WritePolicy::WriteUnprepared;  // v1.3.0: Use WriteUnprepared for safe Snapshot Isolation + skip_prepare compatibility
        bool two_write_queues = true;           // Enable dual write queues (prepare/commit) - reduces lock contention
        uint64_t wp_commit_cache_bits = 23;     // 2^23 ~= 8M commit cache entries
        
        // Data Integrity & Robustness (v1.4.1+)
        // Based on research: Bairavasundaram et al. (2008), Bonwick et al. (2010)
        // See docs/DATABASE_FILE_ROBUSTNESS.md for details
        bool paranoid_checks = true;                // Verify all data on read (catches corruption early)
        bool verify_checksums_on_read = true;       // Verify block checksums on every read
        bool verify_checksums_in_compaction = true; // Background verification during compaction
        bool force_sync_on_write = false;           // Force fsync on every write (max durability, ~30% overhead)
        bool disable_mmap_reads = true;             // Prevent mmap from hiding I/O errors
        bool disable_mmap_writes = true;            // Prevent mmap write errors

        // WAL periodic background flush (v1.9+)
        // When > 0, RocksDB calls fdatasync on the WAL every wal_bytes_per_sync bytes
        // (in the background write thread), providing a durability window without the
        // per-write fsync overhead of force_sync_on_write.
        // Recommended value: 1 MiB (1048576) for balanced durability/throughput.
        // 0 = disabled (OS decides when to flush; default for backward compat).
        uint64_t wal_bytes_per_sync = 0;
        
        // Checksum algorithm (v1.4.1+)
        enum class ChecksumType {
            CRC32,      // Standard, compatible
            XXH3        // Fastest (3x faster than CRC32, recommended)
        };
        ChecksumType checksum_type = ChecksumType::XXH3;

        // Optional built-in RocksDB MergeOperator presets.
        // Use SequenceU64Increment for CDC Changefeed sequence persistence
        // when components call DB::Merge on an uint64 LE counter key.
        enum class MergeOperatorPreset {
            None,
            SequenceU64Increment
        };
        MergeOperatorPreset merge_operator_preset =
            MergeOperatorPreset::None;

        // v2.0.0: Streaming blob write path (PERF-D5)
        // Blobs >= blob_streaming_threshold_bytes are split into 128KB chunks and
        // written in parallel using a WriteBatch (bypasses per-write transaction
        // overhead) and a compact thread pool for encoding.  The manifest key +
        // all chunk keys are committed atomically in one WriteBatch.Write().
        // putBlob() / getBlob() expose the streaming API; putBlob() falls back to
        // put() for small values so callers need no size checks.
        bool enable_blob_streaming = true;
        // Minimum blob size that triggers the chunked streaming path (bytes).
        // Blobs smaller than this threshold are stored via the regular put() path.
        size_t blob_streaming_threshold_bytes = 65536;  // 64 KB
        // Size of each chunk when splitting a large blob (bytes).
        size_t blob_chunk_size_bytes = 131072;           // 128 KB
        // Number of threads used for parallel chunk encoding.
        // Each thread encodes (and optionally compresses) one chunk concurrently.
        int blob_streaming_threads = 4;
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

    /// Register a RocksDB EventListener that will receive compaction/flush/deletion
    /// events once the database is opened.  Must be called before open().
    void addEventListener(std::shared_ptr<rocksdb::EventListener> listener);

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

    /// Struct for a key-value pair used in batch writes.
    struct KeyValuePair {
        std::string key = {};
        std::vector<uint8_t> value;
    };

    /// Write multiple key-value pairs atomically in a single WriteBatch commit.
    ///
    /// All writes succeed or fail together.  This is significantly faster than
    /// N individual put() calls for OLTP workloads with many small writes because
    /// it opens only one MVCC transaction instead of N.
    ///
    /// @param pairs  Key-value pairs to write.
    /// @return true if all writes were committed successfully.
    bool putBatch(const std::vector<KeyValuePair>& pairs);

    // ===== Streaming Blob API (v2.0.0, PERF-D5) =====

    /// Store a blob using the high-throughput streaming write path.
    ///
    /// For blobs >= Config::blob_streaming_threshold_bytes the data is split
    /// into Config::blob_chunk_size_bytes chunks.  Chunks are encoded in
    /// parallel by a compact thread pool (Config::blob_streaming_threads) and
    /// then committed atomically via a single WriteBatch, bypassing per-write
    /// transaction overhead.  A manifest key records chunk metadata so that
    /// getBlob() can reassemble the blob transparently.
    ///
    /// Small blobs (< threshold) fall back to the regular put() path, so
    /// callers need no size checks and the API is backward compatible.
    ///
    /// Key scheme (internal, not part of public contract):
    ///   manifest : "__tmbs_m__:<key>"
    ///   chunk N  : "__tmbs_c__:<key>:<6-digit-index>"
    ///
    /// @param key  Logical blob key (visible to getBlob() / delBlob()).
    /// @param data Blob bytes.
    /// @return true on success.
    bool putBlob(std::string_view key, const std::vector<uint8_t>& data);

    /// Read a blob previously stored by putBlob() or put().
    ///
    /// Automatically detects whether the key was stored as a chunked blob
    /// (reads manifest + all chunks via MultiGet and reassembles) or as a
    /// regular value (single get()).
    ///
    /// @param key Logical blob key.
    /// @return Blob bytes, or std::nullopt if not found.
    std::optional<std::vector<uint8_t>> getBlob(std::string_view key);

    /// Delete a blob stored by putBlob() (removes manifest + all chunk keys).
    /// Falls back to del() for blobs stored via the regular path.
    /// @param key Logical blob key.
    /// @return true if at least one key was deleted.
    bool delBlob(std::string_view key);
    
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
    
    /// Create a write batch with index for fast reads from the batch (WBWI = Write Batch With Index)
    /// Useful for Read-Modify-Write workloads where you need to read recently written data
    /// before committing the batch. GetFromBatchAndDB will first check the batch before hitting DB.
    class WriteBatchWithIndexWrapper {
    public:
        explicit WriteBatchWithIndexWrapper(RocksDBWrapper* db, bool overwrite_key = true);
        ~WriteBatchWithIndexWrapper();
        
        void put(std::string_view key, const std::vector<uint8_t>& value);
        void del(std::string_view key);
        
        /// Get from batch only (very fast)
        std::optional<std::vector<uint8_t>> getFromBatch(std::string_view key) const;
        
        /// Get from batch first, then DB if not found (Read-Your-Own-Writes)
        std::optional<std::vector<uint8_t>> getFromBatchAndDB(std::string_view key) const;
        
        /// Commit the batch atomically
        bool commit();
        
        /// Rollback (discard) the batch
        void rollback();
        
    private:
        RocksDBWrapper* db_;
        std::unique_ptr<rocksdb::WriteBatchWithIndex> batch_;
        friend class RocksDBWrapper;
    };
    
    std::unique_ptr<WriteBatchWithIndexWrapper> createWriteBatchWithIndex(bool overwrite_key = true);
    
    // ===== MVCC Transaction Operations =====
    
    /// Isolation level for transactions (matches themis::IsolationLevel)
    enum class TransactionIsolationLevel {
        ReadCommitted,  // Read latest committed data (no snapshot overhead)
        Snapshot        // Snapshot isolation (repeatable reads, point-in-time consistency)
    };
    
    /// Create a new MVCC transaction with configurable isolation level
    class TransactionWrapper {
    public:
        /// Transaction state enum for better lifecycle management
        enum class State {
            NotStarted,      // Never attempted to start (db_ was null)
            CreationFailed,  // BeginTransaction() failed or operation failed
            Active,          // Transaction is alive and can be used
            Rolledback,      // Transaction rolled back
            Committed        // Transaction committed
        };
        
        explicit TransactionWrapper(RocksDBWrapper* db, TransactionIsolationLevel isolation = TransactionIsolationLevel::ReadCommitted);
        ~TransactionWrapper();
        
        /// Get value with isolation-dependent behavior
        /// ReadCommitted: reads latest committed data
        /// Snapshot: reads from transaction snapshot
        std::optional<std::vector<uint8_t>> get(std::string_view key);
        
        /// Acquire an exclusive write lock on a key for this transaction.
        ///
        /// Uses RocksDB GetForUpdate internally. The exclusive lock is held until the
        /// transaction commits or rolls back, preventing any other concurrent transaction
        /// from acquiring a conflicting lock on the same key.
        ///
        /// Primary use-case: serializing unique-constraint checks in the secondary-index
        /// write path so that two concurrent transactions cannot both pass the check and
        /// then both commit with the same unique value (the "Concurrent-Unique-Lücke").
        ///
        /// Returns true  – lock acquired (key may or may not exist in the DB).
        /// Returns false – lock acquisition failed (e.g. write-write conflict, timeout).
        ///                 Caller should roll back and return an error.
        bool getForUpdate(std::string_view key);

        /// Put key-value pair (visible only after commit)
        bool put(std::string_view key, const std::vector<uint8_t>& value);
        
        /// Delete key (effective only after commit)
        bool del(std::string_view key);
        
        /// Commit the transaction (may fail with conflict)
        bool commit();
        
        /// Rollback the transaction
        void rollback();

        /// Prepare the transaction (for WritePrepared policy)
        bool prepare();
        
        // ── Savepoint API ────────────────────────────────────────────────────

        /**
         * @brief Record a savepoint at the current write position.
         *
         * Multiple savepoints may be set; they form a stack (LIFO).
         * Corresponds to RocksDB Transaction::SetSavePoint().
         */
        void setSavePoint();

        /**
         * @brief Rollback all writes made after the most recent setSavePoint().
         *
         * Pops the most recent savepoint from the stack.  Returns true on
         * success; returns false if there is no outstanding savepoint.
         */
        bool rollbackToSavePoint();

        /**
         * @brief Discard (commit) the most recent savepoint without rolling back.
         *
         * The writes since the savepoint become permanent within the transaction.
         * Returns true on success; false if there is no outstanding savepoint.
         */
        bool popSavePoint();

        // ── Accessors ────────────────────────────────────────────────────────

        /// Check if transaction is still active
        bool isActive() const { return state_ == State::Active; }
        
        /// Get the snapshot (for debugging)
        /// Returns error if transaction is inactive or not initialized
        Result<const rocksdb::Snapshot*> getSnapshot() const;

        /// Reason why the most recent commit() call returned false.
        enum class CommitFailureType {
            None,        ///< No failure (commit succeeded or not yet attempted)
            Busy,        ///< RocksDB IsBusy() – write-write conflict / lock contention
            TimedOut,    ///< RocksDB IsTimedOut() – lock wait timeout
            TryAgain,    ///< RocksDB IsTryAgain() – transient, caller should retry
            CommitError, ///< Other commit error
        };

        /// Return the failure classification of the last commit() call.
        CommitFailureType getLastCommitFailureType() const { return last_commit_failure_type_; }

    private:
        RocksDBWrapper* db_;
        std::unique_ptr<rocksdb::Transaction> txn_;
        TransactionIsolationLevel isolation_;
        State state_ = State::NotStarted;
        bool prepared_ = false;
        CommitFailureType last_commit_failure_type_ = CommitFailureType::None;
        friend class RocksDBWrapper;
    };
    
    std::unique_ptr<TransactionWrapper> beginTransaction(TransactionIsolationLevel isolation = TransactionIsolationLevel::ReadCommitted);
    
    // ===== Iteration / Scanning =====
    
private:
    // Forward declare OperationGuard for use in SafeIterator
    class OperationGuard;
    
public:
    /// RAII wrapper for safe iterator usage
    /// Automatically manages database lifecycle during iteration
    /// Prevents use-after-free by holding OperationGuard
    class SafeIterator {
    public:
        SafeIterator(SafeIterator&& other) noexcept = default;
        SafeIterator& operator=(SafeIterator&& other) noexcept = default;
        
        // No copying - enforce move semantics for safety
        SafeIterator(const SafeIterator&) = delete;
        SafeIterator& operator=(const SafeIterator&) = delete;
        
        ~SafeIterator() = default;
        
        // Forward iterator interface
        void Seek(const std::string& target);
        void SeekToFirst();
        void SeekToLast();
        void Next();
        void Prev();
        bool Valid() const;
        std::string_view key() const;
        std::string_view value() const;
        
        // Check if iterator is usable
        explicit operator bool() const { return iterator_ != nullptr; }
        
    private:
        friend class RocksDBWrapper;
        
        SafeIterator(std::unique_ptr<rocksdb::Iterator> iter, 
                     std::unique_ptr<OperationGuard> guard)
            : iterator_(std::move(iter))
            , guard_(std::move(guard)) {}
        
        std::unique_ptr<rocksdb::Iterator> iterator_;
        std::unique_ptr<OperationGuard> guard_;  // Keeps database alive
    };
    
    /// Creates a safe iterator with automatic lifecycle management
    /// Preferred over newIterator() for most use cases
    /// 
    /// The returned SafeIterator holds an OperationGuard that prevents
    /// the database from being closed while the iterator is in use.
    /// 
    /// Thread-Safety:
    /// - Safe to call from multiple threads concurrently
    /// - Each thread gets its own iterator instance
    /// - Iterator itself is NOT thread-safe (use from single thread)
    /// 
    /// @param read_options Optional read options
    /// @return Result<SafeIterator> with automatic lifecycle management
    Result<SafeIterator> newSafeIterator(const rocksdb::ReadOptions* read_options = nullptr);
    
    /// Scan with prefix (for index scans)
    using ScanCallback = std::function<bool(std::string_view key, std::string_view value)>;
    void scanPrefix(std::string_view prefix, ScanCallback callback);
    
    /// Create a prefix iterator for enumeration
    /// Returns a SafeIterator positioned at the first key with the given prefix.
    /// The caller can iterate through all keys with that prefix by calling Valid()
    /// and Next() on the returned iterator.
    /// 
    /// @param prefix Prefix to search for
    /// @return Result<SafeIterator> positioned at prefix start, or error if iterator creation fails
    Result<SafeIterator> prefixIterator(std::string_view prefix);
    
    /// Scan range [start_key, end_key)
    void scanRange(std::string_view start_key, std::string_view end_key, ScanCallback callback);

    /// Iterate over a key range [start_key, end_key) using a rocksdb::Iterator.
    /// The callback receives each (key, value) pair in order; returning false
    /// from the callback stops iteration early.
    void iterateRange(std::string_view start_key, std::string_view end_key, ScanCallback callback);

    /// Full scan (use sparingly!)
    void scanAll(ScanCallback callback);
    
    // v1.3.0 Phase 2: Async I/O Scan Operations
    
    /// Scan with async I/O and prefetching (prefix-based)
    std::vector<std::pair<std::string, std::vector<uint8_t>>> scanWithAsyncIO(
        std::string_view prefix, int limit = 1000);
    
    /// Range query with async I/O
    std::vector<std::pair<std::string, std::vector<uint8_t>>> rangeQueryWithAsyncIO(
        std::string_view start_key, std::string_view end_key);
    
    /// Reverse scan with async I/O
    std::vector<std::pair<std::string, std::vector<uint8_t>>> reverseScanWithAsyncIO(
        std::string_view start_key, int limit = 1000);
    
    /// MultiGet with async I/O optimization
    std::vector<std::optional<std::vector<uint8_t>>> multiGetWithAsyncIO(
        const std::vector<std::string>& keys);
    
    /// Create async iterator with prefetching
    Result<std::unique_ptr<rocksdb::Iterator>> newAsyncIterator();
    
    /// Create standard iterator (for comparison)
    Result<std::unique_ptr<rocksdb::Iterator>> newIterator();
    
    /// Check if async I/O is enabled
    bool isAsyncIOEnabled() const { return config_.enable_async_io; }
    
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

    /// Get the latest RocksDB sequence number (monotonically increasing with every write).
    /// Returns 0 if the database is not open.
    uint64_t getLatestSequenceNumber() const;

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
    /// @return Result containing column family handle (owned by DB, don't delete) or error
    /// Error: ERR_INDEX_NOT_INITIALIZED if database is not open
    /// Error: ERR_INDEX_CREATION_FAILED if column family creation fails
    Result<rocksdb::ColumnFamilyHandle*> getOrCreateColumnFamily(const std::string& cf_name);

    /// Lightweight metadata snapshot for one column family
    struct CFInfo {
        std::string name;             ///< Column family name
        uint64_t estimated_keys = 0;  ///< rocksdb.estimate-num-keys
        uint64_t approx_size_bytes = 0; ///< rocksdb.total-sst-files-size
    };

    /// Enumerate all open column families with lightweight statistics.
    /// The returned snapshot is consistent under cf_handles_mutex_ but the
    /// statistics are approximate and may lag by one compaction cycle.
    /// @return Vector of CFInfo (empty if DB not open)
    std::vector<CFInfo> listColumnFamilies() const;
    
    /// Get raw RocksDB pointer for advanced operations
    rocksdb::TransactionDB* getRawDB() { return db_.get(); }
    const rocksdb::TransactionDB* getRawDB() const { return db_.get(); }
    // Backward-compatible alias used by older tests/adapters
    rocksdb::TransactionDB* getDB() { return getRawDB(); }
    const rocksdb::TransactionDB* getDB() const { return getRawDB(); }

private:
    // RAII helper to track active operations and prevent close during operations
    /** @brief RAII helper to track active operations and prevent close during operations. */
    class OperationGuard {
    public:
        explicit OperationGuard(const RocksDBWrapper* wrapper) 
            : wrapper_(wrapper), db_(nullptr) {
            if (wrapper_) {
                std::lock_guard<std::mutex> lock(wrapper_->db_lifecycle_mutex_);
                // Refuse to start a new operation once close() has begun (R-1 fix):
                // closing_ is set under db_lifecycle_mutex_ before db_.reset(), so
                // checking it here (while holding the same lock) makes the check
                // and the db_.reset() in close() mutually exclusive and race-free.
                if (wrapper_->db_ && !wrapper_->closing_.load(std::memory_order_relaxed)) {
                    wrapper_->active_operations_.fetch_add(1, std::memory_order_acquire);
                    db_ = wrapper_->db_.get();
                }
            }
        }
        
        ~OperationGuard() {
            if (wrapper_ && db_) {
                wrapper_->active_operations_.fetch_sub(1, std::memory_order_release);
            }
        }
        
        OperationGuard(const OperationGuard&) = delete;
        OperationGuard& operator=(const OperationGuard&) = delete;
        
        rocksdb::TransactionDB* get() const { return db_; }
        explicit operator bool() const { return db_ != nullptr; }
        
    private:
        const RocksDBWrapper* wrapper_;
        rocksdb::TransactionDB* db_;
    };

private:
    Config config_;
    std::unique_ptr<rocksdb::TransactionDB> db_;
    std::unique_ptr<rocksdb::Options> options_;
    std::unique_ptr<rocksdb::TransactionDBOptions> txn_db_options_;
    std::unique_ptr<rocksdb::TransactionOptions> txn_options_;
    std::unique_ptr<rocksdb::ReadOptions> read_options_;
    std::unique_ptr<rocksdb::WriteOptions> write_options_;
    mutable std::mutex options_mutex_;
    // Track created column family handles so they can be destroyed before DB close
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles_;
    // Mutex to protect cf_handles_ from concurrent access (race condition fix #1)
    mutable std::mutex cf_handles_mutex_;
    // Mutex to protect db_ lifecycle (race condition fix #3)
    mutable std::mutex db_lifecycle_mutex_;
    // Active operations counter for safe close (race condition fix #3)
    mutable std::atomic<int> active_operations_{0};
    // Set to true inside db_lifecycle_mutex_ when close() starts so that new
    // OperationGuards see it under the same lock and refuse to start (R-1 fix).
    mutable std::atomic<bool> closing_{false};
    // NVMe optimizations manager (null when enable_nvme_optimizations=false)
    std::unique_ptr<storage::NVMeManager> nvme_manager_;
    
    #ifdef THEMIS_DEBUG_THREADING
    // Track if object is being moved (debug only)
    // Used to detect concurrent move operations during development
    mutable std::atomic<bool> is_being_moved_{false};
    #endif
    
    void configureOptions();
    bool commitBatch(rocksdb::WriteBatch* batch);
};

} // namespace themis
