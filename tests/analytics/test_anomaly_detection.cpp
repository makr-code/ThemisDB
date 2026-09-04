/**
 * Anomaly Detection Engine unit + integration tests.
 *
 * Covers:
 *  - DataPoint helpers (numericFeatures, numericFieldNames, get, set)
 *  - AnomalyDetector – all six algorithms (Z_SCORE, MODIFIED_Z_SCORE, IQR,
 *      ISOLATION_FOREST, LOF, ENSEMBLE)
 *  - Training / isTrained guard
 *  - predict single point
 *  - predictBatch
 *  - explain (feature contributions, sorted order)
 *  - anomaly scoring thresholds (clear outliers always flagged)
 *  - adaptive learning via update()
 *  - serialize / deserialize round-trip (statistics preserved)
 *  - getStats
 *  - StreamingAnomalyDetector warm-up, process, getAnomalies, clearAnomalies,
 *      getWindowStats
 *  - anomalyMethodName helper
 *  - error handling (train on empty, predict before train)
 */

#include <gtest/gtest.h>
#include "analytics/anomaly_detection.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <thread>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

/// Build n normal points for training: features follow N(mean_i, 0.1).
static std::vector<DataPoint> makeNormalData(int n, int n_features = 2,
                                              double mean = 0.0, double noise = 0.1) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id           = "n" + std::to_string(i);
        p.timestamp_ms = i * 1000LL;
        for (int f = 0; f < n_features; ++f) {
            // Simple deterministic pseudo-noise: sine wave within [-noise, noise]
            double v = mean + noise * std::sin(static_cast<double>(i * 17 + f * 31));
            p.set("f" + std::to_string(f), v);
        }
        data.push_back(std::move(p));
    }
    return data;
}

/// Build a clear outlier far from the training distribution.
static DataPoint makeOutlier(int n_features = 2, double offset = 100.0) {
    DataPoint p;
    p.id = "outlier";
    p.timestamp_ms = 999999LL;
    for (int f = 0; f < n_features; ++f)
        p.set("f" + std::to_string(f), offset);
    return p;
}

/// Build a normal-looking point (within 2σ of training mean).
static DataPoint makeNormalPoint(int n_features = 2, double mean = 0.0) {
    DataPoint p;
    p.id = "normal";
    p.timestamp_ms = 0LL;
    for (int f = 0; f < n_features; ++f)
        p.set("f" + std::to_string(f), mean + 0.05);
    return p;
}

// ============================================================================
// DataPoint helpers
// ============================================================================

TEST(DataPointTest, NumericFieldNames) {
    DataPoint p;
    p.set("x", 1.0);
    p.set("y", int64_t(2));
    p.set("z", std::string("hello"));
    p.set("b", true);
    auto names = p.numericFieldNames();
    // x, y, b should be numeric (bool counts); z should not
    EXPECT_EQ(names.size(), 3u);
    EXPECT_NE(std::find(names.begin(), names.end(), "x"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "y"), names.end());
    EXPECT_NE(std::find(names.begin(), names.end(), "b"), names.end());
    EXPECT_EQ(std::find(names.begin(), names.end(), "z"), names.end());
}

TEST(DataPointTest, NumericFeatures) {
    DataPoint p;
    p.set("a", 3.0);
    p.set("b", int64_t(7));
    p.set("c", true);
    p.set("d", std::string("skip"));
    auto feats = p.numericFeatures();
    EXPECT_EQ(feats.size(), 3u);
}

TEST(DataPointTest, GetHelper) {
    DataPoint p;
    p.set("d", 1.5);
    p.set("i", int64_t(42));
    p.set("s", std::string("str"));
    EXPECT_DOUBLE_EQ(*p.get<double>("d"), 1.5);
    EXPECT_EQ(*p.get<int64_t>("i"), 42);
    EXPECT_EQ(*p.get<std::string>("s"), "str");
    EXPECT_FALSE(p.get<double>("missing").has_value());
}

TEST(DataPointTest, FieldOrderIsDeterministic) {
    DataPoint p;
    p.set("z", 3.0);
    p.set("a", 1.0);
    p.set("m", 2.0);
    auto names = p.numericFieldNames();
    // std::map → sorted by key name
    EXPECT_EQ(names[0], "a");
    EXPECT_EQ(names[1], "m");
    EXPECT_EQ(names[2], "z");
}

// ============================================================================
// AnomalyDetector – guard conditions
// ============================================================================

