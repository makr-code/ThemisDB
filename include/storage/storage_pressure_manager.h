/*
 * ThemisDB | File: storage_pressure_manager.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 3 — Error Handling & Edge Cases
 * Purpose: Storage capacity management and pressure-related error handling.
 */

/**
 * @file storage_pressure_manager.h
 * @brief Storage capacity management and pressure mitigation.
 *
 * This header provides mechanisms for:
 *   - Detecting and classifying storage pressure (warnings, alerts, critical)
 *   - Implementing fail-safe behavior under capacity constraints
 *   - Suggesting mitigation actions (archiving, tiering, cleanup)
 *   - Preventing cascading failures under extreme pressure
 *
 * Phase 3 Objective: Standardize fail-safe behavior for storage pressure
 * errors (STORAGE_EXHAUSTED, BACKUP_LIMIT_EXCEEDED) by providing predictable,
 * operator-aware capacity management.
 *
 * @see include/storage/storage_api_contract.h — Error taxonomy
 * @see include/storage/storage_error_diagnostics.h — Error classification
 * @see src/storage/ROADMAP.md — Phase 3 items
 */

#pragma once

#include "storage/storage_api_contract.h"
#include "storage/storage_error_diagnostics.h"

#include <chrono>
#include <cstdint>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace themis {
namespace storage {

// ============================================================================
// § 1  Pressure Escalation Levels
// ============================================================================

/**
 * @brief Storage pressure escalation levels for capacity constraints.
 *
 * Maps percentage-based thresholds to operational alerts and recovery strategies.
 */
enum class PressureEscalationLevel : int {
    /// Utilization 0-75%: Normal operation, no pressure.
    NORMAL = 0,

    /// Utilization 75-85%: Warning level; operator attention recommended.
    WARNING = 1,

    /// Utilization 85-95%: Alert level; compaction/archiving recommended.
    ALERT = 2,

    /// Utilization 95-99%: Critical level; new writes may be rejected.
    CRITICAL = 3,

    /// Utilization >= 99%: Exhausted; all writes must be rejected.
    EXHAUSTED = 4,
};

// ============================================================================
// § 2  Storage Capacity Metrics
// ============================================================================

/**
 * @brief Snapshot of storage capacity and pressure metrics.
 *
 * Used for decision-making about write acceptance, operation prioritization,
 * and mitigation strategy selection.
 */
struct StorageCapacityMetrics {
    /// Total storage capacity in bytes (not all usable for user data).
    std::uint64_t total_capacity_bytes{0};

    /// Bytes currently used (data, WAL, backups, temp files, etc.).
    std::uint64_t used_bytes{0};

    /// Bytes available for new writes.
    std::uint64_t available_bytes{0};

    /// Bytes reserved for system operations (crash recovery, checkpoint, etc.).
    std::uint64_t reserved_bytes{0};

    /// Current pressure escalation level.
    PressureEscalationLevel escalation_level{PressureEscalationLevel::NORMAL};

    /// Percentage of capacity in use (0-100).
    double utilization_percent() const noexcept {
        return total_capacity_bytes == 0 ? 0.0
                                          : (100.0 * used_bytes) / total_capacity_bytes;
    }

    /// Projected time to capacity at current write rate (milliseconds).
    /// Returns -1 if write rate is zero or available space (minus reserve) is
    /// insufficient for meaningful projection (i.e., already at/below reserve).
    std::int64_t time_to_exhaustion_ms(std::uint64_t write_rate_bytes_per_sec) const
        noexcept {
        if (write_rate_bytes_per_sec == 0 || available_bytes <= reserved_bytes) {
            return -1;
        }
        std::uint64_t bytes_until_exhausted = available_bytes - reserved_bytes;
        std::uint64_t seconds_until_exhausted =
            bytes_until_exhausted / write_rate_bytes_per_sec + 1;
        if (seconds_until_exhausted > 31536000) {  // > 1 year
            return -1;
        }
        return static_cast<std::int64_t>(seconds_until_exhausted * 1000);
    }
};

// ============================================================================
// § 3  Storage Pressure Manager
// ============================================================================

/**
 * @brief Centralized manager for storage capacity and pressure-related errors.
 *
 * Provides:
 *   - Capacity monitoring and escalation tracking
 *   - Write acceptance decisions under pressure
 *   - Fail-safe behavior for STORAGE_EXHAUSTED and BACKUP_LIMIT_EXCEEDED
 *   - Diagnostic event emission for operator awareness
 *   - Mitigation strategy recommendations
 *
 * Thread-safe: Safe to call concurrently from multiple storage operations.
 */
class StoragePressureManager {
public:
    /// Constructor: initializes with default capacity thresholds.
    explicit StoragePressureManager() = default;

    /// Non-copyable.
    StoragePressureManager(const StoragePressureManager&) = delete;
    StoragePressureManager& operator=(const StoragePressureManager&) = delete;

    /// Move-enabled.
    StoragePressureManager(StoragePressureManager&&) noexcept = default;
    StoragePressureManager& operator=(StoragePressureManager&&) noexcept = default;

    ~StoragePressureManager() = default;

    // ── Capacity Queries ───────────────────────────────────────────────────

    /**
     * @brief Get current storage capacity metrics.
     *
     * @return StorageCapacityMetrics with current usage and pressure state
     *
     * Thread-safe: Returns consistent snapshot of current metrics.
     */
    [[nodiscard]] StorageCapacityMetrics getCapacityMetrics() const noexcept;

