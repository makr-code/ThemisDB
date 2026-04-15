/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_hop_reasoner.h                               ║
  Version:         0.0.9                                              ║
  Last Modified:   2026-04-15 18:04:49                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     349                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 5f8c6f5fe6  2026-04-12  feat(rag): implement MultiHopReasoner and AdaptiveRetriev... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file multi_hop_reasoner.h
 * @brief Multi-hop reasoning with query decomposition (RAG Phase 7)
 *
 * Enables complex, multi-step question answering that requires decomposing
 * an original query into ordered sub-questions, retrieving documents for
 * each sub-question in turn, and composing a final answer from all
 * intermediate results.
 *
 * Pipeline:
 * @code
 *   User Query
 *       ↓
 *   QueryDecomposer  (heuristic + LLM-based)
 *       ↓
 *   Hop 1: sub_query_1 → retrieve → answer_1
 *       ↓
 *   Hop 2: sub_query_2 + context(answer_1) → retrieve → answer_2
 *       ↓
 *   ...
 *       ↓
 *   AnswerComposer   (partial answers → final answer)
 * @endcode
 *
 * Design goals:
 *   - Bring-your-own retrieval and inference via callback functions.
 *   - Configurable max hops to prevent runaway decomposition.
 *   - Graceful degradation: when decomposition yields a single hop the
 *     result is semantically equivalent to a normal RAG call.
 *   - Error propagation is localised per-hop; a failed hop does not abort
 *     the remaining hops.
 *   - Thread-compatible: instances are NOT shared across threads; create
 *     one MultiHopReasoner per concurrent request.
 */

#pragma once

#include "rag/rag_judge.h"

#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag::multi_hop {

// ---------------------------------------------------------------------------
// Callback types
// ---------------------------------------------------------------------------

/**
 * @brief Retrieve documents for a sub-query.
 *
 * @param sub_query  The decomposed sub-question.
 * @param top_k      Maximum number of documents to return.
 * @return           Retrieved documents (may be empty on failure).
 */
using RetrievalFn = std::function<std::vector<judge::RetrievedDocument>(
    const std::string& sub_query, size_t top_k)>;

/**
 * @brief Generate an answer given a prompt.
 *
 * @param prompt     Fully assembled prompt including context and query.
 * @param max_tokens Maximum tokens to generate.
 * @return           Generated text (empty string on failure).
 */
using InferenceFn = std::function<std::string(const std::string& prompt,
                                               int max_tokens)>;

// ---------------------------------------------------------------------------
// QueryComplexityHint — optional hint for hop count selection
// ---------------------------------------------------------------------------

/**
 * @brief Hint about the estimated complexity of the original query.
 */
enum class QueryComplexityHint {
    SIMPLE,       ///< Single-fact question; usually 1 hop
    MODERATE,     ///< Two-step question; typically 2 hops
    COMPLEX,      ///< Multi-step; 3-5 hops
    UNKNOWN       ///< No hint; let the decomposer decide
};

// ---------------------------------------------------------------------------
// MultiHopConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for MultiHopReasoner.
 */
struct MultiHopConfig {
    /// Hard upper bound on the number of reasoning hops.
    size_t max_hops = 5;

    /// Number of documents to retrieve per hop.
    size_t top_k_per_hop = 5;

    /// Maximum tokens for each intermediate answer.
    int max_tokens_per_hop = 256;

    /// Maximum tokens for the final composed answer.
    int max_tokens_final = 512;

    /// Stop decomposing when all sub-queries are answered.
    bool early_stopping = true;

    /// Prefix added to each sub-question prompt to provide previous context.
    std::string context_prefix = "Given the following context:\n";

    /// Separator between previous partial answers when injecting context.
    std::string answer_separator = "\n\n";

    /// Prompt template for query decomposition.
    /// Use {query} as placeholder for the original query.
    std::string decomposition_prompt_template =
        "Break the following question into simpler sub-questions that can be "
        "answered independently. Output each sub-question on a separate line. "
        "If the question is already simple, output it unchanged.\n\nQuestion: "
        "{query}";

    /// Prompt template for final answer composition.
    /// Use {query} and {partial_answers} as placeholders.
    std::string composition_prompt_template =
        "Given the following partial answers:\n{partial_answers}\n\n"
        "Compose a single, concise final answer to the original question: "
        "{query}";

    /// Hint provided by the caller about query complexity.
    QueryComplexityHint complexity_hint = QueryComplexityHint::UNKNOWN;
};

// ---------------------------------------------------------------------------
// HopRecord — snapshot of a single reasoning hop
// ---------------------------------------------------------------------------

/**
 * @brief Record of one hop in the multi-hop reasoning chain.
 */
struct HopRecord {
    /// 0-based hop index.
    size_t hop_index = 0;

    /// Sub-query used for retrieval at this hop.
    std::string sub_query;

    /// Documents retrieved for this hop.
    std::vector<judge::RetrievedDocument> documents;

    /// Intermediate answer generated at this hop (empty on failure).
    std::string intermediate_answer;

    /// Whether this hop completed successfully (retrieval + inference).
    bool succeeded = false;

    /// Wall-clock time for this hop.
    std::chrono::milliseconds elapsed_ms{0};
};

