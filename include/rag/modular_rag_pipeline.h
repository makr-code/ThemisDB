/**
 * @file modular_rag_pipeline.h
 * @brief ModularRAGPipeline — explicit five-stage RAG orchestration with
 *        provenance propagation and per-stage telemetry.
 *
 * ## Motivation (P2.1)
 *
 * The existing `TensorRAGPipeline` couples FLARE/TARG gating directly into a
 * single orchestration class.  For complex multi-hop queries, cross-domain
 * retrieval, and multi-tenant deployments the pipeline needs to be decomposed
 * into clearly separated, independently observable stages:
 *
 *   1. **Retrieve**   — ANN / BM25 / graph-walk candidate fetch
 *   2. **Rerank**     — cross-encoder or learned scoring
 *   3. **Validate**   — graph truth check + provenance tagging
 *   4. **Assemble**   — prompt builder with token-budget enforcement
 *   5. **Generate**   — LLM inference with policy enforcement
 *
 * Each stage receives a `ModularRAGContext` that accumulates provenance,
 * trace IDs, and per-stage timing.  No stage may silently swallow errors; the
 * pipeline is fail-closed by default.
 *
 * ## Provenance contract
 *
 * Every stage MUST annotate the context with its source identifier
 * (`stage_provenance`).  The final `generate()` stage captures the provenance
 * chain in `ModularRAGResult::provenance_chain`, enabling post-hoc auditing of
 * which documents contributed to which answer tokens.
 *
 * ## Thread safety
 *
 * `ModularRAGPipeline` objects are NOT thread-safe.  Create one instance per
 * request (or per coroutine/async task) and discard after use.
 *
 * @see include/rag/tensor_rag_pipeline.h — lower-level FLARE/TARG pipeline
 * @see include/llm/llm_correlation_context.h — W3C correlation context
 * @see include/llm/llm_generate_operator.h — query-plan LLM operator
 */

#pragma once

#include "llm/llm_correlation_context.h"
#include "llm/llm_plugin_interface.h"

#include <chrono>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace themis {
namespace rag {

// ============================================================================
// Stage identifiers
// ============================================================================

/**
 * @brief Named stages in the ModularRAGPipeline execution order.
 *
 * Stages are executed in enum-value order.  A stage may be skipped by
 * returning `StageResult::status == StageStatus::Skipped`.
 */
enum class RAGStageId : uint8_t {
    Retrieve  = 0, ///< Candidate document fetch (ANN / BM25 / graph walk)
    Rerank    = 1, ///< Cross-encoder or learned scoring
    Validate  = 2, ///< Graph truth verification and provenance tagging
    Assemble  = 3, ///< Prompt construction with token-budget enforcement
    Generate  = 4, ///< LLM inference with policy enforcement
};

/** @return Human-readable stage name for logging. */
[[nodiscard]] inline const char* ragStageIdName(RAGStageId id) noexcept {
    switch (id) {
        case RAGStageId::Retrieve: return "Retrieve";
        case RAGStageId::Rerank:   return "Rerank";
        case RAGStageId::Validate: return "Validate";
        case RAGStageId::Assemble: return "Assemble";
        case RAGStageId::Generate: return "Generate";
    }
    return "Unknown";
}

// ============================================================================
// Candidate document (shared across stages)
// ============================================================================

/**
 * @brief A single retrieved document candidate.
 *
 * Each stage may update `score` and append to `provenance_tags`.
 */
struct RAGCandidate {
    std::string doc_id;            ///< Opaque document identifier
    std::string content;           ///< Document text or excerpt
    float       score = 0.0f;      ///< Retrieval / rerank score (higher = more relevant)
    std::string source_namespace;  ///< Originating index namespace (e.g. "legal_cases")

    /// Free-form tags appended by each stage (e.g. "retrieve:bm25", "validate:graph_ok").
    std::vector<std::string> provenance_tags;
};

// ============================================================================
// Per-stage result
// ============================================================================

/**
 * @brief Outcome of a single pipeline stage.
 */
enum class StageStatus : uint8_t {
    Success,  ///< Stage completed normally
    Skipped,  ///< Stage was intentionally bypassed (e.g. reranker not configured)
    Denied,   ///< Stage blocked the request (policy violation, budget exceeded)
    Error,    ///< Stage encountered an unrecoverable error
};

/**
 * @brief Result produced by a single pipeline stage.
 */
struct StageResult {
    RAGStageId stage;
    StageStatus status = StageStatus::Success;

    /// Updated candidate list.  Must be set by Retrieve, Rerank, and Validate.
    std::vector<RAGCandidate> candidates;

    /// Assembled prompt text.  Must be set by Assemble.
    std::optional<std::string> assembled_prompt;

    /// Inference response.  Must be set by Generate.
    std::optional<llm::InferenceResponse> inference_response;

    /// Human-readable diagnostic (logged on non-Success status).
    std::string diagnostic;

