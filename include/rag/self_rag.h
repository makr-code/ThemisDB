/**
 * @file self_rag.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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
 *
 * Represents a single passage/document retrieved by the retrieval backend.
 * The controller uses the score field as one component of the critic verdict
 * calculation when no external critic callback is provided.
 */
struct SelfRAGDocument {
    std::string id;        ///< Unique passage identifier (must be non-empty; used for deduplication)
    std::string content;   ///< Passage text (may be empty; typically preprocessed/normalized)
    double      score = 0.0; ///< Retrieval score; expected range [0, 1] (higher indicates more relevant)
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
 *
 * This is the primary output of the criticDocuments() method. Each RatedDocument
 * represents a graded passage with its CriticVerdict and continuous confidence score.
 *
 * @note Verdicts are discrete (Relevant, Partial, Irrelevant), while the critic_score
 *       is a continuous value in [0, 1]. Verdicts are derived from the score by comparing
 *       against configuration thresholds (relevant_threshold, partial_threshold).
 */
struct RatedDocument {
    SelfRAGDocument document;
    CriticVerdict   verdict = CriticVerdict::Irrelevant; ///< Discrete rating: Relevant / Partial / Irrelevant
    double          critic_score = 0.0; ///< Continuous rating in [0, 1]; higher indicates more relevant
};

/**
 * @brief Per-round metrics for the refinement loop.
 *
 * Tracks statistics for each iteration of the Self-RAG refinement loop.
 * Used for diagnostics, observability, and understanding loop behavior
 * (e.g., detecting early termination or repeated retrievals).
 *
 * @note One RefinementRoundStats entry is created for each round executed.
 *       If the loop terminates early (target reached), final round_stats reflects
 *       the terminating round (with stop_early = true).
 */
struct RefinementRoundStats {
    size_t round          = 0;  ///< 1-based round index (round 1, 2, 3, ...)
    size_t retrieved      = 0;  ///< Documents retrieved this round (before deduplication)
    size_t relevant       = 0;  ///< Passages rated [Relevant] in this round
    size_t partial        = 0;  ///< Passages rated [Partial] in this round
    size_t irrelevant     = 0;  ///< Passages rated [Irrelevant] in this round
    bool   stop_early     = false; ///< True if target_relevant_docs was met and loop terminated early
};

/**
 * @brief Overall result of a Self-RAG refinement pass.
 *
 * Contains the graded passages, per-round metrics, and a summary of whether retrieval
 * was triggered and completed successfully. Callers should inspect `retrieval_triggered`
 * and the size of `relevant_docs` to determine if the refinement loop produced useful
 * evidence for the query.
 *
 * @note The returned vectors maintain relative order of documents as graded by the critic.
 * @note `round_stats` includes one entry per completed round; use it for observability
 *       and debugging (e.g., to detect early termination or repeated retrievals).
 */
struct SelfRAGResult {
    bool                             retrieval_triggered = false;  ///< True if shouldRetrieve() returned true and at least one round was attempted
    std::vector<RatedDocument>       relevant_docs;   ///< Passages graded [Relevant] across all rounds (deduplicated)
    std::vector<RatedDocument>       partial_docs;    ///< Passages graded [Partial] across all rounds (deduplicated)
    std::vector<RefinementRoundStats> round_stats;     ///< Per-round statistics; used for diagnostics and observability
    size_t                           total_rounds_used = 0; ///< Number of refinement rounds executed (1 to max_rounds)
};

// ============================================================================
// Configuration
// ============================================================================

/**
 * @brief Configuration for the Self-RAG controller.
 *
 * All thresholds and limits are expected to be in valid ranges; the controller
 * does not validate configuration at construction time. Misconfiguration (e.g.,
 * `relevant_threshold > partial_threshold`) may produce unintuitive results.
 *
 * @note Thresholds are used only by the default lexical-overlap critic heuristic;
 *       an external `CriticCallback` may interpret them differently or ignore them.
 * @note Default values are tuned for English-language queries and lexical overlap scoring.
 */
struct SelfRAGConfig {
    /// Maximum refinement rounds (paper default: 3).
    /// Semantics: defines the maximum iterations of the loop before termination.
    size_t max_rounds = 3;

    /// Number of documents to retrieve per round (must be > 0 for meaningful results).
    size_t top_k = 5;

    /// Minimum critic score to rate a passage as [Relevant] (range [0, 1]).
    /// Passages with score >= relevant_threshold are marked Relevant.
    double relevant_threshold = 0.7;

    /// Minimum critic score to rate a passage as [Partial] (range [0, 1], typically < relevant_threshold).
    /// Passages with score in [partial_threshold, relevant_threshold) are marked Partial;
    /// scores below partial_threshold are marked Irrelevant.
    double partial_threshold = 0.4;

