/*
 * ThemisDB | File: maintenance_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen maintenance module contract semantics for the active v1.x major line.
 */

/**
 * @file maintenance_api_contract.h
 * @brief Frozen maintenance module API contract for schedule, execution, and persistence.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines the normative contract for the maintenance module covering schedule
 * orchestration, task-handler dispatch, persistence/reload, and registry-driven
 * setup.  All maintenance module components must honour these contracts within
 * the current v1.x major release line.
 *
 * ## §API Contracts
 *
 * Key behavioural invariants:
 *   1. A schedule may only be executed if its handler is registered in the task
 *      registry; missing handlers produce kHandlerNotRegistered, never a crash.
 *   2. Persistence operations are atomic from the caller's perspective: a failed
 *      write leaves the existing persisted state intact (kPersistenceFailed).
 *   3. Schedule reload on restart MUST reconstruct an identical in-memory
 *      representation from the persisted state.
 *   4. The orchestrator propagates kOrchestratorDegraded and halts further
 *      dispatches when the underlying infrastructure is unhealthy.
 *   5. All timing operations use monotonic clocks; wall-clock adjustments do
 *      not affect schedule correctness.
 *
 * ## §Error Taxonomy
 *
 * | Code  | Constant                  | Meaning                                      |
 * |-------|---------------------------|----------------------------------------------|
 * | 0     | kSuccess                  | Operation completed without error            |
 * | 8100  | kScheduleNotFound         | Referenced schedule ID does not exist        |
 * | 8101  | kHandlerNotRegistered     | No handler registered for task type          |
 * | 8102  | kPersistenceFailed        | Schedule store write or read operation failed|
 * | 8103  | kExecutionTimeout         | Task execution exceeded allowed time budget  |
 * | 8104  | kConcurrentModification   | Schedule modified concurrently; retry safe   |
 * | 8105  | kInvalidSchedule          | Schedule descriptor fails structural checks  |
 * | 8106  | kOrchestratorDegraded     | Orchestrator health check failed; deny all   |
 * | 8107  | kInternalError            | Unclassified internal error; always deny     |
 *
 * ## §Threading Guarantees
 *
 * - ScheduleStore implementations MUST be safe for concurrent reads; writes
 *   are serialized by the orchestrator's internal mutex.
 * - Handler registration is single-threaded at startup; subsequent reads are
 *   lock-free (read-only map).
 * - kConcurrentModification is a retriable error; callers MUST back off and
 *   retry with an appropriate jitter strategy.
 *
 * ## §Contract Freeze
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/maintenance/ROADMAP.md — Phase 1 item
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
 * @brief Canonical error codes for the maintenance module.
 *
 * All maintenance module operations return or throw with one of these codes.
 * Values are in the reserved range [8100, 8199].
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
};

// ============================================================================
// § 2  Schedule descriptor constraints
// ============================================================================

/// Maximum allowed schedule identifier length in bytes.
inline constexpr std::size_t kMaxScheduleIdBytes = 256;

/// Maximum number of schedules manageable by a single orchestrator instance.
inline constexpr std::size_t kMaxSchedulesPerOrchestrator = 65536;

/// Maximum allowed single-task execution wall-clock duration.
inline constexpr std::chrono::minutes kMaxTaskExecutionDuration{60};

/// Minimum permitted schedule interval for recurring tasks.
inline constexpr std::chrono::seconds kMinRecurringIntervalSeconds{10};

// ============================================================================
// § 3  Supporting structs
// ============================================================================

/**
 * @brief Compact descriptor for a maintenance schedule entry.
 *
 * Passed across the orchestrator / persistence boundary.  All fields must be
 * populated before submission; zero-initialised fields are rejected with
 * MaintenanceError::kInvalidSchedule.
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
};

/**
 * @brief Lightweight health snapshot returned by the orchestrator.
 *
 * A degraded report (is_healthy == false) MUST prevent further task dispatch.
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
 * @brief Returns true when the given error mandates fail-closed denial of dispatch.
 *
 * Callers MUST check this before scheduling further work when handling an error.
 */
[[nodiscard]] inline constexpr bool isMaintenanceFailClosed(MaintenanceError e) noexcept {
    return e == MaintenanceError::kOrchestratorDegraded
        || e == MaintenanceError::kInternalError
        || e == MaintenanceError::kPersistenceFailed;
}

} // namespace maintenance
} // namespace themis
