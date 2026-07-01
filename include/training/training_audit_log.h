/**
 * @file training_audit_log.h
 * @brief Training audit logging infrastructure for compliance and forensics
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 1 foundation)
 * @author makr
 * 
 * Provides immutable audit event recording for training pipelines with:
 * - Cryptographic event linking (hash chain)
 * - Structured event types (START, BATCH, CHECKPOINT, END, ERROR)
 * - Persistence options (WAL, JSON Lines)
 * - Forensic replay capability
 * 
 * @since 2026-07-01 (EPIC: LoRA/AdaLoRA Training Pipeline, Phase 1)
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <chrono>
#include <functional>
#include <nlohmann/json.hpp>

namespace themis {
namespace training {

using json = nlohmann::json;

/**
 * @brief Training audit event types
 * 
 * Tracks all significant operations during training lifecycle for reproducibility
 * and forensic analysis.
 */
enum class TrainingAuditEventType {
    TRAINING_START,      ///< Training session initiated
    BATCH_PROCESSED,     ///< Batch training completed with loss
    CHECKPOINT_SAVED,    ///< Training checkpoint persisted
    CHECKPOINT_RESTORED, ///< Checkpoint restored from storage
    EPOCH_COMPLETED,     ///< Epoch completed with metrics
    VALIDATION_RUN,      ///< Validation batch evaluated
    ADAPTER_SAVED,       ///< Final adapter weights saved
    TRAINING_PAUSED,     ///< Training paused (manual)
    TRAINING_RESUMED,    ///< Training resumed from pause
    TRAINING_COMPLETED,  ///< Training completed successfully
    TRAINING_FAILED,     ///< Training failed with error
    DATA_INTEGRITY_CHECK,///< Data integrity validation
    RNG_STATE_SNAPSHOT,  ///< RNG state captured for reproducibility
    MEMORY_CHECKPOINT,   ///< GPU memory state logged
    POLICY_APPLIED,      ///< Selection or training policy applied
    MODEL_PARAMETER_CHANGE, ///< Model or hyperparameter changed
    AUDIT_TRAIL_VERIFIED,///< Audit trail integrity verified
};

/**
 * @brief Convert audit event type to string
 */
std::string toString(TrainingAuditEventType type);

/**
 * @brief Convert string to audit event type
 */
TrainingAuditEventType auditEventTypeFromString(const std::string& str);

/**
 * @brief Immutable audit event record
 * 
 * Represents a single significant event during training with complete context
 * for forensic analysis and reproducibility verification.
 */
struct TrainingAuditEvent {
    // Core identification
    TrainingAuditEventType event_type;              ///< Type of event
    std::string event_id;                           ///< Unique event identifier (UUID v4)
    std::chrono::system_clock::time_point timestamp; ///< Wall-clock timestamp (UTC)
    
    // Context
    std::string training_run_id;                    ///< ID of the training run
    std::string actor_id;                           ///< Who/what triggered the event (e.g., "worker-0", "system")
    std::string session_id;                         ///< Session identifier for correlation
    
    // Event-specific data
    json event_data;                                ///< Event-specific structured data (loss, step, etc.)
    
    // Integrity
    std::string previous_event_hash;                ///< SHA-256 of previous event for chain linking
    std::string event_hash;                         ///< SHA-256 of this event (computed at finalization)
    std::string signature;                          ///< Ed25519 signature (optional, for authentication)
    
    // Status
    bool finalized = false;                         ///< True once event is immutable and persisted
    std::string error_message;                      ///< Error details if event_type is TRAINING_FAILED
    
    TrainingAuditEvent() = default;
    
    /**
     * @brief Convert event to JSON for serialization
     */
    json toJSON() const;
    
    /**
     * @brief Reconstruct event from JSON
     */
    static TrainingAuditEvent fromJSON(const json& j);
    
    /**
     * @brief Compute SHA-256 hash of this event
     * 
     * Hash is computed over all fields except event_hash and signature to enable
     * chain linking and integrity verification.
     * 
     * @return SHA-256 hash as hex string (64 characters)
     */
    std::string computeHash() const;
};

