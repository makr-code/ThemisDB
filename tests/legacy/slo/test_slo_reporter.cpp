/**
 * @file test_slo_reporter.cpp
 * @brief Unit tests for SloReporter – SLO/SLA compliance reporting with
 *        multi-window burn-rate alerting.
 *
 * Tests cover:
 *  - SloDefinition default values
 *  - BurnRateLevel helpers (multiplier, severity strings)
 *  - registerSlo / sloCount
 *  - record() for known and unknown SLO names
 *  - getStatus(): SLI calculation, error budget, burn rate, slo_met flag
 *  - getStatus() with no samples returns full compliance
 *  - getAllStatuses() covers all registered SLOs
 *  - Burn-rate alerts: FAST / MEDIUM / SLOW thresholds
 *  - No alerts when error rate is within budget
 *  - publishMetrics() pushes gauges to MetricsCollector
 *  - generateReport() contains key fields
 *  - generateReportJson() has expected JSON structure
 *  - clear() removes all SLOs and data
 *  - SloStatus::toJson() produces valid JSON
 *  - Sliding window expiry (old samples outside window are ignored)
 *  - getStatus() throws for unknown SLO name
 */

#include <gtest/gtest.h>
#include "observability/slo_reporter.h"
#include "observability/metrics_collector.h"

#include <chrono>
#include <string>
#include <thread>

using namespace themis::observability;
using namespace std::chrono_literals;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static SloDefinition makeSlo(const std::string& name,
                              double objective = 0.999,
                              std::chrono::seconds window = std::chrono::hours(1)) {
    SloDefinition slo;
    slo.name      = name;
    slo.objective = objective;
    slo.window    = window;
    return slo;
}

/// Inject @p good good-requests and @p bad bad-requests with a timestamp
/// @p age_offset before now.
static void injectRequests(SloReporter& reporter,
                            const std::string& slo_name,
                            size_t good, size_t bad,
                            std::chrono::seconds age_offset = 0s) {
    auto ts = std::chrono::system_clock::now() - age_offset;
    for (size_t i = 0; i < good; ++i) {
      reporter.record(slo_name, true,  ts);
    }
    for (size_t i = 0; i < bad;  ++i) {
      reporter.record(slo_name, false, ts);
    }
}

// ---------------------------------------------------------------------------
// BurnRateLevel helpers
// ---------------------------------------------------------------------------

TEST(BurnRateLevelTest, MultiplierValues) {
    EXPECT_DOUBLE_EQ(14.4, burnRateMultiplier(BurnRateLevel::FAST));
    EXPECT_DOUBLE_EQ(6.0,  burnRateMultiplier(BurnRateLevel::MEDIUM));
    EXPECT_DOUBLE_EQ(3.0,  burnRateMultiplier(BurnRateLevel::SLOW));
}

TEST(BurnRateLevelTest, SeverityStrings) {
    EXPECT_STREQ("critical", burnRateSeverity(BurnRateLevel::FAST));
    EXPECT_STREQ("warning",  burnRateSeverity(BurnRateLevel::MEDIUM));
    EXPECT_STREQ("info",     burnRateSeverity(BurnRateLevel::SLOW));
}

// ---------------------------------------------------------------------------
// SloDefinition defaults
// ---------------------------------------------------------------------------

TEST(SloDefinitionTest, DefaultValues) {
    SloDefinition slo;
    EXPECT_TRUE(slo.name.empty());
    EXPECT_DOUBLE_EQ(0.999, slo.objective);
    EXPECT_EQ(std::chrono::hours(24), slo.window);
}

// ---------------------------------------------------------------------------
// SloReporter – registration and sloCount
// ---------------------------------------------------------------------------

class SloReporterTest : public ::testing::Test {
protected:
    SloReporter reporter;
};

TEST_F(SloReporterTest, InitiallyEmpty) {
    EXPECT_EQ(0u, reporter.sloCount());
}

TEST_F(SloReporterTest, RegisterOneSlo_CountIsOne) {
    reporter.registerSlo(makeSlo("avail"));
    EXPECT_EQ(1u, reporter.sloCount());
}

TEST_F(SloReporterTest, RegisterMultipleSlos) {
    reporter.registerSlo(makeSlo("avail"));
    reporter.registerSlo(makeSlo("latency"));
    EXPECT_EQ(2u, reporter.sloCount());
}

