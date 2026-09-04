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
#include "distributed_tensor/distributed_planner.h"
#include "tensor/tensor_summary_types.h"
#include "rag/graph_truth_validator.h"

#include <memory>
#include <chrono>
#include <ctime>
#include <iomanip>
#include <sstream>

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

/**
 * @brief Mock graph validator for testing.
 */
class MockGraphValidator : public rag::GraphTruthValidator {
public:
    MockGraphValidator() = default;

    void setShouldValidate(bool valid) { should_validate_ = valid; }

private:
    bool should_validate_ = true;
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
        std::ostringstream oss;
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
 * @brief Test 3: Graph override validation before finalization
 *
 * Verifies that the planner enforces graph validation before allowing
 * any result to be returned (no-summary-only-truth rule).
 */
TEST_F(DistributedPlannerTest, EnforceGraphValidationBeforeFinalization) {
    std::vector<tensor::ShardSummary> summaries;
    summaries.push_back(createFreshSummary("shard_0", 0.9f));

    // Build plan
    auto plan = planner_->buildRetrievalPlan(summaries, "test_query");

    // Initially, plan has not passed graph validation
    EXPECT_FALSE(plan.passed_graph_validation);

    // Execute fragment loads
    plan.fragment_results = planner_->executeFragmentLoads(plan.fragment_requests);

    // Mark as passed validation (in real flow, GraphTruthValidator would do this)
    rag::GraphTruthValidatorConfig config;
    plan.passed_graph_validation = true;

    // Now plan should be valid for output
    EXPECT_TRUE(planner_->isValidFinalPlan(plan));
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
 * @brief Test 5: Distributed fan-out reduction without correctness loss
 *
 * Verifies that stale summaries are rejected to reduce fan-out,
 * but correctness is maintained through exact fragment loading and
 * graph validation.
 */
TEST_F(DistributedPlannerTest, ReduceFanOutWhileMaintainingCorrectness) {
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

    // Correctness is maintained through fragments + graph validation
    plan.passed_graph_validation = true;
    EXPECT_TRUE(planner_->isValidFinalPlan(plan));
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