/**
 * @brief Immutable training audit log
 * 
 * Central audit trail for training pipeline operations. All events are:
 * - Cryptographically linked (hash chain)
 * - Timestamped with UTC wall-clock
 * - Persisted atomically (all-or-nothing)
 * - Validated for completeness
 * 
 * Thread-safe for concurrent event logging.
 * 
 * Example usage:
 * @code
 * TrainingAuditLog audit_log("training_run_123", "worker-0");
 * 
 * // Log training start
 * auto start_event = audit_log.logEvent(
 *     TrainingAuditEventType::TRAINING_START,
 *     json{{"batch_size", 32}, {"num_epochs", 3}}
 * );
 * 
 * // Log batch processed
 * for (size_t step = 0; step < num_steps; ++step) {
 *     audit_log.logEvent(
 *         TrainingAuditEventType::BATCH_PROCESSED,
 *         json{{"step", step}, {"loss", current_loss}}
 *     );
 * }
 * 
 * // Log training end
 * audit_log.logEvent(
 *     TrainingAuditEventType::TRAINING_COMPLETED,
 *     json{{"final_loss", final_loss}, {"total_steps", num_steps}}
 * );
 * 
 * // Verify chain integrity
 * if (!audit_log.verifyChainIntegrity()) {
 *     throw std::runtime_error("Audit log integrity violation detected!");
 * }
 * @endcode
 */
class TrainingAuditLog {
public:
    /**
     * @brief Construct audit log for a training run
     * 
     * @param training_run_id Unique identifier for this training run
     * @param actor_id Identifier of training executor (e.g., hostname, worker ID)
     * @param persistence_dir Directory for audit log persistence (optional)
     */
    explicit TrainingAuditLog(
        const std::string& training_run_id,
        const std::string& actor_id = "system",
        const std::string& persistence_dir = ""
    );
    
    ~TrainingAuditLog();
    
    // Delete copy, allow move
    TrainingAuditLog(const TrainingAuditLog&) = delete;
    TrainingAuditLog& operator=(const TrainingAuditLog&) = delete;
    TrainingAuditLog(TrainingAuditLog&&) noexcept = default;
    TrainingAuditLog& operator=(TrainingAuditLog&&) noexcept = default;
    
    /**
     * @brief Log a training audit event
     * 
     * Creates and records an immutable audit event. The event is:
     * - Assigned a unique ID
     * - Timestamped
     * - Linked to previous event via hash chain
     * - Persisted if persistence_dir is configured
     * - Protected from modification
     * 
     * @param event_type Type of event
     * @param event_data Event-specific data (JSON object)
     * @return Const reference to recorded event
     * @throws std::runtime_error if persistence fails or chain is corrupted
     */
    const TrainingAuditEvent& logEvent(
        TrainingAuditEventType event_type,
        const json& event_data = json::object()
    );
    
    /**
     * @brief Log an error event and mark training as failed
     * 
     * @param error_message Human-readable error description
     * @param event_data Optional error context data
     * @return Const reference to error event
     */
    const TrainingAuditEvent& logError(
        const std::string& error_message,
        const json& event_data = json::object()
    );
    
    /**
     * @brief Get all logged events (immutable)
     * 
     * @return Const reference to vector of all events in chronological order
     */
    const std::vector<TrainingAuditEvent>& getAllEvents() const;
    
    /**
     * @brief Get events of specific type
     * 
     * @param event_type Filter by event type
     * @return Vector of events matching type (in chronological order)
     */
    std::vector<TrainingAuditEvent> getEventsByType(TrainingAuditEventType event_type) const;
    
    /**
     * @brief Get event count
     * 
     * @return Total number of logged events
     */
    size_t getEventCount() const;
    
    /**
     * @brief Get training start time
     * 
     * @return Timestamp of TRAINING_START event (or epoch if not started)
     */
    std::chrono::system_clock::time_point getStartTime() const;
    
    /**
     * @brief Get training end time
     * 
     * @return Timestamp of TRAINING_COMPLETED/TRAINING_FAILED event (or epoch if not ended)
     */
    std::chrono::system_clock::time_point getEndTime() const;
    
    /**
     * @brief Get training duration in seconds
     * 
     * @return Duration from start to end event, or 0 if not started/ended
     */
    double getDurationSeconds() const;
    
