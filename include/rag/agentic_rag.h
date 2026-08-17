/**
 * @file agentic_rag.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 96/100
 * @note Gap Summary: total=5; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=2, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/knowledge_gap_detector.h"
#include "rag/delegate_evaluator.h"

#include <optional>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <chrono>

namespace themis::rag::agentic {

// ---------------------------------------------------------------------------
// Stop conditions
// ---------------------------------------------------------------------------

/**
 * @brief Reason the iterative loop terminated.
 */
enum class StopReason {
    QUALITY_SATISFIED,     ///< Overall score met the quality threshold
    MAX_ITERATIONS,        ///< Maximum number of iterations reached
    NO_GAP_DETECTED,       ///< KnowledgeGapDetector found no gap
    NO_NEW_DOCUMENTS,      ///< Retrieval returned no additional documents
    CANCELLED,             ///< Externally cancelled via AgenticRAG::cancel()
    BUDGET_EXCEEDED        ///< Session token budget cap reached (Gap 4)
};

// ---------------------------------------------------------------------------
// Per-iteration record
// ---------------------------------------------------------------------------

/**
 * @brief Snapshot of a single iteration of the agentic loop.
 */
struct IterationRecord {
    size_t iteration;                               ///< 0-based iteration index
    std::string query_used;                         ///< Query string used this round
    std::vector<judge::RetrievedDocument> documents;///< Documents retrieved this round
    judge::EvaluationResult evaluation;             ///< Judge result for this round
    knowledge_gap::DetectionResult gap;             ///< Gap-detection result
    std::chrono::milliseconds elapsed_ms;           ///< Wall-clock time for this iteration
};

// ---------------------------------------------------------------------------
// Final result
// ---------------------------------------------------------------------------

/**
 * @brief Result of the complete agentic retrieval loop.
 */
struct AgenticRAGResult {
    /// Final consolidated set of documents (union of all iterations).
    std::vector<judge::RetrievedDocument> final_documents;

    /// Final evaluation result (from the last iteration).
    judge::EvaluationResult final_evaluation;

    /// Per-iteration history, oldest first.
    std::vector<IterationRecord> iterations;

    /// Reason the loop terminated.
    StopReason stop_reason;

    /// Total number of iterations executed.
    size_t total_iterations;

    /// Total wall-clock time across all iterations.
    std::chrono::milliseconds total_elapsed_ms;

    /// True when the quality threshold was ultimately satisfied.
    bool quality_satisfied;

    /// Optional relay result from the DELEGATE-52 safety-net check.
    ///
    /// Populated after `run()` when a `RelayGuardConfig` is set on
    /// `AgenticRAGConfig`.  Present as `std::nullopt` when no relay guard is
    /// configured or when the relay could not be executed.
    std::optional<delegate_eval::RelayResult> delegate_relay;

    /// Cumulative token count consumed across all LLM calls in this session.
    /// Best-effort: summed from InferenceResponse::tokens_generated when
    /// available; may be 0 if the backend does not report token counts.
    /// Used for budget enforcement (Gap 4 — AI_ML_IMPACT_ASSESSMENT.md §7).
    size_t tokens_consumed = 0;
};

// ---------------------------------------------------------------------------
// Retrieval callback
// ---------------------------------------------------------------------------

/**
 * @brief Caller-supplied retrieval function.
 *
 * The function receives the current query string and a list of document IDs
 * that have already been retrieved (so the caller can avoid returning
 * duplicates).  It must return a vector of newly retrieved documents; an
 * empty vector signals that no more documents are available.
 *
 * @param query         Current (possibly reformulated) query.
 * @param seen_ids      IDs of documents already in the context.
 * @return              New candidate documents (may be empty).
 */
using RetrievalFn = std::function<
    std::vector<judge::RetrievedDocument>(
        const std::string& query,
        const std::vector<std::string>& seen_ids)>;

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for AgenticRAG.
 */
