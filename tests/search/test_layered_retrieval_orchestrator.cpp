/**
 * @file test_layered_retrieval_orchestrator.cpp
 * @brief Comprehensive tests for the LayeredRetrievalOrchestrator.
 *
 * Tests cover:
 * - All four layers in sequence
 * - Error handling and fallback strategies
 * - End-to-end retrieval pipeline
 * - Layer interaction and data flow
 * - Configuration and observability
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "search/layered_retrieval_orchestrator.h"
#include "index/ann_frontdoor.h"
#include "tensor/tensor_mid_layer.h"
#include "rag/graph_truth_validator.h"
#include "llm/final_layer_orchestrator.h"

namespace themis {
namespace search {
namespace {

using ::testing::Return;
using ::testing::ReturnRef;
using ::testing::NiceMock;

// ============================================================================
// Mock Layer Components
// ============================================================================

class MockAnnFrontdoor : public index::AnnFrontdoor {
public:
    MOCK_METHOD(index::AnnFrontdoorResult, search,
                (const float*, std::size_t, int, const index::AnnQueryContext&),
                (const));
    MOCK_METHOD(index::AnnStrategy, planStrategy,
                (const index::AnnQueryContext&), (const, noexcept));
    MOCK_METHOD(index::AnnRetrievalPlan, planRetrieval,
                (const index::AnnQueryContext&), (const, noexcept));
    MOCK_METHOD(std::string, explainStrategy,
                (const index::AnnQueryContext&), (const));
};

class MockTensorMidLayer : public tensor::TensorMidLayer {
public:
    MOCK_METHOD(tensor::TensorLayerPlan, plan,
                (const tensor::TensorLayerContext&), (const, noexcept));
    MOCK_METHOD(tensor::TensorLayerSummary, summarize,
                (const tensor::TensorLayerContext&), (const));
};

class MockGraphTruthValidator : public rag::GraphTruthValidator {
public:
    MOCK_METHOD(rag::GraphTruthValidationResult, validate,
                (const std::string&, const tensor::TensorLayerSummary&,
                 const rag::GraphTruthValidatorConfig&, const std::string&), (const));
};

class MockFinalLayerOrchestrator : public llm::FinalLayerOrchestrator {
public:
    MOCK_METHOD(llm::FinalLayerResolution, resolve,
                (const llm::FinalLayerRequest&), (const));
};

// ============================================================================
// Test Fixtures
// ============================================================================

class LayeredRetrievalOrchestratorTest : public ::testing::Test {
protected:
    void SetUp() override {
        orchestrator_ = std::make_shared<LayeredRetrievalOrchestrator>();

        // Create mock layers
        ann_frontdoor_ = std::make_shared<NiceMock<MockAnnFrontdoor>>();
        tensor_layer_ = std::make_shared<NiceMock<MockTensorMidLayer>>();
        graph_validator_ = std::make_shared<NiceMock<MockGraphTruthValidator>>();
        final_layer_ = std::make_shared<NiceMock<MockFinalLayerOrchestrator>>();

        // Inject layers
        orchestrator_->setAnnFrontdoor(ann_frontdoor_);
        orchestrator_->setTensorMidLayer(tensor_layer_);
        orchestrator_->setGraphTruthValidator(graph_validator_);
        orchestrator_->setFinalLayerOrchestrator(final_layer_);

        // Set default configuration
        LayeredRetrievalConfig config;
        config.enable_ann_layer = true;
        config.enable_tensor_layer = true;
        config.enable_graph_layer = true;
        config.enable_llm_layer = true;
        config.ann_k = 100;
        config.tensor_top_k = 50;
        config.graph_top_k = 20;
        orchestrator_->setConfig(config);
    }

    // Helper to create test query vector
    std::vector<float> createTestVector(size_t dim) {
        std::vector<float> vec(dim);
        for (size_t i = 0; i < dim; ++i) {
            vec[i] = 0.1f + (i % 10) * 0.01f;
        }
        return vec;
    }

    // Helper to create test ANN result
    index::AnnFrontdoorResult createTestAnnResult(size_t count = 10) {
        index::AnnFrontdoorResult result;
        result.strategy_used = index::AnnStrategy::HNSW;
        result.routing_reason = "Test routing";
        result.routing_reason_code = "test-hnsw";
        
        for (size_t i = 0; i < count; ++i) {
            index::AnnCandidate candidate;
            candidate.id = i;
            candidate.distance = 0.1f + i * 0.01f;
            result.candidates.push_back(candidate);
        }
        return result;
    }

    // Helper to create test tensor summary
    tensor::TensorLayerSummary createTestTensorSummary(size_t candidate_count = 5) {
        tensor::TensorLayerSummary summary;
        summary.scope_key = "test-scope";
        summary.layer_kind = tensor::TensorLayerKind::Adapter;
        summary.routing_reason = "Test tensor routing";
        summary.candidate_count = candidate_count;
        return summary;
    }

    // Helper to create test graph validation result
    rag::GraphTruthValidationResult createTestGraphResult() {
        rag::GraphTruthValidationResult result;
        result.valid = true;
        result.reasoning_chain = "Test reasoning";
        
        // Add some test evidence
        rag::GraphTruthEvidence evidence;
        evidence.candidate_id = "test-doc-1";
        evidence.graph_score = 0.95;
        evidence.validated = true;
        result.evidence.push_back(evidence);
        
        return result;
    }

    std::shared_ptr<LayeredRetrievalOrchestrator> orchestrator_;
    std::shared_ptr<NiceMock<MockAnnFrontdoor>> ann_frontdoor_;
    std::shared_ptr<NiceMock<MockTensorMidLayer>> tensor_layer_;
    std::shared_ptr<NiceMock<MockGraphTruthValidator>> graph_validator_;
    std::shared_ptr<NiceMock<MockFinalLayerOrchestrator>> final_layer_;
};

// ============================================================================
// Test Cases: Happy Path
// ============================================================================

TEST_F(LayeredRetrievalOrchestratorTest, CompleteLayeredRetrievalSucceeds) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    auto tensor_summary = createTestTensorSummary(5);
    auto graph_result = createTestGraphResult();

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    llm_resolution.model_id = "test-model";
    llm_resolution.package_id = "test-package";

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    LayeredRetrievalContext context;
    context.query_text = "test query";
    context.correlation_id = "test-corr-123";

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query", context);

    // Assert
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layer_decisions.size(), 4);
    EXPECT_EQ(result.layer_decisions[0].layer_name, "ANN");
    EXPECT_EQ(result.layer_decisions[1].layer_name, "Tensor");
    EXPECT_EQ(result.layer_decisions[2].layer_name, "Graph");
    EXPECT_EQ(result.layer_decisions[3].layer_name, "LLM");
    EXPECT_TRUE(result.layer_decisions[0].success);
    EXPECT_TRUE(result.layer_decisions[1].success);
    EXPECT_TRUE(result.layer_decisions[2].success);
    EXPECT_TRUE(result.layer_decisions[3].success);
    EXPECT_EQ(result.correlation_id, "test-corr-123");
    EXPECT_FALSE(result.final_answer.empty());
}

TEST_F(LayeredRetrievalOrchestratorTest, AnnLayerProducesValidCandidates) {
    // Arrange
    auto query_vec = createTestVector(256);
    auto ann_result = createTestAnnResult(20);
    auto tensor_summary = createTestTensorSummary(10);
    auto graph_result = createTestGraphResult();

    EXPECT_CALL(*ann_frontdoor_, search).WillOnce(Return(ann_result));
    EXPECT_CALL(*tensor_layer_, summarize).WillOnce(Return(tensor_summary));
    EXPECT_CALL(*graph_validator_, validate).WillOnce(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    EXPECT_CALL(*final_layer_, resolve).WillOnce(Return(llm_resolution));

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.layer_decisions[0].layer_name, "ANN");
    EXPECT_TRUE(result.layer_decisions[0].success);
}

TEST_F(LayeredRetrievalOrchestratorTest, EvidenceBundlePopulated) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    auto tensor_summary = createTestTensorSummary(5);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    EXPECT_EQ(result.evidence_bundle.size(), 1);
    EXPECT_EQ(result.evidence_bundle[0].candidate_id, "test-doc-1");
    EXPECT_EQ(result.evidence_bundle[0].graph_score, 0.95);
}

// ============================================================================
// Test Cases: Error Handling
// ============================================================================

TEST_F(LayeredRetrievalOrchestratorTest, AnnLayerFailureFallback) {
    // Arrange
    auto query_vec = createTestVector(128);
    index::AnnFrontdoorResult empty_result;
    auto tensor_summary = createTestTensorSummary(0);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(empty_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    LayeredRetrievalConfig config;
    config.fail_closed_on_graph_error = false;  // Allow graceful degradation
    orchestrator_->setConfig(config);

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    EXPECT_FALSE(result.layer_decisions[0].success);
}

TEST_F(LayeredRetrievalOrchestratorTest, TensorLayerFallbackToAnn) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    tensor::TensorLayerSummary empty_summary;
    empty_summary.candidate_count = 0;
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(empty_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    LayeredRetrievalConfig config;
    config.fail_closed_on_graph_error = false;
    config.allow_layer_fallback = true;
    orchestrator_->setConfig(config);

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    // Should continue to graph layer despite tensor layer failure
    EXPECT_GE(result.layer_decisions.size(), 3);
}

TEST_F(LayeredRetrievalOrchestratorTest, FallbackAnswerGenerated) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    auto tensor_summary = createTestTensorSummary(5);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    // LLM layer fails to resolve
    llm::FinalLayerResolution failed_resolution;
    failed_resolution.resolved = false;
    failed_resolution.errors.push_back("Test LLM failure");
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(failed_resolution));

    LayeredRetrievalConfig config;
    config.fail_closed_on_graph_error = false;
    orchestrator_->setConfig(config);

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    EXPECT_TRUE(result.success);  // Should gracefully degrade
    EXPECT_FALSE(result.final_answer.empty());
    EXPECT_THAT(result.final_answer, ::testing::HasSubstr("relevant"));
}

// ============================================================================
// Test Cases: Configuration
// ============================================================================

TEST_F(LayeredRetrievalOrchestratorTest, LayerCanBeDisabledViaConfig) {
    // Arrange
    auto query_vec = createTestVector(128);
    LayeredRetrievalConfig config;
    config.enable_tensor_layer = false;  // Disable tensor layer
    config.enable_llm_layer = false;     // Disable LLM layer
    orchestrator_->setConfig(config);

    auto ann_result = createTestAnnResult(10);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    EXPECT_TRUE(result.layer_decisions[0].success);  // ANN executed
    EXPECT_FALSE(result.layer_decisions[1].success); // Tensor disabled
    EXPECT_TRUE(result.layer_decisions[2].success);  // Graph executed
    EXPECT_FALSE(result.layer_decisions[3].success); // LLM disabled
}

TEST_F(LayeredRetrievalOrchestratorTest, HealthCheckReportsAllLayers) {
    // Act
    bool healthy = orchestrator_->isHealthy();

    // Assert
    EXPECT_TRUE(healthy);

    // Now remove a layer
    orchestrator_->setAnnFrontdoor(nullptr);
    EXPECT_FALSE(orchestrator_->isHealthy());
}

TEST_F(LayeredRetrievalOrchestratorTest, StatusReportAccurate) {
    // Act
    auto status = orchestrator_->statusReport();

    // Assert
    EXPECT_THAT(status, ::testing::HasSubstr("ANN Layer"));
    EXPECT_THAT(status, ::testing::HasSubstr("Tensor Layer"));
    EXPECT_THAT(status, ::testing::HasSubstr("Graph Layer"));
    EXPECT_THAT(status, ::testing::HasSubstr("LLM Layer"));
    EXPECT_THAT(status, ::testing::HasSubstr("HEALTHY"));
}

// ============================================================================
// Test Cases: Observability
// ============================================================================

TEST_F(LayeredRetrievalOrchestratorTest, CorrelationIdPropagated) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    auto tensor_summary = createTestTensorSummary(5);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    LayeredRetrievalContext context;
    context.correlation_id = "my-trace-id-12345";

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query", context);

    // Assert
    EXPECT_EQ(result.correlation_id, "my-trace-id-12345");
}

TEST_F(LayeredRetrievalOrchestratorTest, LatencyMeasured) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    auto tensor_summary = createTestTensorSummary(5);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "test query");

    // Assert
    EXPECT_GT(result.total_latency_ms.count(), 0);
    for (const auto& decision : result.layer_decisions) {
        EXPECT_GE(decision.elapsed_ms.count(), 0);
    }
}

// ============================================================================
// Test Cases: Edge Cases
// ============================================================================

TEST_F(LayeredRetrievalOrchestratorTest, NullQueryVectorHandled) {
    // Act & Assert: Should handle gracefully
    auto result = orchestrator_->execute(nullptr, 128, "test query");
    
    // Expect error handling
    EXPECT_FALSE(result.layer_decisions[0].success);
}

TEST_F(LayeredRetrievalOrchestratorTest, ZeroDimensionHandled) {
    // Arrange
    std::vector<float> query_vec(128);

    // Act
    auto result = orchestrator_->execute(query_vec.data(), 0, "test query");

    // Assert
    EXPECT_FALSE(result.layer_decisions[0].success);
}

TEST_F(LayeredRetrievalOrchestratorTest, EmptyQueryTextHandled) {
    // Arrange
    auto query_vec = createTestVector(128);
    auto ann_result = createTestAnnResult(10);
    auto tensor_summary = createTestTensorSummary(5);
    auto graph_result = createTestGraphResult();

    ON_CALL(*ann_frontdoor_, search).WillByDefault(Return(ann_result));
    ON_CALL(*tensor_layer_, summarize).WillByDefault(Return(tensor_summary));
    ON_CALL(*graph_validator_, validate).WillByDefault(Return(graph_result));

    llm::FinalLayerResolution llm_resolution;
    llm_resolution.resolved = true;
    ON_CALL(*final_layer_, resolve).WillByDefault(Return(llm_resolution));

    // Act
    auto result = orchestrator_->execute(query_vec.data(), query_vec.size(),
                                         "");  // Empty query text

    // Assert
    EXPECT_TRUE(result.success);  // Should handle gracefully
}

} // namespace
} // namespace search
} // namespace themis
