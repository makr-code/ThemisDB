/**
 * @file context_window_budget.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
*
 * @note **Configuration/Metadata**: Defines configuration and tracking structures.
 *       No .cpp implementation needed. Used by consumers for configuration.
 */

#pragma once

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>

namespace themis::llm {

// ---------------------------------------------------------------------------
// Token estimation
// ---------------------------------------------------------------------------

/// Method used to count tokens in a string.
enum class TokenEstimationMethod {
    CHAR_HEURISTIC,  ///< ceil(chars / 3.5) — no external library required
    LLAMA_TOKENIZER  ///< Use llama.cpp llama_tokenize() (requires loaded model)
};

/// Characters-per-token divisor for the heuristic estimator.
static constexpr double kCharsPerTokenHeuristic = 3.5;

/**
 * @brief Estimate the number of tokens in @p text.
 *
 * Uses the CHAR_HEURISTIC regardless of @p method until a live tokenizer is
 * wired in.  The result is always >= 1 for non-empty input.
 *
 * @param text   Input string.
 * @param method Estimation method (currently CHAR_HEURISTIC is always used).
 * @return Estimated token count.
 */
inline size_t estimateTokens(
    const std::string& text,
    TokenEstimationMethod method = TokenEstimationMethod::CHAR_HEURISTIC)
{
    (void)method;
    if (text.empty()) return 0u;
    return static_cast<size_t>(
        std::ceil(static_cast<double>(text.size()) / kCharsPerTokenHeuristic));
}

/**
 * @brief Estimate token count from a raw character count.
 */
inline size_t estimateTokens(
    size_t char_count,
    TokenEstimationMethod method = TokenEstimationMethod::CHAR_HEURISTIC)
{
    (void)method;
    if (char_count == 0u) return 0u;
    return static_cast<size_t>(
        std::ceil(static_cast<double>(char_count) / kCharsPerTokenHeuristic));
}

/**
 * @brief Convert an estimated token count back to an approximate character
 *        budget (inverse of the heuristic).
 */
inline size_t tokensToChars(size_t tokens)
{
    return static_cast<size_t>(
        static_cast<double>(tokens) * kCharsPerTokenHeuristic);
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

/// Fallback context-window size when ModelInfo::context_length is 0.
static constexpr size_t kDefaultContextWindowTokens = 4096u;

/// Minimum fraction of the total window reserved for the model response.
static constexpr double kMinResponseFraction = 0.20;

/// Default minimum tokens reserved for the model response.
static constexpr size_t kDefaultMinResponseTokens = 512u;

// ---------------------------------------------------------------------------
// ContextWindowBudget
// ---------------------------------------------------------------------------

/**
 * @brief Decomposed token budget for a single RAG inference call.
 *
 * Fields:
 * @code
 *   model_max_tokens
 *   ├── system_prompt_tokens
 *   ├── query_tokens
 *   ├── reserved_response_tokens   (enforced lower bound)
 *   └── available_context_tokens   (what remains for retrieved chunks)
 * @endcode
 *
 * Invariant: available_context_tokens >= 0 (never negative; clamped at 0).
 *
 * Usage:
 * @code
 *   auto budget = ContextWindowBudget::compute(
 *       model_info.context_length,
 *       system_prompt,
 *       user_query,
 *       512u); // min_response_tokens
 *
 *   // Budget for context chunks (in characters):
 *   size_t char_budget = budget.availableContextChars();
 * @endcode
 */
struct ContextWindowBudget {
    size_t model_max_tokens         = kDefaultContextWindowTokens;
    size_t system_prompt_tokens     = 0u;
    size_t query_tokens             = 0u;
    size_t reserved_response_tokens = kDefaultMinResponseTokens;
    size_t available_context_tokens = 0u;

    // ── Factory ──────────────────────────────────────────────────────────────

    /**
     * @brief Compute a budget for a given model and request.
     *
     * @param model_ctx     Model's maximum context window in tokens.
     *                      0 triggers the kDefaultContextWindowTokens fallback.
     * @param system_prompt System / instruction prompt text (may be empty).
     * @param query         User query text (may be empty).
     * @param min_response  Caller-specified minimum response token budget.
     *                      The actual reservation is max(min_response, 20% of
     *                      the context window) to prevent the model from
     *                      producing only a few tokens.
     * @return Fully computed ContextWindowBudget.
     */
    static ContextWindowBudget compute(
        size_t             model_ctx,
        const std::string& system_prompt,
        const std::string& query,
        size_t             min_response = kDefaultMinResponseTokens)
    {
        ContextWindowBudget b;
        b.model_max_tokens =
            (model_ctx > 0u) ? model_ctx : kDefaultContextWindowTokens;

        // Enforce: response_budget >= max(min_response, 20% of window)
        const size_t response_floor = static_cast<size_t>(
            std::ceil(static_cast<double>(b.model_max_tokens) *
                      kMinResponseFraction));
        b.reserved_response_tokens = std::max(min_response, response_floor);

        b.system_prompt_tokens = estimateTokens(system_prompt);
        b.query_tokens         = estimateTokens(query);

        const size_t overhead =
            b.system_prompt_tokens +
            b.query_tokens +
            b.reserved_response_tokens;

        // Clamp to zero — never let the context budget go negative.
        b.available_context_tokens =
            (overhead < b.model_max_tokens)
                ? (b.model_max_tokens - overhead)
                : 0u;

        return b;
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    /// Approximate character count available for context chunks.
    size_t availableContextChars() const
    {
        return tokensToChars(available_context_tokens);
    }

    /// True when there is at least one token of context budget remaining.
    bool hasContextBudget() const
    {
        return available_context_tokens > 0u;
    }

    /**
     * @brief Remaining response budget after @p context_tokens_used context
     *        tokens have been consumed.
     *
     * @param context_tokens_used  Tokens actually used by assembled context.
     * @return Tokens available for the model's response (>= reserved_response_tokens).
     */
    size_t responseBudgetAfterContext(size_t context_tokens_used) const
    {
        const size_t used =
            system_prompt_tokens + query_tokens + context_tokens_used;
        if (used >= model_max_tokens) return reserved_response_tokens;
        return model_max_tokens - used;
    }
};

} // namespace themis::llm