struct AgenticRAGConfig {
    /// Maximum number of retrieval–evaluate–detect–reformulate cycles.
    size_t max_iterations = 5;

    /// Minimum overall evaluation score to consider the loop successful.
    double quality_threshold = 0.75;

    /// Minimum faithfulness score required (in addition to overall threshold).
    double faithfulness_threshold = 0.80;

    /// Maximum total documents to accumulate across all iterations.
    size_t max_total_documents = 50;

    /// If true, documents from earlier iterations are carried forward;
    /// if false, each iteration starts with only the newly retrieved docs.
    bool accumulate_documents = true;

    /// Query reformulation strategy applied when a gap is detected.
    /// Currently supported: "expand" (add gap aspects to query),
    /// "rephrase" (generic rephrase marker), "aspect_focus" (focus on
    /// the first missing aspect).
    std::string reformulation_strategy = "expand";

    /// RAGJudge configuration forwarded to the internal judge instance.
    judge::RAGJudgeConfig judge_config;

    /// KnowledgeGapDetector configuration forwarded to the gap detector.
    knowledge_gap::KnowledgeGapConfig gap_config;

    /// Maximum total tokens that may be consumed across all LLM calls in a
    /// single run() session.  When the cumulative token count reaches this
    /// value the loop stops with StopReason::BUDGET_EXCEEDED and the partial
    /// result is returned.
    /// 0 (default) disables enforcement — existing behaviour is preserved.
    /// (Gap 4 — AI_ML_IMPACT_ASSESSMENT.md §7; tracked in
    ///  rag/FUTURE_ENHANCEMENTS.md §"Session Token-Budget Cap")
    size_t max_session_tokens = 0;

    // -----------------------------------------------------------------------
    // DELEGATE-52 relay guard (optional pre-production safety net)
    // -----------------------------------------------------------------------

    /**
     * @brief Configuration for the optional DELEGATE-52 round-trip safety net.
     *
     * When all required fields are populated and `relay_guard` is set, the
     * `AgenticRAG::run()` method fires a `RoundTripSimulator::run()` relay
     * against a compact seed built from the final retrieved documents after
     * the main loop completes.  The relay result is stored in
     * `AgenticRAGResult::delegate_relay`.
     *
     * The relay is **best-effort**: any exception is swallowed and
     * `delegate_relay` remains `std::nullopt` on failure.
     *
     * All pointer members are **non-owning**.  The caller must ensure that
     * `simulator` and `evaluator` outlive the `AgenticRAG::run()` call.
     */
    struct RelayGuardConfig {
        /// Non-owning pointer to a pre-configured `RoundTripSimulator`.
        /// Required — if null, no relay is executed.
        delegate_eval::RoundTripSimulator* simulator = nullptr;

        /// Non-owning pointer to the domain evaluator used by the relay.
        /// Required — if null, no relay is executed.
        delegate_eval::IDomainEvaluator* evaluator = nullptr;

        /// Ordered list of forward/backward edit-pair instructions for the relay.
        /// Required — if empty, no relay is executed.
        std::vector<delegate_eval::RoundTripEditPair> edit_pairs;

        /// Callable that applies a single edit instruction to a document.
        /// Required — if null, no relay is executed.
        delegate_eval::EditFn edit_fn;
    };

    /// Optional DELEGATE-52 relay guard configuration.
    /// When present and all `RelayGuardConfig` fields are non-null / non-empty,
    /// a round-trip relay is executed after the agentic loop and the result is
    /// stored in `AgenticRAGResult::delegate_relay`.
    std::optional<RelayGuardConfig> relay_guard;
};

// ---------------------------------------------------------------------------
// AgenticRAG
// ---------------------------------------------------------------------------

