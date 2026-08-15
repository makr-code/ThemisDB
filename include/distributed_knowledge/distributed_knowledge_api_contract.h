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
 *
 * **Determinism Guarantee (§ 8.1):**
 *  Deduplication uses std::set (ordered container) instead of std::unordered_set:
 *  - Same input shard_results always produce identical output (deterministic)
 *  - Enables reproducible testing and reliable distributed debugging
 *  - Performance: O(n log n) worst-case; negligible for typical dedup sets (<100 docs)
 *  - Validated by DKRG-01..06 benchmarks; <5% regression expected <1%
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

// ============================================================================
// § 9  Consistency and Version Semantics
//
// Specification of consistency levels, version tracking, replication lag
// assumptions, and stale-read handling for distributed operations across
// federation, aggregation, merge, and feedback synchronization surfaces.
// ============================================================================

/**
 * @brief Consistency model for distributed operations.
 *
 * Distributed operations in the distributed_knowledge module employ a **weak
 * consistency model** with explicit consistency-level gates on critical paths.
 *
 * ## Consistency Levels
 *
 * Three consistency levels are distinguished:
 *
 * **Strong Consistency (Blocking)**
 *  - Operation blocks or times out until all replicas are synchronized
 *  - Use: Operations where correctness critically depends on current state
 *  - Examples: Privacy budget checks (verifyPrivacyBudget), conflict resolution
 *  - Trade-off: Higher latency, improved correctness guarantee
 *
 * **Causal Consistency (Ordered)**
 *  - Operation preserves causal ordering of related events
 *  - Replicas must respect happens-before relationships
 *  - Use: Round-based aggregation, distillation round advancement
 *  - Trade-off: Allows temporary divergence between replicas, maintains order
 *
 * **Eventual Consistency (Permissive)**
 *  - Operation uses latest available replica without synchronization
 *  - No ordering guarantee; replicas may diverge temporarily
 *  - Use: Stateless merge operations, feedback aggregation
 *  - Trade-off: Lowest latency, temporary divergence accepted
 *
 * ## Version Ordering and Causal Sequences
 *
 * Distributed operations use implicit versioning to establish causal order:
 *
 * **Gradient Aggregation (LoRA Federation):**
 *  - Round number provides causal ordering (round N+1 occurs after N)
 *  - Variant: EncryptedGradient.round explicitly encodes federation round
 *  - Semantics: Gradient for round N is ignored if current_round_ != N
 *    (ensures strict ordering; future rounds rejected, stale rounds dropped)
 *  - Consistency: **Causal** (round N+1 depends on N)
 *  - Implementation: submitGradient() silently ignores stale/future rounds (intentional)
 *
 * **Distillation Coordinator (Teacher-Student):**
 *  - Round number provides causal ordering (round N+1 occurs after N)
 *  - Variant: DistillationRound.round explicitly encodes distillation round
 *  - Semantics: Broadcast at round N must follow submitSoftLabels() at N
 *  - Consistency: **Causal** (round N+1 broadcast depends on N)
 *  - Implementation: broadcastToStudents() advances round on first submit after
 *    prior broadcast (explicit state machine; prevents out-of-order broadcast)
 *
 * **Federated RAG Merge (Stateless):**
 *  - No version vector; merge is stateless and idempotent
 *  - Variant: ShardRetrievalResult.timestamp provides temporal context (optional)
 *  - Semantics: Merge depends only on input shard_results, not coordinator state
 *  - Consistency: **Eventual** (shard responses may be from different rounds/times)
 *  - Implication: Same shard_results always produce identical output
 *  - Safe-read: Timestamps document intent but do not enforce synchronization
 *
 * **Cross-Shard Feedback Sync:**
 *  - Feedback summary has created_at timestamp (gossip-level temporal context)
 *  - Variant: No round numbers; timestamp-based eventual consistency
 *  - Semantics: Feedback publication is idempotent (dedup by summary_id)
 *  - Consistency: **Eventual** (feedback may arrive out-of-order at other shards)
 *  - Implication: Inbound feedback ordering not guaranteed; dedup required
 *
 * ## Replication Lag Assumptions
 *
 * **Federation Coordinator (Gradient/Distillation):**
 *  - Expected max lag: 1-2 federation rounds (not enforced, implicit)
 *  - Shard timeout: default kFederationNodeTimeout = 5000ms (DK-OR-T)
 *  - Acceptable lag: Shard may be stale relative to current_round_
 *  - Policy: Stale data **handled gracefully** (silently skipped or aggregated)
 *  - Correctness: Stale gradients at round N-k are idempotent over aggregation;
 *    future rounds correct for any missed contributions
 *  - Failure case: If shard doesn't respond: skip round, rely on eventual catch-up
 *    (see Batch 1 diagnostics for timeout handling)
 *
 * **RAG Merge (Shard Retrieval):**
 *  - Expected max lag: 0 rounds (assumed; shard provides best-effort current snapshot)
 *  - Shard timeout: configurable via FederatedRAGMergerConfig.shard_timeout_ms
 *  - Acceptable lag: Shard ranking may reflect stale index (lag OK within timeout)
 *  - Policy: Stale data **accepted** (merge logic tolerates heterogeneous freshness)
 *  - Correctness: Merge strategies (RRF, weighted, round-robin) are robust to
 *    score/rank misalignment; cross-shard bias naturally bounds relevance impact
 *  - Failure case: Timed-out shards skipped; if all timeout: fail-closed (error thrown)
 *
 * **Cross-Shard Feedback:**
 *  - Expected max lag: unbounded (gossip layer assumption)
 *  - Acceptable lag: Feedback may arrive minutes/hours late at distant shards
 *  - Policy: Stale data **accepted and deduplicated** (summary_id-based dedup)
 *  - Correctness: Feedback aggregation is commutative over time; late arrival does
 *    not corrupt state (idempotent operation)
 *  - Failure case: If gossip sink backpressures: skip publish, track skipped_count
 *
 * ## Stale-Read Handling (Intentional Eventual Consistency)
 *
 * **Pattern: Stale Data Accepted**
 *
 * Multiple operations explicitly accept stale or potentially divergent data:
 *
 *  1. **submitGradient() ignores stale/future rounds**
 *     ```
 *     if (gradient.round != current_round_) {
 *         return; // silently ignore stale or future rounds
 *     }
 *     ```
 *     - Rationale: Gradient aggregation is commutative over time; skipped rounds
 *       are corrected by later aggregations when shard catches up
 *     - Consistency: Intentional; causal ordering by round number, not absolute sync
 *     - Safety: Correctness preserved because FedAvg aggregation is idempotent
 *
 *  2. **Merge with heterogeneous shard freshness**
 *     - Shard A may return results from t=T, Shard B from t=T-100ms
 *     - Merge strategies (RRF, weighted) are robust to this divergence
 *     - Rationale: Cross-shard relevance bias naturally bounds impact of stale ranks
 *     - Safety: Deduplication ensures doc_id uniqueness regardless of rank order
 *
 *  3. **Feedback deduplication on late arrival**
 *     - Feedback summary created_at may be hours old when received
 *     - Policy: Checked against created_at, not receive time
 *     - Rationale: Idempotent aggregation; late feedback does not corrupt state
 *     - Safety: Dedup by summary_id prevents double-counting
 *
 * ## Consistency Failure Modes and Recovery
 *
 * **Timeout Handling (DK-OR-T):**
 *  - Shard query timeout → shard marked timed_out=true in result
 *  - Federation: Timeout shards cause round skip (no contribution from that shard)
 *  - RAG: Timeout shards excluded from merge; if all timeout: fail-closed
 *  - Recovery: Coordinator retries on next round; shard eventually catches up
 *
 * **Policy Gate Violations:**
 *  - Policy gate rejects operation (e.g., distillation broadcast policy rejection)
 *  - Result: std::runtime_error thrown; state unchanged
 *  - Recovery: Caller must retry with different parameters or escalate
 *
 * **Privacy Budget Exhaustion:**
 *  - verifyPrivacyBudget() detects DP epsilon spent >= budget
 *  - Result: std::runtime_error thrown (strong consistency guard)
 *  - Recovery: No automatic recovery; manual reset required
 *
 * ## DiagnosticEmitter Context (Reference: Batch 1 § 7)
 *
 * When consistency failures occur, DiagnosticEmitter context should include:
 *  - `consistency_level`: "strong" | "causal" | "eventual"
 *  - `replication_lag_ms`: observed lag (if measurable)
 *  - `fallback_used`: true if eventual consistency fallback invoked
 *  - Example: Timeout causing fallback from strong to eventual consistency
 *
 * See `DiagnosticEmitter::listener_pattern` (Batch 1) for full listener model.
 */

/**
 * @brief Aggregation Safety and Determinism (§ 9.1)
 *
 * **Gradient Aggregation Safety (LoRAFederationCoordinator):**
 *
 * Median aggregation includes defensive bounds checking:
 *  - Empty value vector case: returns 0.0 (safe default)
 *  - Non-empty vector: computes median correctly (even or odd count)
 *  - Guaranteed: No out-of-bounds array access regardless of precondition violations
 *
 * **Aggregation Determinism:**
 *  - FedAvg: sum(v_i × w_i) / sum(w_i) is deterministic (floating-point commutative)
 *  - Median: std::sort on doubles ensures deterministic ordering
 *  - Result: Same gradient inputs from same shards always produce same delta
 *  - Implication: Tests reproducible with kDKContractSeed=42 (deterministic RNG)
 *
 * **Container Safety (§ 9.1.1):**
 *  - key_values population guarantees: non-empty vectors for each key
 *  - But: defensive check added to prevent accidental violations
 *  - Pattern: `if (n == 0) aggregated[key] = 0.0; else compute_median(...)`
 *  - Result: Bounds-safe aggregation with clear error semantics
 */

} // namespace distributed_knowledge
} // namespace themis
