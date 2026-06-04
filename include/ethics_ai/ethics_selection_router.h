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

/*
 * ThemisDB | File: ethics_selection_router.h | Version: 0.0.1 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 206
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): none
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
};

/**
 * @brief Per-school candidate with individual stage scores.
 */
struct RouterCandidate {
    std::string school_id;
    double taxonomy_score{0.0};  ///< 1.0 = direct class match, 0.5 = domain mapping
    double semantic_score{0.0};  ///< Term-overlap similarity in [0, 1]
    double precedent_dc{0.0};    ///< Historical DC score from precedent store
    double final_score{0.0};     ///< Weighted aggregate
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
    EthicsSelectionRouter(EthicsSelectionRouter&&)                 = default;
    EthicsSelectionRouter& operator=(EthicsSelectionRouter&&)      = default;

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