    /**
     * @brief Check if storage has sufficient space for a write operation.
     *
     * Returns false if:
     *   - Available space < requested_bytes + reserved_bytes
     *   - Pressure level is EXHAUSTED
     *   - Operation would cross reject_at_utilization threshold
     *
     * @param requested_bytes Bytes needed for write operation
     * @return true if write should be accepted, false if should be rejected
     */
    [[nodiscard]] bool canAcceptWrite(std::uint64_t requested_bytes) const noexcept;

    /**
     * @brief Check if storage supports a backup operation.
     *
     * Returns false if:
     *   - Concurrent backup limit (kMaxConcurrentBackups) reached
     *   - Available space < backup_size_estimate + reserved_bytes
     *   - Pressure level is CRITICAL or EXHAUSTED
     *
     * @param backup_size_estimate Expected backup size in bytes
     * @param active_backup_count Current number of in-flight backups
     * @return true if backup can be started, false otherwise
     */
    [[nodiscard]] bool canStartBackup(std::uint64_t backup_size_estimate,
                                       int active_backup_count) const noexcept;

    // ── Pressure Classification ────────────────────────────────────────────

    /**
     * @brief Update storage capacity metrics and determine pressure level.
     *
     * Should be called periodically or when capacity changes detected.
     *
     * @param total_capacity Total storage capacity in bytes
     * @param used_bytes Bytes currently used
     * @return StorageCapacityMetrics with updated escalation_level
     *
     * Escalation levels are determined by utilization percentage:
     *   - 0-75%: NORMAL
     *   - 75-85%: WARNING
     *   - 85-95%: ALERT
     *   - 95-99%: CRITICAL
     *   - >=99%: EXHAUSTED
     */
    [[nodiscard]] StorageCapacityMetrics updateCapacity(std::uint64_t total_capacity,
                                                         std::uint64_t used_bytes) noexcept;

    /**
     * @brief Report a storage exhaustion event.
     *
     * Called when STORAGE_EXHAUSTED error is detected. Emits diagnostic event
     * and updates pressure state.
     *
     * Guaranteed behavior:
     *   - Emits CRITICAL severity diagnostic
     *   - Sets escalation level to EXHAUSTED
     *   - Suggests mitigation (free space, tiering, archiving)
     *   - Triggers operator alert
     *
     * @param operation_context Context of the operation that failed
     *                          (e.g., "write(key=foo)", "checkpoint()")
     * @param total_capacity Total storage capacity
     * @param used_bytes Current usage
     * @return StorageErrorContext with CRITICAL severity
     */
    [[nodiscard]] StorageErrorContext reportStorageExhausted(
        std::string_view operation_context,
        std::uint64_t total_capacity,
        std::uint64_t used_bytes) noexcept;

    /**
     * @brief Report backup limit exceeded event.
     *
     * Called when BACKUP_LIMIT_EXCEEDED error would be returned. Emits
     * diagnostic event.
     *
     * Guaranteed behavior:
     *   - Emits HIGH severity diagnostic
     *   - Suggests waiting for backup to complete
     *   - Provides information about concurrent backup limit
     *
     * @param active_backup_count Current number of active backups
     * @param max_concurrent_backups Maximum allowed concurrent backups
     * @return StorageErrorContext with HIGH severity
     */
    [[nodiscard]] StorageErrorContext reportBackupLimitExceeded(
        int active_backup_count,
        int max_concurrent_backups) noexcept;

    // ── Configuration ──────────────────────────────────────────────────────

    /**
     * @brief Set the utilization threshold at which writes should be rejected.
     *
     * Default: 99% (equivalent to PressureEscalationLevel::EXHAUSTED).
     *
     * Writes requested when utilization >= this threshold will be rejected
     * with STORAGE_EXHAUSTED.
     *
     * @param percent Utilization percentage (0-100)
     */
    void setRejectWritesAtUtilization(double percent) noexcept {
        reject_writes_at_utilization_ = percent;
    }

    /**
     * @brief Set the utilization threshold at which new backups should be rejected.
     *
     * Default: 95% (equivalent to PressureEscalationLevel::CRITICAL).
     *
     * Backup requests when utilization >= this threshold will be rejected
     * with BACKUP_LIMIT_EXCEEDED (or similar).
     *
     * @param percent Utilization percentage (0-100)
     */
    void setRejectBackupsAtUtilization(double percent) noexcept {
        reject_backups_at_utilization_ = percent;
    }

    /**
     * @brief Set bytes reserved for system operations (crash recovery, temp files).
     *
     * Default: 5% of total capacity.
     *
     * Available space is computed as: total - used - reserved. This reserve
     * ensures system operations don't fail unexpectedly under pressure.
     *
     * @param reserved_bytes Bytes to reserve for system use
     */
    void setReservedCapacity(std::uint64_t reserved_bytes) noexcept {
        reserved_capacity_bytes_ = reserved_bytes;
    }

private:
    mutable std::mutex mtx_;

    // Capacity tracking
    std::uint64_t total_capacity_bytes_{0};
    std::uint64_t used_bytes_{0};
    PressureEscalationLevel current_level_{PressureEscalationLevel::NORMAL};

    // Configuration thresholds
    double reject_writes_at_utilization_{99.0};      // 99% = EXHAUSTED
    double reject_backups_at_utilization_{95.0};     // 95% = CRITICAL
    std::uint64_t reserved_capacity_bytes_{0};       // 5% computed on first update

    [[nodiscard]] PressureEscalationLevel classifyPressure(double utilization_percent) const
        noexcept;
};

}  // namespace storage
}  // namespace themis

