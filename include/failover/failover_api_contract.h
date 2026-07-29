/*
 * ThemisDB | File: failover_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen failover and leader-election contract semantics for the active v1.x major line.
 */

/**
 * @file failover_api_contract.h
 * @brief Frozen failover, election, and recovery contract for the active v1.x line.
 *
 * This header defines the normative contract for the failover module including
 * leader election, request handover, node recovery, and error classification that
 * all failover module components must honour in the current major release line.
 *
 * ## Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB failover pipeline:
 *   - AutoFailoverManager (auto_failover_manager.h)
 *   - DisasterRecoveryManager (disaster_recovery_manager.h)
 *   - Heartbeat monitors and epoch keepers
 *   - In-flight request buffers and handover coordinators
 *   - Node rejoin / follower-only promotion gates
 *
 * ## Election Contract
 *
 * Exactly one leader is elected per epoch.  Epoch numbers are strictly
 * monotonically increasing.  A node that observes a higher epoch from any
 * peer immediately steps down as leader candidate for all lower epochs.
 *
 * ## Handover Contract
 *
 * When a leader handover occurs every in-flight request MUST be either:
 *   a) completed by the outgoing leader before it yields, or
 *   b) explicitly retried by the new leader after epoch promotion.
 * Silent drops (no completion, no retry) are a contract violation.
 *
 * ## Recovery Contract
 *
 * A failed node that reconnects MUST rejoin as follower only.
 * Direct promotion to leader on rejoin is prohibited; the node must
 * complete full state synchronisation before it may participate in
 * subsequent elections.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/failover/ROADMAP.md — Phase 1 item
 * @see include/failover/auto_failover_manager.h
 * @see include/failover/disaster_recovery_manager.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace failover {

// ============================================================================
// § 1  Timing constants
//
// All failover timing values are enforced at every participating node.
// ============================================================================

/// Default heartbeat interval — how often a leader broadcasts liveness.
inline constexpr std::chrono::milliseconds kHeartbeatInterval{500};

/// Default heartbeat timeout — leader-loss is declared after this period
/// elapses with no heartbeat received from the current leader.
/// Contract: ≤ 3 000 ms (3 s).
inline constexpr std::chrono::milliseconds kHeartbeatTimeout{3'000};

/// Minimum allowed heartbeat timeout (operator-configurable lower bound).
inline constexpr std::chrono::milliseconds kMinHeartbeatTimeout{200};

/// Maximum allowed heartbeat timeout (operator-configurable upper bound).
inline constexpr std::chrono::milliseconds kMaxHeartbeatTimeout{30'000};

/// Maximum wall-clock budget for a leader election round.
inline constexpr std::chrono::milliseconds kElectionTimeoutBudget{5'000};

/// State-sync completion deadline for a rejoining follower.
inline constexpr std::chrono::seconds kStateSyncDeadline{30};

// ============================================================================
// § 2  Epoch contract
//
// Epochs are 64-bit unsigned integers.  The initial epoch is 1.
// Every successful election increments the epoch by exactly 1.
// An epoch value of 0 is reserved and invalid.
// ============================================================================

/// Reserved invalid epoch sentinel.
inline constexpr std::uint64_t kInvalidEpoch = 0u;

/// First valid epoch number.
inline constexpr std::uint64_t kFirstValidEpoch = 1u;

/// Returns true when the given epoch value is valid (non-zero).
[[nodiscard]] inline constexpr bool isValidEpoch(std::uint64_t epoch) noexcept {
    return epoch != kInvalidEpoch;
}

/// Returns true when @p newer is strictly higher than @p current,
/// i.e. the caller should step down and accept @p newer as authoritative.
[[nodiscard]] inline constexpr bool isNewerEpoch(
        std::uint64_t newer, std::uint64_t current) noexcept {
    return newer > current;
}

// ============================================================================
// § 3  Node role contract
// ============================================================================

/**
 * @brief Role that a node occupies at any given instant.
 *
 * A node may hold exactly one role at a time.  Transitions are:
 *   Unknown / Follower → Candidate → Leader (on election win)
 *   Leader → Follower (on stepping down or detecting a higher epoch)
 *   Failed → Follower (on successful rejoin + state sync — never directly to Leader)
 */
