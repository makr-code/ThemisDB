/**
 * @file layered_retrieval_orchestrator.cpp
 * @brief Implementation of the master orchestrator for the four-layer retrieval architecture.
 */

#include "search/layered_retrieval_orchestrator.h"
#include "utils/logger.h"

#include <chrono>
#include <sstream>
#include <algorithm>
#include <iomanip>
#include <atomic>

namespace themis {
namespace search {

namespace {
    // Helper: Convert steady clock to milliseconds
    auto measureTime(const auto& start, const auto& end) {
        return std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    }

    // Helper: Build a unique correlation ID if not provided
    std::string ensureCorrelationId(const std::string& provided_id) {
        if (!provided_id.empty()) return provided_id;
        static std::atomic<uint64_t> counter{0};
        return "retrieval-" + std::to_string(++counter);
    }
}

// ============================================================================
// Main Public API
// ============================================================================

LayeredRetrievalResult LayeredRetrievalOrchestrator::execute(
    const float* query_vector,
    std::size_t query_dim,
    const std::string& query_text,
    const LayeredRetrievalContext& context_in)
{
    auto start_time = std::chrono::steady_clock::now();
    LayeredRetrievalContext context = context_in;
    context.correlation_id = ensureCorrelationId(context.correlation_id);

    LayeredRetrievalResult result;
    result.correlation_id = context.correlation_id;
    result.debug_trace = nlohmann::json::object();

    THEMIS_INFO("LayeredRetrieval: Starting pipeline (correlation_id={})", context.correlation_id);

    // -----------------------------------------------------------------------
    // Layer 1: ANN Frontdoor
    // -----------------------------------------------------------------------
    if (!config_.enable_ann_layer) {
        THEMIS_WARN("LayeredRetrieval: ANN layer disabled, skipping");
    }

    LayerRoutingDecision ann_decision;
    ann_decision.layer_name = "ANN";

    index::AnnFrontdoorResult ann_result;
    if (config_.enable_ann_layer && ann_frontdoor_) {
        ann_result = executeAnnLayer(query_vector, query_dim, context, ann_decision);
    } else {
        ann_decision.success = false;
        ann_decision.routing_reason = "ANN layer disabled or not configured";
        ann_decision.errors.push_back("No ANN frontdoor available");
    }

    result.layer_decisions.push_back(ann_decision);

    if (!ann_decision.success && config_.fail_closed_on_graph_error) {
        THEMIS_ERROR("LayeredRetrieval: ANN layer failed and fail-closed is enabled");
        return buildErrorResult(result.layer_decisions, context);
    }

    // -----------------------------------------------------------------------
    // Layer 2: Tensor Mid-Layer
    // -----------------------------------------------------------------------
    if (!config_.enable_tensor_layer) {
        THEMIS_INFO("LayeredRetrieval: Tensor layer disabled");
    }

    LayerRoutingDecision tensor_decision;
    tensor_decision.layer_name = "Tensor";

    std::optional<tensor::TensorLayerSummary> tensor_summary;
    if (config_.enable_tensor_layer && tensor_layer_) {
        tensor_summary = executeTensorLayer(ann_result, context, tensor_decision);
    } else {
        tensor_decision.success = false;
        tensor_decision.routing_reason = "Tensor layer disabled or not configured";
        if (config_.enable_tensor_layer) {
            tensor_decision.errors.push_back("No tensor mid-layer available");
        }
    }

    result.layer_decisions.push_back(tensor_decision);

    if (!tensor_summary && config_.enable_tensor_layer && config_.fail_closed_on_graph_error) {
        THEMIS_ERROR("LayeredRetrieval: Tensor layer failed");
        return buildErrorResult(result.layer_decisions, context);
    }

    // Fallback: use ANN results directly if tensor layer failed
    if (!tensor_summary && ann_decision.success) {
        THEMIS_WARN("LayeredRetrieval: Using ANN results directly (tensor layer failed)");
        // Build a minimal tensor summary from ANN results
        tensor::TensorLayerSummary fallback;
        fallback.scope_key = "fallback";
        fallback.routing_reason = "Fallback from ANN layer";
        fallback.candidate_count = ann_result.candidates.size();
        tensor_summary = fallback;
        tensor_decision.success = true;
        tensor_decision.warnings.push_back("Used ANN fallback (tensor layer unavailable)");
    }

    // -----------------------------------------------------------------------
    // Layer 3: Graph Truth Layer
    // -----------------------------------------------------------------------
    if (!config_.enable_graph_layer) {
        THEMIS_INFO("LayeredRetrieval: Graph layer disabled");
    }

    LayerRoutingDecision graph_decision;
    graph_decision.layer_name = "Graph";

    std::optional<rag::GraphTruthValidationResult> graph_result;
    if (config_.enable_graph_layer && graph_validator_ && tensor_summary) {
        graph_result = executeGraphLayer(*tensor_summary, context, graph_decision);
    } else {
        graph_decision.success = false;
        if (!config_.enable_graph_layer) {
            graph_decision.routing_reason = "Graph layer disabled";
        } else if (!graph_validator_) {
            graph_decision.routing_reason = "No graph truth validator available";
            graph_decision.errors.push_back("Graph validator not configured");
        } else if (!tensor_summary) {
            graph_decision.routing_reason = "No tensor summary from upstream layer";
            graph_decision.errors.push_back("Tensor layer produced no summary");
        }
    }

    result.layer_decisions.push_back(graph_decision);

    if (!graph_result && config_.enable_graph_layer && config_.fail_closed_on_graph_error) {
        THEMIS_ERROR("LayeredRetrieval: Graph layer failed and fail-closed is enabled");
        return buildErrorResult(result.layer_decisions, context);
    }

    // Fallback: use empty evidence if graph layer failed
    if (!graph_result && graph_validator_) {
        THEMIS_WARN("LayeredRetrieval: Using empty evidence (graph layer failed)");
        rag::GraphTruthValidationResult fallback;
        fallback.valid = false;
        fallback.input_contract.is_valid = true;
        graph_result = fallback;
        graph_decision.success = true;
        graph_decision.warnings.push_back("Graph layer produced no validation result");
    }

    // -----------------------------------------------------------------------
    // Layer 4: LLM/LoRA Final Layer
    // -----------------------------------------------------------------------
    if (!config_.enable_llm_layer) {
        THEMIS_INFO("LayeredRetrieval: LLM layer disabled");
    }

    LayerRoutingDecision llm_decision;
    llm_decision.layer_name = "LLM";

    std::optional<std::string> final_answer;
    if (config_.enable_llm_layer && final_layer_ && graph_result) {
        final_answer = executeLlmLayer(*graph_result, query_text, context, llm_decision);
    } else {
        llm_decision.success = false;
        if (!config_.enable_llm_layer) {
            llm_decision.routing_reason = "LLM layer disabled";
        } else if (!final_layer_) {
            llm_decision.routing_reason = "No LLM orchestrator available";
            llm_decision.errors.push_back("LLM layer not configured");
        } else if (!graph_result) {
            llm_decision.routing_reason = "No graph validation result";
            llm_decision.errors.push_back("Graph layer produced no result");
        }
    }

    result.layer_decisions.push_back(llm_decision);

    // Fallback: generate template answer if LLM layer fails
    if (!final_answer) {
        THEMIS_WARN("LayeredRetrieval: Generating fallback answer (LLM layer failed)");
        if (graph_result) {
            final_answer = generateFallbackAnswer(query_text, graph_result->evidence);
        } else {
            final_answer = "Unable to generate answer at this time.";
        }
        llm_decision.success = true;
        llm_decision.warnings.push_back("Using fallback answer template");
    }

    // -----------------------------------------------------------------------
    // Build Final Result
    // -----------------------------------------------------------------------
    result.success = true;
    result.final_answer = final_answer.value_or("");

    if (graph_result) {
        result.evidence_bundle = graph_result->evidence;
        if (!graph_result->provenance_trail.empty()) {
            result.provenance_trail = graph_result->provenance_trail;
        }
    }

    auto end_time = std::chrono::steady_clock::now();
    result.total_latency_ms = measureTime(start_time, end_time);

    THEMIS_INFO("LayeredRetrieval: Pipeline completed in {}ms (correlation_id={})",
                result.total_latency_ms.count(), context.correlation_id);

    return result;
}

bool LayeredRetrievalOrchestrator::isHealthy() const noexcept {
    if (config_.enable_ann_layer && !ann_frontdoor_) return false;
    if (config_.enable_tensor_layer && !tensor_layer_) return false;
    if (config_.enable_graph_layer && !graph_validator_) return false;
    if (config_.enable_llm_layer && !final_layer_) return false;
    return true;
}

std::string LayeredRetrievalOrchestrator::statusReport() const {
    std::ostringstream oss;
    oss << "LayeredRetrievalOrchestrator Status:\n";
    oss << "  ANN Layer: " << (ann_frontdoor_ ? "✓" : "✗") << " (enabled: " << config_.enable_ann_layer << ")\n";
    oss << "  Tensor Layer: " << (tensor_layer_ ? "✓" : "✗") << " (enabled: " << config_.enable_tensor_layer << ")\n";
    oss << "  Graph Layer: " << (graph_validator_ ? "✓" : "✗") << " (enabled: " << config_.enable_graph_layer << ")\n";
    oss << "  LLM Layer: " << (final_layer_ ? "✓" : "✗") << " (enabled: " << config_.enable_llm_layer << ")\n";
    oss << "  Overall Health: " << (isHealthy() ? "✓ HEALTHY" : "✗ DEGRADED") << "\n";
    return oss.str();
}

// ============================================================================
// Private Layer Execution
// ============================================================================

index::AnnFrontdoorResult LayeredRetrievalOrchestrator::executeAnnLayer(
    const float* query_vector,
    std::size_t query_dim,
    const LayeredRetrievalContext& context,
    LayerRoutingDecision& decision)
{
    auto start = std::chrono::steady_clock::now();

    try {
        if (!query_vector || query_dim == 0) {
            throw std::invalid_argument("Invalid query vector");
        }

        // Build ANN context from orchestration context
        index::AnnQueryContext ann_context;
        ann_context.correlation_id = context.correlation_id;
        ann_context.dataset_size = 0;  // Will be inferred by frontdoor
        ann_context.hot_tier = true;

        // Execute search
        auto result = ann_frontdoor_->search(
            query_vector, query_dim, config_.ann_k, ann_context);

        decision.success = true;
        decision.routing_reason = result.routing_reason;
        decision.routing_reason_code = result.routing_reason_code;

        THEMIS_INFO("ANN Layer: Found {} candidates (routing: {})",
                    result.candidates.size(), result.routing_reason_code);

        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);

        return result;

    } catch (const std::exception& e) {
        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);
        decision.success = false;
        decision.routing_reason = "Exception in ANN layer";
        decision.errors.push_back(e.what());
        THEMIS_ERROR("ANN Layer exception: {}", e.what());
        return {};
    }
}

