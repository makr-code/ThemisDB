#pragma once

#include "auth/authorization_policy.h"
#include "rag/knowledge_graph_retriever.h"
#include "rag/ontology_aware_retriever.h"
#include "tensor/tensor_mid_layer.h"

#include <chrono>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis::rag {

/**
 * @brief Configuration for the explicit graph-truth validation stage.
 */
struct GraphTruthValidatorConfig {
    /// Maximum number of tensor summary candidates converted to graph evidence.
    std::size_t max_evidence_candidates = 8;

    /// If true, ontology-aware validation is preferred over plain KG retrieval.
    bool use_ontology_validation = true;

    /// Minimum graph-derived score required for a candidate to be treated as validated.
    double min_graph_truth_score = 0.15;

    /// If true, ACL/policy validation is mandatory; fail-closed on policy violations.
    bool enable_acl_validation = true;

    /// If true, provenance tracking is enabled; produces full chain of custody.
    bool enable_provenance_tracking = true;

    /// If true, multi-hop validation is performed to establish indirect relationships.
    bool enable_multi_hop_validation = true;

    /// Maximum number of hops for multi-hop validation (default 3).
    std::size_t max_multi_hop_depth = 3;
};

/**
 * @brief Provenance record for tracking evidence origin and chain of custody.
 *
 * Provenance tracks the complete chain from source through ANN->Tensor->Graph layers,
 * including timestamps, layer-specific decisions, and validation results at each stage.
 */
struct ProvenanceRecord {
    /// Unique identifier for this evidence item in the graph.
    std::string evidence_id;

    /// Source layer that originated this evidence (e.g., "ANN", "Tensor", "GraphKG").
    std::string source_layer;

    /// Timestamp when evidence was first created in ANN layer.
    std::chrono::system_clock::time_point creation_timestamp;

    /// Timestamp when evidence passed graph validation.
    std::chrono::system_clock::time_point validation_timestamp;

    /// Validation decision at each layer: "ANN" -> "Tensor" -> "Graph".
    std::vector<std::string> layer_decisions;

    /// Graph node IDs that support this evidence (direct links).
    std::vector<std::string> supporting_nodes;

    /// Indirect node IDs supporting this evidence (multi-hop paths).
    std::vector<std::string> indirect_supporting_nodes;

    /// Graph edges that form the validation chain.
    std::vector<std::pair<std::string, std::string>> validation_edges;

    /// Confidence score assigned at each layer.
    std::unordered_map<std::string, double> layer_confidence_scores;

    /// Human-readable reasoning chain for audit/debugging.
    std::string reasoning_chain;

    /// Policy identifiers that were validated against this evidence.
    std::vector<std::string> validated_policies;

    /// Principal/user that invoked the validation (for ACL tracking).
    std::string validation_principal;

    /// Indicates if provenance was complete and unbroken.
    bool provenance_complete = true;
};

/**
 * @brief Result of ACL/policy validation for a given evidence item.
 *
 * ACL validation determines whether the evidence is accessible and whether
 * policies allow its use in the current context.
 */
struct AclValidationResult {
    /// Whether the evidence passed ACL checks.
    bool acl_passed = false;

    /// Policy IDs that were evaluated.
    std::vector<std::string> evaluated_policies;

    /// Policy IDs that granted access.
    std::vector<std::string> granted_by_policies;

    /// Policy IDs that denied access (fail-closed).
    std::vector<std::string> denied_by_policies;

    /// Reason code for policy decision.
    std::string policy_decision_reason;

    /// Principal that was validated against ACL.
    std::string principal;

    /// Resource identifier in the ACL system.
    std::string resource_id;

    /// Action being validated (e.g., "read", "use_in_generation").
    std::string action;

    /// Free-form reason for denial or approval.
    std::string detail;
};

/**
 * @brief Input contract validation result from tensor layer.
 *
 * Validates that tensor-layer input meets graph validation requirements
 * before processing begins.
 */
struct InputContractValidation {
    /// Whether input contract was satisfied.
    bool contract_satisfied = false;

    /// Issues found in tensor summary (e.g., missing candidates, invalid scope).
    std::vector<std::string> contract_violations;

    /// Warnings that don't block validation but should be noted.
    std::vector<std::string> warnings;

    /// Number of candidates provided by tensor layer.
    std::size_t candidate_count = 0;

    /// Maximum candidates that can be validated.
    std::size_t max_candidates = 8;

    /// Recommendation for proceeding (e.g., "continue", "escalate", "fallback").
    std::string recommendation;
};

/**
 * @brief Multi-hop validation result for indirect relationships.
 *
 * When multi-hop validation is enabled, this captures paths through the graph
 * that indirectly support a candidate.
 */
struct MultiHopValidationResult {
    /// Whether any valid multi-hop path was found.
    bool found_valid_path = false;

    /// The hop path from source to target (node IDs).
    std::vector<std::string> hop_path;

    /// Confidence score for this multi-hop path.
    double path_confidence = 0.0;

    /// Depth of this path (number of edges).
    std::size_t depth = 0;

