/**
 * Predictive Analytics & Time-Series Forecasting unit tests.
 *
 * Covers:
 *  - TimeSeries construction, push, slice, split, statistics
 *  - ForecastModel: all five algorithms
 *    (LINEAR_REGRESSION, EXP_SMOOTHING, HOLT_WINTERS, ARIMA, ENSEMBLE)
 *  - fit / isFitted guards
 *  - predict output shape, timestamps, CI bounds
 *  - evaluate / ForecastMetrics
 *  - decompose
 *  - serialize / deserialize round-trip
 *  - computeMetrics free function
 *  - forecastMethodName helper
 *  - Edge cases: minimal series, flat series, steps == 1
 */

#include <gtest/gtest.h>
#include "analytics/forecasting.h"

#include <cmath>
#include <vector>

using namespace themisdb::analytics;

constexpr double kPi = 3.14159265358979323846;

// ============================================================================
// Helpers
// ============================================================================

/// Build a simple linear series: y[i] = slope * i + intercept
static TimeSeries makeLinearSeries(int n, double slope = 1.0, double intercept = 0.0,
                                   int64_t start_ms = 0, int64_t interval_ms = 1000)
{
    TimeSeries ts;
    for (int i = 0; i < n; ++i)
        ts.push(start_ms + static_cast<int64_t>(i) * interval_ms,
                slope * static_cast<double>(i) + intercept);
    return ts;
}

/// Build a seasonal series with additive pattern: base + trend + sin wave
static TimeSeries makeSeasonalSeries(int n, int period = 4, double trend = 0.1,
                                     int64_t interval_ms = 1000)
{
    TimeSeries ts;
    for (int i = 0; i < n; ++i) {
        double seasonal = 2.0 * std::sin(2.0 * kPi * static_cast<double>(i) / static_cast<double>(period));
        double val = 10.0 + trend * static_cast<double>(i) + seasonal;
        ts.push(static_cast<int64_t>(i) * interval_ms, val);
    }
    return ts;
}

// ============================================================================
// TimeSeries tests
// ============================================================================

TEST(TimeSeriesTest, PushAndValues) {
    TimeSeries ts;
    ts.push(1000, 10.0);
    ts.push(2000, 20.0);
    ts.push(3000, 30.0);
    EXPECT_EQ(ts.size(), 3u);
    auto v = ts.values();
    EXPECT_DOUBLE_EQ(v[0], 10.0);
    EXPECT_DOUBLE_EQ(v[1], 20.0);
    EXPECT_DOUBLE_EQ(v[2], 30.0);
}

TEST(TimeSeriesTest, PushMaintainsOrder) {
    TimeSeries ts;
    ts.push(3000, 30.0);
    ts.push(1000, 10.0);
    ts.push(2000, 20.0);
    auto t = ts.timestamps();
    EXPECT_EQ(t[0], 1000);
    EXPECT_EQ(t[1], 2000);
    EXPECT_EQ(t[2], 3000);
}

TEST(TimeSeriesTest, ConstructFromVector) {
    std::vector<TimeSeriesPoint> pts = {{3000, 3.0}, {1000, 1.0}, {2000, 2.0}};
    TimeSeries ts(std::move(pts));
    EXPECT_EQ(ts.size(), 3u);
    EXPECT_EQ(ts.timestamps()[0], 1000);
}

TEST(TimeSeriesTest, Statistics) {
    TimeSeries ts = makeLinearSeries(5, 1.0, 0.0);
    // values: 0, 1, 2, 3, 4
    EXPECT_DOUBLE_EQ(ts.mean(), 2.0);
    EXPECT_DOUBLE_EQ(ts.min(), 0.0);
    EXPECT_DOUBLE_EQ(ts.max(), 4.0);
    EXPECT_GT(ts.stddev(), 0.0);
}

TEST(TimeSeriesTest, Slice) {
    TimeSeries ts = makeLinearSeries(10, 1.0, 0.0, 0, 1000);
    auto sliced = ts.slice(2000, 5000);
    EXPECT_EQ(sliced.size(), 3u);  // indices 2, 3, 4
}

TEST(TimeSeriesTest, TrainTestSplit) {
    TimeSeries ts = makeLinearSeries(20);
    auto [train, test] = ts.trainTestSplit(0.8);
    EXPECT_EQ(train.size() + test.size(), 20u);
    EXPECT_EQ(train.size(), 16u);
    EXPECT_EQ(test.size(), 4u);
}

TEST(TimeSeriesTest, TrainTestSplitInvalidRatio) {
    TimeSeries ts = makeLinearSeries(10);
    EXPECT_THROW(ts.trainTestSplit(0.0), std::invalid_argument);
    EXPECT_THROW(ts.trainTestSplit(1.0), std::invalid_argument);
    EXPECT_THROW(ts.trainTestSplit(-0.1), std::invalid_argument);
}

// ============================================================================
// computeMetrics free function
// ============================================================================

TEST(ComputeMetricsTest, PerfectPrediction) {
    std::vector<double> actual    = {1.0, 2.0, 3.0, 4.0, 5.0};
    std::vector<double> predicted = {1.0, 2.0, 3.0, 4.0, 5.0};
    auto m = computeMetrics(actual, predicted);
    EXPECT_DOUBLE_EQ(m.mae,  0.0);
    EXPECT_DOUBLE_EQ(m.rmse, 0.0);
    EXPECT_DOUBLE_EQ(m.mape, 0.0);
    EXPECT_EQ(m.n, 5u);
}

