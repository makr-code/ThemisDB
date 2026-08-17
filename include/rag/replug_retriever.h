/**
 * @file replug_retriever.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"

#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag {

// ============================================================
// Pluggable LLM scorer interface
// ============================================================

/**
 * @brief Interface for LLM-based document likelihood scoring.
 *
 * Implement this to integrate any language model's perplexity signal
 * into the REPLUG fusion pipeline.
 *
 * Contract:
 *  - Returns a score in [0, 1] where higher is better.
 *  - Must be exception-safe; any failure should return 0.0.
 *  - Does NOT need to be thread-safe; ReplugRetriever creates one scorer.
 */
class ILLMScorer {
public:
    virtual ~ILLMScorer() = default;

    /**
     * @brief Score how well the document supports the query from the LLM's
     *        perspective (higher = more likely / lower perplexity).
     *
     * @param query    The user's query.
     * @param document Candidate document content.
     * @return Score in [0, 1].
     */
    [[nodiscard]] virtual double score(const std::string& query,
                         const std::string& document) const = 0;

    /** @brief Human-readable name for logging. */
    [[nodiscard]] virtual std::string name() const = 0;
};

// ============================================================
// Heuristic default scorer (no LLM runtime required)
// ============================================================

/**
 * @brief Heuristic ILLMScorer that approximates perplexity via term overlap.
 *
 * Computes the Jaccard similarity between query tokens and document tokens as
 * a lightweight proxy for LLM log-likelihood.  Used when no real LLM scorer
 * is injected; provides a deterministic, reproducible baseline.
 *
 * Performance: O(|query| + |document|).
 */
class HeuristicLLMScorer : public ILLMScorer {
public:
    double score(const std::string& query,
                 const std::string& document) const override;

    std::string name() const override { return "HeuristicLLMScorer"; }
};

// ============================================================
// Configuration
// ============================================================

/**
 * @brief Configuration for ReplugRetriever.
 */
struct ReplugConfig {
    /// Interpolation weight λ ∈ [0, 1] between retrieval and LLM scores.
    /// λ=0 ⟹ pure retrieval; λ=1 ⟹ pure LLM scoring.
    double llm_weight = 0.5;

    /// Maximum documents returned after fusion (0 = return all).
    size_t top_k = 10;

    /// Temperature τ > 0 for softmax normalisation of LLM scores before
    /// interpolation.  Lower τ sharpens the LLM distribution.
    double temperature = 1.0;

    /// Enable KL-divergence-based retriever weight update step
    /// (REPLUG-LSR signal). When true, updateRetrieverWeights() is
    /// meaningful; when false it is a no-op.
    bool enable_weight_update = false;

    /// Learning rate η for the weight update step.
    double weight_update_lr = 0.01;

    /// Minimum retrieval score to include a document in LLM scoring.
    /// Documents below this threshold are dropped before LLM scoring.
    double min_retrieval_score = 0.0;
};

// ============================================================
// Per-document scoring result
// ============================================================

/**
 * @brief Per-document score breakdown from a REPLUG fusion call.
 */
struct ReplugScore {
    std::string document_id;        ///< Matches RetrievedDocument::id
    double retrieval_score  = 0.0;  ///< Original retrieval score (normalised)
    double llm_score        = 0.0;  ///< LLM scorer output (normalised)
    double fused_score      = 0.0;  ///< (1-λ)*retrieval + λ*llm
    double kl_gradient      = 0.0;  ///< KL-divergence gradient (REPLUG-LSR)
};

/**
 * @brief Result of a single ReplugRetriever::fuse() call.
 */
struct ReplugFusionResult {
    /// Documents sorted by descending fused_score.
    std::vector<judge::RetrievedDocument> documents;

    /// Per-document breakdown in the same order as @ref documents.
    std::vector<ReplugScore> scores;

    /// Name of the ILLMScorer used.
    std::string scorer_name;

    /// Number of candidates before top_k truncation.
    size_t total_candidates = 0;
};

// ============================================================
// ReplugRetriever
// ============================================================

/**
 * @brief REPLUG-style retriever that fuses retrieval and LLM scores.
 *
 * Typical usage:
 * @code
 *   ReplugConfig cfg;
 *   cfg.llm_weight  = 0.4;
 *   cfg.top_k       = 10;
 *
 *   auto scorer = std::make_shared<MyLLMScorer>();
 *   ReplugRetriever retriever(cfg, scorer);
 *
 *   auto result = retriever.fuse("What is RAG?", candidates);
 *   for (const auto& doc : result.documents) {
 *       std::cout << doc.id << "  fused=" << doc.similarity_score << "\n";
 *   }
 * @endcode
 *
 * REPLUG-LSR weight update:
 * @code
 *   retriever.updateRetrieverWeights(result);
 *   // Subsequent fuse() calls use the updated per-document weights.
 * @endcode
 */
class ReplugRetriever {
public:
    /**
     * @brief Construct with default config and heuristic scorer.
     */
    ReplugRetriever();

    /**
     * @brief Construct with custom config and optional scorer.
     * @param config         Fusion configuration.
     * @param scorer         Optional LLM scorer; if nullptr, uses
     *                       HeuristicLLMScorer.
     * @throws std::invalid_argument on invalid config parameters.
     */
    explicit ReplugRetriever(
        const ReplugConfig&         config,
        std::shared_ptr<ILLMScorer> scorer = nullptr);

    ~ReplugRetriever() = default;

