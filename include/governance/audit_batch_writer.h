/**
 * @file audit_batch_writer.h
 * @brief High-volume, concurrent audit entry writer with batching and crash recovery
 * @version 0.1.0
 * @note Maturity: 🟡 BETA (Wave C — Security Production Validation)
 * 
 * This module provides:
 * - Lock-free entry submission for high-volume workloads (1000+ events/sec)
 * - Automatic batching with periodic flush and timeout mechanisms
 * - Crash-recovery checkpoints with sequence numbers and hashes
 * - Idempotency token tracking to prevent duplicate entries
 * - Atomic batch writes to AuditIntegrityManager
 * - Backpressure handling when buffer is full
 * 
 * Performance Targets (Wave C Exit Criteria):
 * - Entry submission: ≤100µs p95, ≤500µs p99
 * - Batch write throughput: ≥10k entries/sec
 * - Crash recovery overhead: ≤50ms per recovery cycle
 * - Idempotency check: ≤10µs per token lookup
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <queue>
#include <mutex>
#include <atomic>
#include <thread>
#include <chrono>
#include <functional>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "governance/governance_audit_integrity.h"

namespace themis {
namespace governance {

// ============================================================================
// Crash Recovery and Idempotency Support
// ============================================================================

struct AuditBatchCheckpoint {
    std::string checkpoint_id;                 // Unique checkpoint identifier
    int64_t batch_sequence_number = 0;        // Sequence of batch being flushed
    int64_t first_entry_sequence = 0;         // First entry sequence in batch
    int64_t last_entry_sequence = 0;          // Last entry sequence in batch
    int64_t entry_count = 0;                  // Number of entries in batch
    std::string batch_hash;                    // SHA-256 hash of batch content
    int64_t checkpoint_time_ms = 0;           // When checkpoint was created
    std::string state;                         // "pending", "flushed", "verified", "failed"
    std::string error_message;                 // Error info if state == "failed"
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static AuditBatchCheckpoint fromJson(const nlohmann::json& j);
};

struct IdempotencyToken {
    std::string token;                         // Unique token (client-provided or generated)
    std::string entry_id;                      // ID of entry for this token
    int64_t submitted_at_ms = 0;              // When token was first submitted
    std::string state;                         // "pending", "committed", "failed"
    
    /**
     * @brief To Json.
     * @return Return value.
     */
    nlohmann::json toJson() const;
    /**
     * @brief From Json.
     * @param[in] j Input parameter.
     * @return Return value.
     */
    static IdempotencyToken fromJson(const nlohmann::json& j);
};

// ============================================================================
// High-Volume Audit Batch Writer
// ============================================================================

class AuditBatchWriter {
public:
    struct Config {
        // Buffer and batching settings
        size_t buffer_size = 10000;            // Max entries in buffer
        size_t batch_size = 1000;              // Target batch size
        int64_t flush_interval_ms = 100;       // Flush timeout (ms)
        
        // Crash recovery settings
        bool enable_checkpoints = true;        // Enable crash-recovery checkpoints
        std::string checkpoint_dir = "./audit_checkpoints";  // Checkpoint storage
        int64_t checkpoint_retention_ms = 86400000;  // Keep checkpoints 24h
        
        // Performance settings
        bool enable_metrics = true;            // Track performance metrics
        int64_t metrics_interval_ms = 60000;   // Report metrics interval (ms)
        
        // Backpressure settings
        bool enable_backpressure = true;       // Backpressure when buffer full
        int64_t backpressure_wait_ms = 100;    // Wait time before backpressure
    };
    
    struct WriteResult {
        bool success = false;
        int64_t entries_written = 0;
        int64_t entries_failed = 0;
        std::string error_message;
        nlohmann::json metrics;                // Additional timing info
    };
    
    AuditBatchWriter(
        std::shared_ptr<AuditIntegrityManager> manager,
        const Config& config = Config{}
    );
    
    ~AuditBatchWriter();
    
    /**
     * @brief Start.
     * @return Return value.
     */
    std::string start();
    
    /**
     * @brief Shutdown.
     * @return Return value.
     */
    std::string shutdown();
    
