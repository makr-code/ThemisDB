/**
 * @file context_window_budget.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.10
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
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

enum class TokenEstimationMethod {
    CHAR_HEURISTIC,  ///< ceil(chars / 3.5) — no external library required
    LLAMA_TOKENIZER  ///< Use llama.cpp llama_tokenize() (requires loaded model)
};

static constexpr double kCharsPerTokenHeuristic = 3.5;

inline size_t estimateTokens(
    const std::string& text,
    TokenEstimationMethod method = TokenEstimationMethod::CHAR_HEURISTIC)
{
    (void)method;
    if (text.empty()) {
      return 0u;
    }
    return static_cast<size_t>(
        std::ceil(static_cast<double>(text.size()) / kCharsPerTokenHeuristic));
}

inline size_t estimateTokens(
    size_t char_count,
    TokenEstimationMethod method = TokenEstimationMethod::CHAR_HEURISTIC)
{
    (void)method;
    if (char_count == 0u) {
      return 0u;
    }
    return static_cast<size_t>(
        std::ceil(static_cast<double>(char_count) / kCharsPerTokenHeuristic));
}

/**
 * @brief Tokens To Chars.
 * @param[in] tokens Input parameter.
 * @return Return value.
 */
inline size_t tokensToChars(size_t tokens)
{
    return static_cast<size_t>(
        static_cast<double>(tokens) * kCharsPerTokenHeuristic);
}

// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static constexpr size_t kDefaultContextWindowTokens = 4096u;

static constexpr double kMinResponseFraction = 0.20;

static constexpr size_t kDefaultMinResponseTokens = 512u;

// ---------------------------------------------------------------------------
// ContextWindowBudget
// ---------------------------------------------------------------------------

struct ContextWindowBudget {
    size_t model_max_tokens         = kDefaultContextWindowTokens;
    size_t system_prompt_tokens     = 0u;
    size_t query_tokens             = 0u;
    size_t reserved_response_tokens = kDefaultMinResponseTokens;
    size_t available_context_tokens = 0u;

    // ── Factory ──────────────────────────────────────────────────────────────

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

    size_t availableContextChars() const
    {
        return tokensToChars(available_context_tokens);
    }

    bool hasContextBudget() const
    {
        return available_context_tokens > 0u;
    }

    size_t responseBudgetAfterContext(size_t context_tokens_used) const
    {
        const size_t used =
            system_prompt_tokens + query_tokens + context_tokens_used;
        if (used >= model_max_tokens) {
          return reserved_response_tokens;
        }
        return model_max_tokens - used;
    }
};

} // namespace themis::llm
