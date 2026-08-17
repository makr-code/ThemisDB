/**
 * @file ethics_selection_router.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "ethics_ai/ethics_profile_registry.h"
#include <functional>
#include <string>
#include <vector>
#include <memory>
#include <map>

namespace themis {
namespace plugins {
namespace ethics {

// ============================================================================
// LDM-1 — Layered Discourse Model mode selector
// ============================================================================

/**
 * @brief Operating mode for the Layered Discourse Model (LDM).
 *
 * Controls whether the router runs as a classical Top-N school selector or
 * initiates the multi-layer discourse pipeline.  The mode is set per call via
 * @c RouterConfig::discourse_mode and is passed through to the
 * @c DiscourseOrchestratorPlan produced by @c EthicsSelectionRouter::planDiscourse().
 *
 * ## Mode semantics
 * | Mode          | Ebene-1  | Ebene-2  | Ebene-3  | RouterConfig::top_n |
 * |---------------|----------|----------|----------|---------------------|
 * | SELECTION_ONLY| ✗        | ✗        | ✗        | honoured            |
 * | LAYERED_FAST  | parallel | skipped  | ✗        | ignored             |
 * | LAYERED_FULL  | parallel | clusters | MetaVerdict| ignored           |
 *
 * ### Migration note (Breaking Change)
 * Callers that rely on `top_n`-bounded output **must** explicitly set
 * `discourse_mode = DiscourseMode::SELECTION_ONLY` (which is the default) to
 * preserve the current behaviour.  `LAYERED_FAST` and `LAYERED_FULL` return all
 * non-ABSTAIN schools from Ebene-1, ignoring `top_n`.
 *
 * @since LDM-1 (Target: Q1 2027)
 */
enum class DiscourseMode : uint8_t {
    /// Classical selection path: Top-N school ranking, no discourse phases.
    /// This is the default and preserves all pre-LDM behaviour.
    SELECTION_ONLY = 0,

    /// Fast layered path: runs Ebene-1 (parallel equal-weight initial scoring)
    /// only.  Ebene-2 cluster discourse and Ebene-3 MetaVerdict are skipped.
    /// P95 target: ≤ 1.2 s end-to-end.
    LAYERED_FAST = 1,

    /// Full layered path: runs Ebene-1 → Ebene-2 (cluster discourse) →
    /// Ebene-3 (convergence-counting MetaVerdict).
    /// P95 target: ≤ 8 s end-to-end.
    LAYERED_FULL = 2,
};

/**
 * @brief Output of @c EthicsSelectionRouter::planDiscourse().
 *
 * Describes the per-school Ebene-1 assignments and the cluster mapping that
 * downstream Ebene-2 orchestration will consume.  Produced in < 5 ms.
 *
 * @since LDM-1 (Target: Q1 2027)
 */
struct DiscourseOrchestratorPlan {
    /// Active discourse mode that was used to produce this plan.
    DiscourseMode mode{DiscourseMode::SELECTION_ONLY};

    /// All school ids that will participate in Ebene-1.
    std::vector<std::string> ebene1_school_ids;

    /// Initial equal weight w₀ = 1/N (Ebene-1 contract).
    double initial_weight{1.0};

    /// Cluster assignments: cluster_name → list of school_ids.
    /// Populated only when mode == LAYERED_FULL.
    std::map<std::string, std::vector<std::string>> cluster_map;

    /// Structural tension axes activated for this plan (LAYERED_FULL only).
    /// These are the canonical 4 axes from the LDM design spec:
    ///   "Kant↔Utilitarismus", "Würde-cluster↔Aggregation-cluster",
    ///   "Individualismus↔Kollektivismus", "Positivrecht↔Naturrecht"
    /// @since LDM-1
    std::vector<std::string> tension_axes;

    /// True when the plan is empty because zero schools survived filtering.
    bool empty() const noexcept { return ebene1_school_ids.empty(); }
};

/**
 * @brief Configuration for EthicsSelectionRouter.
 */
struct RouterConfig {
    /// Path to `ethics_taxonomy.yaml`; loaded once at construction.
    std::string taxonomy_yaml_path;

    /// Maximum number of schools returned by the router (Top-N).
    size_t top_n{5};

    /// Maximum candidates considered in Stage-2 semantic filter.
    size_t stage2_top_k{10};

    /// Score weights for aggregated final ranking.
    /// Must sum to 1.0 (enforced by constructor normalisation).
    double weight_semantic{0.40};
    double weight_precedent{0.40};
    double weight_taxonomy{0.20};

