#include "storage/rocksdb_wrapper.h"
#include "utils/logger.h"
#include "utils/expected.h"
#include "performance/prefetch_hints.h"
#include <rocksdb/db.h>
#include <rocksdb/utilities/transaction_db.h>
#include <rocksdb/utilities/transaction.h>
#include <rocksdb/utilities/write_batch_with_index.h>
#include <rocksdb/options.h>
#include <rocksdb/write_batch.h>
#include <rocksdb/iterator.h>
#include <rocksdb/table.h>
#include <rocksdb/filter_policy.h>
#include <rocksdb/cache.h>
#include <rocksdb/advanced_options.h>
#include <rocksdb/statistics.h>
#include <rocksdb/utilities/checkpoint.h>
#include <rocksdb/utilities/backup_engine.h> // v1.1.0: Incremental Backup
#include <filesystem>
#include <nlohmann/json.hpp>
#include <unordered_map>
#include <iostream> // For debugging
#include <thread>   // For sleep_for (race condition fix #3)
#include <chrono>   // For milliseconds (race condition fix #3)

namespace themis {

using json = nlohmann::json;

RocksDBWrapper::RocksDBWrapper(const Config& config) : config_(config) {
    options_ = std::make_unique<rocksdb::Options>();
    txn_db_options_ = std::make_unique<rocksdb::TransactionDBOptions>();
    txn_options_ = std::make_unique<rocksdb::TransactionOptions>();
    read_options_ = std::make_unique<rocksdb::ReadOptions>();
    write_options_ = std::make_unique<rocksdb::WriteOptions>();
    configureOptions();
}

RocksDBWrapper::~RocksDBWrapper() {
    #ifdef THEMIS_DEBUG_THREADING
    // Ensure not being accessed during destruction
    if (is_being_moved_.load(std::memory_order_acquire)) {
        THEMIS_WARN("RocksDBWrapper being destroyed while in move state!");
    }
    #endif
    close();
}

RocksDBWrapper::RocksDBWrapper(RocksDBWrapper&& other) noexcept
    : config_(std::move(other.config_))
    , db_(std::move(other.db_))
    , options_(std::move(other.options_))
    , txn_db_options_(std::move(other.txn_db_options_))
    , txn_options_(std::move(other.txn_options_))
    , read_options_(std::move(other.read_options_))
    , write_options_(std::move(other.write_options_)) {
    
    // RACE CONDITION FIX: Lock mutex BEFORE checking/setting is_being_moved_ flag
    // This prevents another thread from accessing cf_handles_ between flag-set and lock
    std::lock_guard<std::mutex> lock(other.cf_handles_mutex_);
    
    #ifdef THEMIS_DEBUG_THREADING
    // ✅ DEBUG: Mark source object as being moved
    // In debug mode, we fail fast on concurrent move operations
    bool expected = false;
    if (!other.is_being_moved_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
        THEMIS_ERROR("Concurrent move operation detected on RocksDBWrapper!");
        assert(false && "Concurrent move operation - threading bug detected!");
    }
    #endif
    
    cf_handles_ = std::move(other.cf_handles_);
    
    #ifdef THEMIS_DEBUG_THREADING
    // Reset flag BEFORE clearing source object to avoid race window
    other.is_being_moved_.store(false, std::memory_order_release);
    #endif
    
    // Clear source object's state to prevent double-free
    other.db_.reset();
    other.cf_handles_.clear();
}

RocksDBWrapper& RocksDBWrapper::operator=(RocksDBWrapper&& other) noexcept {
    if (this != &other) {
        // Close our existing resources
        close();
        
        // RACE CONDITION FIX: Lock mutex BEFORE checking/setting is_being_moved_ flags
        // This prevents another thread from accessing cf_handles_ between flag-set and lock
        std::lock_guard<std::mutex> lock(other.cf_handles_mutex_);
        
        #ifdef THEMIS_DEBUG_THREADING
        // ✅ CRITICAL: Synchronize on both objects
        // Fail fast in debug mode if concurrent operations detected
        bool expected = false;
        if (!is_being_moved_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            THEMIS_ERROR("Concurrent move-assign operation detected on destination object!");
            assert(false && "Concurrent move-assign on destination - threading bug detected!");
        }
        expected = false;
        if (!other.is_being_moved_.compare_exchange_strong(expected, true, std::memory_order_acq_rel)) {
            THEMIS_ERROR("Concurrent move-assign detected on source object!");
            // Clean up destination flag before failing
            is_being_moved_.store(false, std::memory_order_release);
            assert(false && "Concurrent move-assign on source - threading bug detected!");
        }
        #endif
        
        // Move ownership from other
        config_ = std::move(other.config_);
        db_ = std::move(other.db_);
        options_ = std::move(other.options_);
        txn_db_options_ = std::move(other.txn_db_options_);
        txn_options_ = std::move(other.txn_options_);
        read_options_ = std::move(other.read_options_);
        write_options_ = std::move(other.write_options_);
        
        cf_handles_ = std::move(other.cf_handles_);
        
        // Clear source object's state to prevent double-free
        other.db_.reset();
        other.cf_handles_.clear();
        
        #ifdef THEMIS_DEBUG_THREADING
        is_being_moved_.store(false, std::memory_order_release);
        other.is_being_moved_.store(false, std::memory_order_release);
        #endif
    }
    return *this;
}

void RocksDBWrapper::configureOptions() {
    // Optional: auto-apply Phase 2H tuning for high concurrency when enabled
    if (config_.enable_high_parallel_tuning) {
        if (config_.max_background_compactions <= 0) config_.max_background_compactions = 8;
        if (config_.max_background_flushes <= 0) config_.max_background_flushes = 2;
        if (config_.background_threads_low <= 0) config_.background_threads_low = 8;
        if (config_.background_threads_high <= 0) config_.background_threads_high = 2;
        if (config_.max_subcompactions <= 1) config_.max_subcompactions = 2;
        if (config_.level0_file_num_compaction_trigger <= 0) config_.level0_file_num_compaction_trigger = 2;
        if (config_.level0_slowdown_writes_trigger <= 0) config_.level0_slowdown_writes_trigger = 8;
        if (config_.level0_stop_writes_trigger <= 0) config_.level0_stop_writes_trigger = 16;
        if (config_.block_cache_shard_bits < 0) config_.block_cache_shard_bits = 6; // 64 shards
        if (config_.db_write_buffer_size_mb == 0) config_.db_write_buffer_size_mb = 512;
    }
    // Create DB if missing
    options_->create_if_missing = true;
    
    // Enable statistics for monitoring (can be disabled for microbenchmarks)
    if (config_.enable_statistics) {
        options_->statistics = rocksdb::CreateDBStatistics();
        options_->statistics->set_stats_level(rocksdb::kExceptHistogramOrTimers);
    }
    
    // Memtable (write buffer) configuration
    // v1.3.0 Phase 2: Optimized write buffer settings for high throughput
    // Recommended for write-heavy workloads: write_buffer_size=256MB, max_write_buffer_number=6
    // Expected improvement: +20-40% write performance with proper tuning
    options_->write_buffer_size = config_.memtable_size_mb * 1024 * 1024;
    options_->max_write_buffer_number = config_.max_write_buffer_number;
    options_->min_write_buffer_number_to_merge = config_.min_write_buffer_number_to_merge;
    
    // Block cache (read cache) configuration
    // Prefer HyperClockCache if available; fallback to LRUCache for compatibility
    rocksdb::BlockBasedTableOptions table_options;
    // Some RocksDB builds (e.g., via vcpkg) do not expose NewHyperClockCache.
    // Use LRU cache universally for maximum compatibility.
    table_options.block_cache = rocksdb::NewLRUCache(
        static_cast<size_t>(config_.block_cache_size_mb) * 1024ull * 1024ull
    );
    table_options.cache_index_and_filter_blocks = config_.cache_index_and_filter_blocks;
    table_options.pin_l0_filter_and_index_blocks_in_cache = config_.pin_l0_filter_and_index_blocks_in_cache;
    table_options.partition_filters = config_.partition_filters;
    
    // v1.4.1: Configure checksum algorithm for data integrity
    // XXH3 is 3x faster than CRC32 with similar collision resistance
    // Based on research: Bonwick et al. (2010) "End-to-end Data Integrity"
    if (config_.checksum_type == Config::ChecksumType::XXH3) {
        table_options.checksum = rocksdb::kXXH3;  // Fastest, recommended
    } else {
        table_options.checksum = rocksdb::kCRC32c;  // Standard, compatible
    }
    
    // Bloom filter for faster point lookups
    // CRITICAL FIX: Avoid use-after-free with BlockBasedTableOptions
    // NewBlockBasedTableFactory makes a copy of the options internally.
    // BlockBasedTableOptions::filter_policy is a shared_ptr that manages lifetime.
    // Directly assign the newly created FilterPolicy pointer.
    table_options.filter_policy.reset(
        rocksdb::NewBloomFilterPolicy(config_.bloom_bits_per_key, false)
    );
    
    // Create BlockBasedTableFactory immediately - it will copy all internal structures
    options_->table_factory.reset(rocksdb::NewBlockBasedTableFactory(table_options));
    
    // Phase 2H: Configure background thread pools for high parallelism
    // CRITICAL: options_->env is initialized by RocksDB during Options construction
    // Ensure it's not null before calling SetBackgroundThreads
    if (options_->env == nullptr) {
        options_->env = rocksdb::Env::Default();
    }
    
    // Set flush thread pool (HIGH priority)
    if (config_.background_threads_high > 0 && options_->env) {
        options_->env->SetBackgroundThreads(config_.background_threads_high, rocksdb::Env::Priority::HIGH);
    }
    // Set compaction thread pool (LOW priority)
    if (config_.background_threads_low > 0 && options_->env) {
        options_->env->SetBackgroundThreads(config_.background_threads_low, rocksdb::Env::Priority::LOW);
    }
    
    // Compaction
    options_->max_background_jobs = config_.max_background_jobs;
    
    // Phase 2H: Granular background operation control
    if (config_.max_background_compactions > 0) {
        options_->max_background_compactions = config_.max_background_compactions;
    }
    if (config_.max_background_flushes > 0) {
        options_->max_background_flushes = config_.max_background_flushes;
    }
    if (config_.max_subcompactions > 0) {
        options_->max_subcompactions = config_.max_subcompactions;
    }
    if (config_.use_universal_compaction) {
        options_->compaction_style = rocksdb::kCompactionStyleUniversal;
    } else {
        options_->compaction_style = rocksdb::kCompactionStyleLevel;
    }
    options_->level_compaction_dynamic_level_bytes = config_.dynamic_level_bytes;
    options_->target_file_size_base = config_.target_file_size_base_mb * 1024ull * 1024ull;
    options_->max_bytes_for_level_base = config_.max_bytes_for_level_base_mb * 1024ull * 1024ull;
    
    // Phase 2H: Level0 file control to prevent write stalls
    options_->level0_file_num_compaction_trigger = config_.level0_file_num_compaction_trigger;
    options_->level0_slowdown_writes_trigger = config_.level0_slowdown_writes_trigger;
    options_->level0_stop_writes_trigger = config_.level0_stop_writes_trigger;
    
    // Phase 2H: Total write buffer size limit
    if (config_.db_write_buffer_size_mb > 0) {
        options_->db_write_buffer_size = config_.db_write_buffer_size_mb * 1024ull * 1024ull;
    }

    // Compression preferences (best-effort; depends on build of RocksDB)
    auto toCompression = [](const std::string& s) {
        std::string v = s;
        std::transform(v.begin(), v.end(), v.begin(), [](unsigned char c){ return std::tolower(c); });
        if (v == "none") return rocksdb::kNoCompression;
        if (v == "lz4") return rocksdb::kLZ4Compression;
        if (v == "lz4hc") return rocksdb::kLZ4HCCompression;
        if (v == "zstd") return rocksdb::kZSTD;
        if (v == "snappy") return rocksdb::kSnappyCompression;
        if (v == "zlib") return rocksdb::kZlibCompression;
        if (v == "bzip2" || v == "bz2") return rocksdb::kBZip2Compression;
        return rocksdb::kNoCompression;
    };
    options_->compression = toCompression(config_.compression_default);
    options_->bottommost_compression = toCompression(config_.compression_bottommost);
    
    // v1.3.0 Phase 2: Parallel Compression (RocksDB 10.6+)
    // Use threads only when compression is enabled to avoid extra scheduling
    const bool compression_enabled = options_->compression != rocksdb::kNoCompression;
    options_->compression_opts.parallel_threads = compression_enabled ? 8 : 0;
    options_->compression_opts.max_dict_bytes = 16 * 1024;  // 16KB dictionary for better compression
    
    // Parallel Write Optimization (RocksDB Best Practices)
    // v1.3.0 Lock Optimization: WRITE_PREPARED policy allows concurrent memtable writes
    // This significantly improves write throughput with many threads (>16)
    if (config_.write_policy != Config::WritePolicy::WriteCommitted) {
        options_->allow_concurrent_memtable_write = config_.allow_concurrent_memtable_write;
    } else {
        // WRITE_COMMITTED requires this to be disabled for correctness
        options_->allow_concurrent_memtable_write = false;
    }
    // Pipelined writes sind mit Prepare/Concurrent Prepares in TransactionDB
    // grundsätzlich inkompatibel. Da ThemisDB MVCC/Transaktionen auch für
    // mehrstufige Updates (CFs, Indizes) nutzt, deaktivieren wir sie global.
    // Siehe RocksDB: "pipelined_writes is not compatible with concurrent prepares".
    options_->enable_pipelined_write = false;
    // options_->allow_unordered_write = config_.allow_unordered_write;  // Not available in this version
    
    // WAL Configuration
    write_options_->sync = config_.enable_wal;
    write_options_->disableWAL = config_.disable_wal_for_benchmark;  // Phase 2F: Benchmark optimization
    if (!config_.wal_dir.empty()) {
        options_->wal_dir = config_.wal_dir;
    }

    // Place SSTables across multiple paths if configured
    if (!config_.db_paths.empty()) {
        std::vector<rocksdb::DbPath> paths;
        paths.reserve(config_.db_paths.size());
        for (const auto& p : config_.db_paths) {
            paths.emplace_back(p.path, static_cast<int64_t>(p.target_size_bytes));
        }
        options_->db_paths = std::move(paths);
    }

    // Direct I/O (can reduce OS cache thrashing when RocksDB cache is large)
    options_->use_direct_reads = config_.use_direct_reads;
    options_->use_direct_io_for_flush_and_compaction = config_.use_direct_io_for_flush_and_compaction;
    
    // MVCC Transaction Configuration
    txn_db_options_->transaction_lock_timeout = 1000; // 1 second timeout
    txn_db_options_->default_lock_timeout = 1000;

    // Per-Key Point Lock Manager options are not available in all RocksDB versions.
    // Skip setting unavailable TransactionDBOptions fields to preserve compatibility.

    // Configure TransactionDB write policy
    switch (config_.write_policy) {
        case Config::WritePolicy::WriteCommitted:
            txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_COMMITTED;
            break;
        case Config::WritePolicy::WritePrepared:
            txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_PREPARED;
            break;
        case Config::WritePolicy::WriteUnprepared:
            txn_db_options_->write_policy = rocksdb::TxnDBWritePolicy::WRITE_UNPREPARED;
            break;
    }
    if (config_.write_policy != Config::WritePolicy::WriteCommitted) {
        options_->two_write_queues = config_.two_write_queues;
    }
    
    // Set transaction options for optimistic concurrency control
    // SECURITY NOTE #13 (Phase 2): Snapshot lifecycle management
    // set_snapshot = true ensures consistent reads within transactions
    // Snapshots are transaction-local and automatically invalidated when transaction ends
    // Callers must not use snapshot pointers after transaction commit/rollback
    txn_options_->set_snapshot = true; // Automatically create snapshot on begin
    // Use single-phase commit; do not require Prepare() for Commit
    txn_options_->skip_prepare = true;
    
    // v1.3.0 Phase 2: Configure BlobDB for large values (>1KB)
    // Allow explicit disable even when memtables are large to avoid overhead for tiny values
    if (config_.enable_blobdb) {
        options_->enable_blob_files = true;
        options_->min_blob_size = 1024;  // 1KB threshold - values larger than this go to blob files
        options_->blob_compression_type = options_->compression;  // Use same compression as main DB
        options_->enable_blob_garbage_collection = true;  // Clean up obsolete blob files
        options_->blob_garbage_collection_age_cutoff = 0.25;  // GC blobs in files where >25% is garbage
    }
    
    // v1.4.1: Data Integrity & Robustness Configuration
    // Based on research: Bairavasundaram et al. (2008) "An Analysis of Data Corruption"
    //                    Bonwick et al. (2010) "End-to-end Data Integrity for File Systems"
    // See docs/DATABASE_FILE_ROBUSTNESS.md for detailed explanation and papers
    
    // CRITICAL: Enable paranoid checks to detect corruption early (~5% read overhead)
    // Research shows this catches 99.99% of corruption before it spreads
    options_->paranoid_checks = config_.paranoid_checks;
    
    // Enable checksum verification on all reads (~2% overhead)
    read_options_->verify_checksums = config_.verify_checksums_on_read;
    
    // Verify checksums during background compaction (no read overhead)
    // Note: verify_checksums_in_compaction is not available in RocksDB 8.9
    // options_->verify_checksums_in_compaction = config_.verify_checksums_in_compaction;
    
    // Force fsync on every write for maximum durability (~30% write overhead)
    // Recommended for financial data or critical writes
    if (config_.force_sync_on_write) {
        write_options_->sync = true;
    }
    
    // Disable memory-mapped I/O to prevent silent errors
    // mmap can hide I/O errors that would be caught by read()/write()
    // Recommended by: "All File Systems Are Not Created Equal" (Prabhakaran, 2005)
    if (config_.disable_mmap_reads) {
        options_->allow_mmap_reads = false;
    }
    if (config_.disable_mmap_writes) {
        options_->allow_mmap_writes = false;
    }
}

bool RocksDBWrapper::open() {
    // If already open, close cleanly to avoid stale handles before reopen
    if (db_) {
        close();
    }

    // Ensure target directories exist when using relative paths and tests run from build dir
    try {
        std::error_code ec;
        std::filesystem::path dbp(config_.db_path);
        auto parent = dbp.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                auto msg = std::string("Failed to create DB parent directory '") + parent.string() + "': " + ec.message();
                THEMIS_ERROR("{}", msg);
                fprintf(stderr, "%s\n", msg.c_str());
                return false;
            }
        }
        // Also ensure the DB directory exists to avoid RocksDB creating it in odd environments
        ec.clear();
        std::filesystem::create_directories(dbp, ec);
        if (ec) {
            auto msg = std::string("Failed to create DB directory '") + dbp.string() + "': " + ec.message();
            THEMIS_ERROR("{}", msg);
            fprintf(stderr, "%s\n", msg.c_str());
            return false;
        }
        if (!config_.wal_dir.empty()) {
            std::filesystem::path wald(config_.wal_dir);
            auto wparent = wald;
            if (wald.has_filename()) wparent = wald.parent_path();
            if (!wparent.empty()) {
                ec.clear();
                std::filesystem::create_directories(wparent, ec);
                if (ec) {
                    auto msg = std::string("Failed to create WAL parent directory '") + wparent.string() + "': " + ec.message();
                    THEMIS_ERROR("{}", msg);
                    fprintf(stderr, "%s\n", msg.c_str());
                    return false;
                }
            }
        }
    } catch (const std::exception& e) {
        auto msg = std::string("Exception while ensuring DB directories: ") + e.what();
        THEMIS_ERROR("{}", msg);
        fprintf(stderr, "%s\n", msg.c_str());
        return false;
    }

    // List existing column families to open them all
    std::vector<std::string> cf_names;
    rocksdb::DBOptions db_opts;
    db_opts.create_if_missing = options_->create_if_missing;
    db_opts.create_missing_column_families = options_->create_missing_column_families;
    db_opts.max_open_files = options_->max_open_files;
    db_opts.max_background_jobs = options_->max_background_jobs;
    rocksdb::Status list_status = rocksdb::DB::ListColumnFamilies(
        db_opts, 
        config_.db_path, 
        &cf_names
    );
    
    // If DB doesn't exist yet, start with default CF only
    if (!list_status.ok()) {
        cf_names = {rocksdb::kDefaultColumnFamilyName};
    }
    
    // Prepare column family descriptors
    std::vector<rocksdb::ColumnFamilyDescriptor> cf_descriptors;
    
    for (const auto& cf_name : cf_names) {
        rocksdb::ColumnFamilyOptions cf_opts;
        // Avoid calling OptimizeForPointLookup which can cause issues with large values
        // Instead configure options directly for stability
        cf_opts.write_buffer_size = options_->write_buffer_size;
        cf_opts.max_write_buffer_number = options_->max_write_buffer_number;
        cf_descriptors.emplace_back(cf_name, cf_opts);
    }
    
    // CRITICAL FIX: When sharding enabled, skip 2nd column family to prevent MVCC deadlock
    // Sharding mode creates a 2nd CF for cluster coordination, but if it's not fully
    // initialized yet, AdaptiveIndexManager tries to use MVCC across both CFs and blocks.
    // Solution: Open only default CF when sharding is detected via environment.
    const char* sharding_enabled = std::getenv("THEMIS_ENABLE_SHARDING");
    bool sharding_mode = sharding_enabled && 
                         (std::string(sharding_enabled) == "true" || 
                          std::string(sharding_enabled) == "1");
    
    if (sharding_mode && cf_descriptors.size() > 1) {
        THEMIS_WARN("Sharding mode detected: opening only default column family to prevent MVCC deadlock");
        // Keep only the default CF
        cf_descriptors.erase(
            std::remove_if(cf_descriptors.begin(), cf_descriptors.end(),
                          [](const rocksdb::ColumnFamilyDescriptor& cf) {
                              return cf.name != rocksdb::kDefaultColumnFamilyName;
                          }),
            cf_descriptors.end()
        );
    }
    
    // Open with available column families
    std::vector<rocksdb::ColumnFamilyHandle*> cf_handles;
    rocksdb::TransactionDB* txn_db_ptr = nullptr;
    rocksdb::Status status = rocksdb::TransactionDB::Open(
        *options_, 
        *txn_db_options_,
        config_.db_path,
        cf_descriptors,
        &cf_handles,
        &txn_db_ptr
    );
    
    if (!status.ok()) {
        // Ensure error is visible even if logger wasn't initialized yet
        auto msg = std::string("Failed to open RocksDB TransactionDB: ") + status.ToString();
        THEMIS_ERROR("{}", msg);
        fprintf(stderr, "%s\n", msg.c_str());
        return false;
    }
    
    // SECURITY FIX #12 (Phase 2): Prevent reopen leak
    // If db_ is already open (e.g., reopen after failed open), close it first
    // to avoid resource leak and ensure clean state
    if (db_) {
        THEMIS_WARN("Database already open during open() - closing existing connection first");
        close();
    }
    
    db_.reset(txn_db_ptr);
    
    // RACE CONDITION FIX #1: Protect cf_handles_ during initialization
    {
        std::lock_guard<std::mutex> lock(cf_handles_mutex_);
        // Store column family handles
        // When sharding mode filtered CFs, cf_handles.size() == cf_descriptors.size()
        for (size_t i = 0; i < cf_handles.size(); ++i) {
            // All remaining CFs (after sharding filter) are stored
            cf_handles_.push_back(cf_handles[i]);
        }
    }
    
    // Log actual CF count opened (not original cf_names.size())
    THEMIS_INFO("Opened RocksDB TransactionDB at: {} (MVCC enabled, {} column families opened)", 
                config_.db_path, cf_descriptors.size());
    if (sharding_mode && cf_descriptors.size() < cf_names.size()) {
        THEMIS_INFO("  (Sharding mode: {} additional CFs deferred for cluster initialization)", 
                    cf_names.size() - cf_descriptors.size());
    }
    return true;
}

