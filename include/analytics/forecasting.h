/**
 * @file forecasting.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB Predictive Analytics & Time-Series Forecasting Engine
 *
 * Provides pure C++17 time-series forecasting with no external ML
 * dependencies.  Designed to integrate with the Analytics module for
 * sales forecasting, demand prediction, capacity planning, and
 * trend analysis.
 *
 * Supported algorithms:
 *   - LINEAR_REGRESSION  – ordinary least-squares trend extrapolation
 *   - EXP_SMOOTHING      – simple exponential smoothing (SES / ETS-ANN)
 *   - HOLT_WINTERS       – triple exponential smoothing with trend and
 *                          additive or multiplicative seasonality
 *   - ARIMA              – autoregressive model with optional differencing
 *                          AR(p) + I(d) + MA(q) (Yule–Walker estimation)
 *   - ENSEMBLE           – weighted combination of the above models
 *
 * Features:
 *   - Multi-step ahead forecasting
 *   - Confidence intervals (empirical / bootstrap)
 *   - Seasonal decomposition (additive / multiplicative)
 *   - Trend analysis
 *   - Forecast accuracy metrics (MAE, RMSE, MAPE)
 *   - Model serialisation / deserialisation
 *
 * Thread-safety:
 *   - ForecastModel: fit() is NOT thread-safe; predict/evaluate are.
 *
 * Copyright (c) 2025 VCC-URN Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <algorithm>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace themisdb {
namespace analytics {

// ============================================================================
// Forward declarations
// ============================================================================

class ForecastModel;

// ============================================================================
// TimeSeriesPoint – a single observation
// ============================================================================

struct TimeSeriesPoint {
    int64_t timestamp_ms = 0;
    double  value        = 0.0;

    bool operator<(const TimeSeriesPoint& o) const noexcept {
        return timestamp_ms < o.timestamp_ms;
    }
};

// ============================================================================
// TimeSeries – ordered collection of observations
// ============================================================================

class TimeSeries {
public:
    TimeSeries() = default;

    /**
     * @brief Time Series.
     * @param[in] points Input parameter.
     * @return Return value.
     */
    explicit TimeSeries(std::vector<TimeSeriesPoint> points);


    /**
     * @brief Push.
     * @param[in] timestamp_ms Input parameter.
     * @param[in] value Input parameter.
     */
    void push(int64_t timestamp_ms, double value);
    /**
     * @brief Push.
     * @param[in] point Input parameter.
     */
    void push(const TimeSeriesPoint& point);

    // ---- Accessors ----

    const std::vector<TimeSeriesPoint>& points() const noexcept { return points_; }
    size_t size() const noexcept { return points_.size(); }
    bool   empty() const noexcept { return points_.empty(); }

    /**
     * @brief Values.
     * @return Return value.
     */
    std::vector<double> values() const;

    /**
     * @brief Timestamps.
     * @return Return value.
     */
    std::vector<int64_t> timestamps() const;

    /**
     * @brief Slice.
     * @param[in] from_ms Input parameter.
     * @param[in] to_ms Input parameter.
     * @return Return value.
     */
    TimeSeries slice(int64_t from_ms, int64_t to_ms) const;

    std::pair<TimeSeries, TimeSeries> trainTestSplit(double train_ratio = 0.8) const;


    /**
     * @brief Mean.
     * @return Return value.
     */
    double mean()   const;
    /**
     * @brief Stddev.
     * @return Return value.
     */
    double stddev() const;
    /**
     * @brief Min.
     * @return Return value.
     */
    double min()    const;
    /**
     * @brief Max.
     * @return Return value.
     */
    double max()    const;

private:
    std::vector<TimeSeriesPoint> points_;
};

// ============================================================================
// ForecastMethod
// ============================================================================

