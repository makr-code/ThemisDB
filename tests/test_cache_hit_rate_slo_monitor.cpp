// Copyright 2026 ThemisDB
// Tests for cache hit rate SLO monitor

#include <gtest/gtest.h>
#include "cache/cache_hit_rate_slo_monitor.h"
#include "cache/cache_metrics.h"
#include "observability/alertmanager.h"
#include <memory>
#include <vector>

using namespace themis::cache;
using namespace themis::observability;

// ---------------------------------------------------------------------------
// Mock alertmanager to capture fired/resolved alerts in tests
// ---------------------------------------------------------------------------

namespace {
class MockAlertmanager : public Alertmanager {
public:
    std::vector<Alert> sent_alerts;
    std::vector<std::string> resolved_ids;

    themis::Result<void> sendAlert(const Alert& alert) override {
        sent_alerts.push_back(alert);
        return {};
    }

    themis::Result<void> resolveAlert(const std::string& alert_id) override {
        resolved_ids.push_back(alert_id);
        return {};
    }
};
} // namespace

// ---------------------------------------------------------------------------
// Helper: build metrics with given hits and misses
// ---------------------------------------------------------------------------

static CacheMetrics makeMetrics(uint64_t l1_hits, uint64_t misses,
                                 uint64_t l2_hits = 0, uint64_t l3_hits = 0) {
    CacheMetrics m;
    m.l1_hits = l1_hits;
    m.l2_hits = l2_hits;
    m.l3_hits = l3_hits;
    m.misses  = misses;
    return m;
}

// ---------------------------------------------------------------------------
// Test fixture
// ---------------------------------------------------------------------------

class CacheHitRateSloMonitorTest : public ::testing::Test {
protected:
    void SetUp() override {
        mock_ = std::make_shared<MockAlertmanager>();

        config_.warning_threshold   = 0.60;
        config_.critical_threshold  = 0.40;
        config_.min_requests        = 10;
        config_.alert_cooldown_seconds = 0;  // no cooldown for fast testing
        config_.cache_name          = "test_cache";
    }

    CacheHitRateSloMonitor::Config config_;
    std::shared_ptr<MockAlertmanager> mock_;
};

// ---------------------------------------------------------------------------
// Tests: basic evaluation
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, NoViolation_WhenHitRateAboveWarning) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // 80% hit rate – well above warning threshold (0.60)
    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_DOUBLE_EQ(result.hit_rate, 0.80);
    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_FALSE(result.alert_fired);
    EXPECT_FALSE(result.alert_resolved);
    EXPECT_TRUE(mock_->sent_alerts.empty());
    EXPECT_FALSE(monitor.isSloViolated());
}

TEST_F(CacheHitRateSloMonitorTest, WarningFired_WhenHitRateBelowWarning) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // 50% hit rate – below warning (0.60) but above critical (0.40)
    auto result = monitor.evaluate(makeMetrics(50, 50));

    EXPECT_NEAR(result.hit_rate, 0.50, 1e-9);
    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::WARNING);
    EXPECT_TRUE(result.alert_fired);
    EXPECT_FALSE(result.alert_resolved);
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    EXPECT_EQ(mock_->sent_alerts[0].severity, AlertSeverity::WARNING);
    EXPECT_TRUE(monitor.isSloViolated());
}

TEST_F(CacheHitRateSloMonitorTest, CriticalFired_WhenHitRateBelowCritical) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // 20% hit rate – below critical threshold (0.40)
    auto result = monitor.evaluate(makeMetrics(20, 80));

    EXPECT_NEAR(result.hit_rate, 0.20, 1e-9);
    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::CRITICAL);
    EXPECT_TRUE(result.alert_fired);
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    EXPECT_EQ(mock_->sent_alerts[0].severity, AlertSeverity::CRITICAL);
    EXPECT_TRUE(monitor.isSloViolated());
}

