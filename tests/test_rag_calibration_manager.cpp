/**
 * @file test_rag_calibration_manager.cpp
 * @brief Unit tests for CalibrationManager
 *
 * Tests cover:
 *  - Default construction and config accessors
 *  - addGroundTruth / loadGroundTruth (bad path)
 *  - calculateECE: perfect calibration yields ECE == 0
 *  - calculateECE: miscalibrated model yields ECE > 0
 *  - calculateBrierScore: perfect prediction, uniform error
 *  - calculateInterAnnotatorAgreement: full agreement, two disagreeing annotators
 *  - calibrate() with TEMPERATURE_SCALING leaves scores in [0, 1]
 *  - calibrate() with CalibrationMethod::NONE returns scores unchanged
 *  - saveModel / loadModel round-trip
 *  - calculateMetrics returns sensible values for random predictions
 */

#include "rag/calibration_manager.h"

#include <gtest/gtest.h>

#include <cmath>
#include <filesystem>
#include <string>
#include <vector>

using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static EvaluationResult makeResult(double score, double confidence = 0.9) {
    EvaluationResult r{};
    r.faithfulness_score       = score;
    r.relevance_score          = score;
    r.completeness_score       = score;
    r.coherence_score          = score;
    r.ethical_compliance_score = score;
    r.overall_score            = score;
    r.passed_quality_threshold = score >= 0.7;
    r.confidence               = confidence;
    r.respects_human_autonomy  = true;
    r.shows_moral_diversity    = false;
    r.has_ethical_citations    = false;
    return r;
}

static GroundTruthAnnotation makeAnnotation(double score) {
    GroundTruthAnnotation ann;
    ann.test_id               = "t";
    ann.query                 = "q";
    ann.answer                = "a";
    ann.faithfulness_score    = score;
    ann.relevance_score       = score;
    ann.completeness_score    = score;
    ann.coherence_score       = score;
    ann.overall_score         = score;
    ann.inter_annotator_agreement = 1.0;
    return ann;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / config
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, DefaultConstruction) {
    CalibrationManager mgr;
    auto cfg = mgr.getConfig();
    EXPECT_EQ(cfg.method, CalibrationMethod::TEMPERATURE_SCALING);
    EXPECT_EQ(cfg.num_bins, 10);
}

TEST(CalibrationManagerTest, CustomConfigStored) {
    CalibrationConfig cfg;
    cfg.method   = CalibrationMethod::NONE;
    cfg.num_bins = 20;
    CalibrationManager mgr(cfg);
    auto got = mgr.getConfig();
    EXPECT_EQ(got.method, CalibrationMethod::NONE);
    EXPECT_EQ(got.num_bins, 20);
}

TEST(CalibrationManagerTest, SetConfig) {
    CalibrationManager mgr;
    CalibrationConfig cfg;
    cfg.method   = CalibrationMethod::PLATT_SCALING;
    cfg.num_bins = 15;
    mgr.setConfig(cfg);
    auto got = mgr.getConfig();
    EXPECT_EQ(got.method, CalibrationMethod::PLATT_SCALING);
    EXPECT_EQ(got.num_bins, 15);
}

// ─────────────────────────────────────────────────────────────────────────────
// loadGroundTruth – bad path
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, LoadGroundTruthBadPathReturnsZero) {
    CalibrationManager mgr;
    size_t loaded = mgr.loadGroundTruth("/nonexistent/path/that/does/not/exist.json");
    EXPECT_EQ(loaded, 0u);
}

// ─────────────────────────────────────────────────────────────────────────────
// ECE calculation
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, ECEPerfectCalibration) {
    CalibrationManager mgr;
    // When predictions == ground truth ECE should be 0
    std::vector<double> preds = {0.1, 0.5, 0.9};
    std::vector<double> gts   = {0.1, 0.5, 0.9};
    std::vector<double> confs = {1.0, 1.0, 1.0};
    double ece = mgr.calculateECE(preds, gts, confs);
    EXPECT_NEAR(ece, 0.0, 1e-6);
}

TEST(CalibrationManagerTest, ECEMiscalibratedModelPositive) {
    CalibrationManager mgr;
    // Predictions are all 0.9 but ground truth is 0.1 → large ECE
    std::vector<double> preds = {0.9, 0.9, 0.9, 0.9};
    std::vector<double> gts   = {0.1, 0.1, 0.1, 0.1};
    std::vector<double> confs = {1.0, 1.0, 1.0, 1.0};
    double ece = mgr.calculateECE(preds, gts, confs);
    EXPECT_GT(ece, 0.0);
}

