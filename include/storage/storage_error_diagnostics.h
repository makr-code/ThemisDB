/**
 * @file storage_error_diagnostics.h
 * @brief Unified error diagnostics and incident classification for storage module.
 *
 * This header provides a centralized system for:
 *   - Classifying storage errors by severity and incident type
 *   - Mapping implementation errors to StorageErrorCode
 *   - Emitting structured diagnostic events
 *   - Tracking error context and recovery suggestions
 *
 * Phase 3 Objective: Standardize fail-safe behavior for replay faults, storage
 * pressure, and recovery errors by providing a unified diagnostics interface
 * used consistently across persistence, maintenance, and recovery layers.
 *
 * @see include/storage/storage_api_contract.h — Error taxonomy
 * @see src/storage/ROADMAP.md — Phase 3 items
 */

#pragma once

#include "storage/storage_api_contract.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// § 1  Error Severity Levels
// ============================================================================

/**
 * @brief Severity classification for storage error incidents.
 *
 * Used to determine alerting strategy and operator action prioritization.
 */
enum class ErrorSeverity : int {
    /// No error; operation succeeded.
    INFO = 0,

    /// Low severity: degraded performance or non-critical resources affected.
    /// Example: Tiering migration delays, non-critical index maintenance.
    LOW = 1,

    /// Medium severity: operation failure recoverable by retry or alternative path.
    /// Example: Transaction conflicts, transient backup limit exceeded.
    MEDIUM = 2,

    /// High severity: operation failure requiring operator action.
    /// Example: WAL write failure, storage exhausted, recovery incomplete.
    HIGH = 3,

    /// Critical severity: durability threat or data integrity risk.
    /// Example: WAL corruption, backup corrupted, storage exhausted during recovery.
    CRITICAL = 4,
};

// ============================================================================
// § 2  Error Incident Classes
// ============================================================================

/**
 * @brief Classification of storage error by incident type.
 *
 * Used to determine mitigation strategy and diagnostic scope.
 */
enum class ErrorIncidentType : int {
    /// Replay or recovery-related fault (torn WAL, timeout, checkpoint invalid).
    RECOVERY_FAULT = 0,

    /// Storage capacity or quota exceeded.
    STORAGE_PRESSURE = 1,

    /// Transaction-level conflict or constraint violation.
    TRANSACTION_CONFLICT = 2,

    /// Tiering or data migration failure.
    TIERING_FAILURE = 3,

    /// Compaction or maintenance-related error.
    MAINTENANCE_FAILURE = 4,

    /// Backup or PITR-related error.
    BACKUP_FAILURE = 5,

    /// Concurrency or synchronization issue.
    CONCURRENCY_ISSUE = 6,

    /// Unclassified or internal error.
    INTERNAL_ERROR = 7,
};

// ============================================================================
// § 3  Error Context
// ============================================================================

/**
 * @brief Detailed context information for a storage error.
 *
 * Provides comprehensive diagnostic data to assist with error recovery
 * and root-cause analysis.
 */
struct StorageErrorContext {
    /// The error code (from StorageErrorCode enum).
    StorageErrorCode error_code{StorageErrorCode::OK};

    /// Human-readable error message.
    std::string message;

    /// Incident classification (recovery fault, storage pressure, etc.).
    ErrorIncidentType incident_type{ErrorIncidentType::INTERNAL_ERROR};

    /// Severity level for alerting and prioritization.
    ErrorSeverity severity{ErrorSeverity::LOW};

    /// Optional operation context (e.g., "put(key=foo)", "restore(timestamp=...)")
    std::string operation_context;

    /// Optional affected resource (e.g., WAL path, key, partition ID).
    std::string affected_resource;

    /// Timestamp when error occurred.
    std::chrono::system_clock::time_point timestamp{
        std::chrono::system_clock::now()};

    /// Optional suggestion for operator or automated recovery.
    std::string recovery_suggestion;

    /// Retry policy recommendation (0 = don't retry, N > 0 = max retries).
    int retry_count{0};

    /// Whether error represents a durability threat.
    bool is_durability_threat{false};

    /// Whether error is retryable without manual intervention.
    bool is_retryable{false};
};

// ============================================================================
// § 4  Diagnostics Classification Functions
// ============================================================================

/**
 * @brief Classify a storage error code into an incident type and severity.
 *
 * Maps each StorageErrorCode to its corresponding incident class and
 * severity level for consistent error handling across the storage module.
 *
 * @param code Error code to classify
 * @return StorageErrorContext with incident_type, severity, and recommendations
 *         populated based on the error code.
 *
 * @example
 *   auto ctx = classifyErrorIncident(StorageErrorCode::WAL_WRITE_FAILED);
 *   // ctx.incident_type == ErrorIncidentType::RECOVERY_FAULT
 *   // ctx.severity == ErrorSeverity::CRITICAL
 *   // ctx.is_durability_threat == true
 */
[[nodiscard]] StorageErrorContext classifyErrorIncident(StorageErrorCode code) noexcept;

