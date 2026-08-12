#include <gtest/gtest.h>
#include "observability/ml_anomaly_detector.h"

#include <chrono>
#include <vector>

using namespace themis::observability;
using themisdb::analytics::AnomalyMethod;

namespace {

ForecastSeries makeSeries(const std::vector<double>& values,
                          int64_t start_ms = 0,
                          int64_t interval_ms = 1000)
{
    ForecastSeries ts;
    int idx = 0;
    for (double v : values) {
        ts.push(start_ms + static_cast<int64_t>(idx) * interval_ms, v);
        ++idx;
    }
    return ts;
}

} // namespace

// ---------------------------------------------------------------------------
// Training and baseline detection (Isolation Forest + forecast deviation)
// ---------------------------------------------------------------------------

TEST(MLAnomalyDetectorTest, DetectsOutlierWithIsolationForest) {
    MLConfig cfg;
    cfg.metric_name        = "latency_ms";
    cfg.min_training_points = 10;
    cfg.anomaly_threshold  = 0.6;
    cfg.outlier_config.method    = AnomalyMethod::ISOLATION_FOREST;
    cfg.outlier_config.threshold = 0.55;
    cfg.forecast_backend   = ForecastBackend::ARIMA;

    MLAnomalyDetector det(cfg);
    det.train({makeSeries({10, 10, 11, 9, 10, 10, 10, 11, 9, 10})});

    ForecastSeries current = makeSeries({10, 10, 50}); // spike at the end
    auto anomalies = det.detectAnomalies(current);
    ASSERT_FALSE(anomalies.empty());
    const auto& a = anomalies.back();
    EXPECT_EQ("latency_ms", a.metric_name);
    EXPECT_GT(a.confidence_score, cfg.anomaly_threshold);
    EXPECT_FALSE(a.severity.empty());
}

// ---------------------------------------------------------------------------
// Prophet-style (Holt-Winters) seasonal pattern recognition
// ---------------------------------------------------------------------------

TEST(MLAnomalyDetectorTest, DetectsSeasonalResidualWithProphetBackend) {
    MLConfig cfg;
    cfg.metric_name         = "throughput_qps";
    cfg.min_training_points = 12;
    cfg.seasonality_period  = 4;
    cfg.anomaly_threshold   = 0.5;
    cfg.forecast_backend    = ForecastBackend::PROPHET;
    cfg.outlier_config.method = AnomalyMethod::ISOLATION_FOREST;

    // Seasonal pattern: repeating [10, 12, 14, 12]
    std::vector<double> seasonal = {10, 12, 14, 12, 10, 12, 14, 12, 10, 12, 14, 12};
    MLAnomalyDetector det(cfg);
    det.train({makeSeries(seasonal)});

    // Introduce a seasonal break (value far from expected seasonal component)
    ForecastSeries current = makeSeries({10, 12, 14, 24});
    auto anomalies = det.detectAnomalies(current);
    ASSERT_FALSE(anomalies.empty());
    EXPECT_EQ("throughput_qps", anomalies.front().metric_name);
    EXPECT_GT(anomalies.front().confidence_score, cfg.anomaly_threshold);
}

// ---------------------------------------------------------------------------
// Change-point detection across batch
// ---------------------------------------------------------------------------

TEST(MLAnomalyDetectorTest, DetectsChangePointShift) {
    MLConfig cfg;
    cfg.metric_name         = "cpu_usage";
    cfg.min_training_points = 8;
    cfg.change_point_threshold = 2.5;
    cfg.anomaly_threshold   = 0.5;
    cfg.outlier_config.method = AnomalyMethod::ISOLATION_FOREST;

    MLAnomalyDetector det(cfg);
    det.train({makeSeries({20, 21, 19, 20, 20, 21, 19, 20})});

    // Shift upwards -> change-point should be detected
    ForecastSeries shifted = makeSeries({20, 21, 19, 20, 35, 36, 37, 38});
    auto anomalies = det.detectAnomalies(shifted);
    ASSERT_FALSE(anomalies.empty());
    bool has_cp_factor = false;
    for (const auto& a : anomalies) {
        for (const auto& f : a.contributing_factors) {
            if (f.find("change_point") != std::string::npos) {
                has_cp_factor = true;
                break;
            }
        }
    }
    EXPECT_TRUE(has_cp_factor);
}

// ---------------------------------------------------------------------------
// DBSCAN noise handling
// ---------------------------------------------------------------------------

TEST(MLAnomalyDetectorTest, FlagsDbscanNoise) {
    MLConfig cfg;
    cfg.metric_name          = "io_wait";
    cfg.min_training_points  = 6;
    cfg.dbscan_eps           = 0.5;
    cfg.dbscan_min_samples   = 3;
    cfg.anomaly_threshold    = 0.5;
    cfg.outlier_config.method = AnomalyMethod::ISOLATION_FOREST;

    MLAnomalyDetector det(cfg);
    det.train({makeSeries({5, 5, 5, 5, 5, 5})});

    // Single noise point far away from cluster
    ForecastSeries current = makeSeries({5, 5, 5, 9});
    auto anomalies = det.detectAnomalies(current);
    ASSERT_FALSE(anomalies.empty());
    bool has_dbscan = false;
    for (const auto& f : anomalies.front().contributing_factors) {
        has_dbscan = has_dbscan || (f.find("dbscan_noise") != std::string::npos);
    }
    EXPECT_TRUE(has_dbscan);
}

// ---------------------------------------------------------------------------
// Explanation pathway
// ---------------------------------------------------------------------------

TEST(MLAnomalyDetectorTest, ExplainAnomalyIncludesFactors) {
    MLConfig cfg;
    cfg.metric_name         = "memory";
    cfg.min_training_points = 6;
    cfg.anomaly_threshold   = 0.5;
    cfg.outlier_config.method = AnomalyMethod::ISOLATION_FOREST;

    MLAnomalyDetector det(cfg);
    det.train({makeSeries({100, 101, 99, 100, 101, 100})});
    ForecastSeries current = makeSeries({100, 150});
    auto anomalies = det.detectAnomalies(current);
    ASSERT_FALSE(anomalies.empty());
    auto exp = det.explainAnomaly(anomalies.back());
    EXPECT_EQ("memory", exp.metric_name);
    EXPECT_GE(exp.feature_importance.size(), 1u);
    EXPECT_GT(exp.confidence_score, 0.0);
}