TEST_F(CacheHitRateSloMonitorTest, AlertResolved_WhenHitRateRecovers) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // First: trigger warning
    monitor.evaluate(makeMetrics(50, 50));
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);

    // Then: recover
    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_TRUE(result.alert_resolved);
    EXPECT_EQ(mock_->resolved_ids.size(), 1u);
    EXPECT_FALSE(monitor.isSloViolated());
}

TEST_F(CacheHitRateSloMonitorTest, BelowMinRequests_NoAlertFired) {
    config_.min_requests = 1000;
    CacheHitRateSloMonitor monitor(config_, mock_);

    // Only 10 requests – below min_requests threshold
    auto result = monitor.evaluate(makeMetrics(0, 10));

    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_FALSE(result.alert_fired);
    EXPECT_TRUE(mock_->sent_alerts.empty());
    EXPECT_FALSE(monitor.isSloViolated());
}

TEST_F(CacheHitRateSloMonitorTest, NoAlertmanager_ViolationTrackedLocally) {
    // No alertmanager – violations should still be tracked internally
    CacheHitRateSloMonitor monitor(config_, nullptr);

    auto result = monitor.evaluate(makeMetrics(20, 80));

    EXPECT_TRUE(result.alert_fired);
    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::CRITICAL);
    EXPECT_TRUE(monitor.isSloViolated());
}

// ---------------------------------------------------------------------------
// Tests: hit rate computation across tiers
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, HitRateAggregatesAllTiers) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // l1=20, l2=30, l3=10 hits, 40 misses -> total=100, hit_rate=0.60
    auto result = monitor.evaluate(makeMetrics(/*l1=*/20, /*misses=*/40, /*l2=*/30, /*l3=*/10));

    EXPECT_NEAR(result.hit_rate, 0.60, 1e-9);
    EXPECT_EQ(result.total_requests, 100u);
    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
}

TEST_F(CacheHitRateSloMonitorTest, ZeroRequests_NoEvaluation) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    auto result = monitor.evaluate(makeMetrics(0, 0));

    EXPECT_DOUBLE_EQ(result.hit_rate, 0.0);
    EXPECT_EQ(result.total_requests, 0u);
    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_FALSE(result.alert_fired);
}

// ---------------------------------------------------------------------------
// Tests: status / inspection API
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, GetStatus_ReflectsCurrentState) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    monitor.evaluate(makeMetrics(50, 50));  // trigger warning

    auto status = monitor.getStatus();
    EXPECT_EQ(status["violation_level"], "WARNING");
    EXPECT_NEAR(status["hit_rate"].get<double>(), 0.50, 1e-9);
    EXPECT_EQ(status["thresholds"]["warning"].get<double>(), config_.warning_threshold);
    EXPECT_EQ(status["thresholds"]["critical"].get<double>(), config_.critical_threshold);
    EXPECT_FALSE(status["alerts"].empty());
}

TEST_F(CacheHitRateSloMonitorTest, GetActiveAlertIds_ReturnsCorrectId) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    monitor.evaluate(makeMetrics(50, 50));  // trigger warning

    auto ids = monitor.getActiveAlertIds();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_NE(ids[0].find("warning"), std::string::npos);
}

TEST_F(CacheHitRateSloMonitorTest, GetCurrentViolationLevel_MatchesEvaluation) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    EXPECT_EQ(monitor.getCurrentViolationLevel(),
              CacheHitRateSloMonitor::ViolationLevel::NONE);

    monitor.evaluate(makeMetrics(20, 80));  // critical

    EXPECT_EQ(monitor.getCurrentViolationLevel(),
              CacheHitRateSloMonitor::ViolationLevel::CRITICAL);
}

