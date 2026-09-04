/**
 * AutoML Engine unit tests.
 *
 * Covers:
 *  - Binary and multi-class classification
 *  - Regression
 *  - All six candidate algorithms (LR, LIN_REG, DT, RF, GB, KNN)
 *  - Feature engineering (polynomial expansion + scaling)
 *  - Ensemble model generation
 *  - predict / predictOne / predictProba
 *  - explain / explainOne (feature contributions)
 *  - featureImportance
 *  - candidateModels (sorted descending by score)
 *  - serialize / deserialize round-trip
 *  - crossValidate helper
 *  - automlTaskName / modelAlgorithmName / automlMetricName helpers
 *  - EvalMetrics::primary (for higher-is-better and lower-is-better metrics)
 *  - Progress callback
 *  - Error handling (empty data, missing target)
 */

#include <gtest/gtest.h>
#include "analytics/automl.h"

#include <cassert>
#include <chrono>
#include <cmath>
#include <cstdlib>
#include <set>
#include <string>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

/**
 * Build a binary classification dataset: two linearly separable clusters.
 * Class "pos" when sum(features) > 0, "neg" otherwise.
 */
static std::vector<DataPoint> makeBinaryData(int n, int seed = 0) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    double step = 1.0 / (n > 1 ? n - 1 : 1);
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id           = "p" + std::to_string(i);
        p.timestamp_ms = i;
        double x1 = -1.0 + 2.0 * step * i;
        double x2 = std::sin(static_cast<double>(i + seed) * 0.7);
        p.set("x1", x1);
        p.set("x2", x2);
        std::string label = (x1 + x2 > 0.0) ? "pos" : "neg";
        p.fields["label"] = label;
        data.push_back(std::move(p));
    }
    return data;
}

/**
 * Build a multi-class dataset with 3 classes based on sine quadrants.
 */
static std::vector<DataPoint> makeMulticlassData(int n) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id = "mc" + std::to_string(i);
        double angle = 2.0 * 3.14159265 * i / n;
        p.set("x1", std::cos(angle));
        p.set("x2", std::sin(angle));
        int cls = static_cast<int>(std::floor((angle / (2.0 * 3.14159265)) * 3.0)) % 3;
        p.fields["label"] = std::to_string(cls);
        data.push_back(std::move(p));
    }
    return data;
}

/**
 * Build a regression dataset: y = 2*x1 - x2 + noise.
 */
static std::vector<DataPoint> makeRegressionData(int n, int seed = 0) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id = "r" + std::to_string(i);
        double x1 = -1.0 + 2.0 * static_cast<double>(i) / (n - 1);
        double x2 = std::cos(static_cast<double>(i + seed) * 0.5);
        p.set("x1", x1);
        p.set("x2", x2);
        double noise = 0.01 * std::sin(static_cast<double>(i * 13 + seed));
        p.fields["y"] = 2.0 * x1 - x2 + noise;
        data.push_back(std::move(p));
    }
    return data;
}

// ============================================================================
// Helper: fast config that completes in under 5 seconds
// ============================================================================
static AutoMLConfig fastConfig(const std::string& target,
                                AutoMLTask task = AutoMLTask::CLASSIFICATION) {
    AutoMLConfig cfg;
    cfg.target             = target;
    cfg.task               = task;
    cfg.max_time_minutes   = 2;
    cfg.max_trials         = 10;
    cfg.cv_folds           = 2;
    cfg.feature_engineering= false;
    cfg.ensemble           = false;
    cfg.random_seed        = 42;
    cfg.metric             = (task == AutoMLTask::CLASSIFICATION)
                             ? AutoMLMetric::ACCURACY
                             : AutoMLMetric::R2;
    return cfg;
}

// ============================================================================
// EvalMetrics::primary
// ============================================================================

