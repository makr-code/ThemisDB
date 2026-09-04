/**
 * @file storage_profiler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=9, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "observability/storage_profiler.h"
#include <fstream>
#include <algorithm>
#include <mutex>
#include <numeric>
#include <optional>

namespace themis {
namespace observability {

// Helper functions
const char* to_string(StorageOpType type) {
    switch (type) {
        case StorageOpType::GET: return "GET";
        case StorageOpType::PUT: return "PUT";
        case StorageOpType::DELETE: return "DELETE";
        case StorageOpType::SCAN: return "SCAN";
        case StorageOpType::BATCH_WRITE: return "BATCH_WRITE";
        case StorageOpType::COMPACT: return "COMPACT";
        case StorageOpType::FLUSH: return "FLUSH";
        case StorageOpType::ITERATOR_SEEK: return "ITERATOR_SEEK";
        case StorageOpType::ITERATOR_NEXT: return "ITERATOR_NEXT";
        default: return "UNKNOWN";
    }
}

json StorageOpStats::toJSON() const {
    return json{
        {"type", to_string(type)},
        {"duration_us", duration.count()},
        {"bytes_read", bytes_read},
        {"bytes_written", bytes_written},
        {"keys_processed", keys_processed},
        {"cache_hit", cache_hit},
        {"from_sst", from_sst},
        {"from_memtable", from_memtable},
        {"sst_reads", sst_reads},
        {"column_family", column_family}
    };
}

json RocksDBStats::toJSON() const {
    auto time_t_val = std::chrono::system_clock::to_time_t(timestamp);
    
    json level_sizes_json = json::array();
    for (auto size : level_sizes) {
        level_sizes_json.push_back(size);
    }
    
    return json{
        {"timestamp", time_t_val},
        {"compaction", {
            {"num_compactions", num_compactions},
            {"bytes_read", compaction_bytes_read},
            {"bytes_written", compaction_bytes_written},
            {"total_time_us", total_compaction_time.count()}
        }},
        {"writes", {
            {"num_writes", num_writes},
            {"bytes_written", bytes_written},
            {"wal_bytes", wal_bytes},
            {"memtable_writes", memtable_writes}
        }},
        {"reads", {
            {"num_reads", num_reads},
            {"bytes_read", bytes_read},
            {"block_cache_hits", block_cache_hits},
            {"block_cache_misses", block_cache_misses},
            {"bloom_filter_hits", bloom_filter_hits},
            {"bloom_filter_misses", bloom_filter_misses}
        }},
        {"sst", {
            {"num_files", num_sst_files},
            {"total_size_bytes", total_sst_size_bytes},
            {"num_levels", num_levels},
            {"level_sizes", level_sizes_json}
        }},
        {"memory", {
            {"memtable_bytes", memtable_size_bytes},
            {"block_cache_bytes", block_cache_size_bytes},
            {"block_cache_capacity", block_cache_capacity},
            {"cache_usage_pct", block_cache_capacity > 0 ?
                (100.0 * block_cache_size_bytes / block_cache_capacity) : 0.0}
        }},
        {"wal", {
            {"size_bytes", wal_size_bytes},
            {"syncs", wal_syncs}
        }},
        {"amplification", {
            {"write", write_amplification},
            {"read", read_amplification},
            {"space", space_amplification}
        }}
    };
}

// StorageProfiler::Impl
/** @brief StorageProfiler::Impl. */
class StorageProfiler::Impl {
public:
    StorageProfilerConfig config;
    std::vector<StorageOpStats> operations;
    std::vector<RocksDBStats> rocksdb_stats_history;
    mutable std::mutex mutex;
    
    explicit Impl(const StorageProfilerConfig& cfg) : config(cfg) {}
    
    void cleanup_old() {
        // Remove old operations
        if (static_cast<int>(operations.size()) > config.max_ops_retained) {
            size_t to_remove = operations.size() - config.max_ops_retained;
            operations.erase(operations.begin(), operations.begin() + to_remove);
        }
        
        // Remove old RocksDB stats
        auto now = std::chrono::system_clock::now();
        rocksdb_stats_history.erase(
            std::remove_if(rocksdb_stats_history.begin(), rocksdb_stats_history.end(),
                [&]([[maybe_unused]] const RocksDBStats& stats) {
                    auto age = std::chrono::duration_cast<std::chrono::seconds>(
                        now - stats.timestamp);
                    return age > config.retention_duration;
                }),
            rocksdb_stats_history.end());
    }
};

