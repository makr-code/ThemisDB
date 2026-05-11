/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            rag/tensor_rag_pipeline.h                          ║
  Version:         1.0.0                                              ║
  Last Modified:   2026-05-06                                         ║
  Author:          copilot                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: 🟡 EXPERIMENTAL — Phase 3 (Q1–Q2 2027)                      ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file rag/tensor_rag_pipeline.h
 * @brief TensorRAGPipeline — unified FLARE + TARG coordinator for Phase 3.
 *
 * ## Motivation
 *
 * `FlareRetrieval` and `TARGRetrieval` are independent gating mechanisms that
 * serve complementary roles in a TT-core-backed RAG generation loop:
 *
 * | Gate  | Signal         | Question answered                         |
 * |-------|----------------|-------------------------------------------|
 * | TARG  | Logit gap      | *Should* the model retrieve now?          |
 * | FLARE | Log-probability| *How* to retrieve mid-generation?         |
 *
 * `TensorRAGPipeline` composes both gates into a single per-token `step()`
 * call.  The caller only has to handle one return value (`RAGDecision`) and
 * one state-reset call (`notifyRetrievalDone()`).
 *
 * ## Combination semantics
 *
 * Both gates are evaluated independently.  `RAGDecision::should_retrieve`
 * is set to `true` when *either* gate fires (logical OR), allowing the
 * caller to handle the event with a single conditional.
 *
 * The `trigger` field disambiguates which gate(s) fired, so the caller can
 * tailor the retrieval strategy (e.g., use FLARE's masked query string when
 * only FLARE fires, or apply a tighter budget when only TARG fires).
 *
 * ## Typical usage
 *
 * ```cpp
 * TensorRAGPipelineConfig cfg;
 * cfg.use_flare = true;
 * cfg.use_targ  = true;
 * cfg.flare_config.confidence_threshold = -2.303f;
 * cfg.targ_config.gap_threshold         = 5.0f;
 *
 * TensorRAGPipeline pipeline(cfg);
 *
 * for (auto& tok : generation_loop) {
 *     auto logits   = lm.getLogits();
 *     auto log_prob = lm.tokenLogProb(tok);
 *
 *     auto decision = pipeline.step(tok.text, log_prob, logits);
 *
 *     if (decision.should_retrieve) {
 *         std::string query = decision.flare_triggered
 *                             ? decision.flare_query   // masked FLARE query
 *                             : tok.text;              // fallback: current token
 *         auto results = tensor_index.searchFlat(embed(query), dim, top_k);
 *         lm.injectContext(results);
 *         pipeline.notifyRetrievalDone();
 *     }
 * }
 *
 * auto s = pipeline.stats();
 * // s.flare_triggers, s.targ_triggers, s.combined_triggers
 * ```
 *
 * ## Thread safety
 *
 * Not thread-safe.  Use one instance per generation thread.
 *
 * ## References
 * - Jiang et al. (2023). Active Retrieval Augmented Generation.
 *   arXiv:2305.06983 (FLARE).
 * - paper §TARG: ThemisDB Research Group (2026). Internal pre-print.
 * - paper §Zero-Copy RAG table: TTFT per step ≤ 90ms with TT-core index.
 */

#pragma once

#include "rag/flare_retrieval.h"
#include "rag/targ_retrieval.h"

#include <cstddef>
#include <functional>
#include <mutex>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ============================================================================
// TensorRAGPipelineConfig
// ============================================================================

/**
 * @brief Configuration for the unified FLARE+TARG pipeline.
 */
struct TensorRAGPipelineConfig {
    /**
     * @brief FLARE log-probability gating parameters.
     *
     * Only used when `use_flare = true`.
     */
    FlareConfig flare_config;

    /**
     * @brief TARG logit-gap gating parameters.
     *
     * Only used when `use_targ = true`.
     */
    TARGConfig targ_config;

    /**
     * @brief Enable the FLARE mid-generation retrieval gate.
     *
     * When false, the FLARE gate is bypassed entirely and does not consume
     * any per-token log-probability signal.
     *
     * Default: true.
     */
    bool use_flare = true;