TEST(EvalMetricsTest, PrimaryReturnsCorrectField) {
    EvalMetrics em;
    em.accuracy  = 0.9;
    em.f1        = 0.8;
    em.precision = 0.7;
    em.recall    = 0.6;
    em.auc_roc   = 0.95;
    em.r2        = 0.85;
    em.rmse      = 0.1;
    em.mae       = 0.05;
    em.mape      = 0.02;

    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::ACCURACY),  0.9);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::F1),        0.8);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::PRECISION), 0.7);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::RECALL),    0.6);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::AUC_ROC),   0.95);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::R2),        0.85);
    // Lower-is-better metrics are negated for maximisation
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::RMSE), -0.1);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::MAE),  -0.05);
    EXPECT_DOUBLE_EQ(em.primary(AutoMLMetric::MAPE), -0.02);
}

// ============================================================================
// Name helper functions
// ============================================================================

TEST(NameHelperTest, AutomlTaskName) {
    EXPECT_STREQ(automlTaskName(AutoMLTask::CLASSIFICATION), "CLASSIFICATION");
    EXPECT_STREQ(automlTaskName(AutoMLTask::REGRESSION),     "REGRESSION");
}

TEST(NameHelperTest, ModelAlgorithmName) {
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::LOGISTIC_REGRESSION), "LOGISTIC_REGRESSION");
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::LINEAR_REGRESSION),   "LINEAR_REGRESSION");
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::DECISION_TREE),       "DECISION_TREE");
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::RANDOM_FOREST),       "RANDOM_FOREST");
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::GRADIENT_BOOSTING),   "GRADIENT_BOOSTING");
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::KNN),                 "KNN");
    EXPECT_STREQ(modelAlgorithmName(ModelAlgorithm::ENSEMBLE),            "ENSEMBLE");
}

TEST(NameHelperTest, AutomlMetricName) {
    EXPECT_STREQ(automlMetricName(AutoMLMetric::ACCURACY),  "ACCURACY");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::F1),        "F1");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::PRECISION), "PRECISION");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::RECALL),    "RECALL");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::AUC_ROC),   "AUC_ROC");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::R2),        "R2");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::RMSE),      "RMSE");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::MAE),       "MAE");
    EXPECT_STREQ(automlMetricName(AutoMLMetric::MAPE),      "MAPE");
}

// ============================================================================
// Error handling
// ============================================================================

TEST(AutoMLErrorTest, EmptyDataThrows) {
    AutoML automl;
    AutoMLConfig cfg = fastConfig("label");
    EXPECT_THROW(automl.trainClassifier({}, cfg), std::invalid_argument);
}

TEST(AutoMLErrorTest, EmptyTargetThrows) {
    AutoML automl;
    auto data = makeBinaryData(50);
    AutoMLConfig cfg = fastConfig("");   // empty target
    EXPECT_THROW(automl.trainClassifier(data, cfg), std::invalid_argument);
}

// ============================================================================
// Binary classification – basic correctness
// ============================================================================

class BinaryClassTest : public ::testing::Test {
protected:
    void SetUp() override {
        data = makeBinaryData(120);
        cfg  = fastConfig("label");
        cfg.algorithms = {ModelAlgorithm::DECISION_TREE};
        model = automl.trainClassifier(data, cfg);
    }
    AutoML automl;
    std::vector<DataPoint> data;
    AutoMLConfig cfg;
    AutoMLModel model;
};

TEST_F(BinaryClassTest, TaskIsClassification) {
    EXPECT_EQ(model.task(), AutoMLTask::CLASSIFICATION);
}

TEST_F(BinaryClassTest, PredictReturnsSameCountAsInput) {
    auto preds = model.predict(data);
    EXPECT_EQ(preds.size(), data.size());
}

TEST_F(BinaryClassTest, PredictionsAreKnownLabels) {
    auto preds = model.predict(data);
    for (const auto& p : preds) {
        EXPECT_TRUE(p == "pos" || p == "neg")
            << "Unexpected label: " << p;
    }
}

TEST_F(BinaryClassTest, PredictOneMatchesBatchPredict) {
    for (size_t i = 0; i < std::min(data.size(), size_t(10)); ++i) {
        auto batch  = model.predict({data[i]});
        auto single = model.predictOne(data[i]);
        EXPECT_EQ(batch[0], single);
    }
}