/**
 * @brief Map a generic error string/exception message to StorageErrorCode.
 *
 * Attempts to infer the storage error code from an unclassified error message.
 * Used when integrating with external libraries (RocksDB, cloud backends, etc.)
 * that don't natively understand StorageErrorCode.
 *
 * Fallback: If no match is found, returns StorageErrorCode::INTERNAL_ERROR.
 *
 * @param error_message Error message from underlying implementation
 * @return StorageErrorCode best matching the message, or INTERNAL_ERROR
 *
 * @example
 *   auto code = mapErrorMessage("Disk is full");
 *   // Returns StorageErrorCode::STORAGE_EXHAUSTED
 */
[[nodiscard]] StorageErrorCode mapErrorMessage(std::string_view error_message) noexcept;

/**
 * @brief Build a complete diagnostic context from an error code and operation context.
 *
 * Populates a StorageErrorContext with:
 *   - Incident type and severity
 *   - Retry recommendation
 *   - Durability threat classification
 *   - Recovery suggestions
 *
 * @param code Error code
 * @param operation_context Description of the operation that failed
 *                          (e.g., "put(key=foo/bar)")
 * @param affected_resource Optional affected resource identifier
 *                          (e.g., WAL path, key name)
 * @return Fully populated StorageErrorContext
 *
 * @example
 *   auto ctx = buildErrorContext(
 *       StorageErrorCode::STORAGE_EXHAUSTED,
 *       "checkpoint()",
 *       "/nvme/themis.wal"
 *   );
 *   // ctx.severity == ErrorSeverity::CRITICAL
 *   // ctx.recovery_suggestion == "Free disk space or configure tiering"
 */
[[nodiscard]] StorageErrorContext buildErrorContext(
    StorageErrorCode code,
    std::string_view operation_context,
    std::string_view affected_resource = "") noexcept;

// ============================================================================
// § 5  Diagnostic Event Emission
// ============================================================================

/**
 * @brief Emit a structured diagnostic event for a storage error.
 *
 * Logs and records error context in a format suitable for:
 *   - Operator dashboards and alerts
 *   - Audit logs and compliance records
 *   - Root-cause analysis and incident investigation
 *
 * Thread-safe: Safe to call concurrently from multiple storage operations.
 *
 * @param context Error context to emit
 *
 * @example
 *   auto ctx = buildErrorContext(StorageErrorCode::WAL_WRITE_FAILED, "flush()");
 *   ctx.recovery_suggestion = "Check disk health; consider failover";
 *   emitDiagnosticEvent(ctx);
 *   // Logs to audit, metrics, and alert channels
 */
void emitDiagnosticEvent(const StorageErrorContext& context) noexcept;

/**
 * @brief Emit a recovery fault diagnostic event.
 *
 * Specialized event emitter for recovery-phase errors (torn WAL, checkpoint
 * invalid, replay timeout).  Includes recovery-specific metadata.
 *
 * @param fault_type Description of the recovery fault
 *                   (e.g., "torn_wal_entry", "checkpoint_invalid")
 * @param affected_checkpoint_seq Sequence number of affected checkpoint
 * @param recovered_entries Number of successfully replayed entries before fault
 * @param suggestion Recovery action for operator
 *
 * @see StorageErrorContext::recovery_suggestion
 */
void emitRecoveryFaultEvent(
    std::string_view fault_type,
    std::uint64_t affected_checkpoint_seq,
    std::uint64_t recovered_entries,
    std::string_view suggestion = "") noexcept;

/**
 * @brief Emit a storage pressure event.
 *
 * Specialized event emitter for capacity-related errors (storage exhausted,
 * backup limit exceeded, compaction backpressure).
 *
 * @param pressure_type Type of pressure (e.g., "disk_full", "backup_limit")
 * @param available_bytes Remaining capacity (0 if unknown)
 * @param required_bytes Capacity requested by operation (0 if unknown)
 * @param escalation_level Current pressure level (1-5; 5 = critical)
 */
void emitStoragePressureEvent(
    std::string_view pressure_type,
    std::uint64_t available_bytes,
    std::uint64_t required_bytes,
    int escalation_level = 1) noexcept;

// ============================================================================
// § 6  Error Description Utilities
// ============================================================================

/**
 * @brief Get human-readable name for an error code.
 *
 * @param code Error code
 * @return Short descriptive name (e.g., "WAL_WRITE_FAILED")
 */
[[nodiscard]] std::string_view errorCodeName(StorageErrorCode code) noexcept;

/**
 * @brief Get human-readable description of an error code.
 *
 * @param code Error code
 * @return Longer description suitable for operator dashboards
 *         (e.g., "Write-Ahead Log write failed (I/O error, timeout, or disk full)")
 */
[[nodiscard]] std::string_view errorCodeDescription(StorageErrorCode code) noexcept;

/**
 * @brief Get human-readable name for an incident type.
 *
 * @param type Incident type
 * @return Short descriptive name (e.g., "RECOVERY_FAULT")
 */
[[nodiscard]] std::string_view incidentTypeName(ErrorIncidentType type) noexcept;

/**
 * @brief Get human-readable name for a severity level.
 *
 * @param severity Severity level
 * @return Short name (e.g., "CRITICAL", "HIGH")
 */
[[nodiscard]] std::string_view severityName(ErrorSeverity severity) noexcept;

}  // namespace storage
}  // namespace themis

#endif  // THEMIS_STORAGE_ERROR_DIAGNOSTICS_H_
