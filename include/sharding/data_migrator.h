/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            data_migrator.h                                    ║
  Version:         0.0.47                                             ║
  Last Modified:   2026-04-15 18:47:05                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     281                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 33f9fb7774  2026-03-14  feat(sharding): implement adaptive shard rebalancer with ... ║
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
#include <chrono>
#include <nlohmann/json.hpp>

namespace themis {
namespace sharding {

// Forward declarations
class PrometheusMetrics;
class ShardTopology;
class WALShipper;

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

/**
 * Configuration for live (dual-write) shard migration.
 *
 * During a live migration the old shard continues to accept writes.  The new
 * shard receives a bulk copy of existing data (via migrate()), then catches up
 * with writes that arrived during the copy by replaying WAL entries shipped
 * from the source via WALShipper.  Once the WAL lag falls below
 * max_wal_lag_bytes an atomic topology cutover is performed via ShardTopology,
 * making the new shard the authoritative owner of the token range while the
 * old shard is downgraded to read-only and eventually decommissioned.
 *
 * This protocol guarantees 0 ms read-unavailability: reads are served from
 * the source shard until the moment of cutover, and from the target shard
 * immediately afterwards.
 */
struct LiveMigrationConfig {
    /// Allow dual writes during migration (old shard always accepts writes).
    bool enable_dual_write = true;

    /// How often to poll the WAL shipper for lag convergence.
    std::chrono::milliseconds catchup_poll_interval{200};

    /// Maximum total time to wait for WAL catchup before failing the migration.
    std::chrono::milliseconds catchup_timeout{std::chrono::minutes(10)};

    /// Target WAL lag (bytes) that must be reached before atomic cutover.
    uint64_t max_wal_lag_bytes = 1024 * 1024;  // 1 MB

    /// Perform a final integrity verification after the bulk copy but before
    /// committing to WAL catchup.
    bool verify_after_bulk_copy = true;
};

/// Result of a dual-write live migration.
struct LiveMigrationResult {
    bool success = false;

    /// Result of the initial bulk data copy phase.
    MigrationResult bulk_migration;

    /// Number of WAL entries applied during the catchup phase.
    uint64_t wal_entries_applied = 0;

    /// Final WAL lag at cutover time (0 means fully caught up).
    uint64_t final_wal_lag_bytes = 0;

    std::string error_message;
    std::string migration_id;
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

    /**
     * Perform a live (dual-write) migration with zero read-unavailability.
     *
     * Protocol:
     *  1. Bulk-copy existing data from source to target using migrate().
     *  2. While the copy runs the source shard continues accepting writes.
     *  3. After the bulk copy the target shard is registered with wal_shipper
     *     so it receives incremental WAL entries for the migrated token range.
     *  4. Once the WAL lag drops below live_cfg.max_wal_lag_bytes an atomic
     *     cutover is performed: topology is updated via ShardTopology, the
     *     source shard becomes read-only for the token range, and the target
     *     shard becomes authoritative.
     *
     * @param source_shard_id     Source (hot) shard
     * @param target_shard_id     Destination shard for the token range
     * @param token_range_start   Start of token range to migrate
     * @param token_range_end     End of token range to migrate
     * @param topology            ShardTopology for atomic cutover (may be nullptr to skip cutover)
     * @param wal_shipper         WALShipper for incremental catchup (may be nullptr to skip WAL phase)
     * @param live_cfg            Live migration configuration
     * @param progress_callback   Optional progress callback for bulk-copy phase
     * @return                    LiveMigrationResult
     */
    LiveMigrationResult liveMigrate(
        const std::string& source_shard_id,
        const std::string& target_shard_id,
        uint64_t token_range_start,
        uint64_t token_range_end,
        std::shared_ptr<ShardTopology> topology = nullptr,
        std::shared_ptr<WALShipper> wal_shipper = nullptr,
        const LiveMigrationConfig& live_cfg = LiveMigrationConfig{},
        ProgressCallback progress_callback = nullptr
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