    /**
     * @brief Enable the TARG logit-gap gate.
     *
     * When false, the TARG gate is bypassed entirely and raw logits are not
     * processed.
     *
     * Default: true.
     */
    bool use_targ = true;

    /**
     * @brief Optional session identifier for logging and diagnostics.
     *
     * Not used by pipeline logic; propagated to stats for tracing.
     */
    std::string session_id;
};

// ============================================================================
// RAGDecision
// ============================================================================

/**
 * @brief Combined gating decision returned by `TensorRAGPipeline::step()`.
 */
struct RAGDecision {
    /**
     * @brief Distinguishes which gate(s) triggered retrieval.
     */
    enum class Trigger {
        NONE,        ///< No retrieval needed
        FLARE_ONLY,  ///< Only FLARE fired
        TARG_ONLY,   ///< Only TARG fired
        BOTH         ///< Both gates fired simultaneously
    };

    // ─── Primary decision ─────────────────────────────────────────────────

    /**
     * @brief True if at least one gate recommends retrieval.
     *
     * Equivalent to `(flare_triggered || targ_triggered)`.
     */
    bool    should_retrieve = false;

    /**
     * @brief Which gate(s) caused `should_retrieve = true`.
     */
    Trigger trigger         = Trigger::NONE;

    // ─── FLARE details ────────────────────────────────────────────────────

    /**
     * @brief True if the FLARE gate fired on this token.
     */
    bool        flare_triggered = false;

    /**
     * @brief Log-probability of the current token (from FLARE internal state).
     *
     * Always populated when `use_flare = true`.  Zero when FLARE is disabled.
     */
    float       flare_log_prob  = 0.0f;

    /**
     * @brief Query string built by FLARE when it triggered retrieval.
     *
     * Populated only when `flare_triggered = true`.
     * Contains the masked partial-output window (per FlareConfig).
     *
     * @note This is a text query (STUB #261).  Embedding it before passing to
     *       the TT-core index is the caller's responsibility until Phase 3-C
     *       wires the embedding backend into TensorRAGPipeline via
     *       `setEmbeddingQueryFn()`.
     */
    std::string flare_query;

    /**
     * @brief Embedding vector for `flare_query`.
     *
     * Populated only when `flare_triggered = true` AND an `EmbeddingQueryFn`
     * has been injected via `TensorRAGPipeline::setEmbeddingQueryFn()`.
     * Empty otherwise.  Pass directly to `tensor_index.searchFlat()`.
     */
    std::vector<float> flare_query_embedding;

    // ─── TARG details ─────────────────────────────────────────────────────

    /**
     * @brief True if the TARG gate fired on this token.
     */
    bool  targ_triggered = false;

    /**
     * @brief Logit gap (top-1 minus top-2) computed by TARG for this token.
     *
     * Always populated when `use_targ = true`.  Zero when TARG is disabled.
     */
    float targ_gap       = 0.0f;
};

// ============================================================================
// TensorRAGPipeline
// ============================================================================

/**
 * @brief Unified FLARE + TARG coordinator for TT-core-backed RAG sessions.
 *
 * Composes one `FlareRetrieval` and one `TARGRetrieval` instance under a
 * single per-token API.  Both gates are evaluated independently on every
 * call to `step()`; retrieval is recommended when either gate fires.
 */
class TensorRAGPipeline {
public:
    /**
     * @brief Construct a pipeline with the given configuration.
     *
     * The FLARE and TARG sub-gates are initialised from `cfg.flare_config`
     * and `cfg.targ_config` respectively.
     *
     * @param cfg  Pipeline configuration.
     */
    explicit TensorRAGPipeline(TensorRAGPipelineConfig cfg = {});

    // ─── Embedding injection bridge (STUB #261) ───────────────────────────

    /**
     * @brief Callable type for a text-to-embedding backend.
     *
     * Signature: `std::vector<float> embed(const std::string& text)`
     *
     * When set via `setEmbeddingQueryFn()`, `step()` will populate
     * `RAGDecision::flare_query_embedding` whenever FLARE triggers.
     * The fn must be thread-safe; it is called under no internal lock.
     */
    using EmbeddingQueryFn = std::function<std::vector<float>(const std::string&)>;

