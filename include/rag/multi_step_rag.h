/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            multi_step_rag.h                                   ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-04-13 20:25:26                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     292                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 01a86c4f10  2026-04-07  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file multi_step_rag.h
 * @brief Multi-step RAG orchestration for context windows that are too small
 *        to hold all relevant documents in a single inference call.
 *
 * Two complementary strategies are provided:
 *
 * ### Strategy A — Map-Reduce RAG
 *
 *   When the total token count of the retrieved documents exceeds the model's
 *   context window, the documents are split into batches that each fit within
 *   the window.  The LLM is called once per batch (the "map" step) to
 *   summarise or answer the query for that batch.  The partial answers are
 *   then combined in a single "reduce" call that synthesises the final answer.
 *
 *   @code
 *   Batch 1 [docs 0..k]  → partial_answer_1
 *   Batch 2 [docs k+1..m] → partial_answer_2
 *   ...
 *   Reduce(partial_answer_1, partial_answer_2, …) → final_answer
 *   @endcode
 *
 * ### Strategy B — Iterative RAG with context accumulation
 *
 *   Starts with an initial answer, then iteratively identifies uncovered
 *   aspects of the query, retrieves new documents for those aspects, and
 *   refines the answer.  This is the context-budget-aware companion to the
 *   existing AgenticRAG.
 *
 *   @code
 *   while (open_aspects AND iterations < max_iterations):
 *       new_docs = retrieve(open_aspects)
 *       answer   = generate(query, accumulated_context + new_docs)
 *       open_aspects = identify_uncovered(query, answer)
 *   @endcode
 *
 * Both strategies rely on the RAGContextAssembler to ensure that each
 * individual inference call never overflows the model's context window.
 */

#pragma once

#include "rag/rag_context_assembler.h"
#include "prompt_engineering/rag_prompt_builder.h"

#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace themis::rag {

// ---------------------------------------------------------------------------
// InferenceFn — bring-your-own LLM inference callback
// ---------------------------------------------------------------------------

/**
 * @brief Callable that performs one LLM inference call.
 *
 * @param prompt     The fully assembled prompt string.
 * @param max_tokens Maximum number of tokens to generate.
 * @return           Generated text (may be empty on failure).
 */
using InferenceFn = std::function<std::string(const std::string& prompt,
                                              int                max_tokens)>;

/**
 * @brief Callable that retrieves additional documents for a refined query.
 *
 * Used by the iterative strategy to look up new documents when the current
 * context does not fully cover all aspects of the original query.
 *
 * @param query  Refined / aspect-specific query.
 * @param top_k  Maximum number of documents to return.
 * @return       Retrieved chunks (may be empty).
 */
using RetrievalFn = std::function<std::vector<RetrievedChunk>(
    const std::string& query, size_t top_k)>;

// ---------------------------------------------------------------------------
// MultiStepRAGConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for MultiStepRAGOrchestrator.
 */
struct MultiStepRAGConfig {
    // ── Shared settings ──────────────────────────────────────────────────────

    /// Assembler configuration (model context size, response budget, …).
    RAGContextAssemblerConfig assembler;

    /// System / instruction prompt prepended to every inference call.
    std::string system_prompt;

    /// Hard upper bound on answer tokens per inference call.
    int max_response_tokens = 512;

    // ── Map-Reduce settings ──────────────────────────────────────────────────

    /// Maximum number of map steps (document batches).
    size_t max_map_steps = 8u;

    /// Prompt template for the map step.
    /// Placeholders: {context} = assembled docs, {query} = original query.
    std::string map_prompt_template =
        "Based on the following context, answer this question as accurately as "
        "possible.  If the context does not contain enough information say so.\n\n"
        "Context:\n{context}\n\nQuestion: {query}\nAnswer:";

    /// Prompt template for the reduce step.
    /// Placeholders: {partial_answers} = newline-separated partial answers,
    ///               {query} = original query.
    std::string reduce_prompt_template =
        "You are synthesising multiple partial answers into one comprehensive "
        "response.\n\nOriginal question: {query}\n\nPartial answers:\n"
        "{partial_answers}\n\nFinal comprehensive answer:";

    // ── Iterative settings ───────────────────────────────────────────────────

    /// Maximum refinement iterations (iterative strategy).
    size_t max_iterations = 3u;

    /// Number of additional documents to retrieve per iteration.
    size_t retrieval_top_k = 5u;

    /// Prompt that asks the model which aspects of the query remain unanswered.
    /// Placeholders: {query} = original query, {answer} = current answer.
    std::string gap_detection_prompt =
        "Given the question below and the current answer, list the specific "
        "aspects of the question that are NOT yet adequately addressed.  "
        "Return one aspect per line, or 'NONE' if the answer is complete.\n\n"
        "Question: {query}\n\nCurrent answer: {answer}\n\nUncovered aspects:";
};