// ---------------------------------------------------------------------------
// Tests: violation level escalation
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, EscalationFromWarningToCritical) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // First: warning
    monitor.evaluate(makeMetrics(50, 50));
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    EXPECT_EQ(mock_->sent_alerts[0].severity, AlertSeverity::WARNING);

    // Then: escalate to critical
    auto result = monitor.evaluate(makeMetrics(20, 80));

    EXPECT_EQ(result.level, CacheHitRateSloMonitor::ViolationLevel::CRITICAL);
    EXPECT_TRUE(result.alert_fired);
    // The previous warning should have been resolved before firing critical
    EXPECT_GE(mock_->resolved_ids.size(), 1u);
    EXPECT_EQ(mock_->sent_alerts.back().severity, AlertSeverity::CRITICAL);
}

// ---------------------------------------------------------------------------
// Tests: ViolationLevelToString
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, ViolationLevelToString) {
    EXPECT_EQ(CacheHitRateSloMonitor::violationLevelToString(
                  CacheHitRateSloMonitor::ViolationLevel::NONE),
              "NONE");
    EXPECT_EQ(CacheHitRateSloMonitor::violationLevelToString(
                  CacheHitRateSloMonitor::ViolationLevel::WARNING),
              "WARNING");
    EXPECT_EQ(CacheHitRateSloMonitor::violationLevelToString(
                  CacheHitRateSloMonitor::ViolationLevel::CRITICAL),
              "CRITICAL");
}

// ---------------------------------------------------------------------------
// Tests: setAlertmanager after construction
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, SetAlertmanager_AlertsSentAfterSwap) {
    CacheHitRateSloMonitor monitor(config_, nullptr);

    // Trigger violation before alertmanager is set – should not crash
    auto result1 = monitor.evaluate(makeMetrics(20, 80));
    EXPECT_TRUE(result1.alert_fired);

    // Swap in a mock alertmanager and re-evaluate to fire a new alert
    // (reset violation first by recovering, then drop again)
    monitor.evaluate(makeMetrics(80, 20));   // recover
    monitor.setAlertmanager(mock_);
    auto result2 = monitor.evaluate(makeMetrics(20, 80));  // trigger again

    EXPECT_TRUE(result2.alert_fired);
    EXPECT_EQ(mock_->sent_alerts.size(), 1u);
}

// ---------------------------------------------------------------------------
// Tests: cooldown suppresses repeated same-level alerts
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, Cooldown_SuppressesRepeatSameLevelAlert) {
    config_.alert_cooldown_seconds = 60;  // 60-second cooldown
    CacheHitRateSloMonitor monitor(config_, mock_);

    // First evaluation fires warning
    auto r1 = monitor.evaluate(makeMetrics(50, 50));
    EXPECT_TRUE(r1.alert_fired);
    EXPECT_EQ(mock_->sent_alerts.size(), 1u);

    // Immediate second evaluation at same level: cooldown not expired -- suppressed
    auto r2 = monitor.evaluate(makeMetrics(50, 50));
    EXPECT_FALSE(r2.alert_fired);
    EXPECT_FALSE(r2.alert_resolved);
    EXPECT_EQ(mock_->sent_alerts.size(), 1u);  // still only 1 alert sent

    // Violation level is still tracked correctly
    EXPECT_EQ(r2.level, CacheHitRateSloMonitor::ViolationLevel::WARNING);
    EXPECT_TRUE(monitor.isSloViolated());
}

// ---------------------------------------------------------------------------
// Tests: alert label and annotation content
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, AlertLabels_ContainRequiredFields) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    monitor.evaluate(makeMetrics(20, 80));  // critical

    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    const auto& alert = mock_->sent_alerts[0];

    // Check required labels for Prometheus routing
    EXPECT_EQ(alert.labels.at("component"), "cache");
    EXPECT_EQ(alert.labels.at("cache_name"), "test_cache");
    EXPECT_EQ(alert.labels.at("alertname"), "CacheHitRateSloViolation");
    EXPECT_EQ(alert.labels.at("severity"), "CRITICAL");

    // Check annotations contain numeric context
    EXPECT_FALSE(alert.annotations.at("hit_rate").empty());
    EXPECT_FALSE(alert.annotations.at("total_requests").empty());
    EXPECT_FALSE(alert.annotations.at("threshold").empty());
}

