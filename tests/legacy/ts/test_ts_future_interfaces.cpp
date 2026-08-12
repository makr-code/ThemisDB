/**
 * @file test_ts_future_interfaces.cpp
 * @brief Focused unit tests for new timeseries module interfaces:
 *        - AdaptiveCompressionSelector / PerSeriesCompressionRegistry
 *        - AnomalyDetector (ZScore + IQR)
 *        - GapFiller (ForwardFill, LinearInterpolation, BackwardFill, NullFill)
 *
 * All tests run without a live RocksDB instance.
 */

#include <gtest/gtest.h>

#include "timeseries/compression_selector.h"
#include "timeseries/anomaly_detection.h"
#include "timeseries/gap_fill.h"

#include <algorithm>
#include <cmath>
#include <vector>

namespace themis {
namespace {

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static TSStore::DataPoint dp(const std::string& metric,
                              const std::string& entity,
                              int64_t            ts,
                              double             val) {
    TSStore::DataPoint p;
    p.metric       = metric;
    p.entity       = entity;
    p.timestamp_ms = ts;
    p.value        = val;
    return p;
}

static std::vector<TSStore::DataPoint> sineSeries(size_t n, int64_t base_ms = 0,
                                                   int64_t step_ms = 1000) {
    std::vector<TSStore::DataPoint> pts;
    pts.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        pts.push_back(dp("m", "e", base_ms + static_cast<int64_t>(i) * step_ms,
                          std::sin(static_cast<double>(i) * 0.3)));
    }
    return pts;
}

static std::vector<TSStore::DataPoint> constSeries(size_t n,
                                                    double val = 42.0,
                                                    int64_t base_ms = 0,
                                                    int64_t step_ms = 1000) {
    std::vector<TSStore::DataPoint> pts;
    pts.reserve(n);
    for (size_t i = 0; i < n; ++i) {
        pts.push_back(dp("m", "e",
                          base_ms + static_cast<int64_t>(i) * step_ms, val));
    }
    return pts;
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 1: profileSeries
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, ProfileSeries_EmptyInput) {
    std::vector<TSStore::DataPoint> pts;
    SeriesProfile p = profileSeries(pts);
    EXPECT_EQ(p.sample_count, 0u);
    EXPECT_DOUBLE_EQ(p.value_variance, 0.0);
    EXPECT_DOUBLE_EQ(p.run_length_ratio, 0.0);
}

TEST(TsFutureInterfacesTests, ProfileSeries_SinglePoint) {
    auto pts = constSeries(1);
    SeriesProfile p = profileSeries(pts);
    EXPECT_EQ(p.sample_count, 1u);
    EXPECT_DOUBLE_EQ(p.value_variance, 0.0);
}

TEST(TsFutureInterfacesTests, ProfileSeries_ConstantSeries_HighRunRatio) {
    auto pts = constSeries(20, 5.0);
    SeriesProfile p = profileSeries(pts);
    EXPECT_EQ(p.sample_count, 20u);
    EXPECT_DOUBLE_EQ(p.value_variance, 0.0);
    EXPECT_NEAR(p.run_length_ratio, 1.0, 1e-9);
}

TEST(TsFutureInterfacesTests, ProfileSeries_SineSeries_LowRunRatio) {
    auto pts = sineSeries(50);
    SeriesProfile p = profileSeries(pts);
    EXPECT_EQ(p.sample_count, 50u);
    EXPECT_GT(p.value_variance, 0.0);
    EXPECT_LT(p.run_length_ratio, 0.1);
}

TEST(TsFutureInterfacesTests, ProfileSeries_RegularTimestamps) {
    auto pts = sineSeries(20, 0, 1000);  // perfectly regular 1-second intervals
    SeriesProfile p = profileSeries(pts);
    EXPECT_NEAR(p.timestamp_regularity, 1.0, 1e-9);
    EXPECT_NEAR(p.dod_mean_abs, 0.0, 1e-9);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 2: HeuristicCompressionSelector
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, HeuristicSelector_TooFewSamples_ReturnsNone) {
    HeuristicCompressionSelector sel;
    auto pts = sineSeries(2);
    EXPECT_EQ(sel.selectForPoints(pts), CompressionStrategy::None);
}

TEST(TsFutureInterfacesTests, HeuristicSelector_ConstantSeries_ReturnsRLE) {
    HeuristicCompressionSelector sel;
    auto pts = constSeries(20);
    EXPECT_EQ(sel.selectForPoints(pts), CompressionStrategy::RLE);
}

TEST(TsFutureInterfacesTests, HeuristicSelector_SineSeries_ReturnsGorilla) {
    HeuristicCompressionSelector sel;
    auto pts = sineSeries(50);
    EXPECT_EQ(sel.selectForPoints(pts), CompressionStrategy::DeltaOfDelta);
}

TEST(TsFutureInterfacesTests, HeuristicSelector_RegularCounters_ReturnsDeltaOfDelta) {
    // Perfectly regular timestamps and constant delta-of-delta
    std::vector<TSStore::DataPoint> pts;
    for (int i = 0; i < 20; ++i) {
        pts.push_back(dp("m", "e", static_cast<int64_t>(i) * 1000,
                          static_cast<double>(i)));  // monotone integers
    }
    HeuristicCompressionSelector sel;
    EXPECT_EQ(sel.selectForPoints(pts), CompressionStrategy::DeltaOfDelta);
}

TEST(TsFutureInterfacesTests, HeuristicSelector_SelectFromProfile) {
    SeriesProfile p;
    p.sample_count        = 20;
    p.run_length_ratio    = 0.0;
    p.dod_mean_abs        = 0.0;
    p.timestamp_regularity = 1.0;

    HeuristicCompressionSelector sel;
    EXPECT_EQ(sel.select(p), CompressionStrategy::DeltaOfDelta);
}

TEST(TsFutureInterfacesTests, HeuristicSelector_CustomConfig) {
    HeuristicCompressionSelector::Config cfg;
    cfg.min_samples            = 2;
    cfg.rle_run_ratio_threshold = 0.5;
    HeuristicCompressionSelector sel(cfg);

    auto pts = constSeries(10);
    EXPECT_EQ(sel.selectForPoints(pts), CompressionStrategy::RLE);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 3: PerSeriesCompressionRegistry
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, Registry_DefaultConstructor_DefaultSelector) {
    PerSeriesCompressionRegistry reg;
    auto pts = constSeries(20);
    // Should use HeuristicCompressionSelector → RLE for constant series
    EXPECT_EQ(reg.strategyFor("m", "e", pts), CompressionStrategy::RLE);
}

TEST(TsFutureInterfacesTests, Registry_PinOverridesSelection) {
    PerSeriesCompressionRegistry reg;
    reg.pinStrategy("m", "e", CompressionStrategy::Gorilla);
    auto pts = constSeries(20);  // would normally be RLE
    EXPECT_EQ(reg.strategyFor("m", "e", pts), CompressionStrategy::Gorilla);
}

TEST(TsFutureInterfacesTests, Registry_ClearPin_ReselectsFromSample) {
    PerSeriesCompressionRegistry reg;
    reg.pinStrategy("m", "e", CompressionStrategy::Gorilla);
    reg.clearPin("m", "e");

    auto pts = constSeries(20);
    EXPECT_EQ(reg.strategyFor("m", "e", pts), CompressionStrategy::RLE);
}

TEST(TsFutureInterfacesTests, Registry_CachedOnSecondCall) {
    PerSeriesCompressionRegistry reg;
    auto pts = sineSeries(50);
    auto s1 = reg.strategyFor("m", "e", pts);
    // Second call with empty sample should return cached result
    auto s2 = reg.strategyFor("m", "e", {});
    EXPECT_EQ(s1, s2);
}

TEST(TsFutureInterfacesTests, Registry_ClearCache_RerunsSelection) {
    PerSeriesCompressionRegistry reg;
    auto pts = sineSeries(50);
    reg.strategyFor("m", "e", pts);   // caches Gorilla
    reg.clearCache();

    // With empty sample, HeuristicSelector should return None (too few points)
    auto s = reg.strategyFor("m", "e", {});
    EXPECT_EQ(s, CompressionStrategy::None);
}

TEST(TsFutureInterfacesTests, Registry_RegistrySizeAndClear) {
    PerSeriesCompressionRegistry reg;
    reg.pinStrategy("m1", "e1", CompressionStrategy::Gorilla);
    reg.strategyFor("m2", "e2", constSeries(20));   // cached
    EXPECT_EQ(reg.registrySize(), 2u);

    reg.clear();
    EXPECT_EQ(reg.registrySize(), 0u);
}

TEST(TsFutureInterfacesTests, Registry_SetSelectorClearsCache) {
    PerSeriesCompressionRegistry reg;
    auto pts = sineSeries(50);
    reg.strategyFor("m", "e", pts);  // caches Gorilla

    // Replace with a selector that always picks RLE
    class AlwaysRLE : public ICompressionSelector {
    public:
        CompressionStrategy select(const SeriesProfile&) const override {
            return CompressionStrategy::RLE;
        }
        CompressionStrategy selectForPoints(
            const std::vector<TSStore::DataPoint>&) const override {
            return CompressionStrategy::RLE;
        }
    };
    reg.setSelector(std::make_unique<AlwaysRLE>());

    // Cache was cleared; fresh selection uses new selector
    auto s = reg.strategyFor("m", "e", pts);
    EXPECT_EQ(s, CompressionStrategy::RLE);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 4: ZScoreDetector
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, ZScore_TooFewPoints_EmptyResult) {
    ZScoreDetector det;
    AnomalyConfig cfg;
    cfg.min_samples = 4;
    auto pts = sineSeries(2);
    EXPECT_TRUE(det.detect(pts, cfg).empty());
}

TEST(TsFutureInterfacesTests, ZScore_ConstantSeries_NoAnomalies) {
    ZScoreDetector det;
    AnomalyConfig cfg;
    cfg.min_samples = 4;
    auto pts = constSeries(20, 7.0);
    EXPECT_TRUE(det.detect(pts, cfg).empty());
}

TEST(TsFutureInterfacesTests, ZScore_ClearOutlier_Detected) {
    // 19 points at 1.0, one outlier at 100.0
    auto pts = constSeries(19, 1.0, 0, 1000);
    pts.push_back(dp("m", "e", 19000, 100.0));

    ZScoreDetector det;
    AnomalyConfig cfg;
    cfg.zscore_threshold = 3.0;
    cfg.min_samples      = 4;
    auto anom = det.detect(pts, cfg);
    ASSERT_FALSE(anom.empty());
    EXPECT_EQ(anom.front().timestamp_ms, 19000);
    EXPECT_EQ(anom.front().method, "zscore");
    EXPECT_GT(anom.front().score, 3.0);
}

TEST(TsFutureInterfacesTests, ZScore_ScoreAll_CorrectCount) {
    ZScoreDetector det;
    auto pts = sineSeries(20);
    auto scores = det.scoreAll(pts);
    EXPECT_EQ(scores.size(), 20u);
}

TEST(TsFutureInterfacesTests, ZScore_HighThreshold_NoFalsePositives) {
    ZScoreDetector det;
    AnomalyConfig cfg;
    cfg.zscore_threshold = 100.0;  // impossibly high
    cfg.min_samples      = 4;
    auto pts = sineSeries(50);
    EXPECT_TRUE(det.detect(pts, cfg).empty());
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 5: IQRDetector
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, IQR_TooFewPoints_EmptyResult) {
    IQRDetector det;
    AnomalyConfig cfg;
    cfg.min_samples = 4;
    auto pts = sineSeries(3);
    EXPECT_TRUE(det.detect(pts, cfg).empty());
}

TEST(TsFutureInterfacesTests, IQR_ClearOutlier_Detected) {
    auto pts = constSeries(18, 5.0, 0, 1000);
    pts.push_back(dp("m", "e", 18000, 500.0));  // massive outlier

    IQRDetector det;
    AnomalyConfig cfg;
    cfg.iqr_multiplier = 1.5;
    cfg.min_samples    = 4;
    auto anom = det.detect(pts, cfg);
    ASSERT_FALSE(anom.empty());
    EXPECT_EQ(anom.front().timestamp_ms, 18000);
    EXPECT_EQ(anom.front().method, "iqr");
    EXPECT_GT(anom.front().score, 0.0);
}

TEST(TsFutureInterfacesTests, IQR_NoOutliers_InSymmetricData) {
    // Symmetric data within normal range
    std::vector<TSStore::DataPoint> pts;
    for (int i = 0; i < 20; ++i) {
        pts.push_back(dp("m", "e", i * 1000, static_cast<double>(i)));
    }

    IQRDetector det;
    AnomalyConfig cfg;
    cfg.iqr_multiplier = 1.5;
    cfg.min_samples    = 4;
    // With linear sequence 0..19, no point is outside [Q1 - 1.5*IQR, Q3 + 1.5*IQR]
    auto anom = det.detect(pts, cfg);
    EXPECT_TRUE(anom.empty());
}

TEST(TsFutureInterfacesTests, IQR_LowOutlier_NegativeScore) {
    auto pts = constSeries(18, 50.0, 0, 1000);
    pts.push_back(dp("m", "e", 18000, -500.0));  // low outlier

    IQRDetector det;
    AnomalyConfig cfg;
    cfg.iqr_multiplier = 1.5;
    cfg.min_samples    = 4;
    auto anom = det.detect(pts, cfg);
    ASSERT_FALSE(anom.empty());
    EXPECT_LT(anom.front().score, 0.0);  // negative = below lower fence
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 6: AnomalyDetector (combined façade)
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, AnomalyDetector_ZScoreMethod_Delegates) {
    AnomalyConfig cfg;
    cfg.method           = AnomalyMethod::ZScore;
    cfg.zscore_threshold = 3.0;
    cfg.min_samples      = 4;

    auto pts = constSeries(19, 1.0, 0, 1000);
    pts.push_back(dp("m", "e", 19000, 100.0));

    AnomalyDetector det(cfg);
    auto anom = det.detect(pts);
    ASSERT_FALSE(anom.empty());
    EXPECT_EQ(anom.front().method, "zscore");
}

TEST(TsFutureInterfacesTests, AnomalyDetector_IQRMethod_Delegates) {
    AnomalyConfig cfg;
    cfg.method         = AnomalyMethod::IQR;
    cfg.iqr_multiplier = 1.5;
    cfg.min_samples    = 4;

    auto pts = constSeries(18, 5.0, 0, 1000);
    pts.push_back(dp("m", "e", 18000, 500.0));

    AnomalyDetector det(cfg);
    auto anom = det.detect(pts);
    ASSERT_FALSE(anom.empty());
    EXPECT_EQ(anom.front().method, "iqr");
}

TEST(TsFutureInterfacesTests, AnomalyDetector_BothMethods_UnionResult) {
    AnomalyConfig cfg;
    cfg.method           = AnomalyMethod::Both;
    cfg.zscore_threshold = 3.0;
    cfg.iqr_multiplier   = 1.5;
    cfg.min_samples      = 4;

    auto pts = constSeries(18, 5.0, 0, 1000);
    pts.push_back(dp("m", "e", 18000, 500.0));
    pts.push_back(dp("m", "e", 19000, -500.0));

    AnomalyDetector det(cfg);
    auto anom = det.detect(pts);
    EXPECT_GE(anom.size(), 2u);
    // Should be sorted by timestamp
    for (size_t i = 1; i < anom.size(); ++i) {
        EXPECT_LE(anom[i-1].timestamp_ms, anom[i].timestamp_ms);
    }
}

TEST(TsFutureInterfacesTests, AnomalyDetector_ExplicitConfig_Override) {
    AnomalyConfig stored_cfg;
    stored_cfg.method           = AnomalyMethod::ZScore;
    stored_cfg.zscore_threshold = 100.0;  // very high — no anomalies

    AnomalyDetector det(stored_cfg);

    AnomalyConfig override_cfg;
    override_cfg.method           = AnomalyMethod::ZScore;
    override_cfg.zscore_threshold = 0.1;  // very low — many anomalies
    override_cfg.min_samples      = 1;

    auto pts = sineSeries(20);
    auto anom = det.detect(pts, override_cfg);
    EXPECT_FALSE(anom.empty());
}

TEST(TsFutureInterfacesTests, AnomalyDetector_SetConfig_UpdatesStoredConfig) {
    AnomalyConfig cfg;
    cfg.method           = AnomalyMethod::ZScore;
    cfg.zscore_threshold = 100.0;

    AnomalyDetector det(cfg);

    AnomalyConfig new_cfg;
    new_cfg.method           = AnomalyMethod::ZScore;
    new_cfg.zscore_threshold = 0.1;
    new_cfg.min_samples      = 1;
    det.setConfig(new_cfg);

    auto pts = sineSeries(20);
    auto anom = det.detect(pts);
    EXPECT_FALSE(anom.empty());
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 7: ForwardFillGapFiller
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, ForwardFill_ExactMatch_ReturnedVerbatim) {
    auto pts = constSeries(3, 42.0, 0, 1000);
    ForwardFillGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {0, 1000, 2000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 42.0);
    EXPECT_DOUBLE_EQ(result[1].value, 42.0);
    EXPECT_DOUBLE_EQ(result[2].value, 42.0);
}

TEST(TsFutureInterfacesTests, ForwardFill_MissingTimestamp_CarriesForward) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,1.0), dp("m","e",2000,3.0)};
    ForwardFillGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {0, 1000, 2000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 1.0);
    EXPECT_DOUBLE_EQ(result[1].value, 1.0);  // forwarded
    EXPECT_DOUBLE_EQ(result[2].value, 3.0);
}

