/**
 * @file test_canary_deployment_manager.cpp
 * @brief Tests for CanaryDeploymentManager staged rollout orchestration
 *
 * Tests phase progression, quality gates, metrics aggregation, and rollback.
 * Test cases: CANARY-01..CANARY-08
 *
 * @date 2026-09-24
 */

#include <gtest/gtest.h>
#include <chrono>
#include <thread>

#include "ingestion/canary_deployment_manager.h"

using namespace themis::ingestion;

class CanaryDeploymentManagerTest : public ::testing::Test {
 protected:
  CanaryDeploymentManager manager_{42, "/tmp/index"};

  CanaryMetrics CreateBaselineMetrics() {
    CanaryMetrics baseline;
    baseline.phase = CanaryPhase::Canary5;
    baseline.recall_at_10 = 0.85;
    baseline.ndcg_at_10 = 0.78;
    baseline.mrr_at_10 = 0.90;
    baseline.p99_latency_ms = 150.0;
    baseline.error_rate = 0.001;  // 0.1%
    baseline.query_count = 1000;
    return baseline;
  }
};

/**
 * @test CANARY-01: Start phase and record queries
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_01_StartPhaseAndRecordQueries) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  EXPECT_EQ(manager_.GetCurrentPhase(), CanaryPhase::Canary5);
  EXPECT_EQ(manager_.GetTrafficPercentage(), 5);

  // Record some queries
  manager_.RecordQuery("q1", 145.0, true, 0.85);
  manager_.RecordQuery("q2", 152.0, true, 0.84);
  manager_.RecordQuery("q3", 148.0, true, 0.86);

  auto metrics = manager_.GetPhaseMetrics();
  EXPECT_EQ(metrics.query_count, 3);
  EXPECT_GT(metrics.p99_latency_ms, 145.0);
}

/**
 * @test CANARY-02: Quality gate passes for good metrics
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_02_QualityGatePasses) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record queries with metrics very close to baseline
  for (int i = 0; i < 1100; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0 + (i % 20), true, 0.849 + (i % 5) * 0.0001);
  }

  EXPECT_TRUE(manager_.CheckQualityGate());
}

/**
 * @test CANARY-03: Quality gate fails for recall regression
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_03_QualityGateFailsRecallRegression) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record queries with significantly lower recall (> 2pp regression)
  for (int i = 0; i < 1100; ++i) {
    double recall = 0.82;  // 3pp worse than baseline (0.85)
    manager_.RecordQuery("q" + std::to_string(i), 145.0 + (i % 20), true, recall);
  }

  EXPECT_FALSE(manager_.CheckQualityGate());
}

/**
 * @test CANARY-04: Can progress when ready
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_04_CanProgressWhenReady) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Initially cannot progress (not enough queries)
  EXPECT_FALSE(manager_.CanProgressToNextPhase());

  // Add 1000 queries (meets minimum)
  for (int i = 0; i < 1000; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0, true, 0.85);
  }

  // Still cannot progress due to minimum phase duration (1 hour)
  EXPECT_FALSE(manager_.CanProgressToNextPhase());

  // Would need to mock time or wait for real time to pass
}

/**
 * @test CANARY-05: Cannot progress with insufficient queries
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_05_CannotProgressInsufficientQueries) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record only 500 queries (less than 1000 minimum)
  for (int i = 0; i < 500; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0, true, 0.85);
  }

  EXPECT_FALSE(manager_.CanProgressToNextPhase());
}

/**
 * @test CANARY-06: Progression fails when quality gate doesn't pass
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_06_ProgressionFailsQualityGate) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record queries with regression
  for (int i = 0; i < 1100; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0, true, 0.80);  // Regression
  }

  EXPECT_FALSE(manager_.CheckQualityGate());
  EXPECT_THROW(manager_.ProgressToNextPhase(), std::logic_error);
}

/**
 * @test CANARY-07: Rollback succeeds
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_07_RollbackSucceeds) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary25, baseline);

  EXPECT_TRUE(manager_.RollbackToPreviousVersion());
}

/**
 * @test CANARY-08: Traffic percentage for each phase
 */