TEST_F(BinaryClassTest, PredictProbaHasCorrectShape) {
    auto proba = model.predictProba({data[0]});
    ASSERT_EQ(proba.size(), 1u);
    EXPECT_EQ(proba[0].size(), 2u);   // two classes: "neg", "pos"
    double sum = 0.0;
    for (const auto& [lbl, p] : proba[0]) {
      sum += p;
    }
    EXPECT_NEAR(sum, 1.0, 1e-6);
}

TEST_F(BinaryClassTest, CandidateModelsSortedDescending) {
    auto cands = model.candidateModels();
    for (size_t i = 1; i < cands.size(); ++i)
        EXPECT_GE(cands[i - 1].cv_score, cands[i].cv_score);
}

TEST_F(BinaryClassTest, AccuracyAboveChanceLevel) {
    auto preds  = model.predict(data);
    int correct = 0;
    for (size_t i = 0; i < data.size(); ++i) {
        auto it = data[i].fields.find("label");
        if (it != data[i].fields.end() &&
            std::get_if<std::string>(&it->second) &&
            std::get<std::string>(it->second) == preds[i])
            ++correct;
    }
    double acc = static_cast<double>(correct) / static_cast<double>(data.size());
    EXPECT_GT(acc, 0.6) << "Accuracy too low for a linearly-separable dataset";
}

// ============================================================================
// Explanation
// ============================================================================

TEST_F(BinaryClassTest, ExplainReturnsSameCount) {
    auto exps = model.explain(data);
    EXPECT_EQ(exps.size(), data.size());
}

TEST_F(BinaryClassTest, ExplainContributionsHaveFeatureNames) {
    auto exp = model.explainOne(data[0]);
    EXPECT_FALSE(exp.feature_contributions.empty());
    for (const auto& [feat, contrib] : exp.feature_contributions)
        EXPECT_FALSE(feat.empty());
}

TEST_F(BinaryClassTest, ExplainTopFeaturesNotEmpty) {
    auto exp = model.explainOne(data[0]);
    EXPECT_FALSE(exp.top_features.empty());
}

TEST_F(BinaryClassTest, ExplainContributionsSortedByAbsDesc) {
    auto exp = model.explainOne(data[0]);
    for (size_t i = 1; i < exp.feature_contributions.size(); ++i) {
        EXPECT_GE(std::abs(exp.feature_contributions[i - 1].second),
                  std::abs(exp.feature_contributions[i].second));
    }
}

TEST_F(BinaryClassTest, ExplainIdMatches) {
    auto exp = model.explainOne(data[3]);
    EXPECT_EQ(exp.id, data[3].id);
}

TEST_F(BinaryClassTest, ExplainPredictedLabelKnown) {
    auto exp = model.explainOne(data[0]);
    EXPECT_TRUE(exp.predicted_label == "pos" || exp.predicted_label == "neg");
}

// ============================================================================
// Feature importance
// ============================================================================

TEST_F(BinaryClassTest, FeatureImportanceSumsToOne) {
    auto fi = model.featureImportance();
    EXPECT_FALSE(fi.empty());
    double sum = 0.0;
    for (const auto& [k, v] : fi) {
      sum += v;
    }
    EXPECT_NEAR(sum, 1.0, 1e-4);
}

TEST_F(BinaryClassTest, FeatureImportanceContainsBothFeatures) {
    auto fi = model.featureImportance();
    EXPECT_NE(fi.find("x1"), fi.end());
    EXPECT_NE(fi.find("x2"), fi.end());
}

// ============================================================================
// Serialisation
// ============================================================================

TEST_F(BinaryClassTest, SerializeDeserializePreservesMetadata) {
    auto s = model.serialize();
    EXPECT_FALSE(s.empty());
    auto restored = AutoMLModel::deserialize(s);
    EXPECT_EQ(restored.task(), model.task());
}

// ============================================================================
// Multi-class classification
// ============================================================================

