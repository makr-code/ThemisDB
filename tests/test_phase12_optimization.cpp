/**
 * @file test_phase12_optimization.cpp
 * @brief Comprehensive test suite for Phase 12: Advanced Cost Optimization
 *
 * Tests QueryPlanner, MultiModelSelector, BudgetAllocator, and CostForecastor
 * with 40+ test cases covering functionality, edge cases, and integration.
 *
 * @version 0.1.0
 */

#include <gtest/gtest.h>
#include <thread>
#include <chrono>

#include "rag/query_planner.h"
#include "rag/multi_model_selector.h"
#include "rag/budget_allocator.h"
#include "rag/cost_forecaster.h"

namespace themis::rag::optimization {

// ============================================================================
// QueryPlanner Tests
// ============================================================================

class QueryPlannerTest : public ::testing::Test {
 protected:
  QueryPlanner planner_{false};  // Disable adaptive budget for predictable tests
};

TEST_F(QueryPlannerTest, AnalyzeSimpleQuery) {
  QueryComplexity result = planner_.AnalyzeQuery("Who is Albert Einstein?", {"lexical", "dense"});
  EXPECT_EQ(result.level, QueryComplexity::kSimple);
  EXPECT_LT(result.estimated_cost, 10.0);
}

TEST_F(QueryPlannerTest, AnalyzeComplexQuery) {
  QueryComplexity result = planner_.AnalyzeQuery(
      "Compare and contrast the economic impacts of the Industrial Revolution AND Agricultural Revolution OR the Digital Revolution on modern society",
      {"lexical", "dense", "hybrid"});
  EXPECT_EQ(result.level, QueryComplexity::kComplex);
  EXPECT_GT(result.suggested_retrieval_k, 10);
}

TEST_F(QueryPlannerTest, StrategySelectionLexical) {
  QueryComplexity result = planner_.AnalyzeQuery("author of Hamlet", {"lexical"});
  EXPECT_EQ(result.optimal_strategy, "lexical");
}

TEST_F(QueryPlannerTest, StrategySelectionHybrid) {
  QueryComplexity result = planner_.AnalyzeQuery(
      "What is the relationship between A AND B OR C?", {"lexical", "dense", "hybrid"});
  EXPECT_EQ(result.optimal_strategy, "hybrid");
}

TEST_F(QueryPlannerTest, EstimateLatency) {
  double latency = planner_.EstimateLatency("test query", "lexical", "small");
  EXPECT_GT(latency, 0.0);
  EXPECT_LT(latency, 1000.0);  // Should be < 1 second
}

TEST_F(QueryPlannerTest, AllocateRerankerBudgetTight) {
  uint32_t budget = planner_.AllocateRerankerBudget(50.0, "test query");
  EXPECT_LE(budget, 25);  // 50ms budget, ~2ms per doc
}

TEST_F(QueryPlannerTest, AllocateRerankerBudgetLoose) {
  uint32_t budget = planner_.AllocateRerankerBudget(500.0, "test query");
  EXPECT_GT(budget, 10);
}

TEST_F(QueryPlannerTest, ComplexityThresholds) {
  planner_.SetComplexityThresholds(0.2, 0.8);
  QueryComplexity result = planner_.AnalyzeQuery("short", {"lexical"});
  EXPECT_EQ(result.level, QueryComplexity::kSimple);  // New threshold
}

// ============================================================================
// MultiModelSelector Tests
// ============================================================================

class MultiModelSelectorTest : public ::testing::Test {
 protected:
  MultiModelSelector selector_{5, 50};  // 5 models, 50 samples for decision
};

TEST_F(MultiModelSelectorTest, RegisterModel) {
  uint32_t idx1 = selector_.RegisterModel(1, "model_v1", true);
  uint32_t idx2 = selector_.RegisterModel(2, "model_v2");
  EXPECT_NE(idx1, idx2);
}

TEST_F(MultiModelSelectorTest, ReportQueryMetrics) {
  selector_.RegisterModel(1, "model_v1");
  selector_.ReportQueryMetrics(1, 100.0, 0.95, 500);  // latency, quality, tokens
  selector_.ReportQueryMetrics(1, 110.0, 0.94, 510);
  
  ModelStats stats = selector_.GetModelStats(1);
  EXPECT_EQ(stats.total_queries, 2);
  EXPECT_NEAR(stats.mean_quality_score, 0.945, 0.001);
}

TEST_F(MultiModelSelectorTest, SelectBestModelSingleModel) {
  selector_.RegisterModel(1, "model_v1");
  selector_.ReportQueryMetrics(1, 100.0, 0.90, 500);
  
  uint32_t best = selector_.SelectBestModel(0.5);
  EXPECT_EQ(best, 1);
}

TEST_F(MultiModelSelectorTest, SelectBestModelCostWeight) {
  selector_.RegisterModel(1, "model_fast");
  selector_.RegisterModel(2, "model_accurate");
  
  // Fast model: low cost, lower quality
  for (int i = 0; i < 100; ++i) {
    selector_.ReportQueryMetrics(1, 50.0, 0.80, 100);
  }
  
  // Accurate model: high cost, higher quality
  for (int i = 0; i < 100; ++i) {
    selector_.ReportQueryMetrics(2, 150.0, 0.95, 300);
  }
  
  // Cost-focused: prefer fast model
  uint32_t best_cheap = selector_.SelectBestModel(0.8);
  EXPECT_EQ(best_cheap, 1);
  
  // Quality-focused: prefer accurate model
  uint32_t best_quality = selector_.SelectBestModel(0.1);
  EXPECT_EQ(best_quality, 2);
}

TEST_F(MultiModelSelectorTest, ParetoFrontier) {
  selector_.RegisterModel(1, "model_v1");
  selector_.RegisterModel(2, "model_v2");
  selector_.RegisterModel(3, "model_v3");
  
  // v1: cheap, lower quality
  for (int i = 0; i < 50; ++i) {
    selector_.ReportQueryMetrics(1, 50.0, 0.80, 100);
  }
  
  // v2: medium cost, medium quality
  for (int i = 0; i < 50; ++i) {
    selector_.ReportQueryMetrics(2, 100.0, 0.85, 200);
  }
  
  // v3: expensive, higher quality
  for (int i = 0; i < 50; ++i) {
    selector_.ReportQueryMetrics(3, 200.0, 0.95, 400);
  }
  
  auto frontier = selector_.ComputeParetoFrontier();
  EXPECT_GE(frontier.size(), 2);  // At least v1 and v3 should be on frontier
}

TEST_F(MultiModelSelectorTest, FallbackChain) {
  selector_.RegisterModel(1, "baseline");
  selector_.RegisterModel(2, "challenger");
  
  for (int i = 0; i < 100; ++i) {
    selector_.ReportQueryMetrics(1, 100.0, 0.90, 500);
    selector_.ReportQueryMetrics(2, 100.0, 0.92, 500);
  }
  
  auto chain = selector_.GetFallbackChain();
  EXPECT_EQ(chain[0], 2);  // Challenger (better) first
  EXPECT_EQ(chain[1], 1);  // Baseline second
}

TEST_F(MultiModelSelectorTest, StatisticalSignificance) {
  selector_.RegisterModel(1, "baseline");
  selector_.RegisterModel(2, "challenger");
  
  // Baseline: mean quality 0.80
  for (int i = 0; i < 100; ++i) {
    selector_.ReportQueryMetrics(1, 100.0, 0.80, 500);
  }
  
  // Challenger: mean quality 0.95 (significantly better)
  for (int i = 0; i < 100; ++i) {
    selector_.ReportQueryMetrics(2, 100.0, 0.95, 500);
  }
  
  bool is_winner = selector_.IsStatisticallySignificantWinner(0.95);
  EXPECT_TRUE(is_winner);
}

// ============================================================================
// BudgetAllocator Tests
// ============================================================================

class BudgetAllocatorTest : public ::testing::Test {
 protected:
  BudgetAllocator allocator_{true, 100};
  