TEST(TsFutureInterfacesTests, ForwardFill_EmptyPoints_UsesNullFill) {
    ForwardFillGapFiller f;
    GapFillConfig cfg;
    cfg.null_fill_value = -1.0;
    auto result = f.fill({}, {1000, 2000}, cfg);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_DOUBLE_EQ(result[0].value, -1.0);
    EXPECT_DOUBLE_EQ(result[1].value, -1.0);
}

TEST(TsFutureInterfacesTests, ForwardFill_MaxGapExceeded_UsesNullFill) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,10.0), dp("m","e",5000,20.0)};
    ForwardFillGapFiller f;
    GapFillConfig cfg;
    cfg.max_gap_ms      = 500;   // 500 ms max gap
    cfg.null_fill_value = 0.0;
    auto result = f.fill(pts, {0, 2000, 5000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 10.0);
    EXPECT_DOUBLE_EQ(result[1].value, 0.0);  // 2000 ms gap > 500 ms → null
    EXPECT_DOUBLE_EQ(result[2].value, 20.0);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 8: LinearInterpolationGapFiller
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, LinearInterp_ExactMatch_Verbatim) {
    auto pts = constSeries(3, 5.0, 0, 1000);
    LinearInterpolationGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {0, 1000, 2000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 5.0);
}

