/**
 * @file self_rag.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ============================================================================
// Supporting data structures
// ============================================================================

/**
 * @brief A document candidate supplied by the retrieval callback.
 */
struct SelfRAGDocument {
    std::string id;        ///< Unique passage identifier
    std::string content;   ///< Passage text
    double      score = 0.0; ///< Retrieval score (higher is more relevant)
};

/**
 * @brief Critic verdict for a single retrieved passage.
 */
enum class CriticVerdict {
    Relevant,   ///< Passage directly supports the query
    Partial,    ///< Passage partially addresses the query
    Irrelevant  ///< Passage is not useful for the query
};

/**
 * @brief A passage together with its critic rating.
 */
struct RatedDocument {
    SelfRAGDocument document;
    CriticVerdict   verdict = CriticVerdict::Irrelevant;
    double          critic_score = 0.0; ///< Continuous rating in [0, 1]
};

/**
 * @brief Per-round metrics for the refinement loop.
 */
struct RefinementRoundStats {
    size_t round          = 0;  ///< 1-based round index
    size_t retrieved      = 0;  ///< Documents retrieved this round
    size_t relevant       = 0;  ///< Passages rated [Relevant]
    size_t partial        = 0;  ///< Passages rated [Partial]
    size_t irrelevant     = 0;  ///< Passages rated [Irrelevant]
    bool   stop_early     = false; ///< True when target was met before max rounds
};

/**
 * @brief Overall result of a Self-RAG refinement pass.
 */
struct SelfRAGResult {
    bool                             retrieval_triggered = false;
    std::vector<RatedDocument>       relevant_docs;   ///< Passages graded [Relevant]
    std::vector<RatedDocument>       partial_docs;    ///< Passages graded [Partial]
    std::vector<RefinementRoundStats> round_stats;
    size_t                           total_rounds_used = 0;
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Configuration for the Self-RAG controller.
 */
struct SelfRAGConfig {
    /// Maximum refinement rounds (paper default: 3).
    size_t max_rounds = 3;

    /// Number of documents to retrieve per round.
    size_t top_k = 5;

    /// Minimum critic score to rate a passage as [Relevant].
    double relevant_threshold = 0.7;

    /// Minimum critic score to rate a passage as [Partial] (below = [Irrelevant]).
    double partial_threshold = 0.4;

    /// Stop refinement early once at least this many [Relevant] docs are assembled.
    size_t target_relevant_docs = 3;

    /// Retrieval confidence threshold: query scores below this trigger retrieval.
    /// Used by shouldRetrieve() heuristic when no retrieval model is injected.
    double retrieval_confidence_threshold = 0.6;
};

// ============================================================================
// SelfRAGController
// ============================================================================

/**
 * @brief Self-RAG controller: retrieval decision, critic, and refinement loop.
 *
 * Thread-safety: individual `SelfRAGController` instances are **not**
 * thread-safe.  Each inference thread should own its own instance, or callers
 * must synchronise externally.
 */
class SelfRAGController {
public:
    /**
     * @brief Callback type for the retrieval function injected by the caller.
     *
     * @param query  The retrieval query string.
     * @param top_k  Maximum number of passages to return.
     * @return       Vector of candidate documents (may be empty on failure).
     */
    using RetrievalCallback =
        std::function<std::vector<SelfRAGDocument>(const std::string& query,
                                                    size_t             top_k)>;

    /**
     * @brief Optional callback for scoring query-document relevance.
     *
     * When not set the controller falls back to blended retrieval score and
     * lexical query-document overlap.
     *
     * @param query    The retrieval query.
     * @param doc      Candidate passage.
     * @return         Relevance score in [0, 1].
     */
    using CriticCallback =
        std::function<double(const std::string&     query,
                             const SelfRAGDocument& doc)>;

    explicit SelfRAGController(SelfRAGConfig cfg = {});
    ~SelfRAGController();

    // ------------------------------------------------------------------
    // Callback injection
    // ------------------------------------------------------------------

    /**
     * @brief Inject the retrieval backend.
     *
     * Must be set before calling `runRefinementLoop()`.
     * @param cb Retrieval function (see RetrievalCallback).
     */
    void setRetrievalCallback(RetrievalCallback cb);

    /**
     * @brief Inject an optional critic scoring function.
     *
     * When not set the controller uses a blended retrieval-score/lexical
     * overlap heuristic as proxy critic confidence.
     * @param cb Critic function (see CriticCallback).
     */
    void setCriticCallback(CriticCallback cb);

    // ------------------------------------------------------------------
    // Core API
    // ------------------------------------------------------------------

    /**
     * @brief Decide whether retrieval is needed for the given query context.
     *
     * Returns `true` (retrieve) when the controller estimates that the query
     * requires external evidence.  The heuristic compares `query_confidence`
     * against `cfg_.retrieval_confidence_threshold`.
     *
     * @param query             The natural-language query.
     * @param query_confidence  Caller-supplied confidence in [0, 1].  When
     *                          omitted the controller defaults to a low value
     *                          (always retrieves).
     * @return true if retrieval should be triggered.
     */
    bool shouldRetrieve(const std::string& query,
                        double             query_confidence = 0.0) const;

    /**
     * @brief Grade a set of retrieved documents for the given query.
     *
     * Applies the critic callback (or retrieval-score proxy) to each document
     * and assigns a `CriticVerdict`.
     *
     * @param query     The retrieval query.
     * @param documents Candidate passages.
     * @return Vector of rated documents in the same order as @p documents.
     */
    std::vector<RatedDocument> criticDocuments(
        const std::string&              query,
        const std::vector<SelfRAGDocument>& documents) const;

    /**
     * @brief Run the full Self-RAG refinement loop for a query.
     *
     * Algorithm:
     *  1. Call `shouldRetrieve()`. If false, return early with empty result.
     *  2. Retrieve `top_k` documents via the retrieval callback.
     *  3. Critic all retrieved documents.
     *  4. If `target_relevant_docs` Relevant passages found → stop.
     *  5. Otherwise increment round counter and re-retrieve (up to max_rounds).
     *
     * @param query  Natural-language query.
     * @param query_confidence  Confidence override passed to `shouldRetrieve()`.
     * @return `SelfRAGResult` with graded passages and per-round stats.
     * @throws std::runtime_error if no retrieval callback has been set.
     */
    SelfRAGResult runRefinementLoop(const std::string& query,
                                    double             query_confidence = 0.0);

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    /// Return the active configuration.
    const SelfRAGConfig& config() const noexcept { return cfg_; }

    /// Reset internal round state (for reuse across queries).
    void reset();

private:
    SelfRAGConfig     cfg_;
    RetrievalCallback retrieval_cb_;
    CriticCallback    critic_cb_;

    // Deduplicate passages across rounds by document id.
    std::vector<std::string> seen_ids_;

    std::vector<SelfRAGDocument> deduplicate(
        std::vector<SelfRAGDocument> candidates) const;
};

} // namespace rag
} // namespace themis