// ---------------------------------------------------------------------------
// MultiStepRAGResult
// ---------------------------------------------------------------------------

/**
 * @brief Result of a multi-step RAG orchestration run.
 */
struct MultiStepRAGResult {
    std::string final_answer;          ///< Synthesised final answer
    std::vector<std::string> steps;    ///< Intermediate answers / summaries
    size_t steps_executed   = 0u;      ///< Number of LLM calls made
    bool   context_overflow = false;   ///< True if docs did not fit in one call
    bool   was_truncated    = false;   ///< True if any chunk was truncated
};

// ---------------------------------------------------------------------------
// MultiStepRAGOrchestrator
// ---------------------------------------------------------------------------

/**
 * @brief Orchestrates multi-step RAG for over-budget document sets.
 *
 * Thread-compatible: use one instance per concurrent request (the
 * InferenceFn / RetrievalFn callbacks carry per-request state).
 */
class MultiStepRAGOrchestrator {
public:
    explicit MultiStepRAGOrchestrator(const MultiStepRAGConfig& cfg = {});

    // ── Strategy A: Map-Reduce ───────────────────────────────────────────────

    /**
     * @brief Run Map-Reduce RAG.
     *
     * Splits @p documents into token-budget-sized batches, calls @p infer
     * once per batch, then calls @p infer once more to synthesise the final
     * answer from the partial results.
     *
     * If all documents fit in a single context window the single-pass path is
     * taken and MultiStepRAGResult::context_overflow is false.
     *
     * @param query      Original user query.
     * @param documents  All retrieved candidate documents.
     * @param infer      LLM inference callback.
     * @return MultiStepRAGResult with final_answer and step details.
     */
    MultiStepRAGResult runMapReduce(
        const std::string&                 query,
        const std::vector<RetrievedChunk>& documents,
        const InferenceFn&                 infer) const;

    // ── Strategy B: Iterative ────────────────────────────────────────────────

    /**
     * @brief Run Iterative RAG with context accumulation.
     *
     * Starts with the provided documents, generates an initial answer, then
     * iteratively identifies uncovered query aspects, retrieves new documents,
     * and refines the answer.  Stops when the model reports no open aspects or
     * max_iterations is reached.
     *
     * @param query      Original user query.
     * @param documents  Initial set of retrieved documents.
     * @param infer      LLM inference callback (generation + gap detection).
     * @param retrieve   Retrieval callback for follow-up queries (may be {}).
     * @return MultiStepRAGResult with final_answer and iteration history.
     */
    MultiStepRAGResult runIterative(
        const std::string&                 query,
        const std::vector<RetrievedChunk>& documents,
        const InferenceFn&                 infer,
        const RetrievalFn&                 retrieve = {}) const;

    // ── Configuration ────────────────────────────────────────────────────────

    const MultiStepRAGConfig& getConfig() const;
    void setConfig(const MultiStepRAGConfig& cfg);

private:
    MultiStepRAGConfig    config_;
    RAGContextAssembler   assembler_;

    // ── Internal helpers ─────────────────────────────────────────────────────

    /// Partition @p documents into batches that each fit within the context
    /// budget (system_prompt + query overhead already accounted for).
    std::vector<std::vector<RetrievedChunk>> partitionIntoBatches(
        const std::vector<RetrievedChunk>& documents,
        const std::string&                 query) const;

    /// Build a map-step prompt from @p chunks and @p query.
    std::string buildMapPrompt(
        const std::vector<RetrievedChunk>& chunks,
        const std::string&                 query) const;

    /// Build the reduce-step prompt from @p partial_answers and @p query.
    std::string buildReducePrompt(
        const std::vector<std::string>& partial_answers,
        const std::string&              query) const;

    /// Apply simple placeholder substitution (replaces {key} with value).
    static std::string substitute(
        const std::string&              tmpl,
        const std::string&              key,
        const std::string&              value);

    /// Parse open aspects from a gap-detection LLM response.
    static std::vector<std::string> parseOpenAspects(
        const std::string& llm_response);
};

// ---------------------------------------------------------------------------
// Factory
// ---------------------------------------------------------------------------

/**
 * @brief Factory helpers for common MultiStepRAGOrchestrator configurations.
 */
class MultiStepRAGFactory {
public:
    /// Small context window (4 096 tokens, 512 response, 3 iterations max).
    static std::unique_ptr<MultiStepRAGOrchestrator> createSmallContext();

    /// Medium context window (8 192 tokens, 512 response, 4 map steps max).
    static std::unique_ptr<MultiStepRAGOrchestrator> createMediumContext();

    /// Large context window (32 768 tokens, 1 024 response, 8 map steps max).
    static std::unique_ptr<MultiStepRAGOrchestrator> createLargeContext();

    /// Custom configuration.
    static std::unique_ptr<MultiStepRAGOrchestrator> create(
        const MultiStepRAGConfig& cfg);
};

} // namespace themis::rag