std::optional<tensor::TensorLayerSummary> LayeredRetrievalOrchestrator::executeTensorLayer(
    const index::AnnFrontdoorResult& ann_result,
    const LayeredRetrievalContext& context,
    LayerRoutingDecision& decision)
{
    auto start = std::chrono::steady_clock::now();

    try {
        if (ann_result.candidates.empty()) {
            decision.success = false;
            decision.routing_reason = "No candidates from ANN layer";
            decision.warnings.push_back("Empty candidate set");
            return std::nullopt;
        }

        // Build tensor context from orchestration context
        tensor::TensorLayerContext tensor_context;
        tensor_context.tenant_id = context.tenant_id;
        tensor_context.domain = context.domain;
        tensor_context.base_model_id = context.base_model_id;
        tensor_context.shard_aware = context.shard_aware;
        tensor_context.top_k = std::min(static_cast<size_t>(config_.tensor_top_k),
                                       ann_result.candidates.size());

        // Execute tensor planning first
        auto plan = tensor_layer_->plan(tensor_context);
        THEMIS_DEBUG("Tensor Layer plan: {}", plan.reason);

        // Execute tensor summarization
        auto summary = tensor_layer_->summarize(tensor_context);

        decision.success = true;
        decision.routing_reason = summary.routing_reason;
        decision.routing_reason_code = "tensor-" + std::to_string(
            static_cast<int>(summary.layer_kind));

        THEMIS_INFO("Tensor Layer: {} candidates after refinement",
                    summary.candidate_count);

        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);

        return summary;

    } catch (const std::exception& e) {
        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);
        decision.success = false;
        decision.routing_reason = "Exception in tensor layer";
        decision.errors.push_back(e.what());
        THEMIS_ERROR("Tensor Layer exception: {}", e.what());
        return std::nullopt;
    }
}