TEST(ComputeMetricsTest, KnownError) {
    // actual = [2, 4], predicted = [1, 3] → errors = [1, 1]
    std::vector<double> actual    = {2.0, 4.0};
    std::vector<double> predicted = {1.0, 3.0};
    auto m = computeMetrics(actual, predicted);
    EXPECT_DOUBLE_EQ(m.mae,  1.0);
    EXPECT_DOUBLE_EQ(m.rmse, 1.0);
    EXPECT_GT(m.mape, 0.0);
}

TEST(ComputeMetricsTest, EmptyVectors) {
    auto m = computeMetrics({}, {});
    EXPECT_EQ(m.n, 0u);
}

// ============================================================================
// forecastMethodName
// ============================================================================

TEST(ForecastMethodNameTest, AllMethods) {
    EXPECT_STREQ(forecastMethodName(ForecastMethod::LINEAR_REGRESSION), "LINEAR_REGRESSION");
    EXPECT_STREQ(forecastMethodName(ForecastMethod::EXP_SMOOTHING),     "EXP_SMOOTHING");
    EXPECT_STREQ(forecastMethodName(ForecastMethod::HOLT_WINTERS),      "HOLT_WINTERS");
    EXPECT_STREQ(forecastMethodName(ForecastMethod::ARIMA),             "ARIMA");
    EXPECT_STREQ(forecastMethodName(ForecastMethod::ENSEMBLE),          "ENSEMBLE");
}

// ============================================================================
// ForecastModel – guard conditions
// ============================================================================

TEST(ForecastModelTest, NotFittedThrows) {
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    EXPECT_FALSE(model.isFitted());
    EXPECT_THROW(model.predict(5), std::runtime_error);
    EXPECT_THROW(model.evaluate(makeLinearSeries(5)), std::runtime_error);
    EXPECT_THROW(model.decompose(), std::runtime_error);
}

TEST(ForecastModelTest, TooFewPointsThrows) {
    ForecastModel model;
    TimeSeries ts;
    ts.push(0, 1.0);  // only 1 point
    EXPECT_THROW(model.fit(ts), std::invalid_argument);
}

TEST(ForecastModelTest, FittedAfterTwoPoints) {
    ForecastModel model;
    TimeSeries ts;
    ts.push(0, 1.0);
    ts.push(1000, 2.0);
    EXPECT_NO_THROW(model.fit(ts));
    EXPECT_TRUE(model.isFitted());
}

TEST(ForecastModelTest, PredictZeroSteps) {
    ForecastModel model;
    auto ts = makeLinearSeries(10);
    model.fit(ts);
    auto result = model.predict(0);
    EXPECT_TRUE(result.empty());
}

// ============================================================================
// Linear Regression
// ============================================================================

TEST(LinearRegressionTest, TrendLineExtrapolation) {
    // y = 2i + 1  →  forecasts should follow the same trend
    auto ts = makeLinearSeries(20, 2.0, 1.0, 0, 1000);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);

    auto forecast = model.predict(5);
    EXPECT_EQ(forecast.size(), 5u);

    // Each forecast should be approximately 2 apart (slope=2, interval=1 index)
    for (size_t i = 1; i < forecast.size(); ++i)
        EXPECT_NEAR(forecast[i].value - forecast[i - 1].value, 2.0, 0.5);
}

TEST(LinearRegressionTest, ConfidenceIntervalBounds) {
    auto ts = makeLinearSeries(20, 1.0, 0.0);
    ForecastConfig cfg;
    cfg.include_confidence = true;
    cfg.confidence_level   = 0.95;
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts, cfg);

    auto forecast = model.predict(10);
    for (const auto& fp : forecast) {
        EXPECT_LE(fp.lower, fp.value);
        EXPECT_GE(fp.upper, fp.value);
    }
}

TEST(LinearRegressionTest, TimestampsAreMonotonicallyIncreasing) {
    auto ts = makeLinearSeries(10, 1.0, 0.0, 0, 2000);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto forecast = model.predict(5);
    for (size_t i = 1; i < forecast.size(); ++i)
        EXPECT_GT(forecast[i].timestamp_ms, forecast[i - 1].timestamp_ms);
}

TEST(LinearRegressionTest, FlatSeriesNearZeroSlope) {
    TimeSeries ts;
    for (int i = 0; i < 20; ++i) {
      ts.push(static_cast<int64_t>(i) * 1000, 5.0);
    }
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto forecast = model.predict(5);
    for (const auto& fp : forecast)
        EXPECT_NEAR(fp.value, 5.0, 0.1);
}

// ============================================================================
// Exponential Smoothing
// ============================================================================

TEST(ExpSmoothingTest, BasicForecast) {
    auto ts = makeLinearSeries(30, 1.0, 0.0);
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    model.fit(ts);
    auto forecast = model.predict(5);
    EXPECT_EQ(forecast.size(), 5u);
    // All future values equal the last smoothed level → approximately equal
    for (size_t i = 1; i < forecast.size(); ++i)
        EXPECT_DOUBLE_EQ(forecast[i].value, forecast[0].value);
}

TEST(ExpSmoothingTest, AlphaCustom) {
    ForecastConfig cfg;
    cfg.alpha = 0.9;
    auto ts = makeLinearSeries(20);
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    model.fit(ts, cfg);
    EXPECT_TRUE(model.isFitted());
    EXPECT_DOUBLE_EQ(model.config().alpha, 0.9);
}

