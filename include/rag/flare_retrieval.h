/**
 * @file flare_retrieval.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ============================================================================
// FlareConfig — gating and regeneration parameters
// ============================================================================

/**
 * @brief Configuration for FLARE log-probability gating.
 */
struct FlareConfig {
    /**
     * @brief Log-probability threshold for triggering retrieval.
     *
     * When log(p(t)) < confidence_threshold for a generated token, the token
     * is considered low-confidence and retrieval is scheduled.
     *
     * Default: ln(0.1) ≈ -2.303 (trigger when token probability < 10%).
     * For more conservative retrieval (only very uncertain tokens): -3.0.
     * For aggressive retrieval (more frequent): -1.0.
     */
    float confidence_threshold = -2.303f;

    /**
     * @brief Minimum number of consecutive low-confidence tokens before
     *        retrieval is triggered.
     *
     * Prevents retrieval on a single uncertain token in an otherwise
     * confident sequence (e.g., rare proper nouns with known context).
     *
     * Default: 1 (trigger on first low-confidence token).
     */
    std::size_t min_consecutive_uncertain = 1;

    /**
     * @brief Cool-down period (number of tokens) after a successful retrieval.
     *
     * Prevents redundant re-retrieval while injected context is still fresh.
     * Set to 0 to disable.
     *
     * Default: 30 tokens (roughly one sentence).
     */
    std::size_t retrieval_cooldown_tokens = 30;

    /**
     * @brief Top-K results to request from the TT-core index.
     */
    std::size_t top_k = 5;

    /**
     * @brief Maximum number of FLARE retrieval steps per generation sequence.
     *
     * Caps the total number of mid-generation retrievals to bound latency.
     * Set to 0 for unlimited.
     *
     * Default: 10 steps.
     */
    std::size_t max_retrieval_steps = 10;

    /**
     * @brief Maximum number of tokens to include in the look-ahead query.
     *
     * When building the retrieval query from partial_output, at most
     * `query_window_tokens` most-recent tokens are used.
     *
     * Default: 64 tokens.
     */
    std::size_t query_window_tokens = 64;

    /**
     * @brief If true, low-confidence tokens are masked (replaced with
     *        a special placeholder) in the query text before retrieval.
     *
     * Masking focuses retrieval on predictable context rather than the
     * model's uncertain guesses, improving query relevance.
     *
     * Default: true (per §FLARE algorithm description).
     */
    bool mask_uncertain_tokens = true;

    /**
     * @brief Placeholder string used to replace masked tokens.
     *
     * Default: "[MASK]"
     */
    std::string mask_token = "[MASK]";
};

// ============================================================================
// FlareDecision — result of a single gating step
// ============================================================================

/**
 * @brief Output of `FlareRetrieval::decide()`, describing the gating decision.
 */
struct FlareDecision {
    bool  should_retrieve          = false;  ///< True → trigger retrieval + regenerate
    bool  in_cooldown              = false;  ///< True → suppressed by cooldown
    bool  max_steps_reached        = false;  ///< True → suppressed by max_retrieval_steps
    float last_log_prob            = 0.0f;   ///< Log-probability of most-recent token
    std::size_t consecutive_uncertain = 0;   ///< Running uncertain-token count
    std::size_t retrieval_steps_done  = 0;   ///< Total retrievals so far this session
};

// ============================================================================
// FlareRetrieval — stateful FLARE gating logic
// ============================================================================

/**
 * @brief Stateful FLARE gate: decides per-token whether mid-generation
 *        retrieval and span regeneration are required.
 *
 * Maintains per-session state (partial output, consecutive uncertainty
 * count, cooldown counter, retrieval step count).  Create one instance
 * per generation session and call `notifyTokenEmitted()` for each generated
 * token, then check `shouldRetrieve()`.
 *
 * ### Thread safety
 * Not thread-safe.  Use one instance per generation thread.
 */
class FlareRetrieval {
public:
    explicit FlareRetrieval(FlareConfig cfg = {});

    // ─── Token notification ───────────────────────────────────────────────

    /**
     * @brief Notify the gate that a token was emitted with a given log-prob.
     *
     * Updates internal state (consecutive uncertainty count, cooldown,
     * partial output window).  Call once per generated token.
     *
     * @param token_text  Surface form of the emitted token (for query building).
     * @param log_prob    Natural-log probability of this token, i.e. log(p(t)).
     *                    Must be ≤ 0.
     */
    void notifyTokenEmitted(const std::string& token_text, float log_prob);