    /// Textual description of the relationship chain.
    std::string relationship_chain;
};

/**
 * @brief Evidence assembly model representing a complete evidence bundle.
 *
 * Assembles direct evidence, indirect (multi-hop) evidence, and related
 * provenance/ACL information into a coherent bundle ready for LLM input.
 */
struct EvidenceBundle {
    /// Candidate ID this bundle represents.
    std::string candidate_id;

    /// Direct graph-derived evidence items.
    std::vector<std::string> direct_evidence_nodes;

    /// Indirect evidence from multi-hop validation.
    std::vector<MultiHopValidationResult> indirect_evidence_paths;

    /// Provenance record tracing this evidence back to source.
    ProvenanceRecord provenance;

    /// ACL validation result for access control.
    AclValidationResult acl_result;

    /// Whether this bundle is ready for LLM consumption.
    bool ready_for_llm = false;

    /// Combined confidence across all validation stages.
    double combined_confidence = 0.0;

    /// Free-form context for LLM (assembled from graph/provenance).
    std::string llm_context;

    /// Structured metadata for tracing/debugging.
    std::unordered_map<std::string, std::string> metadata;
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

    /// Extended fields for provenance and ACL.

    /// Complete provenance record for this evidence.
    ProvenanceRecord provenance;

    /// ACL validation result for this evidence.
    AclValidationResult acl_result;

    /// Input contract validation that preceded graph validation.
    InputContractValidation input_contract;

    /// Multi-hop validation results supporting this evidence.
    std::vector<MultiHopValidationResult> multi_hop_results;

    /// Whether evidence passed ACL validation (fail-closed).
    bool acl_validated = false;

    /// Whether evidence passed input contract validation.
    bool input_contract_validated = false;

    /// Whether evidence has supporting multi-hop paths.
    bool has_multi_hop_support = false;

    /// Overall evidence bundle ready for LLM.
    EvidenceBundle evidence_bundle;
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

    /// Extended fields for graph truth formalization.

    /// Input contract validation results.
    InputContractValidation input_contract_validation;

    /// Whether all validations (including ACL/policy) passed.
    bool all_validations_passed = false;

    /// Number of evidences that passed ACL validation.
    std::size_t acl_validated_count = 0;

    /// Number of evidences with multi-hop support.
    std::size_t multi_hop_count = 0;

    /// Number of evidences with complete provenance.
    std::size_t complete_provenance_count = 0;

    /// Policy validation failures that occurred.
    std::vector<std::string> policy_violations;

    /// Graph validation decision (e.g., "accept", "reject", "escalate").
    std::string graph_decision;

    /// Free-form audit trail for compliance/debugging.
    std::string audit_trail;
};


/**
 * @brief Explicit graph-truth validation stage above ANN and Tensor layers.
 *
 * Consumes tensor-layer summaries, projects them into temporary retrieved
 * documents, and validates them using either OntologyAwareRetriever or
 * KnowledgeGraphRetriever.
 *
 * Responsibilities:
 * - Exact relation validation against graph truth
 * - Provenance tracking (complete chain of custody from ANN->Tensor->Graph)
 * - Evidence chain assembly (direct and multi-hop evidence)
 * - ACL/Policy validation (fail-closed enforcement)
 * - Multi-hop validation (for indirect relationships)
 * - Input contract validation (ensure tensor input meets requirements)
 * - Evidence bundle assembly (for LLM consumption)
 *
 * Failure modes:
 * - Missing retriever: degraded continue with fallback
 * - Policy violation: fail-closed, reject evidence
 * - Broken provenance: log warning, still usable if graph validates
 * - Invalid input contract: escalate or use fallback
 *
 * Semantics:
 * - Graph layer is the exact truth layer (no approximations)
 * - Tensor/ANN may prioritize, but graph provides final validation
 * - ACL/policy checks are mandatory when enabled
 * - Evidence must have complete provenance trail for compliance
 */
class GraphTruthValidator {
public:
    GraphTruthValidator() = default;
    ~GraphTruthValidator() = default;

    void setOntologyRetriever(std::shared_ptr<OntologyAwareRetriever> retriever);
    void setKnowledgeGraphRetriever(std::shared_ptr<kg::KnowledgeGraphRetriever> retriever);

    /**
     * @brief Inject an authorization policy engine for ACL validation (Phase 2).
     *
     * When set, `validateAcl()` delegates to this engine using the ABAC triple
     * (subject, resource, action).  When not set and `enable_acl_validation` is
     * true, `validateAcl()` **fails closed** (denies access) to prevent
     * accidental information disclosure.
     *
     * Pass `nullptr` to detach the current policy engine.
     *
     * @param policy  Shared pointer to an IAuthorizationPolicy implementation
     *                (must outlive this validator or be detached first).
     */
    void setAuthorizationPolicy(std::shared_ptr<auth::IAuthorizationPolicy> policy);