    /// Wall-clock duration of this stage.
    std::chrono::microseconds elapsed{0};
};

// ============================================================================
// Mutable pipeline context (passed by reference through all stages)
// ============================================================================

/**
 * @brief Accumulated state for a single pipeline invocation.
 *
 * Passed by reference to each stage handler.  Stages READ the current state
 * and WRITE their output into the appropriate fields.
 */
struct ModularRAGContext {
    // --- Input ---
    std::string query;          ///< Original user query text
    std::string tenant_id;      ///< Tenant identifier for isolation
    llm::LLMCorrelationContext correlation; ///< W3C trace context

    // --- Retrieval budget ---
    std::size_t max_candidates = 20; ///< Maximum candidates after Retrieve
    std::size_t top_k_after_rerank = 5; ///< Candidates forwarded past Rerank

    // --- Token budget ---
    std::size_t max_context_tokens = 4096; ///< Hard cap for Assemble

    // --- Provenance chain ---
    /// Ordered list of source annotations from each stage.
    std::vector<std::string> provenance_chain;

    // --- Stage outputs (set progressively) ---
    std::vector<RAGCandidate> candidates;
    std::optional<std::string> assembled_prompt;
    std::optional<llm::InferenceResponse> inference_response;

    // --- Timing ---
    std::vector<StageResult> stage_results; ///< One entry per executed stage
    std::chrono::steady_clock::time_point pipeline_start{
        std::chrono::steady_clock::now()};

    // --- Helpers ---

    /** @return Total elapsed time since pipeline_start. */
    [[nodiscard]] std::chrono::microseconds totalElapsed() const noexcept {
        return std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - pipeline_start);
    }

    /**
     * @brief Append a provenance annotation.
     * @param stage     Stage that produced this annotation.
     * @param note      Short human-readable note.
     */
    void addProvenance(RAGStageId stage, std::string note) {
        provenance_chain.push_back(
            std::string(ragStageIdName(stage)) + ":" + std::move(note));
    }
};

// ============================================================================
// Final pipeline result
// ============================================================================

/**
 * @brief The final output of a successful pipeline invocation.
 */
struct ModularRAGResult {
    bool        success = false;         ///< True iff Generate stage succeeded
    std::string answer;                  ///< Generated answer text
    std::string error_message;           ///< Non-empty on failure

    std::vector<RAGCandidate> candidates; ///< Final ranked candidates used

    /// Ordered provenance chain across all stages.
    std::vector<std::string> provenance_chain;

    /// Per-stage timing records.
    std::vector<StageResult> stage_results;

    /// W3C trace context echoed from the request.
    llm::LLMCorrelationContext correlation;

    /// Total pipeline wall time.
    std::chrono::microseconds total_elapsed{0};
};

// ============================================================================
// Stage handler function signature
// ============================================================================

/**
 * @brief Function signature for a single pipeline stage handler.
 *
 * @param ctx  Mutable pipeline context (read previous stage outputs, write your own).
 * @return     StageResult describing this stage's outcome.
 *
 * Returning `StageStatus::Error` or `StageStatus::Denied` causes the pipeline
 * to halt and return a failed `ModularRAGResult`.
 */
using RAGStageHandler = std::function<StageResult(ModularRAGContext&)>;

// ============================================================================
// Pipeline configuration
// ============================================================================

/**
 * @brief Configuration bundle for ModularRAGPipeline.
 */
struct ModularRAGPipelineConfig {
    /**
     * @brief Stage handlers in execution order.
     *
     * All five handlers MUST be provided.  A stage may be a pass-through
     * (returning `StageStatus::Skipped`) but must not be null.
     */
    RAGStageHandler retrieve_fn;  ///< Stage 1: candidate fetch
    RAGStageHandler rerank_fn;    ///< Stage 2: cross-encoder rerank
    RAGStageHandler validate_fn;  ///< Stage 3: graph-truth validation
    RAGStageHandler assemble_fn;  ///< Stage 4: prompt assembly
    RAGStageHandler generate_fn;  ///< Stage 5: LLM inference

    /**
     * @brief Whether to halt immediately on a `StageStatus::Error` result.
     *
     * When `true` (default) the pipeline is fail-closed and returns a failed
     * result with the stage diagnostic.  When `false`, the pipeline attempts
     * to continue to the next stage (useful for degraded-mode testing).
     */
    bool fail_closed = true;
};

// ============================================================================
// ModularRAGPipeline
// ============================================================================

/**
 * @brief Orchestrates a five-stage RAG pipeline with provenance propagation.
 *
 * Create one instance per request.  Call `run()` with the initial query and
 * tenant context.  Each stage is invoked in order; the pipeline halts on the
 * first error (when `config.fail_closed == true`).
 *
 * ## Usage example
 *
 * ```cpp
 * ModularRAGPipelineConfig cfg;
 * cfg.retrieve_fn = myANNRetriever;
 * cfg.rerank_fn   = myCrossEncoder;
 * cfg.validate_fn = myGraphValidator;
 * cfg.assemble_fn = myPromptBuilder;
 * cfg.generate_fn = myLLMInvoker;
 *
 * ModularRAGPipeline pipeline{cfg};
 * ModularRAGResult result = pipeline.run("What is the statute of limitations?",
 *                                        "tenant-42", correlation_ctx);
 * if (!result.success) { handle_error(result.error_message); }
 * ```
 */
