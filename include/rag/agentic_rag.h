/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            agentic_rag.h                                      ║
  Version:         0.0.14                                             ║
  Last Modified:   2026-04-15 18:04:46                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     331                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file agentic_rag.h
 * @brief Agentic RAG with iterative retrieval loops (RAG Phase 4)
 *
 * Implements an agentic retrieval-augmented generation loop that:
 *   1. Runs an initial retrieval pass.
 *   2. Evaluates the retrieved context using the existing RAGJudge.
 *   3. Detects knowledge gaps with the KnowledgeGapDetector.
 *   4. When gaps are found, reformulates the query and invokes the
 *      caller-supplied retrieval function again.
 *   5. Repeats until the quality threshold is met, the maximum number
 *      of iterations is reached, or no gap remains.
 *
 * Design goals:
 *   - Bring-your-own retrieval: callers supply a RetrievalFn lambda so
 *     the engine is decoupled from any specific vector store.
 *   - No external dependencies beyond the standard library and the
 *     existing themis::rag components.
 *   - Thread-compatible: instances are not shared across threads;
 *     use one AgenticRAG per concurrent request.
 */

#pragma once

#include "rag/rag_judge.h"
#include "rag/knowledge_gap_detector.h"

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
    CANCELLED              ///< Externally cancelled via AgenticRAG::cancel()
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
    AgenticRAG(AgenticRAG&&)                 = default;
    AgenticRAG& operator=(AgenticRAG&&)      = default;

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
