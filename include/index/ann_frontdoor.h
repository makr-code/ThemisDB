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

enum class AnnStrategy : uint8_t {
    HNSW,

    SCANN,

    DISKANN,

    DISTRIBUTED,

    FLAT_BRUTE_FORCE,
};

enum class AnnScopeKind : uint8_t {
    Generic,
    Adapter,
    Package,
    ShardSummary,
    Document,
    Chunk,
    Entity,
};

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

struct AnnQueryContext {
    std::size_t dataset_size = 0;

    bool hot_tier = true;

    bool shard_aware = false;

    double recall_target = 0.95;

    double latency_budget_ms = 10.0;

    std::string scope_id;

    std::string correlation_id;

    std::string confidence_policy_version;

    std::string confidence_threshold_key;
};

struct AnnRetrievalPlan {
    AnnStrategy strategy = AnnStrategy::FLAT_BRUTE_FORCE;

    IndexTierMeta::Tier effective_tier = IndexTierMeta::Tier::HOT;

    AnnScopeKind scope_kind = AnnScopeKind::Generic;

    bool distributed = false;

    bool hot_path = false;

    std::string reason;

    std::vector<std::string> pruned_shard_ids;
};

// ============================================================================
// AnnFrontdoorResult — candidates + routing metadata returned by search()
// ============================================================================

struct AnnFrontdoorResult {
    std::vector<AnnSearchResult> candidates;

    AnnStrategy strategy_used = AnnStrategy::FLAT_BRUTE_FORCE;

    double estimated_recall = 0.0;

    std::string routing_reason;

    std::string routing_reason_code;

    std::string correlation_id;

    std::string confidence_policy_version;

    std::string confidence_threshold_key;

    std::string fallback_mode = "none";

    std::string fallback_reason_code;

    bool is_distributed = false;

    std::size_t shards_attempted = 0;

    std::size_t shards_succeeded = 0;

    std::size_t shards_failed = 0;

    std::size_t merged_candidates_before_trim = 0;

    bool partial_results = false;

    std::string distributed_merge_policy;
};

// ============================================================================
// AnnFrontdoor
// ============================================================================

class AnnFrontdoor {
public:
    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    struct Config {
        std::size_t hnsw_max_elements = 1'000'000;

        std::size_t scann_max_elements = 50'000'000;
        // Above scann_max_elements: DiskANN (if available) or ScaNN fallback.

        int default_k = 100;

        bool diskann_available = false;

        std::size_t distributed_max_fanout = 0;

        int distributed_retry_attempts = 1;

        bool distributed_allow_partial_results = true;

        bool distributed_include_global_backend = true;

        double distributed_cost_budget = 0.0;

        double distributed_quality_floor = 0.0;

        double distributed_utility_alpha = 0.6;
        double distributed_utility_beta = 0.2;
        double distributed_utility_gamma = 0.2;
    };

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    AnnFrontdoor();

    /**
     * @brief Ann Frontdoor.
     * @param[in] config Input parameter.
     * @return Return value.
     */
    explicit AnnFrontdoor(Config config);
    ~AnnFrontdoor();

    // Non-copyable; move is allowed.
    AnnFrontdoor(const AnnFrontdoor&)            = delete;
    AnnFrontdoor& operator=(const AnnFrontdoor&) = delete;
    AnnFrontdoor(AnnFrontdoor&&)                 noexcept = default;
    AnnFrontdoor& operator=(AnnFrontdoor&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Backend registration
    // -----------------------------------------------------------------------

    void registerBackend(std::string scope_id,
                         std::shared_ptr<IAnnIndex> backend,
                         AnnScopeKind kind = AnnScopeKind::Generic);

    /**
     * @brief Register Scope Kind.
     * @param[in] scope_id Identifier of the scope.
     * @param[in] kind Input parameter.
     */
    void registerScopeKind(std::string scope_id, AnnScopeKind kind);

    [[nodiscard]] AnnScopeKind getScopeKind(const std::string& scope_id) const noexcept;

    /**
     * @brief Register Vector Index Manager.
     * @param[in] vim Input parameter.
     */
    void registerVectorIndexManager(std::shared_ptr<VectorIndexManager> vim);

    /**
     * @brief Register Tiered Index Manager.
     * @param[in] tim Input parameter.
     */
    void registerTieredIndexManager(
        std::shared_ptr<TieredIndexManager> tim);

    // -----------------------------------------------------------------------
    // Retrieval
    // -----------------------------------------------------------------------

    [[nodiscard]] AnnFrontdoorResult search(
        const float*          query_vector,
        std::size_t           dim,
        int                   k       = 0,
        const AnnQueryContext& context = {}) const;

    [[nodiscard]] AnnStrategy planStrategy(
        const AnnQueryContext& context) const noexcept;

    [[nodiscard]] AnnRetrievalPlan planRetrieval(
        const AnnQueryContext& context) const noexcept;

    [[nodiscard]] std::string explainStrategy(
        const AnnQueryContext& context) const;

    // -----------------------------------------------------------------------
    // Diagnostics
    // -----------------------------------------------------------------------

    [[nodiscard]] std::size_t registeredBackendCount() const noexcept;

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

    /**
     * @brief Estimated recall fraction for a given strategy.
     * @param[in] strategy Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
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