enum class ForecastMethod {
    LINEAR_REGRESSION,   ///< OLS trend line extrapolation
    EXP_SMOOTHING,       ///< Simple exponential smoothing
    HOLT_WINTERS,        ///< Triple exponential smoothing
    ARIMA,               ///< AR(p) + I(d) + MA(q) model
    ENSEMBLE,            ///< Weighted combination of the above
    SARIMA,              ///< Seasonal ARIMA (p,d,q)(P,D,Q)_m
    PROPHET              ///< Prophet-style trend + Fourier seasonality + changepoints
};

// ============================================================================
// ForecastConfig – training / prediction options
// ============================================================================

struct ForecastConfig {
    // ---- Exponential Smoothing / Holt-Winters ----
    double alpha            = 0.3;   ///< level smoothing factor (0 < α < 1)
    double beta             = 0.1;   ///< trend smoothing factor (0 < β < 1)
    double gamma            = 0.1;   ///< seasonal smoothing factor (0 < γ < 1)
    int    seasonality      = 0;     ///< seasonal period (0 = no seasonality)
    bool   multiplicative   = false; ///< true = multiplicative, false = additive

    // ---- ARIMA ----
    int    ar_order         = 2;     ///< autoregressive order p
    int    diff_order       = 1;     ///< differencing order d (0 or 1)
    int    ma_order         = 1;     ///< moving-average order q

    // ---- SARIMA – seasonal ARIMA (p,d,q)(P,D,Q)_m ----
    int    sarima_P         = 1;     ///< seasonal AR order
    int    sarima_D         = 1;     ///< seasonal differencing order (0 or 1)
    int    sarima_Q         = 1;     ///< seasonal MA order
    int    sarima_m         = 0;     ///< seasonal period (0 = autodetect / disabled)

    // ---- Prophet-style trend + seasonality ----
    double prophet_changepoint_prior_scale = 0.05;
    int    prophet_fourier_order_weekly    = 3;
    int    prophet_fourier_order_yearly    = 10;
    double prophet_changepoint_range       = 0.8;

    // ---- Confidence intervals ----
    bool   include_confidence = true;
    double confidence_level  = 0.95; ///< e.g., 0.95 → 95% CI

    // ---- Ensemble ----
    std::vector<double> ensemble_weights;

    // ---- Optimisation ----
    bool   auto_tune        = false; ///< grid-search alpha/beta/gamma
};

// ============================================================================
// ForecastPoint – one step of the forecast
// ============================================================================

struct ForecastPoint {
    int64_t timestamp_ms = 0; ///< projected timestamp
    double  value        = 0.0;
    double  lower        = 0.0; ///< confidence interval lower bound
    double  upper        = 0.0; ///< confidence interval upper bound
};

// ============================================================================
// ForecastMetrics – accuracy evaluation
// ============================================================================

struct ForecastMetrics {
    double mae  = 0.0; ///< Mean Absolute Error
    double rmse = 0.0; ///< Root Mean Squared Error
    double mape = 0.0; ///< Mean Absolute Percentage Error (%)
    double smape = 0.0; ///< Symmetric MAPE (%)
    size_t n    = 0;   ///< number of evaluation points
};

// ============================================================================
// DecompositionResult – seasonal decomposition
// ============================================================================

struct DecompositionResult {
    std::vector<double> trend;
    std::vector<double> seasonal;
    std::vector<double> residual;
    bool multiplicative = false;
};

// ============================================================================
// ForecastModel – main class
// ============================================================================

class ForecastModel {
public:
    // ---- Construction ----
    explicit ForecastModel(ForecastMethod method = ForecastMethod::LINEAR_REGRESSION);
    explicit ForecastModel(const ForecastConfig& config,
                           ForecastMethod method = ForecastMethod::LINEAR_REGRESSION);
    ~ForecastModel();

    // Non-copyable; movable
    ForecastModel(const ForecastModel&)            = delete;
    ForecastModel& operator=(const ForecastModel&) = delete;
    ForecastModel(ForecastModel&&)                 noexcept;
    ForecastModel& operator=(ForecastModel&&)      noexcept;


