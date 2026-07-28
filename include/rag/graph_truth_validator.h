#pragma once

#include "rag/knowledge_graph_retriever.h"
#include "rag/ontology_aware_retriever.h"
#include "tensor/tensor_mid_layer.h"

#include <memory>
#include <string>
#include <vector>

namespace themis::rag {

/**
 * @brief Configuration for the explicit graph-truth validation stage.
 */

/**
 * @file graph_truth_validator.h
 * @brief Ground-truth validator for RAG graph-retrieval results.
 *
 * Declares GraphTruthValidator, which compares RAG graph-retrieval
 * outputs against annotated ground-truth sets to compute recall and precision.
 */
struct GraphTruthValidatorConfig {
    /// Maximum number of tensor summary candidates converted to graph evidence.
    std::size_t max_evidence_candidates = 8;

    /// If true, ontology-aware validation is preferred over plain KG retrieval.
    bool use_ontology_validation = true;

    /// Minimum graph-derived score required for a candidate to be treated as validated.
    double min_graph_truth_score = 0.15;
};

/**
 * @brief One graph-backed evidence item produced by the truth layer.
 */
struct GraphTruthEvidence {
    std::string candidate_id;
    double graph_score = 0.0;
    double tensor_score = 0.0;
    bool validated = false;
    std::vector<std::string> supporting_nodes;
    std::string reasoning_chain;
};

/**
 * @brief Result of validating one tensor-layer summary against graph truth.
 */
struct GraphTruthValidationResult {
    std::vector<GraphTruthEvidence> evidences;
    std::string routing_reason;
    std::string routing_reason_code;
    std::string correlation_id;
    std::string fallback_mode;
    std::string fallback_reason_code;
    std::string escalation_source_layer;
    bool used_ontology_validation = false;
};

/**
 * @brief Explicit graph-truth validation stage above ANN and Tensor layers.
 *
 * Consumes tensor-layer summaries, projects them into temporary retrieved
 * documents, and validates them using either OntologyAwareRetriever or
 * KnowledgeGraphRetriever.
 */
class GraphTruthValidator {
public:
    GraphTruthValidator() = default;
    ~GraphTruthValidator() = default;

    void setOntologyRetriever(std::shared_ptr<OntologyAwareRetriever> retriever);
    void setKnowledgeGraphRetriever(std::shared_ptr<kg::KnowledgeGraphRetriever> retriever);

    [[nodiscard]] GraphTruthValidationResult validate(
        const std::string& query,
        const tensor::TensorLayerSummary& tensor_summary,
        const GraphTruthValidatorConfig& config = {},
        const std::string& correlation_id = {}) const;

private:
    [[nodiscard]] static std::vector<judge::RetrievedDocument> makeCandidateDocuments(
        const tensor::TensorLayerSummary& tensor_summary,
        std::size_t max_candidates);

    [[nodiscard]] static GraphTruthValidationResult buildFromOntologyResult(
        const tensor::TensorLayerSummary& tensor_summary,
        const OntologyRetrievalResult& ontology_result,
        const GraphTruthValidatorConfig& config);

    [[nodiscard]] static GraphTruthValidationResult buildFromKgResult(
        const tensor::TensorLayerSummary& tensor_summary,
        const kg::KGRetrievalResult& kg_result,
        const GraphTruthValidatorConfig& config);

    std::shared_ptr<OntologyAwareRetriever> ontology_retriever_;
    std::shared_ptr<kg::KnowledgeGraphRetriever> kg_retriever_;
};

} // namespace themis::rag