    /**
     * @brief Inject an embedding backend (thread-safe, process-global).
     *
     * Once set, `step()` will call @p fn on the FLARE query string and
     * store the result in `RAGDecision::flare_query_embedding`.
     * Pass a null fn to revert to the no-embedding fallback.
     *
     * @param fn  Embedding function, e.g. wrapping a quantised SBERT encoder.
     */
    static void setEmbeddingQueryFn(EmbeddingQueryFn fn);

    // ─── Primary API ─────────────────────────────────────────────────────

    /**
     * @brief Evaluate both gates for a newly emitted token.
     *
     * Must be called once per generated token in the generation loop.
     * Order of evaluation: TARG first (logit-gap), then FLARE (log-prob
     * window update and decision).
     *
     * @param token_text  Surface form of the emitted token.
     *                    Used by FLARE's `buildQuery()` for the sliding window.
     * @param log_prob    Natural-log probability of this token: log(p(token)).
     *                    Must be ≤ 0.  Used by FLARE only.
     * @param logits      Raw (un-softmaxed) model logits over the vocabulary.
     *                    Must have at least 2 elements.  Used by TARG only.
     *
     * @return RAGDecision with combined gating result and per-gate details.
     */
    [[nodiscard]] RAGDecision step(const std::string&        token_text,
                                   float                     log_prob,
                                   const std::vector<float>& logits);

    /**
     * @brief Notify the pipeline that retrieval was successfully executed.
     *
     * Propagates `notifyRetrievalExecuted()` / `notifyRetrievalDone()` to
     * both sub-gates and increments the combined retrieval counter.
     *
     * Call this immediately after completing the retrieval action (before
     * regenerating the uncertain span).
     */
    void notifyRetrievalDone();

    /**
     * @brief Reset all session state in both sub-gates.
     *
     * Use between independent generation requests on the same pipeline
     * instance.  Does not alter the configuration.
     */
    void reset();

    // ─── Sub-gate access ─────────────────────────────────────────────────

    /**
     * @brief Access the underlying FLARE gate for advanced inspection.
     */
    [[nodiscard]] FlareRetrieval&       flare() noexcept       { return flare_; }
    [[nodiscard]] const FlareRetrieval& flare() const noexcept { return flare_; }

    /**
     * @brief Access the underlying TARG gate for advanced inspection.
     */
    [[nodiscard]] TARGRetrieval&       targ() noexcept       { return targ_; }
    [[nodiscard]] const TARGRetrieval& targ() const noexcept { return targ_; }

    // ─── Diagnostics ──────────────────────────────────────────────────────

    /**
     * @brief Aggregate statistics accumulated since construction or last reset().
     */
    struct PipelineStats {
        std::size_t total_token_steps = 0;  ///< Total calls to step()
        std::size_t flare_triggers    = 0;  ///< Times FLARE fired
        std::size_t targ_triggers     = 0;  ///< Times TARG fired
        std::size_t combined_triggers = 0;  ///< Times both fired simultaneously
        std::size_t total_retrievals  = 0;  ///< Times notifyRetrievalDone() called

        /// Fraction of tokens that triggered at least one gate.
        [[nodiscard]] double anyTriggerRate() const noexcept {
            if (total_token_steps == 0) return 0.0;
            const auto any = flare_triggers + targ_triggers - combined_triggers;
            return static_cast<double>(any) / total_token_steps;
        }
    };

    /**
     * @brief Return a snapshot of accumulated statistics.
     */
    [[nodiscard]] PipelineStats stats() const noexcept { return stats_; }

    /**
     * @brief Return the configuration this pipeline was constructed with.
     */
    [[nodiscard]] const TensorRAGPipelineConfig& config() const noexcept {
        return cfg_;
    }

private:
    TensorRAGPipelineConfig cfg_;
    FlareRetrieval          flare_;
    TARGRetrieval           targ_;
    PipelineStats           stats_;
};

} // namespace rag
} // namespace themis