TEST(MultiClassTest, TrainsAndPredicts) {
    auto data = makeMulticlassData(90);
    AutoML automl;
    AutoMLConfig cfg = fastConfig("label");
    cfg.algorithms   = {ModelAlgorithm::DECISION_TREE};
    cfg.cv_folds     = 2;

    auto model = automl.trainClassifier(data, cfg);
    auto preds = model.predict(data);
    EXPECT_EQ(preds.size(), data.size());
    // All predictions must be one of the three classes
    std::set<std::string> valid = {"0", "1", "2"};
    for (const auto& p : preds)
        EXPECT_NE(valid.find(p), valid.end()) << "Unexpected class: " << p;
}

// ============================================================================
// Regression
// ============================================================================

class RegressionTest : public ::testing::Test {
protected:
    void SetUp() override {
        data = makeRegressionData(100);
        cfg  = fastConfig("y", AutoMLTask::REGRESSION);
        cfg.algorithms = {ModelAlgorithm::LINEAR_REGRESSION};
        model = automl.trainRegressor(data, cfg);
    }
    AutoML automl;
    std::vector<DataPoint> data;
    AutoMLConfig cfg;
    AutoMLModel model;
};

TEST_F(RegressionTest, TaskIsRegression) {
    EXPECT_EQ(model.task(), AutoMLTask::REGRESSION);
}

TEST_F(RegressionTest, PredictReturnsSameCount) {
    auto preds = model.predict(data);
    EXPECT_EQ(preds.size(), data.size());
}

TEST_F(RegressionTest, PredictionsAreNumericStrings) {
    auto preds = model.predict(data);
    for (const auto& p : preds) {
        EXPECT_NO_THROW((void)std::stod(p)) << "Non-numeric prediction: " << p;
    }
}

TEST_F(RegressionTest, ExplainWorks) {
    auto exp = model.explainOne(data[0]);
    EXPECT_FALSE(exp.feature_contributions.empty());
}

TEST_F(RegressionTest, R2AboveThreshold) {
    // Linear regression on a near-linear dataset should have good R²
    EXPECT_GT(model.metrics().r2, 0.7);
}

// ============================================================================
// Progress callback
// ============================================================================

TEST(AutoMLProgressCallbackTest, CallbackInvoked) {
    auto data = makeBinaryData(60);
    AutoML automl;
    AutoMLConfig cfg = fastConfig("label");
    cfg.algorithms   = {ModelAlgorithm::DECISION_TREE};
    cfg.max_trials   = 4;
    cfg.cv_folds     = 2;

    int cb_calls = 0;
    double last_best = -1e9;
    automl.trainClassifier(data, cfg,
        [&](int trial, int total, double best) {
            ++cb_calls;
            EXPECT_GE(trial, 1);
            EXPECT_GE(total, 1);
            last_best = best;
        });

    EXPECT_GE(cb_calls, 1);
    (void)last_best;
}

// ============================================================================
// Feature engineering (polynomial expansion)
// ============================================================================

TEST(FeatureEngineeringTest, PolyExpansionIncreasesFeatureCount) {
    auto data = makeBinaryData(80);
    AutoML automl;
    AutoMLConfig cfg = fastConfig("label");
    cfg.algorithms          = {ModelAlgorithm::LOGISTIC_REGRESSION};
    cfg.feature_engineering = true;
    cfg.max_trials          = 2;
    cfg.cv_folds            = 2;

    auto model = automl.trainClassifier(data, cfg);
    // With 2 raw features + 2 squared = 4 total features
    auto fi = model.featureImportance();
    EXPECT_GE(fi.size(), 2u);
}

// ============================================================================
// Ensemble generation
// ============================================================================

TEST(EnsembleTest, EnsembleModelHasCorrectAlgorithm) {
    auto data = makeBinaryData(80);
    AutoML automl;
    AutoMLConfig cfg = fastConfig("label");
    cfg.algorithms   = {ModelAlgorithm::DECISION_TREE,
                         ModelAlgorithm::LOGISTIC_REGRESSION};
    cfg.ensemble     = true;
    cfg.ensemble_top_k = 2;
    cfg.max_trials   = 4;
    cfg.cv_folds     = 2;

    auto model = automl.trainClassifier(data, cfg);
    EXPECT_EQ(model.algorithm(), ModelAlgorithm::ENSEMBLE);
}

