/*
 * ThemisDB | File: storage_recovery_fault_handler.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 3 — Error Handling & Edge Cases
 * Purpose: Standardized recovery fault handling for replay and recovery errors.
 */

/**
 * @file storage_recovery_fault_handler.h
 * @brief Standardized recovery fault handling and edge case resolution.
 *
 * This header provides mechanisms for:
 *   - Detecting and classifying recovery faults (torn WAL, timeout, checkpoint invalid)
 *   - Implementing standardized fail-safe behavior for each fault class
 *   - Emitting structured diagnostic events for operator awareness
 *   - Suggesting and executing recovery actions
 *
 * Phase 3 Objective: Unify error handling for recovery-phase faults across
 * the storage module persistence and replay layers.
 *
 * @see include/storage/storage_api_contract.h — Recovery contract §3
 * @see include/storage/storage_error_diagnostics.h — Error classification
 * @see src/storage/ROADMAP.md — Phase 3 items
 */

#pragma once

#include "storage/storage_api_contract.h"
#include "storage/storage_error_diagnostics.h"

#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// § 1  Recovery Fault Types
// ============================================================================

/**
 * @brief Classification of recovery faults.
 *
 * Maps to scenarios described in storage API contract §3 (Recovery Contract).
 */
enum class RecoveryFaultType : int {
    /// Partial entry at WAL tail (torn write); entry discarded, replay continues.
    TORN_WAL_ENTRY = 0,

    /// Checkpoint sequence number is invalid or past WAL tail.
    CHECKPOINT_INVALID = 1,

    /// Recovery procedure exceeded kRecoveryHardTimeout.
    RECOVERY_TIMEOUT = 2,

    /// WAL file is corrupted (invalid magic, bad CRC, etc.).
    WAL_FILE_CORRUPTED = 3,

    /// Unable to read WAL from persistent storage (I/O error).
    WAL_READ_ERROR = 4,

    /// Replay of a WAL entry failed (sequence error, application failure).
    REPLAY_ENTRY_FAILED = 5,

    /// Maximum recovery attempts exceeded without success.
    RECOVERY_RETRY_EXHAUSTED = 6,
};

// ============================================================================
// § 2  Recovery Fault Report
// ============================================================================

/**
 * @brief Comprehensive information about a recovery fault.
 *
 * Used to communicate fault details to diagnostic systems and recovery
 * decision makers.
 */
struct RecoveryFaultReport {
    /// Classification of the fault.
    RecoveryFaultType fault_type{RecoveryFaultType::TORN_WAL_ENTRY};

    /// The error code resulting from this fault.
    StorageErrorCode error_code{StorageErrorCode::RECOVERY_INCOMPLETE};

    /// Checkpoint sequence number at time of fault.
    std::uint64_t checkpoint_seq{0};

    /// First WAL entry sequence number in current recovery pass.
    std::uint64_t recovery_start_seq{0};

    /// Sequence number where fault was detected.
    std::uint64_t fault_seq{0};

    /// Number of entries successfully replayed before fault.
    std::uint64_t entries_replayed{0};

    /// Human-readable fault description.
    std::string fault_message;

    /// Optional suggestion for operator action.
    std::string recovery_suggestion;

    /// Time at which fault was detected.
    std::chrono::system_clock::time_point detection_time{
        std::chrono::system_clock::now()};

    /// Is this fault retryable (true) or permanent (false)?
    bool is_retryable{false};

    /// Should recovery stop (fail) or continue (warn)?
    bool should_stop_recovery{false};
};

// ============================================================================
// § 3  Recovery Fault Handler Interface
// ============================================================================

/**
 * @brief Centralized handler for recovery-phase faults.
 *
 * Provides standardized, fail-safe behavior for:
 *   - Torn WAL entries (skip and emit diagnostic, continue replay)
 *   - Invalid checkpoints (emit diagnostic, attempt recovery from safe point)
 *   - Recovery timeouts (stop and emit alert)
 *   - WAL file corruption (try alternate recovery strategies)
 *   - Replay errors (classify and decide whether to continue or stop)
 *
 * Thread-safe: Safe to call concurrently from recovery threads.
 */
class RecoveryFaultHandler {
public:
    /// Constructor: initializes handler with default policies.
    RecoveryFaultHandler() = default;