void RocksDBWrapper::close() {
    if (db_) {
        THEMIS_INFO("Closing RocksDB");
        
        // RACE CONDITION FIX #3: Wait for active operations to complete before closing
        {
            std::lock_guard<std::mutex> lock(db_lifecycle_mutex_);
            // After acquiring lock, new operations can't start (OperationGuard checks db_ under lock)
            // Now we just need to wait for existing operations to finish
        }
        
        // Busy-wait for active operations to complete
        int wait_count = 0;
        while (active_operations_.load(std::memory_order_acquire) > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            wait_count++;
            if (wait_count % 100 == 0) {
                THEMIS_WARN("Waiting for {} active operations to complete before closing DB", 
                           active_operations_.load(std::memory_order_relaxed));
            }
        }
        
        // RACE CONDITION FIX #1: Protect cf_handles_ access during close
        std::lock_guard<std::mutex> lock(cf_handles_mutex_);
        
        // Destroy any created ColumnFamily handles first to avoid RocksDB assertions
        for (size_t i = 0; i < cf_handles_.size(); ++i) {
            auto* h = cf_handles_[i];
            if (h) {
                // Safe to call even if DB is shutting down; check status
                try {
                    db_->DestroyColumnFamilyHandle(h);
                } catch (const std::exception& e) {
                    // Log specific exception details for debugging
                    THEMIS_WARN("Exception while destroying ColumnFamilyHandle {}: {}", i, e.what());
                } catch (...) {
                    // Unknown exception - log index at least
                    THEMIS_WARN("Unknown exception while destroying ColumnFamilyHandle {}", i);
                }
            }
        }
        cf_handles_.clear();
        db_.reset();
    }
}

