#include <gtest/gtest.h>
#include "governance/compliance_reporter.h"
#include "governance/policy_manager.h"
#include <nlohmann/json.hpp>
#include <climits>
#include <memory>
#include <string>

using namespace themis::governance;
using json = nlohmann::json;

// ─── helpers ─────────────────────────────────────────────────────────────────

static RuleEvaluationEntry makeEntry(
    int64_t ts_ms,
    const std::string& route         = "/api/data/test",
    const std::string& classification = "vs-nfd",
    const std::string& mode           = "enforce",
    bool   require_encryption         = false,
    bool   ccpa_opted_out             = false,
    bool   export_allowed             = true,
    const std::string& user_id        = "alice")
{
    RuleEvaluationEntry e;
    e.timestamp_ms              = ts_ms;
    e.route                     = route;
    e.classification            = classification;
    e.mode                      = mode;
    e.require_content_encryption = require_encryption;
    e.ccpa_opted_out            = ccpa_opted_out;
    e.export_allowed            = export_allowed;
    e.user_id                   = user_id;
    return e;
}

// ─── RuleEvaluationEntry::fromJson ───────────────────────────────────────────

TEST(RuleEvaluationEntryTest, FromJsonPopulatesAllFields) {
    json j = {
        {"timestamp",                  1000000LL},
        {"route",                      "/api/query"},
        {"classification",             "geheim"},
        {"mode",                       "enforce"},
        {"require_content_encryption", true},
        {"ccpa_opted_out",             true},
        {"export_allowed",             false},
        {"user_id",                    "bob"}
    };

    auto e = RuleEvaluationEntry::fromJson(j);

    EXPECT_EQ(e.timestamp_ms,              1000000LL);
    EXPECT_EQ(e.route,                     "/api/query");
    EXPECT_EQ(e.classification,            "geheim");
    EXPECT_EQ(e.mode,                      "enforce");
    EXPECT_TRUE(e.require_content_encryption);
    EXPECT_TRUE(e.ccpa_opted_out);
    EXPECT_FALSE(e.export_allowed);
    EXPECT_EQ(e.user_id,                   "bob");
}

TEST(RuleEvaluationEntryTest, FromJsonEmptyObjectYieldsDefaults) {
    auto e = RuleEvaluationEntry::fromJson(json::object());

    EXPECT_EQ(e.timestamp_ms,  0);
    EXPECT_TRUE(e.route.empty());
    EXPECT_TRUE(e.classification.empty());
    EXPECT_TRUE(e.mode.empty());
    EXPECT_FALSE(e.require_content_encryption);
    EXPECT_FALSE(e.ccpa_opted_out);
    EXPECT_TRUE(e.export_allowed);
    EXPECT_TRUE(e.user_id.empty());
}

TEST(RuleEvaluationEntryTest, FromJsonIgnoresUnknownKeys) {
    json j = {{"unknown_key", 42}, {"timestamp", 500LL}};
    auto e = RuleEvaluationEntry::fromJson(j);
    EXPECT_EQ(e.timestamp_ms, 500LL);
}

// ─── Test fixture ─────────────────────────────────────────────────────────────

class TimeWindowReportTest : public ::testing::Test {
protected:
    void SetUp() override {
        pm = std::make_shared<PolicyManager>();

        // Add a rule with required controls so the compliance score is not zero
        PolicyRule r;
        r.id                   = "rule-enc-audit";
        r.name                 = "Encryption + Audit Rule";
        r.enabled              = true;
        r.resources            = {"data/sensitive"};
        r.actions              = {"*"};
        r.classification_level = "geheim";
        r.require_encryption   = true;
        r.audit_access         = true;
        r.retention_days       = 365;
        pm->addRule(r);

        reporter = std::make_unique<ComplianceReporter>(pm);
    }

    std::shared_ptr<PolicyManager>   pm;
    std::unique_ptr<ComplianceReporter> reporter;
};

// ─── generateTimeWindowReport – empty input ──────────────────────────────────