    /// Non-copyable.
    RecoveryFaultHandler(const RecoveryFaultHandler&) = delete;
    RecoveryFaultHandler& operator=(const RecoveryFaultHandler&) = delete;

    /// Move-enabled.
    RecoveryFaultHandler(RecoveryFaultHandler&&) noexcept = default;
    RecoveryFaultHandler& operator=(RecoveryFaultHandler&&) noexcept = default;

    ~RecoveryFaultHandler() = default;

    // ── Fault Detection and Classification ──────────────────────────────────

    /**
     * @brief Handle a torn WAL entry (partial write at tail).
     *
     * Implements fail-safe behavior: log diagnostic, skip entry, continue replay.
     *
     * @param wal_path Path to WAL file
     * @param checkpoint_seq Checkpoint position for context
     * @param fault_seq Sequence number of torn entry
     * @param entries_replayed Number of entries replayed before this one
     * @return RecoveryFaultReport with fault details and recovery guidance
     *
     * Guaranteed behavior:
     *   - Returns RECOVERY_INCOMPLETE error code
     *   - Emits diagnostic event for operator awareness
     *   - Provides suggestion: "Partial WAL tail discarded; verify backup..."
     *   - Should continue recovery (is_retryable=false, should_stop_recovery=false)
     */
    [[nodiscard]] RecoveryFaultReport handleTornWalEntry(
        std::string_view wal_path,
        std::uint64_t checkpoint_seq,
        std::uint64_t fault_seq,
        std::uint64_t entries_replayed) noexcept;

    /**
     * @brief Handle an invalid checkpoint (sequence past WAL tail or corrupted).
     *
     * Implements fail-safe behavior: log diagnostic, fall back to safe checkpoint.
     *
     * @param checkpoint_seq Invalid checkpoint sequence number
     * @param wal_tail_seq Actual last sequence in WAL
     * @param recovery_start_seq Suggested safe checkpoint to start recovery from
     * @return RecoveryFaultReport with guidance for fallback checkpoint
     *
     * Guaranteed behavior:
     *   - Returns CHECKPOINT_FAILED error code
     *   - Suggests recovery_start_seq as safe fallback
     *   - Should stop current recovery and restart from suggested point
     *     (should_stop_recovery=true)
     *   - Provides operator suggestion for investigation
     */
    [[nodiscard]] RecoveryFaultReport handleInvalidCheckpoint(
        std::uint64_t checkpoint_seq,
        std::uint64_t wal_tail_seq,
        std::uint64_t recovery_start_seq) noexcept;

    /**
     * @brief Handle recovery timeout (exceeded kRecoveryHardTimeout).
     *
     * Implements fail-safe behavior: log alert, stop recovery, preserve state.
     *
     * @param elapsed_time Time elapsed since recovery started
     * @param entries_replayed Number of entries successfully replayed before timeout
     * @param checkpoint_seq Checkpoint where recovery started
     * @return RecoveryFaultReport with alert-level severity
     *
     * Guaranteed behavior:
     *   - Returns RECOVERY_TIMEOUT error code
     *   - should_stop_recovery=true (must abort)
     *   - is_retryable=true (can retry with increased timeout)
     *   - Emits HIGH/CRITICAL severity diagnostic
     *   - Suggests infrastructure checks (disk speed, memory pressure, etc.)
     */
    [[nodiscard]] RecoveryFaultReport handleRecoveryTimeout(
        std::chrono::milliseconds elapsed_time,
        std::uint64_t entries_replayed,
        std::uint64_t checkpoint_seq) noexcept;

    /**
     * @brief Handle WAL file corruption (invalid magic, CRC mismatch, etc.).
     *
     * Implements fail-safe behavior: log alert, consider alternate recovery
     * strategies (backup restore, PITR, etc.).
     *
     * @param wal_path Path to corrupted WAL file
     * @param corruption_type Description of corruption (e.g., "invalid_magic",
     *                        "crc_mismatch", "truncated_file")
     * @param bytes_verified Bytes successfully verified before corruption
     * @return RecoveryFaultReport with recovery strategy recommendations
     *
     * Guaranteed behavior:
     *   - Returns WAL_CORRUPTED error code
     *   - should_stop_recovery=true (this WAL cannot be used)
     *   - is_retryable=false (corruption is permanent)
     *   - Emits CRITICAL severity diagnostic
     *   - Suggests backup restore or PITR recovery
     */
    [[nodiscard]] RecoveryFaultReport handleWalFileCorruption(
        std::string_view wal_path,
        std::string_view corruption_type,
        std::uint64_t bytes_verified) noexcept;