// ---------------------------------------------------------------------------
// Tests: Config::validate()
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_ValidConfig) {
    std::string err = {};
    EXPECT_TRUE(config_.validate(&err));
    EXPECT_TRUE(err.empty());
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_CriticalNotLessThanWarning) {
    config_.critical_threshold = 0.60;  // same as warning
    config_.warning_threshold  = 0.60;
    std::string err = {};
    EXPECT_FALSE(config_.validate(&err));
    EXPECT_FALSE(err.empty());

    config_.critical_threshold = 0.70;  // critical > warning – nonsensical
    EXPECT_FALSE(config_.validate(&err));
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_ThresholdOutOfRange) {
    config_.warning_threshold = 1.50;  // > 1.0
    std::string err = {};
    EXPECT_FALSE(config_.validate(&err));

    config_.warning_threshold = 0.60;
    config_.critical_threshold = -0.1;  // < 0.0
    EXPECT_FALSE(config_.validate(&err));
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_NegativeCooldown) {
    config_.alert_cooldown_seconds = -1;
    std::string err = {};
    EXPECT_FALSE(config_.validate(&err));
    EXPECT_FALSE(err.empty());
}

// ---------------------------------------------------------------------------
// Tests: latency recording and percentile computation
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, RecordLatency_PercentilesInEvaluationResult) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // Record 100 L1 samples all in the 0.5-1ms bucket
    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 0.7);
    }

    auto result = monitor.evaluate(makeMetrics(80, 20));

    // p50, p95, p99 should all be non-zero and in the same bucket range
    EXPECT_GT(result.p50_latency_ms, 0.0);
    EXPECT_GT(result.p95_latency_ms, 0.0);
    EXPECT_GT(result.p99_latency_ms, 0.0);
    // All samples are in the same bucket so all percentiles are equal
    EXPECT_DOUBLE_EQ(result.p50_latency_ms, result.p99_latency_ms);
}

TEST_F(CacheHitRateSloMonitorTest, RecordLatency_PerTierAllTiersContribute) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // L1 fast (< 0.1ms), L2 medium (1-2ms), L3 slow (10-25ms)
    for (int i = 0; i < 50; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 0.05);
    }
    for (int i = 0; i < 40; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L2, 1.5);
    }
    for (int i = 0; i < 10; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L3, 15.0);
    }

    auto result = monitor.evaluate(makeMetrics(80, 20));

    // p99 should be in the L3 bucket range (10-25ms) since 10% are slow
    EXPECT_GT(result.p99_latency_ms, result.p50_latency_ms);
    EXPECT_GT(result.p99_latency_ms, 0.0);
}

TEST_F(CacheHitRateSloMonitorTest, RecordLatency_ZeroSamples_PercentilesAreZero) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_DOUBLE_EQ(result.p50_latency_ms, 0.0);
    EXPECT_DOUBLE_EQ(result.p95_latency_ms, 0.0);
    EXPECT_DOUBLE_EQ(result.p99_latency_ms, 0.0);
}

TEST_F(CacheHitRateSloMonitorTest, RecordLatency_NegativeValueClamped) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    // Should not crash; negative latency clamped to 0
    EXPECT_NO_THROW(monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, -5.0));

    auto result = monitor.evaluate(makeMetrics(80, 20));
    EXPECT_GE(result.p50_latency_ms, 0.0);
}

// ---------------------------------------------------------------------------
// Tests: latency SLO alerting
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, LatencyAlert_WarningFiredWhenP99ExceedsWarnThreshold) {
    config_.p99_warn_ms     = 10.0;
    config_.p99_critical_ms = 50.0;
    config_.alert_cooldown_seconds = 0;
    CacheHitRateSloMonitor monitor(config_, mock_);

    // p99 in the 10-25ms bucket (~17.5ms) > warn=10ms
    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 15.0);
    }

    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_TRUE(result.latency_alert_fired);
    EXPECT_EQ(result.latency_level, CacheHitRateSloMonitor::ViolationLevel::WARNING);
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    EXPECT_EQ(mock_->sent_alerts[0].alert_name, "CacheLatencySloViolation");
    EXPECT_EQ(mock_->sent_alerts[0].severity, AlertSeverity::WARNING);
    EXPECT_NE(mock_->sent_alerts[0].labels.at("alertname").find("Latency"), std::string::npos);
}

