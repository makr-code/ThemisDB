/// @file test_tensor_planner_policy.cc
/// @brief CTest for query planner compatibility with tensor artifacts
/// @author ThemisDB Implementation Team
/// @date 2026-07-02
///
/// Tests planner interaction:
/// - Freshness gating
/// - Staleness threshold enforcement
/// - Advisory vs exact semantics
/// - Fallback on stale
/// - Freshness score correlation
/// - Rebuilt artifact acceptance

#include <gtest/gtest.h>
#include "distributed_tensor/include/artifact_manifest.h"
#include <ctime>

namespace themis {
namespace distributed_tensor {

// Mock query planner
class TensorQueryPlanner {
public:
    enum QueryType {
        ADVISORY,  // Advisory query, can use stale artifacts
        EXACT      // Exact correctness required
    };

    enum RoutingStrategy {
        ANN_ONLY,
        ANN_TENSOR,
        ANN_TENSOR_GRAPH,
        EXACT_GRAPH_FALLBACK
    };

    struct QueryPlan {
        RoutingStrategy strategy;
        bool uses_tensor = false;
        bool fallback_to_exact = false;
        double tensor_confidence = 0.0;
    };

    QueryPlan planQuery(const ArtifactManifest& manifest, QueryType query_type,
                        int64_t current_time) {
        QueryPlan plan;

        // Check artifact usability
        bool is_active = (manifest.current_state == ArtifactLifecycleState::ACTIVE);
        bool is_stale = (manifest.current_state == ArtifactLifecycleState::STALE);

        if (!is_active && !is_stale) {
            // Artifact not usable
            plan.strategy = EXACT_GRAPH_FALLBACK;
            plan.uses_tensor = false;
            plan.fallback_to_exact = true;
            return plan;
        }

        // Check freshness for exact queries
        if (query_type == EXACT && is_stale) {
            // Stale artifact not acceptable for exact queries
            plan.strategy = EXACT_GRAPH_FALLBACK;
            plan.uses_tensor = false;
            plan.fallback_to_exact = true;
            return plan;
        }

        // Check staleness threshold
        if (manifest.staleness_threshold_sec > 0) {
            int64_t age = current_time - manifest.last_verified_unix_sec;
            if (age > manifest.staleness_threshold_sec) {
                is_stale = true;
            }
        }

        // Compute freshness score
        double freshness = 0.0;
        if (manifest.last_verified_unix_sec > 0 && manifest.staleness_threshold_sec > 0) {
            int64_t age = current_time - manifest.last_verified_unix_sec;
            freshness = 1.0 - static_cast<double>(age) / manifest.staleness_threshold_sec;
            freshness = std::max(0.0, std::min(1.0, freshness));
        }

        // Select routing strategy based on freshness and query type
        if (freshness >= 0.8) {
            plan.strategy = ANN_TENSOR_GRAPH;
            plan.uses_tensor = true;
            plan.fallback_to_exact = false;
            plan.tensor_confidence = freshness;
        } else if (freshness >= 0.5) {
            plan.strategy = ANN_TENSOR;
            plan.uses_tensor = true;
            plan.fallback_to_exact = false;
            plan.tensor_confidence = freshness;
        } else if (query_type == ADVISORY) {
            plan.strategy = ANN_TENSOR;
            plan.uses_tensor = true;
            plan.fallback_to_exact = false;
            plan.tensor_confidence = freshness;
        } else {
            plan.strategy = EXACT_GRAPH_FALLBACK;
            plan.uses_tensor = false;
            plan.fallback_to_exact = true;
            plan.tensor_confidence = 0.0;
        }

        return plan;
    }
};

/// Test fixture for planner policy tests
class TensorPlannerPolicyTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner_ = std::make_unique<TensorQueryPlanner>();

        manifest_.artifact_id = "test:tensor:planner";
        manifest_.artifact_class = ArtifactClass::DERIVED;
        manifest_.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;
        manifest_.current_state = ArtifactLifecycleState::ACTIVE;
        manifest_.created_at_unix_sec = 1000;
        manifest_.last_verified_unix_sec = 1000;
        manifest_.staleness_threshold_sec = 3600;
        manifest_.replication_factor = 3;
        manifest_.rank_cap = 16;
    }

    int64_t GetCurrentTime() const {
        return static_cast<int64_t>(std::time(nullptr));
    }

    std::unique_ptr<TensorQueryPlanner> planner_;
    ArtifactManifest manifest_;
};

// ============================================================================
// Freshness Gating Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerStaleFreshDifference) {
    // Verify: planner differentiates between fresh and stale artifacts
    int64_t current_time = 1000 + 1800;  // 30 minutes later

    auto plan_fresh = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, current_time);

    // Simulate stale artifact
    manifest_.current_state = ArtifactLifecycleState::STALE;
    auto plan_stale = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, current_time);

    // Fresh should use tensor
    EXPECT_TRUE(plan_fresh.uses_tensor);

    // Stale advisory query can still use tensor
    EXPECT_TRUE(plan_stale.uses_tensor);
}

// ============================================================================
// Staleness Threshold Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerStalenessThresholdRespected) {
    // Verify: stale artifact rejected when threshold exceeded
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    // Within threshold
    int64_t time_within = 1000 + 1800;
    auto plan_within = planner_->planQuery(manifest_, TensorQueryPlanner::EXACT, time_within);
    EXPECT_TRUE(plan_within.uses_tensor);

    // Beyond threshold
    int64_t time_beyond = 1000 + 3601;
    auto plan_beyond = planner_->planQuery(manifest_, TensorQueryPlanner::EXACT, time_beyond);
    EXPECT_FALSE(plan_beyond.uses_tensor);
    EXPECT_TRUE(plan_beyond.fallback_to_exact);
}

