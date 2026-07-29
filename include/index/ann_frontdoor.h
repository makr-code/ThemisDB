/**
 * @file ann_frontdoor.h
 * @brief Front-door interface for approximate-nearest-neighbour index operations.
 *
 * Provides a unified entry-point that dispatches ANN queries to the
 * appropriate backend (HNSW, IVF, flat-scan) based on runtime configuration.
 */

#pragma once

// ANN Frontdoor — explicit first universal retrieval gate
//
// Purpose:
//   Single entry point for all approximate nearest-neighbour (ANN) queries.
//   Selects the appropriate backend (HNSW, ScaNN, DiskANN, Distributed) based
//   on dataset size, query context, hot/cold tier, recall target, and latency
//   budget.  Downstream layers (Tensor mid-layer, Graph Truth Layer) receive
//   candidates exclusively from this class.
//
// Architecture position:
//   Query → AnnFrontdoor → [HNSW | ScaNN | DiskANN | Distributed] → candidates
//                                                ↓
//                                    Tensor Mid-Layer (re-ranking)
//                                                ↓
//                                    Graph Truth Layer (validation)
//
// Thread safety:
//   search() and planStrategy() are read-only and safe to call concurrently.
//   registerBackend() / registerVectorIndexManager() must not be called
//   concurrently with search() — perform all registration before first query.

#include "index/ann_index.h"
#include "index/tiered_index_manager.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {

// Forward declarations — avoid pulling in heavy headers here.
class VectorIndexManager;

namespace index {

// ============================================================================
// AnnStrategy — backend selected by the frontdoor router
// ============================================================================

/**
 * @brief Backend strategy chosen by AnnFrontdoor::planStrategy().
 *
 * The strategy is determined at query time from AnnQueryContext; callers can
 * inspect it via AnnFrontdoorResult::strategy_used for observability.
 */
enum class AnnStrategy : uint8_t {
    /// Hierarchical Navigable Small World — fast in-memory graph index.
    /// Best for datasets up to ~1 M vectors on the hot tier.
    HNSW,

    /// ScaNN (Google, ICML'20) — tree-AH partitioned quantized search.
    /// Best for datasets 1 M–50 M vectors; pure-C++ fallback for cold tier.
    SCANN,

    /// DiskANN (Microsoft, NeurIPS'19) — disk-resident graph index.
    /// Best for billion-scale datasets; requires THEMIS_ENABLE_DISKANN.
    DISKANN,

    /// Cross-shard distributed search; aggregates from multiple shard
    /// summaries before returning candidates.
    DISTRIBUTED,

    /// Linear brute-force scan.  Used when no index is registered for the
    /// requested scope or the dataset is trivially small (< 1 000 vectors).
    FLAT_BRUTE_FORCE,
};

/**
 * @brief Logical scope classification for ANN registration and routing.
 *
 * All six artifact classes named in the ANN Frontdoor contract are first-class
 * scopes.  The kind is recorded in the retrieval plan and forwarded to the
 * Tensor mid-layer as observability metadata.
 *
 * Routing influence:
 * - ShardSummary: when shard_aware == true, routes to DISTRIBUTED.
 * - Document, Chunk, Entity: use standard hot/cold routing; no special fanout.
 * - Adapter, Package: use standard hot/cold routing; no special fanout.
 * - Generic: same as Document/Chunk/Entity; use when scope is unknown.
 */
enum class AnnScopeKind : uint8_t {
    /// Unclassified or unknown artifact scope.
    Generic,
    /// LoRA / AdaLoRA adapter embedding (adapter fingerprint).
    Adapter,
    /// Knowledge package embedding (package fingerprint).
    Package,
    /// Cross-shard summary embedding; eligible for distributed fan-out.
    ShardSummary,
    /// Top-level document embedding (full-document vector).
    Document,
    /// Sub-document chunk embedding (passage / paragraph vector).
    Chunk,
    /// Knowledge-graph entity embedding.
    Entity,
};

/**
 * @brief Metadata for one shard in a distributed plan.
 *
 * Used for cost-aware pruning and shard selection ranking.
 */
struct ShardMetadata {
    std::string shard_id;
    double estimated_cost = 1.0;      /// latency_ms + io_cost
    double estimated_relevance = 0.5; /// 0.0-1.0, inferred from shard summary
    double estimated_freshness = 1.0; /// 0.0-1.0, age factor
    double estimated_locality = 0.5;  /// 0.0-1.0, network locality
    double estimated_recall = 0.95;   /// expected recall on this shard
};

// ============================================================================
// AnnQueryContext — routing hints provided by the caller
// ============================================================================

/**
 * @brief Per-query routing context for the ANN frontdoor.
 *
 * All fields are optional hints.  The frontdoor makes a best-effort routing
 * decision using whichever hints are available; missing fields use safe
 * defaults that favour correctness over performance.
 */
struct AnnQueryContext {
    /// Estimated number of indexed vectors for the target scope.
    /// A value of 0 means "unknown"; the frontdoor will prefer HNSW.
    std::size_t dataset_size = 0;

