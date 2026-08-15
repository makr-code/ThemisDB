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

// ============================================================================
// § 7  Exception Safety Contract
//
// All destructors in the distributed_knowledge module are noexcept and may not
// throw under any circumstances. Violations cause std::terminate().
// ============================================================================

/**
 * @brief Exception safety guarantees for distributed_knowledge components.
 *
 * **ILoRAFederationCoordinator & LoRAFederationCoordinator:**
 *  - submitGradient(): weak exception safety; state reverted on throw
 *  - triggerAggregation(): strong exception safety; no state change on throw
 *  - Exceptions: std::runtime_error (budget exhausted, policy violation)
 *
 * **IFederatedDistillationCoordinator & FederatedDistillationCoordinator:**
 *  - submitSoftLabels(): weak exception safety
 *  - broadcastToStudents(): strong exception safety
 *  - Exceptions: std::runtime_error (policy, budget), std::invalid_argument
 *
 * **CrossShardFeedbackSync:**
 *  - publishFeedback(): weak exception safety (increments counter on backpressure)
 *  - handleInboundSummary(): strong exception safety
 *  - Exceptions: std::invalid_argument (dimension), std::runtime_error (ZeroTrust)
 *
 * **Destructor Contract:**
 *  All user-defined destructors in distributed_knowledge are marked noexcept.
 *  No destructor in this module may throw; if it does, std::terminate() is called.
 */

// ============================================================================
// § 8  Merge Strategy and Conflict Resolution Contract
//
// Canonical specification for cross-shard RAG result merging and conflict
// resolution semantics in FederatedRAGMerger.
// ============================================================================

/**
 * @brief Merge strategy algorithm selection contract.
 *
 * Three distinct strategies handle merging retrieval results from N shards,
 * each with different conflict resolution semantics for duplicate doc_ids
 * and tie-breaking behavior.
 */

/// Reciprocal Rank Fusion (RRF) merge strategy (default).
/// Formula: score(doc) = Σ_i 1 / (k + rank_i) per shard
/// Conflict: accumulated scores for duplicate doc_ids
/// Tie-break: stable sort preserves input order
/// Specialisation: if adapter_accuracy_delta > 0, multiply by specialisation_boost
inline constexpr const char* kMergeStrategyRRFDescription =
    "Reciprocal Rank Fusion: robust, scales scores for heterogeneous shard result ranges";

/// Score-weighted merge strategy.
/// Formula: score(doc) = Σ_i score_i × (1.0 + adapter_accuracy_delta_i)
/// Conflict: weighted-summed scores for duplicate doc_ids
/// Tie-break: stable sort preserves input order
/// Weights reflect shard accuracy claims; minimum weight 0.01 to avoid collapse
inline constexpr const char* kMergeStrategyWeightedDescription =
    "Score-weighted: respects shard accuracy, lightweight computation";

/// Round-robin merge strategy.
/// Algorithm: interleave documents across shards at each rank position
/// Conflict: all shards included at position (not deduplicated)
/// Tie-break: shard order in results vector determines interleave order
/// Use case: maximize diversity across multi-specialized shards
inline constexpr const char* kMergeStrategyRoundRobinDescription =
    "Round-robin: interleaved diversity sampling across shards";

/**
 * @brief Timeout and failure handling contract (DK-OR-T).
 *
 * When a shard fails to respond, times out, or returns malformed data,
 * the merge operation handles it gracefully:
 *
 * **Shard Failure Cases:**
 *  1. ShardRetrievalResult.ok == false: shard reported internal error
 *     → Skipped from merge; contributes 0 documents
 *
 *  2. ShardRetrievalResult.timed_out == true: shard exceeded deadline
 *     → Skipped from merge; contributes 0 documents
 *
 *  3. FederatedRAGMergerConfig.shard_timeout_ms == 0: fail-closed policy
 *     → If any shard_results passed to merge(): throw runtime_error("all shards timed out")
 *
 *  4. All shards in shard_results timed out or failed:
 *     → throw runtime_error("all shards timed out")
 *
 *  5. Some shards responded, others timed out:
 *     → Merge proceeds normally with responding shards; no error thrown
 *
 * **Exception Safety:**
 *  - merge() provides strong exception safety
 *  - On throw, no state change; safe to retry with same inputs
 *  - Idempotent: same shard_results always produce identical output
 *
 * **Example:**
 *  - 3 shards queried: shard_A times out, shard_B returns 5 docs, shard_C returns 7 docs
 *  - merge() succeeds with RRF/weighted/round-robin on shard_B and shard_C
 *  - MergedRAGContext.shards_queried = 3; shards_responded = 2
 *  - total_candidate_count = 12 (sum of responding shards)
 */

/**
 * @brief Deduplication contract.
 *
 * When FederatedRAGMergerConfig.deduplicate == true:
 *  - After merge strategy applied, removes documents with duplicate doc_id
 *  - Keeps first (highest-ranked) occurrence
 *  - Discards later occurrences silently
 *  - Order preserved: output documents in input order
 *  - Happens BEFORE top_k truncation
 *  - Result: unique_doc_count reflects deduplicated count
 *
 * **Conflict Resolution:**
 *  When RRF/weighted/round-robin merge produces same doc_id from different shards:
 *  - First occurrence (best ranking) is kept
 *  - Later duplicates discarded (metadata/shard_id not merged)
 *
 * **Top-K Truncation:**
 *  After dedup, final results trimmed to config.top_k
 *  - May result in < top_k docs if dedup reduced count below top_k
 *  - Deterministic: sort order preserved
 */

/**
 * @brief Configuration validation contract.
 *
 * FederatedRAGMergerConfig is valid iff:
 *  1. top_k > 0 (must return at least 1 document)
 *  2. rrf_constant > 0.0 (RRF denominator must be positive)
 *  3. specialisation_boost >= 1.0 (boost never reduces scores)
 *
 * Invalid config in FederatedRAGMerger ctor throws std::invalid_argument.
 *
 * **Per-field Semantics:**
 *  - strategy: MergeStrategy enum (RECIPROCAL_RANK_FUSION, SCORE_WEIGHTED, ROUND_ROBIN)
 *  - top_k: max documents in output (after dedup and truncation)
 *  - deduplicate: enable/disable duplicate removal by doc_id
 *  - rrf_constant: RRF formula denominator (Cormack 2009); typically 60
 *  - boost_specialised: enable/disable specialisation weighting
 *  - specialisation_boost: multiplier for specialized shard scores (>= 1.0)
 *  - shard_timeout_ms: per-shard timeout (0=fail-closed, UINT64_MAX=no timeout)
 *
 * @see include/distributed_knowledge/federated_rag_merger.h for detailed field docs
 */

/**
 * @brief Determinism and retry safety contract.
 *
 * FederatedRAGMerger.merge() is:
 *  - **Deterministic**: Same shard_results input always produces identical output
 *  - **Idempotent**: Safe to retry without state change or side effects
 *  - **Thread-safe**: Stateless read-only operations; concurrent merge() calls allowed
 *  - **Exception-safe**: Strong guarantee; no partial state change on throw
 *
 * Use case: Retry logic for transient shard failures
 */

} // namespace distributed_knowledge
} // namespace themis