TEST(AnomalyDetectorGuardTest, TrainOnEmptyThrows) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    EXPECT_THROW(det.train({}), std::invalid_argument);
}

TEST(AnomalyDetectorGuardTest, PredictBeforeTrainThrows) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    DataPoint p = makeNormalPoint();
    EXPECT_THROW(det.predict(p), std::runtime_error);
}

TEST(AnomalyDetectorGuardTest, ExplainBeforeTrainThrows) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    EXPECT_THROW(det.explain(makeNormalPoint()), std::runtime_error);
}

TEST(AnomalyDetectorGuardTest, IsTrainedFalseInitially) {
    AnomalyDetector det;
    EXPECT_FALSE(det.isTrained());
}

TEST(AnomalyDetectorGuardTest, IsTrainedTrueAfterTrain) {
    AnomalyDetector det;
    auto data = makeNormalData(50);
    det.train(data);
    EXPECT_TRUE(det.isTrained());
}

TEST(AnomalyDetectorGuardTest, UpdateWithoutAdaptiveThrows) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    auto data = makeNormalData(50);
    det.train(data);
    EXPECT_THROW(det.update(makeNormalPoint()), std::runtime_error);
}

// ============================================================================
// Z_SCORE
// ============================================================================

class ZScoreTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto data = makeNormalData(200, 2, 0.0, 0.1);
        det_.train(data);
    }
    AnomalyDetector det_{AnomalyMethod::Z_SCORE};
};

TEST_F(ZScoreTest, NormalPointLowScore) {
    auto result = det_.predict(makeNormalPoint());
    EXPECT_LT(result.score, 0.5);
    EXPECT_FALSE(result.is_anomaly);
}

TEST_F(ZScoreTest, OutlierHighScore) {
    auto result = det_.predict(makeOutlier(2, 50.0));
    EXPECT_GT(result.score, 0.5);
    EXPECT_TRUE(result.is_anomaly);
}

TEST_F(ZScoreTest, ResultHasCorrectMethod) {
    auto result = det_.predict(makeNormalPoint());
    EXPECT_EQ(result.method, AnomalyMethod::Z_SCORE);
}

TEST_F(ZScoreTest, ResultIdMatchesInput) {
    DataPoint p = makeNormalPoint();
    p.id = "test-id-42";
    EXPECT_EQ(det_.predict(p).id, "test-id-42");
}

TEST_F(ZScoreTest, BatchResultsMatchSingle) {
    auto data = makeNormalData(20);
    auto batch = det_.predictBatch(data);
    EXPECT_EQ(batch.size(), 20u);
    for (size_t i = 0; i < batch.size(); ++i)
        EXPECT_DOUBLE_EQ(batch[i].score, det_.predict(data[i]).score);
}

// ============================================================================
// MODIFIED_Z_SCORE
// ============================================================================

TEST(ModifiedZScoreTest, OutlierDetected) {
    AnomalyDetector det(AnomalyMethod::MODIFIED_Z_SCORE);
    det.train(makeNormalData(200, 2, 0.0, 0.1));
    EXPECT_TRUE(det.predict(makeOutlier(2, 50.0)).is_anomaly);
}

TEST(ModifiedZScoreTest, NormalNotFlagged) {
    AnomalyDetector det(AnomalyMethod::MODIFIED_Z_SCORE);
    det.train(makeNormalData(200, 2, 0.0, 0.1));
    EXPECT_FALSE(det.predict(makeNormalPoint(2, 0.0)).is_anomaly);
}

// ============================================================================
// IQR
// ============================================================================

TEST(IQRTest, OutlierDetected) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(300, 2, 0.0, 0.1));
    EXPECT_TRUE(det.predict(makeOutlier(2, 100.0)).is_anomaly);
}

TEST(IQRTest, NormalNotFlagged) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(300, 2, 0.0, 0.1));
    EXPECT_FALSE(det.predict(makeNormalPoint(2, 0.0)).is_anomaly);
}

TEST(IQRTest, ScoreIsInUnitInterval) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(300));
    auto result = det.predict(makeNormalPoint());
    EXPECT_GE(result.score, 0.0);
    EXPECT_LE(result.score, 1.0);
}

// ============================================================================
// ISOLATION_FOREST
// ============================================================================

