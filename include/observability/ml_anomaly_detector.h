/**
 * @file ml_anomaly_detector.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <optional>
#include <string>
#include <utility>
#include <vector>
#include <nlohmann/json.hpp>

#include "analytics/anomaly_detection.h"
#include "analytics/forecasting.h"

namespace themis {
namespace observability {

using json = nlohmann::json;
using ForecastSeries = themisdb::analytics::TimeSeries;

/**
 * @brief Forecast backend used for expected-value estimation.
 */
enum class ForecastBackend {
    ARIMA,
    PROPHET  ///< Implemented via Holt-Winters seasonal model
};

/**
 * @brief Configuration for MLAnomalyDetector.
 */
struct MLConfig {
    /// Human-friendly metric name used in results.
    std::string metric_name{"metric"};

    /// Forecasting backend.
    ForecastBackend forecast_backend{ForecastBackend::ARIMA};

    /// Underlying forecast options (seasonality, confidence interval, etc.).
    themisdb::analytics::ForecastConfig forecast_config{};

    /// Outlier detector configuration (Isolation Forest / LOF / ensemble).
    themisdb::analytics::DetectorConfig outlier_config = [] {
        themisdb::analytics::DetectorConfig cfg;
        cfg.method = themisdb::analytics::AnomalyMethod::ISOLATION_FOREST;
        cfg.threshold = 0.6;
        return cfg;
    }();

    /// Confidence score threshold in [0, 1] above which a point is treated
    /// as anomalous.
    double anomaly_threshold{0.7};

    /// Minimum training points required before inference.
    size_t min_training_points{30};

    /// DBSCAN epsilon (distance) for 1-D clustering of metric values.
    double dbscan_eps{1.5};

    /// Minimum neighbours for DBSCAN core point detection.
    size_t dbscan_min_samples{4};

    /// Change-point sensitivity (z-score threshold against baseline mean).
    double change_point_threshold{3.0};

    /// Optional seasonality hint (number of points per seasonal period).
    size_t seasonality_period{0};
};

/**
 * @brief ML-detected anomaly with enriched context.
 */
struct Anomaly {
    std::chrono::system_clock::time_point timestamp;
    std::string metric_name;
    double actual_value{0.0};
    double expected_value{0.0};
    double confidence_score{0.0};  ///< 0–1
    std::string severity;          ///< low, medium, high, critical
    std::vector<std::string> contributing_factors;

    json toJson() const;
};

/**
 * @brief Explanation of an anomaly (feature contributions).
 */
struct AnomalyExplanation {
    std::chrono::system_clock::time_point timestamp;
    std::string metric_name;
    double confidence_score{0.0};
    std::vector<std::pair<std::string, double>> feature_importance;
    std::string summary;

    json toJson() const;
};

/**
 * @brief Machine learning-based anomaly detector for observability metrics.
 *
 * Combines time-series forecasting (ARIMA / Prophet-style Holt-Winters),
 * outlier detection (Isolation Forest / LOF), DBSCAN density checks, seasonal
 * pattern recognition, and lightweight change-point detection.
 */
class MLAnomalyDetector {
public:
    explicit MLAnomalyDetector(const MLConfig& config = {});

    /// Train on one or more historical time series (merged by timestamp).
    void train(const std::vector<ForecastSeries>& training_data);

    /// Detect anomalies in the provided series.
    std::vector<Anomaly> detectAnomalies(const ForecastSeries& current_data) const;

    /// Predict future values out to @p horizon.
    ForecastSeries forecast(std::chrono::hours horizon) const;

    /// Explain an already-detected anomaly.
    AnomalyExplanation explainAnomaly(const Anomaly& anomaly) const;

    /// True once train() has succeeded.
    bool isTrained() const noexcept { return trained_; }

private:
    MLConfig cfg_;

    mutable themisdb::analytics::ForecastModel   forecast_model_;
    mutable themisdb::analytics::AnomalyDetector outlier_detector_;

    ForecastSeries training_series_;
    std::vector<double> baseline_values_;
    std::vector<double> seasonal_template_;
    double baseline_mean_{0.0};
    double baseline_stddev_{0.0};
    bool trained_{false};

    // Helpers
    static double clamp01(double v) noexcept;
    static std::chrono::system_clock::time_point tsFromMs(int64_t ms);
    static int64_t                              toMs(std::chrono::system_clock::time_point tp);
    static double                               mean(const std::vector<double>& v);
    static double                               stddev(const std::vector<double>& v, double mu);
    double medianIntervalMs(const ForecastSeries& series) const;
    std::vector<int> dbscanLabels(const std::vector<double>& values) const;
    double changePointScore(const std::vector<double>& values) const;
    std::string severityForScore(double s) const;
    std::vector<double> buildSeasonalTemplate(const themisdb::analytics::DecompositionResult& d) const;
    AnomalyExplanation buildExplanation(const Anomaly& anomaly) const;
};

} // namespace observability
} // namespace themis

