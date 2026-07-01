#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "auth/authorization_policy.h"
#include "rag/graph_truth_validator.h"
#include "rag/knowledge_graph_retriever.h"
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
    // Phase 2: no policy engine → fail-closed (deny) to prevent accidental disclosure.
    auto result = validator_->validateAcl(
        "candidate-001",
        "alice@example.com",
        "read",
        {}
    );

    EXPECT_FALSE(result.acl_passed);
    EXPECT_EQ(result.principal, "alice@example.com");
    EXPECT_EQ(result.resource_id, "candidate-001");
    EXPECT_THAT(result.policy_decision_reason,
                ::testing::HasSubstr("fail_closed"));
}

// ============================================================================
// MultiHopValidation Tests
// ============================================================================

TEST_F(GraphTruthValidatorTest, ValidateMultiHopRelationshipsWithNoRetrievers) {
    // Without KnowledgeGraph injected, all paths are not-found.
    std::vector<std::string> targets = {"node-1", "node-2"};
    auto results = validator_->validateMultiHopRelationships(
        "source-node",
        targets,
        3
    );

    ASSERT_EQ(results.size(), targets.size());
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

// ============================================================================
// Phase 2: ACL / Policy Engine Tests
// ============================================================================

/// Minimal in-process IAuthorizationPolicy stub for unit tests.
class AllowAllPolicy : public auth::IAuthorizationPolicy {
public:
    explicit AllowAllPolicy(std::string id = "allow-all") : id_(std::move(id)) {}

    [[nodiscard]] auth::PolicyEvaluationResult evaluate(
        const auth::SubjectAttributes& /*subject*/,
        const auth::ResourceAttributes& /*resource*/,
        const std::string& /*action*/,
        const auth::EnvironmentAttributes& /*env*/) const override {
        return {auth::PolicyDecision::ALLOW, id_, "allow-all policy", {}};
    }
    [[nodiscard]] std::string policyId() const override { return id_; }
    [[nodiscard]] std::string policyVersion() const override { return "1.0"; }
    [[nodiscard]] bool reload() override { return true; }

private:
    std::string id_;
};

/// Policy that always denies access.
class DenyAllPolicy : public auth::IAuthorizationPolicy {
public:
    explicit DenyAllPolicy(std::string id = "deny-all") : id_(std::move(id)) {}

    [[nodiscard]] auth::PolicyEvaluationResult evaluate(
        const auth::SubjectAttributes& /*subject*/,
        const auth::ResourceAttributes& /*resource*/,
        const std::string& /*action*/,
        const auth::EnvironmentAttributes& /*env*/) const override {
        return {auth::PolicyDecision::DENY, id_, "deny-all policy", {}};
    }
    [[nodiscard]] std::string policyId() const override { return id_; }
    [[nodiscard]] std::string policyVersion() const override { return "1.0"; }
    [[nodiscard]] bool reload() override { return true; }

private:
    std::string id_;
};

/// Policy that returns NOT_APPLICABLE — must fail-closed.
class NotApplicablePolicy : public auth::IAuthorizationPolicy {
public:
    [[nodiscard]] auth::PolicyEvaluationResult evaluate(
        const auth::SubjectAttributes& /*subject*/,
        const auth::ResourceAttributes& /*resource*/,
        const std::string& /*action*/,
        const auth::EnvironmentAttributes& /*env*/) const override {
        return {auth::PolicyDecision::NOT_APPLICABLE, "na-policy", "no opinion", {}};
    }
    [[nodiscard]] std::string policyId() const override { return "na-policy"; }
    [[nodiscard]] std::string policyVersion() const override { return "1.0"; }
    [[nodiscard]] bool reload() override { return true; }
};

TEST_F(GraphTruthValidatorTest, ValidateAclAllowsWithPermissivePolicy) {
    validator_->setAuthorizationPolicy(std::make_shared<AllowAllPolicy>());

    auto result = validator_->validateAcl("candidate-42", "bob@example.com", "read", {});

    EXPECT_TRUE(result.acl_passed);
    EXPECT_EQ(result.candidate_id, "candidate-42");
    EXPECT_EQ(result.principal, "bob@example.com");
    EXPECT_THAT(result.evaluated_policies, ::testing::Contains("allow-all"));
    EXPECT_THAT(result.granted_by_policies, ::testing::Contains("allow-all"));
    EXPECT_TRUE(result.denied_by_policies.empty());
}

TEST_F(GraphTruthValidatorTest, ValidateAclDeniesWithDenyAllPolicy) {
    validator_->setAuthorizationPolicy(std::make_shared<DenyAllPolicy>());

    auto result = validator_->validateAcl("candidate-007", "eve@example.com", "read", {});

    EXPECT_FALSE(result.acl_passed);
    EXPECT_THAT(result.denied_by_policies, ::testing::Contains("deny-all"));
    EXPECT_TRUE(result.granted_by_policies.empty());
    EXPECT_THAT(result.detail, ::testing::HasSubstr("denied"));
}

TEST_F(GraphTruthValidatorTest, ValidateAclFailsClosedForNotApplicablePolicy) {
    // NOT_APPLICABLE → no engine has an opinion → deny (fail-closed).
    validator_->setAuthorizationPolicy(std::make_shared<NotApplicablePolicy>());

    auto result = validator_->validateAcl("candidate-X", "unknown@example.com", "read", {});

    EXPECT_FALSE(result.acl_passed);
    EXPECT_THAT(result.policy_decision_reason,
                ::testing::HasSubstr("not_applicable_fail_closed"));
}

TEST_F(GraphTruthValidatorTest, ValidateAclPropagatesContextRoleAndTenant) {
    // Verify that role/tenant_id context keys reach the policy via SubjectAttributes.
    // We use AllowAllPolicy — the intent here is to verify no context-mapping crash.
    validator_->setAuthorizationPolicy(std::make_shared<AllowAllPolicy>());

    std::unordered_map<std::string, std::string> ctx{
        {"role", "analyst"}, {"tenant_id", "acme-corp"}, {"classification", "confidential"}};

    auto result = validator_->validateAcl("evidence-99", "carol@example.com", "use_in_generation", ctx);

    EXPECT_TRUE(result.acl_passed);
    EXPECT_EQ(result.action, "use_in_generation");
}

// ============================================================================
// Phase 2: Multi-hop BFS Traversal Tests
// ============================================================================

/// Build a minimal KnowledgeGraph with a small fixed topology for testing.
///
///   A --0.9--> B --0.8--> C
///   A --0.5--> D
///
static std::shared_ptr<kg::KnowledgeGraph> makeTestGraph() {
    auto g = std::make_shared<kg::KnowledgeGraph>();

    g->addNode({"A", "Node A", {}, kg::EntityType::CONCEPT, {}});
    g->addNode({"B", "Node B", {}, kg::EntityType::CONCEPT, {}});
    g->addNode({"C", "Node C", {}, kg::EntityType::CONCEPT, {}});
    g->addNode({"D", "Node D", {}, kg::EntityType::CONCEPT, {}});

    g->addEdge({"A", "B", kg::RelationType::RELATED_TO, 0.9});
    g->addEdge({"B", "C", kg::RelationType::RELATED_TO, 0.8});
    g->addEdge({"A", "D", kg::RelationType::RELATED_TO, 0.5});

    return g;
}

TEST_F(GraphTruthValidatorTest, MultiHopFindsDirectNeighbor) {
    validator_->setKnowledgeGraph(makeTestGraph());

    auto results = validator_->validateMultiHopRelationships("A", {"B"}, 2);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].found_valid_path);
    EXPECT_EQ(results[0].depth, 1u);
    EXPECT_DOUBLE_EQ(results[0].path_confidence, 0.9);
    ASSERT_GE(results[0].hop_path.size(), 2u);
    EXPECT_EQ(results[0].hop_path.front(), "A");
    EXPECT_EQ(results[0].hop_path.back(), "B");
}