    /**
     * @brief Submit Entry.
     * @param[in] entry Input parameter.
     * @return Return value.
     */
    std::string submitEntry(const ImmutableAuditEntry& entry);
    
    /**
     * @brief Submit Entry Idempotent.
     * @param[in] entry Input parameter.
     * @param[in] idempotency_token Input parameter.
     * @return Return value.
     */
    std::string submitEntryIdempotent(
        const ImmutableAuditEntry& entry,
        const std::string& idempotency_token
    );
    
    /**
     * @brief Flush.
     * @return Return value.
     */
    WriteResult flush();
    
    /**
     * @brief Force Flush.
     * @return Return value.
     */
    WriteResult forceFlush();
    
    /**
     * @brief Get Buffer Stats.
     * @return Return value.
     */
    nlohmann::json getBufferStats() const;
    
    /**
     * @brief Get Metrics.
     * @return Return value.
     */
    nlohmann::json getMetrics() const;
    
    /**
     * @brief Get Checkpoints.
     * @return Return value.
     */
    std::vector<AuditBatchCheckpoint> getCheckpoints() const;
    
    /**
     * @brief Verify Checkpoint.
     * @param[in] checkpoint Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifyCheckpoint(const AuditBatchCheckpoint& checkpoint) const;
    
    /**
     * @brief Recover From Checkpoint.
     * @param[in] checkpoint Input parameter.
     * @return Return value.
     */
    WriteResult recoverFromCheckpoint(const AuditBatchCheckpoint& checkpoint);
    
    /**
     * @brief Get Token Status.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    std::optional<IdempotencyToken> getTokenStatus(const std::string& token) const;
    
    const Config& getConfig() const { return config_; }
    
    bool isRunning() const { return running_.load(); }

private:
    std::shared_ptr<AuditIntegrityManager> manager_;
    Config config_;
    
    // Buffer management
    mutable std::mutex buffer_mutex_;
    std::vector<ImmutableAuditEntry> pending_entries_;
    std::atomic<int64_t> entry_sequence_counter_{0};
    std::atomic<int64_t> batch_sequence_counter_{0};
    
    // Crash recovery
    std::unordered_map<std::string, AuditBatchCheckpoint> checkpoints_;
    std::vector<AuditBatchCheckpoint> checkpoint_history_;
    
    // Idempotency tracking
    std::unordered_map<std::string, IdempotencyToken> idempotency_tokens_;
    mutable std::mutex idempotency_mutex_;
    
    // Background flushing
    std::unique_ptr<std::thread> flush_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
    
    // Metrics
    struct Metrics {
        int64_t total_entries_submitted = 0;
        int64_t total_entries_flushed = 0;
        int64_t total_batches_flushed = 0;
        int64_t total_errors = 0;
        double avg_submission_latency_us = 0.0;
        double p95_submission_latency_us = 0.0;
        double p99_submission_latency_us = 0.0;
    };
    mutable std::mutex metrics_mutex_;
    Metrics metrics_;
    std::vector<double> latency_samples_us_;
    
    // Internal methods
    /**
     * @brief Flush Thread.
     */
    void flushThread();
    /**
     * @brief Flush Batch.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    WriteResult flushBatch(const std::vector<ImmutableAuditEntry>& batch);
    /**
     * @brief Compute Batch Hash.
     * @param[in] batch Input parameter.
     * @return Return value.
     */
    std::string computeBatchHash(const std::vector<ImmutableAuditEntry>& batch) const;
    /**
     * @brief Create Checkpoint.
     * @param[in] batch Input parameter.
     * @param[in] state Input parameter.
     * @return Return value.
     */
    AuditBatchCheckpoint createCheckpoint(
        const std::vector<ImmutableAuditEntry>& batch,
        const std::string& state
    );
    /**
     * @brief Persist Checkpoint.
     * @param[in] checkpoint Input parameter.
     */
    void persistCheckpoint(const AuditBatchCheckpoint& checkpoint);
    /**
     * @brief Record Metrics.
     * @param[in] submission_latency_us Input parameter.
     */
    void recordMetrics(int64_t submission_latency_us);
};

}  // namespace governance
}  // namespace themis