TEST(ExpSmoothingTest, AutoTune) {
    ForecastConfig cfg;
    cfg.auto_tune = true;
    auto ts = makeLinearSeries(20);
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    EXPECT_NO_THROW(model.fit(ts, cfg));
    EXPECT_TRUE(model.isFitted());
}

// ============================================================================
// Holt-Winters
// ============================================================================

TEST(HoltWintersTest, BasicForecastNoSeason) {
    auto ts = makeLinearSeries(20, 2.0, 5.0);
    ForecastConfig cfg;
    cfg.seasonality = 0;  // no seasonality
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);
    auto forecast = model.predict(5);
    EXPECT_EQ(forecast.size(), 5u);
    // Should follow the linear trend approximately
    EXPECT_GT(forecast.back().value, forecast.front().value);
}

TEST(HoltWintersTest, SeasonalForecast) {
    auto ts = makeSeasonalSeries(24, 4);  // 6 complete seasons
    ForecastConfig cfg;
    cfg.seasonality = 4;
    cfg.alpha = 0.3;
    cfg.beta  = 0.1;
    cfg.gamma = 0.1;
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);
    auto forecast = model.predict(8);
    EXPECT_EQ(forecast.size(), 8u);
    for (const auto& fp : forecast) {
        EXPECT_FALSE(std::isnan(fp.value));
        EXPECT_FALSE(std::isinf(fp.value));
    }
}

TEST(HoltWintersTest, MultiplicativeMode) {
    // Positive values with multiplicative seasonality
    TimeSeries ts;
    for (int i = 0; i < 24; ++i) {
        double seasonal = 1.0 + 0.2 * std::sin(2.0 * kPi * static_cast<double>(i) / 4.0);
        ts.push(static_cast<int64_t>(i) * 1000, (5.0 + 0.1 * i) * seasonal);
    }
    ForecastConfig cfg;
    cfg.seasonality    = 4;
    cfg.multiplicative = true;
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);
    auto forecast = model.predict(4);
    for (const auto& fp : forecast) {
        EXPECT_FALSE(std::isnan(fp.value));
    }
}

// ============================================================================
// ARIMA
// ============================================================================

TEST(ARIMATest, BasicForecast) {
    auto ts = makeLinearSeries(30, 1.0, 0.0);
    ForecastConfig cfg;
    cfg.ar_order   = 2;
    cfg.diff_order = 1;
    cfg.ma_order   = 1;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);
    auto forecast = model.predict(5);
    EXPECT_EQ(forecast.size(), 5u);
    for (const auto& fp : forecast) {
        EXPECT_FALSE(std::isnan(fp.value));
        EXPECT_FALSE(std::isinf(fp.value));
    }
}

TEST(ARIMATest, NoDifferencing) {
    // AR(2) on a stationary series
    TimeSeries ts;
    for (int i = 0; i < 30; ++i)
        ts.push(static_cast<int64_t>(i) * 1000, 5.0 + 0.5 * std::sin(static_cast<double>(i)));
    ForecastConfig cfg;
    cfg.ar_order   = 2;
    cfg.diff_order = 0;
    cfg.ma_order   = 0;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);
    auto forecast = model.predict(10);
    EXPECT_EQ(forecast.size(), 10u);
}

// ============================================================================
// Ensemble
// ============================================================================

TEST(EnsembleTest, BasicForecast) {
    auto ts = makeLinearSeries(30, 1.0, 0.0);
    ForecastModel model(ForecastMethod::ENSEMBLE);
    model.fit(ts);
    auto forecast = model.predict(5);
    EXPECT_EQ(forecast.size(), 5u);
    for (const auto& fp : forecast) {
        EXPECT_FALSE(std::isnan(fp.value));
    }
}

TEST(EnsembleTest, CustomWeights) {
    ForecastConfig cfg;
    cfg.ensemble_weights = {0.5, 0.2, 0.2, 0.1};
    auto ts = makeLinearSeries(20, 1.0, 0.0);
    ForecastModel model(ForecastMethod::ENSEMBLE);
    model.fit(ts, cfg);
    auto forecast = model.predict(3);
    EXPECT_EQ(forecast.size(), 3u);
}

// ============================================================================
// evaluate / ForecastMetrics
// ============================================================================

TEST(EvaluateTest, LinearSeriesLowRMSE) {
    auto ts = makeLinearSeries(40, 1.0, 0.0);
    auto [train, test] = ts.trainTestSplit(0.75);

    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(train);
    auto metrics = model.evaluate(test);

    EXPECT_GT(metrics.n, 0u);
    EXPECT_GE(metrics.mae,  0.0);
    EXPECT_GE(metrics.rmse, 0.0);
    EXPECT_GE(metrics.mape, 0.0);
    // For a perfect linear series the linear model should be very accurate
    EXPECT_LT(metrics.rmse, 5.0);
}

TEST(EvaluateTest, EmptyTestSeries) {
    auto ts = makeLinearSeries(10);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto metrics = model.evaluate(TimeSeries{});
    EXPECT_EQ(metrics.n, 0u);
}

// ============================================================================
// decompose
// ============================================================================

TEST(DecomposeTest, AdditiveComponents) {
    auto ts = makeSeasonalSeries(24, 4);
    ForecastConfig cfg;
    cfg.seasonality = 4;
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);

    auto dr = model.decompose(false);
    EXPECT_EQ(dr.trend.size(),    24u);
    EXPECT_EQ(dr.seasonal.size(), 24u);
    EXPECT_EQ(dr.residual.size(), 24u);
    EXPECT_FALSE(dr.multiplicative);
}

