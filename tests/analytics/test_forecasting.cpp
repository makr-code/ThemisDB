/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            test_forecasting.cpp                               ║
  Version:         0.0.1                                              ║
  Last Modified:   2026-02-23                                         ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟡 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     ~450                                           ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

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
        double seasonal = 2.0 * std::sin(2.0 * M_PI * static_cast<double>(i) / static_cast<double>(period));
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
    for (int i = 0; i < 20; ++i) ts.push(static_cast<int64_t>(i) * 1000, 5.0);
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
        double seasonal = 1.0 + 0.2 * std::sin(2.0 * M_PI * static_cast<double>(i) / 4.0);
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
                (5.0 + 0.1 * i) * (1.0 + 0.1 * std::sin(2.0 * M_PI * static_cast<double>(i) / 4.0)));
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
        EXPECT_NEAR(f1[i].value, f2[i].value, 1e-4);  // text serialization precision
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
