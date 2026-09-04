// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file storage_recovery_fault_handler.cpp
 * @brief Implementation of standardized recovery fault handling.
 *
 * Provides fail-safe behavior for recovery-phase faults including torn WAL
 * entries, invalid checkpoints, timeout, corruption, and I/O errors.
 *
 * @see include/storage/storage_recovery_fault_handler.h
 * @see include/storage/storage_api_contract.h
 */

#include "storage/storage_recovery_fault_handler.h"

#include <chrono>
#include <format>
#include <string>

// Logging support
#include "utils/logger.h"

namespace themis {
namespace storage {

// ============================================================================
// § 1  Utility: Build Recovery Fault Report
// ============================================================================

/**
 * @brief Helper to construct a RecoveryFaultReport with common fields.
 */
static RecoveryFaultReport buildFaultReport(
    RecoveryFaultType fault_type,
    StorageErrorCode error_code,
    std::uint64_t checkpoint_seq,
    std::uint64_t recovery_start_seq,
    std::uint64_t fault_seq,
    std::uint64_t entries_replayed,
    std::string_view fault_message,
    std::string_view recovery_suggestion,
    bool is_retryable,
    bool should_stop_recovery) noexcept {
    return RecoveryFaultReport{
        .fault_type = fault_type,
        .error_code = error_code,
        .checkpoint_seq = checkpoint_seq,
        .recovery_start_seq = recovery_start_seq,
        .fault_seq = fault_seq,
        .entries_replayed = entries_replayed,
        .fault_message = std::string(fault_message),
        .recovery_suggestion = std::string(recovery_suggestion),
        .detection_time = std::chrono::system_clock::now(),
        .is_retryable = is_retryable,
        .should_stop_recovery = should_stop_recovery,
    };
}

// ============================================================================
// § 2  RecoveryFaultHandler Implementation
// ============================================================================

RecoveryFaultReport RecoveryFaultHandler::handleTornWalEntry(
    std::string_view wal_path,
    std::uint64_t checkpoint_seq,
    std::uint64_t fault_seq,
    std::uint64_t entries_replayed) noexcept {
    std::string msg = std::format(
        "Torn WAL entry detected at sequence {}: partial write at file end will be discarded. "
        "Checkpoint={}, Entries replayed={}",
        fault_seq, checkpoint_seq, entries_replayed);

    std::string suggestion =
        "Partial WAL entry tail was discarded as expected during recovery. "
        "Verify backup integrity if critical data consistency is required.";

    auto report = buildFaultReport(
        RecoveryFaultType::TORN_WAL_ENTRY,
        StorageErrorCode::RECOVERY_INCOMPLETE,
        checkpoint_seq,
        checkpoint_seq,  // recovery started from checkpoint
        fault_seq,
        entries_replayed,
        msg,
        suggestion,
        false,  // not retryable (this is expected behavior)
        !continue_on_torn_wal_  // stop only if configured to do so
    );

    // Emit diagnostic
    emitRecoveryFaultEvent("torn_wal_entry", checkpoint_seq, entries_replayed, suggestion);

    THEMIS_INFO("Recovery: {}", msg);
    return report;
}

RecoveryFaultReport RecoveryFaultHandler::handleInvalidCheckpoint(
    std::uint64_t checkpoint_seq,
    std::uint64_t wal_tail_seq,
    std::uint64_t recovery_start_seq) noexcept {
    std::string msg = std::format(
        "Invalid checkpoint sequence {} (WAL tail at {}): "
        "checkpoint is beyond WAL extent. Falling back to sequence {}.",
        checkpoint_seq, wal_tail_seq, recovery_start_seq);

    std::string suggestion = std::format(
        "Checkpoint was invalid. Will attempt recovery from safe sequence {}. "
        "Investigate checkpoint management for root cause.",
        recovery_start_seq);

    auto report = buildFaultReport(
        RecoveryFaultType::CHECKPOINT_INVALID,
        StorageErrorCode::CHECKPOINT_FAILED,
        checkpoint_seq,
        recovery_start_seq,  // suggested safe start point
        checkpoint_seq,      // fault at checkpoint itself
        0,
        msg,
        suggestion,
        true,  // retryable with fallback checkpoint
        true   // must stop current recovery and restart from fallback
    );

    emitRecoveryFaultEvent("checkpoint_invalid", checkpoint_seq, 0, suggestion);

    THEMIS_WARN("Recovery: {}", msg);
    return report;
}

RecoveryFaultReport RecoveryFaultHandler::handleRecoveryTimeout(
    std::chrono::milliseconds elapsed_time,
    std::uint64_t entries_replayed,
    std::uint64_t checkpoint_seq) noexcept {
    std::string msg = std::format(
        "Recovery procedure exceeded timeout: {} ms elapsed (limit: {} ms). "
        "{} entries replayed from checkpoint {}.",
        elapsed_time.count(), recovery_timeout_.count(), entries_replayed, checkpoint_seq);

    std::string suggestion =
        "Recovery timeout indicates I/O bottleneck or hardware degradation. "
        "Check disk speed, memory pressure, and system load. "
        "Consider upgrading storage hardware or increasing kRecoveryHardTimeout.";

    auto report = buildFaultReport(
        RecoveryFaultType::RECOVERY_TIMEOUT,
        StorageErrorCode::RECOVERY_TIMEOUT,
        checkpoint_seq,
        checkpoint_seq,
        0,  // no specific entry at fault
        entries_replayed,
        msg,
        suggestion,
        true,   // retryable (can retry with more time)
        true    // must stop (timeout is hard stop)
    );

    emitRecoveryFaultEvent("recovery_timeout", checkpoint_seq, entries_replayed, suggestion);

    THEMIS_ERROR("Recovery: {}", msg);
    return report;
}

RecoveryFaultReport RecoveryFaultHandler::handleWalFileCorruption(
    std::string_view wal_path,
    std::string_view corruption_type,
    std::uint64_t bytes_verified) noexcept {
    std::string msg = std::format(
        "WAL file corrupted: {} (verified {} bytes before corruption). "
        "File: {}",
        corruption_type, bytes_verified, wal_path);

    std::string suggestion =
        "WAL file is permanently corrupted and cannot be replayed. "
        "Options: (1) Restore from backup, (2) Use PITR if WAL before corruption is available, "
        "(3) Reinitialize database if no backups exist.";

    auto report = buildFaultReport(
        RecoveryFaultType::WAL_FILE_CORRUPTED,
        StorageErrorCode::WAL_CORRUPTED,
        0,  // checkpoint unknown (corrupted)
        bytes_verified,
        bytes_verified,  // fault point
        0,
        msg,
        suggestion,
        false,  // not retryable (corruption is permanent)
        true    // must stop (cannot continue with corrupted WAL)
    );

    emitRecoveryFaultEvent("wal_corruption", 0, 0, suggestion);

    THEMIS_ERROR("Recovery: {}", msg);
    return report;
}

RecoveryFaultReport RecoveryFaultHandler::handleWalReadError(
    std::string_view wal_path,
    std::string_view io_error_msg,
    int retry_count) noexcept {
    bool should_retry = retry_count < max_recovery_retries_;

    std::string msg = std::format(
        "WAL read I/O error: {} (File: {}, Retry: {}/{})",
        io_error_msg, wal_path, retry_count + 1, max_recovery_retries_);

    std::string suggestion = {};
    if (should_retry) {
        suggestion = "Transient I/O error during WAL read. "
                     "Will retry with exponential backoff.";
    } else {
        suggestion = "Maximum WAL read retries exhausted. "
                     "Check disk health, consider failover to replica, "
                     "or restore from backup.";
    }

    auto report = buildFaultReport(
        RecoveryFaultType::WAL_READ_ERROR,
        StorageErrorCode::WAL_WRITE_FAILED,
        0,
        0,
        0,
        0,
        msg,
        suggestion,
        should_retry,
        !should_retry  // stop if retries exhausted
    );

    emitRecoveryFaultEvent("wal_read_error", 0, 0, suggestion);

    if (should_retry) {
        THEMIS_WARN("Recovery: {}", msg);
    } else {
        THEMIS_ERROR("Recovery: {}", msg);
    }

    return report;
}

RecoveryFaultReport RecoveryFaultHandler::handleReplayEntryFailure(
    std::uint64_t fault_seq,
    std::string_view replay_error,
    bool is_critical) noexcept {
    std::string msg = std::format(
        "WAL entry replay failed at sequence {}: {}{}",
        fault_seq, replay_error, is_critical ? " (CRITICAL)" : "");

    std::string suggestion = {};
    if (is_critical) {
        suggestion = "Critical error during WAL entry replay. "
                     "Recovery cannot continue. Manual investigation required.";
    } else {
        suggestion = "Non-critical entry replay error. "
                     "Recovery will skip this entry and continue.";
    }

    auto report = buildFaultReport(
        RecoveryFaultType::REPLAY_ENTRY_FAILED,
        is_critical ? StorageErrorCode::RECOVERY_INCOMPLETE
                    : StorageErrorCode::INTERNAL_ERROR,
        0,
        0,
        fault_seq,
        fault_seq - 1,  // entries replayed up to but not including this one
        msg,
        suggestion,
        !is_critical,  // retryable only if non-critical
        is_critical    // stop only if critical
    );

    emitRecoveryFaultEvent("replay_entry_failed", fault_seq, fault_seq - 1, suggestion);

    if (is_critical) {
        THEMIS_ERROR("Recovery: {}", msg);
    } else {
        THEMIS_WARN("Recovery: {}", msg);
    }

    return report;
}

RecoveryFaultReport RecoveryFaultHandler::handleRecoveryRetryExhausted(
    std::uint64_t checkpoint_seq,
    int attempts,
    std::string_view last_error) noexcept {
    std::string msg = std::format(
        "Recovery retry limit exhausted after {} attempts. "
        "Last error: {} (Checkpoint: {})",
        attempts, last_error, checkpoint_seq);

    std::string suggestion =
        "Recovery has been attempted multiple times without success. "
        "Manual intervention required: check logs for root cause, "
        "verify backup integrity, or restore from alternative backup.";

    auto report = buildFaultReport(
        RecoveryFaultType::RECOVERY_RETRY_EXHAUSTED,
        StorageErrorCode::RECOVERY_INCOMPLETE,
        checkpoint_seq,
        checkpoint_seq,
        0,
        0,
        msg,
        suggestion,
        false,  // not retryable (retries exhausted)
        true    // must stop
    );

    emitRecoveryFaultEvent("recovery_retry_exhausted", checkpoint_seq, 0, suggestion);

    THEMIS_ERROR("Recovery: {}", msg);
    return report;
}

}  // namespace storage
}  // namespace themis
