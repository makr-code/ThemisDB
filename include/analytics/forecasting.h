/**
 * @file forecasting.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * One observation in a time series.
 *
 * @c timestamp_ms Wall-clock timestamp in milliseconds since epoch.
 * @c value        Observed value (numeric).
 */
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

/**
 * Ordered time-series container.
 *
 * Points are stored sorted by @c timestamp_ms.  Duplicate timestamps are
 * allowed and retained as separate observations.
 */
class TimeSeries {
public:
    TimeSeries() = default;

    /// Construct from a pre-built vector (will be sorted).
    explicit TimeSeries(std::vector<TimeSeriesPoint> points);

    // ---- Mutation ----

    /// Append a point (keeps the series sorted).
    void push(int64_t timestamp_ms, double value);
    void push(const TimeSeriesPoint& point);

    // ---- Accessors ----

    const std::vector<TimeSeriesPoint>& points() const noexcept { return points_; }
    size_t size() const noexcept { return points_.size(); }
    bool   empty() const noexcept { return points_.empty(); }

    /// Return only the numeric values in time order.
    std::vector<double> values() const;

    /// Return only the timestamps in time order.
    std::vector<int64_t> timestamps() const;

    /// Slice by timestamp range [from_ms, to_ms).
    TimeSeries slice(int64_t from_ms, int64_t to_ms) const;

    /// Split into train / test at a given fraction (0 < ratio < 1).
    std::pair<TimeSeries, TimeSeries> trainTestSplit(double train_ratio = 0.8) const;

    // ---- Statistics ----

    double mean()   const;
    double stddev() const;
    double min()    const;
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

/**
 * Configuration for model training and forecasting.
 */
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
    /// Scale for the piecewise linear changepoint prior (larger → more flexible).
    double prophet_changepoint_prior_scale = 0.05;
    /// Number of Fourier terms for weekly seasonality.
    int    prophet_fourier_order_weekly    = 3;
    /// Number of Fourier terms for yearly seasonality.
    int    prophet_fourier_order_yearly    = 10;
    /// Proportion of history to use for potential changepoints.
    double prophet_changepoint_range       = 0.8;

    // ---- Confidence intervals ----
    bool   include_confidence = true;
    double confidence_level  = 0.95; ///< e.g., 0.95 → 95% CI

    // ---- Ensemble ----
    /// Weights for [LINEAR_REGRESSION, EXP_SMOOTHING, HOLT_WINTERS, ARIMA].
    /// If empty, equal weights are used.
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

/**
 * Time-series forecasting model.
 *
 * Usage:
 * @code
 *   #include "analytics/forecasting.h"
 *
 *   TimeSeries ts;
 *   for (auto& p : my_data) ts.push(p.ts_ms, p.revenue);
 *
 *   ForecastModel model(ForecastMethod::HOLT_WINTERS);
 *   ForecastConfig cfg;
 *   cfg.seasonality = 12;   // monthly data → yearly season
 *   model.fit(ts, cfg);
 *
 *   auto forecast = model.predict(30);
 *   for (const auto& fp : forecast)
 *       std::cout << fp.timestamp_ms << ": " << fp.value << "\n";
 *
 *   auto metrics = model.evaluate(test_ts);
 *   std::cout << "RMSE: " << metrics.rmse << "\n";
 * @endcode
 */
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

    // ---- Training ----

    /**
     * Fit the model to the given time series.
     *
     * @param ts     Training data (must have at least 2 points).
     * @throws std::invalid_argument if ts has fewer than 2 points.
     */
    void fit(const TimeSeries& ts);
    /**
     * Fit the model to the given time series.
     *
     * @param ts     Training data (must have at least 2 points).
     * @param config Optional overrides; if not supplied the model's own
     *               config (set at construction) is used.
     * @throws std::invalid_argument if ts has fewer than 2 points.
     */
    void fit(const TimeSeries& ts, const ForecastConfig& config);

    bool isFitted() const noexcept;

    // ---- Prediction ----

    /**
     * Forecast @p steps steps ahead.
     *
     * The timestamps of the returned points are evenly spaced using the
     * median inter-observation interval of the training series.
     *
     * @param steps  Number of future points to forecast.
     * @returns      Vector of ForecastPoint (size == steps).
     * @throws std::runtime_error if the model has not been fitted.
     */
    std::vector<ForecastPoint> predict(int steps) const;

    /**
     * Batch-predict @p steps steps ahead for each series in @p batch.
     *
     * The model is fitted to each series in @p batch independently and
     * predictions are returned in the same order.  The model's own fitted
     * state is unchanged after this call.
     *
     * Each element of the returned outer vector corresponds to one input
     * series; the inner vector has exactly @p steps ForecastPoint entries.
     *
     * This avoids the per-call model-state copy overhead of calling
     * predict() on N independently constructed models.
     *
     * @param batch  One or more time series to forecast.
     * @param steps  Number of future points per series (must be ≥ 1).
     * @returns      Vector of size batch.size(), each element of size steps.
     * @throws std::invalid_argument if @p steps < 1 or any series has < 2 points.
     */
    std::vector<std::vector<ForecastPoint>> predictBatch(
        const std::vector<TimeSeries>& batch, int steps) const;