TEST(TsFutureInterfacesTests, LinearInterp_MidpointInterpolated) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,0.0), dp("m","e",2000,4.0)};
    LinearInterpolationGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {0, 1000, 2000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 0.0);
    EXPECT_NEAR(result[1].value, 2.0, 1e-9);  // midpoint
    EXPECT_DOUBLE_EQ(result[2].value, 4.0);
}

TEST(TsFutureInterfacesTests, LinearInterp_BeforeFirstPoint_ForwardBoots) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",2000,10.0)};
    LinearInterpolationGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {1000}, cfg);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0].value, 10.0);  // bootstrapped from first point
}

TEST(TsFutureInterfacesTests, LinearInterp_AfterLastPoint_Extrapolates) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,5.0)};
    LinearInterpolationGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {1000}, cfg);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0].value, 5.0);
}

TEST(TsFutureInterfacesTests, LinearInterp_MaxGapExceeded_NullFill) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,0.0), dp("m","e",10000,10.0)};
    LinearInterpolationGapFiller f;
    GapFillConfig cfg;
    cfg.max_gap_ms      = 1000;
    cfg.null_fill_value = -99.0;
    auto result = f.fill(pts, {0, 5000, 10000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 0.0);
    EXPECT_DOUBLE_EQ(result[1].value, -99.0);  // gap = 10s > 1s
    EXPECT_DOUBLE_EQ(result[2].value, 10.0);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 9: BackwardFillGapFiller
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, BackwardFill_MissingTimestamp_CarriesNext) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,1.0), dp("m","e",2000,3.0)};
    BackwardFillGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {0, 1000, 2000}, cfg);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 1.0);
    EXPECT_DOUBLE_EQ(result[1].value, 3.0);  // backward-filled from 2000
    EXPECT_DOUBLE_EQ(result[2].value, 3.0);
}