    /**
     * @brief Inject a KnowledgeGraph for multi-hop BFS path traversal (Phase 2).
     *
     * Provides direct graph access required by `validateMultiHopRelationships()`
     * to trace indirect relationship paths.  This is separate from the
     * KnowledgeGraphRetriever (which performs retrieval-augmented ranking) and
     * provides raw edge traversal for path-finding.
     *
     * Pass `nullptr` to detach the current graph.
     *
     * @param graph  Shared pointer to the KnowledgeGraph.
     */
    void setKnowledgeGraph(std::shared_ptr<kg::KnowledgeGraph> graph);

    /**
     * @brief Validate tensor-layer summary against graph truth.
     *
     * Primary entry point for graph validation. Performs:
     * 1. Input contract validation (ensure tensor summary is well-formed)
     * 2. Graph evidence retrieval (ontology or KG)
     * 3. ACL/policy validation (if enabled)
     * 4. Multi-hop validation (if enabled)
     * 5. Provenance tracking (if enabled)
     * 6. Evidence bundle assembly (ready for LLM)
     *
     * @param query Original query string for context
     * @param tensor_summary Input from tensor mid-layer
     * @param config Validation configuration (ACL, provenance, multi-hop settings)
     * @param correlation_id For tracing across layers
     * @return Validation result with evidences, provenance, ACL results
     */
    [[nodiscard]] GraphTruthValidationResult validate(
        const std::string& query,
        const tensor::TensorLayerSummary& tensor_summary,
        const GraphTruthValidatorConfig& config = {},
        const std::string& correlation_id = {}) const;

    /**
     * @brief Validate input contract from tensor layer.
     *
     * Ensures tensor summary is well-formed and meets graph validation requirements:
     * - Non-empty candidate list
     * - Valid scope key
     * - Sufficient candidate count
     * - No invalid TensorLayerKind
     *
     * @param tensor_summary Input from tensor mid-layer
     * @param max_candidates Maximum candidates acceptable
     * @return Input contract validation result
     */
    [[nodiscard]] static InputContractValidation validateInputContract(
        const tensor::TensorLayerSummary& tensor_summary,
        std::size_t max_candidates);

    /**
     * @brief Validate ACL/policy constraints for evidence.
     *
     * Applies policy engine and ACL checks to determine if evidence is accessible
     * in the current context. Fail-closed: any policy violation rejects evidence.
     *
     * @param candidate_id Identifier of the evidence being checked
     * @param principal User/principal requesting the evidence
     * @param action Action being performed (e.g., "read", "use_in_generation")
     * @param context Additional context for policy evaluation
     * @return ACL validation result
     */
    [[nodiscard]] AclValidationResult validateAcl(
        const std::string& candidate_id,
        const std::string& principal,
        const std::string& action,
        const std::unordered_map<std::string, std::string>& context = {}) const;

    /**
     * @brief Validate multi-hop relationships in the knowledge graph.
     *
     * Attempts to establish indirect relationships between candidates and
     * supporting evidence through multi-hop paths in the knowledge graph.
     *
     * @param source_node Source candidate identifier
     * @param target_nodes Candidate nodes to validate
     * @param max_depth Maximum number of hops to traverse
     * @return Vector of multi-hop validation results (one per valid path found)
     */
    [[nodiscard]] std::vector<MultiHopValidationResult> validateMultiHopRelationships(
        const std::string& source_node,
        const std::vector<std::string>& target_nodes,
        std::size_t max_depth = 3) const;

    /**
     * @brief Assemble complete provenance record for evidence.
     *
     * Constructs a chain-of-custody trail from ANN retrieval through tensor
     * compression to graph validation, including decisions at each stage.
     *
     * @param candidate_id Evidence identifier
     * @param tensor_summary Tensor layer input
     * @param supporting_nodes Graph nodes supporting this evidence
     * @param validation_principal Principal who validated
     * @param layer_confidence Confidence scores at each layer
     * @return Complete provenance record for audit/compliance
     */
    [[nodiscard]] static ProvenanceRecord assembleProvenance(
        const std::string& candidate_id,
        const tensor::TensorLayerSummary& tensor_summary,
        const std::vector<std::string>& supporting_nodes,
        const std::string& validation_principal,
        const std::unordered_map<std::string, double>& layer_confidence);

    /**
     * @brief Assemble evidence bundle ready for LLM consumption.
     *
     * Combines direct evidence, multi-hop paths, provenance, and ACL results
     * into a structured bundle that provides full context for grounded generation.
     *
     * @param evidence Graph evidence item
     * @param multi_hop_results Multi-hop validation results
     * @param provenance Provenance record
     * @param acl_result ACL validation result
     * @return Complete evidence bundle
     */
    [[nodiscard]] static EvidenceBundle assembleEvidenceBundle(
        const GraphTruthEvidence& evidence,
        const std::vector<MultiHopValidationResult>& multi_hop_results,
        const ProvenanceRecord& provenance,
        const AclValidationResult& acl_result);

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
    std::shared_ptr<auth::IAuthorizationPolicy> authorization_policy_;
    std::shared_ptr<kg::KnowledgeGraph> knowledge_graph_;
};

} // namespace themis::rag