// Helper: normal training data + outliers included with given contamination rate.
// Isolation Forest requires outliers in the training subsamples to isolate them.
static std::vector<DataPoint> makeContaminatedData(
        int n_normal, int n_outlier, int n_features = 2,
        double normal_mean = 0.0, double normal_noise = 0.1,
        double outlier_offset = 50.0) {
    auto data = makeNormalData(n_normal, n_features, normal_mean, normal_noise);
    for (int i = 0; i < n_outlier; ++i) {
        DataPoint p;
        p.id = "co" + std::to_string(i);
        for (int f = 0; f < n_features; ++f)
            p.set("f" + std::to_string(f), outlier_offset);
        data.push_back(p);
    }
    return data;
}

TEST(IsolationForestTest, OutlierDetected) {
    DetectorConfig cfg;
    cfg.method       = AnomalyMethod::ISOLATION_FOREST;
    cfg.n_estimators = 50;
    cfg.max_samples  = 64;
    cfg.threshold    = 0.55;
    AnomalyDetector det(cfg);
    // 10% outliers in training so subsamples see the outlier cluster and isolate it quickly
    det.train(makeContaminatedData(180, 20, 2, 0.0, 0.1, 50.0));
    EXPECT_TRUE(det.predict(makeOutlier(2, 50.0)).is_anomaly);
}

TEST(IsolationForestTest, NormalPointLowerScore) {
    DetectorConfig cfg;
    cfg.method       = AnomalyMethod::ISOLATION_FOREST;
    cfg.n_estimators = 50;
    cfg.max_samples  = 64;
    AnomalyDetector det(cfg);
    det.train(makeContaminatedData(180, 20, 2, 0.0, 0.1, 50.0));
    auto normal  = det.predict(makeNormalPoint()).score;
    auto outlier = det.predict(makeOutlier(2, 50.0)).score;
    EXPECT_LT(normal, outlier);
}

TEST(IsolationForestTest, ScoreBoundedUnitInterval) {
    DetectorConfig cfg;
    cfg.method       = AnomalyMethod::ISOLATION_FOREST;
    cfg.n_estimators = 20;
    cfg.max_samples  = 32;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(100));
    auto r = det.predict(makeOutlier());
    EXPECT_GE(r.score, 0.0);
    EXPECT_LE(r.score, 1.0);
}

// ============================================================================
// LOF
// ============================================================================

TEST(LOFTest, OutlierDetected) {
    DetectorConfig cfg;
    cfg.method      = AnomalyMethod::LOF;
    cfg.k_neighbors = 5;
    cfg.threshold   = 0.5;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(100, 2, 0.0, 0.1));
    EXPECT_TRUE(det.predict(makeOutlier(2, 50.0)).is_anomaly);
}

TEST(LOFTest, ScoreBoundedUnitInterval) {
    DetectorConfig cfg;
    cfg.method      = AnomalyMethod::LOF;
    cfg.k_neighbors = 3;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(80));
    auto r = det.predict(makeNormalPoint());
    EXPECT_GE(r.score, 0.0);
    EXPECT_LE(r.score, 1.0);
}

// ============================================================================
// ENSEMBLE
// ============================================================================

TEST(EnsembleTest, OutlierDetected) {
    DetectorConfig cfg;
    cfg.method    = AnomalyMethod::ENSEMBLE;
    cfg.threshold = 0.5;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200, 2, 0.0, 0.1));
    EXPECT_TRUE(det.predict(makeOutlier(2, 50.0)).is_anomaly);
}

TEST(EnsembleTest, CustomMethodsAndWeights) {
    DetectorConfig cfg;
    cfg.method           = AnomalyMethod::ENSEMBLE;
    cfg.ensemble_methods = {AnomalyMethod::Z_SCORE, AnomalyMethod::IQR};
    cfg.ensemble_weights = {0.7, 0.3};
    cfg.threshold        = 0.5;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200));
    auto r = det.predict(makeOutlier(2, 50.0));
    EXPECT_GE(r.score, 0.0);
    EXPECT_LE(r.score, 1.0);
}

TEST(EnsembleTest, ScoreBoundedUnitInterval) {
    DetectorConfig cfg;
    cfg.method = AnomalyMethod::ENSEMBLE;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200));
    for (auto& p : makeNormalData(20)) {
        auto r = det.predict(p);
        EXPECT_GE(r.score, 0.0);
        EXPECT_LE(r.score, 1.0);
    }
}

// ============================================================================
// explain
// ============================================================================

