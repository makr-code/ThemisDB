#include <gtest/gtest.h>
#include "rag/graph_truth_validator.h"
#include "tensor/tensor_mid_layer.h"

namespace themis::rag::test {

// ============================================================================
// Test Fixtures and Utilities
// ============================================================================

class GraphTruthValidatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        validator_ = std::make_shared<GraphTruthValidator>();
    }

    std::shared_ptr<GraphTruthValidator> validator_;

    // Helper: Create a valid tensor summary for testing
    tensor::TensorLayerSummary createValidTensorSummary(
        size_t num_candidates = 3,
        const std::string& scope_key = "doc:test-scope") {
        tensor::TensorLayerSummary summary;
        summary.scope_key = scope_key;
        summary.layer_kind = tensor::TensorLayerKind::Adapter;
        summary.ann_scope_kind = index::AnnScopeKind::Document;
        summary.candidate_count = num_candidates;

        for (size_t i = 0; i < num_candidates; ++i) {
            tensor::SimilarityResult result;
            result.adapter_key = "adapter-" + std::to_string(i);
            result.domain = "test-domain";
            result.base_model_id = "model-v1";
            result.score = 0.8 - (i * 0.1);  // Decreasing scores
            summary.similar_adapters.push_back(result);
        }

        return summary;
    }
};

// ============================================================================
// InputContractValidation Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, ValidateInputContractWithValidSummary) {
    auto summary = createValidTensorSummary(3);
    auto result = GraphTruthValidator::validateInputContract(summary, 8);

    EXPECT_TRUE(result.contract_satisfied);
    EXPECT_EQ(result.candidate_count, 3);
    EXPECT_THAT(result.contract_violations, ::testing::IsEmpty());
    EXPECT_EQ(result.recommendation, "continue");
}

TEST_F(GraphTruthValidatorTest, ValidateInputContractWithEmptyCandidates) {
    auto summary = createValidTensorSummary(0);
    auto result = GraphTruthValidator::validateInputContract(summary, 8);

    EXPECT_FALSE(result.contract_satisfied);
    EXPECT_THAT(result.contract_violations,
                ::testing::Contains(::testing::HasSubstr("no candidates")));
    EXPECT_EQ(result.recommendation, "escalate");
}

TEST_F(GraphTruthValidatorTest, ValidateInputContractWithEmptyScopeKey) {
    auto summary = createValidTensorSummary(2);
    summary.scope_key = "";
    auto result = GraphTruthValidator::validateInputContract(summary, 8);

    EXPECT_FALSE(result.contract_satisfied);
    EXPECT_THAT(result.contract_violations,
                ::testing::Contains(::testing::HasSubstr("empty scope_key")));
}

TEST_F(GraphTruthValidatorTest, ValidateInputContractWithSingleCandidate) {
    auto summary = createValidTensorSummary(1);
    auto result = GraphTruthValidator::validateInputContract(summary, 8);

    EXPECT_TRUE(result.contract_satisfied);
    EXPECT_THAT(result.warnings, ::testing::Not(::testing::IsEmpty()));
    EXPECT_THAT(result.warnings, ::testing::Contains(::testing::HasSubstr("only one candidate")));
}

// ============================================================================
// ProvenanceRecord Assembly Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, AssembleProvenanceWithValidInput) {
    auto summary = createValidTensorSummary(2);
    std::vector<std::string> supporting_nodes = {"node-1", "node-2"};
    std::unordered_map<std::string, double> layer_confidence = {
        {"ANN", 0.85}, {"Tensor", 0.82}, {"Graph", 0.80}
    };

    auto provenance = GraphTruthValidator::assembleProvenance(
        "candidate-123",
        summary,
        supporting_nodes,
        "alice@example.com",
        layer_confidence
    );

    EXPECT_EQ(provenance.evidence_id, "candidate-123");
    EXPECT_EQ(provenance.source_layer, "ANN");
    EXPECT_EQ(provenance.validation_principal, "alice@example.com");
    EXPECT_TRUE(provenance.provenance_complete);
    EXPECT_EQ(provenance.supporting_nodes, supporting_nodes);
    EXPECT_THAT(provenance.layer_decisions, ::testing::ElementsAre("ANN", "Tensor", "Graph"));
}

TEST_F(GraphTruthValidatorTest, AssembleProvenanceWithEmptySupportingNodes) {
    auto summary = createValidTensorSummary(1);
    std::vector<std::string> supporting_nodes;  // Empty
    std::unordered_map<std::string, double> layer_confidence;

    auto provenance = GraphTruthValidator::assembleProvenance(
        "candidate-456",
        summary,
        supporting_nodes,
        "bob@example.com",
        layer_confidence
    );

    EXPECT_FALSE(provenance.provenance_complete);
    EXPECT_THAT(provenance.supporting_nodes, ::testing::IsEmpty());
}

