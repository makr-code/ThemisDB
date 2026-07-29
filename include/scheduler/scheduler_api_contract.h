/*
 * ThemisDB | File: scheduler_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen scheduler module contract semantics for the active v1.x major line.
 */

/**
 * @file scheduler_api_contract.h
 * @brief Frozen scheduler task lifecycle and coordination API contract for v1.x.
 * @version 1.0.0
 *
 * ## §Purpose
 *
 * Defines the normative contract for the scheduler module covering task
 * registration, execution dispatch, lifecycle state management, distributed
 * coordination, retention policies, and anomaly/trigger observability.
 *
 * ## §API Contracts
 *
 * Key behavioural invariants:
 *   1. Task registration is idempotent only if the registration descriptor is
 *      byte-identical to the existing entry; divergent re-registration produces
 *      kTaskAlreadyExists.
 *   2. Execution dispatch is fail-closed: if the coordination layer is
 *      unavailable, tasks are not dispatched and kCoordinationError is raised.
 *   3. Retention limits are enforced before new results are written; an
 *      attempt to store results when at the limit produces kRetentionLimitExceeded.
 *   4. Trigger predicates are evaluated atomically; partial trigger evaluations
 *      that fail produce kTriggerInvalid and do not modify task state.
 *   5. Anomaly detection alerts are advisory; they do not block task execution
 *      but are surfaced via the kAnomalyDetected error code in observability
 *      callbacks.
 *
 * ## §Error Taxonomy
 *
 * | Code  | Constant                  | Meaning                                       |
 * |-------|---------------------------|-----------------------------------------------|
 * | 0     | kSuccess                  | Operation completed without error             |
 * | 8400  | kTaskNotFound             | Referenced task ID not present in scheduler   |
 * | 8401  | kTaskAlreadyExists        | Conflicting task ID already registered        |
 * | 8402  | kExecutionFailed          | Task execution returned a non-success status  |
 * | 8403  | kCoordinationError        | Distributed coordination layer unavailable    |
 * | 8404  | kRetentionLimitExceeded   | Result store retention cap reached            |
 * | 8405  | kTriggerInvalid           | Trigger predicate failed structural checks    |
 * | 8406  | kAnomalyDetected          | Anomaly detector flagged task behaviour       |
 * | 8407  | kInternalError            | Unclassified internal error; always deny      |
 *
 * ## §Threading Guarantees
 *
 * - Task registration and lookup are thread-safe; the registry uses a shared
 *   mutex (read-mostly).
 * - Execution dispatch is serialized per task ID to prevent concurrent runs of
 *   the same task.
 * - Anomaly detection callbacks are delivered on a dedicated background thread
 *   and MUST NOT re-enter the task scheduler.
 *
 * ## §Contract Freeze
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/scheduler/ROADMAP.md — Phase 1 item
 */

#pragma once

#include <cstdint>
#include <string>
#include <chrono>

namespace themis {
namespace scheduler {

// ============================================================================
// § 1  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the scheduler module.
 *
 * All scheduler operations return or throw with one of these codes.
 * Values are in the reserved range [8400, 8499].
 */
enum class SchedulerError : int32_t {
    kSuccess                = 0,
    kTaskNotFound           = 8400, ///< Task ID not present in scheduler.
    kTaskAlreadyExists      = 8401, ///< Conflicting task ID already registered.
    kExecutionFailed        = 8402, ///< Task execution returned non-success.
    kCoordinationError      = 8403, ///< Distributed coordination unavailable.
    kRetentionLimitExceeded = 8404, ///< Result store retention cap reached.
    kTriggerInvalid         = 8405, ///< Trigger predicate fails structural checks.
    kAnomalyDetected        = 8406, ///< Anomaly detector flagged task behaviour.
    kInternalError          = 8407, ///< Unclassified internal error.
};

// ============================================================================
// § 2  Task descriptor constraints
// ============================================================================

/// Maximum task identifier length in bytes.
inline constexpr std::size_t kMaxTaskIdBytes = 256;

/// Maximum task name / description length in bytes.
inline constexpr std::size_t kMaxTaskNameBytes = 1024;

/// Maximum retained execution results per task.
inline constexpr std::size_t kDefaultRetentionLimit = 1000;

/// Default execution timeout for a registered task.
inline constexpr std::chrono::minutes kDefaultTaskExecutionTimeout{10};

/// Maximum scheduler concurrency (simultaneous in-flight tasks).
inline constexpr uint32_t kMaxConcurrentTasks = 256;

// ============================================================================
// § 3  Supporting structs
// ============================================================================

/**
 * @brief Descriptor for a task being registered with the scheduler.
 */
struct TaskRegistrationDescriptor {
    std::string         task_id;           ///< Unique task identifier.
    std::string         task_name;         ///< Human-readable task name.
    std::chrono::seconds execution_timeout{600}; ///< Per-run execution timeout.
    bool                allow_concurrent{false};  ///< Allow overlapping executions.
    uint32_t            retention_limit{static_cast<uint32_t>(kDefaultRetentionLimit)};
};

/**
 * @brief Lightweight execution result record stored by the scheduler.
 */
struct TaskExecutionResult {
    std::string  task_id;            ///< Task that produced this result.
    bool         success{false};     ///< Whether the execution succeeded.
    int32_t      exit_code{0};       ///< Numeric exit code (0 == success).
    std::string  message;            ///< Optional message or error detail.
    std::chrono::milliseconds duration_ms{0}; ///< Wall-clock execution duration.
};

// ============================================================================
// § 4  Fail-closed contract
// ============================================================================

/**
 * @brief Returns true when the given error mandates fail-closed denial of dispatch.
 */
[[nodiscard]] inline constexpr bool isSchedulerFailClosed(SchedulerError e) noexcept {
    return e == SchedulerError::kCoordinationError
        || e == SchedulerError::kInternalError;
}

} // namespace scheduler
} // namespace themis