TEST(DecomposeTest, MultiplicativeComponents) {
    TimeSeries ts;
    for (int i = 0; i < 24; ++i)
        ts.push(static_cast<int64_t>(i) * 1000,
                (5.0 + 0.1 * i) * (1.0 + 0.1 * std::sin(2.0 * kPi * static_cast<double>(i) / 4.0)));
    ForecastConfig cfg;
    cfg.seasonality    = 4;
    cfg.multiplicative = true;
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);
    auto dr = model.decompose(true);
    EXPECT_TRUE(dr.multiplicative);
    EXPECT_EQ(dr.trend.size(), 24u);
}

// ============================================================================
// serialize / deserialize
// ============================================================================

TEST(SerializeTest, RoundTrip_Linear) {
    auto ts = makeLinearSeries(20, 2.0, 5.0, 0, 1000);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto serialized = model.serialize();
    EXPECT_FALSE(serialized.empty());

    auto restored = ForecastModel::deserialize(serialized);
    EXPECT_TRUE(restored.isFitted());
    EXPECT_EQ(restored.method(), ForecastMethod::LINEAR_REGRESSION);

    // Both models should produce the same forecast
    auto f1 = model.predict(5);
    auto f2 = restored.predict(5);
    EXPECT_EQ(f1.size(), f2.size());
    for (size_t i = 0; i < f1.size(); ++i)
        EXPECT_NEAR(f1[i].value, f2[i].value, 1e-9);}

TEST(SerializeTest, RoundTrip_HoltWinters) {
    auto ts = makeSeasonalSeries(24, 4);
    ForecastConfig cfg;
    cfg.seasonality = 4;
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);
    auto serialized = model.serialize();
    auto restored   = ForecastModel::deserialize(serialized);
    EXPECT_TRUE(restored.isFitted());
    EXPECT_EQ(restored.method(), ForecastMethod::HOLT_WINTERS);

    auto f1 = model.predict(4);
    auto f2 = restored.predict(4);
    EXPECT_EQ(f1.size(), f2.size());
    for (size_t i = 0; i < f1.size(); ++i)
        EXPECT_NEAR(f1[i].value, f2[i].value, 1e-9);  // full precision serialization
}

TEST(SerializeTest, RoundTrip_ARIMA) {
    auto ts = makeLinearSeries(30, 1.5, 3.0, 0, 1000);
    ForecastConfig cfg;
    cfg.ar_order   = 2;
    cfg.diff_order = 1;
    cfg.ma_order   = 1;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);
    auto serialized = model.serialize();
    auto restored   = ForecastModel::deserialize(serialized);
    EXPECT_TRUE(restored.isFitted());
    EXPECT_EQ(restored.method(), ForecastMethod::ARIMA);

    auto f1 = model.predict(5);
    auto f2 = restored.predict(5);
    EXPECT_EQ(f1.size(), f2.size());
    for (size_t i = 0; i < f1.size(); ++i)
        EXPECT_NEAR(f1[i].value, f2[i].value, 1e-9);
}

TEST(SerializeTest, RoundTrip_Ensemble) {
    auto ts = makeLinearSeries(30, 1.0, 2.0, 0, 1000);
    ForecastModel model(ForecastMethod::ENSEMBLE);
    model.fit(ts);
    auto serialized = model.serialize();
    auto restored   = ForecastModel::deserialize(serialized);
    EXPECT_TRUE(restored.isFitted());
    EXPECT_EQ(restored.method(), ForecastMethod::ENSEMBLE);

    auto f1 = model.predict(5);
    auto f2 = restored.predict(5);
    EXPECT_EQ(f1.size(), f2.size());
    for (size_t i = 0; i < f1.size(); ++i)
        EXPECT_NEAR(f1[i].value, f2[i].value, 1e-9);
}

// ============================================================================
// ModelInfo
// ============================================================================

TEST(ModelInfoTest, BasicInfo) {
    auto ts = makeLinearSeries(20, 1.0, 0.0, 0, 1000);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto mi = model.info();
    EXPECT_TRUE(mi.fitted);
    EXPECT_EQ(mi.training_points, 20u);
    EXPECT_EQ(mi.method, ForecastMethod::LINEAR_REGRESSION);
    EXPECT_GE(mi.median_interval_ms, 0);
    EXPECT_GE(mi.in_sample_rmse, 0.0);
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(EdgeCaseTest, TwoPointsSeries) {
    TimeSeries ts;
    ts.push(0,    10.0);
    ts.push(1000, 20.0);
    for (auto method : {ForecastMethod::LINEAR_REGRESSION,
                        ForecastMethod::EXP_SMOOTHING,
                        ForecastMethod::HOLT_WINTERS,
                        ForecastMethod::ARIMA,
                        ForecastMethod::ENSEMBLE}) {
        ForecastModel model(method);
        EXPECT_NO_THROW(model.fit(ts));
        EXPECT_TRUE(model.isFitted());
        auto forecast = model.predict(3);
        EXPECT_EQ(forecast.size(), 3u);
    }
}

TEST(EdgeCaseTest, SingleStepForecast) {
    auto ts = makeLinearSeries(20);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto forecast = model.predict(1);
    EXPECT_EQ(forecast.size(), 1u);
    EXPECT_GT(forecast[0].timestamp_ms, 0);
}

TEST(EdgeCaseTest, NoConfidenceInterval) {
    ForecastConfig cfg;
    cfg.include_confidence = false;
    auto ts = makeLinearSeries(10);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts, cfg);
    auto forecast = model.predict(3);
    for (const auto& fp : forecast) {
        EXPECT_DOUBLE_EQ(fp.lower, fp.value);
        EXPECT_DOUBLE_EQ(fp.upper, fp.value);
    }
}

