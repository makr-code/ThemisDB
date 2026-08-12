/**
 * @file test_learning_metrics.cpp
 * @brief Unit tests for the LearningMetrics class
 */

#include <gtest/gtest.h>
#include <chrono>
#include <fstream>
#include <sstream>
#include "rag/learning_metrics.h"

using namespace themis::rag::learning;

// Helper to build an EvaluationEntry with all scores set
static EvaluationEntry makeEntry(double overall, double faith, double rel,
                                  double comp, double coh) {
    EvaluationEntry e;
    e.overall_score      = overall;
    e.faithfulness_score = faith;
    e.relevance_score    = rel;
    e.completeness_score = comp;
    e.coherence_score    = coh;
    e.timestamp          = std::chrono::system_clock::now();
    return e;
}

// ── Default construction ──────────────────────────────────────────────────────

TEST(LearningMetricsTest, DefaultConstructionEmptySnapshot) {
    LearningMetrics lm;
    auto snap = lm.computeMetrics();
    EXPECT_EQ(snap.num_evaluations, 0u);
    EXPECT_DOUBLE_EQ(snap.mean_accuracy,     0.0);
    EXPECT_DOUBLE_EQ(snap.mean_faithfulness, 0.0);
}

// ── recordEvaluation / computeMetrics ────────────────────────────────────────

TEST(LearningMetricsTest, SingleEntryMeans) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.8, 0.9, 0.7, 0.6, 0.5));

    auto snap = lm.computeMetrics();
    EXPECT_EQ(snap.num_evaluations, 1u);
    EXPECT_DOUBLE_EQ(snap.mean_accuracy,     0.8);
    EXPECT_DOUBLE_EQ(snap.mean_faithfulness, 0.9);
    EXPECT_DOUBLE_EQ(snap.mean_relevance,    0.7);
    EXPECT_DOUBLE_EQ(snap.mean_completeness, 0.6);
    EXPECT_DOUBLE_EQ(snap.mean_coherence,    0.5);
}

TEST(LearningMetricsTest, MultipleEntriesMeans) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.6, 0.7, 0.8, 0.5, 0.4));
    lm.recordEvaluation(makeEntry(0.8, 0.9, 0.6, 0.7, 0.8));

    auto snap = lm.computeMetrics();
    EXPECT_EQ(snap.num_evaluations, 2u);
    EXPECT_DOUBLE_EQ(snap.mean_accuracy, 0.7);
    EXPECT_DOUBLE_EQ(snap.mean_faithfulness, 0.8);
}

TEST(LearningMetricsTest, MinMaxAccuracy) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.3, 0.5, 0.5, 0.5, 0.5));
    lm.recordEvaluation(makeEntry(0.9, 0.5, 0.5, 0.5, 0.5));
    lm.recordEvaluation(makeEntry(0.6, 0.5, 0.5, 0.5, 0.5));

    auto snap = lm.computeMetrics();
    EXPECT_DOUBLE_EQ(snap.min_accuracy, 0.3);
    EXPECT_DOUBLE_EQ(snap.max_accuracy, 0.9);
}

TEST(LearningMetricsTest, StdDevSingleEntryIsZero) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.5, 0.5, 0.5, 0.5, 0.5));

    auto snap = lm.computeMetrics();
    EXPECT_DOUBLE_EQ(snap.std_accuracy, 0.0);
}

TEST(LearningMetricsTest, StdDevTwoEntries) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.0, 0.0, 0.0, 0.0, 0.0));
    lm.recordEvaluation(makeEntry(1.0, 1.0, 1.0, 1.0, 1.0));

    auto snap = lm.computeMetrics();
    // sample std-dev of {0, 1} = sqrt(0.5) ≈ 0.707
    EXPECT_NEAR(snap.std_accuracy, std::sqrt(0.5), 1e-9);
}

// ── Trend (slope) ─────────────────────────────────────────────────────────────

TEST(LearningMetricsTest, PositiveTrendWhenScoresIncrease) {
    LearningMetrics lm;
    for (int i = 1; i <= 5; ++i) {
        lm.recordEvaluation(makeEntry(0.1 * i, 0.1 * i, 0.5, 0.5, 0.5));
    }
    auto snap = lm.computeMetrics();
    EXPECT_GT(snap.trend_accuracy, 0.0);
}

