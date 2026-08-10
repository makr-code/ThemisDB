/**
 * @file updates_edge_case_handler.h
 * @brief Deterministic edge-case detection and handling for the Updates module (Phase 4)
 * @version 1.0.0
 * @since 2.4.0 (Q4 2026)
 *
 * Provides `UpdatesEdgeCaseHandler`, a stateful, thread-safe class that inspects
 * an `UpdateStateMachine` snapshot and context hints to classify edge-case
 * conditions that fall outside the normal IDLE → DOWNLOADING → VERIFYING →
 * APPLYING → IDLE happy path.
 *
 * Error code range covered: [7400–7499] (Updates module reserved range).
 *
 * Typical usage:
 * @code
 *   UpdatesEdgeCaseHandler handler;
 *   auto result = handler.detectAndHandle(state_machine, "post-apply");
 *   if (result.handled) {
 *       if (handler.isFatal(result.error_code)) {
 *           // escalate
 *       } else if (result.requires_rollback) {
 *           // trigger rollback path
 *       }
 *   }
 * @endcode
 *
 * Doxygen maturity: 🟢 PRODUCTION-READY
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#pragma once

#include "updates/updates_diagnostics.h"
#include "updates/update_state_machine.h"

#include <cstdint>
#include <mutex>
#include <string>
#include <string_view>

namespace themis {
namespace updates {

/**
 * @brief Result of a single edge-case detection invocation.
 *
 * When `handled` is false all other fields are undefined/default and the
 * caller should treat the state machine as operating normally.
 */
struct EdgeCaseResult {
    /// True when an edge case was detected and classified.
    bool handled{false};

    /// The canonical error code identifying the edge case category.
    DiagnosticErrorCode error_code{DiagnosticErrorCode::STATE_INVALID_TRANSITION};

    /// Human-readable description of the detected condition.
    std::string description;

    /// True when a rollback is the recommended recovery action.
    bool requires_rollback{false};
};

/**
 * @brief Cumulative statistics for edge-case detections across all calls.
 *
 * Updated atomically under the handler's internal mutex.
 */
struct EdgeCaseStats {
    /// Total number of edge cases detected (handled == true results).
    uint64_t total_detected{0};

    /// Subset of total_detected that are classified as fatal.
    uint64_t total_fatal{0};

    /// Subset of total_detected for which rollback was recommended.
    uint64_t total_rollback_triggered{0};
};

/**
 * @class UpdatesEdgeCaseHandler
 * @brief Detects and classifies deterministic edge-case conditions in the
 *        updates state machine.
 *
 * All public methods are thread-safe. The handler maintains no persistent
 * mutable state beyond the `EdgeCaseStats` counters.
 *
 * Detection priority order (highest to lowest):
 *  1. STATE_FAILED_LOCKED (7402)
 *  2. STATE_ALREADY_IN_PROGRESS (7401)
 *  3. STATE_TRANSITION_TIMEOUT (7403)
 *  4. ROLLBACK_CASCADE_DETECTED (7423)
 *  5. ROLLBACK_ISOLATION_ACTIVE (7426)
 *  6. ROLLBACK_PARTIAL_SUCCESS (7424)
 *  7. PATCH_APPLY_FAILED (7440)
 *  8. NETWORK_PARTITION (7460)
 *  9. COORDINATION_ORDERING_VIOLATION (7464)
 */
class UpdatesEdgeCaseHandler {
public:
    /**
     * @brief Default constructor. Statistics are zero-initialised.
     */
    UpdatesEdgeCaseHandler() = default;

    ~UpdatesEdgeCaseHandler() = default;

    // Non-copyable, non-movable (owns a mutex)
    UpdatesEdgeCaseHandler(const UpdatesEdgeCaseHandler&) = delete;
    UpdatesEdgeCaseHandler& operator=(const UpdatesEdgeCaseHandler&) = delete;

    // -------------------------------------------------------------------------
    // Primary detection interface
    // -------------------------------------------------------------------------

    /**
     * @brief Inspect the current state machine snapshot and context hint to
     *        detect and classify an edge case.
     *
     * Detection is purely observational — this method never mutates the state
     * machine.  Statistics are updated if an edge case is detected.
     *
     * @param sm           The state machine to inspect.  The method reads
     *                     `sm.currentState()` and, where applicable,
     *                     `sm.hasPendingRollback()`.
     * @param context_hint Optional free-form caller context used to
     *                     disambiguate conditions that share the same state
     *                     (e.g. "post-apply", "timeout", "cascade",
     *                     "isolation", "patch-conflict", "manifest-conflict",
     *                     "quota", "partial-success").
     * @return             Populated `EdgeCaseResult`; `handled` is false when
     *                     no edge case is detected.
     */
    EdgeCaseResult detectAndHandle(const UpdateStateMachine& sm,
                                   const std::string& context_hint);

    // -------------------------------------------------------------------------
    // Classification helpers (stateless, no mutex needed)
    // -------------------------------------------------------------------------

    /**
     * @brief Return the canonical human-readable class name for a given code.
     *
     * @param code A `DiagnosticErrorCode` in the [7400-7499] range.
     * @return     A stable, non-empty `string_view` (e.g. "StateLocked",
     *             "ConcurrentCollision").  Returns "Unknown" for unrecognised
     *             codes.
     */
    static std::string_view classifyEdgeCase(DiagnosticErrorCode code);

    /**
     * @brief Return true when the supplied code represents a non-recoverable
     *        (fatal) condition that requires operator intervention.
     *
     * Fatal codes (as of Phase 4):
     *  - STATE_FAILED_LOCKED (7402)
     *  - ROLLBACK_CASCADE_DETECTED (7423)
     *  - NETWORK_PARTITION (7460)
     *
     * @param code The error code to test.
     * @return     true if the code is classified as fatal.
     */
    static bool isFatal(DiagnosticErrorCode code);

    /**
     * @brief Return true when node isolation is the appropriate first response
     *        to the supplied code.
     *
     * Isolation-appropriate codes:
     *  - STATE_ALREADY_IN_PROGRESS (7401)
     *  - STATE_FAILED_LOCKED (7402)
     *  - ROLLBACK_CASCADE_DETECTED (7423)
     *  - ROLLBACK_ISOLATION_ACTIVE (7426)
     *
     * @param code The error code to test.
     * @return     true when isolating the affected node is recommended.
     */
    static bool requiresIsolation(DiagnosticErrorCode code);

    // -------------------------------------------------------------------------
    // Statistics
    // -------------------------------------------------------------------------

    /**
     * @brief Return a snapshot of cumulative detection statistics.
     *
     * The returned reference is valid for the lifetime of this handler.
     * Callers that need a consistent snapshot under concurrent use should
     * invoke this from a single thread or copy the struct.
     *
     * @return Const reference to the internal `EdgeCaseStats` struct.
     */
    const EdgeCaseStats& getStats() const;

private:
    // -------------------------------------------------------------------------
    // Internal helpers
    // -------------------------------------------------------------------------

    /// Build a result, update stats, and return to caller.
    EdgeCaseResult makeResult(DiagnosticErrorCode code,
                              std::string description,
                              bool requires_rollback);

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    mutable std::mutex stats_mutex_;
    EdgeCaseStats stats_;
};

} // namespace updates
} // namespace themis