    // Not copyable because the weight table is mutable state.
    ReplugRetriever(const ReplugRetriever&)            = delete;
    ReplugRetriever& operator=(const ReplugRetriever&) = delete;
    ReplugRetriever(ReplugRetriever&&)                 noexcept = default;
    ReplugRetriever& operator=(ReplugRetriever&&)      noexcept = default;

    // ═══════════════════════════════════════════════════════════
    // Core API
    // ═══════════════════════════════════════════════════════════

    /**
     * @brief Score and re-rank candidates by fusing retrieval and LLM scores.
     *
     * Steps performed:
     *  1. Drop candidates with retrieval_score < config_.min_retrieval_score.
     *  2. Normalise retrieval scores to [0, 1].
     *  3. Compute LLM scores for each candidate via the injected scorer.
     *  4. Apply softmax with config_.temperature to LLM raw scores.
     *  5. Interpolate: fused = (1-λ)*retrieval + λ*llm.
     *  6. Sort by fused score descending; truncate to config_.top_k.
     *
     * The @c similarity_score field of every returned document is set to the
     * fused score.
     *
     * @param query       User query.
     * @param candidates  Retrieved candidate documents.
     * @return Fusion result with re-ranked documents and score breakdown.
     */
    ReplugFusionResult fuse(
        const std::string&                       query,
        const std::vector<judge::RetrievedDocument>& candidates) const;

    /**
     * @brief Apply one REPLUG-LSR gradient step to the per-document weight
     *        table using the KL-divergence gradient from a previous fuse().
     *
     * For each document d:
     *  @code
     *    weight[d] += lr * kl_gradient[d]
     *    weight[d]  = clamp(weight[d], 0, 1)
     *  @endcode
     *
     * The updated weights are incorporated in subsequent fuse() calls by
     * multiplying the retrieval score by weight[d] before normalisation.
     *
     * No-op when config_.enable_weight_update is false.
     *
     * @param result Result from the most recent fuse() call.
     */
    void updateRetrieverWeights(const ReplugFusionResult& result);

    /**
     * @brief Reset all per-document retriever weights to 1.0.
     */
    void resetWeights();

    /**
     * @brief Read the current weight for a document ID (default 1.0).
     */
    double getWeight(const std::string& document_id) const;

    // ═══════════════════════════════════════════════════════════
    // Configuration
    // ═══════════════════════════════════════════════════════════

    /** @brief Return the current configuration. */
    const ReplugConfig& getConfig() const;

    /**
     * @brief Replace the configuration.
     * @throws std::invalid_argument on invalid parameters.
     */
    void setConfig(const ReplugConfig& config);

    /** @brief Replace the LLM scorer (nullptr ⟹ heuristic fallback). */
    void setScorer(std::shared_ptr<ILLMScorer> scorer);

    /** @brief Return the active scorer name. */
    std::string scorerName() const;

    /**
     * @brief Validate a ReplugConfig.
     * @throws std::invalid_argument describing the first violation found.
     */
    static void validateConfig(const ReplugConfig& config);

private:
    ReplugConfig                        config_;
    std::shared_ptr<ILLMScorer>         scorer_;
    std::unordered_map<std::string, double> weights_; ///< Per-document update weights

    /// Compute raw LLM scores for all candidates.
    std::vector<double> computeLLMScores(
        const std::string&                       query,
        const std::vector<judge::RetrievedDocument>& candidates) const;

    /// Apply softmax with temperature to a score vector in-place.
    static void applySoftmax(std::vector<double>& scores, double temperature);

    /// Normalise a score vector to [0, 1] in-place.
    static void normalise(std::vector<double>& scores);

    /// Compute KL(llm || retrieval) gradient for each document.
    static std::vector<double> computeKLGradients(
        const std::vector<double>& retrieval_probs,
        const std::vector<double>& llm_probs);
};

// ============================================================
// Factory
// ============================================================

/**
 * @brief Factory helpers for common ReplugRetriever configurations.
 */
class ReplugRetrieverFactory {
public:
    /**
     * @brief Balanced fusion: equal retrieval and LLM weight (λ=0.5).
     * @param scorer Optional LLM scorer; nullptr uses heuristic.
     * @param top_k  Result list length.
     */
    static ReplugRetriever createBalanced(
        std::shared_ptr<ILLMScorer> scorer = nullptr,
        size_t top_k = 10);

    /**
     * @brief LLM-dominant fusion: retrieval as coarse filter (λ=0.8).
     * @param scorer Optional LLM scorer; nullptr uses heuristic.
     * @param top_k  Result list length.
     */
    static ReplugRetriever createLLMDominant(
        std::shared_ptr<ILLMScorer> scorer = nullptr,
        size_t top_k = 10);

    /**
     * @brief Retrieval-dominant fusion: LLM as light re-ranker (λ=0.2).
     * @param scorer Optional LLM scorer; nullptr uses heuristic.
     * @param top_k  Result list length.
     */
    static ReplugRetriever createRetrievalDominant(
        std::shared_ptr<ILLMScorer> scorer = nullptr,
        size_t top_k = 10);

    /**
     * @brief REPLUG-LSR mode: balanced fusion + weight update enabled.
     * @param scorer Optional LLM scorer; nullptr uses heuristic.
     * @param top_k  Result list length.
     */
    static ReplugRetriever createLSR(
        std::shared_ptr<ILLMScorer> scorer = nullptr,
        size_t top_k = 10);
};

} // namespace themis::rag
