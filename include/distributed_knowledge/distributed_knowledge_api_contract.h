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
#include <map>
#include <string>

// Forward-declared in this header; full definition in adapter_capability_announcement.h.
// Include that header directly when the full type is needed.
namespace themis::distributed_knowledge { struct AdapterCapabilityAnnouncement; }

namespace themis {
namespace distributed_knowledge {

// ============================================================================
// § 1  Entity size constraints
// ============================================================================

inline constexpr std::size_t kMaxEntityIdBytes = 256;

inline constexpr std::size_t kMaxRelationTypeBytes = 128;

inline constexpr std::size_t kMaxEntityPayloadBytes = 64 * 1024;

inline constexpr std::size_t kMaxNeighboursPerQuery = 10'000;

// ============================================================================
// § 2  Federation constraints
// ============================================================================

inline constexpr std::chrono::milliseconds kFederationNodeTimeout{5'000};

inline constexpr std::chrono::milliseconds kFederationMaxNodeTimeout{30'000};

inline constexpr std::size_t kMaxFederationPeers = 64;

inline constexpr std::size_t kMaxFederationResultEntities = 100'000;

// ============================================================================
// § 3  LWW conflict resolution contract
// ============================================================================

enum class LwwDecision : int {
    LocalWins  = 0,
    RemoteWins = 1,
    Identical  = 2,
};

[[nodiscard]] inline LwwDecision resolveLww(
        std::int64_t local_ts, std::int64_t remote_ts,
        const std::string& local_node, const std::string& remote_node) noexcept {
    if (local_ts > remote_ts) {
      return LwwDecision::LocalWins;
    }
    if (remote_ts > local_ts) {
      return LwwDecision::RemoteWins;
    }
    // Tie-break: lexicographically larger node-ID wins.
    if (local_node > remote_node) {
      return LwwDecision::LocalWins;
    }
    if (remote_node > local_node) {
      return LwwDecision::RemoteWins;
    }
    return LwwDecision::Identical;
}

// ============================================================================
// § 4  Error taxonomy
// ============================================================================

enum class DKErrorCode : int {
    OK = 0,

    ENTITY_NOT_FOUND = 1,

    FEDERATION_TIMEOUT = 2,

    CONFLICT_UNRESOLVABLE = 3,

    GRAPH_CORRUPTED = 4,

    RETRIEVAL_LIMIT_EXCEEDED = 5,

    TOMBSTONE_PROPAGATION_FAILED = 6,

    CRDT_MERGE_TYPE_MISMATCH = 7,

    INTERNAL_ERROR = 8,

    TRUST_GATE_REJECTED = 9,
};

[[nodiscard]] inline constexpr bool isRetryableCode(DKErrorCode code) noexcept {
    return code == DKErrorCode::FEDERATION_TIMEOUT
        || code == DKErrorCode::TOMBSTONE_PROPAGATION_FAILED;
}

// ============================================================================
// § 5  Path query contract
// ============================================================================

inline constexpr std::size_t kMaxPathQueryDepth = 10;

inline constexpr std::size_t kMaxPathQueryResults = 1'000;

// ============================================================================
// § 6  Consistency model
//
// Reads are eventually consistent with a bounded propagation delay.
// The maximum expected propagation delay under normal load is documented
// here as a SLO reference — it is NOT a hard guarantee.
// ============================================================================

inline constexpr std::chrono::seconds kExpectedPropagationDelay{5};

// ============================================================================
// § 7  Exception Safety Contract
//
// All destructors in the distributed_knowledge module are noexcept and may not
// throw under any circumstances. Violations cause std::terminate().
// ============================================================================


// ============================================================================
// § 8  Merge Strategy and Conflict Resolution Contract
//
// Canonical specification for cross-shard RAG result merging and conflict
// resolution semantics in FederatedRAGMerger.
// ============================================================================


inline constexpr const char* kMergeStrategyRRFDescription =
    "Reciprocal Rank Fusion: robust, scales scores for heterogeneous shard result ranges";

inline constexpr const char* kMergeStrategyWeightedDescription =
    "Score-weighted: respects shard accuracy, lightweight computation";

inline constexpr const char* kMergeStrategyRoundRobinDescription =
    "Round-robin: interleaved diversity sampling across shards";





// ============================================================================
// § 9  Consistency and Version Semantics
//
// Specification of consistency levels, version tracking, replication lag
// assumptions, and stale-read handling for distributed operations across
// federation, aggregation, merge, and feedback synchronization surfaces.
// ============================================================================



// ============================================================================
// § 10  Merge Hardening Policy (Phase 2 / Q4 2026)
//
// Defines deterministic behaviour under multi-shard timeout permutations and
// bounded deduplication windows.  Applied to FederatedRAGMerger and
// LoRAFederationCoordinator to harden merge paths against partial failures.
// ============================================================================

enum class TimeoutBehavior : uint8_t {
    FAIL_CLOSED  = 0,
    BEST_EFFORT  = 1,
};

struct MergeHardeningPolicy {
    uint64_t max_merge_latency_ms    = 0u;
    uint32_t dedup_window_size       = 0u;
    uint32_t partial_shard_min_count = 0u;
    TimeoutBehavior timeout_behavior = TimeoutBehavior::BEST_EFFORT;

    [[nodiscard]] constexpr bool isConstrained() const noexcept {
        return max_merge_latency_ms != 0u
            || dedup_window_size    != 0u
            || partial_shard_min_count != 0u
            || timeout_behavior == TimeoutBehavior::FAIL_CLOSED;
    }
};

// ============================================================================
// § 11  Distillation Bounded Policy (Phase 2 / Q4 2026)
//
// Applies bounded runtime contracts to FederatedDistillationCoordinator so
// that policy-gate violations surface as explicit errors rather than silent
// degradation.
// ============================================================================

enum class PolicyGateEnforcement : uint8_t {
    FAIL_CLOSED = 0,
    LOG_ONLY    = 1,
};

struct DistillationBoundedPolicy {
    uint32_t max_distillation_rounds  = 0u;
    double   privacy_budget_hard_limit = 0.0;
    PolicyGateEnforcement policy_gate_enforcement = PolicyGateEnforcement::FAIL_CLOSED;

    [[nodiscard]] constexpr bool isConstrained() const noexcept {
        return max_distillation_rounds != 0u || privacy_budget_hard_limit > 0.0;
    }
};

// ============================================================================
// § 12  Federation Trust Policy (Phase 3 / Q4 2026)
//
// Defines a fail-closed trust gate evaluated before any Aggregation or Merge
// operation is started.  If the gate rejects an AdapterCapabilityAnnouncement,
// the operation MUST NOT proceed and MUST return TRUST_GATE_REJECTED.
// ============================================================================

enum class TrustDecision : uint8_t {
    PERMIT = 0,
    REJECT = 1,
};

class IFederationTrustPolicy {
public:
    /**
     * @brief IFederation Trust Policy.
     * @return Return value.
     */
    virtual ~IFederationTrustPolicy() = default;

    [[nodiscard]] virtual TrustDecision evaluateTrustGate(
        const AdapterCapabilityAnnouncement& announcement) const noexcept = 0;
};

class AlwaysPermitTrustPolicy final : public IFederationTrustPolicy {
public:
    [[nodiscard]] TrustDecision evaluateTrustGate(
        [[maybe_unused]] const AdapterCapabilityAnnouncement& /*announcement*/) const noexcept override {
        return TrustDecision::PERMIT;
    }
};

// ============================================================================
// § 13  Diagnostic Event Model (Phase 3 / Q4 2026)
//
// Structured events emitted by the distributed_knowledge module for operator
// visibility into federation incidents, merge failures, and dedup collisions.
// ============================================================================

enum class DKDiagnosticSeverity : uint8_t {
    INFO    = 0, ///< Informational; no action required.
    WARNING = 1, ///< Degraded path taken; operator should investigate.
    ERROR   = 2, ///< Operation failed; immediate attention recommended.
    FATAL   = 3, ///< Unrecoverable failure; service restart may be required.
};

enum class DKDiagnosticEventType : uint8_t {
    MERGE_TIMEOUT        = 0, ///< A shard merge exceeded the latency budget.
    DEDUP_COLLISION      = 1, ///< A duplicate doc_id or summary_id was detected.
    PARTIAL_SHARD_MERGE  = 2, ///< Merge completed with fewer shards than requested.
    TRUST_GATE_REJECT    = 3, ///< Trust gate rejected an incoming announcement.
    FEDERATION_ROLLBACK  = 4, ///< A federation round was rolled back.
    POLICY_VIOLATION     = 5, ///< A bounded-policy or budget constraint was violated.
    PRIVACY_BUDGET_WARN  = 6, ///< DP epsilon approaching or at the budget ceiling.
};

struct DKDiagnosticEvent {
    DKDiagnosticEventType type{DKDiagnosticEventType::MERGE_TIMEOUT};
    DKDiagnosticSeverity  severity{DKDiagnosticSeverity::WARNING};

    std::string shard_id;
    std::string operation_id;
    std::string timestamp_utc;
    std::string cause;
    std::map<std::string, std::string> metadata;
};

} // namespace distributed_knowledge
} // namespace themis
