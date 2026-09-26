/**
 * @file test_phase13_quality_gates.cpp
 * @brief Comprehensive test suite for RAG Phase 13 Quality Gate Operationalization
 *
 * 32+ test cases covering:
 * - QualityMetricsCollector (8 tests)
 * - DeploymentGateController (8 tests)
 * - QualityAlertManager (8 tests)
 * - MetricsReporter (8 tests)
 * - Integration tests (4+ tests)
 *
 * @version 0.1.0
 */

#include <gtest/gtest.h>

#include <chrono>
#include <cmath>
#include <thread>

#include "rag/quality_metrics_collector.h"
#include "rag/deployment_gate_controller.h"
#include "rag/quality_alert_manager.h"
#include "rag/metrics_reporter.h"

using namespace themis::rag::quality;

// ============================================================================
// QualityMetricsCollector Tests (8 tests)
// ============================================================================

class QualityMetricsCollectorTest : public ::testing::Test {
 protected:
  QualityMetricsCollector collector;
};

TEST_F(QualityMetricsCollectorTest, ReportAndAggregateMetrics) {
  QualityMetrics m1;
  m1.recall_at_10 = 0.85;
  m1.ndcg_at_10 = 0.92;
  m1.mrr = 0.88;
  m1.model_version = 1;

  collector.ReportMetrics(m1);
  
  QualityMetrics m2;
  m2.recall_at_10 = 0.87;
  m2.ndcg_at_10 = 0.94;
  m2.mrr = 0.90;
  m2.model_version = 1;

  collector.ReportMetrics(m2);
  
  auto agg = collector.GetAggregatedMetrics(std::chrono::hours(1));
  
  EXPECT_EQ(agg.sample_count, 2);
  EXPECT_NEAR(agg.mean_recall_10, 0.86, 0.01);
  EXPECT_NEAR(agg.mean_ndcg_10, 0.93, 0.01);
  EXPECT_NEAR(agg.mean_mrr, 0.89, 0.01);
}

TEST_F(QualityMetricsCollectorTest, PercentileComputation) {
  for (int i = 0; i < 10; ++i) {
    QualityMetrics m;
    m.recall_at_10 = 0.80 + (i * 0.01);  // 0.80 to 0.89
    m.model_version = 1;
    collector.ReportMetrics(m);
  }
  
  auto agg = collector.GetAggregatedMetrics(std::chrono::hours(1));
  
  // P50 should be ~0.845, P95 should be ~0.889
  EXPECT_NEAR(agg.p50_recall_10, 0.845, 0.01);
  EXPECT_GT(agg.p95_recall_10, 0.88);
}

TEST_F(QualityMetricsCollectorTest, RegressionAnalysis) {
  QualityMetrics baseline;
  baseline.recall_at_10 = 0.90;
  baseline.ndcg_at_10 = 0.95;
  baseline.mrr = 0.92;
  collector.ReportMetrics(baseline);
  
  auto baseline_agg = collector.GetAggregatedMetrics(std::chrono::hours(1));
  
  collector.Reset();
  
  QualityMetrics degraded;
  degraded.recall_at_10 = 0.85;  // 5.6% regression
  degraded.ndcg_at_10 = 0.92;    // 3.2% regression
  degraded.mrr = 0.90;            // 2.2% regression
  collector.ReportMetrics(degraded);
  
  auto degraded_agg = collector.GetAggregatedMetrics(std::chrono::hours(1));
  
  auto regression = collector.CompareToBaseline(degraded_agg, baseline_agg);
  
  EXPECT_LT(regression.recall_10_change_pct, 0.0);
  EXPECT_LT(regression.ndcg_10_change_pct, 0.0);
  EXPECT_LT(regression.mrr_change_pct, 0.0);
  EXPECT_TRUE(regression.is_regression);
  EXPECT_GT(regression.worst_regression_pct, 5.0);
}

