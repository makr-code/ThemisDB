/**
 * @file test_metric_anomaly_detector.cpp
 * @brief Unit tests for MetricAnomalyDetector – anomaly detection on
 *        observability metric time-series (Issue #2097).
 *
 * Tests cover:
 *  - MonitoredMetric default values
 *  - scoreSeverity thresholds (tested through MetricAnomaly.severity)
 *  - MetricAnomaly::toJson() structure
 *  - monitor() / unmonitor() / monitoredCount() / monitoredNames()
 *  - Re-registering a metric resets the stream
 *  - observe() returns nullopt for unknown metrics
 *  - observe() returns nullopt during warm-up
 *  - observe() returns result after warm-up threshold
 *  - Anomaly history accumulation and getAnomalies()
 *  - getAllAnomalies() aggregates across streams
 *  - clearAnomalies(name) clears one stream
 *  - clearAllAnomalies() clears all streams
 *  - clearAnomalies() throws for unknown metric
 *  - getAnomalies() throws for unknown metric
 *  - AnomalyCallback is invoked on anomalous observation
 *  - publishMetrics() pushes gauges to MetricsCollector
 *  - generateReport() contains expected fields
 *  - generateReportJson() has expected JSON structure
 *  - Warm-up with IQR method
 *  - Warm-up with MODIFIED_Z_SCORE method
 *  - Multiple metrics monitored independently
 */

#include <gtest/gtest.h>
#include "observability/metric_anomaly_detector.h"
#include "observability/metrics_collector.h"

#include <atomic>
#include <chrono>
#include <string>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

/// Build a MonitoredMetric with a short warm-up so tests can complete fast.
static MonitoredMetric makeMetric(
    const std::string& name,
    AnomalyMethod method = AnomalyMethod::Z_SCORE,
    double threshold = 0.6,
    size_t window_size = 200,
    size_t auto_train_after = 30)
{
    MonitoredMetric cfg;
    cfg.name             = name;
    cfg.method           = method;
    cfg.threshold        = threshold;
    cfg.window_size      = window_size;
    cfg.auto_train_after = auto_train_after;
    cfg.retrain_on_window = true;
    return cfg;
}

/// Push @p n observations of @p value into the detector for @p metric_name.
static void warmUp(MetricAnomalyDetector& det,
                   const std::string& name,
                   size_t n,
                   double value = 10.0)
{
    for (size_t i = 0; i < n; ++i) {
        det.observe(name, value);
    }
}

/// Push @p n observations that ramp from start_val to end_val.
static void warmUpRamp(MetricAnomalyDetector& det,
                       const std::string& name,
                       size_t n,
                       double start_val = 9.0,
                       double end_val   = 11.0)
{
    for (size_t i = 0; i < n; ++i) {
        double v = start_val + (end_val - start_val) * (static_cast<double>(i) / static_cast<double>(n));
        det.observe(name, v);
    }
}

// ---------------------------------------------------------------------------
// MonitoredMetric defaults
// ---------------------------------------------------------------------------

TEST(MonitoredMetricTest, DefaultValues) {
    MonitoredMetric cfg;
    EXPECT_TRUE(cfg.name.empty());
    EXPECT_EQ(AnomalyMethod::Z_SCORE, cfg.method);
    EXPECT_DOUBLE_EQ(0.7, cfg.threshold);
    EXPECT_EQ(500u,  cfg.window_size);
    EXPECT_EQ(50u,   cfg.auto_train_after);
    EXPECT_TRUE(cfg.retrain_on_window);
}

// ---------------------------------------------------------------------------
// MetricAnomaly::toJson
// ---------------------------------------------------------------------------

TEST(MetricAnomalyTest, ToJson_AllFieldsPresent) {
    MetricAnomaly a;
    a.metric_name    = "test_metric";
    a.score          = 0.85;
    a.is_anomaly     = true;
    a.observed_value = 999.0;
    a.severity       = "high";
    a.timestamp      = std::chrono::system_clock::now();
    a.description    = "spike detected";

    auto j = a.toJson();
    EXPECT_EQ("test_metric",  j.at("metric_name").get<std::string>());
    EXPECT_NEAR(0.85, j.at("score").get<double>(),          1e-9);
    EXPECT_TRUE(j.at("is_anomaly").get<bool>());
    EXPECT_NEAR(999.0, j.at("observed_value").get<double>(), 1e-9);
    EXPECT_EQ("high",        j.at("severity").get<std::string>());
    EXPECT_TRUE(j.contains("timestamp_ms"));
    EXPECT_EQ("spike detected", j.at("description").get<std::string>());
}