TEST_F(GraphTruthValidatorTest, MultiHopFindsTwoHopPath) {
    validator_->setKnowledgeGraph(makeTestGraph());

    auto results = validator_->validateMultiHopRelationships("A", {"C"}, 3);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].found_valid_path);
    EXPECT_EQ(results[0].depth, 2u);
    EXPECT_DOUBLE_EQ(results[0].path_confidence, 0.9 * 0.8);
    EXPECT_THAT(results[0].relationship_chain, ::testing::HasSubstr("A"));
    EXPECT_THAT(results[0].relationship_chain, ::testing::HasSubstr("C"));
}

TEST_F(GraphTruthValidatorTest, MultiHopReturnsNotFoundForUnreachableTarget) {
    validator_->setKnowledgeGraph(makeTestGraph());

    auto results = validator_->validateMultiHopRelationships("A", {"Z"}, 3);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].found_valid_path);
    EXPECT_EQ(results[0].depth, 0u);
    EXPECT_DOUBLE_EQ(results[0].path_confidence, 0.0);
}

TEST_F(GraphTruthValidatorTest, MultiHopRespectsDepthLimit) {
    validator_->setKnowledgeGraph(makeTestGraph());

    // C is 2 hops away; depth limit of 1 should not find it.
    auto results = validator_->validateMultiHopRelationships("A", {"C"}, 1);

    ASSERT_EQ(results.size(), 1u);
    EXPECT_FALSE(results[0].found_valid_path);
}

TEST_F(GraphTruthValidatorTest, MultiHopHandlesMultipleTargets) {
    validator_->setKnowledgeGraph(makeTestGraph());

    // B is 1 hop, C is 2 hops, Z is unreachable.
    auto results = validator_->validateMultiHopRelationships("A", {"B", "C", "Z"}, 3);

    ASSERT_EQ(results.size(), 3u);
    EXPECT_TRUE(results[0].found_valid_path);   // B found
    EXPECT_TRUE(results[1].found_valid_path);   // C found
    EXPECT_FALSE(results[2].found_valid_path);  // Z not found
}

TEST_F(GraphTruthValidatorTest, MultiHopPreservesResultOrderMatchingInputTargets) {
    validator_->setKnowledgeGraph(makeTestGraph());

    // Reversed order vs. graph layout.
    auto results = validator_->validateMultiHopRelationships("A", {"C", "B"}, 3);

    ASSERT_EQ(results.size(), 2u);
    // First result must correspond to "C" (2 hops), second to "B" (1 hop).
    EXPECT_EQ(results[0].depth, 2u);  // C
    EXPECT_EQ(results[1].depth, 1u);  // B
}

}  // namespace themis::rag::test
