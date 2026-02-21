/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_migrator.h                                    ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:38:04                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     198                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 812e39337  2026-01-04  Add comprehensive tests for RAID data push, pipeline inte... ║
    • b1854b7a7  2026-01-03  Fix critical thread-safety and signal handling issues ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <functional>
#include <vector>
#include <memory>
#include <unordered_set>
#include <mutex>
#include <atomic>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declaration
class PrometheusMetrics;

// Progress information for data migration
struct MigrationProgress {
    uint64_t records_migrated = 0;
    uint64_t total_records = 0;
    uint64_t bytes_transferred = 0;
    uint64_t errors = 0;
    double progress_percent = 0.0;
    std::string migration_id;  // Deterministic migration ID
};

// Result of a migration operation
struct MigrationResult {
    bool success = false;
    uint64_t records_migrated = 0;
    uint64_t bytes_transferred = 0;
    std::vector<std::string> errors;
    std::string error_message;
    std::string migration_id;  // Deterministic migration ID for tracking
    bool was_already_completed = false;  // True if migration was already done
};

// Configuration for data migrator
struct DataMigratorConfig {
    std::string source_endpoint;
    std::string target_endpoint;
    std::string cert_path;
    std::string key_path;
    std::string ca_cert_path;
    uint32_t batch_size = 1000;
    bool verify_integrity = true;
    uint32_t max_retries = 3;
    uint32_t retry_delay_ms = 1000;
    
    // Idempotency configuration
    bool enable_idempotency = true;     // Enable idempotent migrations
    std::string idempotency_store_path = "./migrations";  // Path to store migration state
};

/**
 * Handles data migration between shards
 * 
 * Features:
 * - Stream-based batch processing
 * - Data integrity verification (hash-based)
 * - Atomic cutover
 * - Progress tracking
 * - Error handling with retry
 */
class DataMigrator {
public:
    using ProgressCallback = std::function<void(const MigrationProgress&)>;

    explicit DataMigrator(
        const DataMigratorConfig& config,
        std::shared_ptr<PrometheusMetrics> metrics = nullptr
    );
    ~DataMigrator() = default;

    /**
     * Migrate data for a token range from source to target shard
     * 
     * @param source_shard_id Source shard identifier
     * @param target_shard_id Target shard identifier
     * @param token_range_start Start of token range to migrate
     * @param token_range_end End of token range to migrate
     * @param progress_callback Optional callback for progress updates
     * @return MigrationResult with success status and statistics
     */
    MigrationResult migrate(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end,
        ProgressCallback progress_callback = nullptr
    );

    /**
     * Verify data integrity between source and target
     * 
     * @param source_shard_id Source shard identifier
     * @param target_shard_id Target shard identifier
     * @param token_range_start Start of token range
     * @param token_range_end End of token range
     * @return true if data matches, false otherwise
     */
    bool verifyIntegrity(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end
    );
    
    // Public for testing
    std::string generateMigrationId(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end
    );
    
    std::string generateBatchId(
        const std::string& migration_id,
        uint32_t batch_index
    );
    
    bool isMigrationCompleted(const std::string& migration_id);
    void markMigrationCompleted(const std::string& migration_id);
    bool isBatchCompleted(const std::string& batch_id);
    void markBatchCompleted(const std::string& batch_id);

private:
    DataMigratorConfig config_;
    std::shared_ptr<PrometheusMetrics> metrics_;
    
    // Idempotency tracking
    mutable std::mutex idempotency_mutex_;
    std::unordered_set<std::string> completed_migrations_;
    std::unordered_set<std::string> completed_batches_;
    std::atomic<size_t> batch_counter_{0};  // Thread-safe counter for idempotency state persistence
    
    /**
     * Load idempotency state from persistent storage
     */
    void loadIdempotencyState();
    
    /**
     * Save idempotency state to persistent storage
     */
    void saveIdempotencyState();

    // Fetch batch of records from source
    nlohmann::json fetchBatch(
        const std::string& source_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end,
        uint32_t offset,
        uint32_t limit
    );

    // Write batch to target
    bool writeBatch(
        const std::string& target_shard_id,
        const nlohmann::json& batch
    );

    // Calculate hash for data integrity
    std::string calculateHash(const nlohmann::json& data);

    // Retry logic for failed operations
    template<typename Func>
    bool retryOperation(Func func);
};

} // namespace sharding
} // namespace themis