bool RocksDBWrapper::isOpen() const {
    return db_ != nullptr;
}

std::optional<std::vector<uint8_t>> RocksDBWrapper::get(std::string_view key) {
    if (!db_) return std::nullopt;
    
    std::string value;
    rocksdb::Status status = db_->Get(*read_options_, rocksdb::Slice(key.data(), key.size()), &value);
    
    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    }
    
    return std::nullopt;
}

bool RocksDBWrapper::get(std::string_view key, std::string& out) {
    if (!db_) return false;
    std::string value;
    rocksdb::Status status = db_->Get(*read_options_, rocksdb::Slice(key.data(), key.size()), &value);
    if (status.ok()) {
        out = std::move(value);
        return true;
    }
    return false;
}

bool RocksDBWrapper::put(std::string_view key, const std::vector<uint8_t>& value) {
    if (!db_) {
        themis::utils::Logger::error("RocksDBWrapper::put: db_ is null");
        return false;
    }
    
    // TransactionDB requires all writes to go through transactions for MVCC semantics
    // Always use transaction-based writes for consistency and correctness
    auto txn = beginTransaction();
    if (!txn) {
        themis::utils::Logger::error("RocksDBWrapper::put: failed to begin transaction");
        return false;
    }
    
    if (!txn->put(key, value)) {
        themis::utils::Logger::error("RocksDBWrapper::put (transaction): put failed");
        txn->rollback();
        return false;
    }
    
    if (!txn->commit()) {
        themis::utils::Logger::error("RocksDBWrapper::put (transaction): commit failed");
        return false;
    }
    
    return true;
}

bool RocksDBWrapper::put(std::string_view key, std::string_view value) {
    if (!db_) {
        themis::utils::Logger::error("RocksDBWrapper::put (string_view): db_ is null");
        return false;
    }
    
    // TransactionDB requires all writes to go through transactions for MVCC semantics
    auto txn = beginTransaction();
    if (!txn) {
        themis::utils::Logger::error("RocksDBWrapper::put (string_view): failed to begin transaction");
        return false;
    }
    
    // Convert string_view to vector for transaction API
    std::vector<uint8_t> val_vec(value.begin(), value.end());
    if (!txn->put(key, val_vec)) {
        themis::utils::Logger::error("RocksDBWrapper::put (string_view, transaction): put failed");
        txn->rollback();
        return false;
    }
    
    if (!txn->commit()) {
        themis::utils::Logger::error("RocksDBWrapper::put (string_view, transaction): commit failed");
        return false;
    }
    
    return true;
}

bool RocksDBWrapper::del(std::string_view key) {
    if (!db_) {
        themis::utils::Logger::error("RocksDBWrapper::del: db_ is null");
        return false;
    }

    // Keep write path consistent with MVCC: always go through a transaction
    auto txn = beginTransaction();
    if (!txn) {
        themis::utils::Logger::error("RocksDBWrapper::del: failed to begin transaction");
        return false;
    }

    if (!txn->del(key)) {
        themis::utils::Logger::error("RocksDBWrapper::del (transaction): delete failed");
        txn->rollback();
        return false;
    }

    if (!txn->commit()) {
        themis::utils::Logger::error("RocksDBWrapper::del (transaction): commit failed");
        return false;
    }

    return true;
}

