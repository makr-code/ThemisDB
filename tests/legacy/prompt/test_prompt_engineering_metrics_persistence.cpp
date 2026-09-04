/**
 * @file test_prompt_engineering_metrics_persistence.cpp
 * @brief Tests for PromptEngineeringMetrics snapshot/restore and alerting (issues 2.5 & 2.6)
 */

#include <gtest/gtest.h>
#include "prompt_engineering/prompt_engineering_metrics.h"
#include <atomic>
#include <string>

using namespace themis::prompt_engineering;

// ============================================================================
// Snapshot / restore
// ============================================================================

TEST(PromptMetricsPersistenceTest, SnapshotCapturesCounters) {
    PromptEngineeringMetrics m;
    m.recordOptimizationAttempt("p1");
    m.recordOptimizationAttempt("p1");
    m.recordOptimizationSuccess("p1", 0.1);
    m.recordPromptExecution("p1", true, 100.0);
    m.recordHallucinationDetection("p1");

    auto snap = m.snapshotToJson();
    EXPECT_EQ(snap["optimization_attempts"].get<int64_t>(), 2);
    EXPECT_EQ(snap["optimization_successes"].get<int64_t>(), 1);
    EXPECT_EQ(snap["prompt_executions"].get<int64_t>(), 1);
    EXPECT_EQ(snap["hallucination_detections"].get<int64_t>(), 1);
}

TEST(PromptMetricsPersistenceTest, RestoreFromSnapshot) {
    PromptEngineeringMetrics original;
    original.recordOptimizationAttempt("p1");
    original.recordOptimizationAttempt("p1");
    original.recordVersionCommit("p1", "main");
    original.recordFeedback("p1", "USER_NEGATIVE");

    auto snap = original.snapshotToJson();

    // Restore into a fresh instance
    PromptEngineeringMetrics restored;
    restored.restoreFromJson(snap);

    auto snap2 = restored.snapshotToJson();
    EXPECT_EQ(snap["optimization_attempts"].get<int64_t>(),
              snap2["optimization_attempts"].get<int64_t>());
    EXPECT_EQ(snap["version_commits"].get<int64_t>(),
              snap2["version_commits"].get<int64_t>());
    EXPECT_EQ(snap["feedback_negative"].get<int64_t>(),
              snap2["feedback_negative"].get<int64_t>());
}

TEST(PromptMetricsPersistenceTest, RestoreFromEmptySnapshot_NoThrow) {
    PromptEngineeringMetrics m;
    EXPECT_NO_THROW(m.restoreFromJson(nlohmann::json::object()));
}

TEST(PromptMetricsPersistenceTest, SnapshotRoundtrip_AllZeroAfterReset) {
    PromptEngineeringMetrics m;
    m.recordOptimizationAttempt("p");
    m.reset();

    auto snap = m.snapshotToJson();
    EXPECT_EQ(snap["optimization_attempts"].get<int64_t>(), 0);
}

// ============================================================================
// Alerting
// ============================================================================

TEST(PromptMetricsAlertTest, FailureRateAlertFired) {
    PromptEngineeringMetrics m;

    PromptEngineeringMetrics::AlertConfig cfg;
    cfg.max_failure_rate = 0.3;  // fire if >30% failure
    m.setAlertConfig(cfg);

    std::atomic<int> alert_count{0};
    std::string last_metric = {};
    m.setAlertCallback([&](const PromptEngineeringMetrics::AlertEvent& ev) {
        alert_count.fetch_add(1);
        last_metric = ev.metric_name;
    });

    // 1 success + 2 failures → failure rate = 2/3 ≈ 67% > 30%
    m.recordPromptExecution("p1", true,  50.0);
    m.recordPromptExecution("p1", false, 50.0);
    m.recordPromptExecution("p1", false, 50.0);

    EXPECT_GT(alert_count.load(), 0);
    EXPECT_EQ(last_metric, "prompt_failure_rate");
}

TEST(PromptMetricsAlertTest, NoAlertWhenBelowThreshold) {
    PromptEngineeringMetrics m;

    PromptEngineeringMetrics::AlertConfig cfg;
    cfg.max_failure_rate = 0.5;  // fire if >50% failure
    m.setAlertConfig(cfg);

    std::atomic<int> alert_count{0};
    m.setAlertCallback([&](const PromptEngineeringMetrics::AlertEvent&) {
        alert_count.fetch_add(1);
    });

    // All successes → 0% failure < 50%
    m.recordPromptExecution("p1", true, 50.0);
    m.recordPromptExecution("p1", true, 50.0);

    EXPECT_EQ(alert_count.load(), 0);
}

TEST(PromptMetricsAlertTest, HallucinationAlertFired) {
    PromptEngineeringMetrics m;

    PromptEngineeringMetrics::AlertConfig cfg;
    cfg.max_hallucinations = 3;
    m.setAlertConfig(cfg);

    std::atomic<int> alert_count{0};
    m.setAlertCallback([&](const PromptEngineeringMetrics::AlertEvent&) {
        alert_count.fetch_add(1);
    });

    for (int i = 0; i < 5; ++i) {
        m.recordHallucinationDetection("p1");
    }

    EXPECT_GT(alert_count.load(), 0);
}

TEST(PromptMetricsAlertTest, NoAlertCallbackRegistered_NoThrow) {
    PromptEngineeringMetrics m;

    PromptEngineeringMetrics::AlertConfig cfg;
    cfg.max_failure_rate = 0.0;  // fire immediately on any failure
    m.setAlertConfig(cfg);
    // No callback registered

    EXPECT_NO_THROW(m.recordPromptExecution("p1", false, 50.0));
}
