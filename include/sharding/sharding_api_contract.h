/*
 * ThemisDB | File: sharding_api_contract.h | Version: 1.0.0
 * Author: ThemisDB Contributors | Maturity: 🟢 PRODUCTION-READY
 * Status: Phase 1 — Frozen Contract
 * Purpose: Frozen sharding module API contract semantics for the active v1.x major line.
 */

/**
 * @file sharding_api_contract.h
 * @brief Frozen sharding module API contracts for the active v1.x line.
 *
 * This header defines the normative contract for the sharding module covering
 * routing (consistent hash), distributed transaction (2PC), WAL durability,
 * migration/rebalance state transitions, error taxonomy, and threading guarantees.
 *
 * @section scope Contract Scope
 *
 * The contracts below are binding for all implementations that participate in
 * the ThemisDB sharding pipeline:
 *   - Request router (ShardRouter, AdaptiveShardRouter, LocalityAwareRouter)
 *   - Consistent hash ring (ConsistentHash)
 *   - Distributed transaction coordinator (DistributedCoordinator, TwoPhaseCommit)
 *   - WAL (PaxosWal, MetadataWal, RaftWalIntegration)
 *   - Data migrator (DataMigrator)
 *   - Auto rebalancer (AutoRebalancer)
 *   - Quorum manager (QuorumManager)
 *   - Health monitor (HealthMonitor, HealthCheck)
 *
 * @section versioning Versioning
 *
 * This contract is stable within v1.x. Breaking changes require a v2.0 bump
 * with migration notes and a CHANGELOG entry.
 *
 * @see src/sharding/ROADMAP.md — Phase 1 item
 * @see tests/sharding/test_sharding_contract_hardening_focused.cpp — SCR-01..SCR-16
 * @see benchmarks/sharding/bench_sharding_release_gates.cpp — SRG-01..SRG-06
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <string>

namespace themis {
namespace sharding {

// ============================================================================
// § 1  Routing Contract: routeRequest()
//
// Consistent hash routing semantics:
//   - routeRequest(key) maps the key to the owning shard via a virtual-node
//     consistent hash ring (default kHashRingVnodes virtual nodes per shard).
//   - The mapping is stable: identical keys always route to the same shard
//     as long as the shard set is unchanged (ring stability guarantee).
//   - On shard addition/removal only ~1/N of keys are remapped (minimal
//     disruption guarantee).
//   - Fallback order: if the primary shard is unavailable, the router tries
//     the next shard in ring order up to kRoutingMaxFallbackDepth times.
//   - After all fallbacks are exhausted, routeRequest() returns SHARD_UNAVAILABLE.
//   - routeRequest() is thread-safe; the hash ring is immutable after
//     construction and updated only via atomic ring-swap on topology change.
// ============================================================================

/// Default number of virtual nodes per shard in the consistent hash ring.
inline constexpr int kHashRingVnodes = 150;

/// Maximum fallback depth when the primary shard is unavailable.
inline constexpr int kRoutingMaxFallbackDepth = 3;

/// Maximum number of shards in a single ring (prevents O(N) hash costs).
inline constexpr std::size_t kMaxShards = 4096;

// ============================================================================
// § 2  2PC Transaction Contract: prepare / commit / abort
//
// Two-phase commit invariants:
//   - prepare(): each participant records the transaction in its WAL before
//     returning PREPARED. No side effects are visible before commit().
//   - commit(): each participant applies the prepared writes and marks the
//     transaction COMMITTED in its WAL. commit() is idempotent: calling it
//     on an already-committed transaction is a no-op (no error).
//   - abort(): each participant rolls back all prepared writes and marks the
//     transaction ABORTED in its WAL. abort() is idempotent.
//   - Silent loss is NEVER permitted: every transaction MUST reach one of
//     {COMMITTED, ABORTED, IN_DOUBT} as its terminal state.
//   - IN_DOUBT state: if the coordinator crashes after prepare() but before
//     commit()/abort(), participants remain in IN_DOUBT until the coordinator
//     recovers and re-drives the decision. Participants in IN_DOUBT MUST block
//     conflicting writes (pessimistic locking) until resolved.
//   - Double-commit safety: a second commit() call for the same txn-id is a
//     no-op; the participant does NOT re-apply writes or emit a WAL entry.
// ============================================================================

/// Default 2PC prepare timeout per participant.
inline constexpr std::chrono::milliseconds kTwoPhaseCommitPrepareTimeout{5000};

/// Default 2PC commit/abort timeout per participant.
inline constexpr std::chrono::milliseconds kTwoPhaseCommitDecisionTimeout{5000};

/// Maximum time a participant may remain IN_DOUBT before triggering recovery.
inline constexpr std::chrono::seconds kInDoubtResolutionTimeout{30};

// ============================================================================
// § 3  WAL Durability Contract
//
// WAL append semantics:
//   - append() is synchronous and durable: the call does not return until the
//     entry is fsync'd (or equivalent) to stable storage.
//   - WAL entries are strictly ordered by LSN (Log Sequence Number). LSNs are
//     monotonically increasing with no gaps.
//   - Recovery replay: WAL entries are replayed in LSN order. Replay is
//     idempotent: replaying the same LSN range twice has no additional effect.
//   - Corruption detection: each WAL entry carries a CRC32 checksum. A
//     mismatch during replay triggers WAL_CORRUPTION error and halts replay.
//   - Truncation: WAL entries below the confirmed checkpoint LSN may be
//     reclaimed. Reclaimed entries are no longer available for replay.
// ============================================================================

/// Maximum WAL entry payload size in bytes.
inline constexpr std::size_t kMaxWalEntryBytes = 64u * 1024u;

/// Default WAL sync interval for batched fsync mode (0 = synchronous per append).
inline constexpr std::chrono::milliseconds kWalBatchSyncInterval{0};

// ============================================================================
// § 4  Migration / Rebalance Contract
//
// State transition guarantees during shard migration:
//   - Migration follows the state machine: IDLE → PLANNED → TRANSFERRING → VERIFYING → COMPLETE.
//   - During TRANSFERRING, the source shard accepts reads. Writes are dual-written
//     to both source and target to prevent data loss.
//   - MIGRATION_CONFLICT: if two migration operations target the same shard
//     concurrently, the second is rejected with MIGRATION_CONFLICT.
//   - A failed migration MUST roll back to the previous shard assignment.
//     Partial migrations that cannot roll back are logged as MIGRATION_FAULT.
//   - Rebalance respects kMaxConcurrentMigrations to bound I/O impact.
// ============================================================================

/// Maximum number of concurrent shard migrations permitted.
inline constexpr int kMaxConcurrentMigrations = 4;

/// Migration transfer timeout per shard.
inline constexpr std::chrono::seconds kMigrationTransferTimeout{300};

// ============================================================================
// § 5  Error Taxonomy
//
// All sharding components MUST map internal error states to one of these
// canonical error codes.
// ============================================================================

/**
 * @brief Canonical sharding error codes.
 *
 * Values are stable across v1.x. Any addition requires a CHANGELOG entry.
 * Removal or renumbering requires a v2.0 major bump.
 */