    /// True when the target index is fully resident in memory (HOT tier).
    /// False triggers disk-oriented strategies (DiskANN / ScaNN cold path).
    bool hot_tier = true;

    /// When true, the query is fanned out across per-shard ANN summaries
    /// and results are merged.  Requires shard backends to be registered.
    bool shard_aware = false;

    /// Fraction of true nearest neighbours that must be in the returned set.
    /// Range [0.0, 1.0].  Higher values select slower, higher-quality backends.
    double recall_target = 0.95;

    /// Soft upper bound on acceptable end-to-end search latency in milliseconds.
    /// The frontdoor uses this to avoid selecting high-latency cold-path backends
    /// when a latency budget is tight.
    double latency_budget_ms = 10.0;

    /// Optional scope identifier.  When non-empty, the frontdoor first checks
    /// for a backend registered under this key before falling back to the
    /// global backend.
    ///
    /// Use cases:
    ///   - Shard-scoped retrieval: shard_id = "shard-42"
    ///   - Adapter retrieval: adapter_id = "lora-codegen-v3"
    ///   - Package retrieval: "pkg:themis-ner-2026q2"
    std::string scope_id;

    /// Stable request correlation id propagated across retrieval layers.
    std::string correlation_id;

    /// Optional confidence policy version propagated from caller.
    std::string confidence_policy_version;

    /// Optional confidence threshold key propagated from caller.
    std::string confidence_threshold_key;
};

/**
 * @brief Explicit retrieval plan produced by planRetrieval().
 *
 * This object makes hot/cold routing transparent before any backend is
 * queried. It can be logged, inspected by the query planner, or forwarded to
 * downstream layers.
 */
struct AnnRetrievalPlan {
    /// Final strategy chosen by the router.
    AnnStrategy strategy = AnnStrategy::FLAT_BRUTE_FORCE;

    /// Effective tier after consulting TieredIndexManager when available.
    IndexTierMeta::Tier effective_tier = IndexTierMeta::Tier::HOT;

    /// Scope classification for the routed target.
    AnnScopeKind scope_kind = AnnScopeKind::Generic;

    /// True when the router expects the result to be merged from shards.
    bool distributed = false;

    /// True when the chosen path is hot-tier optimized (HNSW path).
    bool hot_path = false;

    /// Human-readable route explanation.
    std::string reason;

    /// Shards included after cost-aware pruning (if applicable).
    std::vector<std::string> pruned_shard_ids;
};

// ============================================================================
// AnnFrontdoorResult — candidates + routing metadata returned by search()
// ============================================================================

/**
 * @brief Result bundle returned by AnnFrontdoor::search().
 *
 * The @ref candidates list is sorted by ascending distance (closest first).
 * @ref strategy_used and @ref routing_reason are provided for observability
 * and can be forwarded to the Tensor mid-layer for re-ranking decisions.
 */
struct AnnFrontdoorResult {
    /// Top-k candidates, sorted by ascending distance.
    std::vector<AnnSearchResult> candidates;

    /// Strategy that was actually executed.
    AnnStrategy strategy_used = AnnStrategy::FLAT_BRUTE_FORCE;

    /// Estimated recall fraction for the executed strategy (0.0–1.0).
    /// Derived from the backend's known recall characteristics; not measured.
    double estimated_recall = 0.0;

    /// Human-readable explanation of the routing decision.
    /// Example: "HNSW selected: dataset_size=120000 < hnsw_max=1000000, hot_tier=true"
    std::string routing_reason;

    /// Machine-readable code for the selected routing branch.
    std::string routing_reason_code;

    /// Stable request correlation id propagated from AnnQueryContext.
    std::string correlation_id;

    /// Effective confidence policy version for this ANN decision.
    std::string confidence_policy_version;

    /// Effective confidence threshold key for this ANN decision.
    std::string confidence_threshold_key;

    /// Selected fallback mode for this ANN decision.
    std::string fallback_mode = "none";

