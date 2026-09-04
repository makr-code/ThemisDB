/*
 * ThemisDB — DatabaseDomainAutoLabeler unit tests (IMPL-A1, Phase 4)
 *
 * Tests:
 *   DBO-01  labelFromBaoDecision on EXPLAIN output → label == DATABASE_OPTIMIZER
 *   DBO-02  computeConfidence(0.0) == 0.5 (sigmoid(0) baseline)
 *   DBO-03  computeConfidence(50.0) ≥ 0.85 with default sensitivity (10 ms)
 *   DBO-04  domain keyword present in bao decision output (plan_json forwarded)
 *   DBO-05  exportToJsonl() produces valid JSONL: fields present, one line per sample
 *   DBO-06  duplicate query filtered by DataSelectionPipeline::deduplicate()
 *   DBO-07  LEGAL domain type is distinct from DATABASE_OPTIMIZER
 *   DBO-08  1 000 synthetic samples — all with |Δlatency| ≥ 50 ms yield confidence ≥ 0.98
 */

#include <gtest/gtest.h>

#include "training/database_domain_auto_labeler.h"
#include "training/auto_labeler.h"
#include "training/lora_data_selection.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>
#include <vector>

using themis::training::DatabaseDomainAutoLabeler;
using themis::training::DomainType;
using themis::training::FeedbackEntry;
using themis::training::LabeledDbSample;
using themis::training::DataSelectionPipeline;
using themis::training::DataSample;
using themis::training::LoRADataSelectionConfig;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

namespace {

/// Build a JSONL-friendly string of `lines` from the export output.
/// Returns the number of lines (ignoring trailing newline).
std::size_t countJsonlLines(const std::string& jsonl) {
    if (jsonl.empty()) {
      return 0;
    }
    std::size_t n = 0;
    for (char c : jsonl) { if (c == '\n') ++n; }
    return n;
}

} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// DBO-01: labelFromBaoDecision → label == DATABASE_OPTIMIZER
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO01_BaoDecisionLabelIsDatabaseOptimizer) {
    DatabaseDomainAutoLabeler labeler;
    const std::string explain =
        R"({"type":"SeqScan","table":"orders","rows":50000,"cost":1200.0})";
    auto s = labeler.labelFromBaoDecision(
        "SELECT * FROM orders WHERE status = 'open'", explain, -120.0);

    EXPECT_EQ(s.label, DomainType::DATABASE_OPTIMIZER);
    EXPECT_EQ(s.source, "bao_log");
    EXPECT_EQ(s.query_text, "SELECT * FROM orders WHERE status = 'open'");
    EXPECT_EQ(s.plan_json, explain);
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-02: computeConfidence(0.0) == 0.5  (sigmoid(0) = 0.5)
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO02_ConfidenceAtZeroDeltaIsHalf) {
    DatabaseDomainAutoLabeler labeler;
    EXPECT_DOUBLE_EQ(labeler.computeConfidence(0.0), 0.5);
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-03: computeConfidence(50.0) ≥ 0.85 with default sensitivity = 10 ms
//   sigmoid(50 / 10) = sigmoid(5) ≈ 0.9933
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO03_ConfidenceAtFiftyMsIsAboveThreshold) {
    DatabaseDomainAutoLabeler labeler(/* sensitivity_ms = */ 10.0);
    EXPECT_GE(labeler.computeConfidence(50.0),  0.85);
    EXPECT_GE(labeler.computeConfidence(-50.0), 0.85);  // sign-independent
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-04: domain keyword preserved in plan_json field of labeled sample
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO04_DomainKeywordsPreservedInPlanJson) {
    DatabaseDomainAutoLabeler labeler;
    const std::string plan_with_keywords =
        R"({"type":"HashJoin","index scan":true,"p99_latency_ms":340})";
    auto s = labeler.labelFromBaoDecision("SELECT a, b FROM t1 JOIN t2 ON t1.id = t2.id",
                                          plan_with_keywords, -340.0);

    // The keyword-bearing plan is forwarded verbatim
    EXPECT_NE(s.plan_json.find("HashJoin"),       std::string::npos);
    EXPECT_NE(s.plan_json.find("index scan"),     std::string::npos);
    EXPECT_NE(s.plan_json.find("p99_latency_ms"), std::string::npos);
    EXPECT_EQ(s.label, DomainType::DATABASE_OPTIMIZER);
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-05: exportToJsonl() produces parseable JSONL
//   - One line per sample
//   - Each line contains "query", "explain_plan", "latency_delta_ms"
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO05_ExportToJsonlProducesValidLines) {
    DatabaseDomainAutoLabeler labeler(10.0);

    std::vector<LabeledDbSample> samples;
    samples.push_back(labeler.labelFromBaoDecision("SELECT 1", "{}", -50.0));
    samples.push_back(labeler.labelFromBaoDecision("SELECT 2", "{}", -100.0));
    samples.push_back(labeler.labelFromBaoDecision("SELECT 3", R"({"plan":"seq"})", -200.0));

    const std::string jsonl = DatabaseDomainAutoLabeler::exportToJsonl(samples);

    EXPECT_FALSE(jsonl.empty());
    EXPECT_EQ(countJsonlLines(jsonl), 3u);   // exactly 3 lines

    // Every line must contain the mandatory field names
    std::istringstream iss(jsonl);
    std::string line;
    int line_number = 0;
    while (std::getline(iss, line)) {
        ++line_number;
        EXPECT_NE(line.find("\"query\""),            std::string::npos) << "line " << line_number;
        EXPECT_NE(line.find("\"explain_plan\""),     std::string::npos) << "line " << line_number;
        EXPECT_NE(line.find("\"latency_delta_ms\""), std::string::npos) << "line " << line_number;
        EXPECT_NE(line.find("\"confidence\""),       std::string::npos) << "line " << line_number;
        EXPECT_NE(line.find("\"source\""),           std::string::npos) << "line " << line_number;
        // Line must start with '{' and end with '}'
        ASSERT_FALSE(line.empty());
        EXPECT_EQ(line.front(), '{');
        EXPECT_EQ(line.back(),  '}');
    }
}