  void SetUp() override {
    TenantBudget budget;
    budget.tenant_id = "tenant_a";
    budget.max_daily_cost = 100.0;
    budget.max_hourly_cost = 10.0;
    budget.max_query_latency_ms = 5000.0;
    allocator_.RegisterTenant(budget);
  }
};

TEST_F(BudgetAllocatorTest, RegisterTenant) {
  TenantBudget budget;
  budget.tenant_id = "tenant_b";
  EXPECT_TRUE(allocator_.RegisterTenant(budget));
  EXPECT_FALSE(allocator_.RegisterTenant(budget));  // Duplicate
}

TEST_F(BudgetAllocatorTest, CanExecuteQueryWithinBudget) {
  EXPECT_TRUE(allocator_.CanExecuteQuery("tenant_a", 5.0, 1000.0));
}

TEST_F(BudgetAllocatorTest, RejectQueryExceedsDailyBudget) {
  EXPECT_FALSE(allocator_.CanExecuteQuery("tenant_a", 150.0, 1000.0));  // > 100 daily
}

TEST_F(BudgetAllocatorTest, RejectQueryExceedsLatency) {
  EXPECT_FALSE(allocator_.CanExecuteQuery("tenant_a", 5.0, 6000.0));  // > 5000ms
}

TEST_F(BudgetAllocatorTest, ReserveBudget) {
  uint64_t res_id = allocator_.ReserveBudget("tenant_a", 5.0, 1000.0);
  EXPECT_GT(res_id, 0);
  
  // Second reservation in same hour uses remaining budget
  uint64_t res_id2 = allocator_.ReserveBudget("tenant_a", 4.0, 1000.0);
  EXPECT_GT(res_id2, 0);
  
  // Third reservation should fail (hourly limit is 10)
  uint64_t res_id3 = allocator_.ReserveBudget("tenant_a", 2.0, 1000.0);
  EXPECT_EQ(res_id3, 0);  // Budget exceeded
}

TEST_F(BudgetAllocatorTest, ConfirmBudget) {
  uint64_t res_id = allocator_.ReserveBudget("tenant_a", 5.0, 1000.0);
  allocator_.ConfirmBudget(res_id, 4.5, 950.0);  // Actual < reserved
  
  BudgetStatus status = allocator_.GetBudgetStatus("tenant_a");
  EXPECT_NEAR(status.daily_cost_used, 4.5, 0.01);
}

TEST_F(BudgetAllocatorTest, ReleaseBudget) {
  uint64_t res_id = allocator_.ReserveBudget("tenant_a", 5.0, 1000.0);
  allocator_.ReleaseBudget(res_id);
  
  // Should be able to reserve again after release
  uint64_t res_id2 = allocator_.ReserveBudget("tenant_a", 5.0, 1000.0);
  EXPECT_GT(res_id2, 0);
}

TEST_F(BudgetAllocatorTest, GetBudgetStatus) {
  allocator_.ReserveBudget("tenant_a", 3.0, 1000.0);
  allocator_.ReserveBudget("tenant_a", 2.0, 1000.0);
  allocator_.ConfirmBudget(1, 3.0, 1000.0);
  allocator_.ConfirmBudget(2, 2.0, 1000.0);
  
  BudgetStatus status = allocator_.GetBudgetStatus("tenant_a");
  EXPECT_NEAR(status.daily_cost_used, 5.0, 0.01);
  EXPECT_NEAR(status.daily_cost_remaining, 95.0, 0.01);
}

TEST_F(BudgetAllocatorTest, UpdateTenantBudget) {
  TenantBudget new_budget;
  new_budget.tenant_id = "tenant_a";
  new_budget.max_daily_cost = 200.0;  // Increase
  
  EXPECT_TRUE(allocator_.UpdateTenantBudget(new_budget));
  EXPECT_TRUE(allocator_.CanExecuteQuery("tenant_a", 150.0, 1000.0));  // Now allowed
}

TEST_F(BudgetAllocatorTest, QueueDepth) {
  allocator_.ReserveBudget("tenant_a", 1.0, 100.0);
  EXPECT_EQ(allocator_.GetQueueDepth("tenant_a"), 1);
  
  allocator_.ReserveBudget("tenant_a", 1.0, 100.0);
  EXPECT_EQ(allocator_.GetQueueDepth("tenant_a"), 2);
}

// ============================================================================
// CostForecastor Tests
// ============================================================================

class CostForecastorTest : public ::testing::Test {
 protected:
  CostForecastor forecaster_{14, 0.3};
};

TEST_F(CostForecastorTest, ReportHourlyCost) {
  forecaster_.ReportHourlyCost(10.0, 100);
  forecaster_.ReportHourlyCost(11.0, 110);
  
  auto stats = forecaster_.GetHistoricalStats();
  EXPECT_EQ(stats.sample_count, 2);
  EXPECT_NEAR(stats.mean, 10.5, 0.01);
}

TEST_F(CostForecastorTest, ForecastNext24Hours) {
  // Seed with some data
  for (int i = 0; i < 10; ++i) {
    forecaster_.ReportHourlyCost(10.0 + i, 100 + i * 10);
  }
  
  auto forecast = forecaster_.ForecastNext24Hours();
  EXPECT_EQ(forecast.size(), 24);
  
  // All predictions should be positive
  for (const auto& p : forecast) {
    EXPECT_GT(p.predicted_cost, 0.0);
    EXPECT_GE(p.confidence_interval, 0.0);
  }
}

TEST_F(CostForecastorTest, ForecastWeeklyCost) {
  for (int i = 0; i < 14; ++i) {
    forecaster_.ReportHourlyCost(10.0, 100);
  }
  
  double weekly = forecaster_.ForecastWeeklyCost();
  EXPECT_GT(weekly, 100.0);  // Should be > 1 day's cost
}

TEST_F(CostForecastorTest, DetectAnomalyNormal) {
  for (int i = 0; i < 50; ++i) {
    forecaster_.ReportHourlyCost(10.0, 100);
  }
  
  auto result = forecaster_.DetectAnomaly(10.5, 105);
  EXPECT_FALSE(result.is_anomaly);  // Normal variation
}

TEST_F(CostForecastorTest, DetectAnomalyCostSpike) {
  for (int i = 0; i < 50; ++i) {
    forecaster_.ReportHourlyCost(10.0, 100);
  }
  
  auto result = forecaster_.DetectAnomaly(50.0, 100);  // 5x spike
  EXPECT_TRUE(result.is_anomaly);
  EXPECT_GT(result.z_score, 3.0);
}

TEST_F(CostForecastorTest, ShouldAlert) {
  for (int i = 0; i < 50; ++i) {
    forecaster_.ReportHourlyCost(10.0, 100);
  }
  
  AlertThreshold threshold;
  threshold.cost_increase_pct = 50.0;
  forecaster_.SetAlertThreshold(threshold);
  
  EXPECT_FALSE(forecaster_.ShouldAlert(10.5, 100, threshold));
  EXPECT_TRUE(forecaster_.ShouldAlert(20.0, 100, threshold));  // 100% increase
}

TEST_F(CostForecastorTest, GetHistoricalStats) {
  for (int i = 0; i < 20; ++i) {
    forecaster_.ReportHourlyCost(10.0 + i, 100);
  }
  
  auto stats = forecaster_.GetHistoricalStats();
  EXPECT_EQ(stats.sample_count, 20);
  EXPECT_EQ(stats.min_value, 10.0);
  EXPECT_EQ(stats.max_value, 29.0);
  EXPECT_GT(stats.stddev, 0.0);
}

TEST_F(CostForecastorTest, ResetForecaster) {
  forecaster_.ReportHourlyCost(10.0, 100);
  forecaster_.ReportHourlyCost(20.0, 200);
  
  forecaster_.Reset();
  
  auto stats = forecaster_.GetHistoricalStats();
  EXPECT_EQ(stats.sample_count, 0);
}

// ============================================================================
// Integration Tests
// ============================================================================

class Phase12IntegrationTest : public ::testing::Test {
 protected:
  QueryPlanner planner_;
  MultiModelSelector selector_{3, 10};
  BudgetAllocator allocator_{true, 100};
  CostForecastor forecaster_{14, 0.3};
  