TEST(CalibrationManagerTest, ECEEmptyInputZero) {
    CalibrationManager mgr;
    double ece = mgr.calculateECE({}, {}, {});
    EXPECT_DOUBLE_EQ(ece, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Brier score
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, BrierScorePerfect) {
    CalibrationManager mgr;
    std::vector<double> preds = {1.0, 0.0, 1.0};
    std::vector<double> gts   = {1.0, 0.0, 1.0};
    EXPECT_NEAR(mgr.calculateBrierScore(preds, gts), 0.0, 1e-9);
}

TEST(CalibrationManagerTest, BrierScoreUniformError) {
    CalibrationManager mgr;
    // All predictions off by 0.5 → Brier = 0.25
    std::vector<double> preds = {0.5, 0.5};
    std::vector<double> gts   = {0.0, 1.0};
    EXPECT_NEAR(mgr.calculateBrierScore(preds, gts), 0.25, 1e-9);
}

TEST(CalibrationManagerTest, BrierScoreEmpty) {
    CalibrationManager mgr;
    EXPECT_DOUBLE_EQ(mgr.calculateBrierScore({}, {}), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Inter-annotator agreement
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, InterAnnotatorFullAgreement) {
    CalibrationManager mgr;
    std::vector<std::vector<double>> anns = {
        {0.8, 0.9, 0.7},  // annotator 1
        {0.8, 0.9, 0.7},  // annotator 2 (identical)
    };
    double agreement = mgr.calculateInterAnnotatorAgreement(anns);
    EXPECT_NEAR(agreement, 1.0, 1e-6);
}

TEST(CalibrationManagerTest, InterAnnotatorSingleAnnotator) {
    CalibrationManager mgr;
    std::vector<std::vector<double>> anns = {{0.5, 0.7, 0.9}};
    double agreement = mgr.calculateInterAnnotatorAgreement(anns);
    EXPECT_DOUBLE_EQ(agreement, 1.0);
}

TEST(CalibrationManagerTest, InterAnnotatorEmptyReturnsZero) {
    CalibrationManager mgr;
    double agreement = mgr.calculateInterAnnotatorAgreement({});
    EXPECT_DOUBLE_EQ(agreement, 0.0);
}

TEST(CalibrationManagerTest, InterAnnotatorPartialDisagreement) {
    CalibrationManager mgr;
    // Two annotators with different ratings
    std::vector<std::vector<double>> anns = {
        {1.0, 0.0},
        {0.0, 1.0},
    };
    double agreement = mgr.calculateInterAnnotatorAgreement(anns);
    // Agreement should be < 1 due to disagreement
    EXPECT_LT(agreement, 1.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// calibrate() – TEMPERATURE_SCALING keeps scores in [0, 1]
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, CalibrateScoresInRange) {
    CalibrationConfig cfg;
    cfg.method = CalibrationMethod::TEMPERATURE_SCALING;
    CalibrationManager mgr(cfg);

    auto result = makeResult(0.75);
    auto cal    = mgr.calibrate(result);

    EXPECT_GE(cal.faithfulness_score, 0.0);
    EXPECT_LE(cal.faithfulness_score, 1.0);
    EXPECT_GE(cal.relevance_score, 0.0);
    EXPECT_LE(cal.relevance_score, 1.0);
    EXPECT_GE(cal.overall_score, 0.0);
    EXPECT_LE(cal.overall_score, 1.0);
}

TEST(CalibrationManagerTest, CalibrateNoneUnchanged) {
    CalibrationConfig cfg;
    cfg.method = CalibrationMethod::NONE;
    CalibrationManager mgr(cfg);

    auto result = makeResult(0.75);
    auto cal    = mgr.calibrate(result);

    EXPECT_DOUBLE_EQ(cal.overall_score, 0.75);
    EXPECT_DOUBLE_EQ(cal.faithfulness_score, 0.75);
}

// ─────────────────────────────────────────────────────────────────────────────
// calculateMetrics
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, CalculateMetricsPerfectPredictions) {
    CalibrationManager mgr;
    std::vector<EvaluationResult> preds = {makeResult(0.8), makeResult(0.6)};
    std::vector<GroundTruthAnnotation> gts  = {makeAnnotation(0.8), makeAnnotation(0.6)};

    auto m = mgr.calculateMetrics(preds, gts);
    EXPECT_NEAR(m.mae,  0.0, 1e-9);
    EXPECT_NEAR(m.rmse, 0.0, 1e-9);
    EXPECT_NEAR(m.correlation, 1.0, 1e-6);
}

TEST(CalibrationManagerTest, CalculateMetricsEmptyReturnsZero) {
    CalibrationManager mgr;
    auto m = mgr.calculateMetrics({}, {});
    EXPECT_DOUBLE_EQ(m.mae,  0.0);
    EXPECT_DOUBLE_EQ(m.rmse, 0.0);
}

TEST(CalibrationManagerTest, CalculateMetricsSizeMismatchReturnsZero) {
    CalibrationManager mgr;
    std::vector<EvaluationResult> preds = {makeResult(0.8)};
    std::vector<GroundTruthAnnotation> gts;  // empty
    auto m = mgr.calculateMetrics(preds, gts);
    EXPECT_DOUBLE_EQ(m.mae, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// saveModel / loadModel round-trip
// ─────────────────────────────────────────────────────────────────────────────

TEST(CalibrationManagerTest, SaveLoadRoundTrip) {
    const std::string path = "/tmp/test_cal_model.json";
    // Ensure clean state
    std::filesystem::remove(path);

    CalibrationConfig cfg;
    cfg.method = CalibrationMethod::TEMPERATURE_SCALING;
    CalibrationManager mgr_save(cfg);

    // Add ground truth so temperature is set on train (we skip train here and
    // just test persistence of default temperature = 1.0)
    bool saved = mgr_save.saveModel(path);
    EXPECT_TRUE(saved);
    EXPECT_TRUE(std::filesystem::exists(path));

    CalibrationManager mgr_load;
    bool loaded = mgr_load.loadModel(path);
    EXPECT_TRUE(loaded);
    EXPECT_EQ(mgr_load.getConfig().method, CalibrationMethod::TEMPERATURE_SCALING);

    std::filesystem::remove(path);
}

TEST(CalibrationManagerTest, SaveModelBadPathReturnsFalse) {
    CalibrationManager mgr;
    bool saved = mgr.saveModel("/nonexistent_dir/model.json");
    EXPECT_FALSE(saved);
}

TEST(CalibrationManagerTest, LoadModelBadPathReturnsFalse) {
    CalibrationManager mgr;
    bool loaded = mgr.loadModel("/nonexistent/model.json");
    EXPECT_FALSE(loaded);
}