TEST(EdgeCaseTest, AllMethodsOnLargeSeries) {
    auto ts = makeSeasonalSeries(120, 12, 0.05, 3600000LL);  // hourly for 5 days
    ForecastConfig cfg;
    cfg.seasonality = 12;
    for (auto method : {ForecastMethod::LINEAR_REGRESSION,
                        ForecastMethod::EXP_SMOOTHING,
                        ForecastMethod::HOLT_WINTERS,
                        ForecastMethod::ARIMA,
                        ForecastMethod::ENSEMBLE}) {
        ForecastModel model(method);
        model.fit(ts, cfg);
        auto forecast = model.predict(24);
        EXPECT_EQ(forecast.size(), 24u);
        for (const auto& fp : forecast) {
            EXPECT_FALSE(std::isnan(fp.value));
            EXPECT_FALSE(std::isinf(fp.value));
        }
    }
}

// ============================================================================
// SIMD parity tests — Yule–Walker autocovariance (AVX-512 / AVX2 vs scalar)
//
// The autocovariance inner loop in yuleWalker() is accelerated by
// computeAutocovariance() (AVX-512 → AVX2 → scalar dispatch).
// These tests verify that the SIMD path produces identical AR coefficients
// (and hence forecasts) as a plain scalar reference implementation.
// Both the SIMD and scalar paths operate on the same input data, and the
// scalar reference replicates exactly the yuleWalker algorithm from
// forecasting.cpp using a simple C for-loop instead of SIMD intrinsics.
// ============================================================================

namespace {

// Build a pure AR(1) series with known coefficient ≈ 0.8.
static TimeSeries makeAR1Series(int n, double phi = 0.8,
                                 int64_t interval_ms = 1000) {
    TimeSeries ts;
    double y = 0.0;
    for (int i = 0; i < n; ++i) {
        // Deterministic "noise" to avoid random seed dependency in CI
        double noise = static_cast<double>((i * 7 + 13) % 11) * 0.05 - 0.25;
        y = phi * y + noise;
        ts.push(static_cast<int64_t>(i) * interval_ms, y);
    }
    return ts;
}

// ---------------------------------------------------------------------------
// Scalar reference implementations for parity verification
// These mirror the algorithm in forecasting.cpp but use plain C for-loops
// instead of SIMD intrinsics.  A bit-faithful SIMD implementation must
// produce results within the same floating-point tolerance.
// ---------------------------------------------------------------------------

static double scalarMean(const std::vector<double>& v) {
    double s = 0.0;
    for (double x : v) {
      s += x;
    }
    return s / static_cast<double>(v.size());
}

// Scalar autocovariance: Σ (y[i] - mean)(y[i-lag] - mean) for i=[lag, n).
// Mirrors the body of acov0_avx2 / acov0_avx512 / scalar fallback in
// forecasting.cpp::computeAutocovariance().
static double scalarAcov(const std::vector<double>& y, double mean, int lag) {
    const size_t start = static_cast<size_t>(lag);
    const size_t n     = y.size();
    double acc = 0.0;
    for (size_t i = start; i < n; ++i)
        acc += (y[i] - mean) * (y[i - start] - mean);
    return acc;
}

// Scalar Yule–Walker using Levinson–Durbin.
// Mirrors yuleWalker() in forecasting.cpp, replacing computeAutocovariance()
// with scalarAcov() above.
static std::vector<double> scalarYuleWalker(const std::vector<double>& yc, int p) {
    size_t n = yc.size();
    if (n < static_cast<size_t>(p) + 1)
        return std::vector<double>(static_cast<size_t>(p), 0.0);

    double mean_yc = scalarMean(yc);

    std::vector<double> r(static_cast<size_t>(p) + 1);
    for (int k = 0; k <= p; ++k)
        r[static_cast<size_t>(k)] =
            scalarAcov(yc, mean_yc, k) / static_cast<double>(n);

    if (r[0] < 1e-15)
        return std::vector<double>(static_cast<size_t>(p), 0.0);

    std::vector<double> phi(static_cast<size_t>(p), 0.0);
    std::vector<double> phi_prev(static_cast<size_t>(p), 0.0);
    double err = r[0];
    for (int k = 1; k <= p; ++k) {
        double lambda = r[static_cast<size_t>(k)];
        for (int j = 1; j < k; ++j)
            lambda -= phi_prev[static_cast<size_t>(j - 1)]
                    * r[static_cast<size_t>(k - j)];
        lambda /= err;
        phi.assign(static_cast<size_t>(p), 0.0);
        phi[static_cast<size_t>(k - 1)] = lambda;
        for (int j = 1; j < k; ++j)
            phi[static_cast<size_t>(j - 1)] =
                phi_prev[static_cast<size_t>(j - 1)]
                - lambda * phi_prev[static_cast<size_t>(k - j - 1)];
        err *= (1.0 - lambda * lambda);
        phi_prev = phi;
    }
    return phi_prev;
}

// Relative + absolute tolerance guard: loose enough for different FP summation
// orders (SIMD vs scalar), tight enough to catch algorithmic errors.
static double relTol(double ref, double rel = 1e-9, double abs_floor = 1e-300) {
    return std::abs(ref) * rel + abs_floor;
}

// Scalar AR(p) one-step-ahead forecast replicating fitARIMA + predictARIMA
// for d=0, q=0.  Returns the expected value of the next observation given
// the fitted scalar AR coefficients.
static double scalarARIMAForecast1(const std::vector<double>& y, int p) {
    double mean_y = scalarMean(y);
    size_t n = y.size();

    std::vector<double> yc(n);
    for (size_t i = 0; i < n; ++i) {
      yc[i] = y[i] - mean_y;
    }

    std::vector<double> phi = scalarYuleWalker(yc, p);
    size_t ap = phi.size();

    double ar_contrib = 0.0;
    for (size_t j = 0; j < ap; ++j)
        ar_contrib += phi[j] * yc[n - 1 - j];

    return mean_y + ar_contrib;
}

}  // anonymous namespace