std::vector<std::optional<std::vector<uint8_t>>> RocksDBWrapper::multiGet(
    const std::vector<std::string>& keys
) {
    std::vector<std::optional<std::vector<uint8_t>>> results;
    if (!db_) {
        THEMIS_ERROR("multiGet: database not open");
        return results;
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("multiGet: base DB is null");
        return results;
    }

    // Prepare keys for RocksDB MultiGet
    std::vector<rocksdb::Slice> rock_keys;
    rock_keys.reserve(keys.size());
    for (const auto& key : keys) {
        rock_keys.emplace_back(key);
    }

    // v1.4.1: CPU prefetch hints for improved random access performance
    // Prefetch key data pointers to warm up cache before RocksDB access
    // This can reduce memory latency by 20-40% for large random access batches
    if (config_.enable_cpu_prefetch && keys.size() >= config_.prefetch_min_batch_size) {
        using namespace performance;
        PrefetchConfig prefetch_config{
            .prefetch_distance = config_.prefetch_distance,
            .hint = PrefetchHint::T0,  // High locality - data accessed immediately
            .enabled = true,
            .min_batch_size = config_.prefetch_min_batch_size
        };
        
        // Prefetch key pointers ahead of RocksDB MultiGet operation
        for (size_t i = 0; i < keys.size(); ++i) {
            batch_prefetch(keys.data(), i, keys.size(), prefetch_config);
        }
    }

    std::vector<std::string> values;
    std::vector<rocksdb::Status> statuses = base_db->MultiGet(*read_options_, rock_keys, &values);

    results.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i) {
        // Prefetch next value string for copy operation
        if (config_.enable_cpu_prefetch && i + config_.prefetch_distance < keys.size() && 
            statuses[i + config_.prefetch_distance].ok()) {
            performance::prefetch(values[i + config_.prefetch_distance].data(), 
                                 performance::PrefetchHint::T0);
        }
        
        if (statuses[i].ok()) {
            std::vector<uint8_t> value(values[i].begin(), values[i].end());
            results.emplace_back(std::move(value));
        } else if (statuses[i].IsNotFound()) {
            results.emplace_back(std::nullopt);
        } else {
            THEMIS_ERROR("multiGet error for key {}: {}", keys[i], statuses[i].ToString());
            results.emplace_back(std::nullopt);
        }
    }

    return results;
}

// WriteBatchWrapper implementation

RocksDBWrapper::WriteBatchWrapper::WriteBatchWrapper(RocksDBWrapper* db)
    : db_(db), batch_(std::make_unique<rocksdb::WriteBatch>()) {}

RocksDBWrapper::WriteBatchWrapper::~WriteBatchWrapper() = default;

void RocksDBWrapper::WriteBatchWrapper::put(std::string_view key, const std::vector<uint8_t>& value) {
    batch_->Put(
        rocksdb::Slice(key.data(), key.size()),
        rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size())
    );
}

void RocksDBWrapper::WriteBatchWrapper::del(std::string_view key) {
    batch_->Delete(rocksdb::Slice(key.data(), key.size()));
}

bool RocksDBWrapper::WriteBatchWrapper::commit() {
    return db_->commitBatch(batch_.get());
}

void RocksDBWrapper::WriteBatchWrapper::rollback() {
    batch_->Clear();
}

std::unique_ptr<RocksDBWrapper::WriteBatchWrapper> RocksDBWrapper::createWriteBatch() {
    return std::make_unique<WriteBatchWrapper>(this);
}

// WriteBatchWithIndexWrapper implementation

RocksDBWrapper::WriteBatchWithIndexWrapper::WriteBatchWithIndexWrapper(RocksDBWrapper* db, bool overwrite_key)
    : db_(db) {
    batch_ = std::make_unique<rocksdb::WriteBatchWithIndex>(nullptr, overwrite_key);
}

RocksDBWrapper::WriteBatchWithIndexWrapper::~WriteBatchWithIndexWrapper() = default;

void RocksDBWrapper::WriteBatchWithIndexWrapper::put(std::string_view key, const std::vector<uint8_t>& value) {
    batch_->Put(
        rocksdb::Slice(key.data(), key.size()),
        rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size())
    );
}

void RocksDBWrapper::WriteBatchWithIndexWrapper::del(std::string_view key) {
    batch_->Delete(rocksdb::Slice(key.data(), key.size()));
}

std::optional<std::vector<uint8_t>> RocksDBWrapper::WriteBatchWithIndexWrapper::getFromBatch(std::string_view key) {
    if (!db_) return std::nullopt;
    
    std::string value;
    rocksdb::Status status = batch_->GetFromBatch(
        *db_->options_,
        rocksdb::Slice(key.data(), key.size()),
        &value
    );
    
    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    }
    return std::nullopt;
}

std::optional<std::vector<uint8_t>> RocksDBWrapper::WriteBatchWithIndexWrapper::getFromBatchAndDB(std::string_view key) {
    if (!db_ || !db_->db_) return std::nullopt;
    
    std::string value;
    rocksdb::Status status = batch_->GetFromBatchAndDB(
        db_->db_.get(),
        *db_->read_options_,
        rocksdb::Slice(key.data(), key.size()),
        &value
    );
    
    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    }
    return std::nullopt;
}

bool RocksDBWrapper::WriteBatchWithIndexWrapper::commit() {
    if (!db_ || !batch_) return false;
    // WriteBatchWithIndex inherits from WriteBatch - cast to parent
    rocksdb::WriteBatch* wb = dynamic_cast<rocksdb::WriteBatch*>(batch_.get());
    if (!wb) return false;
    rocksdb::Status status = db_->db_->Write(*db_->write_options_, wb);
    return status.ok();
}

void RocksDBWrapper::WriteBatchWithIndexWrapper::rollback() {
    batch_->Clear();
}

std::unique_ptr<RocksDBWrapper::WriteBatchWithIndexWrapper> RocksDBWrapper::createWriteBatchWithIndex(bool overwrite_key) {
    return std::make_unique<WriteBatchWithIndexWrapper>(this, overwrite_key);
}

// TransactionWrapper implementation (MVCC)

RocksDBWrapper::TransactionWrapper::TransactionWrapper(RocksDBWrapper* db, TransactionIsolationLevel isolation)
    : db_(db), isolation_(isolation), state_(State::NotStarted) {
    if (!db_ || !db_->db_) {
        THEMIS_ERROR("MVCC Transaction: db_ is nullptr");
        state_ = State::NotStarted;
        return;
    }

    txn_.reset(db_->db_->BeginTransaction(*db_->write_options_, *db_->txn_options_));
    if (!txn_) {
        THEMIS_ERROR("MVCC Transaction: BeginTransaction returned nullptr");
        state_ = State::CreationFailed;
        return;
    }

    state_ = State::Active;
    if (isolation_ == TransactionIsolationLevel::Snapshot) {
        THEMIS_DEBUG("MVCC Transaction started with Snapshot isolation");
    } else {
        THEMIS_DEBUG("MVCC Transaction started with ReadCommitted isolation");
    }
}