TEST_F(TimeWindowReportTest, EmptyEntriesProducesZeroEvaluationCounts) {
    auto rpt = reporter->generateTimeWindowReport({});

    EXPECT_EQ(rpt.total_evaluations,         0);
    EXPECT_EQ(rpt.enforce_mode_evaluations,  0);
    EXPECT_EQ(rpt.observe_mode_evaluations,  0);
    EXPECT_EQ(rpt.ccpa_opted_out_count,      0);
    EXPECT_EQ(rpt.encryption_required_count, 0);
    EXPECT_EQ(rpt.export_blocked_count,      0);
    EXPECT_TRUE(rpt.evaluations_by_classification.empty());
    EXPECT_TRUE(rpt.evaluations_by_route.empty());
}

// ─── generateTimeWindowReport – all entries in window ────────────────────────

TEST_F(TimeWindowReportTest, AllEntriesCountedWhenNoWindowFilter) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(1000, "/api/a", "vs-nfd",  "enforce", false, false, true),
        makeEntry(2000, "/api/b", "geheim",  "enforce", true,  false, true),
        makeEntry(3000, "/api/c", "vs-nfd",  "observe", false, true,  false),
    };

    auto rpt = reporter->generateTimeWindowReport(entries);

    EXPECT_EQ(rpt.total_evaluations,         3);
    EXPECT_EQ(rpt.enforce_mode_evaluations,  2);
    EXPECT_EQ(rpt.observe_mode_evaluations,  1);
    EXPECT_EQ(rpt.ccpa_opted_out_count,      1);
    EXPECT_EQ(rpt.encryption_required_count, 1);
    EXPECT_EQ(rpt.export_blocked_count,      1);
}

// ─── generateTimeWindowReport – window filtering ─────────────────────────────

TEST_F(TimeWindowReportTest, EntriesOutsideWindowAreExcluded) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(  500),  // before window
        makeEntry( 1000),  // inside window (start boundary)
        makeEntry( 2000),  // inside window
        makeEntry( 3000),  // inside window (end boundary)
        makeEntry(10000),  // after window
    };

    auto rpt = reporter->generateTimeWindowReport(entries, /*start=*/1000, /*end=*/3000);

    EXPECT_EQ(rpt.total_evaluations, 3);
    EXPECT_EQ(rpt.window_start_ms,   1000);
    EXPECT_EQ(rpt.window_end_ms,     3000);
}

TEST_F(TimeWindowReportTest, AllEntriesFilteredWhenWindowExcludesAll) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(100),
        makeEntry(200),
    };

    auto rpt = reporter->generateTimeWindowReport(entries, /*start=*/5000, /*end=*/9000);

    EXPECT_EQ(rpt.total_evaluations, 0);
}

// ─── generateTimeWindowReport – classification breakdown ─────────────────────

TEST_F(TimeWindowReportTest, ClassificationBreakdownCorrect) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(100, "/a", "vs-nfd"),
        makeEntry(200, "/b", "vs-nfd"),
        makeEntry(300, "/c", "geheim"),
    };

    auto rpt = reporter->generateTimeWindowReport(entries);

    ASSERT_EQ(rpt.evaluations_by_classification.count("vs-nfd"), 1u);
    ASSERT_EQ(rpt.evaluations_by_classification.count("geheim"), 1u);
    EXPECT_EQ(rpt.evaluations_by_classification.at("vs-nfd"), 2);
    EXPECT_EQ(rpt.evaluations_by_classification.at("geheim"), 1);
}

// ─── generateTimeWindowReport – route breakdown ──────────────────────────────

TEST_F(TimeWindowReportTest, RouteBreakdownCorrect) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(100, "/api/data"),
        makeEntry(200, "/api/data"),
        makeEntry(300, "/api/admin"),
    };

    auto rpt = reporter->generateTimeWindowReport(entries);

    ASSERT_EQ(rpt.evaluations_by_route.count("/api/data"),  1u);
    ASSERT_EQ(rpt.evaluations_by_route.count("/api/admin"), 1u);
    EXPECT_EQ(rpt.evaluations_by_route.at("/api/data"),  2);
    EXPECT_EQ(rpt.evaluations_by_route.at("/api/admin"), 1);
}

