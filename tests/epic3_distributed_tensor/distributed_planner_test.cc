/**
 * @file distributed_planner_test.cc
 * @brief Tests for distributed tensor retrieval planner.
 *
 * Test coverage:
 * - Stale shard summary rejection
 * - Exact-on-demand fragment fetch after summary-first routing
 * - Graph override validation before finalization
 * - No summary-only truth result enforcement
 * - Distributed fan-out reduction without correctness loss
 */

#include <gtest/gtest.h>
#include <algorithm>
#include "distributed_tensor/artifact_manifest.h"
#include "distributed_tensor/distributed_planner.h"
#include "distributed_tensor/graph_summary_projection.h"
#include "src/distributed_tensor/include/artifact_manifest.h"
#include "src/distributed_tensor/include/distributed_planner.h"
#include "tensor/tensor_summary_types.h"
#include "rag/graph_truth_validator.h"

#include <memory>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <type_traits>

static_assert(!std::is_same_v<
              themis::distributed_tensor::ArtifactClass,
              themis::distributed_tensor::detail::ArtifactClass>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::ArtifactManifest,
              themis::distributed_tensor::detail::ArtifactManifest>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::RoutingSummary,
              themis::distributed_tensor::detail::RoutingSummary>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::TensorRetrievalPlan,
              themis::distributed_tensor::detail::TensorRetrievalPlan>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::FragmentLoadRequest,
              themis::distributed_tensor::detail::FragmentLoadRequest>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::FragmentLoadResult,
              themis::distributed_tensor::detail::FragmentLoadResult>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::IFragmentFetcher,
              themis::distributed_tensor::detail::IFragmentFetcher>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::DistributedTensorPlanner,
              themis::distributed_tensor::detail::DistributedTensorPlanner>);
static_assert(!std::is_same_v<
              themis::distributed_tensor::DistributedTensorPlanner::Config,
              themis::distributed_tensor::detail::DistributedTensorPlanner::Config>);

namespace themis {
namespace distributed_tensor {
namespace testing {

// ============================================================================
// Mock Implementations
// ============================================================================

/**
 * @brief Mock fragment fetcher for testing.
 */
class MockFragmentFetcher : public IFragmentFetcher {
public:
    MockFragmentFetcher() = default;

    void setShouldFail(bool fail) { should_fail_ = fail; }
    void setLoadLatency(float latency_ms) { load_latency_ms_ = latency_ms; }

    [[nodiscard]] FragmentLoadResult fetchFragment(
        const FragmentLoadRequest& request,
        const std::string& correlation_id = {}) const noexcept override {
        
        FragmentLoadResult result;
        result.shard_id = request.shard_id;
        result.artifact_id = request.artifact_id;
        result.success = !should_fail_;
        result.load_latency_ms = load_latency_ms_;
        result.loaded_at = std::to_string(std::time(nullptr));

        if (should_fail_) {
            result.error_reason = "Mock failure for testing";
        } else {
            result.fragment_data.push_back(0xDE);
            result.fragment_data.push_back(0xAD);
            result.content_hash = "test_hash_" + request.shard_id;
        }

        return result;
    }

    [[nodiscard]] std::vector<FragmentLoadResult> fetchFragments(
        const std::vector<FragmentLoadRequest>& requests,
        const std::string& correlation_id = {}) const noexcept override {
        
        std::vector<FragmentLoadResult> results = {};

        for (const auto& req : requests) {
            results.push_back(fetchFragment(req, correlation_id));
        }
        return results;
    }

private:
    bool should_fail_ = false;
    float load_latency_ms_ = 10.0f;
};

// ============================================================================
// Test Fixtures
// ============================================================================

class DistributedPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner_ = std::make_unique<DistributedTensorPlanner>();
        fetcher_ = std::make_shared<MockFragmentFetcher>();
        planner_->setFragmentFetcher(fetcher_);
    }

