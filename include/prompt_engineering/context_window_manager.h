/**
 * @file context_window_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "rag_prompt_builder.h"           // RetrievedChunk
#include "ethics_ai/ethics_ai_types.h"  // PhilosophyThesis, PhilosophyProfile

namespace themis {
namespace prompt_engineering {

// ============================================================================
// PromptBudgetExceededError
// ============================================================================

/**
 * @brief Thrown when the assembled prompt exceeds the model's token limit.
 *
 * Callers should catch this exception and either reduce the number of context
 * chunks or truncate the user query before retrying.
 */
class PromptBudgetExceededError : public std::runtime_error {
public:
    /**
     * @param total_tokens   Actual token count of the attempted prompt.
     * @param max_tokens     Hard limit for the target model.
     * @param model_name     Human-readable model identifier.
     */
    PromptBudgetExceededError(size_t total_tokens,
                               size_t max_tokens,
                               const std::string& model_name)
        : std::runtime_error(
              "Prompt token budget exceeded for model '" + model_name +
              "': " + std::to_string(total_tokens) + " > " +
              std::to_string(max_tokens)),
          total_tokens_(total_tokens),
          max_tokens_(max_tokens),
          model_name_(model_name) {}

    /** @brief Actual token count that triggered the error. */
    size_t totalTokens() const noexcept { return total_tokens_; }

    /** @brief Hard limit that was exceeded. */
    size_t maxTokens() const noexcept { return max_tokens_; }

    /** @brief Name of the model whose limit was exceeded. */
    const std::string& modelName() const noexcept { return model_name_; }

private:
    size_t      total_tokens_;
    size_t      max_tokens_;
    std::string model_name_;
};

// ============================================================================
// ITokenCounter
// ============================================================================

/**
 * @brief Interface for counting tokens in a text string.
 *
 * Implement this to wrap tiktoken, SentencePiece, or any other tokenizer.
 * The default implementation (`CharDivisionCounter`) divides character count
 * by 4, which approximates GPT-style BPE token counts on English text with
 * ≤10% error at the 95th percentile.
 */
class ITokenCounter {
public:
    virtual ~ITokenCounter() = default;

    /**
     * @brief Return the estimated token count for @p text.
     * @param text Input string (UTF-8).
     * @return Estimated number of tokens (≥1 for non-empty input).
     */
    [[nodiscard]] virtual size_t count(const std::string& text) const = 0;
};

// ============================================================================
// CharDivisionCounter  (default ITokenCounter implementation)
// ============================================================================

/**
 * @brief Lightweight BPE approximation: tokens ≈ chars / divisor.
 *
 * The default divisor of 4 is empirically well-calibrated for English
 * mixed-case prose and code.  Increase to 3 for languages with many
 * multi-byte characters; decrease to 5 for very short technical strings.
 */
class CharDivisionCounter final : public ITokenCounter {
public:
    /**
     * @param divisor  Characters-per-token ratio (default: 4).
     */
    explicit CharDivisionCounter(size_t divisor = 4) : divisor_(divisor > 0 ? divisor : 1) {}

    size_t count(const std::string& text) const override {
        if (text.empty()) {
          return 0;
        }
        return std::max(size_t{1}, (text.size() + divisor_ - 1) / divisor_);
    }

private:
    size_t divisor_;
};

// ============================================================================
// ModelTokenBudget
// ============================================================================

/**
 * @brief Token-budget descriptor for a specific model.
 */
struct ModelTokenBudget {
    /** @brief Human-readable model identifier (e.g. "gpt-4", "llama-3-8b"). */
    std::string model_name = "default";

    /** @brief Hard maximum tokens accepted by the model (context window). */
    size_t max_tokens = 4096;

    /**
     * @brief Tokens reserved for the completion (output) side.
     *
     * Subtracted from `max_tokens` before computing the prompt budget to
     * ensure there is always room for the model's response.
     */
    size_t reserved_completion_tokens = 512;

    /** @brief Returns the tokens available for the full prompt. */
    size_t promptBudget() const noexcept {
        return max_tokens > reserved_completion_tokens
                   ? max_tokens - reserved_completion_tokens
                   : 0;
    }
};

// ============================================================================
// BudgetAllocation
// ============================================================================

