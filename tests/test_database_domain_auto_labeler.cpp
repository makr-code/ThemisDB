/**
 * Unit tests for DatabaseDomainAutoLabeler (IMPL-A1).
 *
 * Tests:
 *   1.  ComputeConfidenceAtZeroIsHalf   — sigmoid(0) == 0.5
 *   2.  ComputeConfidencePositiveAbove  — |delta| > 0 → confidence > 0.5
 *   3.  BaoDecisionProducesDbOptimizerLabel
 *   4.  BaoDecisionConfidenceAboveThresholdFor5ms — |delta| > 5ms → conf > 0.5
 *   5.  DBAFeedbackNegativeConfidenceAtLeast09
 *   6.  DBAFeedbackPositiveUsesNormalSigmoid
 *   7.  LabelFromLogFileEmptyWhenFileMissing
 *   8.  LabelFromLogFileFiltersMinConfidence
 */

#include <gtest/gtest.h>

#include "training/database_domain_auto_labeler.h"
#include "training/auto_labeler.h"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

using themis::training::DatabaseDomainAutoLabeler;
using themis::training::DomainType;
using themis::training::FeedbackEntry;

// ─── Helpers ──────────────────────────────────────────────────────────────────

static fs::path makeTempFile(const std::string& suffix, const std::string& content) {
    auto path = fs::temp_directory_path() / ("ddb_auto_labeler_" + suffix + ".log");
    std::ofstream f(path);
    f << content;
    return path;
}

// ─── Tests ────────────────────────────────────────────────────────────────────

// 1. sigmoid(0) returns exactly 0.5
TEST(DbAutoLabeler, ComputeConfidenceAtZeroIsHalf) {
    DatabaseDomainAutoLabeler labeler;
    EXPECT_DOUBLE_EQ(labeler.computeConfidence(0.0), 0.5);
}

// 2. Any non-zero delta produces confidence > 0.5
TEST(DbAutoLabeler, ComputeConfidencePositiveAbove) {
    DatabaseDomainAutoLabeler labeler;
    EXPECT_GT(labeler.computeConfidence(1.0),   0.5);
    EXPECT_GT(labeler.computeConfidence(-1.0),  0.5);
    EXPECT_GT(labeler.computeConfidence(100.0), 0.5);
}

// 3. labelFromBaoDecision sets the correct DomainType label
TEST(DbAutoLabeler, BaoDecisionProducesDbOptimizerLabel) {
    DatabaseDomainAutoLabeler labeler;
    auto s = labeler.labelFromBaoDecision("SELECT 1", "{}", -20.0);
    EXPECT_EQ(s.label, DomainType::DATABASE_OPTIMIZER);
    EXPECT_EQ(s.source, "bao_log");
    EXPECT_EQ(s.query_text, "SELECT 1");
}

// 4. |delta_p99_ms| > 5ms → confidence > 0.5 (sigmoid is > 0.5 for any x > 0)
TEST(DbAutoLabeler, BaoDecisionConfidenceAboveThresholdFor5ms) {
    DatabaseDomainAutoLabeler labeler(10.0);  // sensitivity = 10 ms
    auto s = labeler.labelFromBaoDecision("SELECT count(*) FROM t", "{}", -5.1);
    EXPECT_GT(s.confidence, 0.5);
}

// 5. Negative DBA feedback → confidence ≥ 0.9
TEST(DbAutoLabeler, DBAFeedbackNegativeConfidenceAtLeast09) {
    DatabaseDomainAutoLabeler labeler;
    FeedbackEntry e;
    e.query_text   = "DELETE FROM t WHERE id = 1";
    e.plan_json    = "{}";
    e.is_positive  = false;
    e.delta_p99_ms = -0.1;   // tiny delta → sigmoid alone would be ~0.50
    e.source_id    = "dba_1";

    auto s = labeler.labelFromDBAFeedback(e);
    EXPECT_GE(s.confidence, 0.9);
    EXPECT_EQ(s.source, "dba_feedback");
}

// 6. Positive DBA feedback uses normal sigmoid (not forced to 0.9)
TEST(DbAutoLabeler, DBAFeedbackPositiveUsesNormalSigmoid) {
    DatabaseDomainAutoLabeler labeler(10.0);
    FeedbackEntry e;
    e.query_text   = "SELECT * FROM orders";
    e.plan_json    = "{}";
    e.is_positive  = true;
    e.delta_p99_ms = 2.0;   // small delta → confidence well below 0.9
    e.source_id    = "dba_2";

    auto s = labeler.labelFromDBAFeedback(e);
    // sigmoid(2/10) = sigmoid(0.2) ≈ 0.55 — well below 0.9
    EXPECT_LT(s.confidence, 0.9);
    EXPECT_GT(s.confidence, 0.5);
}

// 7. labelFromLogFile returns empty when the file does not exist
TEST(DbAutoLabeler, LabelFromLogFileEmptyWhenFileMissing) {
    DatabaseDomainAutoLabeler labeler;
    auto samples = labeler.labelFromLogFile("/tmp/nonexistent_log_file_xyz.log");
    EXPECT_TRUE(samples.empty());
}

// 8. labelFromLogFile applies min_confidence filter
TEST(DbAutoLabeler, LabelFromLogFileFiltersMinConfidence) {
    // Line 1: delta = 50ms → high confidence (passes 0.8 filter)
    // Line 2: delta =  0ms → confidence = 0.5 (filtered out by 0.8)
    const std::string log_content =
        R"({"query": "SELECT a FROM t1", "plan": "{}", "delta_p99_ms": -50.0})" "\n"
        R"({"query": "SELECT b FROM t2", "plan": "{}", "delta_p99_ms": 0.0})" "\n";

    auto path = makeTempFile("filter_test", log_content);

    DatabaseDomainAutoLabeler labeler(10.0);
    auto samples = labeler.labelFromLogFile(path.string(), 0, 0.8);

    EXPECT_EQ(samples.size(), 1u);
    EXPECT_EQ(samples[0].query_text, "SELECT a FROM t1");
    EXPECT_GE(samples[0].confidence, 0.8);

    fs::remove(path);
}