// StorageProfiler implementation
StorageProfiler::StorageProfiler(const StorageProfilerConfig& config)
    : impl_(std::make_unique<Impl>(config)) {}

StorageProfiler::~StorageProfiler() = default;

void StorageProfiler::record_operation(const StorageOpStats& stats) {
    if (!impl_->config.enabled || !impl_->config.collect_op_stats) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    impl_->operations.push_back(stats);
    
    if (impl_->config.log_slow_ops && 
        stats.duration >= std::chrono::duration_cast<std::chrono::microseconds>(
            impl_->config.slow_op_threshold)) {
        log_slow_operation(stats);
    }
    
    cleanup_old_data();
}

RocksDBStats StorageProfiler::collect_rocksdb_stats(const std::string& db_path) {
    (void)db_path;
    if (!impl_->config.enabled || !impl_->config.collect_rocksdb_stats) {
        return RocksDBStats{};
    }
    
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    RocksDBStats stats;
    stats.timestamp = std::chrono::system_clock::now();
    
    // Note: In real implementation, this would query RocksDB Statistics
    // For now, this is a placeholder that would be integrated with RocksDBWrapper
    
    impl_->rocksdb_stats_history.push_back(stats);
    cleanup_old_data();
    
    return stats;
}

std::vector<StorageOpStats> StorageProfiler::get_operations(
    std::optional<StorageOpType> type) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (!type.has_value()) {
        return impl_->operations;
    }
    
    std::vector<StorageOpStats> result;
    std::copy_if(impl_->operations.begin(), impl_->operations.end(),
                std::back_inserter(result),
                [type](const StorageOpStats& op) { return op.type == type.value(); });
    return result;
}

std::vector<StorageOpStats> StorageProfiler::get_slow_operations(
    std::chrono::milliseconds threshold) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    auto threshold_us = std::chrono::duration_cast<std::chrono::microseconds>(threshold);
    
    std::vector<StorageOpStats> result;
    std::copy_if(impl_->operations.begin(), impl_->operations.end(),
                std::back_inserter(result),
                [threshold_us](const StorageOpStats& op) { 
                    return op.duration >= threshold_us; 
                });
    
    std::sort(result.begin(), result.end(),
             [](const StorageOpStats& a, const StorageOpStats& b) {
                 return a.duration > b.duration;
             });
    
    return result;
}

std::vector<RocksDBStats> StorageProfiler::get_rocksdb_stats_history() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->rocksdb_stats_history;
}

std::optional<RocksDBStats> StorageProfiler::get_latest_rocksdb_stats() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->rocksdb_stats_history.empty()) {
        return std::nullopt;
    }
    
    return impl_->rocksdb_stats_history.back();
}

json StorageProfiler::get_operation_summary() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    std::unordered_map<StorageOpType, size_t> op_counts;
    std::unordered_map<StorageOpType, std::chrono::microseconds> op_durations;
    std::unordered_map<StorageOpType, size_t> op_bytes_read;
    std::unordered_map<StorageOpType, size_t> op_bytes_written;
    
    for (const auto& op : impl_->operations) {
        op_counts[op.type]++;
        op_durations[op.type] += op.duration;
        op_bytes_read[op.type] += op.bytes_read;
        op_bytes_written[op.type] += op.bytes_written;
    }
    
    json summary = json::object();
    for (const auto& [type, count] : op_counts) {
        double avg_duration = count > 0 ? 
            static_cast<double>(op_durations[type].count()) / count : 0.0;
        
        summary[to_string(type)] = json{
            {"count", count},
            {"avg_duration_us", avg_duration},
            {"total_duration_us", op_durations[type].count()},
            {"total_bytes_read", op_bytes_read[type]},
            {"total_bytes_written", op_bytes_written[type]}
        };
    }
    
    return summary;
}

json StorageProfiler::get_amplification_metrics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    if (impl_->rocksdb_stats_history.empty()) {
        return json::object();
    }
    
    const auto& latest = impl_->rocksdb_stats_history.back();
    
    return json{
        {"write_amplification", latest.write_amplification},
        {"read_amplification", latest.read_amplification},
        {"space_amplification", latest.space_amplification}
    };
}