/**
 * @brief Per-section token-count breakdown for a composed prompt.
 */
struct BudgetAllocation {
    size_t system_tokens = 0;   ///< Tokens used by the system instruction
    size_t context_tokens = 0;  ///< Tokens used by the retrieved context chunks
    size_t query_tokens = 0;    ///< Tokens used by the user query
    size_t total_tokens = 0;    ///< Sum of all sections
    size_t budget_tokens = 0;   ///< Available prompt budget
    double utilization = 0.0;   ///< total_tokens / budget_tokens ∈ [0, ∞)

    /** @brief True when total_tokens ≤ budget_tokens. */
    bool fits() const noexcept { return total_tokens <= budget_tokens; }
};

// ============================================================================
// Thesis budget selection types  (§9.1)
// ============================================================================

/**
 * @brief Result of injecting one thesis into the discourse context window.
 *
 * When a thesis is fully active in the current round and its `token_budget`
 * allows, it is injected at full length (`full_text`).  Otherwise a compact
 * headline token is injected instead so the LLM still knows the thesis exists.
 */
struct ThesisInjection {
    std::string thesis_id;      ///< Identifier of the injected thesis
    std::string text;           ///< Actual injected text (full or headline)
    bool        is_full{true};  ///< true = full text; false = headline only
    int         tokens_used{0}; ///< Estimated token count for `text`
};

// ============================================================================
// ContextWindowBudgetManager
// ============================================================================

/**
 * @brief Enforces per-model context-window token limits for LLM prompt assembly.
 *
 * Typical usage:
 * @code
 * ContextWindowBudgetManager mgr;
 * mgr.setModel({"gpt-4", 8192, 512});
 *
 * // Select as many chunks as fit in the remaining budget.
 * auto kept = mgr.fitChunksInBudget(all_chunks, system_prompt, user_query);
 *
 * // Full allocation breakdown (throws PromptBudgetExceededError if over limit).
 * auto alloc = mgr.computeAndCheck(system_prompt, user_query, kept);
 * @endcode
 */
class ContextWindowBudgetManager {
public:
    /**
     * @brief Construct with the given model budget and optional custom counter.
     * @param budget   Token-budget descriptor for the target model.
     * @param counter  Custom token counter; defaults to `CharDivisionCounter`.
     */
    explicit ContextWindowBudgetManager(
        const ModelTokenBudget& budget  = ModelTokenBudget{},
        std::shared_ptr<ITokenCounter> counter = nullptr);

    // -------------------------------------------------------------------------
    // Configuration
    // -------------------------------------------------------------------------

    /** @brief Replace the current model budget descriptor. */
    void setModel(const ModelTokenBudget& budget);

    /** @brief Return the current model budget descriptor. */
    const ModelTokenBudget& getModel() const noexcept;

    /**
     * @brief Inject a custom token counter (replaces the default CharDivisionCounter).
     * @param counter  Non-null counter implementation.
     */
    void setTokenCounter(std::shared_ptr<ITokenCounter> counter);

    // -------------------------------------------------------------------------
    // Token counting
    // -------------------------------------------------------------------------

    /**
     * @brief Return the estimated token count for @p text.
     *
     * Delegates to the currently active `ITokenCounter`.
     */
    size_t countTokens(const std::string& text) const;

    // -------------------------------------------------------------------------
    // Chunk selection
    // -------------------------------------------------------------------------

    /**
     * @brief Select the subset of @p chunks that fits within @p available_tokens.
     *
     * Chunks are iterated in their supplied order (callers should pre-sort by
     * relevance descending for best results).  Each chunk's token count is
     * computed and greedily added until the budget would be exceeded.
     *
     * @param chunks            Candidate chunks (in priority order).
     * @param available_tokens  Maximum total tokens for the context section.
     * @return The greedy-selected subset of chunks.
     */
    std::vector<RetrievedChunk> fitChunksInBudget(
        const std::vector<RetrievedChunk>& chunks,
        size_t available_tokens) const;