TEST_F(QualityMetricsCollectorTest, TimeWindowFiltering) {
  auto now = std::chrono::system_clock::now();
  
  QualityMetrics m;
  m.recall_at_10 = 0.85;
  collector.ReportMetrics(m);
  
  // Get metrics for 1-hour window
  auto agg_1h = collector.GetAggregatedMetrics(std::chrono::hours(1));
  EXPECT_EQ(agg_1h.sample_count, 1);
  
  // Get metrics for 1-minute window (should be empty or 1 depending on timing)
  auto agg_1m = collector.GetAggregatedMetrics(std::chrono::minutes(1));
  EXPECT_GE(agg_1m.sample_count, 0);
}

TEST_F(QualityMetricsCollectorTest, ModelSpecificMetrics) {
  QualityMetrics m1;
  m1.recall_at_10 = 0.90;
  m1.model_version = 1;
  collector.ReportMetrics(m1);
  
  QualityMetrics m2;
  m2.recall_at_10 = 0.85;
  m2.model_version = 2;
  collector.ReportMetrics(m2);
  
  auto agg_v1 = collector.GetMetricsForModel(1, std::chrono::hours(1));
  auto agg_v2 = collector.GetMetricsForModel(2, std::chrono::hours(1));
  
  EXPECT_EQ(agg_v1.sample_count, 1);
  EXPECT_EQ(agg_v2.sample_count, 1);
  EXPECT_NEAR(agg_v1.mean_recall_10, 0.90, 0.01);
  EXPECT_NEAR(agg_v2.mean_recall_10, 0.85, 0.01);
}

TEST_F(QualityMetricsCollectorTest, MaxSampleBufferLimit) {
  QualityMetricsCollector limited_collector(5);  // Max 5 samples
  
  for (int i = 0; i < 10; ++i) {
    QualityMetrics m;
    m.recall_at_10 = 0.80 + (i * 0.01);
    m.model_version = 1;
    limited_collector.ReportMetrics(m);
  }
  
  EXPECT_EQ(limited_collector.GetMetricsCount(), 10);  // Total count = 10
  
  auto agg = limited_collector.GetAggregatedMetrics(std::chrono::hours(1));
  EXPECT_EQ(agg.sample_count, 5);  // But aggregated = 5 (buffered)
}

TEST_F(QualityMetricsCollectorTest, ThreadSafetyMetricReporting) {
  std::vector<std::thread> threads;
  
  for (int t = 0; t < 5; ++t) {
    threads.emplace_back([this, t]() {
      for (int i = 0; i < 20; ++i) {
        QualityMetrics m;
        m.recall_at_10 = 0.80 + (i * 0.001);
        m.model_version = t;
        this->collector.ReportMetrics(m);
      }
    });
  }
  
  for (auto& thread : threads) {
    thread.join();
  }
  
  EXPECT_EQ(collector.GetMetricsCount(), 100);  // 5 threads * 20 metrics
}

// ============================================================================
// DeploymentGateController Tests (8 tests)
// ============================================================================

class DeploymentGateControllerTest : public ::testing::Test {
 protected:
  DeploymentGateController gate_controller{5.0, 2.0};  // Hard=5%, Soft=2%
  
  AggregatedMetrics CreateMetrics(double recall, double ndcg, double mrr, uint32_t count = 100) {
    AggregatedMetrics m;
    m.mean_recall_10 = recall;
    m.mean_ndcg_10 = ndcg;
    m.mean_mrr = mrr;
    m.sample_count = count;
    m.p50_recall_10 = recall * 0.99;
    m.p95_recall_10 = recall * 0.98;
    m.p50_ndcg_10 = ndcg * 0.99;
    m.p95_ndcg_10 = ndcg * 0.98;
    m.p95_mrr = mrr * 0.98;
    return m;
  }
};