    /**
     * Incrementally absorb one new observation into the fitted model state.
     *
     * Updates only the ETS level/trend/seasonal components (O(1)); does not
     * re-run full fit().  For ARIMA the last AR window is shifted and the new
     * point appended; for LINEAR_REGRESSION the new point is appended to
     * update the OLS parameters.
     *
     * Calling update() on a model that has not been fitted is a no-op.
     *
     * @param new_value  The new observation value (timestamp is implicitly
     *                   one median-interval step after the last training point).
     */
    void update(double new_value);

    // ---- Evaluation ----

    /**
     * Evaluate the model against a held-out test set.
     *
     * The model predicts len(test_ts) steps ahead starting at the last
     * training observation and computes MAE, RMSE, and MAPE.
     */
    ForecastMetrics evaluate(const TimeSeries& test_ts) const;

    // ---- Seasonal decomposition ----

    /**
     * Decompose the training series into trend, seasonal, and residual
     * components.  Requires isFitted() == true.
     *
     * @param multiplicative  If true, use multiplicative model; otherwise
     *                        additive.
     */
    DecompositionResult decompose(bool multiplicative = false) const;

    // ---- Serialisation ----

    /// Serialise the fitted model state to a string (JSON-like text).
    std::string serialize() const;
    /// Restore a model from a previously serialised string.
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

    ModelInfo        info()   const;
    ForecastMethod   method() const noexcept;
    const ForecastConfig& config() const noexcept;

private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

// ============================================================================
// Free helpers
// ============================================================================

/// Convert a ForecastMethod to a human-readable string.
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

/// Compute forecast accuracy metrics from parallel actual / predicted vectors.
ForecastMetrics computeMetrics(const std::vector<double>& actual,
                               const std::vector<double>& predicted);

// ============================================================================
// Helper functions (Phase 2B)
// ============================================================================

/**
 * @brief Detect seasonal period using autocorrelation or FFT.
 * 
 * Analyzes the time series to determine if there is a periodic pattern
 * and returns the detected seasonal period (in number of steps).
 * 
 * Algorithm:
 * - Computes autocorrelation at various lags
 * - Returns lag with highest autocorrelation > 0.5
 * - Returns 0 if no strong seasonality detected
 * 
 * @param timeseries Vector of time-series values
 * @param max_lag Maximum lag to check (default: 1000)
 * @return Detected seasonal period in steps; 0 if no seasonality
 * @throws std::invalid_argument if timeseries.size() < 2
 * 
 * @code
 *   std::vector<double> ts = { 1.0, 2.0, 3.0, 1.0, 2.0, 3.0, 1.0, 2.0, 3.0 };
 *   int period = seasonalityDuration(ts);  // Returns 3
 * @endcode
 */
int seasonalityDuration(
    const std::vector<double>& timeseries,
    int max_lag = 1000);

/**
 * @brief Validate test data structure and quality.
 * 
 * Checks:
 * - Non-empty
 * - Consistent dimensions with expected input
 * - No NaN or Inf values
 * - Timestamps in chronological order (if provided)
 * 
 * @param test_features Test feature matrix (n_samples × n_features)
 * @param expected_n_features Expected number of features per sample
 * @return Pair of (is_valid, error_message)
 * 
 * @code
 *   std::vector<std::vector<double>> X_test = {{ 1.0, 2.0 }, { 3.0, 4.0 }};
 *   auto [valid, msg] = validateTestData(X_test, 2);
 *   if (!valid) {
 *       std::cerr << "Validation error: " << msg << std::endl;
 *   }
 * @endcode
 */
std::pair<bool, std::string> validateTestData(
    const std::vector<std::vector<double>>& test_features,
    size_t expected_n_features);

/**
 * @brief Apply exponential smoothing (Holt-Winters) to a time series.
 * 
 * Updates the provided ForecastModel with smoothed coefficients and fitted state.
 * Supports:
 * - Simple exponential smoothing (alpha only)
 * - Double exponential / Holt's method (alpha, beta)
 * - Triple exponential / Holt-Winters (alpha, beta, gamma for seasonality)
 * 
 * @param model Target ForecastModel to update with fitted coefficients
 * @param timeseries Input time-series data
 * @param alpha Level smoothing coefficient ∈ (0,1)
 * @param beta Trend smoothing coefficient ∈ (0,1); set to 0 for simple ES
 * @param gamma Seasonal smoothing coefficient ∈ (0,1); set to 0 for non-seasonal
 * @return Status::OK() on success; Status::Error(msg) on validation failure
 * @throws std::invalid_argument if timeseries.size() < 2 or params out of range
 * 
 * @code
 *   ForecastModel model;
 *   std::vector<double> ts = { 100, 110, 120, 130 };
 *   auto status = exponentialSmoothing(model, ts, 0.3, 0.1, 0.0);
 *   if (status.first) {
 *       auto forecast = model.predict(2);  // Forecast 2 steps ahead
 *   }
 * @endcode
 */
std::pair<bool, std::string> exponentialSmoothing(
    ForecastModel& model,
    const std::vector<double>& timeseries,
    double alpha,
    double beta = 0.0,
    double gamma = 0.0);

} // namespace analytics
} // namespace themisdb