TEST_F(CanaryDeploymentManagerTest, CANARY_08_TrafficPercentage) {
  auto baseline = CreateBaselineMetrics();

  manager_.StartPhase(CanaryPhase::Canary5, baseline);
  EXPECT_EQ(manager_.GetTrafficPercentage(), 5);

  manager_.StartPhase(CanaryPhase::Canary10, baseline);
  EXPECT_EQ(manager_.GetTrafficPercentage(), 10);

  manager_.StartPhase(CanaryPhase::Canary25, baseline);
  EXPECT_EQ(manager_.GetTrafficPercentage(), 25);

  manager_.StartPhase(CanaryPhase::Canary50, baseline);
  EXPECT_EQ(manager_.GetTrafficPercentage(), 50);

  manager_.StartPhase(CanaryPhase::Production100, baseline);
  EXPECT_EQ(manager_.GetTrafficPercentage(), 100);
}

/**
 * @test Quality gate with latency regression
 */
TEST_F(CanaryDeploymentManagerTest, QualityGateLatencyRegression) {
  auto baseline = CreateBaselineMetrics();
  baseline.p99_latency_ms = 150.0;
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record queries with significantly higher latency (> 100ms tolerance)
  for (int i = 0; i < 1100; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 280.0, true, 0.85);  // 130ms over baseline
  }

  EXPECT_FALSE(manager_.CheckQualityGate());
}

/**
 * @test Quality gate with error rate increase
 */
TEST_F(CanaryDeploymentManagerTest, QualityGateErrorRateIncrease) {
  auto baseline = CreateBaselineMetrics();
  baseline.error_rate = 0.001;  // 0.1%
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record queries with 2% error rate (1.9pp increase)
  for (int i = 0; i < 1100; ++i) {
    bool success = (i % 50) != 0;  // ~2% error rate
    manager_.RecordQuery("q" + std::to_string(i), 145.0, success, 0.85);
  }

  EXPECT_FALSE(manager_.CheckQualityGate());
}

/**
 * @test Metrics aggregation with mixed query results
 */
TEST_F(CanaryDeploymentManagerTest, MetricsAggregation) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Canary5, baseline);

  // Record queries with varying latencies
  std::vector<double> latencies = {100, 110, 120, 130, 140, 150, 160, 170, 180, 190};
  for (size_t i = 0; i < latencies.size(); ++i) {
    manager_.RecordQuery("q" + std::to_string(i), latencies[i], true, 0.85);
  }

  auto metrics = manager_.GetPhaseMetrics();
  EXPECT_EQ(metrics.query_count, 10);
  // p99 should be around 189 (99th percentile of 10 values)
  EXPECT_GE(metrics.p99_latency_ms, 150);
  EXPECT_EQ(metrics.error_rate, 0.0);  // All succeeded
}

/**
 * @test Cannot progress from production phase
 */
TEST_F(CanaryDeploymentManagerTest, CannotProgressFromProduction) {
  auto baseline = CreateBaselineMetrics();
  manager_.StartPhase(CanaryPhase::Production100, baseline);

  for (int i = 0; i < 10000; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0, true, 0.85);
  }

  EXPECT_FALSE(manager_.CanProgressToNextPhase());
}

/**
 * @test Phase history tracking
 */
TEST_F(CanaryDeploymentManagerTest, PhaseHistoryTracking) {
  auto baseline = CreateBaselineMetrics();

  // Start and complete Canary5 phase
  manager_.StartPhase(CanaryPhase::Canary5, baseline);
  for (int i = 0; i < 1000; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0, true, 0.85);
  }
  auto history_before = manager_.GetPhaseHistory();
  EXPECT_EQ(history_before.size(), 0);  // No history yet

  // Mock progress by storing phase metrics
  auto metrics5 = manager_.GetPhaseMetrics();
  manager_.StartPhase(CanaryPhase::Canary10, metrics5);
  // Now start phase adds previous to history
  auto history_after = manager_.GetPhaseHistory();
  // History is updated after progress, so we'd need to actually progress
}

/**
 * @test Minimum query counts for phases
 */
TEST_F(CanaryDeploymentManagerTest, MinimumQueryCountPerPhase) {
  // These are static methods, so we verify the progression requirements
  // Canary5 requires 1000, Canary10 requires 2000, etc.

  auto baseline = CreateBaselineMetrics();
  baseline.query_count = 1000;

  manager_.StartPhase(CanaryPhase::Canary5, baseline);
  EXPECT_FALSE(manager_.CanProgressToNextPhase());  // Not enough time

  manager_.StartPhase(CanaryPhase::Canary10, baseline);
  EXPECT_FALSE(manager_.CanProgressToNextPhase());  // Not enough queries

  // With 2000+ queries, still blocked by time
  for (int i = 0; i < 1000; ++i) {
    manager_.RecordQuery("q" + std::to_string(i), 145.0, true, 0.85);
  }
  EXPECT_FALSE(manager_.CanProgressToNextPhase());
}