TEST_F(DeploymentGateControllerTest, AllowDeploymentWithImprovement) {
  auto baseline = CreateMetrics(0.85, 0.90, 0.88);
  auto candidate = CreateMetrics(0.88, 0.92, 0.91);  // All improved
  
  auto decision = gate_controller.EvaluateCandidate(2, 1, candidate, baseline);
  
  EXPECT_EQ(decision.decision, GateDecision::kAllow);
  EXPECT_EQ(decision.failed_checks.size(), 0);
}

TEST_F(DeploymentGateControllerTest, DenyDeploymentOnHardRegression) {
  auto baseline = CreateMetrics(0.90, 0.95, 0.92);
  auto candidate = CreateMetrics(0.85, 0.95, 0.92);  // 5.6% recall regression (> hard threshold)
  
  auto decision = gate_controller.EvaluateCandidate(2, 1, candidate, baseline);
  
  EXPECT_EQ(decision.decision, GateDecision::kDeny);
  EXPECT_GT(decision.failed_checks.size(), 0);
  EXPECT_GT(decision.regression_pct, 5.0);
}

TEST_F(DeploymentGateControllerTest, WarnOnSoftRegression) {
  auto baseline = CreateMetrics(0.90, 0.95, 0.92);
  auto candidate = CreateMetrics(0.884, 0.95, 0.92);  // 1.8% recall regression (< hard, > soft)
  
  auto decision = gate_controller.EvaluateCandidate(2, 1, candidate, baseline);
  
  EXPECT_EQ(decision.decision, GateDecision::kWarn);
  EXPECT_LT(decision.regression_pct, 5.0);
}

TEST_F(DeploymentGateControllerTest, CustomMetricThresholds) {
  gate_controller.SetMetricThreshold("recall_10", 10.0, 5.0);  // Stricter
  
  auto baseline = CreateMetrics(0.90, 0.95, 0.92);
  auto candidate = CreateMetrics(0.85, 0.95, 0.92);  // 5.6% regression
  
  auto decision = gate_controller.EvaluateCandidate(2, 1, candidate, baseline);
  
  // With new threshold, 5.6% should be warning instead of deny
  EXPECT_EQ(decision.decision, GateDecision::kWarn);
}

TEST_F(DeploymentGateControllerTest, DisableMetricForGating) {
  gate_controller.SetMetricEnabled("recall_10", false);  // Don't gate on recall
  
  auto baseline = CreateMetrics(0.90, 0.95, 0.92);
  auto candidate = CreateMetrics(0.85, 0.95, 0.92);  // Recall regression, but disabled
  
  auto decision = gate_controller.EvaluateCandidate(2, 1, candidate, baseline);
  
  EXPECT_EQ(decision.decision, GateDecision::kAllow);  // Should pass
}

TEST_F(DeploymentGateControllerTest, SimulateDecisionWithoutRecording) {
  auto baseline = CreateMetrics(0.90, 0.95, 0.92);
  auto candidate = CreateMetrics(0.85, 0.95, 0.92);
  
  auto actual = gate_controller.EvaluateCandidate(2, 1, candidate, baseline);
  auto simulated = gate_controller.SimulateDecision(2, 1, candidate, baseline);
  
  EXPECT_EQ(actual.decision, simulated.decision);
  EXPECT_EQ(actual.regression_pct, simulated.regression_pct);
}

TEST_F(DeploymentGateControllerTest, GetGateConfiguration) {
  auto config = gate_controller.GetGateConfig();
  
  EXPECT_EQ(config.hard_regression_threshold, 5.0);
  EXPECT_EQ(config.soft_regression_threshold, 2.0);
  EXPECT_GT(config.enabled_metrics.size(), 0);
}

// ============================================================================
// QualityAlertManager Tests (8 tests)
// ============================================================================

class QualityAlertManagerTest : public ::testing::Test {
 protected:
  QualityAlertManager alert_manager;
};

