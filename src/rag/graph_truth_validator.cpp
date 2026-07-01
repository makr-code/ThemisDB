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
    std::vector<judge::RetrievedDocument> documents;
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

    std::unordered_map<std::string, double> tensor_scores;
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

    std::unordered_map<std::string, double> tensor_scores;
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

InputContractValidation GraphTruthValidator::validateInputContract(
    const tensor::TensorLayerSummary& tensor_summary,
    std::size_t max_candidates) {
    InputContractValidation result;
    result.max_candidates = max_candidates;
    result.candidate_count = tensor_summary.similar_adapters.size();

    // Check 1: Non-empty candidate list
    if (tensor_summary.similar_adapters.empty()) {
        result.contract_violations.push_back("tensor summary has no candidates");
        result.recommendation = "escalate";
        return result;
    }

    // Check 2: Valid scope key
    if (tensor_summary.scope_key.empty()) {
        result.contract_violations.push_back("tensor summary has empty scope_key");
        result.recommendation = "escalate";
        return result;
    }

    // Check 3: Sufficient candidate count (warn if low)
    if (tensor_summary.similar_adapters.size() < 2) {
        result.warnings.push_back("only one candidate provided by tensor layer");
    }

    // Check 4: Candidate scores are reasonable
    for (const auto& candidate : tensor_summary.similar_adapters) {
        if (candidate.score < 0.0 || candidate.score > 1.0) {
            result.contract_violations.push_back(
                "candidate '" + candidate.adapter_key + "' has invalid score: " +
                std::to_string(candidate.score));
        }
    }

    if (!result.contract_violations.empty()) {
        result.recommendation = "escalate";
        return result;
    }

    result.contract_satisfied = true;
    result.recommendation = "continue";
    return result;
}

AclValidationResult GraphTruthValidator::validateAcl(
    const std::string& candidate_id,
    const std::string& principal,
    const std::string& action,
    const std::unordered_map<std::string, std::string>& context) const {
    AclValidationResult result;
    result.candidate_id = candidate_id;
    result.principal = principal;
    result.action = action;
    result.resource_id = candidate_id;

    // Default: allow if no policy engine is configured (degraded mode)
    // This is a stub implementation; real implementation would consult policy engine
    result.acl_passed = true;
    result.policy_decision_reason = "no_acl_engine_configured_default_allow";
    result.detail = "ACL validation not available; using fail-open default";

    return result;
}

std::vector<MultiHopValidationResult> GraphTruthValidator::validateMultiHopRelationships(
    const std::string& source_node,
    const std::vector<std::string>& target_nodes,
    std::size_t max_depth) const {
    std::vector<MultiHopValidationResult> results;

    // Stub implementation: would traverse KG and find paths
    // Real implementation would use KnowledgeGraphRetriever to find indirect relationships
    for (const auto& target : target_nodes) {
        MultiHopValidationResult mh_result;
        mh_result.found_valid_path = false;  // Stub: no paths found without real KG
        mh_result.depth = 0;
        mh_result.path_confidence = 0.0;
        results.push_back(mh_result);
    }

    return results;
}

ProvenanceRecord GraphTruthValidator::assembleProvenance(
    const std::string& candidate_id,
    const tensor::TensorLayerSummary& tensor_summary,
    const std::vector<std::string>& supporting_nodes,
    const std::string& validation_principal,
    const std::unordered_map<std::string, double>& layer_confidence) {
    ProvenanceRecord provenance;
    provenance.evidence_id = candidate_id;
    provenance.source_layer = "ANN";
    provenance.creation_timestamp = std::chrono::system_clock::now();
    provenance.validation_timestamp = std::chrono::system_clock::now();
    provenance.validation_principal = validation_principal;

    // Layer decisions chain: ANN -> Tensor -> Graph
    provenance.layer_decisions.push_back("ANN");
    provenance.layer_decisions.push_back("Tensor");
    provenance.layer_decisions.push_back("Graph");

    // Supporting nodes from graph validation
    provenance.supporting_nodes = supporting_nodes;

    // Layer confidence scores
    provenance.layer_confidence_scores = layer_confidence;

    // Provenance is complete if we have supporting nodes
    provenance.provenance_complete = !supporting_nodes.empty();

    provenance.reasoning_chain = "Evidence '" + candidate_id + "' retrieved via ANN, " +
                                 "compressed by Tensor layer (scope: " + tensor_summary.scope_key +
                                 "), validated by Graph layer (" +
                                 std::to_string(supporting_nodes.size()) + " supporting nodes)";

    return provenance;
}

EvidenceBundle GraphTruthValidator::assembleEvidenceBundle(
    const GraphTruthEvidence& evidence,
    const std::vector<MultiHopValidationResult>& multi_hop_results,
    const ProvenanceRecord& provenance,
    const AclValidationResult& acl_result) {
    EvidenceBundle bundle;
    bundle.candidate_id = evidence.candidate_id;
    bundle.direct_evidence_nodes = evidence.supporting_nodes;
    bundle.indirect_evidence_paths = multi_hop_results;
    bundle.provenance = provenance;
    bundle.acl_result = acl_result;

    // Bundle is ready for LLM if:
    // 1. ACL validation passed
    // 2. Graph validation passed
    // 3. Provenance is complete
    bundle.ready_for_llm = acl_result.acl_passed && evidence.validated && provenance.provenance_complete;

    // Combine confidence: average across direct evidence
    if (!evidence.supporting_nodes.empty()) {
        bundle.combined_confidence = evidence.graph_score * 0.7 + evidence.tensor_score * 0.3;
    } else {
        bundle.combined_confidence = evidence.graph_score;
    }

    // Assemble LLM context from provenance and evidence
    bundle.llm_context = "Evidence: " + evidence.candidate_id + "\n" +
                        "Graph Score: " + std::to_string(evidence.graph_score) + "\n" +
                        "Tensor Score: " + std::to_string(evidence.tensor_score) + "\n" +
                        "Supporting Nodes: " + std::to_string(evidence.supporting_nodes.size()) + "\n" +
                        "Provenance: " + provenance.reasoning_chain + "\n" +
                        "ACL Status: " + (acl_result.acl_passed ? "PASSED" : "DENIED");

    bundle.metadata["candidate_id"] = evidence.candidate_id;
    bundle.metadata["validation_principal"] = provenance.validation_principal;
    bundle.metadata["provenance_complete"] = provenance.provenance_complete ? "true" : "false";
    bundle.metadata["acl_passed"] = acl_result.acl_passed ? "true" : "false";

    return bundle;
}

} // namespace themis::rag