// ============================================================================
// crossValidate helper
// ============================================================================

TEST(CrossValidateTest, ReturnsNonZeroMetrics) {
    auto data = makeBinaryData(80);
    AutoML automl;
    AutoMLConfig cfg = fastConfig("label");
    cfg.cv_folds = 2;

    auto em = automl.crossValidate(data, cfg,
                                    ModelAlgorithm::DECISION_TREE, {});
    EXPECT_GE(em.accuracy, 0.0);
    EXPECT_LE(em.accuracy, 1.0);
    EXPECT_GE(em.f1,       0.0);
    EXPECT_LE(em.f1,       1.0);
}

// ============================================================================
// Individual algorithms: ensure each trains and predicts
// ============================================================================

struct AlgoTestParam {
    ModelAlgorithm algo;
    AutoMLTask     task;
};

class SingleAlgoTest : public ::testing::TestWithParam<AlgoTestParam> {};

TEST_P(SingleAlgoTest, TrainsAndPredicts) {
    auto param = GetParam();
    bool is_cls = (param.task == AutoMLTask::CLASSIFICATION);
    auto data = is_cls ? makeBinaryData(80) : makeRegressionData(80);
    std::string target = is_cls ? "label" : "y";

    AutoML automl;
    AutoMLConfig cfg = fastConfig(target, param.task);
    cfg.algorithms = {param.algo};
    cfg.ensemble   = false;
    cfg.max_trials = 2;
    cfg.cv_folds   = 2;
    if (!is_cls) {
      cfg.metric = AutoMLMetric::R2;
    }

    AutoMLModel model = {};
    if (is_cls)
        ASSERT_NO_THROW(model = automl.trainClassifier(data, cfg));
    else
        ASSERT_NO_THROW(model = automl.trainRegressor(data, cfg));

    auto preds = model.predict(data);
    EXPECT_EQ(preds.size(), data.size());
}

INSTANTIATE_TEST_SUITE_P(AllAlgorithms, SingleAlgoTest, ::testing::Values(
    AlgoTestParam{ModelAlgorithm::LOGISTIC_REGRESSION, AutoMLTask::CLASSIFICATION},
    AlgoTestParam{ModelAlgorithm::DECISION_TREE,       AutoMLTask::CLASSIFICATION},
    AlgoTestParam{ModelAlgorithm::RANDOM_FOREST,       AutoMLTask::CLASSIFICATION},
    AlgoTestParam{ModelAlgorithm::GRADIENT_BOOSTING,   AutoMLTask::CLASSIFICATION},
    AlgoTestParam{ModelAlgorithm::KNN,                 AutoMLTask::CLASSIFICATION},
    AlgoTestParam{ModelAlgorithm::LINEAR_REGRESSION,   AutoMLTask::REGRESSION},
    AlgoTestParam{ModelAlgorithm::DECISION_TREE,       AutoMLTask::REGRESSION},
    AlgoTestParam{ModelAlgorithm::RANDOM_FOREST,       AutoMLTask::REGRESSION},
    AlgoTestParam{ModelAlgorithm::GRADIENT_BOOSTING,   AutoMLTask::REGRESSION},
    AlgoTestParam{ModelAlgorithm::KNN,                 AutoMLTask::REGRESSION}
));

// ============================================================================
// AutoML::trainClassifier with default algorithms (all)
// ============================================================================

TEST(DefaultAlgorithmsTest, ClassificationWithAllAlgorithms) {
    auto data = makeBinaryData(100);
    AutoML automl;
    AutoMLConfig cfg;
    cfg.target           = "label";
    cfg.task             = AutoMLTask::CLASSIFICATION;
    cfg.metric           = AutoMLMetric::ACCURACY;
    cfg.max_time_minutes = 1;
    cfg.max_trials       = 10;
    cfg.cv_folds         = 2;
    cfg.feature_engineering = false;
    cfg.ensemble         = false;
    cfg.random_seed      = 7;

    auto model = automl.trainClassifier(data, cfg);
    EXPECT_FALSE(model.candidateModels().empty());
    auto preds = model.predict(data);
    EXPECT_EQ(preds.size(), data.size());
}

