/**
 * @file adaptive_retrieval.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <memory>
#include <string>

namespace themis::rag {

// ---------------------------------------------------------------------------
// QueryComplexity — tiered complexity classification
// ---------------------------------------------------------------------------

/**
 * @brief Classification of a query's retrieval complexity.
 */
enum class QueryComplexity {
    SIMPLE,       ///< Single-entity, one-hop question (e.g. "What is X?")
    MODERATE,     ///< Two-aspect or comparative question
    COMPLEX,      ///< Multi-step or multi-entity question
    VERY_COMPLEX  ///< Deeply interconnected, requires broad retrieval sweep
};

// ---------------------------------------------------------------------------
// AdaptiveRetrievalConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for AdaptiveRetrieval.
 */
struct AdaptiveRetrievalConfig {
    /// Baseline `top_k` used for SIMPLE queries.
    size_t base_top_k = 5;

    /// Maximum `top_k` used for VERY_COMPLEX queries.
    size_t max_top_k = 20;

    /// Similarity threshold used for SIMPLE queries (higher → stricter filter).
    double base_similarity_threshold = 0.75;

    /// Similarity threshold used for VERY_COMPLEX queries (lower → broader net).
    double min_similarity_threshold = 0.40;

    /// Multiplier applied to `top_k` for each step up in complexity.
    /// SIMPLE × 1, MODERATE × multiplier, COMPLEX × multiplier², …
    double complexity_scaling = 1.5;

    /// Query length (characters) above which an extra complexity penalty is
    /// added.  Set to 0 to disable length-based adjustment.
    size_t long_query_threshold = 200;

    /// Number of connective words ("and", "or", "but", "because", "while",
    /// "since", "although", "however") that trigger a one-level complexity
    /// bump.
    size_t connective_bump_threshold = 2;
};

// ---------------------------------------------------------------------------
// ComplexityAnalysis — output of the analysis step
// ---------------------------------------------------------------------------

/**
 * @brief Detailed analysis of query complexity.
 */
struct ComplexityAnalysis {
    /// Classified complexity tier.
    QueryComplexity complexity = QueryComplexity::SIMPLE;

    /// Raw complexity score in [0, 1].  Higher means more complex.
    double raw_score = 0.0;

    /// Number of clause connectives detected in the query.
    size_t connective_count = 0;

    /// Number of question-word tokens detected ("who", "what", "when", …).
    size_t question_word_count = 0;

    /// True when the query exceeds the long_query_threshold in characters.
    bool is_long_query = false;

    /// Human-readable explanation of how the score was derived.
    std::string explanation;
};

// ---------------------------------------------------------------------------
// AdaptiveRetrievalParams — computed retrieval parameters
// ---------------------------------------------------------------------------

/**
 * @brief Retrieval parameters computed by AdaptiveRetrieval::computeParams().
 */
struct AdaptiveRetrievalParams {
    /// Recommended number of documents to retrieve.
    size_t top_k = 5;

    /// Recommended minimum similarity for candidate inclusion.
    double similarity_threshold = 0.75;

    /// The complexity analysis that drove these parameters.
    ComplexityAnalysis analysis;
};

// ---------------------------------------------------------------------------
// IComplexityScorer — optional LLM-based scorer injection
// ---------------------------------------------------------------------------

/**
 * @brief Interface for plugging in an LLM-based complexity scorer.
 *
 * Implement this interface and pass an instance to AdaptiveRetrieval to
 * replace the heuristic scorer.  When null, the heuristic is used.
 */
struct IComplexityScorer {
    virtual ~IComplexityScorer() = default;

    /**
     * @brief Score the complexity of @p query in [0, 1].
     *
     * @param query  The user query to score.
     * @return       Complexity score in [0, 1]; higher means more complex.
     */
    [[nodiscard]] virtual double score(const std::string& query) = 0;
};

// ---------------------------------------------------------------------------
// AdaptiveRetrieval
// ---------------------------------------------------------------------------