TEST(ExplainTest, ReturnsContributions) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(100, 3));
    auto exp = det.explain(makeOutlier(3, 50.0));
    EXPECT_FALSE(exp.feature_contributions.empty());
    EXPECT_EQ(exp.feature_contributions.size(), 3u);
}

TEST(ExplainTest, ContributionsSortedDescending) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(100, 4));
    auto exp = det.explain(makeOutlier(4, 50.0));
    for (size_t i = 1; i < exp.feature_contributions.size(); ++i) {
        EXPECT_GE(exp.feature_contributions[i-1].second,
                  exp.feature_contributions[i].second);
    }
}

TEST(ExplainTest, DescriptionNonEmpty) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(100));
    auto exp = det.explain(makeOutlier(2, 10.0));
    EXPECT_FALSE(exp.description.empty());
}

TEST(ExplainTest, ScoreMatchesPredict) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(100));
    auto p   = makeOutlier(2, 20.0);
    auto exp = det.explain(p);
    auto r   = det.predict(p);
    EXPECT_DOUBLE_EQ(exp.score, r.score);
}

TEST(ExplainTest, IsolationForestContributionsNonZeroForOutlier) {
    DetectorConfig cfg;
    cfg.method       = AnomalyMethod::ISOLATION_FOREST;
    cfg.n_estimators = 50;
    cfg.max_samples  = 64;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200, 3, 0.0, 0.1));
    auto exp = det.explain(makeOutlier(3, 50.0));
    EXPECT_EQ(exp.feature_contributions.size(), 3u);
    // At least one feature should have a non-zero contribution
    bool any_nonzero = false;
    for (const auto& [name, c] : exp.feature_contributions)
        if (c > 0.0) {
          any_nonzero = true;
        }
    EXPECT_TRUE(any_nonzero);
}

TEST(ExplainTest, IsolationForestContributionsSortedDescending) {
    DetectorConfig cfg;
    cfg.method       = AnomalyMethod::ISOLATION_FOREST;
    cfg.n_estimators = 50;
    cfg.max_samples  = 64;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200, 3, 0.0, 0.1));
    auto exp = det.explain(makeOutlier(3, 50.0));
    for (size_t i = 1; i < exp.feature_contributions.size(); ++i)
        EXPECT_GE(exp.feature_contributions[i-1].second,
                  exp.feature_contributions[i].second);
}

TEST(ExplainTest, LOFContributionsNonZeroForOutlier) {
    DetectorConfig cfg;
    cfg.method      = AnomalyMethod::LOF;
    cfg.k_neighbors = 5;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(100, 2, 0.0, 0.1));
    auto exp = det.explain(makeOutlier(2, 50.0));
    EXPECT_EQ(exp.feature_contributions.size(), 2u);
    bool any_nonzero = false;
    for (const auto& [name, c] : exp.feature_contributions)
        if (c > 0.0) {
          any_nonzero = true;
        }
    EXPECT_TRUE(any_nonzero);
}

TEST(ExplainTest, LOFContributionsSortedDescending) {
    DetectorConfig cfg;
    cfg.method      = AnomalyMethod::LOF;
    cfg.k_neighbors = 5;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(100, 2, 0.0, 0.1));
    auto exp = det.explain(makeOutlier(2, 50.0));
    for (size_t i = 1; i < exp.feature_contributions.size(); ++i)
        EXPECT_GE(exp.feature_contributions[i-1].second,
                  exp.feature_contributions[i].second);
}

TEST(ExplainTest, EnsembleContributionsNonEmpty) {
    DetectorConfig cfg;
    cfg.method    = AnomalyMethod::ENSEMBLE;
    cfg.threshold = 0.5;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200, 2, 0.0, 0.1));
    auto exp = det.explain(makeOutlier(2, 50.0));
    EXPECT_EQ(exp.feature_contributions.size(), 2u);
}

// ============================================================================
// getStats training_samples
// ============================================================================

TEST(StatsTest, TrainingSamplesCountIsAccurate) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    auto data = makeNormalData(75, 2);
    det.train(data);
    EXPECT_EQ(det.getStats().training_samples, 75u);
}

TEST(StatsTest, TrainingSamplesCountIsAccurateForLOF) {
    DetectorConfig cfg;
    cfg.method = AnomalyMethod::LOF;
    AnomalyDetector det(cfg);
    auto data = makeNormalData(60, 2);
    det.train(data);
    EXPECT_EQ(det.getStats().training_samples, 60u);
}