TEST_F(CacheHitRateSloMonitorTest, LatencyAlert_CriticalFiredWhenP99ExceedsCriticalThreshold) {
    config_.p99_warn_ms     = 10.0;
    config_.p99_critical_ms = 30.0;
    config_.alert_cooldown_seconds = 0;
    CacheHitRateSloMonitor monitor(config_, mock_);

    // p99 in 25-50ms bucket (~37.5ms) > critical=30ms
    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L3, 37.0);
    }

    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_TRUE(result.latency_alert_fired);
    EXPECT_EQ(result.latency_level, CacheHitRateSloMonitor::ViolationLevel::CRITICAL);
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    EXPECT_EQ(mock_->sent_alerts[0].severity, AlertSeverity::CRITICAL);
}

TEST_F(CacheHitRateSloMonitorTest, LatencyAlert_NoAlertWhenBelowWarnThreshold) {
    config_.p99_warn_ms     = 50.0;
    config_.p99_critical_ms = 100.0;
    CacheHitRateSloMonitor monitor(config_, mock_);

    // All samples in <0.1ms bucket – p99 well below 50ms threshold
    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 0.05);
    }

    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_FALSE(result.latency_alert_fired);
    EXPECT_EQ(result.latency_level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_TRUE(mock_->sent_alerts.empty());
}

TEST_F(CacheHitRateSloMonitorTest, LatencyAlert_DisabledWhenThresholdsAreZero) {
    // p99_warn_ms and p99_critical_ms both 0 (default) → no latency alerting
    CacheHitRateSloMonitor monitor(config_, mock_);

    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L3, 999.0);
    }

    auto result = monitor.evaluate(makeMetrics(80, 20));

    EXPECT_FALSE(result.latency_alert_fired);
    EXPECT_EQ(result.latency_level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_TRUE(mock_->sent_alerts.empty());
}

TEST_F(CacheHitRateSloMonitorTest, LatencyAlert_ResolvedWhenLatencyRecovers) {
    config_.p99_warn_ms     = 10.0;
    config_.p99_critical_ms = 50.0;
    config_.alert_cooldown_seconds = 0;
    CacheHitRateSloMonitor monitor(config_, mock_);

    // First: record 100 slow samples (15ms → bucket midpoint 17.5ms > 10ms warn)
    // p99 = 17.5ms → warning fires.
    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 15.0);
    }
    auto r1 = monitor.evaluate(makeMetrics(80, 20));
    ASSERT_TRUE(r1.latency_alert_fired);
    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    ASSERT_EQ(r1.latency_level, CacheHitRateSloMonitor::ViolationLevel::WARNING);

    // Now flood the histogram with 9900 fast samples (0.05ms each).
    // After this the slow 100 samples represent only 1% of total 10000 →
    // p99 now resolves to the fast bucket (<0.1ms, midpoint 0.05ms) which is
    // below the 10ms warning threshold.
    for (int i = 0; i < 9900; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 0.05);
    }
    auto r2 = monitor.evaluate(makeMetrics(160, 40));

    EXPECT_EQ(r2.latency_level, CacheHitRateSloMonitor::ViolationLevel::NONE);
    EXPECT_TRUE(r2.latency_alert_resolved);
    EXPECT_FALSE(mock_->resolved_ids.empty());
}