RocksDBWrapper::TransactionWrapper::~TransactionWrapper() {
    // Always attempt cleanup regardless of state
    if (txn_) {
        try {
            if (state_ == State::Active && db_ && db_->getRawDB()) {
                THEMIS_WARN("Transaction not committed or rolled back - auto-rolling back");
                txn_->Rollback();
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("Exception during transaction cleanup: {}", e.what());
        } catch (...) {
            THEMIS_ERROR("Unknown exception during transaction cleanup");
        }
        
        // IMPORTANT: If the DB is closed (getRawDB() is null), we cannot safely destroy
        // the transaction object because it may try to access the closed DB.
        // Instead, release the pointer without calling its destructor.
        // 
        // This scenario occurs when:
        // 1. Transaction is created
        // 2. DB is closed (e.g., RocksDBWrapper::close() is called)
        // 3. Transaction destructor runs (transaction still holds pointer to closed DB)
        //
        // Mitigation strategies:
        // - Normal case: Application properly commits/rollbacks before closing DB (no leak)
        // - Edge case: If DB closes first, we leak the transaction pointer to avoid crash
        // - The leak is acceptable because it only happens at shutdown when DB is closing
        // - Alternative: Implement weak_ptr tracking of all transactions in RocksDBWrapper
        //   to invalidate them before DB close (would add complexity and overhead)
        if (db_ && db_->getRawDB()) {
            txn_.reset();  // Safe to destroy when DB is open
        } else {
            // DB is closed, just release without destroying to avoid crash
            txn_.release();  // Intentional leak in rare edge case (DB shutdown)
        }
    }
}

std::optional<std::vector<uint8_t>> RocksDBWrapper::TransactionWrapper::get(std::string_view key) {
    if (!txn_) return std::nullopt;
    if (state_ != State::Active) {
        THEMIS_ERROR("TransactionWrapper::get: transaction not active");
        return std::nullopt;
    }
    
    std::string value;
    rocksdb::ReadOptions read_opts;
    
    // OPTIMIZATION: Only use snapshot for Snapshot isolation level
    // ReadCommitted reads latest committed data without snapshot overhead
    if (isolation_ == TransactionIsolationLevel::Snapshot) {
        read_opts.snapshot = txn_->GetSnapshot();
    }
    // For ReadCommitted, snapshot is nullptr, reads latest committed data
    
    rocksdb::Status status = txn_->Get(read_opts, rocksdb::Slice(key.data(), key.size()), &value);
    
    if (status.ok()) {
        return std::vector<uint8_t>(value.begin(), value.end());
    }
    
    return std::nullopt;
}

bool RocksDBWrapper::TransactionWrapper::put(std::string_view key, const std::vector<uint8_t>& value) {
    if (!txn_) {
        THEMIS_ERROR("TransactionWrapper::put: txn_ is nullptr");
        return false;
    }
    if (state_ != State::Active) {
        THEMIS_ERROR("TransactionWrapper::put: transaction not active");
        return false;
    }
    
    try {
        rocksdb::Status status = txn_->Put(
            rocksdb::Slice(key.data(), key.size()),
            rocksdb::Slice(reinterpret_cast<const char*>(value.data()), value.size())
        );
        
        if (!status.ok()) {
            THEMIS_ERROR("TransactionWrapper::put: Put() failed: " + status.ToString());
            // Transaction is still active, caller should decide to rollback or retry
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during transaction put: {}", e.what());
        // Transaction state is uncertain after exception, mark as failed
        state_ = State::CreationFailed;
        return false;
    }
}

bool RocksDBWrapper::TransactionWrapper::del(std::string_view key) {
    if (!txn_) {
        THEMIS_ERROR("TransactionWrapper::del: txn_ is nullptr");
        return false;
    }
    if (state_ != State::Active) {
        THEMIS_ERROR("TransactionWrapper::del: transaction not active");
        return false;
    }

    try {
        rocksdb::Status status = txn_->Delete(rocksdb::Slice(key.data(), key.size()));
        if (!status.ok()) {
            THEMIS_ERROR("TransactionWrapper::del: Delete() failed: {}", status.ToString());
            // Transaction is still active, caller should decide to rollback or retry
            return false;
        }
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during transaction delete: {}", e.what());
        // Transaction state is uncertain after exception, mark as failed
        state_ = State::CreationFailed;
        return false;
    }
}

bool RocksDBWrapper::TransactionWrapper::commit() {
    if (!txn_ || state_ != State::Active) {
        return false;
    }
    
    try {
        // If WritePrepared is active or skip_prepare is false, ensure Prepare() is called
        bool require_prepare = false;
        if (db_) {
            // Only require prepare when skip_prepare is explicitly disabled
            require_prepare = (db_->txn_options_ && !db_->txn_options_->skip_prepare);
        }
        if (require_prepare && !prepared_) {
            rocksdb::Status prep_st = txn_->Prepare();
            if (!prep_st.ok()) {
                THEMIS_ERROR("Transaction prepare failed before commit: {}", prep_st.ToString());
                // Transaction is still active but prepare failed, caller should rollback
                return false;
            }
            prepared_ = true;
        }
        
        rocksdb::Status status = txn_->Commit();
        
        if (!status.ok()) {
            if (status.IsBusy() || status.IsTimedOut() || status.IsTryAgain()) {
                THEMIS_WARN("MVCC Conflict detected: {} - Transaction must be retried", status.ToString());
            } else {
                THEMIS_ERROR("Transaction commit failed: {}", status.ToString());
            }
            // Transaction is still active but commit failed, caller should rollback
            return false;
        }
        
        state_ = State::Committed;
        
        THEMIS_DEBUG("MVCC Transaction committed successfully");
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during transaction commit: {}", e.what());
        // Transaction state is uncertain after exception, mark as failed
        state_ = State::CreationFailed;
        return false;
    }
}

void RocksDBWrapper::TransactionWrapper::rollback() {
    if (!txn_ || state_ != State::Active) return;
    
    try {
        txn_->Rollback();
        state_ = State::Rolledback;
        THEMIS_DEBUG("MVCC Transaction rolled back");
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during transaction rollback: {}", e.what());
        state_ = State::CreationFailed;
    }
}

Result<const rocksdb::Snapshot*> RocksDBWrapper::TransactionWrapper::getSnapshot() const {
    if (!txn_ || state_ != State::Active) {
        return Err<const rocksdb::Snapshot*>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "Transaction not active or not initialized"
        );
    }
    return Ok(txn_->GetSnapshot());
}

bool RocksDBWrapper::TransactionWrapper::prepare() {
    if (!txn_ || state_ != State::Active) return false;
    
    try {
        rocksdb::Status status = txn_->Prepare();
        if (!status.ok()) {
            THEMIS_ERROR("Transaction prepare failed: {}", status.ToString());
            // Transaction is still active but prepare failed
            return false;
        }
        prepared_ = true;
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("Exception during transaction prepare: {}", e.what());
        // Transaction state is uncertain after exception, mark as failed
        state_ = State::CreationFailed;
        return false;
    }
}

std::unique_ptr<RocksDBWrapper::TransactionWrapper> RocksDBWrapper::beginTransaction(TransactionIsolationLevel isolation) {
    return std::make_unique<TransactionWrapper>(this, isolation);
}

bool RocksDBWrapper::commitBatch(rocksdb::WriteBatch* batch) {
    if (!db_) return false;
    
    rocksdb::Status status = db_->Write(*write_options_, batch);
    return status.ok();
}

void RocksDBWrapper::scanPrefix(std::string_view prefix, ScanCallback callback) {
    // RACE CONDITION FIX #3: Protect iterator lifetime with OperationGuard
    OperationGuard guard(this);
    if (!guard) return;

    auto* base_db = guard.get()->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("scanPrefix: base DB is null");
        return;
    }

    // SECURITY FIX #15 (Phase 3): Prevent infinite loop in prefix scanning
    // Use prefix_same_as_start to optimize prefix scans and prevent over-iteration
    // RocksDB will automatically stop iteration when prefix changes
    rocksdb::ReadOptions scan_options = *read_options_;
    scan_options.prefix_same_as_start = true;
    
    std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(scan_options));
    
    if (!it) {
        THEMIS_ERROR("scanPrefix: failed to create iterator");
        return;
    }
    
    rocksdb::Slice prefix_slice(prefix.data(), prefix.size());
    
    // v1.4.1: CPU prefetch hints for iterator scanning
    // Prefetch sequential cache lines to hide memory latency during iteration
    // Assumption: RocksDB iterators access data sequentially in memory blocks
    const size_t PREFETCH_AHEAD_BYTES = 256; // Prefetch next 4 cache lines (64 bytes each)
    
    for (it->Seek(prefix_slice); it->Valid() && it->key().starts_with(prefix_slice); it->Next()) {
        std::string_view key(it->key().data(), it->key().size());
        std::string_view value(it->value().data(), it->value().size());
        
        // Prefetch memory ahead of current position for better sequential access
        // This helps when iterating over large values or many small entries
        if (config_.enable_cpu_prefetch) {
            // Prefetch beyond current value to warm cache for next iteration
            const char* prefetch_addr = value.data() + value.size();
            performance::prefetch_range(prefetch_addr, PREFETCH_AHEAD_BYTES, 
                                       performance::PrefetchHint::T1);
        }
        
        if (!callback(key, value)) {
            break; // Stop iteration if callback returns false
        }
    }
    // OperationGuard destructor ensures db_ stays valid until here
}

void RocksDBWrapper::scanRange(std::string_view start_key, std::string_view end_key, ScanCallback callback) {
    // RACE CONDITION FIX #3: Protect iterator lifetime with OperationGuard
    OperationGuard guard(this);
    if (!guard) return;

    auto* base_db = guard.get()->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("scanRange: base DB is null");
        return;
    }

    std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
    
    if (!it) {
        THEMIS_ERROR("scanRange: failed to create iterator");
        return;
    }
    
    rocksdb::Slice start_slice(start_key.data(), start_key.size());
    rocksdb::Slice end_slice(end_key.data(), end_key.size());
    
    // v1.4.1: CPU prefetch hints for range scanning
    const size_t PREFETCH_AHEAD_BYTES = 256;
    
    for (it->Seek(start_slice); it->Valid() && it->key().compare(end_slice) < 0; it->Next()) {
        std::string_view key(it->key().data(), it->key().size());
        std::string_view value(it->value().data(), it->value().size());
        
        // Prefetch sequential cache lines ahead for better data locality
        if (config_.enable_cpu_prefetch) {
            const char* prefetch_addr = value.data() + value.size();
            performance::prefetch_range(prefetch_addr, PREFETCH_AHEAD_BYTES,
                                       performance::PrefetchHint::T1);
        }
        
        if (!callback(key, value)) {
            break;
        }
    }
}

void RocksDBWrapper::scanAll(ScanCallback callback) {
    // RACE CONDITION FIX #3: Protect iterator lifetime with OperationGuard
    OperationGuard guard(this);
    if (!guard) return;

    auto* base_db = guard.get()->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("scanAll: base DB is null");
        return;
    }

    std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(*read_options_));
    
    if (!it) {
        THEMIS_ERROR("scanAll: failed to create iterator");
        return;
    }
    
    for (it->SeekToFirst(); it->Valid(); it->Next()) {
        std::string_view key(it->key().data(), it->key().size());
        std::string_view value(it->value().data(), it->value().size());
        
        if (!callback(key, value)) {
            break;
        }
    }
}