// ---------------------------------------------------------------------------
// monitor / unmonitor / monitoredCount / monitoredNames
// ---------------------------------------------------------------------------

class MetricAnomalyDetectorTest : public ::testing::Test {
protected:
    MetricAnomalyDetector det;
};

TEST_F(MetricAnomalyDetectorTest, Initially_Empty) {
    EXPECT_EQ(0u, det.monitoredCount());
    EXPECT_TRUE(det.monitoredNames().empty());
}

TEST_F(MetricAnomalyDetectorTest, MonitorOne_CountIsOne) {
    det.monitor(makeMetric("latency"));
    EXPECT_EQ(1u, det.monitoredCount());
    auto names = det.monitoredNames();
    ASSERT_EQ(1u, names.size());
    EXPECT_EQ("latency", names[0]);
}

TEST_F(MetricAnomalyDetectorTest, MonitorMultiple) {
    det.monitor(makeMetric("a"));
    det.monitor(makeMetric("b"));
    det.monitor(makeMetric("c"));
    EXPECT_EQ(3u, det.monitoredCount());
}

TEST_F(MetricAnomalyDetectorTest, Unmonitor_RemovesStream) {
    det.monitor(makeMetric("a"));
    det.monitor(makeMetric("b"));
    det.unmonitor("a");
    EXPECT_EQ(1u, det.monitoredCount());
    auto names = det.monitoredNames();
    EXPECT_EQ("b", names[0]);
}

TEST_F(MetricAnomalyDetectorTest, UnmonitorUnknown_IsNoOp) {
    det.monitor(makeMetric("a"));
    EXPECT_NO_THROW(det.unmonitor("nonexistent"));
    EXPECT_EQ(1u, det.monitoredCount());
}

TEST_F(MetricAnomalyDetectorTest, ReRegister_ResetsStream) {
    det.monitor(makeMetric("latency", AnomalyMethod::Z_SCORE));
    warmUp(det, "latency", 50, 10.0);
    // Re-register with a different method – should reset.
    det.monitor(makeMetric("latency", AnomalyMethod::IQR));
    EXPECT_EQ(1u, det.monitoredCount());
    // Anomalies from before re-registration should be gone.
    EXPECT_TRUE(det.getAnomalies("latency").empty());
}

// ---------------------------------------------------------------------------
// observe() – unknown metric returns nullopt
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, ObserveUnknown_ReturnsNullopt) {
    EXPECT_FALSE(det.observe("no_such_metric", 1.0).has_value());
}

// ---------------------------------------------------------------------------
// observe() – warm-up period returns nullopt
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, WarmUp_ReturnsNullopt_BeforeThreshold) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 50));
    // Feed 49 points (below auto_train_after = 50) – all should be nullopt.
    for (int i = 0; i < 49; ++i) {
        EXPECT_FALSE(det.observe("lat", 10.0).has_value())
            << "Expected nullopt at point " << i;
    }
}

// ---------------------------------------------------------------------------
// observe() – result available after warm-up
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, AfterWarmUp_ReturnsResult) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    // Feed 30 points to complete warm-up.
    warmUp(det, "lat", 30, 10.0);
    // Next point should return a result (even if not flagged as anomaly).
    auto result = det.observe("lat", 10.0);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ("lat", result->metric_name);
    EXPECT_NEAR(10.0, result->observed_value, 1e-9);
}

// ---------------------------------------------------------------------------
// Anomaly detection fires on extreme outlier
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, ExtremeOutlier_Detected) {
    // Use Z_SCORE with a moderate threshold; feed a tight cluster then spike.
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat", 80, 10.0);  // establish tight baseline

    // Now inject a large spike – should be flagged.
    bool found_anomaly = false;
    for (int i = 0; i < 5; ++i) {
        auto r = det.observe("lat", 1000.0);
        if (r && r->is_anomaly) {
            found_anomaly = true;
            EXPECT_GT(r->score, 0.6);
            EXPECT_FALSE(r->severity.empty());
        }
    }
    EXPECT_TRUE(found_anomaly) << "Expected at least one anomaly on spike to 1000.0";
}