    /// Machine-readable reason code when fallback was applied.
    std::string fallback_reason_code;

    /// True when results were merged from multiple shard backends.
    bool is_distributed = false;

    /// Number of shard/global backends selected for distributed execution.
    std::size_t shards_attempted = 0;

    /// Number of backends that returned at least one successful response.
    std::size_t shards_succeeded = 0;

    /// Number of backends that failed after all retry attempts.
    std::size_t shards_failed = 0;

    /// Candidate count immediately after merge, before top-k truncation.
    std::size_t merged_candidates_before_trim = 0;

    /// True when at least one backend failed but partial results were returned.
    bool partial_results = false;

    /// Machine-readable merge policy used for distributed execution.
    std::string distributed_merge_policy;
};

// ============================================================================
// AnnFrontdoor
// ============================================================================

/**
 * @brief Explicit ANN frontdoor — the single universal retrieval gate.
 *
 * ### Responsibilities
 * - Accept a query vector and AnnQueryContext.
 * - Select the optimal ANN backend via planStrategy().
 * - Execute the search and return a ranked candidate list.
 * - Provide routing transparency (strategy_used, routing_reason).
 *
 * ### Supported ANN artifact classes (AnnScopeKind)
 * The frontdoor supports all six artifact classes defined by the ANN Frontdoor
 * contract:
 *
 * | Scope kind    | Typical scope_id prefix | Routing notes                      |
 * |---------------|-------------------------|------------------------------------|
 * | Document      | `doc:`                  | standard hot/cold routing          |
 * | Chunk         | `chunk:`                | standard hot/cold routing          |
 * | Entity        | `entity:`               | standard hot/cold routing          |
 * | Adapter       | `adapter:`              | standard hot/cold routing          |
 * | Package       | `pkg:`                  | standard hot/cold routing          |
 * | ShardSummary  | `shard:`                | DISTRIBUTED when shard_aware=true  |
 *
 * ### Backend selection rules (in priority order)
 * 1. Scope-specific backend registered under context.scope_id (if set).
 * 2. DISTRIBUTED  when context.shard_aware == true and shard backends exist.
 * 3. HNSW         when hot_tier && dataset_size <= hnsw_max_elements.
 * 4. SCANN        when dataset_size <= scann_max_elements.
 * 5. DISKANN      when diskann_available == true.
 * 6. FLAT_BRUTE_FORCE as safe fallback.
 *
 * ### Hot/cold tiering
 * The frontdoor consults the optionally registered TieredIndexManager to
 * verify the actual tier of the target index.  If the index is on WARM/COLD
 * tier, disk-oriented strategies (ScaNN / DiskANN) are preferred regardless
 * of dataset_size.
 *
 * ### Integration
 * Register the AnnFrontdoor as the vector-leg supplier in HybridRetriever:
 * @code
 *   auto fd = std::make_shared<AnnFrontdoor>();
 *   fd->registerVectorIndexManager(vim);
 *   retriever.setAnnFrontdoor(fd);
 * @endcode
 *
 * @note All public const methods are thread-safe.
 *       Registration methods must be called before the first search().
 */
class AnnFrontdoor {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Construction-time configuration for AnnFrontdoor.
     *
     * Thresholds are compared against AnnQueryContext::dataset_size to pick
     * the default strategy when no scope-specific backend is registered.
     */
    struct Config {
        /// Datasets at or below this size use HNSW (fast in-memory graph).
        std::size_t hnsw_max_elements = 1'000'000;

        /// Datasets above hnsw_max_elements and at or below this size use ScaNN.
        std::size_t scann_max_elements = 50'000'000;
        // Above scann_max_elements: DiskANN (if available) or ScaNN fallback.

        /// Number of nearest-neighbour candidates returned when the caller
        /// does not specify k explicitly.
        int default_k = 100;

        /// Whether the DiskANN backend was compiled in (THEMIS_ENABLE_DISKANN).
        /// Set to true only if a DiskANN IAnnIndex is actually registered.
        bool diskann_available = false;

        /// Max number of shard backends queried in DISTRIBUTED mode.
        /// Value 0 means no explicit limit (all registered shard backends).
        std::size_t distributed_max_fanout = 0;

        /// Number of retry attempts after the initial shard call.
        /// Example: 0 => single try, 1 => one retry.
        int distributed_retry_attempts = 1;

        /// If true, return partial shard results when one or more shards fail.
        /// If false, all-shard failure drives fail-closed semantics.
        bool distributed_allow_partial_results = true;

