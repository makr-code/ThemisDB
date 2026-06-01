/*
 * ThemisDB | File: targ_retrieval.h | Version: 1.0.0
 * Maturity: 🟢 PRODUCTION-READY | Score: 94/100
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

/**
 * @file rag/targ_retrieval.h
 * @brief TARG — Token-Adaptive Retrieval Gating for ThemisDB RAG pipelines.
 *
 * ## Problem
 *
 * Classical RAG pipelines retrieve on every generated token, causing
 * 70–90% of retrieval calls to be unnecessary.  This inflates TTFT and
 * overwhelms the TT-core index with low-value queries.
 *
 * ## Solution (TARG — paper §TARG)
 *
 * Before each potential retrieval call, measure the **logit gap**:
 *
 *   gap = logit[top-1] − logit[top-2]
 *
 * A large gap (≥ threshold) means the model is highly confident about
 * the next token — retrieval is unnecessary.  A small gap triggers a
 * ThemisDB TT-core retrieval to inject additional evidence.
 *
 * Expected outcome: ≤ 1.5× latency vs. Never-RAG while matching
 * Always-RAG accuracy on MMLU / NQ benchmarks.
 *
 * ## Integration
 *
 * ```cpp
 * TARGConfig cfg;
 * cfg.gap_threshold = 5.0f;   // trigger when logit gap < 5
 * cfg.top_k         = 3;
 *
 * TARGRetrieval targ(cfg);
 *
 * // Inside the generation loop:
 * auto logits = lm.getLogits();          // raw logits over vocab
 * if (targ.shouldRetrieve(logits)) {
 *     auto query_tt = targ.buildQueryTT(partial_output);
 *     auto results  = tensor_index.search(query_tt, cfg.top_k);
 *     lm.injectContext(results);
 * }
 * ```
 *
 * ## References
 * - paper §TARG: single-shot draft measures top-1/top-2 logit gap
 * - ThemisDB Research Group (2026). Internal pre-print.
 */

#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ============================================================================
// TARGConfig — gating parameters
// ============================================================================

/**
 * @brief Configuration for TARG logit-gap gating.
 */
struct TARGConfig {
    /**
     * @brief Logit-gap threshold for triggering retrieval.
     *
     * When (logit[top-1] − logit[top-2]) < gap_threshold, retrieval is
     * triggered.  Larger values trigger retrieval more often; smaller values
     * trust the language model more.
     *
     * Default 5.0 is tuned for 7B–13B models on MMLU (paper §TARG Table 2).
     * For smaller models (1B–3B), try 3.0; for larger (30B+), try 8.0.
     */
    float gap_threshold = 5.0f;

    /**
     * @brief Minimum number of consecutive low-confidence tokens before
     *        retrieval is triggered.  Prevents over-retrieval on single
     *        uncertain tokens in otherwise confident sequences.
     *
     * Default: 1 (trigger immediately on first low-confidence token).
     */
    std::size_t min_consecutive_uncertain = 1;

    /**
     * @brief Cool-down period (number of tokens) after a successful retrieval.
     *
     * Prevents redundant re-retrieval while injected context is still fresh.
     * Set to 0 to disable.
     *
     * Default: 20 tokens.
     */
    std::size_t retrieval_cooldown_tokens = 20;

    /**
     * @brief Top-K results to request from the TT-core index when triggered.
     */
    std::size_t top_k = 3;

    /**
     * @brief If true, also trigger retrieval when the entropy of the top-K
     *        probability mass exceeds `entropy_threshold`.  This catches
     *        spread-uncertainty scenarios not captured by the gap metric.
     */
    bool use_entropy_gate = false;

    /**
     * @brief Entropy threshold in nats.  Only used when use_entropy_gate = true.
     *
     * For a uniform distribution over V tokens: entropy = ln(V) ≈ 10.8 for V=50k.
     * Typical "uncertain" threshold: 3.0–5.0 nats.
     */
    float entropy_threshold = 4.0f;
};

// ============================================================================
// TARGDecision — result of a single gating call
// ============================================================================

/**
 * @brief Output of `TARGRetrieval::gate()`, describing the gating decision.
 */