    std::shared_ptr<rag::kg::KnowledgeGraph> graph_;
    std::unique_ptr<DistributedTensorPlanner> planner_;
    std::shared_ptr<MockFragmentFetcher> fetcher_;

    /**
     * @brief Create a fresh shard summary.
     */
    tensor::ShardSummary createFreshSummary(
        const std::string& shard_id = "shard_0",
        float relevance = 0.9f) {
        
        // Get current ISO-8601 timestamp
        auto now = std::chrono::system_clock::now();
        auto time_t_now = std::chrono::system_clock::to_time_t(now);
        std::ostringstream oss = {};
        oss << std::put_time(std::gmtime(&time_t_now), "%Y-%m-%dT%H:%M:%SZ");
        std::string timestamp = oss.str();
        
        tensor::ShardSummary summary;
        summary.shard_id = shard_id;
        summary.id = shard_id;
        summary.candidates_before_compression = 100;
        summary.candidates_after_compression = 10;
        summary.shard_relevance = relevance;
        summary.shard_healthy = true;
        summary.freshness_state = tensor::SummaryFreshnessState::FRESH;
        summary.freshness_ttl_seconds = 3600;
        summary.created_at = timestamp;
        summary.last_update_timestamp = timestamp;
        
        return summary;
    }

    /**
     * @brief Create a stale shard summary.
     */
    tensor::ShardSummary createStaleSummary(
        const std::string& shard_id = "shard_1") {
        
        auto summary = createFreshSummary(shard_id, 0.5f);
        summary.freshness_state = tensor::SummaryFreshnessState::STALE;
        summary.markAsStale();
        return summary;
    }

    /**
     * @brief Create an invalid shard summary.
     */
    tensor::ShardSummary createInvalidSummary(
        const std::string& shard_id = "shard_2") {
        
        auto summary = createFreshSummary(shard_id, 0.3f);
        summary.freshness_state = tensor::SummaryFreshnessState::INVALID;
        summary.markAsInvalid();
        return summary;
    }
};

// ============================================================================
// Test Cases
// ============================================================================

/**
 * @brief Test 1: Stale shard summary rejection
 *
 * Verifies that the planner correctly identifies and rejects stale summaries
 * during the summary-first routing phase.
 */
TEST_F(DistributedPlannerTest, RejectStaleSummaries) {
    // Create mixed fresh and stale summaries
    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("fresh_0", 0.95f));
    summaries.push_back(createStaleSummary("stale_0"));
    summaries.push_back(createFreshSummary("fresh_1", 0.85f));
    summaries.push_back(createInvalidSummary("invalid_0"));

    // Plan summary-first routing
    auto routing = planner_->planSummaryFirstRouting(summaries);

    // Verify results
    EXPECT_EQ(routing.size(), 4);

    // Fresh summaries should not be rejected
    EXPECT_FALSE(routing[0].rejected_as_stale);
    EXPECT_FALSE(routing[0].rejected_as_unhealthy);

    // Stale summary should be rejected
    EXPECT_TRUE(routing[1].rejected_as_stale);
    EXPECT_EQ(routing[1].freshness_state, tensor::SummaryFreshnessState::STALE);

    // Fresh summary should not be rejected
    EXPECT_FALSE(routing[2].rejected_as_stale);

    // Invalid summary should be rejected
    EXPECT_TRUE(routing[3].rejected_as_stale);
    EXPECT_EQ(routing[3].freshness_state, tensor::SummaryFreshnessState::INVALID);
}

/**
 * @brief Test 2: Exact-on-demand fragment fetch after routing
 *
 * Verifies that after summary-first routing identifies valid shards,
 * the planner correctly plans exact-on-demand fragment loading.
 */
