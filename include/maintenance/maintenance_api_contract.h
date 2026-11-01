/**
 * @file maintenance_api_contract.h
 * @brief Compatibility helper types for maintenance contract tests and benchmarks.
 * @version 1.0.0
 *
 * @note This header is not the runtime maintenance API contract. The production
 *       orchestrator uses MaintenanceScheduleEntry and Result<T>/Error; it does
 *       not return MaintenanceError, MaintenanceScheduleDescriptor, or
 *       MaintenanceHealthSnapshot. The declarations below remain available to
 *       helper tests and synthetic benchmarks only.
 * @see maintenance/database_maintenance_orchestrator.h
 * @see maintenance/maintenance_schedule.h
 * @see maintenance/maintenance_health_report.h
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace maintenance {

// ============================================================================
// § 1  Error taxonomy
// ============================================================================

/**
 * @brief Compatibility error values used by maintenance helper tests.
 *
 * These values are not returned by the production orchestrator. Runtime
 * operations expose repository ErrorCode values through Result<T>.
 */
enum class MaintenanceError : int32_t {
    kSuccess               = 0,
    kScheduleNotFound      = 8100, ///< Referenced schedule ID does not exist.
    kHandlerNotRegistered  = 8101, ///< No handler registered for task type.
    kPersistenceFailed     = 8102, ///< Schedule store write or read failed.
    kExecutionTimeout      = 8103, ///< Task execution exceeded time budget.
    kConcurrentModification = 8104, ///< Concurrent modification detected; retry.
    kInvalidSchedule       = 8105, ///< Schedule descriptor fails structural checks.
    kOrchestratorDegraded  = 8106, ///< Orchestrator health check failed.
    kInternalError         = 8107, ///< Unclassified internal error.
    kSkippedConcurrent     = 8108, ///< Schedule already in-flight; this invocation was skipped.
    kPersistenceCorrupt    = 8109, ///< Persisted data is corrupt or truncated; cannot be loaded.
    kChurnLimitExceeded    = 8110, ///< Helper-test value; runtime uses ERR_RATE_LIMIT_EXCEEDED.
};

// ============================================================================
// § 2  Schedule descriptor constraints
// ============================================================================

/// Helper-contract limit only; not enforced by the production orchestrator.
inline constexpr std::size_t kMaxScheduleIdBytes = 256;

/// Helper-contract limit only; not enforced by the production orchestrator.
inline constexpr std::size_t kMaxSchedulesPerOrchestrator = 65536;

/// Helper-contract value only; not enforced as a runtime task timeout.
inline constexpr std::chrono::minutes kMaxTaskExecutionDuration{60};

/// Helper-contract value only; not enforced by the production schedule API.
inline constexpr std::chrono::seconds kMinRecurringIntervalSeconds{10};

// ============================================================================
// § 3  Supporting structs
// ============================================================================

/**
 * @brief Compact descriptor for a maintenance schedule entry.
 *
 * This is a helper value type, not the descriptor accepted by the production
 * orchestrator or schedule store. Its fields are not runtime-validated.
 */
struct MaintenanceScheduleDescriptor {
    /// Unique schedule identifier (non-empty, max kMaxScheduleIdBytes).
    std::string   schedule_id;

    /// Task type token used to look up the registered handler.
    std::string   task_type;

    /// Wall-clock interval between consecutive executions (must be >= kMinRecurringIntervalSeconds).
    std::chrono::seconds interval{0};

    /// Whether this schedule is currently active.
    bool          enabled{false};

    /// Maximum execution time budget before kExecutionTimeout is raised.
    std::chrono::seconds timeout{0};

    /**
     * @brief Maximum number of schedule changes (create/update/patch/delete)
     *        permitted per interval window.  0 means disabled (no limit).
     *
     * This field does not configure the production orchestrator. Runtime churn
     * enforcement is configured on MaintenanceScheduleEntry.
     */
    uint32_t      max_schedule_changes_per_interval{0};
};

/**
 * @brief Compatibility health value type for helper tests.
 *
 * The production orchestrator returns MaintenanceHealthReport instead; this
 * snapshot is not used to gate dispatch.
 */
struct MaintenanceHealthSnapshot {
    bool          is_healthy{false};       ///< Overall orchestrator health.
    uint32_t      active_schedule_count{0}; ///< Currently active schedule count.
    uint32_t      failed_execution_count{0}; ///< Total failed executions since restart.
    std::string   last_error_message;      ///< Most recent error description (may be empty).
};

// ============================================================================
// § 4  Fail-closed contract for degraded orchestrator
// ============================================================================

/**
 * @brief Classifies compatibility error values for helper tests.
 *
 * The production orchestrator does not call this predicate, and it does not
 * enforce fail-closed dispatch behavior.
 */
[[nodiscard]] inline constexpr bool isMaintenanceFailClosed(MaintenanceError e) noexcept {
    return e == MaintenanceError::kOrchestratorDegraded
        || e == MaintenanceError::kInternalError
        || e == MaintenanceError::kPersistenceFailed
        || e == MaintenanceError::kPersistenceCorrupt;
}

} // namespace maintenance
} // namespace themis