// ─── generateTimeWindowReport – framework label ──────────────────────────────

TEST_F(TimeWindowReportTest, FrameworkLabelStoredInReport) {
    auto rpt = reporter->generateTimeWindowReport({}, 0, INT64_MAX, "GDPR");
    EXPECT_EQ(rpt.framework, "GDPR");
}

TEST_F(TimeWindowReportTest, EmptyFrameworkLabelStoredAsEmpty) {
    auto rpt = reporter->generateTimeWindowReport({});
    EXPECT_TRUE(rpt.framework.empty());
}

// ─── generateTimeWindowReport – compliance score and gaps ────────────────────

TEST_F(TimeWindowReportTest, ComplianceScoreInRange) {
    auto rpt = reporter->generateTimeWindowReport({});
    EXPECT_GE(rpt.compliance_score, 0.0);
    EXPECT_LE(rpt.compliance_score, 100.0);
}

TEST_F(TimeWindowReportTest, GeneratedAtIsPositive) {
    auto rpt = reporter->generateTimeWindowReport({});
    EXPECT_GT(rpt.generated_at, 0);
}

// ─── TimeWindowReport::toJson ────────────────────────────────────────────────

TEST_F(TimeWindowReportTest, ToJsonContainsAllRequiredKeys) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(1000, "/api/x", "offen", "enforce", true, false, true),
    };
    auto rpt = reporter->generateTimeWindowReport(entries, 0, INT64_MAX, "HIPAA");
    auto j = rpt.toJson();

    EXPECT_TRUE(j.contains("window_start_ms"));
    EXPECT_TRUE(j.contains("window_end_ms"));
    EXPECT_TRUE(j.contains("generated_at"));
    EXPECT_TRUE(j.contains("framework"));
    EXPECT_TRUE(j.contains("total_evaluations"));
    EXPECT_TRUE(j.contains("enforce_mode_evaluations"));
    EXPECT_TRUE(j.contains("observe_mode_evaluations"));
    EXPECT_TRUE(j.contains("ccpa_opted_out_count"));
    EXPECT_TRUE(j.contains("encryption_required_count"));
    EXPECT_TRUE(j.contains("export_blocked_count"));
    EXPECT_TRUE(j.contains("compliance_score"));
    EXPECT_TRUE(j.contains("evaluations_by_classification"));
    EXPECT_TRUE(j.contains("evaluations_by_route"));
    EXPECT_TRUE(j.contains("gaps"));
}

TEST_F(TimeWindowReportTest, ToJsonValuesMatchReport) {
    std::vector<RuleEvaluationEntry> entries = {
        makeEntry(500, "/api/q", "geheim", "enforce", true, true, false),
    };
    auto rpt = reporter->generateTimeWindowReport(entries, 0, INT64_MAX, "SOC2");
    auto j = rpt.toJson();

    EXPECT_EQ(j["total_evaluations"].get<int>(),            1);
    EXPECT_EQ(j["enforce_mode_evaluations"].get<int>(),     1);
    EXPECT_EQ(j["observe_mode_evaluations"].get<int>(),     0);
    EXPECT_EQ(j["ccpa_opted_out_count"].get<int>(),         1);
    EXPECT_EQ(j["encryption_required_count"].get<int>(),    1);
    EXPECT_EQ(j["export_blocked_count"].get<int>(),         1);
    EXPECT_EQ(j["framework"].get<std::string>(),            "SOC2");
}

// ─── TimeWindowReport::toCSV ──────────────────────────────────────────────────

TEST_F(TimeWindowReportTest, ToCSVContainsHeaderAndDataRow) {
    auto rpt = reporter->generateTimeWindowReport({});
    auto csv = rpt.toCSV();

    EXPECT_NE(csv.find("total_evaluations"), std::string::npos);
    EXPECT_NE(csv.find("compliance_score"),  std::string::npos);
    // Two lines: header + data
    size_t line_count = 0;
    for (char c : csv)
        if (c == '\n') {
          ++line_count;
        }
    EXPECT_GE(line_count, 2u);
}
