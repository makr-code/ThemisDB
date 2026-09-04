/**
 * @file tensor_rag_pipeline.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 94/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "rag/flare_retrieval.h"
#include "rag/graph_truth_validator.h"
#include "rag/targ_retrieval.h"
#include "llm/final_layer_orchestrator.h"
#include "observability/provenance_store.h"
#include "observability/retrieval_provenance.h"
#include "tensor/tensor_mid_layer.h"

#include <cstddef>
#include <functional>
#include <memory>
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

    /**
     * @brief Optional final-layer package to resolve when retrieval triggers.
     */
    std::string final_layer_package_id;

    /**
     * @brief Optional base model name used for final-layer compatibility checks.
     */
    std::string final_layer_base_model;

    /**
     * @brief Optional base model version used for final-layer compatibility checks.
     */
    std::string final_layer_base_model_version;

    /**
     * @brief Optional metadata forwarded to the final-layer orchestrator.
     */
    nlohmann::json final_layer_metadata = nlohmann::json::object();

    /**
     * @brief Optional persistent provenance store for per-step lineage records.
     *
     * When configured, step() persists one ProvenanceStepRecord per call,
     * allowing post-generation chain queryability by query_id.
     */
    std::shared_ptr<observability::IProvenanceStore> provenance_store;
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

    /**
     * @brief Cross-layer fallback mode selected for this decision.
     */
    enum class FallbackMode {
        NONE,              ///< No fallback required
        DEGRADED_CONTINUE, ///< Continue execution with reduced guarantees
        FAIL_CLOSED        ///< Stop sensitive path due to policy/safety violation
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

    /**
     * @brief Stable correlation identifier propagated across layer handoffs.
     */
    std::string correlation_id;

    /**
     * @brief Machine-readable reason code for the selected routing branch.
     */
    std::string routing_reason_code;

    /**
     * @brief Version tag of the confidence policy used for this decision.
     */
    std::string confidence_policy_version;

    /**
     * @brief Confidence threshold key that governed this decision.
     */
    std::string confidence_threshold_key;

    /**
     * @brief Selected fallback behavior for this decision.
     */
    FallbackMode fallback_mode = FallbackMode::NONE;

    /**
     * @brief Machine-readable reason code when a fallback mode was applied.
     */
    std::string fallback_reason_code;

    /**
     * @brief Source layer that escalated this request, if any.
     */
    std::string escalation_source_layer;

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
    std::string flare_query = {};

    /**
     * @brief Embedding vector for `flare_query`.
     *
     * Populated only when `flare_triggered = true` AND an `EmbeddingQueryFn`
     * has been injected via `TensorRAGPipeline::setEmbeddingQueryFn()`.
     * Empty otherwise.  Pass directly to `tensor_index.searchFlat()`.
     */
    std::vector<float> flare_query_embedding;

    /**
     * @brief Optional tensor mid-layer summary generated for this decision.
     *
     * Populated when a TensorMidLayer is attached and at least one gate
     * triggered retrieval.
     */
    std::vector<tensor::SimilarityResult> tensor_summary_candidates;

    /**
     * @brief Human-readable tensor-layer routing explanation.
     */
    std::string tensor_routing_reason;

    /**
     * @brief Graph-truth evidences attached after tensor summary validation.
     */
    std::vector<GraphTruthEvidence> graph_truth_evidences;

    /**
     * @brief Human-readable graph-truth routing explanation.
     */
    std::string graph_truth_reason;

    /**
     * @brief Machine-readable graph-truth routing reason code.
     */
    std::string graph_truth_routing_reason_code;

    /**
     * @brief Correlation id propagated by graph-truth validation.
     */
    std::string graph_truth_correlation_id;

    /**
     * @brief Final-layer orchestration result when an LLM/LoRA package was resolved.
     */
    llm::FinalLayerResolution final_layer_resolution;

    /**
     * @brief Unified end-to-end provenance record for this retrieval decision.
     *
     * Populated by step() when at least one gate fires. Can be exported for
     * audit, diagnostics, and provenance dashboards.
     */
    observability::RetrievalProvenanceRecord provenance;

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

    /**
     * @brief Attach a TensorMidLayer used for higher-layer retrieval summaries.
     *
     * When set, `step()` derives a tensor-layer summary after FLARE/TARG
     * triggers and exposes it through `RAGDecision`.
     */
    void setTensorMidLayer(std::shared_ptr<tensor::TensorMidLayer> tensor_mid_layer);

    /**
     * @brief Attach a GraphTruthValidator used after tensor mid-layer summarization.
     */
    void setGraphTruthValidator(std::shared_ptr<GraphTruthValidator> graph_truth_validator);

    /**
     * @brief Attach the final LLM/LoRA orchestration layer.
     */
    void setFinalLayerOrchestrator(std::shared_ptr<llm::FinalLayerOrchestrator> final_layer_orchestrator);

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
            if (total_token_steps == 0) {
              return 0.0;
            }
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
    std::shared_ptr<tensor::TensorMidLayer> tensor_mid_layer_;
    std::shared_ptr<GraphTruthValidator> graph_truth_validator_;
    std::shared_ptr<llm::FinalLayerOrchestrator> final_layer_orchestrator_;
    std::size_t step_sequence_ = 0;
};

} // namespace rag
} // namespace themis