        /// If true, include the global backend in distributed fan-out.
        bool distributed_include_global_backend = true;

        /// Cost-aware pruning budget: max total "cost units" across all shards.
        /// Cost is estimated as latency_ms + io_cost.
        /// Value 0 means no budget limit (all selected shards execute).
        double distributed_cost_budget = 0.0;

        /// Quality floor: minimum estimated recall ratio for shard participation.
        /// Shards with estimated recall below this floor are pruned.
        /// Range [0.0, 1.0]; 0.0 means no quality gate.
        double distributed_quality_floor = 0.0;

        /// Weights for utility scoring: alpha*relevance + beta*freshness + gamma*locality.
        double distributed_utility_alpha = 0.6;
        double distributed_utility_beta = 0.2;
        double distributed_utility_gamma = 0.2;
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Construct with default configuration.
     */
    AnnFrontdoor();

    /**
     * @brief Construct with explicit configuration.
     * @param config  Routing thresholds and defaults.
     */
    explicit AnnFrontdoor(Config config);
    ~AnnFrontdoor();

    // Non-copyable; move is allowed.
    AnnFrontdoor(const AnnFrontdoor&)            = delete;
    AnnFrontdoor& operator=(const AnnFrontdoor&) = delete;
    AnnFrontdoor(AnnFrontdoor&&)                 = default;
    AnnFrontdoor& operator=(AnnFrontdoor&&)      = default;

    // -----------------------------------------------------------------------
    // Backend registration
    // -----------------------------------------------------------------------

    /**
     * @brief Register a raw IAnnIndex backend for a given scope.
     *
     * @param scope_id  Matches AnnQueryContext::scope_id.  Use the empty
     *                  string "" for the global/default backend.
     * @param backend   Owning pointer to an IAnnIndex implementation.
     *                  Must not be nullptr.
     *
     * @note Overwrites any previously registered backend for the same scope.
     */
    void registerBackend(std::string scope_id,
                         std::shared_ptr<IAnnIndex> backend,
                         AnnScopeKind kind = AnnScopeKind::Generic);

    /**
     * @brief Register or update the logical kind for a scope.
     *
     * This is useful when the backend is already registered, but the caller
     * wants to mark it explicitly as adapter-, package-, or shard-summary
     * scoped for clearer retrieval planning.
     */
    void registerScopeKind(std::string scope_id, AnnScopeKind kind);

    /**
     * @brief Return the registered scope kind, or Generic if unknown.
     */
    [[nodiscard]] AnnScopeKind getScopeKind(const std::string& scope_id) const noexcept;

    /**
     * @brief Register the global VectorIndexManager (HNSW path).
     *
     * Used for the HNSW strategy when no scope-specific IAnnIndex is set.
     *
     * @param vim  Must not be nullptr.
     */
    void registerVectorIndexManager(std::shared_ptr<VectorIndexManager> vim);

    /**
     * @brief Register the TieredIndexManager for hot/cold tier queries.
     *
     * When set, the frontdoor calls TieredIndexManager::getMetadata() to
     * verify whether a named index is on the HOT tier before selecting HNSW.
     * Falls back to AnnQueryContext::hot_tier when the manager is absent.
     *
     * @param tim  May be nullptr (disables tier-aware routing).
     */
    void registerTieredIndexManager(
        std::shared_ptr<TieredIndexManager> tim);

    // -----------------------------------------------------------------------
    // Retrieval
    // -----------------------------------------------------------------------

    /**
     * @brief Execute an ANN search through the frontdoor.
     *
     * Selects the appropriate backend, executes the query, and returns a
     * ranked candidate list suitable for forwarding to the Tensor mid-layer.
     *
     * @param query_vector  Pointer to a float array of @p dim elements.
     *                      Must not be nullptr.
     * @param dim           Embedding dimensionality.  Must match the index.
     * @param k             Number of nearest-neighbour candidates requested.
     *                      Pass 0 to use Config::default_k.
     * @param context       Routing hints.  Default-constructed is safe.
     *
     * @return AnnFrontdoorResult with candidates sorted by ascending distance.
     *
     * @throws std::invalid_argument  when query_vector is nullptr or dim == 0.
     * @throws std::runtime_error     when the selected backend signals failure.
     */
    [[nodiscard]] AnnFrontdoorResult search(
        const float*          query_vector,
        std::size_t           dim,
        int                   k       = 0,
        const AnnQueryContext& context = {}) const;