enum class NodeRole : int {
    Unknown   = 0, ///< Role not yet determined (start-up only).
    Follower  = 1, ///< Passive participant; accepts log entries from leader.
    Candidate = 2, ///< Actively seeking votes to become leader.
    Leader    = 3, ///< Authoritative node for current epoch.
    Failed    = 4, ///< Node has been declared failed; not yet rejoined.
};

/// Returns true when the role allows a node to accept write operations.
[[nodiscard]] inline constexpr bool isWriteEligible(NodeRole role) noexcept {
    return role == NodeRole::Leader;
}

/// Returns true when the role allows a node to cast votes in elections.
[[nodiscard]] inline constexpr bool isVoteEligible(NodeRole role) noexcept {
    return role == NodeRole::Follower || role == NodeRole::Candidate;
}

// ============================================================================
// § 4  Error taxonomy
//
// All failover components must map their internal error states to one of these
// canonical codes so that callers can apply uniform policy.
// ============================================================================

/**
 * @brief Canonical error codes for the failover module.
 *
 * Codes are assigned contiguous integers for easy serialisation.  New codes
 * must be appended; existing values must never be renumbered.
 */
enum class FailoverErrorCode : int {
    /// No error.
    OK = 0,

    /// Election did not complete within kElectionTimeoutBudget.
    ELECTION_TIMEOUT = 1,

    /// Two or more nodes simultaneously believe they are leader for the same epoch.
    SPLIT_BRAIN_DETECTED = 2,

    /// Leader handover did not complete; in-flight requests may be at risk.
    HANDOVER_INCOMPLETE = 3,

    /// A failed node attempted to rejoin directly as leader (contract violation).
    NODE_REJOIN_FAILED = 4,

    /// A heartbeat was missed; counter is incremented toward leader-loss threshold.
    HEARTBEAT_MISSED = 5,

    /// State synchronisation from leader to rejoining follower timed out.
    STATE_SYNC_TIMEOUT = 6,

    /// A quorum could not be established (insufficient live nodes).
    QUORUM_UNAVAILABLE = 7,

    /// The epoch value supplied was invalid (zero) or stale.
    INVALID_EPOCH = 8,

    /// Internal failover component error; always treated as fail-safe.
    INTERNAL_ERROR = 9,
};

/// Returns true for error codes that mandate a fail-safe (no-leader) outcome.
[[nodiscard]] inline constexpr bool isFailSafeCode(FailoverErrorCode code) noexcept {
    return code == FailoverErrorCode::SPLIT_BRAIN_DETECTED
        || code == FailoverErrorCode::QUORUM_UNAVAILABLE
        || code == FailoverErrorCode::INTERNAL_ERROR;
}

// ============================================================================
// § 5  In-flight request buffer contract
//
// During handover, the outgoing leader MUST drain its in-flight buffer or
// transfer ownership to the new leader within kHandoverDrainDeadline.
// Requests that cannot be completed or transferred are returned to callers
// with a retriable error — they are NEVER silently dropped.
// ============================================================================

/// Maximum in-flight request buffer size per leader node.
inline constexpr std::size_t kMaxInFlightRequests = 4096;

/// Budget for the outgoing leader to drain its in-flight buffer on handover.
inline constexpr std::chrono::milliseconds kHandoverDrainDeadline{1'000};

// ============================================================================
// § 6  Quorum contract
//
// Elections and commits require a strict majority: floor(N/2)+1 votes.
// The cluster size N MUST be odd for unambiguous majorities.
// ============================================================================

/// Compute the quorum size required for a cluster of @p cluster_size nodes.
[[nodiscard]] inline constexpr std::size_t quorumSize(std::size_t cluster_size) noexcept {
    return (cluster_size / 2u) + 1u;
}

} // namespace failover
} // namespace themis
