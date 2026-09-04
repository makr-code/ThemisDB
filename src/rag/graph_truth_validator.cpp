/**
 * @file graph_truth_validator.cpp
 * @brief RAG graph ground-truth validator implementation.
 *
 * Implements GraphTruthValidator: set-intersection scoring, per-query
 * breakdowns, and integration with the evaluation metric collector.
 */

#include "rag/graph_truth_validator.h"
#include "observability/layer_decision_log.h"
#include "observability/reason_codes.h"
#include "observability/telemetry_keys.h"

#include <algorithm>
#include <unordered_map>
#include <utility>

namespace themis::rag {

void GraphTruthValidator::setOntologyRetriever(std::shared_ptr<OntologyAwareRetriever> retriever) {
    ontology_retriever_ = std::move(retriever);
}

void GraphTruthValidator::setKnowledgeGraphRetriever(
    std::shared_ptr<kg::KnowledgeGraphRetriever> retriever) {
    kg_retriever_ = std::move(retriever);
}

GraphTruthValidationResult GraphTruthValidator::validate(
    const std::string& query,
    const tensor::TensorLayerSummary& tensor_summary,
    const GraphTruthValidatorConfig& config,
    const std::string& correlation_id) const {
    auto candidates = makeCandidateDocuments(tensor_summary, config.max_evidence_candidates);

    if (config.use_ontology_validation && ontology_retriever_) {
        auto ontology_result = ontology_retriever_->retrieve(query, candidates);
        auto result = buildFromOntologyResult(tensor_summary, ontology_result, config);
        result.correlation_id = correlation_id;
        result.routing_reason_code = std::string(observability::reason_codes::graph_truth::kOntologyValidation);
        result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kNone);
        result.escalation_source_layer = std::string(observability::telemetry::layers::kGraph);
        observability::emitLayerDecisionLog(
            observability::telemetry::layers::kGraph,
            result.correlation_id,
            result.routing_reason_code,
            std::string(observability::reason_codes::kPolicyVersionDefault),
            std::string(observability::reason_codes::tensor_rag::kThresholdKeyNone),
            result.fallback_mode,
            result.fallback_reason_code,
            result.escalation_source_layer,
            !result.evidences.empty());
        return result;
    }

    if (kg_retriever_) {
        auto kg_result = kg_retriever_->retrieve(query, candidates);
        auto result = buildFromKgResult(tensor_summary, kg_result, config);
        result.correlation_id = correlation_id;
        result.routing_reason_code = std::string(observability::reason_codes::graph_truth::kKgValidation);
        result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kNone);
        result.escalation_source_layer = std::string(observability::telemetry::layers::kGraph);
        observability::emitLayerDecisionLog(
            observability::telemetry::layers::kGraph,
            result.correlation_id,
            result.routing_reason_code,
            std::string(observability::reason_codes::kPolicyVersionDefault),
            std::string(observability::reason_codes::tensor_rag::kThresholdKeyNone),
            result.fallback_mode,
            result.fallback_reason_code,
            result.escalation_source_layer,
            !result.evidences.empty());
        return result;
    }

    GraphTruthValidationResult result;
    result.routing_reason = "graph truth validator has no retriever configured";
    result.routing_reason_code = std::string(observability::reason_codes::graph_truth::kNoRetriever);
    result.correlation_id = correlation_id;
    result.fallback_mode = std::string(observability::reason_codes::fallback_mode::kDegradedContinue);
    result.fallback_reason_code = std::string(observability::reason_codes::graph_truth::kFallbackRetrieverNotConfigured);
    result.escalation_source_layer = std::string(observability::telemetry::layers::kGraph);
    result.used_ontology_validation = false;
    observability::emitLayerDecisionLog(
        observability::telemetry::layers::kGraph,
        result.correlation_id,
        result.routing_reason_code,
        std::string(observability::reason_codes::kPolicyVersionDefault),
        std::string(observability::reason_codes::tensor_rag::kThresholdKeyNone),
        result.fallback_mode,
        result.fallback_reason_code,
        result.escalation_source_layer,
        false);
    return result;
}

