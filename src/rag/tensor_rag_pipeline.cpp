/**
 * @file tensor_rag_pipeline.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=5; TODO=1, Stub=3, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=1, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#include "rag/tensor_rag_pipeline.h"
#include <stdexcept>

#include <cstdio>
#include <mutex>

namespace themis {
namespace rag {

// ============================================================================
// EmbeddingQueryFn injection bridge
// ============================================================================

static std::mutex& pipelineEmbedFnMutex() { static std::mutex m; return m; }
static TensorRAGPipeline::EmbeddingQueryFn& pipelineEmbedFnStorage() {
    static TensorRAGPipeline::EmbeddingQueryFn fn;
    return fn;
}

/*static*/
void TensorRAGPipeline::setEmbeddingQueryFn(EmbeddingQueryFn fn) {
    std::lock_guard<std::mutex> lk(pipelineEmbedFnMutex());
    pipelineEmbedFnStorage() = std::move(fn);
}

// ============================================================================
// Construction
// ============================================================================

TensorRAGPipeline::TensorRAGPipeline(TensorRAGPipelineConfig cfg)
    : cfg_(std::move(cfg))
    , flare_(cfg_.flare_config)
    , targ_(cfg_.targ_config)
    , stats_{}
{}

void TensorRAGPipeline::setTensorMidLayer(
    std::shared_ptr<tensor::TensorMidLayer> tensor_mid_layer) {
    tensor_mid_layer_ = std::move(tensor_mid_layer);
}

void TensorRAGPipeline::setGraphTruthValidator(
    std::shared_ptr<GraphTruthValidator> graph_truth_validator) {
    graph_truth_validator_ = std::move(graph_truth_validator);
}

void TensorRAGPipeline::setFinalLayerOrchestrator(
    std::shared_ptr<llm::FinalLayerOrchestrator> final_layer_orchestrator) {
    final_layer_orchestrator_ = std::move(final_layer_orchestrator);
}

// ============================================================================
// step() — core per-token evaluation
// ============================================================================

RAGDecision TensorRAGPipeline::step(const std::string&        token_text,
                                    float                     log_prob,
                                    const std::vector<float>& logits)
{
    ++stats_.total_token_steps;

    RAGDecision decision;
    bool        targ_fired  = false;
    bool        flare_fired = false;

    // ── 1. TARG gate (logit-gap) ─────────────────────────────────────────
    if (cfg_.use_targ) {
        const TARGDecision td = targ_.gate(logits);
        targ_.notifyTokenEmitted();  // advance cooldown regardless of gate decision
        decision.targ_gap = td.logit_gap;
        if (td.should_retrieve) {
            targ_fired             = true;
            decision.targ_triggered = true;
            ++stats_.targ_triggers;
        }
    }

    // ── 2. FLARE gate (log-probability window) ───────────────────────────
    if (cfg_.use_flare) {
        flare_.notifyTokenEmitted(token_text, log_prob);
        const FlareDecision fd = flare_.decide();
        decision.flare_log_prob = fd.last_log_prob;
        if (fd.should_retrieve) {
            flare_fired             = true;
            decision.flare_triggered = true;
            decision.flare_query     = flare_.buildQuery();
            // Populate embedding vector when a backend is wired.
            {
                std::lock_guard<std::mutex> lk(pipelineEmbedFnMutex());
                const auto& efn = pipelineEmbedFnStorage();
                if (efn) {
                    try {
                        decision.flare_query_embedding = efn(decision.flare_query);
                    } catch (...) {
                        // Fail-closed: embedding fn threw; leave embedding empty.
                        // Distinct from "no fn wired" — the backend is registered but
                        // failed at runtime. Operators should diagnose the root cause.
                        std::fprintf(stderr,
                            "[ThemisDB][WARN] TensorRAGPipeline::step: EmbeddingQueryFn "
                            "threw for FLARE query (len=%zu); embedding left empty "
                            "(fail-closed).\n",
                            decision.flare_query.size());
                        decision.flare_query_embedding.clear();
                    }
                }
            }
            ++stats_.flare_triggers;
        }
    }

    // ── 3. Combined decision ─────────────────────────────────────────────
    decision.should_retrieve = (targ_fired || flare_fired);

    if (targ_fired && flare_fired) {
        decision.trigger = RAGDecision::Trigger::BOTH;
        ++stats_.combined_triggers;
    } else if (flare_fired) {
        decision.trigger = RAGDecision::Trigger::FLARE_ONLY;
    } else if (targ_fired) {
        decision.trigger = RAGDecision::Trigger::TARG_ONLY;
    } else {
        decision.trigger = RAGDecision::Trigger::NONE;
    }

    if (decision.should_retrieve && tensor_mid_layer_) {
        tensor::TensorLayerContext tensor_context;
        tensor_context.tenant_id = cfg_.session_id;
        tensor_context.scope_id = cfg_.session_id.empty() ? "__tensor_rag_pipeline__" : cfg_.session_id;
        tensor_context.top_k = 5;
        tensor_context.use_fingerprint_summary = true;

        const auto summary = tensor_mid_layer_->summarize(tensor_context);
        decision.tensor_summary_candidates = summary.similar_adapters;
        decision.tensor_routing_reason = summary.routing_reason;

        if (graph_truth_validator_) {
            const auto graph_validation = graph_truth_validator_->validate(
                decision.flare_query.empty() ? token_text : decision.flare_query,
                summary);
            decision.graph_truth_evidences = graph_validation.evidences;
            decision.graph_truth_reason = graph_validation.routing_reason;
        }

        if (final_layer_orchestrator_) {
            llm::FinalLayerRequest request;
            request.prompt = decision.flare_query.empty() ? token_text : decision.flare_query;
            request.metadata = cfg_.final_layer_metadata;
            request.requested_package_id = cfg_.final_layer_package_id;
            request.base_model_name = cfg_.final_layer_base_model;
            request.base_model_version = cfg_.final_layer_base_model_version;
            request.allow_draft_adapter = true;
            decision.final_layer_resolution = final_layer_orchestrator_->resolve(request);
        }
    }

    return decision;
}

// ============================================================================
// notifyRetrievalDone()
// ============================================================================

void TensorRAGPipeline::notifyRetrievalDone()
{
    if (cfg_.use_flare) {
        flare_.notifyRetrievalExecuted();
    }
    if (cfg_.use_targ) {
        targ_.notifyRetrievalExecuted();
    }
    ++stats_.total_retrievals;
}

// ============================================================================
// reset()
// ============================================================================

void TensorRAGPipeline::reset()
{
    flare_.reset();
    targ_.reset();
    stats_ = {};
}

} // namespace rag
} // namespace themis