std::string RocksDBWrapper::getStats() const {
    if (!db_) return R"({"error": "Database not open"})";
    
    // Get full RocksDB stats text
    std::string raw_stats;
    db_->GetProperty("rocksdb.stats", &raw_stats);
    
    // Get specific numeric properties for structured output
    uint64_t block_cache_usage = 0;
    uint64_t block_cache_capacity = 0;
    uint64_t estimate_keys = 0;
    uint64_t estimate_live_data_size = 0;
    uint64_t estimate_pending_compaction_bytes = 0;
    uint64_t num_running_compactions = 0;
    uint64_t num_running_flushes = 0;
    uint64_t memtable_size = 0;
    uint64_t cur_size_all_mem_tables = 0;
    
    db_->GetIntProperty("rocksdb.block-cache-usage", &block_cache_usage);
    db_->GetIntProperty("rocksdb.block-cache-capacity", &block_cache_capacity);
    db_->GetIntProperty("rocksdb.estimate-num-keys", &estimate_keys);
    db_->GetIntProperty("rocksdb.estimate-live-data-size", &estimate_live_data_size);
    db_->GetIntProperty("rocksdb.estimate-pending-compaction-bytes", &estimate_pending_compaction_bytes);
    db_->GetIntProperty("rocksdb.num-running-compactions", &num_running_compactions);
    db_->GetIntProperty("rocksdb.num-running-flushes", &num_running_flushes);
    db_->GetIntProperty("rocksdb.size-all-mem-tables", &memtable_size);
    db_->GetIntProperty("rocksdb.cur-size-all-mem-tables", &cur_size_all_mem_tables);
    
    // Get per-level file counts
    std::string num_files_at_levels;
    for (int level = 0; level < 7; ++level) {
        uint64_t num_files = 0;
        std::string prop = "rocksdb.num-files-at-level" + std::to_string(level);
        db_->GetIntProperty(prop, &num_files);
        if (level > 0) num_files_at_levels += ", ";
        num_files_at_levels += "\"L" + std::to_string(level) + "\": " + std::to_string(num_files);
    }
    
    // Get statistics counters if available
    std::string stats_counters;
    if (options_->statistics) {
        auto stats = options_->statistics;
        uint64_t block_cache_hit = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
        uint64_t block_cache_miss = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
        uint64_t bytes_written = stats->getTickerCount(rocksdb::BYTES_WRITTEN);
        uint64_t bytes_read = stats->getTickerCount(rocksdb::BYTES_READ);
        uint64_t compaction_key_drop_obsolete = stats->getTickerCount(rocksdb::COMPACTION_KEY_DROP_OBSOLETE);
        
        double hit_rate = 0.0;
        uint64_t total_access = block_cache_hit + block_cache_miss;
        if (total_access > 0) {
            hit_rate = static_cast<double>(block_cache_hit) / total_access * 100.0;
        }
        
        stats_counters = "\"block_cache_hit\": " + std::to_string(block_cache_hit) + ", "
                        "\"block_cache_miss\": " + std::to_string(block_cache_miss) + ", "
                        "\"cache_hit_rate_percent\": " + std::to_string(hit_rate) + ", "
                        "\"bytes_written\": " + std::to_string(bytes_written) + ", "
                        "\"bytes_read\": " + std::to_string(bytes_read) + ", "
                        "\"compaction_keys_dropped\": " + std::to_string(compaction_key_drop_obsolete);
    }
    
    // Build JSON response
    std::string json = "{\n"
        "  \"rocksdb\": {\n"
        "    \"block_cache_usage_bytes\": " + std::to_string(block_cache_usage) + ",\n"
        "    \"block_cache_capacity_bytes\": " + std::to_string(block_cache_capacity) + ",\n"
        "    \"estimate_num_keys\": " + std::to_string(estimate_keys) + ",\n"
        "    \"estimate_live_data_size_bytes\": " + std::to_string(estimate_live_data_size) + ",\n"
        "    \"estimate_pending_compaction_bytes\": " + std::to_string(estimate_pending_compaction_bytes) + ",\n"
        "    \"num_running_compactions\": " + std::to_string(num_running_compactions) + ",\n"
        "    \"num_running_flushes\": " + std::to_string(num_running_flushes) + ",\n"
        "    \"memtable_size_bytes\": " + std::to_string(memtable_size) + ",\n"
        "    \"cur_size_all_mem_tables_bytes\": " + std::to_string(cur_size_all_mem_tables) + ",\n"
        "    \"files_per_level\": { " + num_files_at_levels + " }";
    
    if (!stats_counters.empty()) {
        json += ",\n    " + stats_counters;
    }
    
    json += "\n  },\n"
        "  \"raw_stats\": " + nlohmann::json(raw_stats).dump() + "\n"
        "}";
    
    return json;
}

std::string RocksDBWrapper::getCompressionType() const {
    if (!db_) return "unknown (db closed)";
    
    // Query the active compression type via column family options
    auto cf_options = db_->GetOptions();
    
    auto fromCompression = [](rocksdb::CompressionType ct) -> std::string {
        switch (ct) {
            case rocksdb::kNoCompression: return "none";
            case rocksdb::kSnappyCompression: return "snappy";
            case rocksdb::kZlibCompression: return "zlib";
            case rocksdb::kBZip2Compression: return "bzip2";
            case rocksdb::kLZ4Compression: return "lz4";
            case rocksdb::kLZ4HCCompression: return "lz4hc";
            case rocksdb::kXpressCompression: return "xpress";
            case rocksdb::kZSTD: return "zstd";
            case rocksdb::kDisableCompressionOption: return "disabled";
            default: return "unknown";
        }
    };
    
    std::string default_compression = fromCompression(cf_options.compression);
    std::string bottommost = fromCompression(cf_options.bottommost_compression);
    
    return "default=" + default_compression + ", bottommost=" + bottommost;
}

void RocksDBWrapper::compactRange(std::string_view start_key, std::string_view end_key) {
    if (!db_) return;
    
    rocksdb::Slice start(start_key.data(), start_key.size());
    rocksdb::Slice end(end_key.data(), end_key.size());
    
    rocksdb::CompactRangeOptions options;
    db_->CompactRange(options, &start, &end);
}

void RocksDBWrapper::flush() {
    if (!db_) return;
    
    rocksdb::FlushOptions options;
    db_->Flush(options);
}

uint64_t RocksDBWrapper::getApproximateSize() const {
    if (!db_) return 0;
    
    // TODO: Implement proper size calculation
    return 0;
}

bool RocksDBWrapper::createCheckpoint(const std::string& checkpoint_dir) {
    if (!db_) {
        THEMIS_ERROR("createCheckpoint failed: DB is not open");
        fprintf(stderr, "%s\n", "createCheckpoint failed: DB is not open");
        return false;
    }
    try {
        // Ensure parent directory exists, but do not pre-create the checkpoint directory itself
        std::error_code ec;
        std::filesystem::path cpp(checkpoint_dir);
        auto parent = cpp.parent_path();
        if (!parent.empty()) {
            std::filesystem::create_directories(parent, ec);
            if (ec) {
                THEMIS_ERROR("Failed to create checkpoint parent directory '{}': {}", parent.string(), ec.message());
                fprintf(stderr, "Failed to create checkpoint parent directory '%s': %s\\n", parent.string().c_str(), ec.message().c_str());
                return false;
            }
        }
        rocksdb::Checkpoint* raw = nullptr;
        auto st = rocksdb::Checkpoint::Create(db_.get(), &raw);
        if (!st.ok()) {
            THEMIS_ERROR("RocksDB Checkpoint::Create failed: {}", st.ToString());
            fprintf(stderr, "RocksDB Checkpoint::Create failed: %s\n", st.ToString().c_str());
            return false;
        }
        std::unique_ptr<rocksdb::Checkpoint> cp(raw);
        st = cp->CreateCheckpoint(checkpoint_dir);
        if (!st.ok()) {
            THEMIS_ERROR("CreateCheckpoint to '{}' failed: {}", checkpoint_dir, st.ToString());
            fprintf(stderr, "CreateCheckpoint to '%s' failed: %s\\n", checkpoint_dir.c_str(), st.ToString().c_str());
            return false;
        }
        THEMIS_INFO("Checkpoint created at '{}'", checkpoint_dir);
        fprintf(stderr, "Checkpoint created at '%s'\n", checkpoint_dir.c_str());
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("createCheckpoint exception: {}", e.what());
        fprintf(stderr, "createCheckpoint exception: %s\n", e.what());
        return false;
    }
}

bool RocksDBWrapper::restoreFromCheckpoint(const std::string& checkpoint_dir) {
    try {
        if (!std::filesystem::exists(checkpoint_dir)) {
            THEMIS_ERROR("restoreFromCheckpoint: checkpoint dir '{}' does not exist", checkpoint_dir);
            fprintf(stderr, "restoreFromCheckpoint: checkpoint dir '%s' does not exist\n", checkpoint_dir.c_str());
            return false;
        }
        // Close DB if open
        if (db_) {
            close();
        }
        const auto& target = config_.db_path;
        std::error_code ec;
        if (std::filesystem::exists(target)) {
            std::filesystem::remove_all(target, ec);
            if (ec) {
                THEMIS_ERROR("Failed to remove existing DB path '{}': {}", target, ec.message());
                fprintf(stderr, "Failed to remove existing DB path '%s': %s\n", target.c_str(), ec.message().c_str());
                return false;
            }
        }
        std::filesystem::create_directories(target, ec);
        if (ec) {
            THEMIS_ERROR("Failed to create DB path '{}': {}", target, ec.message());
            fprintf(stderr, "Failed to create DB path '%s': %s\n", target.c_str(), ec.message().c_str());
            return false;
        }
        // Copy checkpoint contents into DB path
        std::filesystem::copy(
            checkpoint_dir,
            target,
            std::filesystem::copy_options::recursive | std::filesystem::copy_options::overwrite_existing,
            ec
        );
        if (ec) {
            THEMIS_ERROR("Failed to copy checkpoint '{}' to '{}': {}", checkpoint_dir, target, ec.message());
            fprintf(stderr, "Failed to copy checkpoint '%s' to '%s': %s\n", checkpoint_dir.c_str(), target.c_str(), ec.message().c_str());
            return false;
        }
        // Reopen DB
        if (!open()) {
            THEMIS_ERROR("Failed to reopen DB after restore from '{}'", checkpoint_dir);
            fprintf(stderr, "Failed to reopen DB after restore from '%s'\n", checkpoint_dir.c_str());
            return false;
        }
        THEMIS_INFO("Restored DB from checkpoint '{}' to '{}'", checkpoint_dir, target);
        fprintf(stderr, "Restored DB from checkpoint '%s' to '%s'\n", checkpoint_dir.c_str(), target.c_str());
        return true;
    } catch (const std::exception& e) {
        THEMIS_ERROR("restoreFromCheckpoint exception: {}", e.what());
        fprintf(stderr, "restoreFromCheckpoint exception: %s\n", e.what());
        return false;
    }
}