    /**
     * @brief Handle I/O error during WAL read.
     *
     * Implements fail-safe behavior: log diagnostic, decide whether to retry
     * or fail over to backup recovery.
     *
     * @param wal_path Path to WAL file
     * @param io_error_msg Description of I/O error (e.g., "permission denied",
     *                     "disk offline")
     * @param retry_count Number of previous retry attempts
     * @return RecoveryFaultReport with retry/failover guidance
     *
     * Guaranteed behavior:
     *   - Returns WAL_WRITE_FAILED or INTERNAL_ERROR
     *   - is_retryable based on error type and retry_count
     *   - Suggests failover strategy if retries exhausted
     *   - Emits HIGH/CRITICAL severity diagnostic
     */
    [[nodiscard]] RecoveryFaultReport handleWalReadError(
        std::string_view wal_path,
        std::string_view io_error_msg,
        int retry_count) noexcept;

    /**
     * @brief Handle failure during entry replay (e.g., sequence number error,
     *        application-level replay failure).
     *
     * Implements fail-safe behavior: log diagnostic, decide whether to skip
     * entry or stop recovery.
     *
     * @param fault_seq Sequence number of entry that failed to replay
     * @param replay_error Description of failure
     * @param is_critical true if error cannot be safely skipped
     * @return RecoveryFaultReport with skip/stop guidance
     *
     * Guaranteed behavior:
     *   - Returns appropriate error code based on failure type
     *   - is_critical determines should_stop_recovery flag
     *   - Emits MEDIUM/HIGH severity diagnostic
     *   - Suggests manual investigation if critical
     */
    [[nodiscard]] RecoveryFaultReport handleReplayEntryFailure(
        std::uint64_t fault_seq,
        std::string_view replay_error,
        bool is_critical = false) noexcept;

    /**
     * @brief Handle maximum retry attempts exceeded during recovery.
     *
     * Implements fail-safe behavior: abort recovery, emit alert for manual
     * intervention.
     *
     * @param checkpoint_seq Checkpoint where recovery started
     * @param attempts Number of failed attempts
     * @param last_error Description of last error encountered
     * @return RecoveryFaultReport with CRITICAL severity
     *
     * Guaranteed behavior:
     *   - Returns RECOVERY_TIMEOUT or INTERNAL_ERROR
     *   - should_stop_recovery=true (must abort)
     *   - is_retryable=false (retry limit reached)
     *   - Emits CRITICAL severity diagnostic
     *   - Suggests manual recovery intervention
     */
    [[nodiscard]] RecoveryFaultReport handleRecoveryRetryExhausted(
        std::uint64_t checkpoint_seq,
        int attempts,
        std::string_view last_error) noexcept;

    // ── Configuration and Policies ─────────────────────────────────────────

    /**
     * @brief Set maximum number of recovery retry attempts.
     *
     * Default: 3 retries for transient faults.
     *
     * @param max_retries Maximum number of retries (0 = no retries)
     */
    void setMaxRecoveryRetries(int max_retries) noexcept {
        max_recovery_retries_ = max_retries;
    }

    /**
     * @brief Set timeout for entire recovery procedure.
     *
     * Default: kRecoveryHardTimeout from storage_api_contract.h.
     *
     * @param timeout Maximum duration for recovery operation
     */
    void setRecoveryTimeout(std::chrono::milliseconds timeout) noexcept {
        recovery_timeout_ = timeout;
    }

    /**
     * @brief Set whether recovery should continue on torn WAL entries.
     *
     * Default: true (continue; emit RECOVERY_INCOMPLETE diagnostic).
     *
     * @param continue_on_torn true to skip torn entries and continue,
     *                          false to abort recovery
     */
    void setContinueOnTornWal(bool continue_on_torn) noexcept {
        continue_on_torn_wal_ = continue_on_torn;
    }

private:
    int max_recovery_retries_{3};
    std::chrono::milliseconds recovery_timeout_{kRecoveryHardTimeout};
    bool continue_on_torn_wal_{true};
};

}  // namespace storage
}  // namespace themis

