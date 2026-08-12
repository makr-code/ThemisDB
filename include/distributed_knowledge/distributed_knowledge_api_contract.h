/**
 * @file distributed_knowledge_api_contract.h
 * @brief Frozen distributed knowledge contract: entity lifecycle, federation, retrieval,
 *        and LWW conflict resolution.
 *
 * This header defines the normative contract for the distributed_knowledge module.
 * All components (entity stores, federation coordinators, retrieval engines, and
 * conflict resolvers) must honour these semantics within the v1.x major line.
 *
 * ## Entity Contract
 *
 * Entity-relation pairs are immutable after commit.  Readers see a consistent
 * snapshot; updates produce a new version rather than mutating in place.
 * Reads are eventually consistent: a read immediately after a write on a
 * different node may return the previous version until propagation completes.
 *
 * ## Federation Contract
 *
 * A cross-node query result is the union of per-node results with duplicates
 * removed.  The federation layer must not introduce phantom entities or omit
 * entities that exist on at least one reachable node.
 *
 * ## Conflict Resolution Contract
 *
 * When the same entity is written on two nodes concurrently, the Last-Write-Wins
 * (LWW) strategy is applied using the entity's wall-clock timestamp.  On a
 * timestamp tie, the node with the lexicographically larger node-ID wins.
 *
 * ## Retrieval Contract
 *
 * `neighbours()` returns ALL direct edges from the given node within the local
 * shard.  The result set MUST be complete (no silent omissions) and MUST NOT
 * contain duplicate edges.
 *
 * ## Versioning
 *
 * This contract is stable within v1.x.  Breaking changes require a v2.0 bump.
 *
 * @see src/distributed_knowledge/ROADMAP.md — Phase 1 item
 * @see include/distributed_knowledge/federated_rag_merger.h
 * @see include/distributed_knowledge/lora_federation_coordinator.h
 * @see include/distributed_knowledge/cross_shard_feedback_sync.h
 */

#pragma once

#include <chrono>
#include <cstdint>
#include <string>

namespace themis {
namespace distributed_knowledge {

// ============================================================================
// § 1  Entity size constraints
// ============================================================================

/// Maximum entity ID length in bytes.
inline constexpr std::size_t kMaxEntityIdBytes = 256;

/// Maximum relation type label length in bytes.
inline constexpr std::size_t kMaxRelationTypeBytes = 128;

/// Maximum entity payload size in bytes (serialised form).
inline constexpr std::size_t kMaxEntityPayloadBytes = 64 * 1024;

/// Maximum number of direct edges returned by neighbours() per query.
inline constexpr std::size_t kMaxNeighboursPerQuery = 10'000;

// ============================================================================
// § 2  Federation constraints
// ============================================================================

/// Default per-node federation query timeout.
inline constexpr std::chrono::milliseconds kFederationNodeTimeout{5'000};

/// Hard maximum per-node federation query timeout (operator upper bound).
inline constexpr std::chrono::milliseconds kFederationMaxNodeTimeout{30'000};

/// Maximum number of federation peer nodes queried in a single request.
inline constexpr std::size_t kMaxFederationPeers = 64;

/// Maximum total result size (entity count) returned by a federated query.
inline constexpr std::size_t kMaxFederationResultEntities = 100'000;

// ============================================================================
// § 3  LWW conflict resolution contract
// ============================================================================

/**
 * @brief Result of an LWW (Last-Write-Wins) conflict resolution decision.
 */
enum class LwwDecision : int {
    /// The local version is authoritative (higher timestamp or wins tie-break).
    LocalWins  = 0,
    /// The remote version is authoritative.
    RemoteWins = 1,
    /// Versions are identical; no merge needed.
    Identical  = 2,
};

/**
 * @brief Resolve an LWW conflict between two entity versions.
 *
 * @param local_ts    Wall-clock timestamp of the local version (µs since epoch).
 * @param remote_ts   Wall-clock timestamp of the remote version (µs since epoch).
 * @param local_node  Node ID of the local writer.
 * @param remote_node Node ID of the remote writer.
 * @return LwwDecision indicating which version wins.
 */
[[nodiscard]] inline LwwDecision resolveLww(
        std::int64_t local_ts, std::int64_t remote_ts,
        const std::string& local_node, const std::string& remote_node) noexcept {
    if (local_ts > remote_ts)  return LwwDecision::LocalWins;
    if (remote_ts > local_ts)  return LwwDecision::RemoteWins;
    // Tie-break: lexicographically larger node-ID wins.
    if (local_node > remote_node) return LwwDecision::LocalWins;
    if (remote_node > local_node) return LwwDecision::RemoteWins;
    return LwwDecision::Identical;
}

// ============================================================================
// § 4  Error taxonomy
// ============================================================================

/**
 * @brief Canonical error codes for the distributed_knowledge module.
 */
enum class DKErrorCode : int {
    /// No error.
    OK = 0,

    /// The requested entity does not exist on any reachable node.
    ENTITY_NOT_FOUND = 1,

    /// A cross-node federation query timed out on one or more peers.
    FEDERATION_TIMEOUT = 2,

    /// A conflict could not be resolved by LWW (e.g. clock skew beyond tolerance).
    CONFLICT_UNRESOLVABLE = 3,

    /// Local or remote graph data is structurally inconsistent.
    GRAPH_CORRUPTED = 4,

    /// The retrieval result would exceed kMaxNeighboursPerQuery or kMaxFederationResultEntities.
    RETRIEVAL_LIMIT_EXCEEDED = 5,

    /// A tombstone for a deleted entity was not propagated to all peers.
    TOMBSTONE_PROPAGATION_FAILED = 6,

    /// Schema or type mismatch during typed-field CRDT merge.
    CRDT_MERGE_TYPE_MISMATCH = 7,

    /// Internal distributed knowledge component error.
    INTERNAL_ERROR = 8,
};

/// Returns true for codes where the caller should retry with back-off.
[[nodiscard]] inline constexpr bool isRetryableCode(DKErrorCode code) noexcept {
    return code == DKErrorCode::FEDERATION_TIMEOUT
        || code == DKErrorCode::TOMBSTONE_PROPAGATION_FAILED;
}

// ============================================================================
// § 5  Path query contract
// ============================================================================

/// Maximum traversal depth for a path query.
inline constexpr std::size_t kMaxPathQueryDepth = 10;

/// Maximum number of paths returned by a single path query.
inline constexpr std::size_t kMaxPathQueryResults = 1'000;

// ============================================================================
// § 6  Consistency model
//
// Reads are eventually consistent with a bounded propagation delay.
// The maximum expected propagation delay under normal load is documented
// here as a SLO reference — it is NOT a hard guarantee.
// ============================================================================

/// Expected maximum propagation delay for an entity write across the cluster.
inline constexpr std::chrono::seconds kExpectedPropagationDelay{5};

} // namespace distributed_knowledge
} // namespace themis