TEST_F(SloReporterTest, ReRegisterReplacesSlo) {
    reporter.registerSlo(makeSlo("avail", 0.999));
    reporter.registerSlo(makeSlo("avail", 0.9999));  // replace
    EXPECT_EQ(1u, reporter.sloCount());
    auto status = reporter.getStatus("avail");
    EXPECT_DOUBLE_EQ(0.9999, status.objective);
}

// ---------------------------------------------------------------------------
// record() – unknown SLO silently ignored
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, RecordUnknownSlo_DoesNotThrow) {
    EXPECT_NO_THROW(reporter.record("nonexistent", true));
}

// ---------------------------------------------------------------------------
// getStatus() – throws for unknown SLO
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, GetStatusUnknown_Throws) {
    EXPECT_THROW(reporter.getStatus("no_such_slo"), std::out_of_range);
}

// ---------------------------------------------------------------------------
// getStatus() – no samples: full compliance
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, NoSamples_FullCompliance) {
    reporter.registerSlo(makeSlo("avail", 0.999));
    auto s = reporter.getStatus("avail");

    EXPECT_DOUBLE_EQ(1.0,  s.current_sli);
    EXPECT_DOUBLE_EQ(1.0,  s.error_budget_remaining);
    EXPECT_DOUBLE_EQ(0.0,  s.burn_rate);
    EXPECT_TRUE(s.slo_met);
    EXPECT_EQ(0u, s.total_requests);
    EXPECT_TRUE(s.active_burn_rate_alerts.empty());
}

// ---------------------------------------------------------------------------
// SLI and error budget computation
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, AllGoodRequests_SloMet) {
    reporter.registerSlo(makeSlo("avail", 0.999));
    injectRequests(reporter, "avail", 1000, 0);

    auto s = reporter.getStatus("avail");
    EXPECT_DOUBLE_EQ(1.0, s.current_sli);
    EXPECT_TRUE(s.slo_met);
    EXPECT_DOUBLE_EQ(1.0, s.error_budget_remaining);
    EXPECT_NEAR(0.0, s.burn_rate, 1e-9);
}

TEST_F(SloReporterTest, ErrorRateEqualsBudget_BurnRateIsOne) {
    // 99.9% SLO → error budget = 0.1%
    // Inject exactly 0.1% errors → burn rate should be ≈ 1.0
    reporter.registerSlo(makeSlo("avail", 0.999));
    // 1000 good + 1 bad = 0.999... SLI → error = 0.0009...
    // Burn rate ≈ 0.0009... / 0.001 ≈ 0.999 (≈ 1)
    injectRequests(reporter, "avail", 999, 1);

    auto s = reporter.getStatus("avail");
    EXPECT_TRUE(s.slo_met);  // 999/1000 = 0.999 meets objective exactly
    EXPECT_NEAR(1.0, s.burn_rate, 0.01);
}

TEST_F(SloReporterTest, HighErrorRate_SloViolated) {
    reporter.registerSlo(makeSlo("avail", 0.999));
    // 5% error rate → far above the 0.1% budget
    injectRequests(reporter, "avail", 950, 50);

    auto s = reporter.getStatus("avail");
    EXPECT_FALSE(s.slo_met);
    EXPECT_LT(s.error_budget_remaining, 0.0 + 1e-9);  // budget exhausted
    EXPECT_GT(s.burn_rate, 10.0);
}

TEST_F(SloReporterTest, CorrectTotalAndErrorCounts) {
    reporter.registerSlo(makeSlo("avail", 0.99));
    injectRequests(reporter, "avail", 80, 20);

    auto s = reporter.getStatus("avail");
    EXPECT_EQ(100u, s.total_requests);
    EXPECT_EQ(20u,  s.error_requests);
}

// ---------------------------------------------------------------------------
// getAllStatuses
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, GetAllStatuses_ReturnsAll) {
    reporter.registerSlo(makeSlo("a1"));
    reporter.registerSlo(makeSlo("a2"));
    reporter.registerSlo(makeSlo("a3"));

    auto statuses = reporter.getAllStatuses();
    EXPECT_EQ(3u, statuses.size());
}

// ---------------------------------------------------------------------------
// Burn-rate alerts
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, NoAlerts_WhenWithinBudget) {
    // 0 errors → no burn-rate alert
    reporter.registerSlo(makeSlo("avail", 0.999));
    injectRequests(reporter, "avail", 1000, 0);

    auto s = reporter.getStatus("avail");
    EXPECT_TRUE(s.active_burn_rate_alerts.empty());
}