/**
 * @brief Orchestrates iterative retrieval loops for agentic RAG.
 *
 * Usage:
 * @code
 * AgenticRAGConfig cfg;
 * cfg.max_iterations = 4;
 * cfg.quality_threshold = 0.80;
 *
 * AgenticRAG agent(cfg);
 *
 * AgenticRAGResult result = agent.run(
 *     "What caused the 2008 financial crisis?",
 *     initial_documents,
 *     [&](const std::string& q, const std::vector<std::string>& seen) {
 *         return myVectorStore.search(q, seen);
 *     }
 * );
 * @endcode
 */
class AgenticRAG {
public:
    /**
     * @brief Construct with default configuration.
     */
    AgenticRAG();

    /**
     * @brief Construct with custom configuration.
     * @param config Agentic loop configuration.
     */
    explicit AgenticRAG(const AgenticRAGConfig& config);

    /**
     * @brief Destructor.
     */
    ~AgenticRAG();

    // Non-copyable, movable
    AgenticRAG(const AgenticRAG&)            = delete;
    AgenticRAG& operator=(const AgenticRAG&) = delete;
    AgenticRAG(AgenticRAG&&)                 noexcept = default;
    AgenticRAG& operator=(AgenticRAG&&)      noexcept = default;

    // -----------------------------------------------------------------------
    // Primary entry point
    // -----------------------------------------------------------------------

    /**
     * @brief Run the agentic retrieval loop.
     *
     * @param initial_query   Original user query.
     * @param initial_docs    Documents from the first retrieval pass (may be empty).
     * @param retrieval_fn    Callback to fetch more documents on subsequent iterations.
     * @return                Final result including iteration history.
     */
    AgenticRAGResult run(
        const std::string& initial_query,
        std::vector<judge::RetrievedDocument> initial_docs,
        RetrievalFn retrieval_fn);

    /**
     * @brief Run the agentic loop without a retrieval callback.
     *
     * Useful when all documents are provided up-front.  The loop still
     * evaluates and detects gaps but cannot retrieve additional documents.
     *
     * @param query  User query.
     * @param docs   All available documents.
     * @return       Final result.
     */
    AgenticRAGResult run(
        const std::string& query,
        std::vector<judge::RetrievedDocument> docs);

    // -----------------------------------------------------------------------
    // Control
    // -----------------------------------------------------------------------

    /**
     * @brief Signal the running loop to stop after the current iteration.
     *        Safe to call from another thread.
     */
    void cancel();

    // -----------------------------------------------------------------------
    // Configuration
    // -----------------------------------------------------------------------

    /**
     * @brief Return a copy of the current configuration.
     */
    AgenticRAGConfig getConfig() const;

    /**
     * @brief Replace the current configuration.
     * @param config New configuration.
     */
    void setConfig(const AgenticRAGConfig& config);

    // -----------------------------------------------------------------------
    // Query reformulation (exposed for testing)
    // -----------------------------------------------------------------------

    /**
     * @brief Reformulate a query given a gap detection result.
     *
     * Applies the strategy from AgenticRAGConfig::reformulation_strategy:
     *   - "expand"       – append gap aspects to the original query
     *   - "aspect_focus" – focus on the first missing aspect
     *   - "rephrase"     – prepend a rephrase marker (for LLM delegation)
     *
     * @param original_query  The query used in the previous iteration.
     * @param gap             Gap detection result from the previous iteration.
     * @return                Reformulated query string.
     */
    std::string reformulateQuery(
        const std::string& original_query,
        const knowledge_gap::DetectionResult& gap) const;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Convenience factory for common AgenticRAG configurations.
 */
class AgenticRAGFactory {
public:
    /**
     * @brief Aggressive agent: up to 8 iterations, 0.85 quality threshold.
     */
    static std::unique_ptr<AgenticRAG> createAggressive();

    /**
     * @brief Balanced agent: up to 5 iterations, 0.75 quality threshold (default).
     */
    static std::unique_ptr<AgenticRAG> createBalanced();

    /**
     * @brief Conservative agent: up to 3 iterations, 0.65 quality threshold.
     */
    static std::unique_ptr<AgenticRAG> createConservative();
};

} // namespace themis::rag::agentic
