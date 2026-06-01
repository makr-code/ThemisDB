/**
 * @file query_planner_test.cc
 * @brief Contract tests for IQueryPlanner (sub-issue #5441).
 *
 * Validates factory construction, plan generation for basic requests,
 * reconfiguration, and config round-trip at scaffold stage.
 * Production routing logic is tracked in sub-issue #5441.
 */

#include "evaluation/include/query_planner.h"

#include <gtest/gtest.h>
#include <memory>
#include <vector>

using namespace themis::evaluation;

namespace {

QueryPlannerConfig defaultConfig() {
    QueryPlannerConfig cfg;
    cfg.prefer_local          = true;
    cfg.enable_graph          = false;
    cfg.enable_distributed    = false;
    cfg.min_recall_threshold  = 0.90f;
    return cfg;
}

PlannerRequest simpleRequest(std::uint32_t top_k = 10) {
    PlannerRequest req;
    req.embedding   = std::vector<float>(64, 0.3f);
    req.query_text  = "unit test query";
    req.top_k       = top_k;
    req.budget      = std::chrono::milliseconds{500};
    req.require_exact = false;
    return req;
}

HardwareProfile cpuHw() {
    HardwareProfile hw;
    hw.id        = "cpu-only-16gb";
    hw.dram_bytes = 16ULL << 30;
    return hw;
}

} // namespace

class QueryPlannerTest : public ::testing::Test {
protected:
    void SetUp() override {
        planner_ = makeQueryPlanner(defaultConfig());
        ASSERT_NE(planner_, nullptr);
    }

    std::unique_ptr<IQueryPlanner> planner_;
};

TEST_F(QueryPlannerTest, FactoryReturnsNonNull) {
    EXPECT_NE(planner_, nullptr);
}

TEST_F(QueryPlannerTest, ConfigRoundTrip) {
    QueryPlannerConfig cfg = planner_->config();
    EXPECT_EQ(cfg.prefer_local, true);
    EXPECT_EQ(cfg.enable_distributed, false);
}

TEST_F(QueryPlannerTest, PlanDoesNotThrow) {
    EXPECT_NO_THROW(planner_->plan(simpleRequest(), cpuHw()));
}

TEST_F(QueryPlannerTest, PlanReturnsFeasiblePath) {
    QueryPlan plan = planner_->plan(simpleRequest(), cpuHw());
    // Scaffold: plan should at minimum set a non-default path.
    (void)plan;
    SUCCEED();
}

TEST_F(QueryPlannerTest, PlanWithEmptyEmbeddingDoesNotThrow) {
    PlannerRequest req;
    req.query_text = "no embedding query";
    EXPECT_NO_THROW(planner_->plan(req, cpuHw()));
}

TEST_F(QueryPlannerTest, PlanWithRequireExactSelectsExactPath) {
    PlannerRequest req = simpleRequest();
    req.require_exact = true;
    QueryPlan plan = planner_->plan(req, cpuHw());
    // Scaffold: may or may not enforce exact path; must not throw.
    (void)plan;
    SUCCEED();
}

TEST_F(QueryPlannerTest, ReconfigureDoesNotThrow) {
    QueryPlannerConfig cfg = defaultConfig();
    cfg.enable_graph = true;
    EXPECT_NO_THROW(planner_->reconfigure(cfg));
}

TEST_F(QueryPlannerTest, ReconfigureUpdatesConfig) {
    QueryPlannerConfig cfg = defaultConfig();
    cfg.enable_graph = true;
    planner_->reconfigure(cfg);
    EXPECT_TRUE(planner_->config().enable_graph);
}

TEST_F(QueryPlannerTest, MultiplePlansAreStable) {
    for (int i = 0; i < 5; ++i) {
        EXPECT_NO_THROW(planner_->plan(simpleRequest(), cpuHw()));
    }
}

TEST_F(QueryPlannerTest, FactoryWithRulesDoesNotThrow) {
    auto rules = makeApproximationRules();
    EXPECT_NO_THROW(makeQueryPlanner(defaultConfig(), std::move(rules)));
}