    /**
     * @brief Verify hash chain integrity
     * 
     * Validates that the entire audit trail is cryptographically intact:
     * - Each event hash matches computed hash
     * - Each event's previous_event_hash matches predecessor's event_hash
     * - No events are missing from the sequence
     * 
     * @return True if chain is valid, false if corruption detected
     */
    bool verifyChainIntegrity() const;
    
    /**
     * @brief Get the Merkle root hash of entire audit trail
     * 
     * Useful for efficiently verifying that audit trail matches known baseline
     * without transmitting entire trail.
     * 
     * @return SHA-256 hash as hex string (64 characters)
     */
    std::string getMerkleRootHash() const;
    
    /**
     * @brief Export audit log to JSON
     * 
     * Serializes entire audit trail in JSON Lines format (one event per line)
     * for archival or transmission.
     * 
     * @param include_signatures Include Ed25519 signatures if present
     * @return JSON array of events
     */
    json toJSON(bool include_signatures = true) const;
    
    /**
     * @brief Export audit log to JSON Lines format
     * 
     * Each line is a complete JSON event object for streaming-friendly processing.
     * 
     * @return String with newline-separated JSON objects
     */
    std::string toJSONLines() const;
    
    /**
     * @brief Persist audit log to file
     * 
     * Writes audit trail to disk in JSON Lines format. Uses atomic write
     * (write-then-rename) to ensure no partial data on crash.
     * 
     * @param file_path Target file path
     * @throws std::runtime_error if write fails
     */
    void persistToFile(const std::string& file_path) const;
    
    /**
     * @brief Load audit log from file
     * 
     * Reconstructs audit trail from persisted JSON Lines file.
     * Automatically verifies chain integrity after loading.
     * 
     * @param file_path Source file path
     * @return New TrainingAuditLog with loaded events
     * @throws std::runtime_error if file not found or chain integrity fails
     */
    static TrainingAuditLog loadFromFile(
        const std::string& file_path,
        const std::string& actor_id = "system"
    );
    
    /**
     * @brief Get whether training has failed
     * 
     * @return True if a TRAINING_FAILED event was logged
     */
    bool hasFailed() const;
    
    /**
     * @brief Get whether training has completed
     * 
     * @return True if TRAINING_COMPLETED or TRAINING_FAILED event was logged
     */
    bool hasCompleted() const;
    
    /**
     * @brief Get last recorded error (if any)
     * 
     * @return Error message from last TRAINING_FAILED event, or empty string
     */
    std::string getLastErrorMessage() const;
    
private:
    std::string training_run_id_;
    std::string actor_id_;
    std::string persistence_dir_;
    std::vector<TrainingAuditEvent> events_;
    mutable std::mutex events_mutex_;
    
    /**
     * @brief Finalize event and add to chain
     */
    const TrainingAuditEvent& recordEvent(TrainingAuditEvent event);
};

/**
 * @brief RAII wrapper for audit logging with automatic START/END logging
 * 
 * Automatically logs TRAINING_START on construction and TRAINING_END
 * on destruction, ensuring all training sessions are properly bracketed.
 * 
 * Example:
 * @code
 * {
 *     TrainingAuditLogGuard audit_guard(audit_log, json{{"mode", "incremental"}});
 *     // Training happens here
 *     // Automatically logs TRAINING_END on scope exit
 * }
 * @endcode
 */
class TrainingAuditLogGuard {
public:
    /**
     * @brief Construct and log training start
     * 
     * @param audit_log Reference to audit log
     * @param start_data Optional data for TRAINING_START event
     */
    explicit TrainingAuditLogGuard(
        TrainingAuditLog& audit_log,
        const json& start_data = json::object()
    );
    
    /**
     * @brief Log training end and cleanup
     */
    ~TrainingAuditLogGuard();
    
    // Delete copy
    TrainingAuditLogGuard(const TrainingAuditLogGuard&) = delete;
    TrainingAuditLogGuard& operator=(const TrainingAuditLogGuard&) = delete;
    
private:
    TrainingAuditLog& audit_log_;
};

} // namespace training
} // namespace themis