TEST_F(DistributedPlannerTest, PlanExactOnDemandFragmentLoading) {
    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("shard_0", 0.95f));
    summaries.push_back(createFreshSummary("shard_1", 0.80f));
    summaries.push_back(createStaleSummary("shard_2"));

    // Plan routing
    auto routing = planner_->planSummaryFirstRouting(summaries);

    // Plan fragment loading
    auto requests = planner_->planExactOnDemandLoading(routing);

    // Verify: only fresh shards are included in fragment requests
    EXPECT_EQ(requests.size(), 2);  // shard_0 and shard_1
    EXPECT_EQ(requests[0].shard_id, "shard_0");
    EXPECT_EQ(requests[1].shard_id, "shard_1");

    // High-relevance shard should be expedited
    EXPECT_TRUE(requests[0].expedited);  // shard_0 has relevance 0.95
    EXPECT_FALSE(requests[1].expedited);  // shard_1 has relevance 0.80
}

/**
 * @brief Test 3: Graph-truth validation fails closed before finalization
 *
 * Verifies that raw mock bytes and non-manifest-bound requests remain
 * fail-closed instead of being treated as graph candidates.
 */
TEST_F(DistributedPlannerTest, GraphTruthValidationFailsClosedBeforeFinalization) {
    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("shard_0", 0.9f));
    planner_->setGraphValidator(std::make_shared<rag::GraphTruthValidator>());

    // Build plan
    auto plan = planner_->buildRetrievalPlan(summaries, "test_query");

    // Initially, plan has not passed graph validation
    EXPECT_FALSE(plan.passed_graph_validation);

    // Execute fragment loads
    plan.fragment_results = planner_->executeFragmentLoads(plan.fragment_requests);

    const auto validation =
        planner_->validateAgainstGraphTruth(plan, "test_query");
    EXPECT_EQ(validation.routing_reason_code,
              "FRAGMENT_REQUEST_UNBOUND");
    EXPECT_FALSE(plan.passed_graph_validation);

    // Transport success alone cannot satisfy graph-truth validation.
    EXPECT_FALSE(planner_->isValidFinalPlan(plan));
}

/**
 * @brief Test 4: No summary-only truth result
 *
 * Verifies that a result using only summaries without exact fragments
 * is rejected, enforcing the no-summary-only-truth rule.
 */
TEST_F(DistributedPlannerTest, RejectSummaryOnlyResults) {
    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("shard_0", 0.9f));

    // Build a plan without executing fragment loads
    auto plan = planner_->buildRetrievalPlan(summaries, "test_query");

    // Set config to require strict graph validation
    auto config = planner_->config();
    config.strict_graph_validation = true;
    planner_->setConfig(config);

    // Plan should be invalid because:
    // 1. Fragment results are empty
    // 2. Graph validation hasn't passed
    EXPECT_FALSE(planner_->isValidFinalPlan(plan));
}

/**
 * @brief Test 5: Distributed fan-out reduction without claiming graph truth
 *
 * Verifies that stale summaries reduce fragment-fetch fan-out while successful
 * transport remains distinct from graph-truth validation.
 */
TEST_F(DistributedPlannerTest, ReduceFanOutWithoutClaimingGraphValidation) {
    // Create 10 summaries: 5 fresh, 5 stale
    std::vector<tensor::ShardSummary> summaries = {};

    for (int i = 0; i < 5; ++i) {
        summaries.push_back(createFreshSummary("fresh_" + std::to_string(i), 0.7f));
    }
    for (int i = 0; i < 5; ++i) {
        summaries.push_back(createStaleSummary("stale_" + std::to_string(i)));
    }

    // Build plan
    auto plan = planner_->buildRetrievalPlan(summaries, "test_query");

    // Verify fan-out was reduced
    EXPECT_EQ(plan.stale_shards_rejected, 5);
    EXPECT_EQ(plan.routing_summaries.size(), 10);
    
    // Count non-rejected shards in fragment requests
    std::size_t fragment_requests_count = 0;
    for (const auto& req : plan.fragment_requests) {
        fragment_requests_count++;
    }
    EXPECT_EQ(fragment_requests_count, 5);  // Only fresh shards

    // Execute fragment loads
    plan.fragment_results = planner_->executeFragmentLoads(plan.fragment_requests);

    // All fragments should load successfully
    for (const auto& result : plan.fragment_results) {
        EXPECT_TRUE(result.success);
    }

    // Successful transport is not graph-truth evidence.
    EXPECT_FALSE(plan.passed_graph_validation);
    EXPECT_FALSE(planner_->isValidFinalPlan(plan));
}