    /**
     * @brief Determine the routing strategy without executing a search.
     *
     * Useful for the query planner to decide candidate set size or to log the
     * expected retrieval path before committing to a full search.
     *
     * @param context  Routing hints (dataset_size, hot_tier, etc.).
     * @return         The strategy that search() would use for this context.
     */
    [[nodiscard]] AnnStrategy planStrategy(
        const AnnQueryContext& context) const noexcept;

    /**
     * @brief Build the full retrieval plan, including hot/cold decision data.
     *
     * The plan is the canonical pre-execution explanation of how the frontdoor
     * will route the request. It is intentionally richer than planStrategy().
     * @param context Routing hints (dataset_size, hot_tier, etc.).
     * @return The complete retrieval plan with strategy and reasoning.
     */
    [[nodiscard]] AnnRetrievalPlan planRetrieval(
        const AnnQueryContext& context) const noexcept;

    /**
     * @brief Human-readable description of the routing decision.
     *
     * Returns the same string that would appear in
     * AnnFrontdoorResult::routing_reason for the given context, without
     * executing a search.
     *
     * @param context  Routing hints.
     * @return         Explanation string.
     */
    [[nodiscard]] std::string explainStrategy(
        const AnnQueryContext& context) const;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    /**
     * @brief Returns the number of registered backends (including global).
     * @return Number of backends currently registered.
     */
    [[nodiscard]] std::size_t registeredBackendCount() const noexcept;

    /**
     * @brief Returns the active configuration.
     * @return Const reference to the current configuration.
     */
    [[nodiscard]] const Config& config() const noexcept;

private:
    // Resolve the IAnnIndex for a given scope; returns nullptr when absent.
    [[nodiscard]] std::shared_ptr<IAnnIndex> resolveBackend(
        const std::string& scope_id) const noexcept;

    // Execute search against an IAnnIndex; throws on backend error.
    [[nodiscard]] std::vector<AnnSearchResult> executeSearch(
        IAnnIndex&            backend,
        const float*          query,
        std::size_t           dim,
        int                   k) const;

    // Brute-force fallback using the registered VectorIndexManager.
    [[nodiscard]] std::vector<AnnSearchResult> bruteForceSearch(
        const float*           query,
        std::size_t            dim,
        int                    k,
        const AnnQueryContext& context) const;

    // Build the routing_reason string.
    [[nodiscard]] std::string buildRoutingReason(
        AnnStrategy           strategy,
        const AnnQueryContext& context) const;

    // Estimated recall fraction for a given strategy.
    static double recallEstimate(AnnStrategy strategy) noexcept;

    Config                                                    config_;
    std::unordered_map<std::string, std::shared_ptr<IAnnIndex>> backends_;
    std::unordered_map<std::string, AnnScopeKind>               scope_kinds_;
    std::shared_ptr<VectorIndexManager>                       vim_;
    std::shared_ptr<TieredIndexManager>                       tiered_;
};

// ============================================================================
// Utility: strategy name for logging
// ============================================================================

/**
 * @brief Returns the name of an AnnStrategy as a C-string.
 * @param s  Strategy value.
 * @return   Null-terminated string, e.g. "HNSW", "DISKANN".
 */
[[nodiscard]] constexpr const char* annStrategyName(AnnStrategy s) noexcept {
    switch (s) {
        case AnnStrategy::HNSW:            return "HNSW";
        case AnnStrategy::SCANN:           return "SCANN";
        case AnnStrategy::DISKANN:         return "DISKANN";
        case AnnStrategy::DISTRIBUTED:     return "DISTRIBUTED";
        case AnnStrategy::FLAT_BRUTE_FORCE: return "FLAT_BRUTE_FORCE";
    }
    return "UNKNOWN";
}

/**
 * @brief Returns the name of an AnnScopeKind as a C-string.
 * @param k  Scope kind value.
 * @return   Null-terminated string, e.g. "Document", "Chunk", "Entity".
 */
[[nodiscard]] constexpr const char* annScopeKindName(AnnScopeKind k) noexcept {
    switch (k) {
        case AnnScopeKind::Generic:      return "Generic";
        case AnnScopeKind::Adapter:      return "Adapter";
        case AnnScopeKind::Package:      return "Package";
        case AnnScopeKind::ShardSummary: return "ShardSummary";
        case AnnScopeKind::Document:     return "Document";
        case AnnScopeKind::Chunk:        return "Chunk";
        case AnnScopeKind::Entity:       return "Entity";
    }
    return "Unknown";
}

} // namespace index
} // namespace themis