class ModularRAGPipeline {
public:
    /**
     * @brief Construct a pipeline with the given configuration.
     * @throws std::invalid_argument if any stage handler is null.
     */
    explicit ModularRAGPipeline(ModularRAGPipelineConfig config)
        : config_(std::move(config))
    {
        if (!config_.retrieve_fn) throw std::invalid_argument("retrieve_fn must not be null");
        if (!config_.rerank_fn)   throw std::invalid_argument("rerank_fn must not be null");
        if (!config_.validate_fn) throw std::invalid_argument("validate_fn must not be null");
        if (!config_.assemble_fn) throw std::invalid_argument("assemble_fn must not be null");
        if (!config_.generate_fn) throw std::invalid_argument("generate_fn must not be null");
    }

    ModularRAGPipeline(const ModularRAGPipeline&) = delete;
    ModularRAGPipeline& operator=(const ModularRAGPipeline&) = delete;
    ModularRAGPipeline(ModularRAGPipeline&&) noexcept = default;
    ModularRAGPipeline& operator=(ModularRAGPipeline&&) noexcept = default;
    ~ModularRAGPipeline() = default;

    /**
     * @brief Execute the full pipeline for a single query.
     *
     * @param query          User query text.
     * @param tenant_id      Tenant identifier for isolation.
     * @param correlation    W3C trace context (auto-generated if invalid).
     * @return               Final result after all stages.
     */
    [[nodiscard]] ModularRAGResult run(
        const std::string& query,
        const std::string& tenant_id,
        llm::LLMCorrelationContext correlation = {}) const
    {
        ModularRAGContext ctx;
        ctx.query       = query;
        ctx.tenant_id   = tenant_id;
        ctx.correlation = correlation.ensure();

        const RAGStageHandler* const stages[] = {
            &config_.retrieve_fn,
            &config_.rerank_fn,
            &config_.validate_fn,
            &config_.assemble_fn,
            &config_.generate_fn,
        };
        const RAGStageId stage_ids[] = {
            RAGStageId::Retrieve,
            RAGStageId::Rerank,
            RAGStageId::Validate,
            RAGStageId::Assemble,
            RAGStageId::Generate,
        };

        for (std::size_t i = 0; i < 5; ++i) {
            const auto t0 = std::chrono::steady_clock::now();
            StageResult sr = (*stages[i])(ctx);
            sr.stage   = stage_ids[i];
            sr.elapsed = std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - t0);

            // Propagate stage outputs into context.
            if (!sr.candidates.empty()) {
                ctx.candidates = std::move(sr.candidates);
            }
            if (sr.assembled_prompt.has_value()) {
                ctx.assembled_prompt = std::move(sr.assembled_prompt);
            }
            if (sr.inference_response.has_value()) {
                ctx.inference_response = std::move(sr.inference_response);
            }

            ctx.stage_results.push_back(sr);

            if (sr.status == StageStatus::Error || sr.status == StageStatus::Denied) {
                if (config_.fail_closed) {
                    return buildFailedResult(ctx, sr.diagnostic);
                }
                // Degraded-mode: continue despite error (testing only).
            }
        }

        return buildSuccessResult(ctx);
    }

    /** @return The configuration this pipeline was constructed with. */
    [[nodiscard]] const ModularRAGPipelineConfig& config() const noexcept {
        return config_;
    }

private:
    ModularRAGPipelineConfig config_;

    [[nodiscard]] static ModularRAGResult buildSuccessResult(
        const ModularRAGContext& ctx) noexcept
    {
        ModularRAGResult r;
        r.success          = ctx.inference_response.has_value() &&
                             ctx.inference_response->success;
        if (ctx.inference_response.has_value()) {
            r.answer = ctx.inference_response->text;
            if (!ctx.inference_response->success) {
                r.error_message = ctx.inference_response->error_message;
                r.success = false;
            }
        } else {
            r.error_message = "Generate stage did not produce an inference response";
        }
        r.candidates       = ctx.candidates;
        r.provenance_chain = ctx.provenance_chain;
        r.stage_results    = ctx.stage_results;
        r.correlation      = ctx.correlation;
        r.total_elapsed    = ctx.totalElapsed();
        return r;
    }

    [[nodiscard]] static ModularRAGResult buildFailedResult(
        const ModularRAGContext& ctx,
        const std::string& diagnostic) noexcept
    {
        ModularRAGResult r;
        r.success          = false;
        r.error_message    = diagnostic;
        r.candidates       = ctx.candidates;
        r.provenance_chain = ctx.provenance_chain;
        r.stage_results    = ctx.stage_results;
        r.correlation      = ctx.correlation;
        r.total_elapsed    = ctx.totalElapsed();
        return r;
    }
};

} // namespace rag
} // namespace themis