// ---------------------------------------------------------------------------
// Anomaly history
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, AnomalyHistory_Accumulated) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat", 80, 10.0);

    for (int i = 0; i < 5; ++i) {
        det.observe("lat", 10000.0);
    }
    auto anomalies = det.getAnomalies("lat");
    EXPECT_GE(anomalies.size(), 1u);
    for (const auto& a : anomalies) {
        EXPECT_TRUE(a.is_anomaly);
        EXPECT_EQ("lat", a.metric_name);
    }
}

// ---------------------------------------------------------------------------
// getAnomalies() throws for unknown metric
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, GetAnomalies_ThrowsForUnknown) {
    EXPECT_THROW(det.getAnomalies("no_such"), std::out_of_range);
}

// ---------------------------------------------------------------------------
// getAllAnomalies
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, GetAllAnomalies_AggregatesAcrossStreams) {
    det.monitor(makeMetric("a", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    det.monitor(makeMetric("b", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "a", 80, 10.0);
    warmUp(det, "b", 80, 10.0);

    // Spike both metrics
    for (int i = 0; i < 3; ++i) {
        det.observe("a", 9999.0);
        det.observe("b", 9999.0);
    }

    auto all = det.getAllAnomalies();
    // Should have entries from both streams
    bool saw_a = false, saw_b = false;
    for (const auto& a : all) {
        if (a.metric_name == "a") saw_a = true;
        if (a.metric_name == "b") saw_b = true;
    }
    EXPECT_TRUE(saw_a);
    EXPECT_TRUE(saw_b);
}

// ---------------------------------------------------------------------------
// clearAnomalies
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, ClearAnomalies_ClearsOneStream) {
    det.monitor(makeMetric("a", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    det.monitor(makeMetric("b", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "a", 80, 10.0);
    warmUp(det, "b", 80, 10.0);
    for (int i = 0; i < 3; ++i) {
        det.observe("a", 9999.0);
        det.observe("b", 9999.0);
    }
    det.clearAnomalies("a");
    EXPECT_TRUE(det.getAnomalies("a").empty());
    // b should still have history from the spikes
    EXPECT_GT(det.getAnomalies("b").size(), 0u);
}

TEST_F(MetricAnomalyDetectorTest, ClearAnomalies_ThrowsForUnknown) {
    EXPECT_THROW(det.clearAnomalies("nonexistent"), std::out_of_range);
}

TEST_F(MetricAnomalyDetectorTest, ClearAllAnomalies) {
    det.monitor(makeMetric("a", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    det.monitor(makeMetric("b", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "a", 80, 10.0);
    warmUp(det, "b", 80, 10.0);
    for (int i = 0; i < 3; ++i) {
        det.observe("a", 9999.0);
        det.observe("b", 9999.0);
    }
    det.clearAllAnomalies();
    EXPECT_TRUE(det.getAnomalies("a").empty());
    EXPECT_TRUE(det.getAnomalies("b").empty());
}

// ---------------------------------------------------------------------------
// AnomalyCallback
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, Callback_InvokedOnAnomaly) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat", 80, 10.0);

    std::atomic<int> callback_count{0};
    det.setCallback([&](const MetricAnomaly& a) {
        EXPECT_TRUE(a.is_anomaly);
        EXPECT_EQ("lat", a.metric_name);
        ++callback_count;
    });

    for (int i = 0; i < 5; ++i) {
        det.observe("lat", 9999.0);
    }
    EXPECT_GE(callback_count.load(), 1);
}

TEST_F(MetricAnomalyDetectorTest, Callback_NotInvoked_WhenNormal) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat", 80, 10.0);

    std::atomic<int> callback_count{0};
    det.setCallback([&](const MetricAnomaly&) { ++callback_count; });

    // Feed values within the normal range.
    for (int i = 0; i < 10; ++i) {
        det.observe("lat", 10.0);
    }
    EXPECT_EQ(0, callback_count.load());
}

TEST_F(MetricAnomalyDetectorTest, Callback_CanBeCleared) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat", 80, 10.0);

    std::atomic<int> callback_count{0};
    det.setCallback([&](const MetricAnomaly&) { ++callback_count; });
    det.setCallback({});  // clear

    for (int i = 0; i < 5; ++i) {
        det.observe("lat", 9999.0);
    }
    EXPECT_EQ(0, callback_count.load());
}

// ---------------------------------------------------------------------------
// publishMetrics
// ---------------------------------------------------------------------------

class MetricAnomalyPublishTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
    MetricAnomalyDetector det;
};

TEST_F(MetricAnomalyPublishTest, PublishMetrics_GaugesAppearInOutput) {
    det.monitor(makeMetric("query_lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "query_lat", 80, 10.0);
    det.publishMetrics();

    std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(std::string::npos, prom.find("themis_anomaly_score"))
        << "Expected themis_anomaly_score gauge";
    EXPECT_NE(std::string::npos, prom.find("themis_anomaly_detected"))
        << "Expected themis_anomaly_detected gauge";
    EXPECT_NE(std::string::npos, prom.find("themis_anomaly_total"))
        << "Expected themis_anomaly_total gauge";
    EXPECT_NE(std::string::npos, prom.find("themis_anomaly_window_size"))
        << "Expected themis_anomaly_window_size gauge";
}

// ---------------------------------------------------------------------------
// generateReport / generateReportJson
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, GenerateReport_ContainsKeyFields) {
    det.monitor(makeMetric("q_lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "q_lat", 30, 10.0);

    std::string report = det.generateReport();
    EXPECT_NE(std::string::npos, report.find("q_lat"))       << report;
    EXPECT_NE(std::string::npos, report.find("Method"))       << report;
    EXPECT_NE(std::string::npos, report.find("Points seen"))  << report;
    EXPECT_NE(std::string::npos, report.find("Threshold"))    << report;
}

TEST_F(MetricAnomalyDetectorTest, GenerateReportJson_Structure) {
    det.monitor(makeMetric("lat", AnomalyMethod::Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat", 30, 10.0);

    auto j = det.generateReportJson();
    EXPECT_TRUE(j.contains("generated_at_ms"));
    EXPECT_TRUE(j.contains("monitored_count"));
    EXPECT_TRUE(j.contains("metrics"));
    EXPECT_EQ(1u, j.at("monitored_count").get<size_t>());

    auto metrics = j.at("metrics");
    ASSERT_EQ(1u, metrics.size());
    auto& m0 = metrics[0];
    EXPECT_TRUE(m0.contains("metric_name"));
    EXPECT_TRUE(m0.contains("method"));
    EXPECT_TRUE(m0.contains("threshold"));
    EXPECT_TRUE(m0.contains("points_seen"));
    EXPECT_TRUE(m0.contains("trained"));
    EXPECT_TRUE(m0.contains("anomaly_count"));
    EXPECT_TRUE(m0.contains("anomalies"));
    EXPECT_EQ("lat", m0.at("metric_name").get<std::string>());
}

// ---------------------------------------------------------------------------
// IQR method warm-up test
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, IQR_Method_WarmUpAndDetect) {
    det.monitor(makeMetric("lat_iqr", AnomalyMethod::IQR, 0.6, 200, 30));
    warmUp(det, "lat_iqr", 80, 10.0);
    // Normal value should return result after warm-up
    auto r = det.observe("lat_iqr", 10.0);
    EXPECT_TRUE(r.has_value());
}

// ---------------------------------------------------------------------------
// MODIFIED_Z_SCORE method
// ---------------------------------------------------------------------------

TEST_F(MetricAnomalyDetectorTest, ModifiedZScore_WarmUpAndDetect) {
    det.monitor(makeMetric("lat_mad", AnomalyMethod::MODIFIED_Z_SCORE, 0.6, 200, 30));
    warmUp(det, "lat_mad", 80, 10.0);
    auto r = det.observe("lat_mad", 10.0);
    EXPECT_TRUE(r.has_value());
}