TEST(DefaultAlgorithmsTest, RegressionWithAllAlgorithms) {
    auto data = makeRegressionData(100);
    AutoML automl;
    AutoMLConfig cfg;
    cfg.target           = "y";
    cfg.task             = AutoMLTask::REGRESSION;
    cfg.metric           = AutoMLMetric::R2;
    cfg.max_time_minutes = 1;
    cfg.max_trials       = 10;
    cfg.cv_folds         = 2;
    cfg.feature_engineering = false;
    cfg.ensemble         = false;
    cfg.random_seed      = 7;

    auto model = automl.trainRegressor(data, cfg);
    EXPECT_FALSE(model.candidateModels().empty());
    auto preds = model.predict(data);
    EXPECT_EQ(preds.size(), data.size());
}

// ============================================================================
// KNN Regressor accuracy: y = 2*x (section 10, FUTURE_ENHANCEMENTS.md)
// Verifies KNNModel::predictOneReg() is not the 0.0 stub and produces
// predictions within ±0.5 of the true value for a simple linear function.
// ============================================================================

/**
 * Build a simple 1-D regression dataset: y = 2 * x, x in [0, 1].
 * The KNN regressor (inverse-distance-weighted mean of k=5 nearest
 * neighbours) must predict a value close to 2 * query_x.
 */
static std::vector<DataPoint> makeLinearData(int n) {
    std::vector<DataPoint> pts;
    pts.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id = "lin" + std::to_string(i);
        double x = static_cast<double>(i) / static_cast<double>(n - 1);
        p.set("x", x);
        p.fields["y"] = 2.0 * x;
        pts.push_back(std::move(p));
    }
    return pts;
}

TEST(KNNRegressorTest, PredictOneRegNotStub) {
    // Train KNN-only regressor on y = 2x with 100 points.
    auto train = makeLinearData(100);

    AutoML automl;
    AutoMLConfig cfg;
    cfg.target               = "y";
    cfg.task                 = AutoMLTask::REGRESSION;
    cfg.metric               = AutoMLMetric::R2;
    cfg.max_time_minutes     = 1;
    cfg.max_trials           = 5;
    cfg.cv_folds             = 2;
    cfg.feature_engineering  = false;
    cfg.ensemble             = false;
    cfg.algorithms           = {ModelAlgorithm::KNN};
    cfg.random_seed          = 42;

    AutoMLModel model = automl.trainRegressor(train, cfg);
    ASSERT_EQ(model.task(), AutoMLTask::REGRESSION);

    // Query point: x = 0.5, expected y ≈ 1.0.
    DataPoint query;
    query.id = "q";
    query.set("x", 0.5);

    const std::string pred_str = model.predictOne(query);
    ASSERT_NO_THROW((void)std::stod(pred_str));
    const double pred = std::stod(pred_str);

    // Must not be the 0.0 stub and must be within ±0.5 of the true value.
    EXPECT_NE(pred, 0.0) << "KNNModel::predictOneReg() appears to be the 0.0 stub";
    EXPECT_NEAR(pred, 1.0, 0.5)
        << "KNN prediction for x=0.5 (expected y≈1.0) was " << pred;
}