/**
 * @brief Computes adaptive retrieval parameters based on query complexity.
 *
 * Usage:
 * @code
 *   AdaptiveRetrievalConfig cfg;
 *   cfg.base_top_k  = 5;
 *   cfg.max_top_k   = 20;
 *
 *   AdaptiveRetrieval ar(cfg);
 *   auto params = ar.computeParams("Who invented the transistor and when?");
 *   // params.top_k ≈ 10 (MODERATE query)
 *   // params.similarity_threshold ≈ 0.60
 *
 *   auto docs = vector_index.search(query_vec, params.top_k,
 *                                    params.similarity_threshold);
 * @endcode
 */
class AdaptiveRetrieval {
public:
    /**
     * @brief Construct with default configuration.
     */
    AdaptiveRetrieval() = default;

    /**
     * @brief Construct with explicit configuration.
     */
    explicit AdaptiveRetrieval(const AdaptiveRetrievalConfig& config);

    /**
     * @brief Analyse the complexity of @p query.
     *
     * When a custom scorer is registered via setScorer() it is called first;
     * the heuristic is used as a fallback when the scorer returns a score of
     * 0 (or when no scorer is set).
     *
     * @param query  User query string.
     * @return       Detailed complexity analysis.
     */
    ComplexityAnalysis analyzeComplexity(const std::string& query) const;

    /**
     * @brief Compute retrieval parameters for @p query.
     *
     * Calls analyzeComplexity() and maps the resulting complexity tier to
     * concrete top_k and similarity_threshold values.
     *
     * @param query  User query string.
     * @return       Recommended retrieval parameters plus complexity analysis.
     */
    AdaptiveRetrievalParams computeParams(const std::string& query) const;

    /**
     * @brief Convert a raw complexity score to a QueryComplexity tier.
     *
     * Thresholds (inclusive):
     *   [0.00, 0.30) → SIMPLE
     *   [0.30, 0.55) → MODERATE
     *   [0.55, 0.75) → COMPLEX
     *   [0.75, 1.00] → VERY_COMPLEX
     *
     * @param raw_score  Score in [0, 1].
     * @return           Corresponding complexity tier.
     */
    static QueryComplexity scoreToComplexity(double raw_score);

    /**
     * @brief Convert a QueryComplexity tier to a human-readable string.
     */
    static const char* complexityToString(QueryComplexity complexity);

    /**
     * @brief Register an optional LLM-based scorer.
     *
     * Passing nullptr removes the custom scorer (heuristic is used instead).
     *
     * @param scorer  Scorer implementation (lifetime managed by caller).
     */
    void setScorer(IComplexityScorer* scorer);

    /// Return the current configuration.
    const AdaptiveRetrievalConfig& getConfig() const;

    /// Replace the current configuration.
    void setConfig(const AdaptiveRetrievalConfig& config);

private:
    AdaptiveRetrievalConfig config_;
    IComplexityScorer* scorer_ = nullptr;

    /// Heuristic complexity scoring (no LLM required).
    ComplexityAnalysis heuristicAnalyze(const std::string& query) const;

    /// Map complexity tier to top_k.
    size_t complexityToTopK(QueryComplexity complexity) const;

    /// Map complexity tier to similarity threshold.
    double complexityToThreshold(QueryComplexity complexity) const;
};

// ---------------------------------------------------------------------------
// AdaptiveRetrievalFactory
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for common AdaptiveRetrieval configurations.
 */
struct AdaptiveRetrievalFactory {
    /**
     * @brief Minimal footprint: top_k in [3, 8]; tight thresholds.
     */
    static std::unique_ptr<AdaptiveRetrieval> createLightweight();

    /**
     * @brief Balanced default: top_k in [5, 15]; moderate thresholds.
     *
     * base_top_k=5, max_top_k=15, base_similarity_threshold=0.75,
     * min_similarity_threshold=0.40, complexity_scaling=1.5.
     */
    static std::unique_ptr<AdaptiveRetrieval> createBalanced();

    /**
     * @brief Wide sweep: top_k in [8, 30]; loose thresholds for recall.
     */
    static std::unique_ptr<AdaptiveRetrieval> createHighRecall();
};

} // namespace themis::rag