TEST(LearningMetricsTest, NegativeTrendWhenScoresDecrease) {
    LearningMetrics lm;
    for (int i = 5; i >= 1; --i) {
        lm.recordEvaluation(makeEntry(0.1 * i, 0.1 * i, 0.5, 0.5, 0.5));
    }
    auto snap = lm.computeMetrics();
    EXPECT_LT(snap.trend_accuracy, 0.0);
}

// ── Window size enforcement ───────────────────────────────────────────────────

TEST(LearningMetricsTest, WindowSizeEnforced) {
    LearningMetrics::Config cfg;
    cfg.window_size = 3;
    LearningMetrics lm(cfg);

    // Record 5 entries; only last 3 should be kept
    lm.recordEvaluation(makeEntry(0.1, 0.1, 0.1, 0.1, 0.1));
    lm.recordEvaluation(makeEntry(0.2, 0.2, 0.2, 0.2, 0.2));
    lm.recordEvaluation(makeEntry(0.9, 0.9, 0.9, 0.9, 0.9));
    lm.recordEvaluation(makeEntry(0.9, 0.9, 0.9, 0.9, 0.9));
    lm.recordEvaluation(makeEntry(0.9, 0.9, 0.9, 0.9, 0.9));

    auto snap = lm.computeMetrics();
    EXPECT_EQ(snap.num_evaluations, 3u);
    EXPECT_DOUBLE_EQ(snap.mean_accuracy, 0.9);
}

// ── exportMetrics ────────────────────────────────────────────────────────────

TEST(LearningMetricsTest, ExportCreatesValidCSV) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.8, 0.7, 0.6, 0.5, 0.4));
    lm.recordEvaluation(makeEntry(0.9, 0.8, 0.7, 0.6, 0.5));

    std::string tmpfile = "/tmp/test_learning_metrics_export.csv";
    lm.exportMetrics(tmpfile);

    std::ifstream f(tmpfile);
    ASSERT_TRUE(f.is_open()) << "CSV file should have been created";

    std::string header;
    std::getline(f, header);
    EXPECT_EQ(header, "timestamp,accuracy,faithfulness,relevance,completeness,coherence");

    // Should have exactly two data rows
    int row_count = 0;
    std::string line;
    while (std::getline(f, line)) {
        if (!line.empty()) ++row_count;
    }
    EXPECT_EQ(row_count, 2);
}

TEST(LearningMetricsTest, ExportEmptyMetricsCreatesHeaderOnly) {
    LearningMetrics lm;
    std::string tmpfile = "/tmp/test_learning_metrics_empty.csv";
    lm.exportMetrics(tmpfile);

    std::ifstream f(tmpfile);
    ASSERT_TRUE(f.is_open());
    std::string header;
    std::getline(f, header);
    EXPECT_EQ(header, "timestamp,accuracy,faithfulness,relevance,completeness,coherence");

    std::string extra;
    std::getline(f, extra);
    EXPECT_TRUE(extra.empty());
}

// ── printReport ──────────────────────────────────────────────────────────────

TEST(LearningMetricsTest, PrintReportContainsKeyLabels) {
    LearningMetrics lm;
    lm.recordEvaluation(makeEntry(0.75, 0.80, 0.65, 0.70, 0.85));

    std::ostringstream oss;
    lm.printReport(oss);
    std::string report = oss.str();

    EXPECT_NE(report.find("Accuracy"),     std::string::npos);
    EXPECT_NE(report.find("Faithfulness"), std::string::npos);
    EXPECT_NE(report.find("Relevance"),    std::string::npos);
    EXPECT_NE(report.find("Completeness"), std::string::npos);
    EXPECT_NE(report.find("Coherence"),    std::string::npos);
}

TEST(LearningMetricsTest, PrintReportEmptyMetrics) {
    LearningMetrics lm;
    std::ostringstream oss;
    EXPECT_NO_THROW(lm.printReport(oss));
    // Should produce a non-empty report even with no data
    EXPECT_FALSE(oss.str().empty());
}