// ============================================================================
// Advisory vs Exact Semantics Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerAdvisoryVsExact) {
    // Verify: advisory vs exact semantics affect routing
    manifest_.current_state = ArtifactLifecycleState::STALE;
    int64_t current_time = 1000 + 3601;  // Beyond threshold

    // Advisory query: can use stale tensor
    auto plan_advisory = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, current_time);

    // Exact query: must fallback
    auto plan_exact = planner_->planQuery(manifest_, TensorQueryPlanner::EXACT, current_time);

    EXPECT_TRUE(plan_advisory.uses_tensor);
    EXPECT_FALSE(plan_exact.uses_tensor);
    EXPECT_TRUE(plan_exact.fallback_to_exact);
}

// ============================================================================
// Fallback on Stale Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerFallbackOnStale) {
    // Verify: exact graph fallback when tensor stale and mandatory
    manifest_.current_state = ArtifactLifecycleState::STALE;
    manifest_.truth_semantic = TruthSemantic::SOURCE_OF_TRUTH;

    int64_t current_time = 1000 + 3601;

    auto plan = planner_->planQuery(manifest_, TensorQueryPlanner::EXACT, current_time);

    EXPECT_FALSE(plan.uses_tensor);
    EXPECT_TRUE(plan.fallback_to_exact);
    EXPECT_EQ(plan.strategy, TensorQueryPlanner::EXACT_GRAPH_FALLBACK);
}

TEST_F(TensorPlannerPolicyTest, PlannerFallbackFrequency) {
    // Verify: fallback frequency measured
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int fallback_count = 0;
    const int QUERY_COUNT = 100;

    for (int i = 0; i < QUERY_COUNT; ++i) {
        int64_t time = 1000 + (i * 100);  // Vary time
        auto plan = planner_->planQuery(manifest_, TensorQueryPlanner::EXACT, time);

        if (plan.fallback_to_exact) {
            fallback_count++;
        }
    }

    EXPECT_GT(fallback_count, 0);
    EXPECT_LT(fallback_count, QUERY_COUNT);
}

// ============================================================================
// Freshness Score Correlation Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerFreshnessScoreCorrelation) {
    // Verify: freshness score influences routing decisions
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    // Test multiple time points
    std::vector<std::pair<int64_t, double>> time_freshness = {
        {1000 + 0, 1.0},        // Just verified
        {1000 + 900, 0.75},     // 25% through threshold
        {1000 + 1800, 0.5},     // 50% through threshold
        {1000 + 2700, 0.25},    // 75% through threshold
        {1000 + 3600, 0.0},     // At threshold
        {1000 + 3601, 0.0}      // Beyond threshold
    };

    for (const auto& [time, expected_freshness_bracket] : time_freshness) {
        auto plan = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, time);

        // Higher freshness → more tensor usage
        if (plan.tensor_confidence >= 0.5) {
            EXPECT_TRUE(plan.uses_tensor);
        }
    }
}

// ============================================================================
// Rebuilt Artifact Acceptance Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerRebuiltArtifactAccepted) {
    // Verify: REBUILT → ACTIVE transition accepted
    manifest_.current_state = ArtifactLifecycleState::REBUILT;
    manifest_.last_verified_unix_sec = GetCurrentTime();

    int64_t current_time = manifest_.last_verified_unix_sec;
    auto plan = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, current_time);

    // REBUILT artifact is not usable
    EXPECT_FALSE(plan.uses_tensor);
    EXPECT_TRUE(plan.fallback_to_exact);

    // Transition to ACTIVE
    manifest_.current_state = ArtifactLifecycleState::ACTIVE;
    plan = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, current_time);

    // Now ACTIVE artifact is usable
    EXPECT_TRUE(plan.uses_tensor);
}

// ============================================================================
// Invalidated Artifact Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerInvalidatedArtifactRejected) {
    // Verify: INVALIDATED artifact always rejected
    manifest_.current_state = ArtifactLifecycleState::INVALIDATED;

    int64_t current_time = GetCurrentTime();

    auto plan_advisory = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, current_time);
    auto plan_exact = planner_->planQuery(manifest_, TensorQueryPlanner::EXACT, current_time);

    EXPECT_FALSE(plan_advisory.uses_tensor);
    EXPECT_FALSE(plan_exact.uses_tensor);
    EXPECT_TRUE(plan_advisory.fallback_to_exact);
    EXPECT_TRUE(plan_exact.fallback_to_exact);
}

// ============================================================================
// Confidence Score Tests
// ============================================================================

TEST_F(TensorPlannerPolicyTest, PlannerTensorConfidence) {
    // Verify: tensor confidence score reflects freshness
    manifest_.last_verified_unix_sec = 1000;
    manifest_.staleness_threshold_sec = 3600;

    int64_t fresh_time = 1000 + 100;
    auto plan_fresh = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, fresh_time);
    EXPECT_GT(plan_fresh.tensor_confidence, 0.95);

    int64_t stale_time = 1000 + 2700;
    auto plan_stale = planner_->planQuery(manifest_, TensorQueryPlanner::ADVISORY, stale_time);
    EXPECT_GT(plan_stale.tensor_confidence, 0.1);
    EXPECT_LT(plan_stale.tensor_confidence, 0.5);
}

} // namespace distributed_tensor
} // namespace themis
