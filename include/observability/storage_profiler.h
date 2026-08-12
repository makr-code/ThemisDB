/**
 * @file storage_profiler.h
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
#include <chrono>
#include <memory>
#include <unordered_map>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace observability {

using json = nlohmann::json;

#ifdef DELETE
#undef DELETE
#endif

/**
 * @brief Storage operation type
 */
enum class StorageOpType {
    GET,
    PUT,
    DELETE,
    SCAN,
    BATCH_WRITE,
    COMPACT,
    FLUSH,
    ITERATOR_SEEK,
    ITERATOR_NEXT
};

/**
 * @brief Storage operation statistics
 */
struct StorageOpStats {
    StorageOpType type;
    std::chrono::microseconds duration{0};
    size_t bytes_read = 0;
    size_t bytes_written = 0;
    size_t keys_processed = 0;
    bool cache_hit = false;
    bool from_sst = false;
    bool from_memtable = false;
    size_t sst_reads = 0;
    std::string column_family;
    
    json toJSON() const;
};

/**
 * @brief RocksDB statistics snapshot
 */
struct RocksDBStats {
    std::chrono::system_clock::time_point timestamp;
    
    // Compaction stats
    size_t num_compactions = 0;
    size_t compaction_bytes_read = 0;
    size_t compaction_bytes_written = 0;
    std::chrono::microseconds total_compaction_time{0};
    
    // Write stats
    size_t num_writes = 0;
    size_t bytes_written = 0;
    size_t wal_bytes = 0;
    size_t memtable_writes = 0;
    
    // Read stats
    size_t num_reads = 0;
    size_t bytes_read = 0;
    size_t block_cache_hits = 0;
    size_t block_cache_misses = 0;
    size_t bloom_filter_hits = 0;
    size_t bloom_filter_misses = 0;
    
    // SST stats
    size_t num_sst_files = 0;
    size_t total_sst_size_bytes = 0;
    size_t num_levels = 0;
    std::vector<size_t> level_sizes;
    
    // Memory stats
    size_t memtable_size_bytes = 0;
    size_t block_cache_size_bytes = 0;
    size_t block_cache_capacity = 0;
    
    // WAL stats
    size_t wal_size_bytes = 0;
    size_t wal_syncs = 0;
    
    // Performance metrics
    double write_amplification = 0.0;
    double read_amplification = 0.0;
    double space_amplification = 0.0;
    
    json toJSON() const;
};

/**
 * @brief Storage profiler configuration
 */
struct StorageProfilerConfig {
    bool enabled = true;
    bool collect_op_stats = true;
    bool collect_rocksdb_stats = true;
    size_t max_ops_retained = 10000;
    std::chrono::seconds stats_collection_interval{60};
    std::chrono::seconds retention_duration{3600};
    bool log_slow_ops = true;
    std::chrono::milliseconds slow_op_threshold{100};
};

/**
 * @brief Storage profiler for RocksDB operations
 * 
 * Profiles storage layer operations, collecting timing,
 * I/O statistics, and RocksDB internal metrics.
 */
class StorageProfiler {
public:
    explicit StorageProfiler(const StorageProfilerConfig& config = StorageProfilerConfig{});
    ~StorageProfiler();
    
    // Disable copy
    StorageProfiler(const StorageProfiler&) = delete;
    StorageProfiler& operator=(const StorageProfiler&) = delete;
    
    /**
     * @brief Record storage operation
     * @param stats Operation statistics
     */
    void record_operation(const StorageOpStats& stats);
    
    /**
     * @brief Collect RocksDB statistics snapshot
     * @param db_path Database path for identification
     * @return Statistics snapshot
     */
    RocksDBStats collect_rocksdb_stats(const std::string& db_path);
    
    /**
     * @brief Get operation statistics
     * @param type Operation type (optional filter)
     * @return Vector of operation statistics
     */
    std::vector<StorageOpStats> get_operations(
        std::optional<StorageOpType> type = std::nullopt) const;
    
    /**
     * @brief Get slow operations
     * @param threshold Slow operation threshold
     * @return Vector of slow operations
     */
    std::vector<StorageOpStats> get_slow_operations(
        std::chrono::milliseconds threshold) const;
    
    /**
     * @brief Get RocksDB statistics history
     * @return Vector of statistics snapshots
     */
    std::vector<RocksDBStats> get_rocksdb_stats_history() const;
    
    /**
     * @brief Get latest RocksDB statistics
     * @return Latest statistics snapshot
     */
    std::optional<RocksDBStats> get_latest_rocksdb_stats() const;
    
    /**
     * @brief Get operation summary
     * @return JSON summary of operations
     */
    json get_operation_summary() const;
    
    /**
     * @brief Get amplification metrics
     * @return JSON with amplification metrics
     */
    json get_amplification_metrics() const;
    
    /**
     * @brief Get cache performance metrics
     * @return JSON with cache metrics
     */
    json get_cache_metrics() const;
    
    /**
     * @brief Clear all statistics
     */
    void clear();
    
    /**
     * @brief Export statistics to JSON
     * @param filename Output file path
     */
    void export_to_json(const std::string& filename) const;
    
    /**
     * @brief Get configuration
     */
    StorageProfilerConfig get_config() const;
    
    /**
     * @brief Set configuration
     */
    void set_config(const StorageProfilerConfig& config);
    
    /**
     * @brief Enable profiling
     */
    void enable();
    
    /**
     * @brief Disable profiling
     */
    void disable();
    
    /**
     * @brief Check if profiling is enabled
     */
    bool is_enabled() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
    
    void cleanup_old_data();
    void log_slow_operation(const StorageOpStats& stats);
};

/**
 * @brief RAII helper for storage operation profiling
 */
class ScopedStorageOp {
public:
    ScopedStorageOp(StorageProfiler& profiler, StorageOpType type,
                   const std::string& column_family = "");
    ~ScopedStorageOp();
    
    // Disable copy
    ScopedStorageOp(const ScopedStorageOp&) = delete;
    ScopedStorageOp& operator=(const ScopedStorageOp&) = delete;
    
    void record_bytes_read(size_t bytes);
    void record_bytes_written(size_t bytes);
    void record_keys(size_t count);
    void set_cache_hit(bool hit);
    void set_from_sst(bool from_sst);
    void set_from_memtable(bool from_memtable);
    void record_sst_read();
    
private:
    StorageProfiler& profiler_;
    StorageOpStats stats_;
    std::chrono::high_resolution_clock::time_point start_;
};

// Helper functions
const char* to_string(StorageOpType type);

} // namespace observability
} // namespace themis