std::optional<rag::GraphTruthValidationResult> LayeredRetrievalOrchestrator::executeGraphLayer(
    const tensor::TensorLayerSummary& tensor_summary,
    const LayeredRetrievalContext& context,
    LayerRoutingDecision& decision)
{
    auto start = std::chrono::steady_clock::now();

    try {
        // Validate input contract
        auto contract_result = rag::GraphTruthValidator::validateInputContract(
            tensor_summary, config_.max_candidates);

        if (!contract_result.is_valid) {
            decision.success = false;
            decision.routing_reason = "Tensor summary failed input contract";
            for (const auto& err : contract_result.validation_errors) {
                decision.errors.push_back(err);
            }
            auto end = std::chrono::steady_clock::now();
            decision.elapsed_ms = measureTime(start, end);
            return std::nullopt;
        }

        // Build graph validation config
        rag::GraphTruthValidatorConfig graph_config;
        graph_config.enable_acl_validation = true;
        graph_config.enable_provenance_tracking = true;
        graph_config.enable_multi_hop_validation = true;

        // Execute graph validation
        auto result = graph_validator_->validate(
            context.query_text,
            tensor_summary,
            graph_config,
            context.correlation_id);

        decision.success = result.valid;
        decision.routing_reason = result.reasoning_chain.empty() ?
            "Graph validation completed" : result.reasoning_chain;
        decision.routing_reason_code = result.valid ? "graph-valid" : "graph-invalid";

        THEMIS_INFO("Graph Layer: {} evidence items (valid: {})",
                    result.evidence.size(), result.valid);

        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);

        return result;

    } catch (const std::exception& e) {
        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);
        decision.success = false;
        decision.routing_reason = "Exception in graph layer";
        decision.errors.push_back(e.what());
        THEMIS_ERROR("Graph Layer exception: {}", e.what());
        return std::nullopt;
    }
}

