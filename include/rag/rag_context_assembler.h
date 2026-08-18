/**
 * @file rag_context_assembler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
 * Deterministically packs retrieved chunks into a fixed context window,
 * ensuring the model's response budget is always reserved. Supports
 * optional chunk truncation to maximize context utilization.
 *
 * ## Thread Safety
 * All methods are **thread-safe**: no mutable shared state beyond
 * constructor-injected configuration. The config_ member is immutable
 * after construction or explicit setConfig() call; concurrent assemble()
 * calls do not race.
 *
 * ## Complexity Analysis
 * - **assemble()**: O(n log n) for sorting + O(n) for greedy fill
 *   - n = number of input chunks
 *   - Sorting: deterministic tie-breaking by relevance_score, chunk_id, source, content
 *   - Greedy fill: single pass with token estimation per chunk
 * - **truncateContent()**: O(k) where k = content length
 *   - Deterministic UTF-8-safe truncation with marker suffix
 *
 * ## Failure Modes
 * - **Empty input**: returns valid empty AssembledContext (not error)
 * - **Zero budget**: returns valid empty AssembledContext with response reservation
 * - **All chunks over-budget**: greedy returns what fits or empty if nothing fits
 * - **Budget overflow**: integer overflow in token counting prevented by bounds
 *
 * ## Response Reservation Guarantee
 * The response budget is always @c max(min_response_tokens, 20% of window).
 * This is enforced by ContextWindowBudget::compute() and never violated
 * even if all retrieved chunks fit.
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
     * Deterministically selects a subset of @p chunks that fit within the
     * context window while reserving tokens for the model's response.
     *
     * ## Algorithm
     * 1. Compute ContextWindowBudget from config + system_prompt + query.
     * 2. Sort @p chunks by relevance_score descending with deterministic
     *    tie-breaking (chunk_id, source, content).
     * 3. Greedily fill the available token budget in sorted order.
     * 4. Optionally truncate the last over-budget chunk if config_.allow_partial_chunk is true.
     *
     * ## Complexity
     * - **Time**: O(n log n) where n = chunks.size()
     * - **Space**: O(n) for result.chunks_used
     *
     * @param chunks        Candidate retrieved chunks (may be empty).
     * @param system_prompt System / instruction prompt (used for budget calculation).
     * @param query         User query (used for budget calculation).
     *
     * @return AssembledContext with:
     *   - chunks_used: selected chunks in relevance order
     *   - tokens_used: estimated token count of assembled context
     *   - tokens_remaining_for_response: guaranteed response budget
     *   - was_truncated: true if the last chunk was truncated to fit
     *
     * @pre config_.model_context_tokens > 0 (or defaults to kDefaultContextWindowTokens)
     * @pre config_.min_response_tokens >= 0
     * @pre chunks.empty() is valid; returns valid empty context
     *
     * @post result.tokens_remaining_for_response >= min_response_tokens
     * @post result.tokens_remaining_for_response >= 20% of model_context_tokens
     * @post result.tokens_used + result.tokens_remaining_for_response <= model_context_tokens
     * @post result.chunks_used is sorted by relevance descending
     *
     * @note Thread-safe: no concurrent mutation of config_ during call
     * @note Fail-closed: empty input or zero budget returns valid empty context
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
     * context window can hold after the prompt is filled. This guards against
     * token budget exhaustion during generation.
     *
     * ## Calculation
     * 1. Calculate available_response_tokens = budget.reserved_response_tokens
     * 2. If @p user_max > 0: clamp to min(user_max, available_response_tokens)
     * 3. Otherwise: use available_response_tokens
     * 4. Ensure result >= 1 (model must be able to generate at least one token)
     *
     * @param budget    Pre-computed ContextWindowBudget for the request (from
     *                  ContextWindowBudget::compute(context_window,
     *                                               system_prompt, query,
     *                                               min_response_tokens)).
     * @param user_max  Caller-supplied cap. If 0: unconstrained by caller.
     *                  If >0: result is min(user_max, available_response).
     *
     * @return Recommended max_tokens value for InferenceRequest.
     * @post return value >= 1
     * @post return value <= budget.reserved_response_tokens
     * @post if user_max > 0: return value <= user_max
     *
     * @note Thread-safe: stateless function
     * @note Complexity: O(1)
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