TEST(StatsTest, TrainingSamplesCountIsAccurateForIsolationForest) {
    DetectorConfig cfg;
    cfg.method       = AnomalyMethod::ISOLATION_FOREST;
    cfg.n_estimators = 10;
    cfg.max_samples  = 32;
    AnomalyDetector det(cfg);
    auto data = makeNormalData(50, 2);
    det.train(data);
    EXPECT_EQ(det.getStats().training_samples, 50u);
}

// ============================================================================
// Adaptive learning (update)
// ============================================================================

TEST(AdaptiveTest, UpdateAfterAdaptiveEnabled) {
    DetectorConfig cfg;
    cfg.method   = AnomalyMethod::Z_SCORE;
    cfg.adaptive = true;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(100));
    // Should not throw
    EXPECT_NO_THROW(det.update(makeNormalPoint()));
    EXPECT_TRUE(det.isTrained());  // still trained after update
}

TEST(AdaptiveTest, UpdateChangesModel) {
    DetectorConfig cfg;
    cfg.method   = AnomalyMethod::Z_SCORE;
    cfg.adaptive = true;
    AnomalyDetector det(cfg);
    det.train(makeNormalData(100, 1, 0.0, 0.1));

    // Inject points at mean = 50 → model should shift
    for (int i = 0; i < 200; ++i) {
        DataPoint p;
        p.id = "shift_" + std::to_string(i);
        p.set("f0", 50.0 + 0.01 * i);
        det.update(p);
    }

    // After shifting, a point near 50 should look normal
    DataPoint near50;
    near50.set("f0", 50.0);
    auto r = det.predict(near50);
    EXPECT_LT(r.score, 0.8);  // should not be a strong outlier anymore
}

// ============================================================================
// serialize / deserialize
// ============================================================================

TEST(SerializeTest, RoundTripPreservesStats) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    auto data = makeNormalData(100, 2);
    det.train(data);

    std::string serial = det.serialize();
    EXPECT_FALSE(serial.empty());

    auto det2 = AnomalyDetector::deserialize(serial);
    EXPECT_TRUE(det2.isTrained());

    auto stats1 = det.getStats();
    auto stats2 = det2.getStats();
    EXPECT_EQ(stats1.feature_names, stats2.feature_names);
    ASSERT_EQ(stats1.feature_means.size(), stats2.feature_means.size());
    for (size_t i = 0; i < stats1.feature_means.size(); ++i)
        EXPECT_NEAR(stats1.feature_means[i], stats2.feature_means[i], 1e-9);
}

TEST(SerializeTest, DeserializedDetectorPredictsConsistently) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(150, 2));

    auto det2 = AnomalyDetector::deserialize(det.serialize());
    auto p = makeNormalPoint(2, 0.0);
    EXPECT_DOUBLE_EQ(det.predict(p).score, det2.predict(p).score);
}

// ============================================================================
// getStats
// ============================================================================

TEST(StatsTest, FeatureNamesPresent) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(50, 3));
    auto stats = det.getStats();
    EXPECT_EQ(stats.n_features, 3u);
    EXPECT_EQ(stats.feature_names.size(), 3u);
}

TEST(StatsTest, StddevsNonNegative) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(50, 2));
    auto stats = det.getStats();
    for (double sd : stats.feature_stddevs) {
      EXPECT_GE(sd, 0.0);
    }
}

// ============================================================================
// predictBatch
// ============================================================================

TEST(BatchTest, SizeMatchesInput) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(100));
    auto results = det.predictBatch(makeNormalData(30));
    EXPECT_EQ(results.size(), 30u);
}

TEST(BatchTest, EmptyBatchReturnsEmpty) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(100));
    EXPECT_TRUE(det.predictBatch({}).empty());
}

// ============================================================================
// Threshold control
// ============================================================================

TEST(ThresholdTest, HighThresholdSuppressesAnomaly) {
    DetectorConfig cfg;
    cfg.method    = AnomalyMethod::Z_SCORE;
    cfg.threshold = 0.9999;  // extremely high → nothing is anomaly
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200));
    // offset=0.5: z-score ≈ 7 → squash ≈ 0.98 < 0.9999 → not flagged
    EXPECT_FALSE(det.predict(makeOutlier(2, 0.5)).is_anomaly);
}