std::vector<judge::RetrievedDocument> GraphTruthValidator::makeCandidateDocuments(
    const tensor::TensorLayerSummary& tensor_summary,
    std::size_t max_candidates) {
    std::vector<judge::RetrievedDocument> documents = {};

    const auto take = std::min<std::size_t>(tensor_summary.similar_adapters.size(), max_candidates);
    documents.reserve(take);

    for (std::size_t i = 0; i < take; ++i) {
        const auto& candidate = tensor_summary.similar_adapters[i];
        judge::RetrievedDocument doc;
        doc.id = candidate.adapter_key;
        doc.content = candidate.domain + " " + candidate.base_model_id + " " + candidate.adapter_key;
        doc.similarity_score = candidate.score;
        documents.push_back(std::move(doc));
    }

    return documents;
}

GraphTruthValidationResult GraphTruthValidator::buildFromOntologyResult(
    const tensor::TensorLayerSummary& tensor_summary,
    const OntologyRetrievalResult& ontology_result,
    const GraphTruthValidatorConfig& config) {
    GraphTruthValidationResult result;
    result.used_ontology_validation = true;
    result.routing_reason = "graph truth validation via OntologyAwareRetriever";

    std::unordered_map<std::string, double> tensor_scores = {};

    for (const auto& candidate : tensor_summary.similar_adapters) {
        tensor_scores.emplace(candidate.adapter_key, candidate.score);
    }

    result.evidences.reserve(ontology_result.documents.size());
    for (const auto& doc : ontology_result.documents) {
        GraphTruthEvidence evidence;
        evidence.candidate_id = doc.document.id;
        evidence.graph_score = doc.kg_boost;
        evidence.tensor_score = tensor_scores.contains(doc.document.id) ? tensor_scores[doc.document.id] : 0.0;
        evidence.validated = doc.final_score >= config.min_graph_truth_score;

        for (const auto& link : doc.entity_links) {
            if (link.is_linked) {
                evidence.supporting_nodes.push_back(link.node_id);
            }
        }

        auto reasoning_it = doc.document.metadata.find(std::string(observability::telemetry::metadata_keys::kReasoningChain));
        if (reasoning_it != doc.document.metadata.end()) {
            evidence.reasoning_chain = reasoning_it->second;
        }

        result.evidences.push_back(std::move(evidence));
    }

    return result;
}

GraphTruthValidationResult GraphTruthValidator::buildFromKgResult(
    const tensor::TensorLayerSummary& tensor_summary,
    const kg::KGRetrievalResult& kg_result,
    const GraphTruthValidatorConfig& config) {
    GraphTruthValidationResult result;
    result.used_ontology_validation = false;
    result.routing_reason = "graph truth validation via KnowledgeGraphRetriever";

    std::unordered_map<std::string, double> tensor_scores = {};

    for (const auto& candidate : tensor_summary.similar_adapters) {
        tensor_scores.emplace(candidate.adapter_key, candidate.score);
    }

    result.evidences.reserve(kg_result.documents.size());
    for (const auto& doc : kg_result.documents) {
        GraphTruthEvidence evidence;
        evidence.candidate_id = doc.document.id;
        evidence.graph_score = doc.kg_boost;
        evidence.tensor_score = tensor_scores.contains(doc.document.id) ? tensor_scores[doc.document.id] : 0.0;
        evidence.validated = doc.final_score >= config.min_graph_truth_score;

        for (const auto& link : doc.entity_links) {
            if (link.is_linked) {
                evidence.supporting_nodes.push_back(link.node_id);
            }
        }

        auto reasoning_it = doc.document.metadata.find(std::string(observability::telemetry::metadata_keys::kReasoningChain));
        if (reasoning_it != doc.document.metadata.end()) {
            evidence.reasoning_chain = reasoning_it->second;
        }

        result.evidences.push_back(std::move(evidence));
    }

    return result;
}

} // namespace themis::rag