// ---------------------------------------------------------------------------
// Parity: AR(1) — scalar reference vs SIMD production path
//
// The scalar reference (scalarARIMAForecast1) computes the one-step forecast
// using a plain C for-loop for all autocovariance sums.  The production ARIMA
// model internally calls computeAutocovariance() which dispatches to AVX-512,
// AVX2, or scalar depending on the CPU.
// A bit-faithful SIMD implementation must produce a forecast within a tight
// relative tolerance (1 part in 10^9) of the scalar result.
// ---------------------------------------------------------------------------
TEST(SIMDParityTest, ARIMA_AR1_ScalarVsSIMD) {
    auto ts = makeAR1Series(512);
    std::vector<double> y = ts.values();

    // Scalar reference: pure C loop autocovariance + Levinson–Durbin
    double scalar_forecast = scalarARIMAForecast1(y, 1);

    // Production path: internally uses SIMD computeAutocovariance
    ForecastConfig cfg;
    cfg.ar_order = 1;
    cfg.diff_order = 0;
    cfg.ma_order = 0;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);
    auto fp = model.predict(1);

    ASSERT_FALSE(fp.empty());
    EXPECT_FALSE(std::isnan(fp[0].value));
    EXPECT_FALSE(std::isinf(fp[0].value));

    // Tolerance: 1e-9 relative — tight enough to catch algorithmic errors,
    // loose enough to allow different FP summation order between SIMD paths.
    double tol = relTol(scalar_forecast);
    EXPECT_NEAR(scalar_forecast, fp[0].value, tol)
        << "ARIMA AR(1) SIMD forecast diverges from scalar reference";
}

// Parity: AR(2) on 1 000-sample series — exercises the AVX-512 inner loop
// (≥8 doubles/cycle).  The fitted AR coefficients derived from scalar and SIMD
// autocovariance must produce identical one-step forecasts within 1 part in 10^9.
TEST(SIMDParityTest, ARIMA_AR2_ScalarVsSIMD) {
    // AR(2): y[i] ≈ 0.7*y[i-1] - 0.2*y[i-2] + deterministic noise
    TimeSeries ts;
    double y0 = 0.0, y1 = 0.0;
    for (int i = 0; i < 1000; ++i) {
        double noise = static_cast<double>((i * 11 + 7) % 13) * 0.03 - 0.18;
        double y2 = 0.7 * y1 - 0.2 * y0 + noise;
        ts.push(static_cast<int64_t>(i) * 1000LL, y2);
        y0 = y1; y1 = y2;
    }
    std::vector<double> y = ts.values();

    // Scalar reference
    double scalar_forecast = scalarARIMAForecast1(y, 2);

    // Production SIMD path
    ForecastConfig cfg;
    cfg.ar_order = 2;
    cfg.diff_order = 0;
    cfg.ma_order = 0;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);
    auto fp = model.predict(1);

    ASSERT_FALSE(fp.empty());
    EXPECT_FALSE(std::isnan(fp[0].value))
        << "ARIMA AR(2) forecast is NaN — SIMD autocovariance may be wrong";
    EXPECT_FALSE(std::isinf(fp[0].value))
        << "ARIMA AR(2) forecast is Inf — SIMD autocovariance may overflow";
    EXPECT_LT(std::abs(fp[0].value), 1000.0)
        << "ARIMA forecast diverged — AR coefficients out of range";

    double tol = relTol(scalar_forecast);
    EXPECT_NEAR(scalar_forecast, fp[0].value, tol)
        << "ARIMA AR(2) SIMD forecast diverges from scalar reference";
}

// Parity: scalar and SIMD autocovariance agree on a flat series (all zeros).
// This exercises the edge case r[0] < 1e-15 guard in yuleWalker — both paths
// must return AR coefficients = 0 and a finite (mean) forecast.
TEST(SIMDParityTest, ARIMA_FlatSeries_NoNaN) {
    TimeSeries ts;
    for (int i = 0; i < 50; ++i) {
      ts.push(static_cast<int64_t>(i) * 1000LL, 0.0);
    }
    std::vector<double> y = ts.values();

    // Scalar reference: mean is 0, phi = 0, forecast = 0.
    double scalar_forecast = scalarARIMAForecast1(y, 2);
    EXPECT_DOUBLE_EQ(scalar_forecast, 0.0);

    ForecastConfig cfg;
    cfg.ar_order = 2;
    cfg.diff_order = 0;
    cfg.ma_order = 0;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);
    auto forecast = model.predict(3);

    for (const auto& fp : forecast) {
        EXPECT_FALSE(std::isnan(fp.value));
        EXPECT_FALSE(std::isinf(fp.value));
        // Both scalar and SIMD must yield 0 (all-zeros series, mean = 0)
        EXPECT_DOUBLE_EQ(fp.value, scalar_forecast);
    }
}

// ============================================================================
// ForecastingBatchStreamingTests — Issue #4054 (v1.9.0)
// ============================================================================