TEST_F(SloReporterTest, FastBurnAlert_Fires) {
    // 99.9% SLO → allowed error rate = 0.001
    // Fast burn threshold = 14.4×  → need error rate > 1.44%
    // Inject 2% error rate → burn_rate ≈ 20 > 14.4 → FAST fires
    reporter.registerSlo(makeSlo("avail", 0.999, std::chrono::hours(1)));
    injectRequests(reporter, "avail", 980, 20);  // 2% error rate

    auto s = reporter.getStatus("avail");
    bool fast_found = false;
    for (const auto& alert : s.active_burn_rate_alerts) {
        if (alert.level == BurnRateLevel::FAST) {
            fast_found = true;
            EXPECT_STREQ("critical", alert.severity.c_str());
            EXPECT_GT(alert.burn_rate, 14.4);
            EXPECT_FALSE(alert.message.empty());
        }
    }
    EXPECT_TRUE(fast_found) << "Expected FAST burn-rate alert";
}

TEST_F(SloReporterTest, MediumBurnAlert_Fires_WhenBurnRateExceeds6x) {
    // 99.9% SLO → error budget = 0.1%
    // 6× threshold → need error rate > 0.6%
    reporter.registerSlo(makeSlo("avail", 0.999, std::chrono::hours(6)));
    // 1% error rate → burn rate ≈ 10× > 6× → MEDIUM fires
    injectRequests(reporter, "avail", 990, 10);

    auto s = reporter.getStatus("avail");
    bool medium_found = false;
    for (const auto& alert : s.active_burn_rate_alerts) {
        if (alert.level == BurnRateLevel::MEDIUM) {
            medium_found = true;
            EXPECT_STREQ("warning", alert.severity.c_str());
        }
    }
    EXPECT_TRUE(medium_found) << "Expected MEDIUM burn-rate alert";
}

TEST_F(SloReporterTest, AlertMessage_ContainsSloName) {
    reporter.registerSlo(makeSlo("my_special_slo", 0.999, std::chrono::hours(1)));
    injectRequests(reporter, "my_special_slo", 950, 50);

    auto s = reporter.getStatus("my_special_slo");
    for (const auto& alert : s.active_burn_rate_alerts) {
        EXPECT_NE(std::string::npos, alert.message.find("my_special_slo"))
            << "Alert message must contain SLO name: " << alert.message;
    }
}

// ---------------------------------------------------------------------------
// Sliding window expiry
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, OldSamplesExpired_OutsideWindow) {
    // Window = 1 hour; inject 10 errors 2 hours ago → should be expired
    reporter.registerSlo(makeSlo("avail", 0.999, std::chrono::hours(1)));
    injectRequests(reporter, "avail", 0, 10, std::chrono::hours(2));
    // Inject 990 good requests now
    injectRequests(reporter, "avail", 990, 0, 0s);

    auto s = reporter.getStatus("avail");
    // The old errors are outside the 1h window → should not count
    EXPECT_EQ(990u, s.total_requests);
    EXPECT_EQ(0u,   s.error_requests);
    EXPECT_TRUE(s.slo_met);
}

TEST_F(SloReporterTest, RecentSamplesWithinWindow_CountCorrectly) {
    reporter.registerSlo(makeSlo("avail", 0.999, std::chrono::hours(1)));
    // 30-minute old requests – should still be in window
    injectRequests(reporter, "avail", 900, 100, std::chrono::minutes(30));

    auto s = reporter.getStatus("avail");
    EXPECT_EQ(1000u, s.total_requests);
    EXPECT_EQ(100u,  s.error_requests);
}

// ---------------------------------------------------------------------------
// publishMetrics
// ---------------------------------------------------------------------------

class SloReporterMetricsTest : public ::testing::Test {
protected:
    void SetUp() override {
        MetricsCollector::getInstance().reset();
    }
    void TearDown() override {
        MetricsCollector::getInstance().reset();
    }
    SloReporter reporter;
};

TEST_F(SloReporterMetricsTest, PublishMetrics_GaugesAppearInPrometheusOutput) {
    reporter.registerSlo(makeSlo("avail", 0.999));
    injectRequests(reporter, "avail", 990, 10);
    reporter.publishMetrics();

    std::string prom = MetricsCollector::getInstance().getPrometheusMetrics();
    EXPECT_NE(std::string::npos, prom.find("themis_slo_current_sli"))
        << "Expected themis_slo_current_sli in metrics";
    EXPECT_NE(std::string::npos, prom.find("themis_slo_error_budget_remaining"))
        << "Expected error budget gauge";
    EXPECT_NE(std::string::npos, prom.find("themis_slo_burn_rate"))
        << "Expected burn_rate gauge";
    EXPECT_NE(std::string::npos, prom.find("themis_slo_met"))
        << "Expected slo_met gauge";
}

