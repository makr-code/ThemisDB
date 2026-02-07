#include "storage/batch_write_optimizer.h"
#include "utils/logger.h"
#include <rocksdb/options.h>

namespace themis {

BatchWriteOptimizer::BatchWriteOptimizer(const Config& config)
    : config_(config)
{
    validateConfig(config_);
}

BatchWriteOptimizer::~BatchWriteOptimizer() = default;

rocksdb::WriteOptions BatchWriteOptimizer::getOptimizedWriteOptions(
    size_t estimated_batch_size
) const {
    rocksdb::WriteOptions opts;
    
    // Configure WAL syncing based on durability mode
    switch (config_.durability) {
        case DurabilityMode::Sync:
            opts.sync = true;
            opts.disableWAL = false;
            break;
            
        case DurabilityMode::Async:
            opts.sync = false;  // OS will batch fsync calls
            opts.disableWAL = false;
            break;
            
        case DurabilityMode::NoSync:
            opts.sync = false;
            opts.disableWAL = false;  // Keep WAL for recovery
            THEMIS_WARN("BatchWriteOptimizer: NoSync mode active - risk of data loss!");
            break;
    }
    
    // Disable WAL entirely if configured (maximum speed, no durability)
    if (config_.disable_wal) {
        opts.disableWAL = true;
        THEMIS_WARN("BatchWriteOptimizer: WAL disabled - data cannot be recovered!");
    }
    
    // Low-priority writes don't block normal operations
    opts.low_pri = false;  // Set to true for background bulk loads
    
    // Don't wait for write buffer flushes (improves throughput)
    opts.no_slowdown = false;  // Set to true to fail fast instead of waiting
    
    // Ignore missing column families (useful for migrations)
    opts.ignore_missing_column_families = false;
    
    return opts;
}

BatchWriteOptimizer::Stats BatchWriteOptimizer::getStats() const {
    Stats stats;
    stats.total_batches_written = total_batches_.load();
    stats.total_items_written = total_items_.load();
    stats.total_write_time_ms = total_latency_ms_.load();
    
    if (stats.total_batches_written > 0) {
        stats.avg_batch_size = static_cast<double>(stats.total_items_written) / 
                                static_cast<double>(stats.total_batches_written);
        stats.avg_write_latency_ms = stats.total_write_time_ms / 
                                      static_cast<double>(stats.total_batches_written);
    }
    
    if (stats.total_write_time_ms > 0) {
        // Convert ms to seconds for throughput calculation
        stats.throughput_items_per_sec = (static_cast<double>(stats.total_items_written) /
                                           stats.total_write_time_ms) * 1000.0;
    }
    
    return stats;
}

void BatchWriteOptimizer::recordBatchWrite(size_t items, double latency_ms) {
    total_batches_.fetch_add(1);
    total_items_.fetch_add(items);
    
    // Atomic double add (not perfect but good enough for statistics)
    double old_val = total_latency_ms_.load();
    while (!total_latency_ms_.compare_exchange_weak(old_val, old_val + latency_ms)) {
        // Retry on failure
    }
}

BatchWriteOptimizer::Config BatchWriteOptimizer::recommendedConfigForUseCase(
    const std::string& use_case
) {
    Config config;
    
    if (use_case == "production") {
        config.durability = DurabilityMode::Async;
        config.disable_wal = false;
        config.allow_concurrent_memtable_write = true;
        
    } else if (use_case == "bulk_load") {
        config.durability = DurabilityMode::NoSync;
        config.disable_wal = false;  // Keep WAL for safety
        config.allow_concurrent_memtable_write = true;
        config.sequential_write_hint = true;
        
    } else if (use_case == "benchmark") {
        config.durability = DurabilityMode::NoSync;
        config.disable_wal = true;  // Maximum speed
        config.allow_concurrent_memtable_write = true;
        
    } else if (use_case == "critical") {
        config.durability = DurabilityMode::Sync;
        config.disable_wal = false;
        config.allow_concurrent_memtable_write = true;
        
    } else {
        // Default: production-safe
        config.durability = DurabilityMode::Async;
    }
    
    return config;
}

void BatchWriteOptimizer::validateConfig(const Config& config) {
    if (config.disable_wal) {
        THEMIS_WARN("⚠️  BatchWriteOptimizer: WAL disabled!");
        THEMIS_WARN("    Data will be lost on crash/power failure");
        THEMIS_WARN("    Only use for bulk loads or benchmarks");
    }
    
    if (config.durability == DurabilityMode::NoSync && !config.disable_wal) {
        THEMIS_WARN("⚠️  BatchWriteOptimizer: NoSync mode active");
        THEMIS_WARN("    WAL writes not fsynced - data may be lost on crash");
        THEMIS_WARN("    Recommended for bulk loads only");
    }
    
    if (config.durability == DurabilityMode::Sync) {
        THEMIS_INFO("BatchWriteOptimizer: Sync mode - maximum durability, lower throughput");
    }
    
    if (config.durability == DurabilityMode::Async) {
        THEMIS_INFO("BatchWriteOptimizer: Async mode - balanced durability and performance");
    }
}

} // namespace themis