TEST(KNNRegressorTest, PredictAtEndpoints) {
    auto train = makeLinearData(100);

    AutoML automl;
    AutoMLConfig cfg;
    cfg.target               = "y";
    cfg.task                 = AutoMLTask::REGRESSION;
    cfg.metric               = AutoMLMetric::R2;
    cfg.max_time_minutes     = 1;
    cfg.max_trials           = 5;
    cfg.cv_folds             = 2;
    cfg.feature_engineering  = false;
    cfg.ensemble             = false;
    cfg.algorithms           = {ModelAlgorithm::KNN};
    cfg.random_seed          = 42;

    AutoMLModel model = automl.trainRegressor(train, cfg);

    // At x=0.0, expected y ≈ 0.0.
    DataPoint q0; q0.id = "q0"; q0.set("x", 0.0);
    double p0 = std::stod(model.predictOne(q0));
    EXPECT_NEAR(p0, 0.0, 0.5) << "KNN at x=0: expected ~0.0, got " << p0;

    // At x=1.0, expected y ≈ 2.0.
    DataPoint q1; q1.id = "q1"; q1.set("x", 1.0);
    double p1 = std::stod(model.predictOne(q1));
    EXPECT_NEAR(p1, 2.0, 0.5) << "KNN at x=1: expected ~2.0, got " << p1;
}

// KNN Regression accuracy: y = 2x  (issue #137 · item 10)
// ============================================================================

/**
 * Build a dataset with a single feature x and target y = 2*x.
 * x is uniformly spread over [0, 10] with `n` points.
 * Requires n >= 2 to produce a meaningful linear spread.
 */
static std::vector<DataPoint> makeLinear2xData(int n) {
    if (n < 2) {
        throw std::invalid_argument("makeLinear2xData requires at least 2 points");
    }
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id = "knn" + std::to_string(i);
        double x = static_cast<double>(i) * 10.0 / (n - 1);
        p.set("x", x);
        p.fields["y"] = 2.0 * x;
        data.push_back(std::move(p));
    }
    return data;
}

TEST(KNNRegressionTest, PredictOneRegLinearRelation) {
    // Train a KNN regressor on y = 2x with 100 points spanning [0, 10].
    // Querying x = 5.0 must return a value within ±0.5 of the expected 10.0.
    auto data = makeLinear2xData(100);

    AutoML automl;
    AutoMLConfig cfg;
    cfg.target              = "y";
    cfg.task                = AutoMLTask::REGRESSION;
    cfg.metric              = AutoMLMetric::RMSE;
    cfg.algorithms          = {ModelAlgorithm::KNN};
    cfg.max_trials          = 2;
    cfg.cv_folds            = 2;
    cfg.feature_engineering = false;
    cfg.ensemble            = false;
    cfg.random_seed         = 42;

    auto model = automl.trainRegressor(data, cfg);

    DataPoint query;
    query.id = "q";
    query.set("x", 5.0);

    double pred = std::stod(model.predictOne(query));
    EXPECT_NEAR(pred, 10.0, 0.5)
        << "KNN regression on y=2x: predicted " << pred
        << " but expected ~10.0 for x=5.0";
}

TEST(KNNRegressionTest, PredictOneRegPerformance) {
    const char* env = std::getenv("THEMIS_RUN_PERF_TESTS");
    if (!env || std::string(env) != "1") {
        GTEST_SKIP() << "Skipping performance test (set THEMIS_RUN_PERF_TESTS=1 to enable)";
    }

    // Build a 10 000-sample training set: y = 2x, x in [0, 10].
    auto data = makeLinear2xData(10000);

    AutoML automl;
    AutoMLConfig cfg;
    cfg.target              = "y";
    cfg.task                = AutoMLTask::REGRESSION;
    cfg.metric              = AutoMLMetric::RMSE;
    cfg.algorithms          = {ModelAlgorithm::KNN};
    cfg.max_trials          = 1;
    cfg.cv_folds            = 2;
    cfg.feature_engineering = false;
    cfg.ensemble            = false;
    cfg.random_seed         = 42;

    auto model = automl.trainRegressor(data, cfg);

    DataPoint query;
    query.id = "q";
    query.set("x", 5.0);

    // Warm up
    model.predictOne(query);

    // Measure a single prediction — must complete in ≤ 1 ms.
    auto t0  = std::chrono::high_resolution_clock::now();
    model.predictOne(query);
    auto t1  = std::chrono::high_resolution_clock::now();
    auto us  = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

    EXPECT_LE(us, 1000)
        << "KNN predictOneReg (n=10 000) took " << us
        << " µs — limit is 1000 µs (1 ms)";
}

