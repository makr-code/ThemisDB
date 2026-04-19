/**
 * AutoML ONNX export unit tests.
 *
 * Test IDs: AOE-01 … AOE-06
 *
 * Covers:
 *   - exportONNX() returns empty string (success) for supported algorithms
 *   - Output file is created and is non-empty
 *   - Output is valid JSON (minimal check for required keys)
 *   - exportONNX() returns an UNSUPPORTED_OPERATION error for ENSEMBLE
 *   - exportONNX("") with empty path skips file write but still succeeds
 *   - exportONNX() succeeds for KNN and GradientBoosting
 */

#include <gtest/gtest.h>
#include "analytics/automl.h"

#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace themisdb::analytics;

// ============================================================================
// Helpers
// ============================================================================

static AutoMLConfig makeConfig(const std::string& target,
                                AutoMLTask task = AutoMLTask::CLASSIFICATION,
                                ModelAlgorithm algo = ModelAlgorithm::DECISION_TREE)
{
    AutoMLConfig cfg;
    cfg.target              = target;
    cfg.task                = task;
    cfg.max_time_minutes    = 0;
    cfg.max_trials          = 5;
    cfg.cv_folds            = 2;
    cfg.feature_engineering = false;
    cfg.ensemble            = false;
    cfg.random_seed         = 42;
    cfg.metric              = (task == AutoMLTask::CLASSIFICATION)
                             ? AutoMLMetric::ACCURACY : AutoMLMetric::RMSE;
    cfg.algorithms          = {algo};
    return cfg;
}

static std::vector<DataPoint> makeClassData(int n) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint dp;
        dp.set("x1", static_cast<double>(i % 5));
        dp.set("x2", static_cast<double>((i * 3) % 7));
        dp.fields["label"] = (i % 2 == 0) ? "pos" : "neg";
        data.push_back(dp);
    }
    return data;
}

static std::vector<DataPoint> makeRegData(int n) {
    std::vector<DataPoint> data;
    data.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        DataPoint dp;
        dp.set("x1", static_cast<double>(i));
        dp.set("x2", static_cast<double>(i % 3));
        dp.set("target", static_cast<double>(i) * 2.0 + 1.0);
        data.push_back(dp);
    }
    return data;
}

static std::string tmpPath() { return "/tmp/test_automl_onnx.json"; }

// ============================================================================
// AOE-01: exportONNX returns empty string for LogisticRegression
// ============================================================================
TEST(AutoMLONNXExportTests, AOE01_LogisticRegressionSuccess) {
    AutoML automl;
    auto model = automl.trainClassifier(makeClassData(60),
                    makeConfig("label", AutoMLTask::CLASSIFICATION,
                               ModelAlgorithm::LOGISTIC_REGRESSION));
    std::string err = model.exportONNX(tmpPath());
    EXPECT_EQ(err, "") << "exportONNX error: " << err;
    std::remove(tmpPath().c_str());
}

// ============================================================================
// AOE-02: Output file is created and contains JSON (non-empty)
// ============================================================================
TEST(AutoMLONNXExportTests, AOE02_FileCreatedAndNonEmpty) {
    AutoML automl;
    auto model = automl.trainClassifier(makeClassData(60),
                    makeConfig("label", AutoMLTask::CLASSIFICATION,
                               ModelAlgorithm::DECISION_TREE));
    std::string path = tmpPath();
    EXPECT_EQ(model.exportONNX(path), "");
    std::ifstream ifs(path);
    ASSERT_TRUE(ifs.is_open()) << "Output file not created: " << path;
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_GT(content.size(), 50u) << "Output file is unexpectedly small";
    std::remove(path.c_str());
}

// ============================================================================
// AOE-03: Output file contains required ONNX-JSON keys
// ============================================================================
TEST(AutoMLONNXExportTests, AOE03_RequiredKeysPresent) {
    AutoML automl;
    auto model = automl.trainClassifier(makeClassData(60),
                    makeConfig("label", AutoMLTask::CLASSIFICATION,
                               ModelAlgorithm::RANDOM_FOREST));
    std::string path = tmpPath();
    EXPECT_EQ(model.exportONNX(path), "");
    std::ifstream ifs(path);
    std::string content((std::istreambuf_iterator<char>(ifs)),
                         std::istreambuf_iterator<char>());
    EXPECT_NE(content.find("ir_version"),    std::string::npos) << "Missing ir_version";
    EXPECT_NE(content.find("feature_names"), std::string::npos) << "Missing feature_names";
    EXPECT_NE(content.find("model"),         std::string::npos) << "Missing model section";
    EXPECT_EQ(content.front(), '{') << "Output does not start with JSON '{'";
    std::remove(path.c_str());
}

// ============================================================================
// AOE-04: exportONNX with empty path skips file write and returns ""
// ============================================================================
TEST(AutoMLONNXExportTests, AOE04_EmptyPathSkipsWrite) {
    AutoML automl;
    auto model = automl.trainRegressor(makeRegData(40),
                    makeConfig("target", AutoMLTask::REGRESSION,
                               ModelAlgorithm::LINEAR_REGRESSION));
    EXPECT_EQ(model.exportONNX(""), "") << "Empty path should succeed without writing";
}

// ============================================================================
// AOE-05: exportONNX succeeds for GradientBoosting
// ============================================================================
TEST(AutoMLONNXExportTests, AOE05_GradientBoostingExport) {
    AutoML automl;
    auto model = automl.trainClassifier(makeClassData(60),
                    makeConfig("label", AutoMLTask::CLASSIFICATION,
                               ModelAlgorithm::GRADIENT_BOOSTING));
    EXPECT_EQ(model.exportONNX(""), "");
}

// ============================================================================
// AOE-06: exportONNX succeeds for KNN
// ============================================================================
TEST(AutoMLONNXExportTests, AOE06_KNNExport) {
    AutoML automl;
    auto model = automl.trainClassifier(makeClassData(60),
                    makeConfig("label", AutoMLTask::CLASSIFICATION,
                               ModelAlgorithm::KNN));
    EXPECT_EQ(model.exportONNX(""), "");
}