    /// Deployment context identifier for this router instance.
    ///
    /// Identifies the operational environment and is propagated to
    /// context-aware components (e.g. audit logs, monitoring).  When empty,
    /// callers should default to the global ThemisDB deployment context value
    /// from the core configuration (`ai.deployment_context`, typically
    /// @c "themisdb").
    ///
    /// Well-known values: `"themisdb"`, `"standalone"`, `"embedded"`,
    /// `"enterprise"`.  Custom values are allowed and passed through as-is.
    std::string deployment_context;

    // =========================================================================
    // LDM-1 additions
    // =========================================================================

    /// Discourse operating mode.  Defaults to SELECTION_ONLY so all existing
    /// callers continue to work without modification.
    DiscourseMode discourse_mode{DiscourseMode::SELECTION_ONLY};

    /// Per-school bias multipliers applied to the aggregated @c final_score.
    ///
    /// The multiplier for a given @c school_id is applied **after** the
    /// three-stage weighted aggregation:
    /// @code
    ///   final_score = bias[school_id] * (ws * semantic + wp * precedent + wt * taxonomy)
    /// @endcode
    ///
    /// Semantic:
    /// - @c value > 1.0 — favour this school (selected more often in Top-N)
    /// - @c value = 1.0 — neutral; purely merit-based (default for unlisted schools)
    /// - @c value < 1.0 — penalise this school (selected less often)
    /// - @c value = 0.0 — effectively exclude the school from all results
    ///
    /// Values are clamped to [0, ∞) at runtime; negative values are treated as 0.
    ///
    /// Load from `config/ethics_ai/router_config.yaml` (`school_bias` section)
    /// or supply programmatically.  Schools not listed default to 1.0.
    std::map<std::string, double> school_bias;

    /// Output directory for mandatory ChainVisualizer artifacts (DOT + Mermaid)
    /// emitted by decision/discourse runs. Empty disables file emission.
    std::string chain_visualizer_output_path;
};

/**
 * @brief Per-school candidate with individual stage scores.
 */
struct RouterCandidate {
    std::string school_id;
    double taxonomy_score{0.0};  ///< 1.0 = direct class match, 0.5 = domain mapping
    double semantic_score{0.0};  ///< Term-overlap similarity in [0, 1]
    double precedent_dc{0.0};    ///< Historical DC score from precedent store
    /// Weighted aggregate after applying per-school bias from RouterConfig::school_bias.
    /// Always clamped to [0, 1]. Formula:
    ///   final_score = clamp(bias * (ws*semantic + wp*precedent + wt*taxonomy), 0, 1)
    double final_score{0.0};
};

/**
 * @brief Result from EthicsSelectionRouter::route().
 */
struct RouterResult {
    /// Top-N candidates sorted by `final_score` (descending).
    std::vector<RouterCandidate> selected;
    /// How many candidates reached each stage.
    size_t stage1_count{0};
    size_t stage2_count{0};
    size_t stage3_count{0};
};

/**
 * @brief Three-stage ethics school selection router.
 *
 * Implements the scalable funnel described in FUTURE_ENHANCEMENTS §11:
 *
 * | Stage | Method           | Latency target |
 * |-------|------------------|----------------|
 * | 1     | Tag / taxonomy   | ≤ 2 ms         |
 * | 2     | Semantic overlap | ≤ 20 ms        |
 * | 3     | Precedent lookup | ≤ 50 ms        |
 *
 * The router operates **read-only** against the `IEthicsProfileRegistry`
 * and does not modify any profile state.
 *
 * ## Thread-safety
 * `route()` is const-safe when called concurrently from multiple threads.
 * `recordDecisionOutcome()` and `setEmbeddingFn()`/`setPrecedentQueryFn()`
 * are protected by or serialised via internal mutexes.
 */
class EthicsSelectionRouter {
public:
    /**
     * @brief Signature for a Stage-2 text embedding function.
     *
     * Called once per text fragment (query or profile).  Returns a dense
     * float vector; the router computes cosine similarity between query
     * and profile embeddings.  Must be thread-safe.
     *
     * @param text  UTF-8 input text.
     * @return      Dense embedding; an empty vector disables the real-embedding
     *              path and reverts to the term-overlap fallback.
     */
    using EmbeddingFn = std::function<std::vector<float>(const std::string& text)>;

    /**
     * @brief Signature for a Stage-3 precedent query function.
     *
     * Retrieves the historical DC score for a (dilemma_domain, school_id)
     * pair from an external persistent store (e.g. ArangoDB graph).
     * Must be thread-safe.
     *
     * @param dilemma_domain  Normalised dilemma domain key.
     * @param school_id       Ethics school identifier.
     * @return                Historical DC score in [0, 1]; return 0.5 if
     *                        no precedent is available.
     */
    using PrecedentQueryFn = std::function<double(const std::string& dilemma_domain,
                                                   const std::string& school_id)>;