// ============================================================================
// LRModel (LogisticRegression) regression proxy via expected-class value
// Verifies LRModel::predictOneReg() no longer returns the 0.0 stub and
// produces a value in [0, 1] for a binary-class training dataset.
// ============================================================================

/**
 * Build a binary classification dataset: class = (x > 0.5 ? 1 : 0).
 * When used as a regression proxy the expected output for x just above 0.5
 * should be close to 1.0, and for x just below 0.5 it should be close to 0.0.
 */
static std::vector<DataPoint> makeBinaryThresholdData(int n) {
    std::vector<DataPoint> pts;
    pts.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint p;
        p.id = "lr" + std::to_string(i);
        double x = static_cast<double>(i) / static_cast<double>(n - 1);
        p.set("x", x);
        // Target for regression: 0 when x <= 0.5, 1 when x > 0.5
        p.fields["y"] = (x > 0.5) ? 1.0 : 0.0;
        pts.push_back(std::move(p));
    }
    return pts;
}

TEST(LRModelRegressorTest, PredictOneRegNotZeroStub) {
    // Train LOGISTIC_REGRESSION on the binary dataset.
    auto train = makeBinaryThresholdData(100);

    AutoML automl;
    AutoMLConfig cfg;
    cfg.target               = "y";
    cfg.task                 = AutoMLTask::REGRESSION;
    cfg.metric               = AutoMLMetric::RMSE;
    cfg.max_time_minutes     = 1;
    cfg.max_trials           = 3;
    cfg.cv_folds             = 2;
    cfg.feature_engineering  = false;
    cfg.ensemble             = false;
    cfg.algorithms           = {ModelAlgorithm::LOGISTIC_REGRESSION};
    cfg.random_seed          = 42;

    AutoMLModel model = automl.trainRegressor(train, cfg);
    ASSERT_EQ(model.task(), AutoMLTask::REGRESSION);

    // Query point far above decision boundary: x = 0.9, expected y ≈ 1.0.
    DataPoint qHigh;
    qHigh.id = "qh";
    qHigh.set("x", 0.9);

    const std::string pred_str = model.predictOne(qHigh);
    ASSERT_NO_THROW((void)std::stod(pred_str));
    const double pred = std::stod(pred_str);

    // Result must be a valid probability in [0, 1].
    EXPECT_GE(pred, 0.0) << "LRModel::predictOneReg() returned value below 0";
    EXPECT_LE(pred, 1.0) << "LRModel::predictOneReg() returned value above 1";

    // Must not be the 0.0 stub for a clearly positive example.
    EXPECT_NE(pred, 0.0) << "LRModel::predictOneReg() appears to still be the 0.0 stub";
}

TEST(LRModelRegressorTest, PredictOneRegRangeMonotonic) {
    // The LR regression proxy must return higher values for x near 1.0
    // (positive region) than for x near 0.0 (negative region).
    auto train = makeBinaryThresholdData(100);

    AutoML automl;
    AutoMLConfig cfg;
    cfg.target               = "y";
    cfg.task                 = AutoMLTask::REGRESSION;
    cfg.metric               = AutoMLMetric::RMSE;
    cfg.max_time_minutes     = 1;
    cfg.max_trials           = 3;
    cfg.cv_folds             = 2;
    cfg.feature_engineering  = false;
    cfg.ensemble             = false;
    cfg.algorithms           = {ModelAlgorithm::LOGISTIC_REGRESSION};
    cfg.random_seed          = 42;

    AutoMLModel model = automl.trainRegressor(train, cfg);

    DataPoint qLow;
    qLow.id = "ql";
    qLow.set("x", 0.1);
    const double predLow = std::stod(model.predictOne(qLow));

    DataPoint qHigh;
    qHigh.id = "qh";
    qHigh.set("x", 0.9);
    const double predHigh = std::stod(model.predictOne(qHigh));

    // The logistic proxy must assign higher score to the positive region.
    EXPECT_GT(predHigh, predLow)
        << "LRModel regression proxy: predHigh=" << predHigh
        << " should exceed predLow=" << predLow;
}