TEST(ThresholdTest, LowThresholdMakesEverythingAnomaly) {
    DetectorConfig cfg;
    cfg.method    = AnomalyMethod::Z_SCORE;
    cfg.threshold = 0.0001;  // extremely low → everything is anomaly
    AnomalyDetector det(cfg);
    det.train(makeNormalData(200));
    EXPECT_TRUE(det.predict(makeNormalPoint()).is_anomaly);
}

// ============================================================================
// StreamingAnomalyDetector
// ============================================================================

class StreamingTest : public ::testing::Test {
protected:
    StreamingAnomalyDetector::Config cfg_;
    void SetUp() override {
        cfg_.method           = AnomalyMethod::Z_SCORE;
        cfg_.threshold        = 0.6;
        cfg_.window_size      = 200;
        cfg_.auto_train       = true;
        cfg_.auto_train_after = 50;
        cfg_.retrain_on_window = false;
    }
};

TEST_F(StreamingTest, ReturnsNulloptDuringWarmup) {
    StreamingAnomalyDetector sad(cfg_);
    auto p = makeNormalPoint();
    // First few points: no result yet
    auto r = sad.process(p);
    EXPECT_FALSE(r.has_value());
}

TEST_F(StreamingTest, ReturnsResultAfterWarmup) {
    StreamingAnomalyDetector sad(cfg_);
    auto data = makeNormalData(60);
    std::optional<AnomalyResult> last = {};

    for (auto& p : data) {
      last = sad.process(p);
    }
    // After 60 points (> auto_train_after=50) should be producing results
    EXPECT_TRUE(last.has_value());
}

TEST_F(StreamingTest, DetectsOutlierAfterWarmup) {
    StreamingAnomalyDetector sad(cfg_);
    // Warm up
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }

    // Inject clear outlier
    auto outlier = makeOutlier(2, 50.0);
    auto result  = sad.process(outlier);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->is_anomaly);
}

TEST_F(StreamingTest, NormalPointNotFlaggedAfterWarmup) {
    StreamingAnomalyDetector sad(cfg_);
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }
    auto result = sad.process(makeNormalPoint(2, 0.0));
    ASSERT_TRUE(result.has_value());
    EXPECT_FALSE(result->is_anomaly);
}

TEST_F(StreamingTest, GetAnomaliesReturnsStoredAnomalies) {
    StreamingAnomalyDetector sad(cfg_);
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }
    sad.process(makeOutlier(2, 50.0));
    EXPECT_GE(sad.getAnomalies().size(), 1u);
}

TEST_F(StreamingTest, ClearAnomaliesResetsHistory) {
    StreamingAnomalyDetector sad(cfg_);
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }
    sad.process(makeOutlier(2, 50.0));
    sad.clearAnomalies();
    EXPECT_TRUE(sad.getAnomalies().empty());
}

TEST_F(StreamingTest, WindowStatsTrainedFlagSetAfterWarmup) {
    StreamingAnomalyDetector sad(cfg_);
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }
    EXPECT_TRUE(sad.getWindowStats().trained);
}

TEST_F(StreamingTest, WindowStatsSizeBoundedByConfig) {
    StreamingAnomalyDetector sad(cfg_);
    for (auto& p : makeNormalData(300)) {
      sad.process(p);
    }
    EXPECT_LE(sad.getWindowStats().window_size, cfg_.window_size);
}

TEST_F(StreamingTest, AnomalyRateIsNonNegative) {
    StreamingAnomalyDetector sad(cfg_);
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }
    EXPECT_GE(sad.getWindowStats().anomaly_rate, 0.0);
}