TEST_F(CacheHitRateSloMonitorTest, LatencyAlert_AlertIdContainsLatencyLabel) {
    config_.p99_warn_ms     = 5.0;
    config_.p99_critical_ms = 20.0;
    config_.alert_cooldown_seconds = 0;
    CacheHitRateSloMonitor monitor(config_, mock_);

    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L2, 10.0);
    }
    monitor.evaluate(makeMetrics(80, 20));

    ASSERT_EQ(mock_->sent_alerts.size(), 1u);
    const auto& alert = mock_->sent_alerts[0];
    EXPECT_NE(alert.alert_id.find("latency"), std::string::npos);
    EXPECT_FALSE(alert.annotations.at("p99_ms").empty());
    EXPECT_FALSE(alert.annotations.at("threshold").empty());
}

// ---------------------------------------------------------------------------
// Tests: getStatus() latency section
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, GetStatus_ExposesLatencyPercentiles) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    for (int i = 0; i < 100; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 0.7);
    }
    monitor.evaluate(makeMetrics(80, 20));

    auto status = monitor.getStatus();
    ASSERT_TRUE(status.contains("latency"));
    EXPECT_DOUBLE_EQ(status["latency"]["p50_ms"].get<double>(),
                     status["latency"]["l1"]["p50_ms"].get<double>());
    EXPECT_GE(status["latency"]["p99_ms"].get<double>(), 0.0);
}

TEST_F(CacheHitRateSloMonitorTest, GetStatus_LatencyPerTierBreakdown) {
    CacheHitRateSloMonitor monitor(config_, mock_);

    for (int i = 0; i < 50; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L1, 0.05);
    }
    for (int i = 0; i < 50; ++i) {
        monitor.recordLatency(CacheHitRateSloMonitor::Tier::L3, 200.0);
    }
    monitor.evaluate(makeMetrics(80, 20));

    auto status = monitor.getStatus();
    ASSERT_TRUE(status.contains("latency"));
    EXPECT_TRUE(status["latency"].contains("l1"));
    EXPECT_TRUE(status["latency"].contains("l2"));
    EXPECT_TRUE(status["latency"].contains("l3"));

    // L3 p99 should be far higher than L1 p99
    double l1_p99 = status["latency"]["l1"]["p99_ms"].get<double>();
    double l3_p99 = status["latency"]["l3"]["p99_ms"].get<double>();
    EXPECT_GT(l3_p99, l1_p99);
}

// ---------------------------------------------------------------------------
// Tests: Config::validate() – latency threshold constraints
// ---------------------------------------------------------------------------

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_LatencyThresholdsValid) {
    config_.p99_warn_ms     = 10.0;
    config_.p99_critical_ms = 50.0;
    std::string err = {};
    EXPECT_TRUE(config_.validate(&err));
    EXPECT_TRUE(err.empty());
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_LatencyCriticalMustBeGreaterThanWarn) {
    config_.p99_warn_ms     = 50.0;
    config_.p99_critical_ms = 10.0;  // critical < warn – invalid
    std::string err = {};
    EXPECT_FALSE(config_.validate(&err));
    EXPECT_FALSE(err.empty());
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_LatencyEqualThresholdsInvalid) {
    config_.p99_warn_ms     = 20.0;
    config_.p99_critical_ms = 20.0;  // equal – invalid (critical must be strictly greater)
    std::string err = {};
    EXPECT_FALSE(config_.validate(&err));
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_LatencyNegativeThresholdInvalid) {
    config_.p99_warn_ms = -1.0;
    std::string err = {};
    EXPECT_FALSE(config_.validate(&err));
}

TEST_F(CacheHitRateSloMonitorTest, ConfigValidate_LatencyOnlyOneThresholdSetIsValid) {
    // Only warn set, critical stays 0 → valid (only warning alerting enabled)
    config_.p99_warn_ms     = 20.0;
    config_.p99_critical_ms = 0.0;
    std::string err = {};
    EXPECT_TRUE(config_.validate(&err));

    // Only critical set, warn stays 0 → valid (only critical alerting enabled)
    config_.p99_warn_ms     = 0.0;
    config_.p99_critical_ms = 50.0;
    EXPECT_TRUE(config_.validate(&err));
}
