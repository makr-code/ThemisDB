/*
 * ThemisDB | File: replication_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen replication runtime contract semantics for the active v1.x major line.
 */

/**
 * @file replication_api_contract.h
 * @brief Frozen replication runtime contract semantics for the active v1.x line.
 *
 * This header defines the normative contract for all replication module
 * components, covering logical replication, CRDT operations, conflict
 * resolution, change-stream delivery, and Kafka integration.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all components in the ThemisDB
 * replication pipeline:
 *   - Logical replication (WAL event ordering, idempotent apply)
 *   - CRDT types (GCounter, PNCounter, LWWRegister, ORSet)
 *   - Conflict resolution (LWW, field-level merge, tombstone)
 *   - Change streams (partition-ordered, at-least-once delivery)
 *   - Kafka integration (acks=all, offset commit after successful apply)
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/replication/ROADMAP.md  — Phase 1 contract item
 * @see include/replication/ROADMAP.md — Phase 6 documentation item
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <string_view>

namespace themis {
namespace replication {

// ============================================================================
// § 1  Logical replication / WAL ordering contract
//
// WAL event ordering guarantee:
//   Events are assigned monotonically increasing Log Sequence Numbers (LSNs).
//   An apply operation MUST honour LSN order; applying an event with a lower
//   LSN after a higher LSN has already been applied returns APPLY_FAILED.
//
// Idempotent apply:
//   Replaying an already-applied WAL event (same LSN, same content) is a
//   no-op; no error is raised and the data state is unchanged.
//
// Gap detection:
//   A gap in the received LSN sequence (LSN[n+1] > LSN[n] + 1) is flagged as
//   REPLICATION_LAG_EXCEEDED; the apply pipeline MUST pause and signal the
//   operator rather than applying events across the gap.
// ============================================================================

/// Maximum acceptable replication lag in bytes before REPLICATION_LAG_EXCEEDED.
inline constexpr std::int64_t kMaxReplicationLagBytes = 512LL * 1024 * 1024; // 512 MiB

/// Maximum number of in-flight unapplied WAL events before backpressure.
inline constexpr std::size_t kMaxPendingWalEvents = 100'000u;

// ============================================================================
// § 2  CRDT contract
//
// Merge commutativity:
//   For any two CRDT states A and B, merge(A, B) == merge(B, A).
//
// Merge associativity:
//   merge(merge(A, B), C) == merge(A, merge(B, C)).
//
// Idempotency:
//   merge(A, A) == A.  Repeated application of the same delta is safe.
//
// Type safety:
//   Merging two CRDT instances of incompatible types raises CRDT_TYPE_MISMATCH.
//   No silent type coercion or data corruption occurs.
// ============================================================================

// ============================================================================
// § 3  Conflict resolution contract
//
// LWW (Last-Write-Wins) determinism:
//   Given two conflicting versions V1 (timestamp T1) and V2 (timestamp T2),
//   the winner is always the version with the greater timestamp value.  The
//   result is deterministic: same inputs always produce the same winner.
//
// Tie-break rule:
//   When T1 == T2, the version from the node with the lexicographically greater
//   node-ID wins.  This rule is applied consistently across all nodes.
//
// Tombstone wins:
//   A delete tombstone always wins over a concurrent update when both carry the
//   same timestamp.  This prevents resurrection of deleted entries.
// ============================================================================

// ============================================================================
// § 4  Change stream delivery contract
//
// At-least-once delivery:
//   Every committed database event MUST be delivered to all registered stream
//   consumers at least once.  Loss of a committed event is a contract violation.
//
// Partition ordering:
//   Events within a single partition are delivered in commit order.  Cross-
//   partition ordering is not guaranteed.
//
// Redelivery idempotency:
//   Consumer implementations MUST be idempotent; a redelivered event must
//   produce the same final state as the original delivery.
// ============================================================================

// ============================================================================
// § 5  Kafka integration contract
//
// Producer acks=all:
//   All Kafka producers writing replication events MUST use acks=all.  Using
//   acks=1 or acks=0 is a contract violation.
//
// Offset commit timing:
//   Consumer offset commits are performed AFTER successful apply of the event.
//   Pre-apply offset commits are prohibited; they risk silent data loss on
//   consumer restart.
// ============================================================================

/// Default Kafka producer acknowledgement level (all replicas).
inline constexpr std::string_view kKafkaRequiredAcks = "all";

// ============================================================================
// § 6  Error taxonomy
// ============================================================================

/**
 * @brief Canonical replication error codes.
 *
 * Codes in range [200, 299] are replication-specific.
 */
enum class ReplicationErrorCode : int {
    /// Replica lag has exceeded the configured maximum threshold.
    REPLICATION_LAG_EXCEEDED      = 200,

    /// A conflict between two versions cannot be resolved deterministically.
    CONFLICT_UNRESOLVABLE         = 201,

    /// CRDT merge attempted on two instances of incompatible types.
    CRDT_TYPE_MISMATCH            = 202,

    /// Change stream partition offset is invalid or behind the current commit.
    STREAM_OFFSET_ERROR           = 203,

    /// WAL event apply failed (wrong LSN order or malformed event).
    APPLY_FAILED                  = 204,

    /// Kafka producer or consumer encountered a broker-level error.
    KAFKA_BROKER_ERROR            = 205,

    /// Replication slot has been invalidated and cannot be resumed.
    SLOT_INVALIDATED              = 206,

    /// Unclassified replication internal error; always fail-safe.
    INTERNAL_ERROR                = 299,
};

/**
 * @brief Returns true when the error code indicates a non-retryable hard error.
 *
 * Hard replication errors must be reported to the operator immediately; the
 * pipeline must not silently continue in a degraded state.
 */
[[nodiscard]] inline constexpr bool isHardReplicationError(ReplicationErrorCode code) noexcept {
    return code == ReplicationErrorCode::CONFLICT_UNRESOLVABLE
        || code == ReplicationErrorCode::CRDT_TYPE_MISMATCH
        || code == ReplicationErrorCode::APPLY_FAILED
        || code == ReplicationErrorCode::SLOT_INVALIDATED
        || code == ReplicationErrorCode::INTERNAL_ERROR;
}

/**
 * @brief Returns true when the error code indicates a recoverable lag / flow
 *        control condition.
 */
[[nodiscard]] inline constexpr bool isLagError(ReplicationErrorCode code) noexcept {
    return code == ReplicationErrorCode::REPLICATION_LAG_EXCEEDED
        || code == ReplicationErrorCode::STREAM_OFFSET_ERROR;
}

// ============================================================================
// § 7  Node identity contract
//
// Every participating replication node MUST have a non-empty, cluster-unique
// node identifier.  The node-ID is used as the LWW tie-break key and must
// remain stable across node restarts.
// ============================================================================

/// Maximum byte length of a replication node identifier.
inline constexpr std::size_t kMaxNodeIdBytes = 256;

/// Maximum number of replica nodes supported in a single replication group.
inline constexpr std::size_t kMaxReplicaCount = 64;

} // namespace replication
} // namespace themis