    /**
     * @brief Fit.
     * @param[in] ts Input parameter.
     */
    void fit(const TimeSeries& ts);
    /**
     * @brief Fit.
     * @param[in] ts Input parameter.
     * @param[in] config Input parameter.
     */
    void fit(const TimeSeries& ts, const ForecastConfig& config);

    /**
     * @brief Is Fitted.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    bool isFitted() const noexcept;


    /**
     * @brief Predict.
     * @param[in] steps Input parameter.
     * @return Return value.
     */
    std::vector<ForecastPoint> predict(int steps) const;

    /**
     * @brief Predict Batch.
     * @param[in] batch Input parameter.
     * @param[in] steps Input parameter.
     * @return Return value.
     */
    std::vector<std::vector<ForecastPoint>> predictBatch(
        const std::vector<TimeSeries>& batch, int steps) const;

    /**
     * @brief Update.
     * @param[in] new_value Input parameter.
     */
    void update(double new_value);


    /**
     * @brief Evaluate.
     * @param[in] test_ts Input parameter.
     * @return Return value.
     */
    ForecastMetrics evaluate(const TimeSeries& test_ts) const;

    // ---- Seasonal decomposition ----

    DecompositionResult decompose(bool multiplicative = false) const;


    /**
     * @brief Serialize.
     * @return Return value.
     */
    std::string serialize() const;
    /**
     * @brief Deserialize.
     * @param[in] data Input parameter.
     * @return Return value.
     */
    static ForecastModel deserialize(const std::string& data);

    // ---- Diagnostics ----

    struct ModelInfo {
        ForecastMethod method;
        size_t         training_points  = 0;
        int64_t        train_start_ms   = 0;
        int64_t        train_end_ms     = 0;
        int64_t        median_interval_ms = 0;
        double         in_sample_rmse   = 0.0;
        bool           fitted           = false;
    };

    /**
     * @brief Info.
     * @return Return value.
     */
    ModelInfo        info()   const;
    /**
     * @brief Method.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    ForecastMethod   method() const noexcept;
    /**
     * @brief Config.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    const ForecastConfig& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;

    friend std::pair<bool, std::string> exponentialSmoothing(
        ForecastModel& model,
        const std::vector<double>& timeseries,
        double alpha,
        double beta,
        double gamma);
};

// ============================================================================
// Free helpers
// ============================================================================

inline const char* forecastMethodName(ForecastMethod m) noexcept {
    switch (m) {
        case ForecastMethod::LINEAR_REGRESSION: return "LINEAR_REGRESSION";
        case ForecastMethod::EXP_SMOOTHING:     return "EXP_SMOOTHING";
        case ForecastMethod::HOLT_WINTERS:      return "HOLT_WINTERS";
        case ForecastMethod::ARIMA:             return "ARIMA";
        case ForecastMethod::ENSEMBLE:          return "ENSEMBLE";
        case ForecastMethod::SARIMA:            return "SARIMA";
        case ForecastMethod::PROPHET:           return "PROPHET";
        default:                                return "UNKNOWN";
    }
}

/**
 * @brief Compute Metrics.
 * @param[in] actual Input parameter.
 * @param[in] predicted Input parameter.
 * @return Return value.
 */
ForecastMetrics computeMetrics(const std::vector<double>& actual,
                               const std::vector<double>& predicted);

// ============================================================================
// Helper functions (Phase 2B)
// ============================================================================

int seasonalityDuration(
    const std::vector<double>& timeseries,
    int max_lag = 1000);

std::pair<bool, std::string> validateTestData(
    const std::vector<std::vector<double>>& test_features,
    size_t expected_n_features);

std::pair<bool, std::string> exponentialSmoothing(
    ForecastModel& model,
    const std::vector<double>& timeseries,
    double alpha,
    double beta = 0.0,
    double gamma = 0.0);

} // namespace analytics
} // namespace themisdb