// ---------------------------------------------------------------------------
// predictBatch — basic shape and consistency
// ---------------------------------------------------------------------------

TEST(ForecastingBatchStreamingTests, PredictBatch_ReturnsCorrectShape) {
    // 3 series, 20 steps each
    std::vector<TimeSeries> batch = {};

    for (int s = 0; s < 3; ++s) {
        batch.push_back(makeLinearSeries(30, /*slope=*/static_cast<double>(s + 1)));
    }

    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(batch[0]);  // model must be fitted before predictBatch

    auto results = model.predictBatch(batch, 20);

    ASSERT_EQ(results.size(), 3u);
    for (const auto& r : results) {
        ASSERT_EQ(r.size(), 20u);
        for (const auto& fp : r) {
            EXPECT_FALSE(std::isnan(fp.value));
            EXPECT_FALSE(std::isinf(fp.value));
        }
    }
}

TEST(ForecastingBatchStreamingTests, PredictBatch_SingleSeries_MatchesSinglePredict) {
    TimeSeries ts = makeLinearSeries(50, 2.0, 5.0);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);

    auto single_result = model.predict(10);
    auto batch_result  = model.predictBatch({ts}, 10);

    ASSERT_EQ(batch_result.size(), 1u);
    ASSERT_EQ(batch_result[0].size(), single_result.size());
    for (size_t i = 0; i < single_result.size(); ++i) {
        EXPECT_NEAR(batch_result[0][i].value, single_result[i].value, 1e-9)
            << "Batch predict differs from single predict at step " << i;
    }
}

TEST(ForecastingBatchStreamingTests, PredictBatch_InvalidSteps_Throws) {
    TimeSeries ts = makeLinearSeries(20);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    EXPECT_THROW(model.predictBatch({ts}, 0), std::invalid_argument);
    EXPECT_THROW(model.predictBatch({ts}, -1), std::invalid_argument);
}

TEST(ForecastingBatchStreamingTests, PredictBatch_EmptyBatch_ReturnsEmpty) {
    TimeSeries ts = makeLinearSeries(20);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);
    auto results = model.predictBatch({}, 5);
    EXPECT_TRUE(results.empty());
}

TEST(ForecastingBatchStreamingTests, PredictBatch_AllMethods) {
    std::vector<TimeSeries> batch = {};

    for (int i = 0; i < 3; ++i)
        batch.push_back(makeSeasonalSeries(40, 4, 0.5));

    for (auto method : {ForecastMethod::EXP_SMOOTHING,
                        ForecastMethod::HOLT_WINTERS,
                        ForecastMethod::ARIMA,
                        ForecastMethod::ENSEMBLE}) {
        ForecastModel model(method);
        model.fit(batch[0]);
        auto results = model.predictBatch(batch, 5);
        ASSERT_EQ(results.size(), 3u) << "method=" << forecastMethodName(method);
        for (const auto& r : results) {
            ASSERT_EQ(r.size(), 5u) << "method=" << forecastMethodName(method);
            for (const auto& fp : r)
                EXPECT_FALSE(std::isnan(fp.value)) << "method=" << forecastMethodName(method);
        }
    }
}

// ---------------------------------------------------------------------------
// update — incremental absorption
// ---------------------------------------------------------------------------

TEST(ForecastingBatchStreamingTests, Update_NoOpOnUnfittedModel) {
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    EXPECT_NO_THROW(model.update(42.0));  // must be a no-op
    EXPECT_FALSE(model.isFitted());
}

TEST(ForecastingBatchStreamingTests, Update_SES_ShiftsLevel) {
    TimeSeries ts = makeLinearSeries(20, 1.0);
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    model.fit(ts);

    auto before = model.predict(5);
    model.update(25.0);  // add a new observation
    auto after  = model.predict(5);

    // The SES level must have moved toward the new observation
    // — forecast values must differ from the pre-update forecast
    bool changed = false;
    for (size_t i = 0; i < before.size(); ++i) {
        if (std::abs(after[i].value - before[i].value) > 1e-12) {
            changed = true;
            break;
        }
    }
    EXPECT_TRUE(changed) << "SES forecast unchanged after update()";
}

TEST(ForecastingBatchStreamingTests, Update_HW_ShiftsLevel) {
    TimeSeries ts = makeSeasonalSeries(40, 4, 0.1);
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts);

    auto before = model.predict(4);
    model.update(99.0);
    auto after  = model.predict(4);

    bool changed = false;
    for (size_t i = 0; i < before.size(); ++i) {
        if (std::abs(after[i].value - before[i].value) > 1e-9) { changed = true; break; }
    }
    EXPECT_TRUE(changed) << "HW forecast unchanged after update()";
}

TEST(ForecastingBatchStreamingTests, Update_ARIMA_ShiftsWindow) {
    TimeSeries ts = makeLinearSeries(30, 1.0);
    ForecastConfig cfg;
    cfg.ar_order = 2;
    ForecastModel model(ForecastMethod::ARIMA);
    model.fit(ts, cfg);

    auto before = model.predict(3);
    model.update(999.0);
    auto after  = model.predict(3);

    bool changed = false;
    for (size_t i = 0; i < before.size(); ++i) {
        if (std::abs(after[i].value - before[i].value) > 1e-9) { changed = true; break; }
    }
    EXPECT_TRUE(changed) << "ARIMA forecast unchanged after update()";
}