    /// Stop refinement early once at least this many [Relevant] docs are assembled.
    /// When target_relevant_docs [Relevant] passages are found, the loop terminates immediately.
    size_t target_relevant_docs = 3;

    /// Retrieval confidence threshold: query scores below this trigger retrieval.
    /// Used by shouldRetrieve() heuristic when no retrieval model is injected (range [0, 1]).
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

    /**
     * @brief Construct a Self-RAG controller with optional configuration.
     *
     * @param cfg Configuration struct with thresholds, round limits, and target document counts.
     *            If omitted, defaults are used (see SelfRAGConfig).
     *
     * @note The controller is **not** thread-safe; each inference thread should own
     *       its own instance, or callers must serialize externally.
     * @note `setRetrievalCallback()` must be called before `runRefinementLoop()`,
     *       or runRefinementLoop() will throw std::runtime_error.
     */
    explicit SelfRAGController(SelfRAGConfig cfg = {});

    /**
     * @brief Destructor.
     *
     * Releases internal state and callbacks. After destruction, the controller
     * must not be used.
     */
    ~SelfRAGController();

    // ------------------------------------------------------------------
    // Callback injection
    // ------------------------------------------------------------------

    /**
     * @brief Inject the retrieval backend.
     *
     * Sets the callback function that will be invoked to retrieve candidate passages
     * for each round of the refinement loop.
     *
     * @param cb Retrieval function (see RetrievalCallback).
     *           The callback receives the query string and top_k count, and must return
     *           a vector of SelfRAGDocument instances. The vector may be empty if no
     *           passages match the query.
     *
     * @note This callback **must** be set before calling `runRefinementLoop()`,
     *       or runRefinementLoop() will throw std::runtime_error("Retrieval callback not set").
     * @note The callback is copied and stored internally; the caller retains ownership
     *       of the original callback function.
     * @note The callback should be idempotent and thread-safe if the same controller
     *       instance is shared across threads (though this is not recommended).
     *
     * @see runRefinementLoop()
     */
    void setRetrievalCallback(RetrievalCallback cb);

    /**
     * @brief Inject an optional critic scoring function.
     *
     * Sets the callback that rates the relevance of each retrieved document for the query.
     * When not set, the controller uses a blended heuristic combining retrieval score
     * and lexical query-document overlap to estimate relevance.
     *
     * @param cb Critic function (see CriticCallback).
     *           The callback receives the query string and a candidate document,
     *           and must return a score in [0, 1] where higher indicates more relevant.
     *
     * @note This callback is **optional**. If not set, a built-in lexical-overlap heuristic
     *       is used (65% weight on retrieval score, 35% weight on normalized query-document overlap).
     * @note If set, the critic callback takes precedence over the lexical heuristic;
     *       the configuration thresholds (relevant_threshold, partial_threshold) are still
     *       applied to the critic's scores.
     * @note The callback is copied and stored internally; the caller retains ownership
     *       of the original callback function.
     *
     * @see criticDocuments()
     */
    void setCriticCallback(CriticCallback cb);

    // ------------------------------------------------------------------
    // Core API
    // ------------------------------------------------------------------

    /**
     * @brief Decide whether retrieval is needed for the given query context.
     *
     * Returns `true` (retrieve) when the controller estimates that the query
     * requires external evidence. The heuristic compares `query_confidence`
     * against `cfg_.retrieval_confidence_threshold`.
     *
     * **Behavior:**
     *   - If query_confidence < retrieval_confidence_threshold → return true (retrieve)
     *   - Otherwise → return false (do not retrieve)
     *
     * @param query             The natural-language query (used for observation only;
     *                          not directly used in the decision logic).
     * @param query_confidence  Caller-supplied confidence in [0, 1] that the query can
     *                          be answered without external evidence.
     *                          Default 0.0 means "assume low confidence; always retrieve".
     *
     * @return true if retrieval should be triggered; false if caller believes query
     *         can be answered without retrieval.
     *
     * @note This method does not access any internal state; it is a pure heuristic
     *       based on the provided confidence score.
     * @note Edge cases:
     *       - query_confidence == retrieval_confidence_threshold → returns false (does not retrieve).
     *       - Negative query_confidence or out-of-range values: behavior is undefined; callers
     *         should ensure values are in [0, 1].
     * @note The `query` parameter is provided for future extension (e.g., LLM-based
     *       confidence estimation); it is not used in the default heuristic.
     */
    bool shouldRetrieve(const std::string& query,
                        double             query_confidence = 0.0) const;