/**
 * @brief Test 6: Handle missing dependencies gracefully
 *
 * Verifies that planner handles missing fragment fetcher and graph validator
 * without crashing.
 */
TEST_F(DistributedPlannerTest, HandleMissingDependenciesGracefully) {
    auto planner_no_deps = std::make_unique<DistributedTensorPlanner>();
    
    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("shard_0", 0.9f));

    // Should not crash when fragment fetcher is missing
    auto requests = planner_no_deps->planExactOnDemandLoading(
        planner_no_deps->planSummaryFirstRouting(summaries));
    
    auto results = planner_no_deps->executeFragmentLoads(requests);
    
    // Results should indicate fetcher not configured
    EXPECT_FALSE(results.empty());
    EXPECT_FALSE(results[0].success);
    EXPECT_EQ(results[0].error_reason, "No fragment fetcher configured");
}

TEST_F(DistributedPlannerTest, GraphTruthValidationRejectsUnboundResults) {
    auto validator = std::make_shared<rag::GraphTruthValidator>();
    planner_->setGraphValidator(validator);

    TensorRetrievalPlan public_plan;
    FragmentLoadResult public_fragment;
    public_fragment.success = true;
    public_plan.fragment_results.push_back(public_fragment);
    const auto public_result =
        planner_->validateAgainstGraphTruth(public_plan, "query");
    EXPECT_EQ(public_result.routing_reason_code,
              "NO_FRAGMENTS");

    detail::DistributedTensorPlanner internal_planner;
    internal_planner.setGraphValidator(validator);
    detail::TensorRetrievalPlan internal_plan;
    detail::FragmentLoadResult internal_fragment;
    internal_fragment.success = true;
    internal_plan.fragment_results.push_back(internal_fragment);
    const auto internal_result =
        internal_planner.validateAgainstGraphTruth(
            internal_plan, "query", rag::GraphTruthValidatorConfig{});
    EXPECT_EQ(internal_result.routing_reason_code,
              "FRAGMENT_PROJECTION_UNAVAILABLE");
    EXPECT_FALSE(internal_planner.isValidFinalPlan(internal_plan));
}

