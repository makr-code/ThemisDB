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

/**
 * @struct AuditBatchCheckpoint
 * @brief Crash-recovery checkpoint for audit batch writes
 */
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
    
    nlohmann::json toJson() const;
    static AuditBatchCheckpoint fromJson(const nlohmann::json& j);
};

/**
 * @struct IdempotencyToken
 * @brief Token for ensuring idempotent audit entry submissions
 */
struct IdempotencyToken {
    std::string token;                         // Unique token (client-provided or generated)
    std::string entry_id;                      // ID of entry for this token
    int64_t submitted_at_ms = 0;              // When token was first submitted
    std::string state;                         // "pending", "committed", "failed"
    
    nlohmann::json toJson() const;
    static IdempotencyToken fromJson(const nlohmann::json& j);
};

// ============================================================================
// High-Volume Audit Batch Writer
// ============================================================================

/**
 * @class AuditBatchWriter
 * @brief Concurrent, buffered writer for high-volume audit entries
 * 
 * Features:
 * - Lock-free entry submission queue
 * - Automatic batching with periodic flush
 * - Crash-recovery checkpoint support
 * - Idempotency token tracking
 * - Backpressure handling
 * - Metrics and performance tracking
 * 
 * Typical usage:
 * ```
 * auto writer = AuditBatchWriter(
 *     integrity_manager,
 *     AuditBatchWriter::Config{
 *         .buffer_size = 10000,
 *         .batch_size = 1000,
 *         .flush_interval_ms = 100
 *     }
 * );
 * writer.start();
 * 
 * ImmutableAuditEntry entry = ...;
 * Status status = writer.submitEntry(entry);
 * 
 * // Optional: idempotent submission
 * Status status = writer.submitEntryIdempotent(entry, "token-123");
 * 
 * writer.shutdown();
 * ```
 */
class AuditBatchWriter {
public:
    /**
     * @struct Config
     * @brief Configuration for batch writer
     */
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
    
    /**
     * @struct WriteResult
     * @brief Result of batch write operation
     */
    struct WriteResult {
        bool success = false;
        int64_t entries_written = 0;
        int64_t entries_failed = 0;
        std::string error_message;
        nlohmann::json metrics;                // Additional timing info
    };
    
    /**
     * @brief Create batch writer
     * @param manager AuditIntegrityManager to write batches to
     * @param config Configuration options
     */
    AuditBatchWriter(
        std::shared_ptr<AuditIntegrityManager> manager,
        const Config& config = Config{}
    );
    
    ~AuditBatchWriter();
    
    /**
     * @brief Start the writer's background flush thread
     * @return Status
     */
    std::string start();
    
    /**
     * @brief Gracefully shutdown the writer
     * Flushes remaining entries and stops background thread
     * @return Status
     */
    std::string shutdown();
    
    /**
     * @brief Submit an audit entry for batching
     * @param entry Entry to submit
     * @return Status ("OK", "BUFFER_FULL", error message)
     */
    std::string submitEntry(const ImmutableAuditEntry& entry);
    
    /**
     * @brief Submit an entry with idempotency token
     * Guarantees: if same token submitted twice, only one entry is written
     * @param entry Entry to submit
     * @param idempotency_token Unique token for this submission
     * @return Status ("OK", "DUPLICATE", "BUFFER_FULL", error)
     */
    std::string submitEntryIdempotent(
        const ImmutableAuditEntry& entry,
        const std::string& idempotency_token
    );
    
    /**
     * @brief Manually flush pending entries
     * @return WriteResult with statistics
     */
    WriteResult flush();
    
    /**
     * @brief Force flush immediately (blocking)
     * @return WriteResult
     */
    WriteResult forceFlush();
    
    /**
     * @brief Get current buffer statistics
     * @return JSON with buffer size, pending count, etc.
     */
    nlohmann::json getBufferStats() const;
    
    /**
     * @brief Get performance metrics
     * @return JSON with latency, throughput, error rates
     */
    nlohmann::json getMetrics() const;
    
    /**
     * @brief Get crash recovery checkpoints
     * @return Vector of recent checkpoints
     */
    std::vector<AuditBatchCheckpoint> getCheckpoints() const;
    
    /**
     * @brief Verify integrity of a checkpoint
     * @param checkpoint Checkpoint to verify
     * @return true if checkpoint is valid
     */
    bool verifyCheckpoint(const AuditBatchCheckpoint& checkpoint) const;
    
    /**
     * @brief Recover from a checkpoint
     * Resubmit entries from checkpoint that weren't fully flushed
     * @param checkpoint Checkpoint to recover from
     * @return WriteResult
     */
    WriteResult recoverFromCheckpoint(const AuditBatchCheckpoint& checkpoint);
    
    /**
     * @brief Check idempotency token status
     * @param token Token to check
     * @return Optional IdempotencyToken if exists
     */
    std::optional<IdempotencyToken> getTokenStatus(const std::string& token) const;
    
    /**
     * @brief Get configuration
     */
    const Config& getConfig() const { return config_; }
    
    /**
     * @brief Check if writer is running
     */
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
    /// Rolling window of the last 1 000 submission latency samples (µs).
    /// Used to compute p95 / p99 in recordMetrics().  Protected by metrics_mutex_.
    std::vector<double> latency_samples_us_;
    
    // Internal methods
    void flushThread();
    WriteResult flushBatch(const std::vector<ImmutableAuditEntry>& batch);
    std::string computeBatchHash(const std::vector<ImmutableAuditEntry>& batch) const;
    AuditBatchCheckpoint createCheckpoint(
        const std::vector<ImmutableAuditEntry>& batch,
        const std::string& state
    );
    void persistCheckpoint(const AuditBatchCheckpoint& checkpoint);
    void recordMetrics(int64_t submission_latency_us);
};

}  // namespace governance
}  // namespace themis