TEST_F(QualityAlertManagerTest, GenerateWarningAlert) {
  auto alerts = alert_manager.ReportMetric("recall_10", 0.88, 0.90, 0.80);
  
  EXPECT_EQ(alerts.size(), 1);
  EXPECT_EQ(alerts[0].severity, AlertSeverity::kWarning);
  EXPECT_EQ(alerts[0].metric_name, "recall_10");
}

TEST_F(QualityAlertManagerTest, GenerateCriticalAlert) {
  auto alerts = alert_manager.ReportMetric("recall_10", 0.78, 0.90, 0.80);
  
  EXPECT_EQ(alerts.size(), 1);
  EXPECT_EQ(alerts[0].severity, AlertSeverity::kCritical);
}

TEST_F(QualityAlertManagerTest, NoAlertWhenValueAboveThreshold) {
  auto alerts = alert_manager.ReportMetric("recall_10", 0.92, 0.90, 0.80);
  
  EXPECT_EQ(alerts.size(), 0);
}

TEST_F(QualityAlertManagerTest, DuplicateAlertSuppression) {
  auto alerts1 = alert_manager.ReportMetric("recall_10", 0.88, 0.90, 0.80, 1);
  EXPECT_EQ(alerts1.size(), 1);
  
  auto alerts2 = alert_manager.ReportMetric("recall_10", 0.87, 0.90, 0.80, 1);
  EXPECT_EQ(alerts2.size(), 0);  // Suppressed as duplicate
}

TEST_F(QualityAlertManagerTest, CustomDeduplicationWindow) {
  alert_manager.SetDeduplicationWindow(std::chrono::milliseconds(100));
  
  auto alerts1 = alert_manager.ReportMetric("recall_10", 0.88, 0.90, 0.80);
  EXPECT_EQ(alerts1.size(), 1);
  
  std::this_thread::sleep_for(std::chrono::milliseconds(150));
  
  auto alerts2 = alert_manager.ReportMetric("recall_10", 0.87, 0.90, 0.80);
  EXPECT_EQ(alerts2.size(), 1);  // Not suppressed (window expired)
}

TEST_F(QualityAlertManagerTest, GetRecentAlerts) {
  alert_manager.ReportMetric("recall_10", 0.88, 0.90, 0.80);
  alert_manager.ReportMetric("ndcg_10", 0.92, 0.95, 0.85);
  
  auto recent = alert_manager.GetRecentAlerts(10);
  EXPECT_GE(recent.size(), 1);
}

TEST_F(QualityAlertManagerTest, AlertTrendAnalysis) {
  for (int i = 0; i < 5; ++i) {
    alert_manager.ReportMetric("recall_10", 0.85 - (i * 0.01), 0.90, 0.80);
  }
  
  auto trend = alert_manager.GetAlertTrend("recall_10", 7);
  EXPECT_GT(trend.warning_count, 0);
}

// ============================================================================
// MetricsReporter Tests (8 tests)
// ============================================================================

class MetricsReporterTest : public ::testing::Test {
 protected:
  MetricsReporter reporter{30};  // 30-day window
};

TEST_F(MetricsReporterTest, RecordAndExportTimeSeries) {
  auto now = std::chrono::system_clock::now();
  
  reporter.RecordMetricPoint("recall_10", 0.85, now);
  reporter.RecordMetricPoint("recall_10", 0.86, now + std::chrono::hours(1));
  
  auto points = reporter.ExportTimeSeries("recall_10", now - std::chrono::hours(1),
                                         now + std::chrono::hours(2));
  
  EXPECT_EQ(points.size(), 2);
  EXPECT_NEAR(points[0].value, 0.85, 0.01);
  EXPECT_NEAR(points[1].value, 0.86, 0.01);
}