TEST_F(DistributedPlannerTest, GraphSummaryProjectionCodecIsVersionedAndBounded) {
    tensor::TensorLayerSummary summary;
    summary.scope_key = "__adapters__:tenant-a:science:model-a";
    summary.layer_kind = tensor::TensorLayerKind::FingerprintSummary;
    summary.ann_scope_kind = index::AnnScopeKind::Adapter;
    summary.similar_adapters = {
        {"adapter-z", "science", "model-a", 0.6f},
        {"adapter-a", "science", "model-a", 0.9f},
    };
    summary.candidate_count = summary.similar_adapters.size();

    const auto encoded = GraphSummaryProjectionCodec::encode(summary);
    auto decoded = GraphSummaryProjectionCodec::decode(encoded);
    ASSERT_TRUE(decoded.ok());
    EXPECT_EQ(decoded.summary->scope_key, summary.scope_key);
    ASSERT_EQ(decoded.summary->similar_adapters.size(), 2U);
    EXPECT_EQ(decoded.summary->similar_adapters[0].adapter_key, "adapter-a");
    EXPECT_FLOAT_EQ(decoded.summary->similar_adapters[0].score, 0.9f);

    auto unsupported_format = encoded;
    unsupported_format[0] = 'X';
    EXPECT_EQ(GraphSummaryProjectionCodec::decode(unsupported_format).error,
              GraphSummaryProjectionError::UnsupportedFormat);

    auto unsupported_version = encoded;
    unsupported_version[5] = 2;
    EXPECT_EQ(GraphSummaryProjectionCodec::decode(unsupported_version).error,
              GraphSummaryProjectionError::UnsupportedVersion);

    auto trailing_data = encoded;
    trailing_data.push_back(0);
    EXPECT_EQ(GraphSummaryProjectionCodec::decode(trailing_data).error,
              GraphSummaryProjectionError::MalformedPayload);

    auto duplicate_summary = summary;
    duplicate_summary.similar_adapters.push_back(
        {"adapter-m", "science", "model-a", 0.75f});
    duplicate_summary.candidate_count =
        duplicate_summary.similar_adapters.size();
    auto duplicate_candidate_payload =
        GraphSummaryProjectionCodec::encode(duplicate_summary);
    const std::string original_key = "adapter-z";
    const std::string duplicate_key = "adapter-a";
    auto original_key_position = std::search(
        duplicate_candidate_payload.begin(), duplicate_candidate_payload.end(),
        original_key.begin(), original_key.end());
    ASSERT_NE(original_key_position, duplicate_candidate_payload.end());
    std::copy(duplicate_key.begin(), duplicate_key.end(),
              original_key_position);
    EXPECT_EQ(
        GraphSummaryProjectionCodec::decode(duplicate_candidate_payload).error,
        GraphSummaryProjectionError::NonCanonicalPayload);

    auto invalid_summary = summary;
    invalid_summary.similar_adapters[0].score =
        std::numeric_limits<float>::quiet_NaN();
    EXPECT_THROW(GraphSummaryProjectionCodec::encode(invalid_summary),
                 std::invalid_argument);

    tensor::TensorLayerSummary second_summary;
    second_summary.scope_key = summary.scope_key;
    second_summary.similar_adapters = {
        {"adapter-a", "science", "model-a", 0.95f},
    };
    second_summary.candidate_count = 1;
    const std::vector<tensor::TensorLayerSummary> merge_inputs{
        *decoded.summary, second_summary};
    const auto merged = GraphSummaryProjectionCodec::merge(merge_inputs);
    ASSERT_TRUE(merged.ok());
    EXPECT_TRUE(merged.summary->federated);
    EXPECT_EQ(merged.summary->participating_shards, 2U);
    ASSERT_EQ(merged.summary->similar_adapters.size(), 2U);
    EXPECT_FLOAT_EQ(merged.summary->similar_adapters[0].score, 0.95f);
}

TEST_F(DistributedPlannerTest, GraphTruthValidationProjectsVerifiedFragments) {
    graph_ = std::make_shared<rag::kg::KnowledgeGraph>();
    rag::kg::KGNode graph_node;
    graph_node.id = "alpha-node";
    graph_node.canonical_name = "Alpha";
    graph_node.type = rag::kg::EntityType::CONCEPT;
    graph_->addNode(std::move(graph_node));

    auto validator = std::make_shared<rag::GraphTruthValidator>();
    validator->setKnowledgeGraphRetriever(
        std::make_shared<rag::kg::KnowledgeGraphRetriever>(*graph_));
    planner_->setGraphValidator(validator);

    tensor::TensorLayerSummary summary;
    summary.scope_key = "__adapters__:tenant-a:alpha:model-a";
    summary.similar_adapters = {
        {"adapter-alpha", "Alpha", "Alpha", 0.9f},
    };
    summary.candidate_count = 1;
    const auto payload = GraphSummaryProjectionCodec::encode(summary);

    TensorRetrievalPlan plan;
    plan.correlation_id = "graph-projection-test";
    FragmentLoadRequest request;
    request.shard_id = "shard-a";
    request.artifact_id = "artifact-a";
    request.target_node_id = "node-a";
    request.expected_content_hash = std::string(64, 'a');
    request.max_response_bytes = payload.size();
    plan.fragment_requests.push_back(request);

    FragmentLoadResult fragment;
    fragment.shard_id = request.shard_id;
    fragment.artifact_id = request.artifact_id;
    fragment.success = true;
    fragment.content_hash = request.expected_content_hash;
    fragment.fragment_data = payload;
    plan.fragment_results.push_back(std::move(fragment));

    const auto validation = planner_->validateAgainstGraphTruth(plan, "Alpha");
    EXPECT_FALSE(validation.evidences.empty());
    EXPECT_TRUE(plan.passed_graph_validation);
    EXPECT_EQ(plan.validation_result.correlation_id, plan.correlation_id);
    EXPECT_TRUE(planner_->isValidFinalPlan(plan));
}

