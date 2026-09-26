// Copyright ThemisDB Contributors
// SPDX-License-Identifier: Apache-2.0

#include <gtest/gtest.h>

#include "rag/reranker_cost_analyzer.h"

namespace themis::rag {

class RerankerCostAnalyzerTest : public ::testing::Test {
 protected:
  RerankerCostAnalyzer analyzer_;
};

TEST_F(RerankerCostAnalyzerTest, RecordCostAndRetrieve) {
  RerankerCostAnalyzer::CostRecord record;
  record.query_id = "q1";
  record.tenant_id = "tenant_1";
  record.request_time_ms = 50;
  record.tokens_used = 100;
  record.quality_improvement = 0.05f;
  record.was_beneficial = true;

  analyzer_.RecordCostAndQuality(record);
  
  auto metrics = analyzer_.GetCostMetrics("tenant_1");
  EXPECT_TRUE(metrics.has_value());
  EXPECT_EQ(metrics->sample_count, 1);
}

TEST_F(RerankerCostAnalyzerTest, MultipleRecordsPerTenant) {
  std::vector<RerankerCostAnalyzer::CostRecord> records = {
      {"q1", "tenant_1", 30, 80, 0.03f, true, 0, ""},
      {"q2", "tenant_1", 50, 100, 0.05f, true, 0, ""},
      {"q3", "tenant_1", 70, 120, 0.02f, false, 0, ""}
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
    RerankerCostAnalyzer::CostRecord record;
    record.query_id = "q" + std::to_string(i);
    record.tenant_id = "tenant_perc";
    record.request_time_ms = latencies[i];
    record.tokens_used = 100;
    record.quality_improvement = 0.05f;
    record.was_beneficial = true;
    analyzer_.RecordCostAndQuality(record);
  }

  auto metrics = analyzer_.GetCostMetrics("tenant_perc");
  EXPECT_TRUE(metrics.has_value());
  EXPECT_GE(metrics->p50_cost_ms, 30);
  EXPECT_LE(metrics->p50_cost_ms, 60);
  EXPECT_GE(metrics->p95_cost_ms, 80);
  EXPECT_LE(metrics->p95_cost_ms, 100);
}

TEST_F(RerankerCostAnalyzerTest, GetTenantAllocations) {
  RerankerCostAnalyzer::CostRecord record1;
  record1.query_id = "q1";
  record1.tenant_id = "tenant_a";
  record1.request_time_ms = 50;
  record1.tokens_used = 100;
  analyzer_.RecordCostAndQuality(record1);

  RerankerCostAnalyzer::CostRecord record2;
  record2.query_id = "q2";
  record2.tenant_id = "tenant_b";
  record2.request_time_ms = 40;
  record2.tokens_used = 80;
  analyzer_.RecordCostAndQuality(record2);

  auto allocations = analyzer_.GetTenantAllocations();
  
  EXPECT_GE(allocations.size(), 2);
}

TEST_F(RerankerCostAnalyzerTest, BeneficialRateThreshold) {
  // Create records with varying beneficial rates
  for (int i = 0; i < 10; ++i) {
    RerankerCostAnalyzer::CostRecord record;
    record.query_id = "q" + std::to_string(i);
    record.tenant_id = "tenant_rate";
    record.request_time_ms = 50;
    record.tokens_used = 100;
    record.quality_improvement = 0.05f;
    record.was_beneficial = i < 8;  // 80% beneficial
    analyzer_.RecordCostAndQuality(record);
  }

  bool above_80 = analyzer_.IsBeneficialRateAboveThreshold("tenant_rate", 0.8f);
  EXPECT_TRUE(above_80);

  bool above_90 = analyzer_.IsBeneficialRateAboveThreshold("tenant_rate", 0.9f);
  EXPECT_FALSE(above_90);
}

TEST_F(RerankerCostAnalyzerTest, GetCostComponentBreakdown) {
  RerankerCostAnalyzer::CostRecord record;
  record.query_id = "q1";
  record.tenant_id = "tenant_breakdown";
  record.request_time_ms = 100;
  record.tokens_used = 1000;
  record.quality_improvement = 0.05f;
  record.was_beneficial = true;
  analyzer_.RecordCostAndQuality(record);

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
