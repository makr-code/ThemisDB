// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/reranker_cost_analyzer.h"

namespace themis::rag {

namespace {

RerankerCostAnalyzer::CostRecord MakeCostRecord(
    const std::string& query_id,
    const std::string& tenant_id,
    uint64_t request_time_ms,
    uint32_t tokens_used,
    float quality_improvement,
    bool was_beneficial,
    const std::string& cost_breakdown = "") {
  RerankerCostAnalyzer::CostRecord record{};
  record.query_id = query_id;
  record.tenant_id = tenant_id;
  record.request_time_ms = request_time_ms;
  record.tokens_used = tokens_used;
  record.quality_improvement = quality_improvement;
  record.was_beneficial = was_beneficial;
  record.cost_breakdown = cost_breakdown;
  record.timestamp_us = 0;
  return record;
}

}  // namespace

class RerankerCostAnalyzerTest : public ::testing::Test {
 protected:
  RerankerCostAnalyzer analyzer_;
};

TEST_F(RerankerCostAnalyzerTest, RecordCostAndRetrieve) {
  auto record = MakeCostRecord("q1", "tenant_1", 50, 100, 0.05f, true);

  analyzer_.RecordCostAndQuality(record);
  
  auto metrics = analyzer_.GetCostMetrics("tenant_1");
  EXPECT_TRUE(metrics.has_value());
  EXPECT_EQ(metrics->sample_count, 1);
}

TEST_F(RerankerCostAnalyzerTest, MultipleRecordsPerTenant) {
  std::vector<RerankerCostAnalyzer::CostRecord> records = {
      MakeCostRecord("q1", "tenant_1", 30, 80, 0.03f, true),
      MakeCostRecord("q2", "tenant_1", 50, 100, 0.05f, true),
      MakeCostRecord("q3", "tenant_1", 70, 120, 0.02f, false)
  };

  for (const auto& record : records) {
    analyzer_.RecordCostAndQuality(record);
  }

  auto metrics = analyzer_.GetCostMetrics("tenant_1");
  EXPECT_TRUE(metrics.has_value());
  EXPECT_EQ(metrics->sample_count, 3);
  EXPECT_NEAR(metrics->mean_quality_improvement, 0.033f, 0.01f);
  EXPECT_NEAR(metrics->beneficial_rate, 2.0f / 3.0f, 0.01f);
}

TEST_F(RerankerCostAnalyzerTest, PercentileCalculation) {
  // Create records with known latencies
  std::vector<uint64_t> latencies = {10, 20, 30, 40, 50, 60, 70, 80, 90, 100};
  
  for (size_t i = 0; i < latencies.size(); ++i) {
    analyzer_.RecordCostAndQuality(
        MakeCostRecord("q" + std::to_string(i),
                       "tenant_perc",
                       latencies[i],
                       100,
                       0.05f,
                       true));
  }

  auto metrics = analyzer_.GetCostMetrics("tenant_perc");
  EXPECT_TRUE(metrics.has_value());
  EXPECT_GE(metrics->p50_cost_ms, 30);
  EXPECT_LE(metrics->p50_cost_ms, 60);
  EXPECT_GE(metrics->p95_cost_ms, 80);
  EXPECT_LE(metrics->p95_cost_ms, 100);
}

TEST_F(RerankerCostAnalyzerTest, GetTenantAllocations) {
  analyzer_.RecordCostAndQuality(MakeCostRecord("q1", "tenant_a", 50, 100, 0.04f, true));
  analyzer_.RecordCostAndQuality(MakeCostRecord("q2", "tenant_b", 40, 80, 0.03f, true));

  auto allocations = analyzer_.GetTenantAllocations();
  
  EXPECT_GE(allocations.size(), 2);
}

TEST_F(RerankerCostAnalyzerTest, BeneficialRateThreshold) {
  // Create records with varying beneficial rates
  for (int i = 0; i < 10; ++i) {
    analyzer_.RecordCostAndQuality(
        MakeCostRecord("q" + std::to_string(i),
                       "tenant_rate",
                       50,
                       100,
                       0.05f,
                       i < 8));
  }

  bool above_80 = analyzer_.IsBeneficialRateAboveThreshold("tenant_rate", 0.8f);
  EXPECT_TRUE(above_80);

  bool above_90 = analyzer_.IsBeneficialRateAboveThreshold("tenant_rate", 0.9f);
  EXPECT_FALSE(above_90);
}

TEST_F(RerankerCostAnalyzerTest, GetCostComponentBreakdown) {
  analyzer_.RecordCostAndQuality(
      MakeCostRecord("q1", "tenant_breakdown", 100, 1000, 0.05f, true));

  auto breakdown = analyzer_.GetCostComponentBreakdown("tenant_breakdown");
  
  EXPECT_GT(breakdown["tokens"], 0.0f);
  EXPECT_GT(breakdown["latency"], 0.0f);
}

TEST_F(RerankerCostAnalyzerTest, ExportCostRecordsAsJSON) {
  RerankerCostAnalyzer::CostRecord record1;
  record1.query_id = "q1";
  record1.tenant_id = "tenant_export";
  record1.request_time_ms = 50;
  record1.tokens_used = 100;
  record1.quality_improvement = 0.05f;
  record1.was_beneficial = true;
  analyzer_.RecordCostAndQuality(record1);

  RerankerCostAnalyzer::CostRecord record2;
  record2.query_id = "q2";
  record2.tenant_id = "tenant_export";
  record2.request_time_ms = 60;
  record2.tokens_used = 120;
  record2.quality_improvement = 0.08f;
  record2.was_beneficial = true;
  analyzer_.RecordCostAndQuality(record2);

  auto json_export = analyzer_.ExportCostRecordsAsJSON("tenant_export");
  
  EXPECT_FALSE(json_export.empty());
  EXPECT_GT(json_export.find("q1"), 0);
  EXPECT_GT(json_export.find("q2"), 0);
}

TEST_F(RerankerCostAnalyzerTest, GetCostTrendsPerHour) {
  // Create records spanning time windows
  for (int i = 0; i < 5; ++i) {
    RerankerCostAnalyzer::CostRecord record;
    record.query_id = "q" + std::to_string(i);
    record.tenant_id = "tenant_trends";
    record.request_time_ms = 40 + i * 10;
    record.tokens_used = 100 + i * 10;
    record.quality_improvement = 0.05f;
    record.was_beneficial = true;
    // Timestamp would vary in real scenario
    analyzer_.RecordCostAndQuality(record);
  }

  auto trends = analyzer_.GetCostTrends("tenant_trends", 5);
  
  EXPECT_GT(trends.size(), 0);
}

TEST_F(RerankerCostAnalyzerTest, EmptyMetricsForUnknownTenant) {
  auto metrics = analyzer_.GetCostMetrics("unknown_tenant");
  
  EXPECT_FALSE(metrics.has_value());
}

TEST_F(RerankerCostAnalyzerTest, TimeWindowFiltering) {
  RerankerCostAnalyzer::CostRecord record;
  record.query_id = "q1";
  record.tenant_id = "tenant_window";
  record.request_time_ms = 50;
  record.tokens_used = 100;
  record.quality_improvement = 0.05f;
  record.was_beneficial = true;
  record.timestamp_us = 1000000;
  analyzer_.RecordCostAndQuality(record);

  // Query with future timestamp should return no results
  auto now = std::chrono::system_clock::now();
  auto future = now + std::chrono::hours(1);
  auto metrics = analyzer_.GetCostMetrics("tenant_window", future);
  
  EXPECT_FALSE(metrics.has_value());
}

}  // namespace themis::rag