enum class ShardingErrorCode : int {
    /// Operation succeeded.
    OK                     =  0,
    /// Quorum of required shards is unavailable (< n/2+1 reachable).
    QUORUM_LOST            =  1,
    /// Transaction coordinator crashed and has not yet recovered.
    COORDINATOR_FAILURE    =  2,
    /// Target shard is unreachable or not accepting requests.
    SHARD_UNAVAILABLE      =  3,
    /// Two migration operations conflict on the same shard.
    MIGRATION_CONFLICT     =  4,
    /// WAL CRC check failed during replay; recovery halted.
    WAL_CORRUPTION         =  5,
    /// 2PC / Raft consensus timed out waiting for participant responses.
    CONSENSUS_TIMEOUT      =  6,
    /// Transaction is in IN_DOUBT state pending coordinator recovery.
    TRANSACTION_IN_DOUBT   =  7,
    /// Routing ring is inconsistent or no valid shard mapping exists.
    ROUTING_RING_INVALID   =  8,
    /// Rebalance/migration failed and could not roll back cleanly.
    MIGRATION_FAULT        =  9,
    /// Consistent hash ring has no registered shards.
    RING_EMPTY             = 10,
    /// Requested shard index exceeds kMaxShards.
    SHARD_INDEX_OUT_OF_RANGE = 11,
    /// Unclassified internal sharding error.
    INTERNAL_ERROR         = 12,
};

// ============================================================================
// § 6  Threading Contract
//
// Coordinator thread-safety:
//   - DistributedCoordinator is thread-safe. Concurrent prepare/commit/abort
//     calls from different client threads are serialised internally per txn-id.
//   - The coordinator's in-memory transaction table uses a sharded mutex to
//     reduce contention across unrelated transactions.
//
// Participant isolation:
//   - Each shard participant maintains an independent WAL and state machine.
//   - Participant state is not shared across shards; cross-shard coordination
//     goes through the coordinator only.
//   - HealthMonitor probes are read-only; concurrent probes do not affect
//     shard state.
//
// Hash ring:
//   - Ring lookup (routeRequest) is lock-free for reads.
//   - Ring topology updates use a copy-on-write atomic swap. During the swap,
//     in-flight lookups complete against the old ring; new lookups use the new ring.
// ============================================================================

/// Returns true when the given error code mandates fail-closed behaviour.
[[nodiscard]] inline constexpr bool isShardingFailClosedCode(ShardingErrorCode ec) noexcept {
    return ec == ShardingErrorCode::QUORUM_LOST
        || ec == ShardingErrorCode::COORDINATOR_FAILURE
        || ec == ShardingErrorCode::WAL_CORRUPTION
        || ec == ShardingErrorCode::INTERNAL_ERROR;
}

} // namespace sharding
} // namespace themis