TEST_F(DistributedPlannerTest, GraphTruthValidationRejectsUnsupportedProjection) {
    auto validator = std::make_shared<rag::GraphTruthValidator>();
    planner_->setGraphValidator(validator);

    TensorRetrievalPlan plan;
    FragmentLoadRequest request;
    request.shard_id = "shard-a";
    request.artifact_id = "artifact-a";
    request.target_node_id = "node-a";
    request.expected_content_hash = std::string(64, 'b');
    request.max_response_bytes = 16;
    plan.fragment_requests.push_back(request);

    FragmentLoadResult fragment;
    fragment.shard_id = request.shard_id;
    fragment.artifact_id = request.artifact_id;
    fragment.success = true;
    fragment.content_hash = request.expected_content_hash;
    fragment.fragment_data.resize(16, 0xde);
    plan.fragment_results.push_back(std::move(fragment));

    const auto validation = planner_->validateAgainstGraphTruth(plan, "query");
    EXPECT_EQ(validation.routing_reason_code,
              "FRAGMENT_PROJECTION_UNSUPPORTED_FORMAT");
    EXPECT_FALSE(plan.passed_graph_validation);
    EXPECT_FALSE(planner_->isValidFinalPlan(plan));
}

TEST_F(DistributedPlannerTest, InternalFinalPlanChecksFragmentIdentityAndSuccessRate) {
    detail::DistributedTensorPlanner internal_planner;
    auto config = internal_planner.config();
    config.strict_graph_validation = false;
    config.min_fragment_success_rate = 0.75f;
    internal_planner.setConfig(config);

    detail::TensorRetrievalPlan plan;
    detail::FragmentLoadRequest first_request;
    first_request.shard_id = "shard-a";
    first_request.artifact_id = "artifact-a";
    plan.fragment_requests.push_back(first_request);
    detail::FragmentLoadRequest second_request;
    second_request.shard_id = "shard-b";
    second_request.artifact_id = "artifact-b";
    plan.fragment_requests.push_back(second_request);

    detail::FragmentLoadResult first_result;
    first_result.shard_id = first_request.shard_id;
    first_result.artifact_id = first_request.artifact_id;
    first_result.success = true;
    plan.fragment_results.push_back(first_result);
    detail::FragmentLoadResult second_result;
    second_result.shard_id = second_request.shard_id;
    second_result.artifact_id = second_request.artifact_id;
    plan.fragment_results.push_back(second_result);

    EXPECT_FALSE(internal_planner.isValidFinalPlan(plan));

    config.min_fragment_success_rate = 0.5f;
    internal_planner.setConfig(config);
    EXPECT_TRUE(internal_planner.isValidFinalPlan(plan));

    plan.fragment_results.pop_back();
    EXPECT_FALSE(internal_planner.isValidFinalPlan(plan));

    plan.fragment_results.push_back(second_result);
    plan.fragment_results.back().artifact_id = "unexpected-artifact";
    EXPECT_FALSE(internal_planner.isValidFinalPlan(plan));
}

/**
 * @brief Test 7: Freshness state transitions
 *
 * Verifies that ShardSummary correctly tracks freshness state transitions.
 */
