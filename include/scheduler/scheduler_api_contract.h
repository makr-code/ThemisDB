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

enum class SchedulerError : int32_t {
	kSuccess                = 0,
	kTaskNotFound           = 8400,
	kTaskAlreadyExists      = 8401,
	kExecutionFailed        = 8402,
	kCoordinationError      = 8403,
	kRetentionLimitExceeded = 8404,
	kTriggerInvalid         = 8405,
	kAnomalyDetected        = 8406,
	kInternalError          = 8407,
};

// Configuration constants kept minimal in this header.
inline constexpr std::size_t kMaxTaskIdBytes = 256;
inline constexpr std::size_t kDefaultRetentionLimit = 1000;
inline constexpr std::chrono::minutes kDefaultTaskExecutionTimeout{10};

[[nodiscard]] inline constexpr bool isSchedulerFailClosed(SchedulerError e) noexcept {
	return e == SchedulerError::kCoordinationError || e == SchedulerError::kInternalError;
}

} // namespace scheduler

using scheduler::SchedulerError;

} // namespace themis
