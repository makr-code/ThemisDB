/**
 * SARIMA and Prophet-style forecasting unit tests.
 *
 * Test IDs:
 *   FSP-01 … FSP-08  – SARIMA (Seasonal ARIMA)
 *   FPR-01 … FPR-04  – Prophet-style trend + Fourier seasonality
 */

#include <gtest/gtest.h>
#include "analytics/forecasting.h"

#include <cmath>
#include <string>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

static TimeSeries makeSineSeries(int n, double period = 12.0, double amp = 5.0,
                                 double trend_slope = 0.0,
                                 int64_t start_ms   = 0,
                                 int64_t interval_ms = 86400000LL)
{
    TimeSeries ts;
    for (int i = 0; i < n; ++i) {
        double v = amp * std::sin(2.0 * 3.14159265358979323846 * static_cast<double>(i) / period)
                 + trend_slope * static_cast<double>(i)
                 + 10.0; // baseline
        ts.push(start_ms + static_cast<int64_t>(i) * interval_ms, v);
    }
    return ts;
}

// ============================================================================
// FSP: SARIMA tests
// ============================================================================

// FSP-01: SARIMA fit does not throw on a seasonal series
TEST(SARIMAForecastTests, FSP01_FitNoThrow) {
    auto ts = makeSineSeries(60, /*period=*/12);
    ForecastConfig cfg;
    cfg.sarima_m = 12;
    cfg.ar_order  = 2; cfg.diff_order = 1; cfg.ma_order = 1;
    cfg.sarima_P  = 1; cfg.sarima_D   = 1; cfg.sarima_Q = 1;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    EXPECT_NO_THROW(model.fit(ts));
    EXPECT_TRUE(model.isFitted());
}

// FSP-02: SARIMA predict returns correct number of steps
TEST(SARIMAForecastTests, FSP02_PredictStepCount) {
    auto ts = makeSineSeries(48, 12);
    ForecastConfig cfg; cfg.sarima_m = 12;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    model.fit(ts);
    auto preds = model.predict(12);
    EXPECT_EQ(static_cast<int>(preds.size()), 12);
}

// FSP-03: SARIMA timestamps are monotonically increasing
TEST(SARIMAForecastTests, FSP03_TimestampsMonotonic) {
    auto ts = makeSineSeries(36, 12);
    ForecastConfig cfg; cfg.sarima_m = 12;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    model.fit(ts);
    auto preds = model.predict(6);
    for (size_t i = 1; i < preds.size(); ++i)
        EXPECT_GT(preds[i].timestamp_ms, preds[i - 1].timestamp_ms);
}

// FSP-04: SARIMA CI lower ≤ value ≤ upper
TEST(SARIMAForecastTests, FSP04_CIBoundsConsistent) {
    auto ts = makeSineSeries(36, 12);
    ForecastConfig cfg; cfg.sarima_m = 12; cfg.include_confidence = true;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    model.fit(ts);
    for (const auto& fp : model.predict(12)) {
        EXPECT_LE(fp.lower, fp.value + 1e-9);
        EXPECT_GE(fp.upper, fp.value - 1e-9);
    }
}

// FSP-05: SARIMA predict throws if not fitted
TEST(SARIMAForecastTests, FSP05_PredictThrowsIfNotFitted) {
    ForecastConfig cfg; cfg.sarima_m = 12;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    EXPECT_THROW(model.predict(5), std::runtime_error);
}

// FSP-06: SARIMA serialize / deserialize round-trip preserves method
TEST(SARIMAForecastTests, FSP06_SerializeRoundTrip) {
    auto ts = makeSineSeries(48, 12);
    ForecastConfig cfg; cfg.sarima_m = 12;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    model.fit(ts);
    auto blob  = model.serialize();
    auto model2 = ForecastModel::deserialize(blob);
    EXPECT_EQ(model2.method(), ForecastMethod::SARIMA);
    EXPECT_TRUE(model2.isFitted());
}