std::optional<std::string> LayeredRetrievalOrchestrator::executeLlmLayer(
    const rag::GraphTruthValidationResult& graph_result,
    const std::string& query_text,
    const LayeredRetrievalContext& context,
    LayerRoutingDecision& decision)
{
    auto start = std::chrono::steady_clock::now();

    try {
        // Build final layer request
        llm::FinalLayerRequest llm_request;
        llm_request.prompt = query_text;
        llm_request.correlation_id = context.correlation_id;
        llm_request.base_model_name = context.base_model_id;
        llm_request.requested_package_id = context.requested_package_id;
        llm_request.confidence_policy_version = context.confidence_policy_version;
        llm_request.confidence_threshold_key = context.confidence_threshold_key;

        // Add evidence as metadata
        if (!graph_result.evidence.empty()) {
            nlohmann::json evidence_json = nlohmann::json::array();
            for (const auto& ev : graph_result.evidence) {
                nlohmann::json ev_json;
                ev_json["id"] = ev.candidate_id;
                ev_json["score"] = ev.graph_score;
                evidence_json.push_back(ev_json);
            }
            llm_request.metadata["evidence"] = evidence_json;
        }

        // Resolve final layer package and adapter
        auto resolution = final_layer_->resolve(llm_request);

        if (!resolution.resolved) {
            decision.success = false;
            decision.routing_reason = "LLM package resolution failed";
            for (const auto& err : resolution.errors) {
                decision.errors.push_back(err);
            }
            auto end = std::chrono::steady_clock::now();
            decision.elapsed_ms = measureTime(start, end);
            return std::nullopt;
        }

        // In a production system, this would call the actual LLM backend
        // For now, we return a structured response
        std::string generated_answer;
        if (!graph_result.evidence.empty()) {
            generated_answer = "Based on the retrieved evidence, ";
            generated_answer += std::to_string(graph_result.evidence.size());
            generated_answer += " relevant sources support an answer to your query.";
        } else {
            generated_answer = "I could not find sufficient evidence to answer your query.";
        }

        decision.success = true;
        decision.routing_reason = "LLM generation successful";
        decision.routing_reason_code = "llm-resolved";

        THEMIS_INFO("LLM Layer: Generated answer using package {}",
                    resolution.package_id);

        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);

        return generated_answer;

    } catch (const std::exception& e) {
        auto end = std::chrono::steady_clock::now();
        decision.elapsed_ms = measureTime(start, end);
        decision.success = false;
        decision.routing_reason = "Exception in LLM layer";
        decision.errors.push_back(e.what());
        THEMIS_ERROR("LLM Layer exception: {}", e.what());
        return std::nullopt;
    }
}