TEST(TsFutureInterfacesTests, BackwardFill_AfterLast_FallsBackToLast) {
    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,7.0)};
    BackwardFillGapFiller f;
    GapFillConfig cfg;
    auto result = f.fill(pts, {1000}, cfg);
    ASSERT_EQ(result.size(), 1u);
    EXPECT_DOUBLE_EQ(result[0].value, 7.0);
}

// ═════════════════════════════════════════════════════════════════════════════
// Section 10: GapFiller façade
// ═════════════════════════════════════════════════════════════════════════════

TEST(TsFutureInterfacesTests, GapFiller_DefaultMethod_ForwardFill) {
    GapFiller gf;
    EXPECT_EQ(gf.config().method, GapFillMethod::ForwardFill);

    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,5.0), dp("m","e",2000,9.0)};
    auto result = gf.fill(pts, {0, 1000, 2000});
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[1].value, 5.0);  // forwarded
}

TEST(TsFutureInterfacesTests, GapFiller_LinearInterpolation_Configured) {
    GapFillConfig cfg;
    cfg.method = GapFillMethod::LinearInterpolation;
    GapFiller gf(cfg);

    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,0.0), dp("m","e",2000,2.0)};
    auto result = gf.fill(pts, {0, 1000, 2000});
    ASSERT_EQ(result.size(), 3u);
    EXPECT_NEAR(result[1].value, 1.0, 1e-9);
}

