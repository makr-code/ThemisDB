/**
 * @file sharding_error_recovery_impl.h
 * @brief Phase 3: Implementation of Error Recovery Policy Mapping
 *
 * Canonical mapping from ShardingErrorCode to ErrorRecoveryStrategy.
 * This is the normative source of truth for error handling across all components.
 */

#pragma once

#include "sharding_api_contract.h"
#include "sharding_error_recovery.h"

namespace themis {
namespace sharding {

// ============================================================================
// § 1  Recovery Strategy Mapping Implementation
// ============================================================================

/**
 * @brief Get the recovery action for a given error code.
 *
 * Mapping rationale:
 *
 * QUORUM_LOST (1)
 *   - Strategy: FAIL_CLOSED
 *   - Rationale: Cannot proceed without quorum; no safe fallback.
 *   - Recovery: Operator must restore node availability.
 *   - Details: See QUORUM_LOSS_RUNBOOK.md
 *
 * COORDINATOR_FAILURE (2)
 *   - Strategy: DEGRADE_READONLY
 *   - Rationale: Coordinator is essential for writes but not reads.
 *   - Recovery: Automatic failover to replica or manual restart.
 *   - Details: Existing writes in-flight may remain in IN_DOUBT state.
 *
 * SHARD_UNAVAILABLE (3)
 *   - Strategy: RETRY_WITH_BACKOFF
 *   - Rationale: Transient network partition or temporary unavailability.
 *   - Recovery: Automatic retry; may succeed when shard recovers.
 *   - Details: Max 5 attempts with exponential backoff (100ms → 10s).
 *
 * MIGRATION_CONFLICT (4)
 *   - Strategy: FAIL_CLOSED
 *   - Rationale: Concurrent migrations to same shard must be serialized.
 *   - Recovery: Caller must wait for in-flight migration and retry.
 *   - Details: Lock-based serialization prevents this in normal operation.
 *
 * WAL_CORRUPTION (5)
 *   - Strategy: RECOVERY_REQUIRED
 *   - Rationale: Data integrity compromised; manual inspection needed.
 *   - Recovery: Operator must inspect/repair WAL before resuming.
 *   - Details: Automatic recovery is unsafe (data loss risk).
 *
 * CONSENSUS_TIMEOUT (6)
 *   - Strategy: TIMEOUT_AND_ABORT
 *   - Rationale: Operation timed out; abort to unblock client.
 *   - Recovery: Client may retry; consensus layer may eventually succeed.
 *   - Details: Bounded timeout prevents indefinite hangs.
 *
 * TRANSACTION_IN_DOUBT (7)
 *   - Strategy: TIMEOUT_AND_ABORT
 *   - Rationale: 2PC coordinator crashed; must await recovery.
 *   - Recovery: Automatic coordinator recovery resolves in-doubt txns.
 *   - Details: Participant blocks conflicting writes until resolved.
 *
 * ROUTING_RING_INVALID (8)
 *   - Strategy: FAIL_CLOSED
 *   - Rationale: Routing configuration is corrupted; cannot proceed safely.
 *   - Recovery: Operator must repair topology/shard configuration.
 *   - Details: Automatic fallback would mask consistency issues.
 *
 * MIGRATION_FAULT (9)
 *   - Strategy: ROLLBACK_AUTOMATIC
 *   - Rationale: Partial migration; must revert to safe state.
 *   - Recovery: Automatic rollback restores previous shard assignment.
 *   - Details: Rollback is idempotent (can be replayed safely).
 *
 * RING_EMPTY (10)
 *   - Strategy: FAIL_CLOSED
 *   - Rationale: No shards available; cannot route any request.
 *   - Recovery: Operator must add shards to cluster.
 *   - Details: Automatic degradation would route to nowhere.
 *
 * SHARD_INDEX_OUT_OF_RANGE (11)
 *   - Strategy: FAIL_CLOSED
 *   - Rationale: Configuration error (shard index > kMaxShards).
 *   - Recovery: Operator must correct shard index.
 *   - Details: Automatic fallback would hide configuration bugs.
 *
 * INTERNAL_ERROR (12)
 *   - Strategy: RECOVERY_REQUIRED
 *   - Rationale: Unclassified internal error; nature unknown.
 *   - Recovery: Operator must inspect logs and diagnose.
 *   - Details: Automatic handling unsafe without understanding error.
 */
inline RecoveryAction getRecoveryAction(ShardingErrorCode ec) noexcept {
    switch (ec) {
        case ShardingErrorCode::OK:
            return RecoveryAction(
                ErrorRecoveryStrategy::FAIL_CLOSED,
                0, 0,
                "Operation succeeded; no recovery needed"
            );

        case ShardingErrorCode::QUORUM_LOST:
            return RecoveryAction(
                ErrorRecoveryStrategy::FAIL_CLOSED,
                0, 0,
                "Quorum lost: < n/2+1 shards reachable. Operator must restore availability. "
                "See QUORUM_LOSS_RUNBOOK.md for recovery procedures."
            );

        case ShardingErrorCode::COORDINATOR_FAILURE:
            return RecoveryAction(
                ErrorRecoveryStrategy::DEGRADE_READONLY,
                0, kErrorRecoveryTimeout,
                "Coordinator crashed. Downgrading to read-only mode. "
                "Automatic failover or restart will restore write capability."
            );

        case ShardingErrorCode::SHARD_UNAVAILABLE:
            return RecoveryAction(
                ErrorRecoveryStrategy::RETRY_WITH_BACKOFF,
                kRetryMaxAttempts, 0,
                "Shard unavailable (network partition or temporary failure). "
                "Will retry with exponential backoff."
            );

        case ShardingErrorCode::MIGRATION_CONFLICT:
            return RecoveryAction(
                ErrorRecoveryStrategy::FAIL_CLOSED,
                0, 0,
                "Concurrent migrations target same shard. Caller must wait and retry. "
                "This indicates a control-plane synchronization issue."
            );

        case ShardingErrorCode::WAL_CORRUPTION:
            return RecoveryAction(
                ErrorRecoveryStrategy::RECOVERY_REQUIRED,
                0, 0,
                "WAL entry CRC check failed. Data integrity compromised. "
                "Operator must inspect WAL and decide recovery path. "
                "See WAL repair procedures in operational documentation."
            );

        case ShardingErrorCode::CONSENSUS_TIMEOUT:
            return RecoveryAction(
                ErrorRecoveryStrategy::TIMEOUT_AND_ABORT,
                0, kErrorRecoveryTimeout,
                "Consensus operation timed out. Aborting to unblock client. "
                "Transaction may be committed or aborted; client must verify."
            );

        case ShardingErrorCode::TRANSACTION_IN_DOUBT:
            return RecoveryAction(
                ErrorRecoveryStrategy::TIMEOUT_AND_ABORT,
                0, kErrorRecoveryTimeout,
                "2PC transaction in IN_DOUBT state (coordinator crashed). "
                "Blocking conflicting writes. Automatic coordinator recovery will resolve."
            );

        case ShardingErrorCode::ROUTING_RING_INVALID:
            return RecoveryAction(
                ErrorRecoveryStrategy::FAIL_CLOSED,
                0, 0,
                "Routing ring is invalid or empty. Cannot route request safely. "
                "Operator must repair topology configuration."
            );

        case ShardingErrorCode::MIGRATION_FAULT:
            return RecoveryAction(
                ErrorRecoveryStrategy::ROLLBACK_AUTOMATIC,
                0, kRollbackTimeout,
                "Shard migration failed. Initiating automatic rollback to previous assignment. "
                "Rollback is idempotent and can be safely retried."
            );

        case ShardingErrorCode::RING_EMPTY:
            return RecoveryAction(
                ErrorRecoveryStrategy::FAIL_CLOSED,
                0, 0,
                "Consistent hash ring is empty (no shards). Cannot route any request. "
                "Operator must add shards to cluster."
            );

        case ShardingErrorCode::SHARD_INDEX_OUT_OF_RANGE:
            return RecoveryAction(
                ErrorRecoveryStrategy::FAIL_CLOSED,
                0, 0,
                "Shard index exceeds kMaxShards. Configuration error. "
                "Operator must correct shard index (max: " +
                std::to_string(kMaxShards) + ")."
            );

        case ShardingErrorCode::INTERNAL_ERROR:
            return RecoveryAction(
                ErrorRecoveryStrategy::RECOVERY_REQUIRED,
                0, 0,
                "Internal error (unclassified). Operator must inspect logs. "
                "Check spdlog output for stack trace and error context."
            );

        default:
            return RecoveryAction(
                ErrorRecoveryStrategy::RECOVERY_REQUIRED,
                0, 0,
                "Unknown error code (invalid). Recovery required."
            );
    }
}

// ============================================================================
// § 2  Error Code Name Mapping
// ============================================================================

inline std::string errorCodeName(ShardingErrorCode ec) noexcept {
    switch (ec) {
        case ShardingErrorCode::OK:
            return "OK";
        case ShardingErrorCode::QUORUM_LOST:
            return "QUORUM_LOST";
        case ShardingErrorCode::COORDINATOR_FAILURE:
            return "COORDINATOR_FAILURE";
        case ShardingErrorCode::SHARD_UNAVAILABLE:
            return "SHARD_UNAVAILABLE";
        case ShardingErrorCode::MIGRATION_CONFLICT:
            return "MIGRATION_CONFLICT";
        case ShardingErrorCode::WAL_CORRUPTION:
            return "WAL_CORRUPTION";
        case ShardingErrorCode::CONSENSUS_TIMEOUT:
            return "CONSENSUS_TIMEOUT";
        case ShardingErrorCode::TRANSACTION_IN_DOUBT:
            return "TRANSACTION_IN_DOUBT";
        case ShardingErrorCode::ROUTING_RING_INVALID:
            return "ROUTING_RING_INVALID";
        case ShardingErrorCode::MIGRATION_FAULT:
            return "MIGRATION_FAULT";
        case ShardingErrorCode::RING_EMPTY:
            return "RING_EMPTY";
        case ShardingErrorCode::SHARD_INDEX_OUT_OF_RANGE:
            return "SHARD_INDEX_OUT_OF_RANGE";
        case ShardingErrorCode::INTERNAL_ERROR:
            return "INTERNAL_ERROR";
        default:
            return "UNKNOWN_ERROR_CODE";
    }
}

// ============================================================================
// § 3  Fail-Closed Check
// ============================================================================

inline bool isFailClosedError(ShardingErrorCode ec) noexcept {
    return getRecoveryAction(ec).strategy == ErrorRecoveryStrategy::FAIL_CLOSED;
}

} // namespace sharding
} // namespace themis