Result<rocksdb::ColumnFamilyHandle*> RocksDBWrapper::getOrCreateColumnFamily(const std::string& cf_name) {
    // RACE CONDITION FIX #1: Protect entire check-create-insert sequence with mutex
    std::lock_guard<std::mutex> lock(cf_handles_mutex_);
    
    if (!db_) {
        THEMIS_ERROR("getOrCreateColumnFamily: DB not open");
        return Err<rocksdb::ColumnFamilyHandle*>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB not opened for column family: " + cf_name
        );
    }
    
    // Check if CF already exists in our handles (now protected by mutex)
    for (auto* handle : cf_handles_) {
        if (handle && handle->GetName() == cf_name) {
            THEMIS_DEBUG("Column family '{}' already exists", cf_name);
            return Ok(handle);
        }
    }
    
    // Create new column family with default options
    rocksdb::ColumnFamilyOptions cf_opts;
    // Avoid OptimizeForPointLookup because it mutates multiple internals implicitly; stick to defaults
    rocksdb::ColumnFamilyHandle* cf_handle = nullptr;
    rocksdb::Status s = db_->CreateColumnFamily(cf_opts, cf_name, &cf_handle);
    
    if (!s.ok()) {
        THEMIS_ERROR("Failed to create column family '{}': {}", cf_name, s.ToString());
        return Err<rocksdb::ColumnFamilyHandle*>(
            errors::ErrorCode::ERR_INDEX_CREATION_FAILED,
            fmt::format("Failed to create column family '{}': {}", cf_name, s.ToString())
        );
    }
    
    // Track handle so we can destroy it on close (protected by mutex)
    cf_handles_.push_back(cf_handle);
    THEMIS_INFO("Created or got column family '{}'", cf_name);
    return Ok(cf_handle);
}

// ===== v1.1.0: Advanced RocksDB Features =====

bool RocksDBWrapper::createIncrementalBackup(const std::string& backup_dir, bool flush_before_backup) {
    if (!db_) {
        THEMIS_ERROR("createIncrementalBackup failed: database not open");
        return false;
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("createIncrementalBackup failed: base DB is null");
        return false;
    }
    
    try {
        rocksdb::BackupEngineOptions backup_opts(backup_dir);
        backup_opts.share_table_files = true; // Enable incremental backups
        
        rocksdb::BackupEngine* backup_engine_ptr = nullptr;
        rocksdb::Status s = rocksdb::BackupEngine::Open(
            rocksdb::Env::Default(),
            backup_opts,
            &backup_engine_ptr
        );
        
        if (!s.ok()) {
            THEMIS_ERROR("Failed to open BackupEngine: {}", s.ToString());
            return false;
        }
        
        std::unique_ptr<rocksdb::BackupEngine> backup_engine(backup_engine_ptr);
        
        // Create incremental backup (only delta since last backup)
        s = backup_engine->CreateNewBackup(base_db, flush_before_backup);
        
        if (!s.ok()) {
            THEMIS_ERROR("Failed to create incremental backup: {}", s.ToString());
            return false;
        }
        
        THEMIS_INFO("Created incremental backup in '{}' (flush={})", backup_dir, flush_before_backup);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("createIncrementalBackup exception: {}", e.what());
        return false;
    }
}

bool RocksDBWrapper::restoreFromBackup(const std::string& backup_dir) {
    try {
        rocksdb::BackupEngineOptions backup_opts(backup_dir);
        rocksdb::BackupEngine* backup_engine_ptr = nullptr;
        rocksdb::Status s = rocksdb::BackupEngine::Open(
            rocksdb::Env::Default(),
            backup_opts,
            &backup_engine_ptr
        );
        
        if (!s.ok()) {
            THEMIS_ERROR("Failed to open BackupEngine for restore: {}", s.ToString());
            return false;
        }
        
        std::unique_ptr<rocksdb::BackupEngine> backup_engine(backup_engine_ptr);
        
        // Close current DB before restore
        if (db_) {
            close();
        }
        
        // Restore from latest backup
        s = backup_engine->RestoreDBFromLatestBackup(config_.db_path, config_.db_path);
        
        if (!s.ok()) {
            THEMIS_ERROR("Failed to restore from backup: {}", s.ToString());
            return false;
        }
        
        // Reopen DB
        if (!open()) {
            THEMIS_ERROR("Failed to reopen DB after restore from backup '{}'", backup_dir);
            return false;
        }
        
        THEMIS_INFO("Restored DB from backup '{}' to '{}'", backup_dir, config_.db_path);
        return true;
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("restoreFromBackup exception: {}", e.what());
        return false;
    }
}

uint32_t RocksDBWrapper::getBackupCount(const std::string& backup_dir) const {
    try {
        rocksdb::BackupEngineOptions backup_opts(backup_dir);
        rocksdb::BackupEngine* backup_engine_ptr = nullptr;
        rocksdb::Status s = rocksdb::BackupEngine::Open(
            rocksdb::Env::Default(),
            backup_opts,
            &backup_engine_ptr
        );
        
        if (!s.ok()) {
            return 0;
        }
        
        std::unique_ptr<rocksdb::BackupEngine> backup_engine(backup_engine_ptr);
        std::vector<rocksdb::BackupInfo> backup_info;
        backup_engine->GetBackupInfo(&backup_info);
        
        return static_cast<uint32_t>(backup_info.size());
        
    } catch (...) {
        return 0;
    }
}

std::string RocksDBWrapper::exportStatisticsJSON() const {
    if (!db_ || !options_->statistics) {
        return "{}";
    }
    
    try {
        json stats_obj;
        
        // Export key RocksDB statistics
        auto stats = options_->statistics;
        
        stats_obj["bytes_written"] = stats->getTickerCount(rocksdb::BYTES_WRITTEN);
        stats_obj["bytes_read"] = stats->getTickerCount(rocksdb::BYTES_READ);
        stats_obj["number_keys_written"] = stats->getTickerCount(rocksdb::NUMBER_KEYS_WRITTEN);
        stats_obj["number_keys_read"] = stats->getTickerCount(rocksdb::NUMBER_KEYS_READ);
        stats_obj["number_keys_updated"] = stats->getTickerCount(rocksdb::NUMBER_KEYS_UPDATED);
        stats_obj["block_cache_miss"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_MISS);
        stats_obj["block_cache_hit"] = stats->getTickerCount(rocksdb::BLOCK_CACHE_HIT);
        stats_obj["bloom_filter_useful"] = stats->getTickerCount(rocksdb::BLOOM_FILTER_USEFUL);
        stats_obj["memtable_hit"] = stats->getTickerCount(rocksdb::MEMTABLE_HIT);
        stats_obj["memtable_miss"] = stats->getTickerCount(rocksdb::MEMTABLE_MISS);
        stats_obj["compaction_key_drop_obsolete"] = stats->getTickerCount(rocksdb::COMPACTION_KEY_DROP_OBSOLETE);
        stats_obj["wal_file_synced"] = stats->getTickerCount(rocksdb::WAL_FILE_SYNCED);
        stats_obj["stall_micros"] = stats->getTickerCount(rocksdb::STALL_MICROS);
        
        return stats_obj.dump();
        
    } catch (const std::exception& e) {
        THEMIS_ERROR("exportStatisticsJSON exception: {}", e.what());
        return "{}";
    }
}

uint64_t RocksDBWrapper::getStatistic(const std::string& ticker_name) const {
    if (!db_ || !options_->statistics) {
        return 0;
    }
    
    // Map ticker names to RocksDB ticker types
    static const std::unordered_map<std::string, rocksdb::Tickers> ticker_map = {
        {"BYTES_WRITTEN", rocksdb::BYTES_WRITTEN},
        {"BYTES_READ", rocksdb::BYTES_READ},
        {"NUMBER_KEYS_WRITTEN", rocksdb::NUMBER_KEYS_WRITTEN},
        {"NUMBER_KEYS_READ", rocksdb::NUMBER_KEYS_READ},
        {"NUMBER_KEYS_UPDATED", rocksdb::NUMBER_KEYS_UPDATED},
        {"BLOCK_CACHE_MISS", rocksdb::BLOCK_CACHE_MISS},
        {"BLOCK_CACHE_HIT", rocksdb::BLOCK_CACHE_HIT},
        {"BLOOM_FILTER_USEFUL", rocksdb::BLOOM_FILTER_USEFUL},
        {"MEMTABLE_HIT", rocksdb::MEMTABLE_HIT},
        {"MEMTABLE_MISS", rocksdb::MEMTABLE_MISS},
        {"WAL_FILE_SYNCED", rocksdb::WAL_FILE_SYNCED},
        {"STALL_MICROS", rocksdb::STALL_MICROS}
    };
    
    auto it = ticker_map.find(ticker_name);
    if (it != ticker_map.end()) {
        return options_->statistics->getTickerCount(it->second);
    }
    
    return 0;
}

// v1.3.0 Phase 2: Async I/O MultiScan Implementation