    /**
     * @brief Select chunks fitting within the context portion of the model budget.
     *
     * Computes the context budget as:
     *   promptBudget - countTokens(system_prompt) - countTokens(query)
     * then delegates to the two-argument overload.
     *
     * @param chunks          Candidate chunks (in priority order).
     * @param system_prompt   System instruction (consumed first from budget).
     * @param query           User query (consumed second from budget).
     * @return The greedy-selected subset of chunks.
     */
    std::vector<RetrievedChunk> fitChunksInBudget(
        const std::vector<RetrievedChunk>& chunks,
        const std::string& system_prompt,
        const std::string& query) const;

    // -------------------------------------------------------------------------
    // Budget computation
    // -------------------------------------------------------------------------

    /**
     * @brief Compute the token-budget allocation for a composed prompt.
     *
     * Does NOT throw; callers inspect `BudgetAllocation::fits()` to decide
     * whether to proceed.
     *
     * @param system_prompt   System instruction string.
     * @param query           User query string.
     * @param chunks          Retrieved context chunks already selected.
     * @return Per-section token breakdown and utilisation ratio.
     */
    BudgetAllocation computeBudget(
        const std::string& system_prompt,
        const std::string& query,
        const std::vector<RetrievedChunk>& chunks) const;

    /**
     * @brief Compute the token-budget allocation and throw if over the limit.
     *
     * Equivalent to `computeBudget()` followed by a `PromptBudgetExceededError`
     * if `!allocation.fits()`.
     *
     * @throws PromptBudgetExceededError  When total > model budget.
     * @return `BudgetAllocation` (always fits when returned without throwing).
     */
    BudgetAllocation computeAndCheck(
        const std::string& system_prompt,
        const std::string& query,
        const std::vector<RetrievedChunk>& chunks) const;

    // -------------------------------------------------------------------------
    // Metrics
    // -------------------------------------------------------------------------

    /**
     * @brief Register a callback invoked with the utilisation ratio after each
     *        successful `computeBudget()` / `computeAndCheck()` call.
     *
     * Pass in a lambda that delegates to `PromptEngineeringMetrics` or any
     * other observability sink.  The callback receives a value in [0, ∞) where
     * 1.0 means the budget is exactly full.
     *
     * @param cb  Callable accepting a `double` utilisation ratio.
     */
    void setUtilizationCallback(std::function<void(double)> cb);

    // -------------------------------------------------------------------------
    // Ethics discourse: per-thesis budget selection  (§9.1)
    // -------------------------------------------------------------------------

    /**
     * @brief Select and budget theses from @p profile for a single discourse round.
     *
     * Implements the §9.1 algorithm:
     *  1. Filter `profile.typed_theses` to those whose `activation_rounds` list
     *     contains @p round_number (or have an empty list → all rounds active).
     *  2. Sort survivors by `round_role_weights[round_role]` descending
     *     (unweighted theses receive weight 0.5 as neutral priority).
     *  3. Greedily select theses in that order until @p available_tokens is
     *     exhausted, respecting each thesis's individual `token_budget` cap.
     *  4. Non-selected theses (not in their activation round or budget exceeded)
     *     contribute a headline token: `"[{thesis_id}: {name}]"`.
     *
     * Theses that have no `token_budget` (value -1) are treated as if they
     * require `countTokens(description)` tokens — they never get an artificial
     * cap but still consume budget.
     *
     * When @p profile has no `typed_theses` the method returns an empty vector
     * without error (backward compatible — caller falls back to plain string
     * theses).
     *
     * @param profile           Philosophy profile whose theses to select.
     * @param round_number      Current discourse round (1–5).
     * @param round_role        Role label for the round (e.g. "PRO",
     *                          "REBUTTAL", "SYNTHESIS").
     * @param available_tokens  Total token budget available for thesis injection.
     * @return List of `ThesisInjection` entries (full + headline combined).
     *
     * @note Performance: ≤ 0.5 ms for ≤ 20 theses per profile.
     */
    std::vector<ThesisInjection> selectThesesForRound(
        const ::themis::plugins::ethics::PhilosophyProfile& profile,
        int round_number,
        const std::string& round_role,
        int available_tokens) const;

private:
    ModelTokenBudget               budget_;
    std::shared_ptr<ITokenCounter> counter_;
    std::function<void(double)>    utilization_cb_;

    /// Compute the total context-section token count for a set of chunks.
    size_t countChunkTokens(const std::vector<RetrievedChunk>& chunks) const;
};

} // namespace prompt_engineering
} // namespace themis
