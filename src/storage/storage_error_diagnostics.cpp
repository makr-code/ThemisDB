// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file storage_error_diagnostics.cpp
 * @brief Implementation of unified error diagnostics and incident classification.
 *
 * Provides centralized error classification, mapping, and diagnostic event emission
 * for consistent error handling across storage module persistence, maintenance,
 * and recovery layers.
 *
 * @see include/storage/storage_error_diagnostics.h
 * @see include/storage/storage_api_contract.h
 */

#include "storage/storage_error_diagnostics.h"

#include <algorithm>
#include <chrono>
#include <format>
#include <mutex>
#include <sstream>
#include <unordered_map>

// Logging support
#include "utils/logger.h"

namespace themis {
namespace storage {

// ============================================================================
// § 1  Error Severity Mapping
// ============================================================================

/**
 * @brief Map StorageErrorCode to default severity.
 */
static constexpr std::pair<StorageErrorCode, ErrorSeverity> kErrorSeverityMap[] = {
    {StorageErrorCode::OK, ErrorSeverity::INFO},

    // WAL — Critical
    {StorageErrorCode::WAL_WRITE_FAILED, ErrorSeverity::CRITICAL},
    {StorageErrorCode::WAL_SEQUENCE_ERROR, ErrorSeverity::CRITICAL},
    {StorageErrorCode::WAL_CORRUPTED, ErrorSeverity::CRITICAL},

    // MVCC/Transaction — Medium (mostly retryable)
    {StorageErrorCode::TRANSACTION_CONFLICT, ErrorSeverity::MEDIUM},
    {StorageErrorCode::TRANSACTION_TIMEOUT, ErrorSeverity::MEDIUM},
    {StorageErrorCode::VERSION_CHAIN_FULL, ErrorSeverity::HIGH},
    {StorageErrorCode::KEY_NOT_FOUND, ErrorSeverity::LOW},

    // Recovery — High (operator attention required)
    {StorageErrorCode::RECOVERY_TIMEOUT, ErrorSeverity::HIGH},
    {StorageErrorCode::RECOVERY_INCOMPLETE, ErrorSeverity::HIGH},
    {StorageErrorCode::CHECKPOINT_FAILED, ErrorSeverity::HIGH},

    // Backup/PITR — High/Critical
    {StorageErrorCode::PITR_INVALID_TIMESTAMP, ErrorSeverity::MEDIUM},
    {StorageErrorCode::BACKUP_CORRUPTED, ErrorSeverity::CRITICAL},
    {StorageErrorCode::BACKUP_LIMIT_EXCEEDED, ErrorSeverity::HIGH},
    {StorageErrorCode::RESTORE_FAILED, ErrorSeverity::HIGH},

    // Tiering/Compaction — Medium
    {StorageErrorCode::TIERING_MIGRATION_FAILED, ErrorSeverity::MEDIUM},
    {StorageErrorCode::COMPACTION_ABORTED, ErrorSeverity::MEDIUM},

    // Capacity — Critical
    {StorageErrorCode::STORAGE_EXHAUSTED, ErrorSeverity::CRITICAL},

    // Internal — High (shouldn't happen)
    {StorageErrorCode::INTERNAL_ERROR, ErrorSeverity::HIGH},
};

// ============================================================================
// § 2  Error Incident Type Mapping
// ============================================================================

/**
 * @brief Map StorageErrorCode to incident type.
 */
static constexpr std::pair<StorageErrorCode, ErrorIncidentType> kErrorIncidentMap[] = {
    {StorageErrorCode::OK, ErrorIncidentType::INTERNAL_ERROR},

    // WAL — Recovery Fault
    {StorageErrorCode::WAL_WRITE_FAILED, ErrorIncidentType::RECOVERY_FAULT},
    {StorageErrorCode::WAL_SEQUENCE_ERROR, ErrorIncidentType::RECOVERY_FAULT},
    {StorageErrorCode::WAL_CORRUPTED, ErrorIncidentType::RECOVERY_FAULT},

    // MVCC/Transaction — Transaction Conflict
    {StorageErrorCode::TRANSACTION_CONFLICT, ErrorIncidentType::TRANSACTION_CONFLICT},
    {StorageErrorCode::TRANSACTION_TIMEOUT, ErrorIncidentType::TRANSACTION_CONFLICT},
    {StorageErrorCode::VERSION_CHAIN_FULL, ErrorIncidentType::TRANSACTION_CONFLICT},
    {StorageErrorCode::KEY_NOT_FOUND, ErrorIncidentType::TRANSACTION_CONFLICT},

    // Recovery — Recovery Fault
    {StorageErrorCode::RECOVERY_TIMEOUT, ErrorIncidentType::RECOVERY_FAULT},
    {StorageErrorCode::RECOVERY_INCOMPLETE, ErrorIncidentType::RECOVERY_FAULT},
    {StorageErrorCode::CHECKPOINT_FAILED, ErrorIncidentType::RECOVERY_FAULT},

    // Backup/PITR — Backup Failure
    {StorageErrorCode::PITR_INVALID_TIMESTAMP, ErrorIncidentType::BACKUP_FAILURE},
    {StorageErrorCode::BACKUP_CORRUPTED, ErrorIncidentType::BACKUP_FAILURE},
    {StorageErrorCode::BACKUP_LIMIT_EXCEEDED, ErrorIncidentType::STORAGE_PRESSURE},
    {StorageErrorCode::RESTORE_FAILED, ErrorIncidentType::BACKUP_FAILURE},

    // Tiering/Compaction
    {StorageErrorCode::TIERING_MIGRATION_FAILED, ErrorIncidentType::TIERING_FAILURE},
    {StorageErrorCode::COMPACTION_ABORTED, ErrorIncidentType::MAINTENANCE_FAILURE},

    // Capacity — Storage Pressure
    {StorageErrorCode::STORAGE_EXHAUSTED, ErrorIncidentType::STORAGE_PRESSURE},

    // Internal
    {StorageErrorCode::INTERNAL_ERROR, ErrorIncidentType::INTERNAL_ERROR},
};

// ============================================================================
// § 3  Error Name and Description Strings
// ============================================================================

static constexpr std::pair<StorageErrorCode, const char*> kErrorNames[] = {
    {StorageErrorCode::OK, "OK"},
    {StorageErrorCode::WAL_WRITE_FAILED, "WAL_WRITE_FAILED"},
    {StorageErrorCode::WAL_SEQUENCE_ERROR, "WAL_SEQUENCE_ERROR"},
    {StorageErrorCode::WAL_CORRUPTED, "WAL_CORRUPTED"},
    {StorageErrorCode::TRANSACTION_CONFLICT, "TRANSACTION_CONFLICT"},
    {StorageErrorCode::TRANSACTION_TIMEOUT, "TRANSACTION_TIMEOUT"},
    {StorageErrorCode::VERSION_CHAIN_FULL, "VERSION_CHAIN_FULL"},
    {StorageErrorCode::KEY_NOT_FOUND, "KEY_NOT_FOUND"},
    {StorageErrorCode::RECOVERY_TIMEOUT, "RECOVERY_TIMEOUT"},
    {StorageErrorCode::RECOVERY_INCOMPLETE, "RECOVERY_INCOMPLETE"},
    {StorageErrorCode::CHECKPOINT_FAILED, "CHECKPOINT_FAILED"},
    {StorageErrorCode::PITR_INVALID_TIMESTAMP, "PITR_INVALID_TIMESTAMP"},
    {StorageErrorCode::BACKUP_CORRUPTED, "BACKUP_CORRUPTED"},
    {StorageErrorCode::BACKUP_LIMIT_EXCEEDED, "BACKUP_LIMIT_EXCEEDED"},
    {StorageErrorCode::RESTORE_FAILED, "RESTORE_FAILED"},
    {StorageErrorCode::TIERING_MIGRATION_FAILED, "TIERING_MIGRATION_FAILED"},
    {StorageErrorCode::COMPACTION_ABORTED, "COMPACTION_ABORTED"},
    {StorageErrorCode::STORAGE_EXHAUSTED, "STORAGE_EXHAUSTED"},
    {StorageErrorCode::INTERNAL_ERROR, "INTERNAL_ERROR"},
};

static constexpr std::pair<StorageErrorCode, const char*> kErrorDescriptions[] = {
    {StorageErrorCode::OK, "Operation succeeded"},
    {StorageErrorCode::WAL_WRITE_FAILED, "Write-Ahead Log write failed (I/O error, timeout, or disk full)"},
    {StorageErrorCode::WAL_SEQUENCE_ERROR, "WAL sequence number is non-monotonic (implementation bug)"},
    {StorageErrorCode::WAL_CORRUPTED, "WAL file is corrupted beyond recovery"},
    {StorageErrorCode::TRANSACTION_CONFLICT, "Write-write conflict detected; transaction must be retried"},
    {StorageErrorCode::TRANSACTION_TIMEOUT, "Transaction exceeded maximum lifetime"},
    {StorageErrorCode::VERSION_CHAIN_FULL, "MVCC version chain reached maximum length per key"},
    {StorageErrorCode::KEY_NOT_FOUND, "Requested key does not exist"},
    {StorageErrorCode::RECOVERY_TIMEOUT, "Recovery procedure exceeded timeout"},
    {StorageErrorCode::RECOVERY_INCOMPLETE, "Recovery completed with partial WAL entries discarded"},
    {StorageErrorCode::CHECKPOINT_FAILED, "Checkpoint position is invalid or past WAL tail"},
    {StorageErrorCode::PITR_INVALID_TIMESTAMP, "PITR timestamp is in future or outside retention window"},
    {StorageErrorCode::BACKUP_CORRUPTED, "Backup snapshot is corrupted or incomplete"},
    {StorageErrorCode::BACKUP_LIMIT_EXCEEDED, "Maximum concurrent backup limit reached"},
    {StorageErrorCode::RESTORE_FAILED, "Restore operation failed due to I/O or consistency error"},
    {StorageErrorCode::TIERING_MIGRATION_FAILED, "Tier-migration operation failed; value remains in source tier"},
    {StorageErrorCode::COMPACTION_ABORTED, "Compaction was aborted (I/O pressure or shutdown signal)"},
    {StorageErrorCode::STORAGE_EXHAUSTED, "Storage space has been exhausted"},
    {StorageErrorCode::INTERNAL_ERROR, "Unclassified internal storage error"},
};

// ============================================================================
// § 4  Incident Type Name Strings
// ============================================================================

static constexpr std::pair<ErrorIncidentType, const char*> kIncidentTypeNames[] = {
    {ErrorIncidentType::RECOVERY_FAULT, "RECOVERY_FAULT"},
    {ErrorIncidentType::STORAGE_PRESSURE, "STORAGE_PRESSURE"},
    {ErrorIncidentType::TRANSACTION_CONFLICT, "TRANSACTION_CONFLICT"},
    {ErrorIncidentType::TIERING_FAILURE, "TIERING_FAILURE"},
    {ErrorIncidentType::MAINTENANCE_FAILURE, "MAINTENANCE_FAILURE"},
    {ErrorIncidentType::BACKUP_FAILURE, "BACKUP_FAILURE"},
    {ErrorIncidentType::CONCURRENCY_ISSUE, "CONCURRENCY_ISSUE"},
    {ErrorIncidentType::INTERNAL_ERROR, "INTERNAL_ERROR"},
};

// ============================================================================
// § 5  Severity Name Strings
// ============================================================================

static constexpr std::pair<ErrorSeverity, const char*> kSeverityNames[] = {
    {ErrorSeverity::INFO, "INFO"},
    {ErrorSeverity::LOW, "LOW"},
    {ErrorSeverity::MEDIUM, "MEDIUM"},
    {ErrorSeverity::HIGH, "HIGH"},
    {ErrorSeverity::CRITICAL, "CRITICAL"},
};

// ============================================================================
// § 6  Classification Helper Functions
// ============================================================================

[[nodiscard]] static ErrorSeverity lookupSeverity(StorageErrorCode code) noexcept {
    for (const auto& [ec, sev] : kErrorSeverityMap) {
        if (ec == code) {
            return sev;
        }
    }
    return ErrorSeverity::HIGH;  // Default fallback
}

[[nodiscard]] static ErrorIncidentType lookupIncidentType(StorageErrorCode code) noexcept {
    for (const auto& [ec, it] : kErrorIncidentMap) {
        if (ec == code) {
            return it;
        }
    }
    return ErrorIncidentType::INTERNAL_ERROR;  // Default fallback
}

[[nodiscard]] static std::string_view lookupErrorName(StorageErrorCode code) noexcept {
    for (const auto& [ec, name] : kErrorNames) {
        if (ec == code) {
            return name;
        }
    }
    return "UNKNOWN";
}

[[nodiscard]] static std::string_view lookupErrorDescription(StorageErrorCode code) noexcept {
    for (const auto& [ec, desc] : kErrorDescriptions) {
        if (ec == code) {
            return desc;
        }
    }
    return "Unknown error";
}

// ============================================================================
// § 7  Public API Implementation
// ============================================================================

StorageErrorContext classifyErrorIncident(StorageErrorCode code) noexcept {
    StorageErrorContext ctx;
    ctx.error_code = code;
    ctx.message = std::string(lookupErrorDescription(code));
    ctx.incident_type = lookupIncidentType(code);
    ctx.severity = lookupSeverity(code);
    ctx.is_durability_threat = isDurabilityThreat(code);
    ctx.is_retryable = isRetryableConflict(code);

    // Set default retry counts
    if (ctx.is_retryable) {
        ctx.retry_count = 3;  // Default exponential backoff retries
    }

    return ctx;
}

StorageErrorCode mapErrorMessage(std::string_view error_message) noexcept {
    std::string msg_lower = std::string(error_message);
    std::transform(msg_lower.begin(), msg_lower.end(), msg_lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });

    // Map common error patterns
    if (msg_lower.find("disk full") != std::string::npos ||
        msg_lower.find("no space") != std::string::npos ||
        msg_lower.find("exhausted") != std::string::npos) {
        return StorageErrorCode::STORAGE_EXHAUSTED;
    }

    if (msg_lower.find("corrupted") != std::string::npos ||
        msg_lower.find("checksum") != std::string::npos) {
        return StorageErrorCode::WAL_CORRUPTED;
    }

    if (msg_lower.find("timeout") != std::string::npos) {
        return StorageErrorCode::RECOVERY_TIMEOUT;
    }

    if (msg_lower.find("conflict") != std::string::npos) {
        return StorageErrorCode::TRANSACTION_CONFLICT;
    }

    if (msg_lower.find("not found") != std::string::npos ||
        msg_lower.find("key not exist") != std::string::npos) {
        return StorageErrorCode::KEY_NOT_FOUND;
    }

    if (msg_lower.find("io error") != std::string::npos ||
        msg_lower.find("write failed") != std::string::npos) {
        return StorageErrorCode::WAL_WRITE_FAILED;
    }

    // Fallback
    return StorageErrorCode::INTERNAL_ERROR;
}

StorageErrorContext buildErrorContext(
    StorageErrorCode code,
    std::string_view operation_context,
    std::string_view affected_resource) noexcept {
    auto ctx = classifyErrorIncident(code);
    ctx.operation_context = std::string(operation_context);
    ctx.affected_resource = std::string(affected_resource);

    // Add recovery suggestions based on error type
    switch (code) {
        case StorageErrorCode::WAL_WRITE_FAILED:
            ctx.recovery_suggestion =
                "Check disk health, verify permissions, and ensure sufficient space. "
                "Consider failover if primary disk is degraded.";
            break;

        case StorageErrorCode::STORAGE_EXHAUSTED:
            ctx.recovery_suggestion =
                "Free disk space by archiving/deleting old data or enable tiering to "
                "cold storage. Configure storage alerts for 80%+ utilization.";
            break;

        case StorageErrorCode::RECOVERY_INCOMPLETE:
            ctx.recovery_suggestion =
                "Partial WAL entries were detected. Review logs, verify backup integrity, "
                "and consider point-in-time restore if data consistency is compromised.";
            break;

        case StorageErrorCode::TRANSACTION_CONFLICT:
            ctx.recovery_suggestion = "Retry transaction with exponential backoff. "
                                       "If persistent, check for application-level deadlock.";
            break;

        case StorageErrorCode::BACKUP_CORRUPTED:
            ctx.recovery_suggestion =
                "Backup is corrupted. Verify backup file integrity (MD5/hash). "
                "If failed, restore from earlier backup or use PITR with WAL.";
            break;

        case StorageErrorCode::TIERING_MIGRATION_FAILED:
            ctx.recovery_suggestion =
                "Tiering migration failed but data remains in source tier. "
                "Retry migration or verify cloud backend connectivity.";
            break;

        case StorageErrorCode::RECOVERY_TIMEOUT:
            ctx.recovery_suggestion =
                "Recovery exceeded timeout. Check for I/O bottlenecks, consider "
                "increasing kRecoveryHardTimeout or upgrading storage hardware.";
            break;

        default:
            break;
    }

    return ctx;
}

void emitDiagnosticEvent(const StorageErrorContext& context) noexcept {
    std::string severity_str(severityName(context.severity));
    std::string incident_str(incidentTypeName(context.incident_type));
    std::string code_str(errorCodeName(context.error_code));

    std::string log_msg = std::format(
        "[STORAGE_ERROR] code={} incident={} severity={} op={} resource={} "
        "msg={} recovery={}",
        code_str, incident_str, severity_str, context.operation_context,
        context.affected_resource, context.message, context.recovery_suggestion);

    // Route to appropriate log level
    if (context.severity == ErrorSeverity::CRITICAL) {
        THEMIS_ERROR("{}", log_msg);
    } else if (context.severity == ErrorSeverity::HIGH) {
        THEMIS_WARN("{}", log_msg);
    } else {
        THEMIS_INFO("{}", log_msg);
    }

    // TODO: Emit to metrics/audit channel for operational dashboards
}

void emitRecoveryFaultEvent(
    std::string_view fault_type,
    std::uint64_t affected_checkpoint_seq,
    std::uint64_t recovered_entries,
    std::string_view suggestion) noexcept {
    std::string msg = std::format(
        "[RECOVERY_FAULT] type={} checkpoint_seq={} recovered_entries={} "
        "recovery_suggestion={}",
        fault_type, affected_checkpoint_seq, recovered_entries,
        suggestion.empty() ? "(none)" : suggestion);

    THEMIS_WARN("{}", msg);
    // TODO: Emit to recovery-specific diagnostic channel
}

void emitStoragePressureEvent(
    std::string_view pressure_type,
    std::uint64_t available_bytes,
    std::uint64_t required_bytes,
    int escalation_level) noexcept {
    std::string msg = std::format(
        "[STORAGE_PRESSURE] type={} available={} required={} escalation_level={}",
        pressure_type, available_bytes, required_bytes, escalation_level);

    if (escalation_level >= 4) {
        THEMIS_ERROR("{}", msg);
    } else if (escalation_level >= 3) {
        THEMIS_WARN("{}", msg);
    } else {
        THEMIS_INFO("{}", msg);
    }
    // TODO: Emit to capacity management dashboard
}

std::string_view errorCodeName(StorageErrorCode code) noexcept {
    return lookupErrorName(code);
}

std::string_view errorCodeDescription(StorageErrorCode code) noexcept {
    return lookupErrorDescription(code);
}

std::string_view incidentTypeName(ErrorIncidentType type) noexcept {
    for (const auto& [it, name] : kIncidentTypeNames) {
        if (it == type) {
            return name;
        }
    }
    return "UNKNOWN";
}

std::string_view severityName(ErrorSeverity severity) noexcept {
    for (const auto& [sev, name] : kSeverityNames) {
        if (sev == severity) {
            return name;
        }
    }
    return "UNKNOWN";
}

}  // namespace storage
}  // namespace themis