    // ─── Gating decision ─────────────────────────────────────────────────

    /**
     * @brief Full FLARE decision after the most-recent token was emitted.
     *
     * Call immediately after `notifyTokenEmitted()`.
     *
     * @return FlareDecision struct with all gating metrics.
     */
    [[nodiscard]] FlareDecision decide() const noexcept;

    /**
     * @brief Convenience wrapper — returns true if retrieval should occur.
     *
     * Equivalent to `decide().should_retrieve`.
     */
    [[nodiscard]] bool shouldRetrieve() const noexcept;

    // ─── Query construction ───────────────────────────────────────────────

    /**
     * @brief Callable type for a text-to-embedding backend.
     *
     * Signature: `std::vector<float> embed(const std::string& text)`
     *
     * When set via `setEmbeddingQueryFn()`, `buildQueryEmbedding()` delegates
     * to this fn, enabling semantic TT-cosine similarity lookups.  The fn must
     * be thread-safe; it is called under no internal lock.
     */
    using EmbeddingQueryFn = std::function<std::vector<float>(const std::string&)>;

    /**
     * @brief Inject an embedding backend (thread-safe, process-global).
     *
     * Once set, `buildQueryEmbedding()` will call @p fn to convert the
     * surface-form query into a float vector.  Pass a null fn to revert to
     * the no-embedding fallback (returns empty vector).
     *
     * @param fn  Embedding function wrapping e.g. a quantised SBERT encoder.
     */
    static void setEmbeddingQueryFn(EmbeddingQueryFn fn);

    /**
     * @brief Build a retrieval query from the current partial-output window.
     *
     * Concatenates the most-recent `cfg_.query_window_tokens` tokens.
     * When `mask_uncertain_tokens = true`, low-confidence tokens are
     * replaced with `cfg_.mask_token`.
     *
     * @return Query string ready to be embedded and passed to the TT-core index.
     */
    [[nodiscard]] std::string buildQuery() const;

    /**
     * @brief Build an embedding vector from the current partial-output window.
     *
     * Calls the injected `EmbeddingQueryFn` on the text produced by
     * `buildQuery()`.  Returns an empty vector when no fn is set or when the
     * fn returns empty / throws (fail-closed, warning logged).
     *
     * @return Float embedding vector, or empty when no backend is wired.
     */
    [[nodiscard]] std::vector<float> buildQueryEmbedding() const;

    // ─── State management ─────────────────────────────────────────────────

    /**
     * @brief Notify the gate that a retrieval was successfully executed.
     *
     * Increments the retrieval step counter, resets consecutive-uncertain
     * count, and starts the cool-down timer.
     *
     * Call this immediately after performing retrieval (before regenerating).
     */
    void notifyRetrievalExecuted();

    /**
     * @brief Reset all state for a new generation session.
     */
    void reset();

    // ─── Diagnostics ──────────────────────────────────────────────────────

    struct FlareStats {
        std::size_t tokens_emitted      = 0;
        std::size_t retrieval_triggers  = 0;
        std::size_t cooldown_skips      = 0;
        std::size_t max_steps_skips     = 0;
        double      mean_log_prob       = 0.0;
        float       min_log_prob        = 0.0f;
        float       max_log_prob        = 0.0f;

        /// Fraction of tokens that triggered retrieval.
        [[nodiscard]] double triggerRate() const noexcept {
            return tokens_emitted > 0
                ? static_cast<double>(retrieval_triggers) / tokens_emitted
                : 0.0;
        }
    };

    /**
     * @brief Cumulative statistics since construction (or last reset()).
     */
    [[nodiscard]] FlareStats stats() const noexcept { return stats_; }

    /**
     * @brief Number of retrieval steps executed in this session.
     */
    [[nodiscard]] std::size_t retrievalStepsDone() const noexcept {
        return retrieval_steps_done_;
    }

private:
    FlareConfig cfg_;

    // Per-token sliding window (most-recent `query_window_tokens` entries)
    struct TokenEntry {
        std::string text;
        float       log_prob = 0.0f;
        bool        uncertain = false;
    };
    std::vector<TokenEntry> window_;

    std::size_t consecutive_uncertain_ = 0;
    std::size_t cooldown_remaining_    = 0;
    std::size_t retrieval_steps_done_  = 0;

    FlareStats stats_;

    // Whether the most-recently evaluated gate state said should_retrieve.
    bool pending_retrieval_ = false;
};

} // namespace rag
} // namespace themis