// ============================================================================
// EvidenceBundle Assembly Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, AssembleEvidenceBundleWithValidEvidence) {
    GraphTruthEvidence evidence;
    evidence.candidate_id = "cand-789";
    evidence.graph_score = 0.75;
    evidence.tensor_score = 0.80;
    evidence.validated = true;
    evidence.supporting_nodes = {"node-a", "node-b"};

    ProvenanceRecord provenance;
    provenance.evidence_id = "cand-789";
    provenance.provenance_complete = true;

    AclValidationResult acl_result;
    acl_result.acl_passed = true;
    acl_result.principal = "user@example.com";

    std::vector<MultiHopValidationResult> multi_hop;

    auto bundle = GraphTruthValidator::assembleEvidenceBundle(
        evidence, multi_hop, provenance, acl_result
    );

    EXPECT_EQ(bundle.candidate_id, "cand-789");
    EXPECT_TRUE(bundle.ready_for_llm);
    EXPECT_GT(bundle.combined_confidence, 0.0);
    EXPECT_FALSE(bundle.llm_context.empty());
    EXPECT_THAT(bundle.metadata, ::testing::Contains(
        ::testing::Pair("acl_passed", "true")));
}

TEST_F(GraphTruthValidatorTest, AssembleEvidenceBundleWithFailedAcl) {
    GraphTruthEvidence evidence;
    evidence.candidate_id = "cand-999";
    evidence.validated = true;

    ProvenanceRecord provenance;
    provenance.provenance_complete = true;

    AclValidationResult acl_result;
    acl_result.acl_passed = false;  // Policy rejection

    std::vector<MultiHopValidationResult> multi_hop;

    auto bundle = GraphTruthValidator::assembleEvidenceBundle(
        evidence, multi_hop, provenance, acl_result
    );

    EXPECT_FALSE(bundle.ready_for_llm);  // Not ready due to failed ACL
}

// ============================================================================
// AclValidationResult Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, ValidateAclWithNoEngineConfigured) {
    // When no policy engine is set, should degrade to fail-open (warning)
    auto result = validator_->validateAcl(
        "candidate-001",
        "alice@example.com",
        "read",
        {}
    );

    // Current implementation: stub returns fail-open
    EXPECT_TRUE(result.acl_passed);
    EXPECT_EQ(result.principal, "alice@example.com");
    EXPECT_EQ(result.resource_id, "candidate-001");
}

// ============================================================================
// MultiHopValidation Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, ValidateMultiHopRelationshipsWithNoRetrievers) {
    // Without KG retriever configured, returns empty results
    std::vector<std::string> targets = {"node-1", "node-2"};
    auto results = validator_->validateMultiHopRelationships(
        "source-node",
        targets,
        3
    );

    // Current implementation: stub returns empty paths
    for (const auto& result : results) {
        EXPECT_FALSE(result.found_valid_path);
    }
}

// ============================================================================
// Configuration Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, ConfigurationDefaultsAreCorrect) {
    GraphTruthValidatorConfig config;

    EXPECT_EQ(config.max_evidence_candidates, 8);
    EXPECT_TRUE(config.use_ontology_validation);
    EXPECT_DOUBLE_EQ(config.min_graph_truth_score, 0.15);
    EXPECT_TRUE(config.enable_acl_validation);
    EXPECT_TRUE(config.enable_provenance_tracking);
    EXPECT_TRUE(config.enable_multi_hop_validation);
    EXPECT_EQ(config.max_multi_hop_depth, 3);
}

TEST_F(GraphTruthValidatorTest, ConservativeConfigurationIsCorrect) {
    GraphTruthValidatorConfig config;
    config.max_evidence_candidates = 4;
    config.min_graph_truth_score = 0.5;
    config.max_multi_hop_depth = 2;

    EXPECT_EQ(config.max_evidence_candidates, 4);
    EXPECT_DOUBLE_EQ(config.min_graph_truth_score, 0.5);
    EXPECT_EQ(config.max_multi_hop_depth, 2);
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST_F(GraphTruthValidatorTest, InputContractWithInvalidCandidateScores) {
    auto summary = createValidTensorSummary(2);
    summary.similar_adapters[0].score = -0.1;  // Invalid negative score
    auto result = GraphTruthValidator::validateInputContract(summary, 8);

    EXPECT_FALSE(result.contract_satisfied);
    EXPECT_THAT(result.contract_violations,
                ::testing::Contains(::testing::HasSubstr("invalid score")));
}

TEST_F(GraphTruthValidatorTest, ProvenanceWithMixedLayerConfidence) {
    auto summary = createValidTensorSummary(1);
    std::vector<std::string> nodes = {"n1"};
    std::unordered_map<std::string, double> confidence = {
        {"ANN", 0.95}, {"Tensor", 0.85}, {"Graph", 0.70}
    };

    auto provenance = GraphTruthValidator::assembleProvenance(
        "test-id", summary, nodes, "user", confidence
    );

    EXPECT_EQ(provenance.layer_confidence_scores["ANN"], 0.95);
    EXPECT_EQ(provenance.layer_confidence_scores["Tensor"], 0.85);
    EXPECT_EQ(provenance.layer_confidence_scores["Graph"], 0.70);
}

}  // namespace themis::rag::test
