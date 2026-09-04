// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file storage_pressure_manager.cpp
 * @brief Implementation of storage capacity management and pressure handling.
 *
 * Provides fail-safe behavior for capacity constraints and pressure-related
 * errors (STORAGE_EXHAUSTED, BACKUP_LIMIT_EXCEEDED).
 *
 * @see include/storage/storage_pressure_manager.h
 * @see include/storage/storage_api_contract.h
 */

#include "storage/storage_pressure_manager.h"

#include <chrono>
#include <format>
#include <mutex>
#include <string>

// Logging support
#include "utils/logger.h"

namespace themis {
namespace storage {

// ============================================================================
// § 1  Utility Functions
// ============================================================================

/**
 * @brief Classify pressure level based on utilization percentage.
 */
[[nodiscard]] PressureEscalationLevel classifyPressureLevel([[maybe_unused]] double utilization_percent) noexcept {
    if (utilization_percent < 75.0) {
        return PressureEscalationLevel::NORMAL;
    } else if (utilization_percent < 85.0) {
        return PressureEscalationLevel::WARNING;
    } else if (utilization_percent < 95.0) {
        return PressureEscalationLevel::ALERT;
    } else if (utilization_percent < 99.0) {
        return PressureEscalationLevel::CRITICAL;
    } else {
        return PressureEscalationLevel::EXHAUSTED;
    }
}

// ============================================================================
// § 2  StoragePressureManager Implementation
// ============================================================================

StorageCapacityMetrics StoragePressureManager::getCapacityMetrics() const noexcept {
    std::lock_guard<std::mutex> lk(mtx_);
    StorageCapacityMetrics metrics;
    metrics.total_capacity_bytes = total_capacity_bytes_;
    metrics.used_bytes = used_bytes_;
    metrics.reserved_bytes = reserved_capacity_bytes_;
    metrics.available_bytes = total_capacity_bytes_ > used_bytes_
                                  ? total_capacity_bytes_ - used_bytes_
                                  : 0;
    metrics.escalation_level = current_level_;
    return metrics;
}

bool StoragePressureManager::canAcceptWrite(std::uint64_t requested_bytes) const noexcept {
    std::lock_guard<std::mutex> lk(mtx_);

    // Fail-closed: reject writes if already exhausted
    if (current_level_ == PressureEscalationLevel::EXHAUSTED) {
        return false;
    }

    // Check utilization threshold
    double util_percent =
        total_capacity_bytes_ == 0 ? 0.0 : (100.0 * used_bytes_) / total_capacity_bytes_;
    if (util_percent >= reject_writes_at_utilization_) {
        return false;
    }

    // Check available space including reserved capacity
    std::uint64_t available_for_writes = total_capacity_bytes_ > used_bytes_
                                             ? total_capacity_bytes_ - used_bytes_
                                             : 0;
    if (available_for_writes < requested_bytes + reserved_capacity_bytes_) {
        return false;
    }

    return true;
}

bool StoragePressureManager::canStartBackup(std::uint64_t backup_size_estimate,
                                             int active_backup_count) const noexcept {
    std::lock_guard<std::mutex> lk(mtx_);

    // Check concurrent backup limit
    if (active_backup_count >= kMaxConcurrentBackups) {
        return false;
    }

    // Fail-closed: reject backups if pressure is high
    if (current_level_ == PressureEscalationLevel::CRITICAL ||
        current_level_ == PressureEscalationLevel::EXHAUSTED) {
        return false;
    }

    // Check available space for backup
    double util_percent =
        total_capacity_bytes_ == 0 ? 0.0 : (100.0 * used_bytes_) / total_capacity_bytes_;
    if (util_percent >= reject_backups_at_utilization_) {
        return false;
    }

    std::uint64_t available = total_capacity_bytes_ > used_bytes_
                                  ? total_capacity_bytes_ - used_bytes_
                                  : 0;
    if (available < backup_size_estimate + reserved_capacity_bytes_) {
        return false;
    }

    return true;
}

StorageCapacityMetrics StoragePressureManager::updateCapacity(std::uint64_t total_capacity,
                                                               std::uint64_t used_bytes) noexcept {
    std::lock_guard<std::mutex> lk(mtx_);

    total_capacity_bytes_ = total_capacity;
    used_bytes_ = used_bytes;

    // Initialize reserved capacity to 5% on first update
    if (reserved_capacity_bytes_ == 0 && total_capacity > 0) {
        reserved_capacity_bytes_ = total_capacity / 20;  // 5%
    }

    // Recompute pressure level
    double util_percent =
        total_capacity == 0 ? 0.0 : (100.0 * used_bytes) / total_capacity;
    auto old_level = current_level_;
    current_level_ = classifyPressureLevel(util_percent);

    // Emit diagnostic if pressure escalated
    if (current_level_ > old_level) {
        std::string msg =
            std::format("Storage pressure escalated from {} to {} (utilization: {:.1f}%)",
                        static_cast<int>(old_level), static_cast<int>(current_level_),
                        util_percent);
        THEMIS_WARN("{}", msg);

        int escalation_level = static_cast<int>(current_level_);
        emitStoragePressureEvent("pressure_escalation", total_capacity - used_bytes,
                                 0,  // no specific operation
                                 escalation_level);
    }

    StorageCapacityMetrics metrics;
    metrics.total_capacity_bytes = total_capacity;
    metrics.used_bytes = used_bytes;
    metrics.reserved_bytes = reserved_capacity_bytes_;
    metrics.available_bytes = total_capacity > used_bytes ? total_capacity - used_bytes : 0;
    metrics.escalation_level = current_level_;
    return metrics;
}

StorageErrorContext StoragePressureManager::reportStorageExhausted(
    std::string_view operation_context,
    std::uint64_t total_capacity,
    std::uint64_t used_bytes) noexcept {
    // Update metrics
    auto metrics = updateCapacity(total_capacity, used_bytes);

    StorageErrorContext ctx;
    ctx.error_code = StorageErrorCode::STORAGE_EXHAUSTED;
    ctx.severity = ErrorSeverity::CRITICAL;
    ctx.incident_type = ErrorIncidentType::STORAGE_PRESSURE;
    ctx.operation_context = std::string(operation_context);
    ctx.affected_resource = "storage";
    ctx.is_durability_threat = true;
    ctx.is_retryable = false;

    double util_percent = metrics.utilization_percent();
    ctx.message = std::format(
        "Storage exhausted: {} bytes used of {} total ({:.1f}%). "
        "Operation '{}' cannot be completed.",
        used_bytes, total_capacity, util_percent, operation_context);

    ctx.recovery_suggestion =
        "Immediate action required: Free disk space by deleting/archiving old data, "
        "enable tiering to cold storage, or add storage capacity. "
        "Without intervention, all writes will be rejected.";

    // Emit diagnostic event
    emitStoragePressureEvent("storage_exhausted", metrics.available_bytes,
                             0,  // no specific allocation
                             static_cast<int>(metrics.escalation_level));
    emitDiagnosticEvent(ctx);

    THEMIS_ERROR("Storage: {}", ctx.message);
    return ctx;
}

StorageErrorContext StoragePressureManager::reportBackupLimitExceeded(
    int active_backup_count,
    int max_concurrent_backups) noexcept {
    StorageErrorContext ctx;
    ctx.error_code = StorageErrorCode::BACKUP_LIMIT_EXCEEDED;
    ctx.severity = ErrorSeverity::HIGH;
    ctx.incident_type = ErrorIncidentType::STORAGE_PRESSURE;
    ctx.affected_resource = "backup";
    ctx.is_durability_threat = false;
    ctx.is_retryable = true;
    ctx.retry_count = 5;  // Allow retries with backoff

    ctx.message = std::format(
        "Backup limit exceeded: {} of {} concurrent backups already active.",
        active_backup_count, max_concurrent_backups);

    ctx.recovery_suggestion = std::format(
        "Wait for {} backup(s) to complete before starting new backup. "
        "Monitor backup_active_count metric.",
        active_backup_count);

    // Emit diagnostic event
    emitStoragePressureEvent("backup_limit_exceeded", 0, 0, 2);  // Alert level
    emitDiagnosticEvent(ctx);

    THEMIS_INFO("Backup: {}", ctx.message);
    return ctx;
}

PressureEscalationLevel StoragePressureManager::classifyPressure(
    double utilization_percent) const noexcept {
    return classifyPressureLevel(utilization_percent);
}

}  // namespace storage
}  // namespace themis