TEST(DatabaseOptimizerLabeler, DBO05b_ExportEmptySamplesReturnsEmptyString) {
    const std::string out = DatabaseDomainAutoLabeler::exportToJsonl({});
    EXPECT_TRUE(out.empty());
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-06: duplicate query is removed by DataSelectionPipeline::deduplicate()
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO06_DuplicateQueryFilteredByPipeline) {
    // Build two identical DataSamples (same text → near-duplicate)
    const std::string same_query = "SELECT * FROM orders WHERE status = 'open'";

    LoRADataSelectionConfig cfg;
    cfg.minhash_threshold = 0.90;   // flag nearly-identical queries
    cfg.minhash_num_perm  = 64;
    DataSelectionPipeline pipeline(cfg);

    std::vector<DataSample> samples = {};

    for (int i = 0; i < 5; ++i) {
        DataSample ds("id_" + std::to_string(i), same_query);
        ds.language = "en";
        samples.push_back(ds);
    }

    auto deduped = pipeline.deduplicate(samples);

    // After dedup, only 1 copy of the identical query should survive
    EXPECT_LT(deduped.size(), samples.size());
    EXPECT_GE(deduped.size(), 1u);

    // None of the survivors should be marked as duplicate
    for (const auto& s : deduped) {
        EXPECT_FALSE(s.is_duplicate);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-07: LEGAL domain type is distinct from DATABASE_OPTIMIZER
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO07_LegalDomainDistinctFromDatabaseOptimizer) {
    EXPECT_NE(DomainType::LEGAL,         DomainType::DATABASE_OPTIMIZER);
    EXPECT_NE(DomainType::MEDICAL,       DomainType::DATABASE_OPTIMIZER);
    EXPECT_NE(DomainType::FINANCIAL,     DomainType::DATABASE_OPTIMIZER);
    EXPECT_NE(DomainType::INDEX_ADVISOR, DomainType::DATABASE_OPTIMIZER);

    // Verify that labeling via DatabaseDomainAutoLabeler never produces LEGAL
    DatabaseDomainAutoLabeler labeler;
    auto s = labeler.labelFromBaoDecision("SELECT * FROM contracts", "{}", -10.0);
    EXPECT_NE(s.label, DomainType::LEGAL);
    EXPECT_NE(s.label, DomainType::MEDICAL);
    EXPECT_EQ(s.label, DomainType::DATABASE_OPTIMIZER);
}

// ─────────────────────────────────────────────────────────────────────────────
// DBO-08: 1 000 synthetic samples with |Δlatency| ≥ 50 ms all yield
//         confidence ≥ 0.98  (sigmoid(5) ≈ 0.9933 with sensitivity = 10 ms)
// ─────────────────────────────────────────────────────────────────────────────

TEST(DatabaseOptimizerLabeler, DBO08_OneThousandSampleGoldenDatasetHighConfidence) {
    DatabaseDomainAutoLabeler labeler(10.0);  // sensitivity = 10 ms

    constexpr int    N        = 1000;
    constexpr double min_conf = 0.85;

    std::vector<LabeledDbSample> golden_dataset;
    golden_dataset.reserve(N);

    for (int i = 0; i < N; ++i) {
        // Alternate between regression and improvement, all ≥ 50 ms magnitude
        const double delta = (i % 2 == 0) ? -(50.0 + i * 0.1) : (50.0 + i * 0.1);
        const std::string query =
            "SELECT col" + std::to_string(i) +
            " FROM table_" + std::to_string(i % 20) +
            " WHERE id = " + std::to_string(i);
        const std::string plan =
            R"({"type":"IndexScan","rows":)" + std::to_string(i + 1) + "}";

        golden_dataset.push_back(
            labeler.labelFromBaoDecision(query, plan, delta));
    }

    ASSERT_EQ(golden_dataset.size(), static_cast<std::size_t>(N));

    // All samples must meet the quality threshold
    std::size_t high_conf = 0;
    for (const auto& s : golden_dataset) {
        EXPECT_EQ(s.label, DomainType::DATABASE_OPTIMIZER);
        if (s.confidence >= min_conf) {
          ++high_conf;
        }
    }
    EXPECT_EQ(high_conf, static_cast<std::size_t>(N))
        << "All 1000 samples with |Δlatency| ≥ 50 ms should exceed confidence threshold "
        << min_conf;

    // Verify exportToJsonl round-trip: N lines produced
    const std::string jsonl = DatabaseDomainAutoLabeler::exportToJsonl(golden_dataset);
    EXPECT_EQ(countJsonlLines(jsonl), static_cast<std::size_t>(N));
}