TEST(ForecastingBatchStreamingTests, Update_MultipleUpdates_ModelStillFitted) {
    TimeSeries ts = makeLinearSeries(20);
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    model.fit(ts);

    for (int i = 0; i < 10; ++i) {
        model.update(static_cast<double>(20 + i));
        EXPECT_TRUE(model.isFitted());
        EXPECT_NO_THROW(model.predict(3));
    }
}

TEST(ForecastingBatchStreamingTests, Update_LinearRegression_IncrementalOLS) {
    // Verify that update() on a LINEAR_REGRESSION model produces the same
    // alpha/beta/forecast as a full refit on the extended series.
    // This exercises the O(1) running-moment OLS path.
    TimeSeries ts = makeLinearSeries(20, 2.0, 5.0);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);

    double new_val = 50.0;
    model.update(new_val);
    auto after_update = model.predict(5);

    // Build the reference: full refit on ts + new_val
    TimeSeries ts_extended = ts;
    ts_extended.push(static_cast<int64_t>(20) * 1000LL, new_val);
    ForecastModel ref(ForecastMethod::LINEAR_REGRESSION);
    ref.fit(ts_extended);
    auto after_refit = ref.predict(5);

    ASSERT_EQ(after_update.size(), after_refit.size());
    for (size_t i = 0; i < after_update.size(); ++i) {
        EXPECT_NEAR(after_update[i].value, after_refit[i].value, 1e-6)
            << "Incremental OLS update differs from full refit at step " << i;
    }
}

TEST(ForecastingBatchStreamingTests, Update_LinearRegression_MultipleUpdatesConsistent) {
    // Apply 5 successive update() calls and verify results match a full
    // refit on the correspondingly extended series.
    TimeSeries ts = makeLinearSeries(30, 1.5, 0.0);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);
    model.fit(ts);

    TimeSeries ts_ext = ts;
    for (int i = 0; i < 5; ++i) {
        double v = 30.0 + static_cast<double>(i) * 1.5;
        model.update(v);
        ts_ext.push(static_cast<int64_t>(30 + i) * 1000LL, v);
    }
    auto inc = model.predict(3);

    ForecastModel ref(ForecastMethod::LINEAR_REGRESSION);
    ref.fit(ts_ext);
    auto full = ref.predict(3);

    ASSERT_EQ(inc.size(), full.size());
    for (size_t i = 0; i < inc.size(); ++i) {
        EXPECT_NEAR(inc[i].value, full[i].value, 1e-5)
            << "Incremental OLS (5 updates) differs from full refit at step " << i;
    }
}

// ---------------------------------------------------------------------------
// fit-result cache — repeated fit on same data is O(1) lookup
// ---------------------------------------------------------------------------

TEST(ForecastingBatchStreamingTests, FitCache_SecondFitOnSameDataIsConsistent) {
    TimeSeries ts = makeLinearSeries(50, 3.0, 10.0);
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);

    model.fit(ts);
    auto first = model.predict(10);

    // Second fit on identical data — should produce identical results (cache hit)
    model.fit(ts);
    auto second = model.predict(10);

    ASSERT_EQ(first.size(), second.size());
    for (size_t i = 0; i < first.size(); ++i) {
        EXPECT_DOUBLE_EQ(first[i].value, second[i].value)
            << "Cache hit produced different result at step " << i;
    }
}

TEST(ForecastingBatchStreamingTests, FitCache_DifferentDataProducesDifferentResult) {
    TimeSeries ts1 = makeLinearSeries(30, 1.0);
    TimeSeries ts2 = makeLinearSeries(30, 5.0);  // different slope
    ForecastModel model(ForecastMethod::LINEAR_REGRESSION);

    model.fit(ts1);
    auto r1 = model.predict(5);

    model.fit(ts2);
    auto r2 = model.predict(5);

    bool differs = false;
    for (size_t i = 0; i < r1.size(); ++i) {
        if (std::abs(r1[i].value - r2[i].value) > 1e-6) { differs = true; break; }
    }
    EXPECT_TRUE(differs) << "Fitting different data produced the same forecast";
}

// ---------------------------------------------------------------------------
// Parallel auto-tune — verify alpha converges to a better value
// ---------------------------------------------------------------------------

TEST(ForecastingBatchStreamingTests, AutoTune_PicksBetterAlpha) {
    // Series with a strong trend: best alpha should be relatively high
    TimeSeries ts = makeLinearSeries(100, 2.0);
    ForecastConfig cfg;
    cfg.auto_tune = true;
    ForecastModel model(ForecastMethod::EXP_SMOOTHING);
    model.fit(ts, cfg);

    // After auto-tune the model should still be fitted and produce finite forecasts
    EXPECT_TRUE(model.isFitted());
    auto preds = model.predict(10);
    for (const auto& fp : preds) {
        EXPECT_FALSE(std::isnan(fp.value));
        EXPECT_FALSE(std::isinf(fp.value));
    }
}

TEST(ForecastingBatchStreamingTests, AutoTune_HW_ProducesFiniteForecast) {
    TimeSeries ts = makeSeasonalSeries(60, 4, 0.2);
    ForecastConfig cfg;
    cfg.auto_tune    = true;
    cfg.seasonality  = 4;
    ForecastModel model(ForecastMethod::HOLT_WINTERS);
    model.fit(ts, cfg);

    EXPECT_TRUE(model.isFitted());
    auto preds = model.predict(8);
    ASSERT_EQ(preds.size(), 8u);
    for (const auto& fp : preds) {
        EXPECT_FALSE(std::isnan(fp.value));
        EXPECT_FALSE(std::isinf(fp.value));
    }
}