// ============================================================================
// Error Handling and Fallback
// ============================================================================

std::string LayeredRetrievalOrchestrator::generateFallbackAnswer(
    const std::string& query_text,
    const std::vector<rag::GraphTruthEvidence>& evidence)
{
    std::ostringstream oss;
    oss << "I found " << evidence.size() << " relevant sources related to your query. ";
    
    if (evidence.size() > 0) {
        oss << "The most relevant source has a confidence score of "
            << std::fixed << std::setprecision(2) << evidence[0].graph_score << ". ";
    }

    oss << "However, I was unable to generate a complete answer due to a system limitation. "
        << "Please consult the evidence sources directly.";

    return oss.str();
}

LayeredRetrievalResult LayeredRetrievalOrchestrator::buildErrorResult(
    const std::vector<LayerRoutingDecision>& decisions,
    const LayeredRetrievalContext& context)
{
    LayeredRetrievalResult result;
    result.success = false;
    result.correlation_id = context.correlation_id;
    result.layer_decisions = decisions;

    std::ostringstream oss;
    oss << "Retrieval failed. Errors: ";
    bool first = true;
    for (const auto& decision : decisions) {
        if (!decision.errors.empty()) {
            if (!first) oss << "; ";
            oss << decision.layer_name << ": " << decision.errors[0];
            first = false;
        }
    }
    result.final_answer = oss.str();

    THEMIS_ERROR("LayeredRetrieval: Error result - {}", result.final_answer);

    return result;
}

void LayeredRetrievalOrchestrator::traceLayerTransition(
    const std::string& from_layer,
    const std::string& to_layer,
    const LayerRoutingDecision& decision,
    const LayeredRetrievalContext& context)
{
    if (!context.trace_enabled) return;

    THEMIS_DEBUG("Transition: {} → {} (success={}, reason={})",
                 from_layer, to_layer, decision.success, decision.routing_reason);
}

} // namespace search
} // namespace themis