// FSP-07: SARIMA predict after deserialize matches original predict
TEST(SARIMAForecastTests, FSP07_SerializePreservesPredict) {
    auto ts = makeSineSeries(48, 12);
    ForecastConfig cfg; cfg.sarima_m = 12;
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    model.fit(ts);
    auto preds_orig = model.predict(6);
    auto model2     = ForecastModel::deserialize(model.serialize());
    auto preds_des  = model2.predict(6);
    ASSERT_EQ(preds_orig.size(), preds_des.size());
    for (size_t i = 0; i < preds_orig.size(); ++i)
        EXPECT_NEAR(preds_orig[i].value, preds_des[i].value, 1e-6);
}

// FSP-08: SARIMA with sarima_m autodetected from config.seasonality
TEST(SARIMAForecastTests, FSP08_AutodetectSeasonality) {
    auto ts = makeSineSeries(60, 12);
    ForecastConfig cfg;
    cfg.seasonality = 12;
    cfg.sarima_m    = 0; // trigger autodetect
    ForecastModel model(cfg, ForecastMethod::SARIMA);
    EXPECT_NO_THROW(model.fit(ts));
    auto preds = model.predict(3);
    EXPECT_EQ(static_cast<int>(preds.size()), 3);
}

// ============================================================================
// FPR: Prophet-style tests
// ============================================================================

// FPR-01: Prophet fit does not throw on a series with trend + seasonality
TEST(ProphetForecastTests, FPR01_FitNoThrow) {
    // Daily series spanning ~3 years (≈ 1095 days)
    auto ts = makeSineSeries(365, /*period=*/365.0, /*amp=*/10.0, /*slope=*/0.05);
    ForecastConfig cfg;
    cfg.prophet_fourier_order_weekly = 3;
    cfg.prophet_fourier_order_yearly = 6;
    ForecastModel model(cfg, ForecastMethod::PROPHET);
    EXPECT_NO_THROW(model.fit(ts));
    EXPECT_TRUE(model.isFitted());
}

// FPR-02: Prophet predict returns the correct number of steps
TEST(ProphetForecastTests, FPR02_PredictStepCount) {
    auto ts = makeSineSeries(200, 7);
    ForecastConfig cfg;
    cfg.prophet_fourier_order_weekly = 2;
    cfg.prophet_fourier_order_yearly = 4;
    ForecastModel model(cfg, ForecastMethod::PROPHET);
    model.fit(ts);
    auto preds = model.predict(30);
    EXPECT_EQ(static_cast<int>(preds.size()), 30);
}

// FPR-03: Prophet CI bounds are consistent (lower ≤ value ≤ upper)
TEST(ProphetForecastTests, FPR03_CIBoundsConsistent) {
    auto ts = makeSineSeries(120, 12);
    ForecastConfig cfg; cfg.include_confidence = true; cfg.confidence_level = 0.95;
    ForecastModel model(cfg, ForecastMethod::PROPHET);
    model.fit(ts);
    for (const auto& fp : model.predict(12)) {
        EXPECT_LE(fp.lower, fp.value + 1e-9);
        EXPECT_GE(fp.upper, fp.value - 1e-9);
    }
}

// FPR-04: Prophet serialize / deserialize round-trip
TEST(ProphetForecastTests, FPR04_SerializeRoundTrip) {
    auto ts = makeSineSeries(60, 7);
    ForecastConfig cfg;
    ForecastModel model(cfg, ForecastMethod::PROPHET);
    model.fit(ts);
    auto blob   = model.serialize();
    auto model2 = ForecastModel::deserialize(blob);
    EXPECT_EQ(model2.method(), ForecastMethod::PROPHET);
    EXPECT_TRUE(model2.isFitted());
    // Predictions should match
    auto p1 = model.predict(5);
    auto p2 = model2.predict(5);
    ASSERT_EQ(p1.size(), p2.size());
    for (size_t i = 0; i < p1.size(); ++i)
        EXPECT_NEAR(p1[i].value, p2[i].value, 1e-6);
}
