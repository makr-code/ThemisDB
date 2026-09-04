/**
 * @file updates_edge_case_handler.cpp
 * @brief Implementation of UpdatesEdgeCaseHandler — deterministic edge-case
 *        detection for the Updates module (Phase 4, Q4 2026).
 * @version 1.0.0
 * @since 2.4.0 (Q4 2026)
 *
 * Doxygen maturity: 🟢 PRODUCTION-READY
 */

// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

#include "updates/updates_edge_case_handler.h"

#include <algorithm>
#include <mutex>
#include <string>

namespace themis {
namespace updates {

// ============================================================================
// detectAndHandle
// ============================================================================

EdgeCaseResult UpdatesEdgeCaseHandler::detectAndHandle(
        const UpdateStateMachine& sm,
        const std::string& context_hint) {

    const UpdateState state = sm.currentState();
    const bool pending_rollback = sm.hasPendingRollback();

    // -----------------------------------------------------------------------
    // Priority 1: STATE_FAILED_LOCKED (7402)
    //   The state machine is stuck in FAILED.  This is the highest-priority
    //   check because nothing else can proceed until the operator resets.
    // -----------------------------------------------------------------------
    if (state == UpdateState::FAILED) {
        return makeResult(
            DiagnosticErrorCode::STATE_FAILED_LOCKED,
            "State machine is locked in FAILED state; operator reset required "
            "(context: " + context_hint + ")",
            /*requires_rollback=*/false);
    }

    // -----------------------------------------------------------------------
    // Priority 2: STATE_ALREADY_IN_PROGRESS (7401)
    //   A concurrent update attempt was detected while the machine is already
    //   active.  Context hint "concurrent" or "collision" triggers this.
    // -----------------------------------------------------------------------
    if ((state == UpdateState::DOWNLOADING ||
         state == UpdateState::VERIFYING   ||
         state == UpdateState::APPLYING) &&
        (context_hint.find("concurrent") != std::string::npos ||
         context_hint.find("collision")  != std::string::npos)) {
        return makeResult(
            DiagnosticErrorCode::STATE_ALREADY_IN_PROGRESS,
            "Concurrent update collision detected while machine is in state " +
            UpdateStateMachine::stateName(state) +
            " (context: " + context_hint + ")",
            /*requires_rollback=*/true);
    }

    // -----------------------------------------------------------------------
    // Priority 3: STATE_TRANSITION_TIMEOUT (7403)
    //   The caller reports that a timed-out transition occurred.
    //   Context hint "timeout" triggers this.
    // -----------------------------------------------------------------------
    if (context_hint.find("timeout") != std::string::npos) {
        return makeResult(
            DiagnosticErrorCode::STATE_TRANSITION_TIMEOUT,
            "State transition timeout while in state " +
            UpdateStateMachine::stateName(state) +
            " (context: " + context_hint + ")",
            /*requires_rollback=*/true);
    }

    // -----------------------------------------------------------------------
    // Priority 4: ROLLBACK_CASCADE_DETECTED (7423)
    //   The state machine is rolling back AND context indicates cascade.
    // -----------------------------------------------------------------------
    if (state == UpdateState::ROLLING_BACK &&
        context_hint.find("cascade") != std::string::npos) {
        return makeResult(
            DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED,
            "Rollback cascade detected and must be contained; "
            "node isolation recommended (context: " + context_hint + ")",
            /*requires_rollback=*/false);
    }

    // -----------------------------------------------------------------------
    // Priority 5: ROLLBACK_ISOLATION_ACTIVE (7426)
    //   A prior rollback has left an isolation fence in place.
    //   Context hint "isolation" triggers this.
    // -----------------------------------------------------------------------
    if (context_hint.find("isolation") != std::string::npos) {
        return makeResult(
            DiagnosticErrorCode::ROLLBACK_ISOLATION_ACTIVE,
            "Rollback isolation is active; further updates are blocked until "
            "isolation is cleared (context: " + context_hint + ")",
            /*requires_rollback=*/false);
    }

    // -----------------------------------------------------------------------
    // Priority 6: ROLLBACK_PARTIAL_SUCCESS (7424)
    //   A rolling-back or post-rollback state with context "partial" or
    //   "partial-success" indicates that only some nodes succeeded.
    // -----------------------------------------------------------------------
    if ((state == UpdateState::ROLLING_BACK || state == UpdateState::IDLE) &&
        (context_hint.find("partial") != std::string::npos)) {
        return makeResult(
            DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS,
            "Partial rollback success; some cluster nodes failed — retry "
            "recommended (context: " + context_hint + ")",
            /*requires_rollback=*/true);
    }

    // -----------------------------------------------------------------------
    // Priority 7: PATCH_APPLY_FAILED (7440)
    //   Detected while in APPLYING state with context "patch-conflict" or
    //   "patch".
    // -----------------------------------------------------------------------
    if (state == UpdateState::APPLYING &&
        (context_hint.find("patch") != std::string::npos)) {
        return makeResult(
            DiagnosticErrorCode::PATCH_APPLY_FAILED,
            "Patch conflict detected during APPLYING phase "
            "(context: " + context_hint + ")",
            /*requires_rollback=*/true);
    }

    // -----------------------------------------------------------------------
    // Priority 8: NETWORK_PARTITION (7460)
    //   Manifest version conflict maps to NETWORK_PARTITION as the upstream
    //   coordination channel is disrupted.
    //   Context hint "manifest" or "manifest-conflict" triggers this.
    // -----------------------------------------------------------------------
    if (context_hint.find("manifest") != std::string::npos) {
        return makeResult(
            DiagnosticErrorCode::NETWORK_PARTITION,
            "Manifest version conflict — likely network partition or split "
            "coordinator view (context: " + context_hint + ")",
            /*requires_rollback=*/false);
    }

    // -----------------------------------------------------------------------
    // Priority 9: COORDINATION_ORDERING_VIOLATION (7464)
    //   Quota exhaustion / ordering violation detected.
    //   Context hint "quota" or "ordering" triggers this.
    // -----------------------------------------------------------------------
    if (context_hint.find("quota")    != std::string::npos ||
        context_hint.find("ordering") != std::string::npos) {
        return makeResult(
            DiagnosticErrorCode::COORDINATION_ORDERING_VIOLATION,
            "Coordination ordering violation or quota exhaustion "
            "(context: " + context_hint + ")",
            /*requires_rollback=*/false);
    }

    // -----------------------------------------------------------------------
    // No edge case detected — return default (not-handled) result.
    // -----------------------------------------------------------------------
    if (pending_rollback) {
        // Pending rollback without explicit context — treat as partial success.
        return makeResult(
            DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS,
            "Pending rollback detected at state " +
            UpdateStateMachine::stateName(state),
            /*requires_rollback=*/true);
    }

    return EdgeCaseResult{};  // handled == false
}

// ============================================================================
// classifyEdgeCase
// ============================================================================

std::string_view UpdatesEdgeCaseHandler::classifyEdgeCase(DiagnosticErrorCode code) {
    switch (code) {
        case DiagnosticErrorCode::STATE_INVALID_TRANSITION:       return "InvalidTransition";
        case DiagnosticErrorCode::STATE_ALREADY_IN_PROGRESS:     return "ConcurrentCollision";
        case DiagnosticErrorCode::STATE_FAILED_LOCKED:           return "StateLocked";
        case DiagnosticErrorCode::STATE_TRANSITION_TIMEOUT:      return "TransitionTimeout";
        case DiagnosticErrorCode::STATE_HISTORY_CORRUPT:         return "HistoryCorrupt";
        case DiagnosticErrorCode::ROLLBACK_CHECKPOINT_NOT_FOUND: return "CheckpointNotFound";
        case DiagnosticErrorCode::ROLLBACK_NO_CHECKPOINTS:       return "NoCheckpoints";
        case DiagnosticErrorCode::ROLLBACK_FAILED:               return "RollbackFailed";
        case DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED:     return "CascadeRollback";
        case DiagnosticErrorCode::ROLLBACK_PARTIAL_SUCCESS:      return "PartialSuccess";
        case DiagnosticErrorCode::ROLLBACK_DEFERRED:             return "RollbackDeferred";
        case DiagnosticErrorCode::ROLLBACK_ISOLATION_ACTIVE:     return "RollbackIsolation";
        case DiagnosticErrorCode::PATCH_APPLY_FAILED:            return "PatchConflict";
        case DiagnosticErrorCode::PATCH_CHECKSUM_MISMATCH:       return "ChecksumMismatch";
        case DiagnosticErrorCode::PATCH_INCOMPATIBLE_BASE:       return "IncompatibleBase";
        case DiagnosticErrorCode::PATCH_INCOMPLETE:              return "PatchIncomplete";
        case DiagnosticErrorCode::PATCH_DECODE_ERROR:            return "PatchDecodeError";
        case DiagnosticErrorCode::NETWORK_PARTITION:             return "ManifestVersionConflict";
        case DiagnosticErrorCode::COORDINATION_TIMEOUT:          return "CoordinationTimeout";
        case DiagnosticErrorCode::COORDINATION_PEER_FAILED:      return "PeerFailed";
        case DiagnosticErrorCode::COORDINATION_QUORUM_LOST:      return "QuorumLost";
        case DiagnosticErrorCode::COORDINATION_ORDERING_VIOLATION: return "QuotaExhaustion";
        case DiagnosticErrorCode::CASCADE_DETECTED:              return "CascadeDetected";
        case DiagnosticErrorCode::DATA_LOSS_RISK:                return "DataLossRisk";
        case DiagnosticErrorCode::UNSUPPORTED_OPERATION:         return "UnsupportedOperation";
        case DiagnosticErrorCode::RESOURCE_EXHAUSTED:            return "ResourceExhausted";
        default:                                                  return "Unknown";
    }
}

// ============================================================================
// isFatal
// ============================================================================

bool UpdatesEdgeCaseHandler::isFatal(DiagnosticErrorCode code) {
    switch (code) {
        case DiagnosticErrorCode::STATE_FAILED_LOCKED:
        [[fallthrough]];
        case DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED:
        [[fallthrough]];
        case DiagnosticErrorCode::NETWORK_PARTITION:
        [[fallthrough]];
        case DiagnosticErrorCode::DATA_LOSS_RISK:
        [[fallthrough]];
        case DiagnosticErrorCode::CASCADE_DETECTED:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// requiresIsolation
// ============================================================================

bool UpdatesEdgeCaseHandler::requiresIsolation(DiagnosticErrorCode code) {
    switch (code) {
        case DiagnosticErrorCode::STATE_ALREADY_IN_PROGRESS:
        [[fallthrough]];
        case DiagnosticErrorCode::STATE_FAILED_LOCKED:
        [[fallthrough]];
        case DiagnosticErrorCode::ROLLBACK_CASCADE_DETECTED:
        [[fallthrough]];
        case DiagnosticErrorCode::ROLLBACK_ISOLATION_ACTIVE:
            return true;
        default:
            return false;
    }
}

// ============================================================================
// getStats
// ============================================================================

const EdgeCaseStats& UpdatesEdgeCaseHandler::getStats() const {
    return stats_;
}

// ============================================================================
// makeResult (private)
// ============================================================================

EdgeCaseResult UpdatesEdgeCaseHandler::makeResult(DiagnosticErrorCode code,
                                                   std::string description,
                                                   bool requires_rollback) {
    const bool fatal = isFatal(code);

    {
        std::lock_guard<std::mutex> lock(stats_mutex_);
        ++stats_.total_detected;
        if (fatal) {
          ++stats_.total_fatal;
        }
        if (requires_rollback) {
          ++stats_.total_rollback_triggered;
        }
    }

    EdgeCaseResult result;
    result.handled          = true;
    result.error_code       = code;
    result.description      = std::move(description);
    result.requires_rollback = requires_rollback;
    return result;
}

} // namespace updates
} // namespace themis