    /**
     * @brief Grade a set of retrieved documents for the given query.
     *
     * Applies the critic callback (or retrieval-score proxy heuristic) to each document
     * and assigns a `CriticVerdict` (Relevant, Partial, or Irrelevant) based on the
     * configured thresholds.
     *
     * Algorithm:
     *   1. For each document, compute a relevance score (via critic callback or heuristic).
     *   2. Assign verdict: if score >= relevant_threshold → Relevant;
     *      else if score >= partial_threshold → Partial; else → Irrelevant.
     *   3. Return a vector of RatedDocument in the same order as input documents.
     *
     * @param query     The retrieval query (used to compute relevance scores).
     * @param documents Candidate passages to critic (may be empty; returns empty vector if so).
     *
     * @return Vector of RatedDocument entries with verdicts, in the same order as @p documents.
     *         Each entry contains the original document plus its verdict and continuous score.
     *
     * @note The output vector preserves the order of input documents; no sorting or filtering occurs.
     * @note Edge cases:
     *       - Empty input → returns empty output (no error).
     *       - Empty query string → critic still attempts to score (may result in low/zero scores).
     *       - Missing critic callback → uses lexical overlap heuristic (see setCriticCallback()).
     *
     * @see criticDocuments()
     */
    std::vector<RatedDocument> criticDocuments(
        const std::string&              query,
        const std::vector<SelfRAGDocument>& documents) const;

    /**
     * @brief Run the full Self-RAG refinement loop for a query.
     *
     * Orchestrates the Self-RAG pipeline: retrieve-critic-refine loop that iteratively
     * fetches and grades passages until sufficient [Relevant] documents are found or
     * the maximum number of rounds is exhausted.
     *
     * Algorithm:
     *  1. Call `shouldRetrieve(query, query_confidence)`. If false, return early with empty result.
     *  2. For each round (up to `max_rounds`):
     *     a. Retrieve `top_k` documents via the retrieval callback.
     *     b. Deduplicate: exclude documents already seen in prior rounds.
     *     c. Critic each document (assign Relevant / Partial / Irrelevant verdict).
     *     d. Collect statistics (retrieved count, verdict distribution).
     *     e. If collected >= `target_relevant_docs` [Relevant] docs → stop and return result.
     *  3. Return aggregated `SelfRAGResult` with all graded documents and per-round stats.
     *
     * @param query                The natural-language query.
     * @param query_confidence     Confidence in the query (range [0, 1]); passed to shouldRetrieve().
     *                             Default 0.0 means "low confidence, always retrieve".
     *
     * @return `SelfRAGResult` with:
     *         - `retrieval_triggered`: true if shouldRetrieve() returned true.
     *         - `relevant_docs`, `partial_docs`: graded passages, deduplicated across rounds.
     *         - `round_stats`: per-round metrics for diagnostics.
     *         - `total_rounds_used`: number of rounds executed (1 to max_rounds).
     *
     * @throws std::runtime_error if no retrieval callback has been set via setRetrievalCallback().
     *
     * @note Thread-safety: This controller instance is **not** thread-safe. Callers must
     *       serialize concurrent calls to runRefinementLoop() or use separate controller instances.
     * @note Deduplication state: Internal `seen_ids_` persists across calls. To reset
     *       deduplication for a new query, call reset() before calling runRefinementLoop().
     * @note Edge cases:
     *       - Retrieval callback returns empty → no documents graded, loop attempts next round
     *         (up to max_rounds).
     *       - No [Relevant] documents found across all rounds → returns partial_docs and irrelevant
     *         (if critic was injected) or empty result (if using heuristic).
     *       - query_confidence >= retrieval_confidence_threshold → skips retrieval, returns empty result
     *         (shouldRetrieve() returns false).
     *
     * @see shouldRetrieve()
     * @see criticDocuments()
     * @see reset()
     */
    SelfRAGResult runRefinementLoop(const std::string& query,
                                    double             query_confidence = 0.0);

    // ------------------------------------------------------------------
    // Accessors
    // ------------------------------------------------------------------

    /**
     * @brief Return the active configuration.
     *
     * @return Const reference to the SelfRAGConfig struct passed at construction time.
     *         The configuration is immutable after construction and remains valid
     *         for the lifetime of the controller.
     */
    const SelfRAGConfig& config() const noexcept { return cfg_; }

    /**
     * @brief Reset internal round state for reuse across queries.
     *
     * Clears the internal deduplication set (`seen_ids_`) and round counters,
     * allowing the controller to process a new query as if it were freshly constructed.
     * The configuration and callbacks remain unchanged.
     *
     * @note Call this between consecutive queries to ensure clean deduplication
     *       boundaries if you want to reuse a single controller instance.
     * @note If you do not call reset() between queries, deduplication continues
     *       across queries (documents retrieved in prior queries are not retrieved again).
     */
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
