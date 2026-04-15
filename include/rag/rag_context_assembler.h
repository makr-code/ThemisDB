/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag_context_assembler.h                            ║
  Version:         0.0.8                                              ║
  Last Modified:   2026-04-15 07:08:50                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     186                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 01a86c4f10  2026-04-07  Changes before error encountered        ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file rag_context_assembler.h
 * @brief Budget-aware context assembler for RAG inference.
 *
 * Implements the "Greedy Fill with Response Guard" strategy:
 *
 *   1. Compute a ContextWindowBudget for the current model context window,
 *      system prompt, and user query.
 *   2. Sort retrieved chunks by relevance score (descending).
 *   3. Greedily add chunks while the token budget allows.
 *   4. If the last chunk does not fit in full, truncate it and append a
 *      configurable truncation marker instead of silently dropping it.
 *   5. Return an AssembledContext that reports which chunks were used, how
 *      many tokens they consumed, and how many tokens are left for the
 *      model response.
 *
 * The assembler also provides computeMaxTokens() which derives the
 * InferenceRequest::max_tokens value so the caller never accidentally
 * requests more response tokens than the context window can hold.
 */

#pragma once

#include "llm/context_window_budget.h"
#include "prompt_engineering/rag_prompt_builder.h"

#include <string>
#include <vector>

namespace themis::rag {

using ::themis::llm::ContextWindowBudget;
using ::themis::prompt_engineering::RetrievedChunk;

// ---------------------------------------------------------------------------
// AssembledContext
// ---------------------------------------------------------------------------

/**
 * @brief Result of a context assembly pass.
 *
 * Carries the selected (and possibly truncated) chunks together with token
 * accounting data that allows the caller to set InferenceRequest::max_tokens
 * precisely.
 */
struct AssembledContext {
    /// Chunks that fit within the context token budget (in relevance order).
    std::vector<RetrievedChunk> chunks_used;

    /// Estimated tokens consumed by the assembled chunks.
    size_t tokens_used = 0u;

    /// Tokens still available for the model's response after context fill.
    size_t tokens_remaining_for_response = 0u;

    /// True when the last chunk was truncated to fit the remaining budget.
    bool was_truncated = false;
};

// ---------------------------------------------------------------------------
// RAGContextAssemblerConfig
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for RAGContextAssembler.
 */
struct RAGContextAssemblerConfig {
    /// Model context window in tokens.
    /// Set from ModelInfo::context_length; 0 → kDefaultContextWindowTokens.
    size_t model_context_tokens = 4096u;

    /// Minimum tokens reserved for the model's answer.
    /// The actual reservation is max(min_response_tokens, 20% of the window).
    size_t min_response_tokens = 512u;

    /// Marker appended to a truncated chunk so readers know content was cut.
    std::string truncation_marker = " [TRUNCATED]";

    /// When true, the last chunk that does not fully fit is truncated rather
    /// than dropped.  Set to false to always include only complete chunks.
    bool allow_partial_chunk = true;
};

// ---------------------------------------------------------------------------
// RAGContextAssembler
// ---------------------------------------------------------------------------

/**
 * @brief Budget-aware context assembler for RAG inference.
 *
 * Thread-safe: all methods are const or operate on local state.
 *
 * Usage:
 * @code
 *   RAGContextAssemblerConfig cfg;
 *   cfg.model_context_tokens = model_info.context_length;
 *   cfg.min_response_tokens  = 512;
 *
 *   RAGContextAssembler assembler(cfg);
 *   AssembledContext ctx = assembler.assemble(chunks, system_prompt, query);
 *
 *   // Wire into the inference request:
 *   request.max_tokens = RAGContextAssembler::computeMaxTokens(
 *       ContextWindowBudget::compute(cfg.model_context_tokens,
 *                                    system_prompt, query,
 *                                    cfg.min_response_tokens),
 *       user_supplied_max_tokens);
 * @endcode
 */
class RAGContextAssembler {
public:
    explicit RAGContextAssembler(const RAGContextAssemblerConfig& cfg = {});

    // ── Core API ─────────────────────────────────────────────────────────────

    /**
     * @brief Assemble context chunks within the token budget.
     *
     * Steps:
     *  1. Compute ContextWindowBudget from config + system_prompt + query.
     *  2. Sort @p chunks by relevance_score descending.
     *  3. Greedily fill the available token budget.
     *  4. Optionally truncate the last over-budget chunk.
     *
     * @param chunks        Candidate retrieved chunks.
     * @param system_prompt System / instruction prompt (used for budget calc).
     * @param query         User query (used for budget calc).
     * @return AssembledContext describing the selected chunks and token usage.
     */
    AssembledContext assemble(
        const std::vector<RetrievedChunk>& chunks,
        const std::string&                 system_prompt,
        const std::string&                 query) const;

    // ── Response-budget helper ────────────────────────────────────────────────

    /**
     * @brief Compute the recommended InferenceRequest::max_tokens value.
     *
     * Ensures the model cannot be asked to generate more tokens than the
     * context window can hold after the prompt is filled.
     *
     * @param budget    Pre-computed ContextWindowBudget for the request.
     * @param user_max  Caller-supplied cap (0 = unconstrained by caller).
     * @return Recommended max_tokens value (always >= 1).
     */
    static int computeMaxTokens(
        const ContextWindowBudget& budget,
        int                        user_max = 0);

    // ── Configuration ────────────────────────────────────────────────────────

    const RAGContextAssemblerConfig& getConfig() const;
    void setConfig(const RAGContextAssemblerConfig& cfg);

private:
    RAGContextAssemblerConfig config_;

    /// Truncate @p content to at most @p max_chars, appending truncation_marker.
    std::string truncateContent(const std::string& content,
                                size_t             max_chars) const;
};

} // namespace themis::rag