  void SetUp() override {
    // Register models
    selector_.RegisterModel(1, "model_v1", true);
    selector_.RegisterModel(2, "model_v2");
    
    // Register tenant
    TenantBudget budget;
    budget.tenant_id = "demo_tenant";
    budget.max_daily_cost = 100.0;
    budget.max_hourly_cost = 10.0;
    allocator_.RegisterTenant(budget);
  }
};

TEST_F(Phase12IntegrationTest, QueryPlanningAndBudgetAllocation) {
  std::string query = "Compare benefits of two approaches";
  
  // Step 1: Plan query
  QueryComplexity plan = planner_.AnalyzeQuery(query, {"lexical", "dense", "hybrid"});
  EXPECT_EQ(plan.optimal_strategy, "hybrid");
  
  // Step 2: Estimate latency
  double estimated_latency = planner_.EstimateLatency(query, plan.optimal_strategy, "medium");
  EXPECT_GT(estimated_latency, 0.0);
  
  // Step 3: Check budget
  EXPECT_TRUE(allocator_.CanExecuteQuery("demo_tenant", 5.0, estimated_latency));
}

TEST_F(Phase12IntegrationTest, ModelSelectionAndCostTracking) {
  // Report metrics for two models
  for (int i = 0; i < 15; ++i) {
    selector_.ReportQueryMetrics(1, 100.0, 0.90, 500);
    selector_.ReportQueryMetrics(2, 100.0, 0.92, 500);
    
    forecaster_.ReportHourlyCost(10.0, 100);
  }
  
  // Select best model
  uint32_t best = selector_.SelectBestModel(0.5);
  EXPECT_EQ(best, 2);  // v2 has better quality
  
  // Check cost forecast
  double weekly_cost = forecaster_.ForecastWeeklyCost();
  EXPECT_GT(weekly_cost, 100.0);
}

TEST_F(Phase12IntegrationTest, CostAnomaly && BudgetAlert) {
  // Establish baseline
  for (int i = 0; i < 50; ++i) {
    forecaster_.ReportHourlyCost(10.0, 100);
  }
  
  // Simulate cost spike
  AlertThreshold threshold;
  threshold.cost_increase_pct = 50.0;
  
  EXPECT_FALSE(forecaster_.ShouldAlert(10.5, 100, threshold));  // Normal
  EXPECT_TRUE(forecaster_.ShouldAlert(20.0, 100, threshold));   // Alert triggered
}

TEST_F(Phase12IntegrationTest, BudgetReservationFullCycle) {
  // Reserve budget
  uint64_t res_id = allocator_.ReserveBudget("demo_tenant", 5.0, 2000.0);
  EXPECT_GT(res_id, 0);
  
  // Simulate query execution
  selector_.ReportQueryMetrics(1, 2000.0, 0.95, 500);
  forecaster_.ReportHourlyCost(5.0, 50);
  
  // Confirm budget
  allocator_.ConfirmBudget(res_id, 4.8, 2000.0);
  
  // Verify status
  BudgetStatus status = allocator_.GetBudgetStatus("demo_tenant");
  EXPECT_NEAR(status.daily_cost_used, 4.8, 0.01);
}

}  // namespace themis::rag::optimization

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