    /**
     * @brief Construct and load taxonomy YAML.
     *
     * If `config.taxonomy_yaml_path` is empty or the file cannot be read,
     * Stage-1 falls back to a "return all known schools" behaviour.
     *
     * @param registry Non-owning pointer; must outlive this router.
     * @param config   Router tuning parameters.
     */
    EthicsSelectionRouter(IEthicsProfileRegistry* registry,
                          const RouterConfig& config = {});

    ~EthicsSelectionRouter();

    // Non-copyable, movable
    EthicsSelectionRouter(const EthicsSelectionRouter&)            = delete;
    EthicsSelectionRouter& operator=(const EthicsSelectionRouter&) = delete;
    EthicsSelectionRouter(EthicsSelectionRouter&&)                 noexcept = default;
    EthicsSelectionRouter& operator=(EthicsSelectionRouter&&)      noexcept = default;

    /**
     * @brief Build a DiscourseOrchestratorPlan from the currently loaded school set.
     *
     * Returns a lightweight plan describing which schools participate in
     * Ebene-1 and how they are clustered for Ebene-2.  The call completes in
     * < 5 ms regardless of the number of loaded schools.
     *
     * When @c config.discourse_mode == SELECTION_ONLY the returned plan has
     * mode == SELECTION_ONLY and an empty cluster_map; callers can short-circuit
     * to the classical route() path.
     *
     * @param domain_context  Optional domain hint used for affinity-based
     *                        cluster pre-sorting (e.g. "medical", "ai_governance").
     * @return DiscourseOrchestratorPlan ready for downstream Ebene-1 execution.
     *
     * @note LDM-1 delivery: enum + plan generation.  Ebene-1/2/3 execution
     *       (LDM-2 through LDM-4) is implemented in subsequent milestones.
     *
     * @since LDM-1 (Target: Q1 2027)
     */
    DiscourseOrchestratorPlan planDiscourse(
        const std::string& domain_context = {}) const;

    /**
     * @brief Select the best-matching ethics schools for a dilemma.
     *
     * @param dilemma_text       Full dilemma description (used for Stage-2).
     * @param dilemma_domain     Primary domain, e.g. "medical", "ai_governance".
     * @param dilemma_tags       Topic tags for Stage-1 filtering.
     * @param regulatory_context When true, compliance schools are always included.
     * @return                   RouterResult with top-N ranked candidates.
     */
    RouterResult route(
        const std::string& dilemma_text,
        const std::string& dilemma_domain,
        const std::vector<std::string>& dilemma_tags = {},
        bool regulatory_context = false) const;

    /**
     * @brief Feed a completed discourse outcome back into the precedent store.
     *
     * Enables the router's Stage-3 to rank schools that historically
     * performed well on structurally similar dilemmas higher.
     *
     * @param dilemma_type  Normalised dilemma type key (e.g. "trolley_001").
     * @param school_id     Ethics school that produced the argument.
     * @param dc_score      Dialectical coherence score in [0, 1].
     */
    void recordDecisionOutcome(const std::string& dilemma_type,
                               const std::string& school_id,
                               double dc_score);

    /** Access current configuration. */
    const RouterConfig& config() const;

    /**
     * @brief Inject a real embedding function for Stage-2 semantic scoring.
     *
     * When set, `route()` calls @p fn to embed the dilemma text and each
     * candidate profile snippet, then ranks by cosine similarity.
     * When @p fn is empty (or returns an empty vector), the term-overlap
     * fallback is used transparently.
     *
     * Thread safety: safe to call before first `route()` call.  Replacing
     * the function while `route()` is running on another thread is undefined.
     *
     * @param fn  Embedding function; pass an empty `std::function` to revert
     *            to term-overlap mode.
     */
    void setEmbeddingFn(EmbeddingFn fn);

    /**
     * @brief Inject a persistent precedent query function for Stage-3.
     *
     * When set, Stage-3 calls @p fn(dilemma_domain, school_id) instead of
     * consulting the in-memory precedent map.  The in-memory map is still
     * updated by `recordDecisionOutcome()` but is only consulted as a
     * fallback when @p fn is not set.
     *
     * Thread safety: safe to call before first `route()` call.  Replacing
     * the function while `route()` is running on another thread is undefined.
     *
     * @param fn  Precedent query function; pass an empty `std::function` to
     *            revert to in-memory precedent mode.
     */
    void setPrecedentQueryFn(PrecedentQueryFn fn);

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace ethics
} // namespace plugins
} // namespace themis