// ---------------------------------------------------------------------------
// MultiHopResult — output of the full reasoning chain
// ---------------------------------------------------------------------------

/**
 * @brief Result returned by MultiHopReasoner::reason().
 */
struct MultiHopResult {
    /// Final composed answer.  Empty when all hops failed.
    std::string final_answer;

    /// Ordered record of every hop that was executed.
    std::vector<HopRecord> hop_records;

    /// Union of all documents retrieved across all hops.
    std::vector<judge::RetrievedDocument> all_documents;

    /// Number of hops actually executed.
    size_t hops_executed = 0;

    /// True when early stopping triggered (quality satisfied).
    bool early_stopped = false;

    /// True when the hop limit was reached before the question was answered.
    bool hit_hop_limit = false;

    /// Total wall-clock time across all hops.
    std::chrono::milliseconds total_elapsed_ms{0};
};

// ---------------------------------------------------------------------------
// MultiHopReasoner
// ---------------------------------------------------------------------------

/**
 * @brief Orchestrates multi-hop retrieval and reasoning for complex queries.
 *
 * Usage:
 * @code
 *   MultiHopConfig cfg;
 *   cfg.max_hops = 3;
 *
 *   MultiHopReasoner reasoner(cfg);
 *   auto result = reasoner.reason(
 *       "Who won the Nobel Prize in Physics the year the first iPhone was released?",
 *       retrieval_fn,
 *       inference_fn);
 *
 *   for (const auto& hop : result.hop_records) {
 *       std::cout << "Hop " << hop.hop_index << ": " << hop.sub_query << "\n";
 *       std::cout << "  Answer: " << hop.intermediate_answer << "\n";
 *   }
 *   std::cout << "Final: " << result.final_answer << "\n";
 * @endcode
 */
class MultiHopReasoner {
public:
    /**
     * @brief Construct with default configuration.
     */
    MultiHopReasoner() = default;

    /**
     * @brief Construct with explicit configuration.
     */
    explicit MultiHopReasoner(const MultiHopConfig& config);

    /**
     * @brief Run multi-hop reasoning for a complex query.
     *
     * Decomposes the query into sub-questions, runs one retrieval + inference
     * hop per sub-question (injecting previous partial answers as context),
     * then composes a final answer from all intermediate results.
     *
     * When either @p retrieval_fn or @p inference_fn is null the method
     * returns an empty MultiHopResult immediately.
     *
     * @param query        Original user query.
     * @param retrieval_fn Callable that retrieves documents for a sub-query.
     * @param inference_fn Callable that generates text from a prompt.
     * @return             Full multi-hop reasoning result.
     */
    MultiHopResult reason(const std::string& query,
                          RetrievalFn retrieval_fn,
                          InferenceFn inference_fn) const;

    /**
     * @brief Decompose a query into ordered sub-questions.
     *
     * When an inference function is available it is called with the
     * decomposition prompt template; the response is split on newlines.
     * Falls back to heuristic splitting (sentence boundaries and conjunctions)
     * when no inference function is provided.
     *
     * @param query        Original query.
     * @param inference_fn Optional LLM for decomposition (may be null).
     * @return             Ordered list of sub-questions.
     */
    std::vector<std::string> decomposeQuery(
        const std::string& query,
        InferenceFn inference_fn = nullptr) const;

    /**
     * @brief Build a prompt for one hop.
     *
     * Injects the sub-question plus any previous partial answers into the
     * retrieved context so the model can reason across hops.
     *
     * @param sub_query        Sub-question for this hop.
     * @param documents        Documents retrieved for this hop.
     * @param previous_answers Partial answers from earlier hops.
     * @return                 Assembled prompt string.
     */
    std::string buildHopPrompt(
        const std::string& sub_query,
        const std::vector<judge::RetrievedDocument>& documents,
        const std::vector<std::string>& previous_answers) const;

    /// Return the current configuration.
    const MultiHopConfig& getConfig() const;

    /// Replace the current configuration.
    void setConfig(const MultiHopConfig& config);

private:
    MultiHopConfig config_;

    /// Heuristic sub-query decomposition (fallback when no LLM is available).
    std::vector<std::string> heuristicDecompose(
        const std::string& query) const;

    /// Parse LLM decomposition response into sub-question list.
    std::vector<std::string> parseDecompositionResponse(
        const std::string& response) const;

    /// Compose the final answer from per-hop partial answers.
    std::string composeAnswer(
        const std::string& original_query,
        const std::vector<HopRecord>& hop_records,
        InferenceFn inference_fn) const;
};

// ---------------------------------------------------------------------------
// MultiHopReasonerFactory
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for common MultiHopReasoner configurations.
 */
struct MultiHopReasonerFactory {
    /**
     * @brief Single-hop configuration (equivalent to simple RAG; 1 hop max).
     */
    static std::unique_ptr<MultiHopReasoner> createSingleHop();

    /**
     * @brief Balanced configuration (3 hops, 5 docs/hop).
     */
    static std::unique_ptr<MultiHopReasoner> createBalanced();

    /**
     * @brief Deep-reasoning configuration (5 hops, 8 docs/hop).
     */
    static std::unique_ptr<MultiHopReasoner> createDeepReasoning();
};

} // namespace themis::rag::multi_hop