json StorageProfiler::get_cache_metrics() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    size_t cache_hits = 0;
    size_t cache_misses = 0;
    
    for (const auto& op : impl_->operations) {
        if (op.cache_hit) {
            cache_hits++;
        } else {
            cache_misses++;
        }
    }
    
    size_t total = cache_hits + cache_misses;
    double hit_rate = total > 0 ? (100.0 * cache_hits / total) : 0.0;
    
    json result = json{
        {"operation_cache", {
            {"hits", cache_hits},
            {"misses", cache_misses},
            {"hit_rate_pct", hit_rate}
        }}
    };
    
    if (!impl_->rocksdb_stats_history.empty()) {
        const auto& latest = impl_->rocksdb_stats_history.back();
        size_t block_total = latest.block_cache_hits + latest.block_cache_misses;
        double block_hit_rate = block_total > 0 ? 
            (100.0 * latest.block_cache_hits / block_total) : 0.0;
        
        result["block_cache"] = json{
            {"hits", latest.block_cache_hits},
            {"misses", latest.block_cache_misses},
            {"hit_rate_pct", block_hit_rate},
            {"size_bytes", latest.block_cache_size_bytes},
            {"capacity_bytes", latest.block_cache_capacity}
        };
    }
    
    return result;
}

void StorageProfiler::clear() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->operations.clear();
    impl_->rocksdb_stats_history.clear();
}

void StorageProfiler::export_to_json(const std::string& filename) const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    
    json ops_json = json::array();
    for (const auto& op : impl_->operations) {
        ops_json.push_back(op.toJSON());
    }
    
    json stats_json = json::array();
    for (const auto& stats : impl_->rocksdb_stats_history) {
        stats_json.push_back(stats.toJSON());
    }
    
    json export_data = json{
        {"operations", ops_json},
        {"rocksdb_stats", stats_json},
        {"summary", get_operation_summary()},
        {"cache_metrics", get_cache_metrics()},
        {"amplification_metrics", get_amplification_metrics()}
    };
    
    std::ofstream file(filename);
    file << export_data.dump(2);
}

StorageProfilerConfig StorageProfiler::get_config() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config;
}

void StorageProfiler::set_config(const StorageProfilerConfig& config) {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config = config;
}

void StorageProfiler::enable() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.enabled = true;
}

void StorageProfiler::disable() {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    impl_->config.enabled = false;
}

bool StorageProfiler::is_enabled() const {
    std::lock_guard<std::mutex> lock(impl_->mutex);
    return impl_->config.enabled;
}

void StorageProfiler::cleanup_old_data() {
    impl_->cleanup_old();
}

void StorageProfiler::log_slow_operation(const StorageOpStats& stats) {
    (void)stats;
    // Log slow operation (could be integrated with logging system)
    // For now, just a placeholder
}

// ScopedStorageOp implementation
ScopedStorageOp::ScopedStorageOp(StorageProfiler& profiler, StorageOpType type,
                               const std::string& column_family)
    : profiler_(profiler), start_(std::chrono::high_resolution_clock::now()) {
    stats_.type = type;
    stats_.column_family = column_family;
}

ScopedStorageOp::~ScopedStorageOp() {
    auto end = std::chrono::high_resolution_clock::now();
    stats_.duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start_);
    profiler_.record_operation(stats_);
}

void ScopedStorageOp::record_bytes_read([[maybe_unused]] size_t bytes) {
    stats_.bytes_read += bytes;
}

void ScopedStorageOp::record_bytes_written([[maybe_unused]] size_t bytes) {
    stats_.bytes_written += bytes;
}

void ScopedStorageOp::record_keys([[maybe_unused]] size_t count) {
    stats_.keys_processed += count;
}

void ScopedStorageOp::set_cache_hit([[maybe_unused]] bool hit) {
    stats_.cache_hit = hit;
}

void ScopedStorageOp::set_from_sst([[maybe_unused]] bool from_sst) {
    stats_.from_sst = from_sst;
}

void ScopedStorageOp::set_from_memtable([[maybe_unused]] bool from_memtable) {
    stats_.from_memtable = from_memtable;
}

void ScopedStorageOp::record_sst_read() {
    stats_.sst_reads++;
}

} // namespace observability
} // namespace themis