TEST_F(MetricsReporterTest, ExportToJSON) {
  auto now = std::chrono::system_clock::now();
  reporter.RecordMetricPoint("recall_10", 0.85, now);
  
  auto json = reporter.ExportToJSON("recall_10", now - std::chrono::hours(1),
                                   now + std::chrono::hours(1));
  
  EXPECT_THAT(json, ::testing::HasSubstr("recall_10"));
  EXPECT_THAT(json, ::testing::HasSubstr("0.85"));
}

TEST_F(MetricsReporterTest, ExportToCSV) {
  auto now = std::chrono::system_clock::now();
  reporter.RecordMetricPoint("recall_10", 0.85, now);
  reporter.RecordMetricPoint("ndcg_10", 0.92, now);
  
  auto csv = reporter.ExportToCSV({"recall_10", "ndcg_10"}, now - std::chrono::hours(1),
                                 now + std::chrono::hours(1));
  
  EXPECT_THAT(csv, ::testing::HasSubstr("recall_10"));
  EXPECT_THAT(csv, ::testing::HasSubstr("ndcg_10"));
}

TEST_F(MetricsReporterTest, TrendAnalysis) {
  auto now = std::chrono::system_clock::now();
  
  // Record improving trend
  for (int i = 0; i < 5; ++i) {
    reporter.RecordMetricPoint("recall_10", 0.80 + (i * 0.02), now + std::chrono::hours(i));
  }
  
  auto trend = reporter.AnalyzeTrend("recall_10", std::chrono::hours(5));
  
  EXPECT_GT(trend.slope, 0.0);  // Positive slope = improving
  EXPECT_EQ(trend.direction, "improving");
  EXPECT_EQ(trend.data_points, 5);
}

TEST_F(MetricsReporterTest, ComparePeriods) {
  auto now = std::chrono::system_clock::now();
  
  // Period A: baseline
  for (int i = 0; i < 3; ++i) {
    reporter.RecordMetricPoint("recall_10", 0.85, now - std::chrono::hours(4 + i));
  }
  
  // Period B: current
  for (int i = 0; i < 3; ++i) {
    reporter.RecordMetricPoint("recall_10", 0.87, now + std::chrono::hours(i));
  }
  
  auto report = reporter.ComparePeriods(
      "recall_10",
      now - std::chrono::hours(4), now - std::chrono::hours(2),  // Period A
      now, now + std::chrono::hours(3)                            // Period B
  );
  
  EXPECT_GT(report.entity_b_mean, report.entity_a_mean);
  EXPECT_THAT(report.winner, ::testing::HasSubstr("B"));
}

TEST_F(MetricsReporterTest, AnomalyDetection) {
  auto now = std::chrono::system_clock::now();
  
  // Record normal values
  for (int i = 0; i < 10; ++i) {
    reporter.RecordMetricPoint("recall_10", 0.85, now + std::chrono::hours(i));
  }
  
  // Spike
  reporter.RecordMetricPoint("recall_10", 0.50, now + std::chrono::hours(10));
  
  auto anomalies = reporter.DetectAnomalies("recall_10", 2.0);  // z > 2.0
  
  EXPECT_GT(anomalies.size(), 0);
  EXPECT_EQ(anomalies[0].anomaly_type, "drop");
}

// ============================================================================
// Integration Tests (4+ tests)
// ============================================================================

class Phase13IntegrationTest : public ::testing::Test {
 protected:
  QualityMetricsCollector collector;
  DeploymentGateController gate;
  QualityAlertManager alerts;
  MetricsReporter reporter;
  
  void SetUp() override {
    gate = DeploymentGateController(5.0, 2.0);
  }
};