std::vector<std::pair<std::string, std::vector<uint8_t>>> RocksDBWrapper::scanWithAsyncIO(
    std::string_view prefix, int limit) {
    
    std::vector<std::pair<std::string, std::vector<uint8_t>>> results;
    
    if (!db_) {
        THEMIS_ERROR("scanWithAsyncIO: database not open");
        return results;
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("scanWithAsyncIO: base DB is null");
        return results;
    }
    
    // Configure read options with async I/O if enabled
    rocksdb::ReadOptions read_opts;
    if (config_.enable_async_io) {
        read_opts.async_io = true;
        read_opts.readahead_size = config_.async_io_readahead_size_mb * 1024 * 1024;
    }
    
    // Create iterator
    std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
    if (!it) {
        THEMIS_ERROR("scanWithAsyncIO: failed to create iterator");
        return results;
    }
    
    // Seek to prefix or start of database
    if (prefix.empty()) {
        it->SeekToFirst();
    } else {
        it->Seek(prefix);
    }
    
    // Collect results up to limit
    int count = 0;
    while (it->Valid() && count < limit) {
        std::string key = it->key().ToString();
        
        // Check prefix match
        if (!prefix.empty() && !key.starts_with(prefix)) {
            break;
        }
        
        std::vector<uint8_t> value(it->value().data(), 
                                   it->value().data() + it->value().size());
        results.emplace_back(std::move(key), std::move(value));
        
        it->Next();
        count++;
    }
    
    if (!it->status().ok()) {
        THEMIS_ERROR("scanWithAsyncIO iterator error: {}", it->status().ToString());
    }
    
    return results;
}

std::vector<std::pair<std::string, std::vector<uint8_t>>> RocksDBWrapper::rangeQueryWithAsyncIO(
    std::string_view start_key, std::string_view end_key) {
    
    std::vector<std::pair<std::string, std::vector<uint8_t>>> results;
    
    if (!db_) {
        THEMIS_ERROR("rangeQueryWithAsyncIO: database not open");
        return results;
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("rangeQueryWithAsyncIO: base DB is null");
        return results;
    }
    
    // Configure read options with async I/O if enabled
    rocksdb::ReadOptions read_opts;
    if (config_.enable_async_io) {
        read_opts.async_io = true;
        read_opts.readahead_size = config_.async_io_readahead_size_mb * 1024 * 1024;
    }
    
    // Create iterator
    std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
    if (!it) {
        THEMIS_ERROR("rangeQueryWithAsyncIO: failed to create iterator");
        return results;
    }
    
    // Seek to start key
    it->Seek(start_key);
    
    // Collect results in range
    while (it->Valid()) {
        std::string key = it->key().ToString();
        
        // Check if we've exceeded end_key
        if (key > end_key) {
            break;
        }
        
        std::vector<uint8_t> value(it->value().data(), 
                                   it->value().data() + it->value().size());
        results.emplace_back(std::move(key), std::move(value));
        
        it->Next();
    }
    
    if (!it->status().ok()) {
        THEMIS_ERROR("rangeQueryWithAsyncIO iterator error: {}", it->status().ToString());
    }
    
    return results;
}

std::vector<std::pair<std::string, std::vector<uint8_t>>> RocksDBWrapper::reverseScanWithAsyncIO(
    std::string_view start_key, int limit) {
    
    std::vector<std::pair<std::string, std::vector<uint8_t>>> results;
    
    if (!db_) {
        THEMIS_ERROR("reverseScanWithAsyncIO: database not open");
        return results;
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("reverseScanWithAsyncIO: base DB is null");
        return results;
    }
    
    // Configure read options with async I/O if enabled
    rocksdb::ReadOptions read_opts;
    if (config_.enable_async_io) {
        read_opts.async_io = true;
        read_opts.readahead_size = config_.async_io_readahead_size_mb * 1024 * 1024;
    }
    
    // Create iterator
    std::unique_ptr<rocksdb::Iterator> it(base_db->NewIterator(read_opts));
    if (!it) {
        THEMIS_ERROR("reverseScanWithAsyncIO: failed to create iterator");
        return results;
    }
    
    // Seek to start key or last if empty
    if (start_key.empty()) {
        it->SeekToLast();
    } else {
        it->Seek(start_key);
        if (!it->Valid()) {
            it->SeekToLast();
        }
    }
    
    // Collect results in reverse order up to limit
    int count = 0;
    while (it->Valid() && count < limit) {
        std::string key = it->key().ToString();
        std::vector<uint8_t> value(it->value().data(), 
                                   it->value().data() + it->value().size());
        results.emplace_back(std::move(key), std::move(value));
        
        it->Prev();
        count++;
    }
    
    if (!it->status().ok()) {
        THEMIS_ERROR("reverseScanWithAsyncIO iterator error: {}", it->status().ToString());
    }
    
    return results;
}

std::vector<std::optional<std::vector<uint8_t>>> RocksDBWrapper::multiGetWithAsyncIO(
    const std::vector<std::string>& keys) {
    
    std::vector<std::optional<std::vector<uint8_t>>> results;
    
    if (!db_) {
        THEMIS_ERROR("multiGetWithAsyncIO: database not open");
        return results;
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        THEMIS_ERROR("multiGetWithAsyncIO: base DB is null");
        return results;
    }
    
    // Configure read options with async I/O if enabled
    rocksdb::ReadOptions read_opts;
    if (config_.enable_async_io) {
        read_opts.async_io = true;
        read_opts.readahead_size = config_.async_io_readahead_size_mb * 1024 * 1024;
    }
    
    // Prepare keys for RocksDB MultiGet
    std::vector<rocksdb::Slice> rock_keys;
    rock_keys.reserve(keys.size());
    for (const auto& key : keys) {
        rock_keys.emplace_back(key);
    }
    
    // Perform MultiGet
    std::vector<std::string> values;
    std::vector<rocksdb::Status> statuses = base_db->MultiGet(read_opts, rock_keys, &values);
    
    // Process results
    results.reserve(keys.size());
    for (size_t i = 0; i < keys.size(); ++i) {
        if (statuses[i].ok()) {
            std::vector<uint8_t> value(values[i].begin(), values[i].end());
            results.emplace_back(std::move(value));
        } else if (statuses[i].IsNotFound()) {
            results.emplace_back(std::nullopt);
        } else {
            THEMIS_ERROR("multiGetWithAsyncIO error for key {}: {}", 
                        keys[i], statuses[i].ToString());
            results.emplace_back(std::nullopt);
        }
    }
    
    return results;
}

Result<std::unique_ptr<rocksdb::Iterator>> RocksDBWrapper::newAsyncIterator() {
    if (!db_) {
        return Err<std::unique_ptr<rocksdb::Iterator>>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB not opened for async iterator"
        );
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        return Err<std::unique_ptr<rocksdb::Iterator>>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB base DB is null"
        );
    }
    
    // Configure read options with async I/O if enabled
    rocksdb::ReadOptions read_opts;
    if (config_.enable_async_io) {
        read_opts.async_io = true;
        read_opts.readahead_size = config_.async_io_readahead_size_mb * 1024 * 1024;
    }
    
    auto it = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(read_opts));
    if (!it) {
        return Err<std::unique_ptr<rocksdb::Iterator>>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "Failed to create async iterator"
        );
    }
    
    return Ok(std::move(it));
}

Result<std::unique_ptr<rocksdb::Iterator>> RocksDBWrapper::newIterator() {
    if (!db_) {
        return Err<std::unique_ptr<rocksdb::Iterator>>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB not opened for iterator"
        );
    }

    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        return Err<std::unique_ptr<rocksdb::Iterator>>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB base DB is null"
        );
    }

    rocksdb::ReadOptions read_opts;
    auto it = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(read_opts));
    if (!it) {
        return Err<std::unique_ptr<rocksdb::Iterator>>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "Failed to create iterator"
        );
    }
    
    return Ok(std::move(it));
}

// SafeIterator implementation - SOLUTION 1B for iterator lifecycle safety
Result<RocksDBWrapper::SafeIterator> RocksDBWrapper::newSafeIterator(const rocksdb::ReadOptions* read_options) {
    // Create operation guard first to extend database lifetime
    auto guard = std::make_unique<OperationGuard>(this);
    
    if (!guard || !*guard) {
        // Database not open - return error
        return Err<SafeIterator>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB not opened for safe iterator"
        );
    }
    
    // Use provided read options or default
    const rocksdb::ReadOptions* opts = read_options ? read_options : read_options_.get();
    
    // Create iterator while holding guard
    auto* base_db = db_->GetBaseDB();
    if (!base_db) {
        return Err<SafeIterator>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "RocksDB base DB is null"
        );
    }
    
    auto iter = std::unique_ptr<rocksdb::Iterator>(base_db->NewIterator(*opts));
    if (!iter) {
        return Err<SafeIterator>(
            errors::ErrorCode::ERR_INDEX_NOT_INITIALIZED,
            "Failed to create safe iterator"
        );
    }
    
    return Ok(SafeIterator(std::move(iter), std::move(guard)));
}

// SafeIterator method implementations
void RocksDBWrapper::SafeIterator::Seek(const std::string& target) {
    if (iterator_) iterator_->Seek(target);
}

void RocksDBWrapper::SafeIterator::SeekToFirst() {
    if (iterator_) iterator_->SeekToFirst();
}

void RocksDBWrapper::SafeIterator::SeekToLast() {
    if (iterator_) iterator_->SeekToLast();
}

void RocksDBWrapper::SafeIterator::Next() {
    if (iterator_) iterator_->Next();
}

void RocksDBWrapper::SafeIterator::Prev() {
    if (iterator_) iterator_->Prev();
}

bool RocksDBWrapper::SafeIterator::Valid() const {
    return iterator_ && iterator_->Valid();
}

std::string_view RocksDBWrapper::SafeIterator::key() const {
    if (!iterator_) return std::string_view();
    auto s = iterator_->key();
    return std::string_view(s.data(), s.size());
}

std::string_view RocksDBWrapper::SafeIterator::value() const {
    if (!iterator_) return std::string_view();
    auto s = iterator_->value();
    return std::string_view(s.data(), s.size());
}

} // namespace themis