struct TARGDecision {
    bool   should_retrieve   = false;  ///< True → trigger retrieval now
    float  logit_gap         = 0.0f;   ///< top-1 minus top-2 logit
    float  entropy           = 0.0f;   ///< entropy of top-K mass (0 if not computed)
    bool   in_cooldown       = false;  ///< True → suppressed by cooldown
    std::size_t consecutive_uncertain = 0; ///< Running uncertain-token count
};

// ============================================================================
// TARGRetrieval — stateful gating logic
// ============================================================================

/**
 * @brief Stateful TARG gate: decides per-token whether retrieval is needed.
 *
 * Maintains per-session state (consecutive uncertainty count, cooldown
 * counter).  Create one instance per generation session and call
 * `gate()` or `shouldRetrieve()` on each generated token.
 */
class TARGRetrieval {
public:
    explicit TARGRetrieval(TARGConfig cfg = {});

    // ─── Primary API ─────────────────────────────────────────────────────

    /**
     * @brief Full gating decision for a raw logit vector.
     *
     * @param logits  Raw (un-softmaxed) model logits over the full vocabulary.
     *                Must have at least 2 elements.
     * @return TARGDecision with all gating metrics.
     */
    [[nodiscard]] TARGDecision gate(const std::vector<float>& logits);

    /**
     * @brief Convenience wrapper — returns true if retrieval should occur.
     *
     * Equivalent to `gate(logits).should_retrieve`.
     *
     * @param logits  Raw model logits (vocabulary-size vector).
     * @return        true if retrieval is recommended.
     */
    [[nodiscard]] bool shouldRetrieve(const std::vector<float>& logits);

    // ─── FullEntropyFn bridge (STUB #262) ─────────────────────────────────

    /**
     * @brief Injectable full-vocabulary entropy computation function.
     *
     * When set via `setFullEntropyFn()`, the entropy gate uses the injected
     * function instead of the top-32 logit approximation.  This allows
     * plugging in a full-vocabulary softmax-entropy computation at runtime.
     *
     * Signature: `float fn(const std::vector<float>& logits)` returning
     * the Shannon entropy in nats.
     */
    using FullEntropyFn = std::function<float(const std::vector<float>&)>;

    /**
     * @brief Inject a full-vocabulary entropy computation function.
     *
     * Once set, `gate()` will call @p fn instead of the built-in top-32
     * approximation whenever `TARGConfig::use_entropy_gate == true`.
     * Pass a null / default-constructed `FullEntropyFn` to revert to the
     * built-in approximation.
     *
     * @param fn  Callable accepting raw logits and returning entropy in nats.
     */
    static void setFullEntropyFn(FullEntropyFn fn);

    /** @brief Remove a previously injected FullEntropyFn. */
    static void clearFullEntropyFn();

    // ─── State management ─────────────────────────────────────────────────

    /**
     * @brief Notify the gate that a retrieval was successfully executed.
     *
     * Resets the consecutive-uncertain counter and starts the cool-down
     * timer.  Call this immediately after performing retrieval.
     */
    void notifyRetrievalExecuted();

    /**
     * @brief Notify the gate that one token was emitted (advances cooldown).
     *
     * Call once per generated token, regardless of gating decision.
     */
    void notifyTokenEmitted();

    /**
     * @brief Reset all stateful counters to initial values.
     *
     * Use between independent generation sessions.
     */
    void reset();

    // ─── Diagnostics ──────────────────────────────────────────────────────

    struct GateStats {
        std::size_t tokens_seen        = 0;
        std::size_t retrieval_triggers = 0;
        std::size_t cooldown_skips     = 0;
        double      mean_gap           = 0.0;
        float       min_gap            = 1e9f;
        float       max_gap            = -1e9f;

        /// Fraction of tokens that triggered retrieval.
        [[nodiscard]] double triggerRate() const noexcept {
            return tokens_seen > 0
                ? static_cast<double>(retrieval_triggers) / tokens_seen
                : 0.0;
        }
    };

    /**
     * @brief Cumulative statistics since construction (or last reset()).
     */
    [[nodiscard]] GateStats stats() const noexcept { return stats_; }

private:
    TARGConfig cfg_;

    std::size_t consecutive_uncertain_ = 0;
    std::size_t cooldown_remaining_    = 0;

    GateStats stats_;

    /// Compute the logit gap and optional entropy from raw logits.
    static void computeMetrics(const std::vector<float>& logits,
                                float&  out_gap,
                                float&  out_entropy,
                                bool    compute_entropy);
};

} // namespace rag
} // namespace themis