// ---------------------------------------------------------------------------
// generateReport / generateReportJson
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, GenerateReport_ContainsKeyFields) {
    reporter.registerSlo(makeSlo("query_avail", 0.999));
    injectRequests(reporter, "query_avail", 1000, 0);

    std::string report = reporter.generateReport();
    EXPECT_NE(std::string::npos, report.find("query_avail"))   << report;
    EXPECT_NE(std::string::npos, report.find("SLO Target"))     << report;
    EXPECT_NE(std::string::npos, report.find("Current SLI"))    << report;
    EXPECT_NE(std::string::npos, report.find("Error Budget"))   << report;
    EXPECT_NE(std::string::npos, report.find("Burn Rate"))      << report;
    EXPECT_NE(std::string::npos, report.find("MET"))            << report;
}

TEST_F(SloReporterTest, GenerateReport_ShowsViolationStatus) {
    reporter.registerSlo(makeSlo("query_avail", 0.999));
    injectRequests(reporter, "query_avail", 900, 100);

    std::string report = reporter.generateReport();
    EXPECT_NE(std::string::npos, report.find("VIOLATED")) << report;
}

TEST_F(SloReporterTest, GenerateReportJson_ContainsExpectedFields) {
    reporter.registerSlo(makeSlo("write_avail", 0.9999));
    injectRequests(reporter, "write_avail", 1000, 0);

    auto j = reporter.generateReportJson();

    EXPECT_TRUE(j.contains("generated_at_ms"));
    EXPECT_TRUE(j.contains("slo_count"));
    EXPECT_TRUE(j.contains("slos"));
    EXPECT_EQ(1u, j.at("slo_count").get<size_t>());

    auto slos = j.at("slos");
    ASSERT_EQ(1u, slos.size());
    auto& slo0 = slos[0];
    EXPECT_TRUE(slo0.contains("name"));
    EXPECT_TRUE(slo0.contains("objective"));
    EXPECT_TRUE(slo0.contains("current_sli"));
    EXPECT_TRUE(slo0.contains("slo_met"));
    EXPECT_TRUE(slo0.contains("burn_rate"));
    EXPECT_EQ("write_avail", slo0.at("name").get<std::string>());
}

// ---------------------------------------------------------------------------
// SloStatus::toJson
// ---------------------------------------------------------------------------

TEST(SloStatusTest, ToJson_AllFieldsPresent) {
    SloStatus s;
    s.name                   = "test_slo";
    s.objective              = 0.99;
    s.current_sli            = 0.995;
    s.error_budget_total     = 0.01;
    s.error_budget_remaining = 0.5;
    s.total_requests         = 100;
    s.error_requests         = 5;
    s.burn_rate              = 5.0;
    s.slo_met                = true;

    auto j = s.toJson();

    EXPECT_EQ("test_slo", j.at("name").get<std::string>());
    EXPECT_NEAR(0.99,  j.at("objective").get<double>(),              1e-9);
    EXPECT_NEAR(0.995, j.at("current_sli").get<double>(),            1e-9);
    EXPECT_NEAR(0.5,   j.at("error_budget_remaining").get<double>(), 1e-9);
    EXPECT_EQ(100u,    j.at("total_requests").get<uint64_t>());
    EXPECT_EQ(5u,      j.at("error_requests").get<uint64_t>());
    EXPECT_NEAR(5.0,   j.at("burn_rate").get<double>(),              1e-9);
    EXPECT_TRUE(j.at("slo_met").get<bool>());
    EXPECT_TRUE(j.at("active_burn_rate_alerts").is_array());
}

// ---------------------------------------------------------------------------
// clear()
// ---------------------------------------------------------------------------

TEST_F(SloReporterTest, Clear_RemovesAllSlos) {
    reporter.registerSlo(makeSlo("a"));
    reporter.registerSlo(makeSlo("b"));
    reporter.clear();
    EXPECT_EQ(0u, reporter.sloCount());
    EXPECT_THROW(reporter.getStatus("a"), std::out_of_range);
}
