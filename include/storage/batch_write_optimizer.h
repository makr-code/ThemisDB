/**
 * @file batch_write_optimizer.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <memory>
#include <string>
#include <vector>
#include <atomic>
#include <chrono>
#include <rocksdb/options.h>

namespace themis {

class RocksDBWrapper;

/**
 * @brief Batch Write Optimizer for maximizing write throughput
 * 
 * Provides optimized WriteOptions configurations for different batch scenarios:
 * - High-throughput bulk inserts (disable WAL syncing)
 * - Balanced mode (async WAL)
 * - Safe mode (sync every batch)
 * 
 * Performance Impact:
 * - Async mode: 2-5x faster than sync
 * - No-sync mode: 10-50x faster (benchmark only!)
 * 
 * Sources:
 * - PERFORMANCE_TIPS.md: Batch Operations section
 * - benchmarks/BATCH_INSERT_PERFORMANCE_RESULTS.md
 * 
 * Thread-safety: Thread-safe: Multiple threads can use the same instance
 */
class BatchWriteOptimizer {
public:
    /**
     * @brief Write durability levels
     */
    enum class DurabilityMode {
        /// Fully synchronous: fsync every batch
        /// - Slowest but safest
        /// - Use for critical transactions
        Sync,
        
        /// Asynchronous WAL: OS buffers fsync
        /// - 2-5x faster than sync
        /// - Recommended for production
        Async,
        
        /// No WAL syncing: Maximum speed
        /// - 10-50x faster than sync
        /// - DANGEROUS: Data loss on crash
        /// - Use ONLY for benchmarks/testing
        NoSync
    };
    
    struct Config {
        DurabilityMode durability = DurabilityMode::Async;
        
        /// Disable WAL entirely (NoSync + skip WAL)
        /// Only for bulk loads where data can be reconstructed
        bool disable_wal = false;
    };
    
    BatchWriteOptimizer();
    explicit BatchWriteOptimizer(const Config& config);
    ~BatchWriteOptimizer();
    
    /**
     * @brief Get optimized WriteOptions for batch operations
     * 
     * @return Configured WriteOptions optimized for batching
     */
    rocksdb::WriteOptions getOptimizedWriteOptions() const;
    
    /**
     * @brief Get statistics since optimizer creation
     */
    struct Stats {
        uint64_t total_batches_written = 0;
        uint64_t total_items_written = 0;
        double avg_batch_size = 0.0;
        double total_write_time_ms = 0.0;
        double avg_write_latency_ms = 0.0;
        double throughput_items_per_sec = 0.0;
    };
    
    Stats getStats() const;
    
    /**
     * @brief Record batch write for statistics
     */
    void recordBatchWrite(size_t items, double latency_ms);
    
    /**
     * @brief Create recommended configuration for use case
     */
    static Config recommendedConfigForUseCase(const std::string& use_case);
    
    /**
     * @brief Validate configuration and warn about dangerous settings
     */
    static void validateConfig(const Config& config);
    
private:
    Config config_;
    
    // Statistics
    mutable std::atomic<uint64_t> total_batches_{0};
    mutable std::atomic<uint64_t> total_items_{0};
    mutable std::atomic<double> total_latency_ms_{0.0};
};

} // namespace themis