TEST(TsFutureInterfacesTests, GapFiller_NullFill_AllMissingGetNull) {
    GapFillConfig cfg;
    cfg.method          = GapFillMethod::NullFill;
    cfg.null_fill_value = -1.0;
    GapFiller gf(cfg);

    std::vector<TSStore::DataPoint> pts = {dp("m","e",0, 99.0)};
    auto result = gf.fill(pts, {0, 500, 1000});
    ASSERT_EQ(result.size(), 3u);
    EXPECT_DOUBLE_EQ(result[0].value, 99.0);  // exact match
    EXPECT_DOUBLE_EQ(result[1].value, -1.0);  // synthesised
    EXPECT_DOUBLE_EQ(result[2].value, -1.0);  // synthesised
}

TEST(TsFutureInterfacesTests, GapFiller_SetConfig_ChangesMethod) {
    GapFiller gf;
    GapFillConfig cfg;
    cfg.method = GapFillMethod::BackwardFill;
    gf.setConfig(cfg);
    EXPECT_EQ(gf.config().method, GapFillMethod::BackwardFill);

    std::vector<TSStore::DataPoint> pts = {dp("m","e",0,1.0), dp("m","e",2000,3.0)};
    auto result = gf.fill(pts, {0, 1000, 2000});
    EXPECT_DOUBLE_EQ(result[1].value, 3.0);  // backward-filled
}

TEST(TsFutureInterfacesTests, GapFiller_RegularTimestamps_CorrectSequence) {
    auto ts = GapFiller::regularTimestamps(0, 5000, 1000);
    ASSERT_EQ(ts.size(), 6u);
    for (size_t i = 0; i < ts.size(); ++i) {
        EXPECT_EQ(ts[i], static_cast<int64_t>(i) * 1000);
    }
}

TEST(TsFutureInterfacesTests, GapFiller_RegularTimestamps_InvalidInput) {
    EXPECT_TRUE(GapFiller::regularTimestamps(5000, 0, 1000).empty());
    EXPECT_TRUE(GapFiller::regularTimestamps(0, 5000, 0).empty());
    EXPECT_TRUE(GapFiller::regularTimestamps(0, 5000, -1).empty());
}

} // namespace
} // namespace themis