TEST_F(StreamingTest, ThreadSafety) {
    StreamingAnomalyDetector sad(cfg_);
    // Warm up first
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }

    std::vector<std::thread> threads = {};

    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([&, t] {
            auto data = makeNormalData(20, 2, 0.0 + t * 0.01);
            for (auto& p : data) {
              sad.process(p);
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }
    // No crash and stats are coherent
    auto stats = sad.getWindowStats();
    EXPECT_LE(stats.window_size, cfg_.window_size);
}

// ============================================================================
// anomalyMethodName helper
// ============================================================================

TEST(HelperTest, MethodNameCoversAllEnumValues) {
    EXPECT_STREQ(anomalyMethodName(AnomalyMethod::Z_SCORE),          "Z_SCORE");
    EXPECT_STREQ(anomalyMethodName(AnomalyMethod::MODIFIED_Z_SCORE), "MODIFIED_Z_SCORE");
    EXPECT_STREQ(anomalyMethodName(AnomalyMethod::IQR),              "IQR");
    EXPECT_STREQ(anomalyMethodName(AnomalyMethod::ISOLATION_FOREST), "ISOLATION_FOREST");
    EXPECT_STREQ(anomalyMethodName(AnomalyMethod::LOF),              "LOF");
    EXPECT_STREQ(anomalyMethodName(AnomalyMethod::ENSEMBLE),         "ENSEMBLE");
}

// ============================================================================
// Multi-feature correctness (3+ features)
// ============================================================================

TEST(MultiFeatureTest, ZScoreOutlierOnSingleFeature) {
    // Three features; only f2 is anomalous
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    auto train = makeNormalData(200, 3, 0.0, 0.1);
    det.train(train);

    DataPoint p;
    p.id = "partial_outlier";
    p.set("f0", 0.0);   // normal
    p.set("f1", 0.0);   // normal
    p.set("f2", 50.0);  // outlier

    auto r = det.predict(p);
    EXPECT_TRUE(r.is_anomaly);

    auto exp = det.explain(p);
    // f2 should have the highest contribution
    EXPECT_EQ(exp.feature_contributions[0].first, "f2");
}

// ============================================================================
// Move semantics
// ============================================================================

TEST(MoveTest, MoveConstructedDetectorWorks) {
    AnomalyDetector det(AnomalyMethod::Z_SCORE);
    det.train(makeNormalData(100));
    AnomalyDetector moved(std::move(det));
    EXPECT_TRUE(moved.isTrained());
    EXPECT_NO_THROW(moved.predict(makeNormalPoint()));
}

TEST(MoveTest, MoveAssignedDetectorWorks) {
    AnomalyDetector det(AnomalyMethod::IQR);
    det.train(makeNormalData(100));
    AnomalyDetector other(AnomalyMethod::Z_SCORE);
    other = std::move(det);
    EXPECT_TRUE(other.isTrained());
}

// ============================================================================
// Concurrency stress test — 8 producer threads, P99 latency ≤ 1 ms
// Gated on THEMIS_RUN_PERF_TESTS=1 to avoid CI flakiness.
// ============================================================================

TEST(StreamingConcurrencyStress, EightProducersP99Latency) {
    const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!env || std::string(env) != "1") {
        GTEST_SKIP() << "Skipping performance/stress test; set THEMIS_RUN_PERF_TESTS=1 to run";
    }

    StreamingAnomalyDetector::Config cfg;
    cfg.method            = AnomalyMethod::Z_SCORE;
    cfg.threshold         = 0.6;
    cfg.window_size       = 200;
    cfg.auto_train        = true;
    cfg.auto_train_after  = 50;
    cfg.retrain_on_window = true;

    StreamingAnomalyDetector sad(cfg);

    // Warm up synchronously so all threads start with a trained model.
    for (auto& p : makeNormalData(60)) {
      sad.process(p);
    }

    constexpr int kThreads       = 8;
    constexpr int kPointsPerThread = 500;   // total 4 000 calls

    std::vector<std::vector<int64_t>> per_thread_latencies(kThreads);
    for (auto& v : per_thread_latencies)
        v.reserve(kPointsPerThread);

    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&, t] {
            auto data = makeNormalData(kPointsPerThread, 2, t * 0.01);
            auto& lats = per_thread_latencies[t];
            for (auto& p : data) {
                auto t0 = std::chrono::steady_clock::now();
                sad.process(p);
                auto t1 = std::chrono::steady_clock::now();
                lats.push_back(
                    std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count());
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    // Collect all latencies and compute P99.
    std::vector<int64_t> all_latencies;
    all_latencies.reserve(kThreads * kPointsPerThread);
    for (auto& v : per_thread_latencies)
        all_latencies.insert(all_latencies.end(), v.begin(), v.end());

    std::sort(all_latencies.begin(), all_latencies.end());
    const size_t p99_idx = static_cast<size_t>(
        std::ceil(static_cast<double>(all_latencies.size() - 1) * 0.99));
    const int64_t p99_us = all_latencies[p99_idx];

    EXPECT_LE(p99_us, 1000)   // P99 ≤ 1 ms = 1 000 µs
        << "P99 latency " << p99_us << " µs exceeds 1 ms threshold";

    // Sanity: detector must still be trained and window bounded.
    auto stats = sad.getWindowStats();
    EXPECT_TRUE(stats.trained);
    EXPECT_LE(stats.window_size, cfg.window_size);
}
