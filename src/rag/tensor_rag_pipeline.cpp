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
#include "observability/layer_decision_log.h"
#include "observability/reason_codes.h"
#include "observability/retrieval_provenance.h"
#include "observability/telemetry_keys.h"
#include <stdexcept>

#include <cstdio>
#include <ctime>
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
    decision.correlation_id = cfg_.session_id.empty()
        ? std::string(observability::telemetry::defaults::kTensorRagNoSessionCorrelation)
        : cfg_.session_id;
    decision.confidence_policy_version = std::string(observability::reason_codes::kPolicyVersionDefault);
    decision.confidence_threshold_key = std::string(observability::reason_codes::tensor_rag::kThresholdKeyNone);
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
                    } catch (const std::exception& e) {
                        // Fail-closed: embedding fn threw; leave embedding empty.
                        // Distinct from "no fn wired" — the backend is registered but
                        // failed at runtime. Operators should diagnose the root cause.
                        THEMIS_WARN("TensorRAGPipeline::step: EmbeddingQueryFn threw for FLARE query (len={}): {}",
                                    decision.flare_query.size(), e.what());
                        decision.flare_query_embedding.clear();
                        decision.fallback_mode = RAGDecision::FallbackMode::FAIL_CLOSED;
                        decision.fallback_reason_code = std::string(observability::reason_codes::tensor_rag::kFallbackEmbeddingFnThrow);
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
        decision.routing_reason_code = std::string(observability::reason_codes::tensor_rag::kTriggerBoth);
        decision.confidence_threshold_key = std::string(observability::reason_codes::tensor_rag::kThresholdKeyBoth);
        ++stats_.combined_triggers;
    } else if (flare_fired) {
        decision.trigger = RAGDecision::Trigger::FLARE_ONLY;
        decision.routing_reason_code = std::string(observability::reason_codes::tensor_rag::kTriggerFlare);
        decision.confidence_threshold_key = std::string(observability::reason_codes::tensor_rag::kThresholdKeyFlare);
    } else if (targ_fired) {
        decision.trigger = RAGDecision::Trigger::TARG_ONLY;
        decision.routing_reason_code = std::string(observability::reason_codes::tensor_rag::kTriggerTarg);
        decision.confidence_threshold_key = std::string(observability::reason_codes::tensor_rag::kThresholdKeyTarg);
    } else {
        decision.trigger = RAGDecision::Trigger::NONE;
        decision.routing_reason_code = std::string(observability::reason_codes::tensor_rag::kNoRetrieval);
        decision.confidence_threshold_key = std::string(observability::reason_codes::tensor_rag::kThresholdKeyNone);
    }

    if (decision.should_retrieve && tensor_mid_layer_) {
        decision.escalation_source_layer = std::string(observability::telemetry::layers::kTensor);
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
                summary,
                {},
                decision.correlation_id);
            decision.graph_truth_evidences = graph_validation.evidences;
            decision.graph_truth_reason = graph_validation.routing_reason;
            decision.graph_truth_routing_reason_code = graph_validation.routing_reason_code;
            decision.graph_truth_correlation_id = graph_validation.correlation_id;
            decision.escalation_source_layer = std::string(observability::telemetry::layers::kGraph);
        }

        if (final_layer_orchestrator_) {
            llm::FinalLayerRequest request;
            request.prompt = decision.flare_query.empty() ? token_text : decision.flare_query;
            request.metadata = cfg_.final_layer_metadata;
            request.requested_package_id = cfg_.final_layer_package_id;
            request.base_model_name = cfg_.final_layer_base_model;
            request.base_model_version = cfg_.final_layer_base_model_version;
            request.correlation_id = decision.correlation_id;
            request.confidence_policy_version = decision.confidence_policy_version;
            request.confidence_threshold_key = decision.confidence_threshold_key;
            request.upstream_routing_reason_code = decision.routing_reason_code;
            request.escalation_source_layer = decision.escalation_source_layer;
            request.allow_draft_adapter = true;
            decision.final_layer_resolution = final_layer_orchestrator_->resolve(request);
            decision.escalation_source_layer = std::string(observability::telemetry::layers::kFinalLayer);
            if (!decision.final_layer_resolution.routing_reason_code.empty()) {
                decision.routing_reason_code = decision.final_layer_resolution.routing_reason_code;
            }
            if (decision.fallback_mode == RAGDecision::FallbackMode::NONE) {
                if (decision.final_layer_resolution.fallback_mode == "fail_closed") {
                    decision.fallback_mode = RAGDecision::FallbackMode::FAIL_CLOSED;
                    decision.fallback_reason_code = decision.final_layer_resolution.fallback_reason_code;
                } else if (decision.final_layer_resolution.fallback_mode == "degraded_continue") {
                    decision.fallback_mode = RAGDecision::FallbackMode::DEGRADED_CONTINUE;
                    decision.fallback_reason_code = decision.final_layer_resolution.fallback_reason_code;
                }
            }
        }

        if (decision.tensor_summary_candidates.empty() &&
            decision.fallback_mode == RAGDecision::FallbackMode::NONE) {
            decision.fallback_mode = RAGDecision::FallbackMode::DEGRADED_CONTINUE;
            decision.fallback_reason_code = std::string(observability::reason_codes::tensor_rag::kFallbackTensorSummaryEmpty);
        }
    }

    const auto fallback_mode_sv =
        decision.fallback_mode == RAGDecision::FallbackMode::FAIL_CLOSED
            ? observability::reason_codes::fallback_mode::kFailClosed
            : (decision.fallback_mode == RAGDecision::FallbackMode::DEGRADED_CONTINUE
                ? observability::reason_codes::fallback_mode::kDegradedContinue
                : observability::reason_codes::fallback_mode::kNone);

    observability::emitLayerDecisionLog(
        observability::telemetry::layers::kTensor,
        decision.correlation_id,
        decision.routing_reason_code,
        decision.confidence_policy_version,
        decision.confidence_threshold_key,
        fallback_mode_sv,
        decision.fallback_reason_code,
        decision.escalation_source_layer,
        decision.should_retrieve);

    // ── Build and emit unified provenance record ───────────────────────────
    observability::RetrievalProvenanceRecord prov;
    prov.correlation_id             = decision.correlation_id;
    prov.confidence_policy_version  = decision.confidence_policy_version;
    prov.trigger_reason_code        = decision.routing_reason_code;
    prov.flare_fired                = decision.flare_triggered;
    prov.targ_fired                 = decision.targ_triggered;
    prov.tensor_candidate_count     = decision.tensor_summary_candidates.size();
    prov.tensor_routing_reason      = decision.tensor_routing_reason;
    prov.graph_evidence_count       = decision.graph_truth_evidences.size();
    prov.graph_routing_reason_code  = decision.graph_truth_routing_reason_code;
    prov.final_resolved             = decision.final_layer_resolution.resolved;
    prov.final_package_id           = decision.final_layer_resolution.package_id;
    prov.final_adapter_id           = decision.final_layer_resolution.primary_adapter_id;
    prov.final_routing_reason_code  = decision.final_layer_resolution.routing_reason_code;
    prov.fallback_mode              = std::string(fallback_mode_sv);
    prov.fallback_reason_code       = decision.fallback_reason_code;
    prov.chain_complete             = decision.should_retrieve
                                      && (prov.fallback_mode != std::string(observability::reason_codes::fallback_mode::kFailClosed));
    decision.provenance = prov;
    observability::emitProvenanceLog(prov);

    // Persist one lineage step for post-generation provenance queries.
    if (cfg_.provenance_store) {
        observability::ProvenanceStepRecord step_record;
        step_record.query_id                  = decision.correlation_id;
        step_record.step_number               = static_cast<int>(step_sequence_);
        step_record.correlation_id            = decision.correlation_id;
        step_record.timestamp_ms              = static_cast<int64_t>(std::time(nullptr)) * 1000;
        step_record.layer_name                = decision.escalation_source_layer.empty()
                                                    ? std::string(observability::telemetry::layers::kTensor)
                                                    : decision.escalation_source_layer;
        step_record.source_layer              = decision.escalation_source_layer;
        step_record.input_vector_hash         = decision.flare_query;
        step_record.num_candidates            = static_cast<int64_t>(decision.tensor_summary_candidates.size());
        step_record.num_selected              = static_cast<int64_t>(decision.graph_truth_evidences.size());
        step_record.backend_name              = "TensorRAGPipeline";
        step_record.routing_reason_code       = decision.routing_reason_code;
        step_record.fallback_mode             = std::string(fallback_mode_sv);
        step_record.confidence_policy_version = decision.confidence_policy_version;
        step_record.decision_duration_us      = 0;

        (void)cfg_.provenance_store->storeRecord(step_record.query_id,
                                                 step_record.step_number,
                                                 step_record);
    }
    ++step_sequence_;

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
    step_sequence_ = 0;
}

} // namespace rag
} // namespace themis