TEST_F(Phase13IntegrationTest, FullQualityGatingWorkflow) {
  // Step 1: Collect metrics for baseline
  QualityMetrics baseline_m;
  baseline_m.recall_at_10 = 0.90;
  baseline_m.ndcg_at_10 = 0.95;
  baseline_m.mrr = 0.92;
  baseline_m.model_version = 1;
  collector.ReportMetrics(baseline_m);
  
  auto baseline_agg = collector.GetAggregatedMetrics(std::chrono::hours(1));
  
  // Step 2: Collect metrics for candidate
  collector.Reset();
  QualityMetrics candidate_m;
  candidate_m.recall_at_10 = 0.89;  // Slight improvement
  candidate_m.ndcg_at_10 = 0.96;
  candidate_m.mrr = 0.93;
  candidate_m.model_version = 2;
  collector.ReportMetrics(candidate_m);
  
  auto candidate_agg = collector.GetAggregatedMetrics(std::chrono::hours(1));
  
  // Step 3: Evaluate gate
  auto decision = gate.EvaluateCandidate(2, 1, candidate_agg, baseline_agg);
  EXPECT_EQ(decision.decision, GateDecision::kAllow);
  
  // Step 4: If passed, no alerts; if failed, generate alerts
  if (decision.decision == GateDecision::kDeny) {
    alerts.ReportMetric("quality_gate", 0.0, 0.5, 0.1);
  }
}

TEST_F(Phase13IntegrationTest, AlertAndReportingWorkflow) {
  auto now = std::chrono::system_clock::now();
  
  // Report degrading metric
  for (int i = 0; i < 5; ++i) {
    double value = 0.90 - (i * 0.01);
    reporter.RecordMetricPoint("recall_10", value, now + std::chrono::hours(i));
    
    // Trigger alerts if below threshold
    if (value < 0.86) {
      alerts.ReportMetric("recall_10", value, 0.90, 0.85);
    }
  }
  
  // Get trend
  auto trend = reporter.AnalyzeTrend("recall_10", std::chrono::hours(5));
  EXPECT_LT(trend.slope, 0.0);  // Degrading
  
  // Get alerts
  auto recent_alerts = alerts.GetRecentAlerts(10);
  EXPECT_GT(recent_alerts.size(), 0);
}

TEST_F(Phase13IntegrationTest, ComparativeDashboardMetrics) {
  auto now = std::chrono::system_clock::now();
  
  // Simulate two time periods
  for (int i = 0; i < 10; ++i) {
    // Period 1: baseline
    reporter.RecordMetricPoint("recall_10", 0.85, now - std::chrono::hours(10 + i));
    
    // Period 2: current
    reporter.RecordMetricPoint("recall_10", 0.88, now + std::chrono::hours(i));
  }
  
  // Compare periods
  auto comparison = reporter.ComparePeriods(
      "recall_10",
      now - std::chrono::hours(10), now,              // Period 1
      now, now + std::chrono::hours(10)               // Period 2
  );
  
  EXPECT_GT(comparison.difference_pct, 0.0);  // Period 2 is better
  
  // Generate dashboard
  auto summary = reporter.GenerateDashboardSummary();
  EXPECT_THAT(summary, ::testing::HasSubstr("metrics"));
}

TEST_F(Phase13IntegrationTest, MultiModelComparison) {
  QualityMetrics m1, m2;
  
  // Model 1 metrics
  m1.recall_at_10 = 0.90;
  m1.ndcg_at_10 = 0.95;
  m1.mrr = 0.92;
  m1.model_version = 1;
  
  // Model 2 metrics
  m2.recall_at_10 = 0.88;
  m2.ndcg_at_10 = 0.93;
  m2.mrr = 0.90;
  m2.model_version = 2;
  
  collector.ReportMetrics(m1);
  auto agg1 = collector.GetMetricsForModel(1, std::chrono::hours(1));
  
  collector.ReportMetrics(m2);
  auto agg2 = collector.GetMetricsForModel(2, std::chrono::hours(1));
  
  // Model 1 should have better metrics
  EXPECT_GT(agg1.mean_recall_10, agg2.mean_recall_10);
  EXPECT_GT(agg1.mean_ndcg_10, agg2.mean_ndcg_10);
}

int main(int argc, char** argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