TEST_F(DistributedPlannerTest, FreshnessStateTransitions) {
    auto summary = createFreshSummary("test_shard");

    // Initially fresh
    EXPECT_EQ(summary.freshness_state, tensor::SummaryFreshnessState::FRESH);

    // Mark as stale
    summary.markAsStale();
    EXPECT_EQ(summary.freshness_state, tensor::SummaryFreshnessState::STALE);
    EXPECT_TRUE(summary.isStale());

    // Mark as invalid
    summary.markAsInvalid();
    EXPECT_EQ(summary.freshness_state, tensor::SummaryFreshnessState::INVALID);
    EXPECT_TRUE(summary.isStale());

    // Mark as fresh again
    summary.markAsFresh();
    EXPECT_EQ(summary.freshness_state, tensor::SummaryFreshnessState::FRESH);
    EXPECT_FALSE(summary.isStale());
}

/**
 * @brief Test 8: Configuration and policy enforcement
 *
 * Verifies that planner configuration is respected and policies
 * are enforced correctly.
 */
TEST_F(DistributedPlannerTest, ConfigurationAndPolicyEnforcement) {
    // Set custom configuration
    DistributedTensorPlanner::Config config;
    config.max_summary_ttl_seconds = 1800;  // 30 minutes
    config.min_fragment_success_rate = 0.9f;
    config.strict_graph_validation = true;
    config.enable_fallback_routing = true;

    planner_->setConfig(config);

    // Verify configuration was set
    const auto& stored_config = planner_->config();
    EXPECT_EQ(stored_config.max_summary_ttl_seconds, 1800);
    EXPECT_EQ(stored_config.min_fragment_success_rate, 0.9f);
    EXPECT_TRUE(stored_config.strict_graph_validation);
    EXPECT_TRUE(stored_config.enable_fallback_routing);
}

/**
 * @brief Test 9: Fragment load with partial failures
 *
 * Verifies correct handling of scenarios where some fragment loads
 * succeed and others fail.
 */
TEST_F(DistributedPlannerTest, HandlePartialFragmentLoadFailures) {
    // Create mock that fails on specific requests
    auto selective_fetcher = std::make_shared<MockFragmentFetcher>();
    planner_->setFragmentFetcher(selective_fetcher);

    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("shard_0", 0.9f));
    summaries.push_back(createFreshSummary("shard_1", 0.8f));

    auto plan = planner_->buildRetrievalPlan(summaries, "test_query");

    // Execute fragment loads
    plan.fragment_results = planner_->executeFragmentLoads(plan.fragment_requests);

    // With default mock, all should succeed
    std::size_t success_count = 0;
    for (const auto& result : plan.fragment_results) {
        if (result.success) {
            success_count++;
        }
    }
    EXPECT_EQ(success_count, plan.fragment_results.size());

    // Now set fetcher to fail
    selective_fetcher->setShouldFail(true);
    plan.fragment_results = planner_->executeFragmentLoads(plan.fragment_requests);

    // All should now fail
    for (const auto& result : plan.fragment_results) {
        EXPECT_FALSE(result.success);
    }
}

/**
 * @brief Test 10: Unhealthy shard rejection
 *
 * Verifies that shards marked as unhealthy are rejected even if
 * their summaries are fresh.
 */
TEST_F(DistributedPlannerTest, RejectUnhealthyShards) {
    std::vector<tensor::ShardSummary> summaries;
    
    auto fresh_healthy = createFreshSummary("shard_0", 0.9f);
    summaries.push_back(fresh_healthy);

    auto fresh_unhealthy = createFreshSummary("shard_1", 0.8f);
    fresh_unhealthy.shard_healthy = false;
    summaries.push_back(fresh_unhealthy);

    auto routing = planner_->planSummaryFirstRouting(summaries);

    // Verify results
    EXPECT_FALSE(routing[0].rejected_as_unhealthy);
    EXPECT_TRUE(routing[1].rejected_as_unhealthy);

    // Fragment requests should only include healthy shard
    auto requests = planner_->planExactOnDemandLoading(routing);
    EXPECT_EQ(requests.size(), 1);
    EXPECT_EQ(requests[0].shard_id, "shard_0");
}

} // namespace testing
} // namespace distributed_tensor
} // namespace themis
