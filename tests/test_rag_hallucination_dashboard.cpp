/**
 * @file test_rag_hallucination_dashboard.cpp
 * @brief Unit tests for the hallucination rate tracking dashboard
 *
 * Tests cover:
 *  - Initial state (empty dashboard)
 *  - Basic recording via record() and recordFaithfulness()
 *  - Hallucination detection at threshold boundary
 *  - Rolling window eviction
 *  - snapshot() statistics (mean, std-dev, trend, min/max)
 *  - Alert threshold levels (INFO < WARNING < CRITICAL)
 *  - Alert callback invocation
 *  - reset() clears state
 *  - exportCSV() writes a parseable file
 *  - printReport() produces non-empty output
 *  - Thread-safety stress test
 */

#include "rag/hallucination_dashboard.h"

#include <gtest/gtest.h>

#include <atomic>
#include <fstream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

using namespace themis::rag::judge;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

static EvaluationResult makeResult(double faithfulness) {
    EvaluationResult r{};
    r.faithfulness_score        = faithfulness;
    r.relevance_score           = 0.9;
    r.completeness_score        = 0.9;
    r.coherence_score           = 0.9;
    r.ethical_compliance_score  = 0.9;
    r.overall_score             = faithfulness;
    r.passed_quality_threshold  = faithfulness >= 0.7;
    r.confidence                = 0.95;
    r.respects_human_autonomy   = true;
    r.shows_moral_diversity     = false;
    r.has_ethical_citations     = false;
    return r;
}

// ─────────────────────────────────────────────────────────────────────────────
// Basic construction
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, InitialStateIsEmpty) {
    HallucinationDashboard dashboard;
    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.total_recorded,     0u);
    EXPECT_EQ(snap.window_size,        0u);
    EXPECT_EQ(snap.hallucination_count, 0u);
    EXPECT_DOUBLE_EQ(snap.hallucination_rate, 0.0);
    EXPECT_FALSE(snap.alert_triggered);
}

TEST(HallucinationDashboardTest, InitialRateIsZero) {
    HallucinationDashboard dashboard;
    EXPECT_DOUBLE_EQ(dashboard.hallucinationRate(), 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Recording via EvaluationResult
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, RecordIncrementsTotalCount) {
    HallucinationDashboard dashboard;
    dashboard.record(makeResult(0.9), "query1");
    dashboard.record(makeResult(0.5), "query2");

    EXPECT_EQ(dashboard.snapshot().total_recorded, 2u);
    EXPECT_EQ(dashboard.snapshot().window_size,    2u);
}

TEST(HallucinationDashboardTest, RecordClassifiesHallucination) {
    HallucinationDashboard dashboard;
    // default threshold is 0.8
    dashboard.record(makeResult(0.9), "good");
    dashboard.record(makeResult(0.5), "bad");

    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.hallucination_count, 1u);
    EXPECT_NEAR(snap.hallucination_rate, 0.5, 1e-9);
}

// ─────────────────────────────────────────────────────────────────────────────
// Recording via direct faithfulness score
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, RecordFaithfulnessBasic) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(0.95, "q1");
    dashboard.recordFaithfulness(0.40, "q2");

    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.total_recorded, 2u);
    EXPECT_EQ(snap.hallucination_count, 1u);
}

TEST(HallucinationDashboardTest, RecordFaithfulnessClampsAboveOne) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(1.5, "q");  // should be clamped to 1.0
    auto entries = dashboard.recentEntries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_LE(entries[0].faithfulness_score, 1.0);
    EXPECT_FALSE(entries[0].is_hallucination);
}

TEST(HallucinationDashboardTest, RecordFaithfulnessClampsBelowZero) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(-0.5, "q");  // should be clamped to 0.0
    auto entries = dashboard.recentEntries();
    ASSERT_EQ(entries.size(), 1u);
    EXPECT_GE(entries[0].faithfulness_score, 0.0);
    EXPECT_TRUE(entries[0].is_hallucination);
}

// ─────────────────────────────────────────────────────────────────────────────
// Threshold boundary behaviour
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, ThresholdBoundaryExactlyAtThresholdIsNotHallucination) {
    HallucinationDashboardConfig cfg;
    cfg.faithfulness_threshold = 0.8;
    HallucinationDashboard dashboard(cfg);

    // exactly at threshold → not a hallucination
    dashboard.recordFaithfulness(0.8, "boundary");
    EXPECT_EQ(dashboard.snapshot().hallucination_count, 0u);
}

TEST(HallucinationDashboardTest, ThresholdBoundaryJustBelowIsHallucination) {
    HallucinationDashboardConfig cfg;
    cfg.faithfulness_threshold = 0.8;
    HallucinationDashboard dashboard(cfg);

    dashboard.recordFaithfulness(0.799, "below");
    EXPECT_EQ(dashboard.snapshot().hallucination_count, 1u);
}

TEST(HallucinationDashboardTest, CustomThresholdRespected) {
    HallucinationDashboardConfig cfg;
    cfg.faithfulness_threshold = 0.5;
    HallucinationDashboard dashboard(cfg);

    dashboard.recordFaithfulness(0.6, "above_custom");
    dashboard.recordFaithfulness(0.4, "below_custom");

    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.hallucination_count, 1u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Rolling window eviction
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, WindowEvictsOldEntries) {
    HallucinationDashboardConfig cfg;
    cfg.window_size             = 5;
    cfg.faithfulness_threshold  = 0.8;
    HallucinationDashboard dashboard(cfg);

    // Insert 5 hallucinations (score = 0.1)
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(0.1);
    }
    EXPECT_EQ(dashboard.snapshot().hallucination_count, 5u);
    EXPECT_NEAR(dashboard.hallucinationRate(), 1.0, 1e-9);

    // Now insert 5 good evaluations; the old ones should be evicted
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(0.95);
    }
    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.window_size, 5u);
    EXPECT_EQ(snap.hallucination_count, 0u);
    EXPECT_NEAR(snap.hallucination_rate, 0.0, 1e-9);
}

TEST(HallucinationDashboardTest, TotalRecordedExceedsWindowSize) {
    HallucinationDashboardConfig cfg;
    cfg.window_size = 3;
    HallucinationDashboard dashboard(cfg);

    for (int i = 0; i < 10; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.total_recorded, 10u);
    EXPECT_EQ(snap.window_size, 3u);
}

// ─────────────────────────────────────────────────────────────────────────────
// Snapshot statistics
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, MeanFaithfulnessCorrect) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(0.6);
    dashboard.recordFaithfulness(0.8);

    auto snap = dashboard.snapshot();
    EXPECT_NEAR(snap.mean_faithfulness, 0.7, 1e-9);
}

TEST(HallucinationDashboardTest, MinMaxFaithfulnessCorrect) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(0.3);
    dashboard.recordFaithfulness(0.7);
    dashboard.recordFaithfulness(0.5);

    auto snap = dashboard.snapshot();
    EXPECT_NEAR(snap.min_faithfulness, 0.3, 1e-9);
    EXPECT_NEAR(snap.max_faithfulness, 0.7, 1e-9);
}

TEST(HallucinationDashboardTest, FaithfulnessTrendPositiveWhenImproving) {
    HallucinationDashboard dashboard;
    // Strictly increasing scores → positive trend
    for (int i = 1; i <= 10; ++i) {
        dashboard.recordFaithfulness(static_cast<double>(i) / 10.0);
    }
    auto snap = dashboard.snapshot();
    EXPECT_GT(snap.faithfulness_trend, 0.0);
}

TEST(HallucinationDashboardTest, FaithfulnessTrendNegativeWhenDegrading) {
    HallucinationDashboard dashboard;
    // Strictly decreasing scores → negative trend
    for (int i = 10; i >= 1; --i) {
        dashboard.recordFaithfulness(static_cast<double>(i) / 10.0);
    }
    auto snap = dashboard.snapshot();
    EXPECT_LT(snap.faithfulness_trend, 0.0);
}

TEST(HallucinationDashboardTest, StdDevNonNegative) {
    HallucinationDashboard dashboard;
    for (int i = 0; i < 10; ++i) {
        dashboard.recordFaithfulness(0.5 + 0.1 * (i % 3));
    }
    auto snap = dashboard.snapshot();
    EXPECT_GE(snap.std_faithfulness, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// Alert thresholds
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, NoAlertBelowInfoThreshold) {
    HallucinationDashboardConfig cfg;
    cfg.alert_threshold_info     = 0.2;
    cfg.alert_threshold_warning  = 0.4;
    cfg.alert_threshold_critical = 0.6;
    HallucinationDashboard dashboard(cfg);

    // 1 hallucination out of 10 = 10% → below all thresholds
    dashboard.recordFaithfulness(0.1);
    for (int i = 1; i < 10; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    auto snap = dashboard.snapshot();
    EXPECT_FALSE(snap.alert_triggered);
    EXPECT_TRUE(snap.active_alerts.empty());
}

TEST(HallucinationDashboardTest, InfoAlertTriggered) {
    HallucinationDashboardConfig cfg;
    cfg.alert_threshold_info     = 0.05;
    cfg.alert_threshold_warning  = 0.2;
    cfg.alert_threshold_critical = 0.4;
    HallucinationDashboard dashboard(cfg);

    // 1 hallucination out of 10 = 10% → above info, below warning
    dashboard.recordFaithfulness(0.1);
    for (int i = 1; i < 10; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    auto snap = dashboard.snapshot();
    EXPECT_TRUE(snap.alert_triggered);
    ASSERT_FALSE(snap.active_alerts.empty());
    EXPECT_EQ(snap.active_alerts[0].severity, AlertSeverity::INFO);
}

TEST(HallucinationDashboardTest, WarningAlertTriggered) {
    HallucinationDashboardConfig cfg;
    cfg.alert_threshold_info     = 0.05;
    cfg.alert_threshold_warning  = 0.2;
    cfg.alert_threshold_critical = 0.4;
    HallucinationDashboard dashboard(cfg);

    // 3 hallucinations out of 10 = 30% → above warning, below critical
    for (int i = 0; i < 3; ++i) {
        dashboard.recordFaithfulness(0.1);
    }
    for (int i = 0; i < 7; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    auto snap = dashboard.snapshot();
    EXPECT_TRUE(snap.alert_triggered);
    ASSERT_FALSE(snap.active_alerts.empty());
    EXPECT_EQ(snap.active_alerts[0].severity, AlertSeverity::WARNING);
}

TEST(HallucinationDashboardTest, CriticalAlertTriggered) {
    HallucinationDashboardConfig cfg;
    cfg.alert_threshold_info     = 0.05;
    cfg.alert_threshold_warning  = 0.2;
    cfg.alert_threshold_critical = 0.3;
    HallucinationDashboard dashboard(cfg);

    // 5 hallucinations out of 10 = 50% → above critical threshold
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(0.1);
    }
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    auto snap = dashboard.snapshot();
    EXPECT_TRUE(snap.alert_triggered);
    ASSERT_FALSE(snap.active_alerts.empty());
    EXPECT_EQ(snap.active_alerts[0].severity, AlertSeverity::CRITICAL);
}

// ─────────────────────────────────────────────────────────────────────────────
// Alert callback
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, AlertCallbackInvokedOnThresholdBreached) {
    HallucinationDashboardConfig cfg;
    cfg.alert_threshold_info     = 0.05;
    cfg.alert_threshold_warning  = 0.2;
    cfg.alert_threshold_critical = 0.5;
    HallucinationDashboard dashboard(cfg);

    std::atomic<int> callback_count{0};
    dashboard.setAlertCallback([&](const HallucinationAlert& alert) {
        ++callback_count;
        EXPECT_GE(alert.current_rate, cfg.alert_threshold_info);
    });

    // Drive rate above warning threshold
    for (int i = 0; i < 3; ++i) {
        dashboard.recordFaithfulness(0.1);
    }
    for (int i = 0; i < 7; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    EXPECT_GT(callback_count.load(), 0);
}

TEST(HallucinationDashboardTest, NoCallbackWhenBelowThreshold) {
    HallucinationDashboardConfig cfg;
    cfg.alert_threshold_warning = 0.5;
    HallucinationDashboard dashboard(cfg);

    std::atomic<int> callback_count{0};
    dashboard.setAlertCallback([&](const HallucinationAlert&) { ++callback_count; });

    // All evaluations above threshold → no hallucinations
    for (int i = 0; i < 10; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    EXPECT_EQ(callback_count.load(), 0);
}

// ─────────────────────────────────────────────────────────────────────────────
// recentEntries()
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, RecentEntriesReturnsAll) {
    HallucinationDashboard dashboard;
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(0.9, "q" + std::to_string(i));
    }
    EXPECT_EQ(dashboard.recentEntries().size(), 5u);
    EXPECT_EQ(dashboard.recentEntries(0).size(), 5u);
}

TEST(HallucinationDashboardTest, RecentEntriesRespectsLimit) {
    HallucinationDashboard dashboard;
    for (int i = 0; i < 10; ++i) {
        dashboard.recordFaithfulness(0.9);
    }
    EXPECT_EQ(dashboard.recentEntries(3).size(), 3u);
}

TEST(HallucinationDashboardTest, RecentEntriesContainsQueryString) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(0.9, "my_special_query");
    auto entries = dashboard.recentEntries();
    ASSERT_FALSE(entries.empty());
    EXPECT_EQ(entries.back().query, "my_special_query");
}

// ─────────────────────────────────────────────────────────────────────────────
// reset()
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, ResetClearsAllState) {
    HallucinationDashboard dashboard;
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(0.5);
    }
    dashboard.reset();

    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.total_recorded,     0u);
    EXPECT_EQ(snap.window_size,        0u);
    EXPECT_EQ(snap.hallucination_count, 0u);
    EXPECT_DOUBLE_EQ(snap.hallucination_rate, 0.0);
}

// ─────────────────────────────────────────────────────────────────────────────
// exportCSV()
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, ExportCSVCreatesFile) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(0.9, "q1", "FAST");
    dashboard.recordFaithfulness(0.3, "q2", "BALANCED");

    const std::string path = "/tmp/test_hallucination_dashboard_export.csv";
    EXPECT_TRUE(dashboard.exportCSV(path));

    std::ifstream file(path);
    ASSERT_TRUE(file.is_open());

    // First line should be the header
    std::string header;
    std::getline(file, header);
    EXPECT_NE(header.find("faithfulness_score"), std::string::npos);
    EXPECT_NE(header.find("is_hallucination"),   std::string::npos);

    // Should have at least two data rows
    int row_count = 0;
    std::string line;
    while (std::getline(file, line)) {
      ++row_count;
    }
    EXPECT_GE(row_count, 2);
}

TEST(HallucinationDashboardTest, ExportCSVReturnsFalseForBadPath) {
    HallucinationDashboard dashboard;
    dashboard.recordFaithfulness(0.9);
    EXPECT_FALSE(dashboard.exportCSV("/nonexistent/path/file.csv"));
}

// ─────────────────────────────────────────────────────────────────────────────
// printReport()
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, PrintReportProducesOutput) {
    HallucinationDashboard dashboard;
    for (int i = 0; i < 5; ++i) {
        dashboard.recordFaithfulness(i % 2 == 0 ? 0.9 : 0.3);
    }
    std::ostringstream oss;
    dashboard.printReport(oss);
    EXPECT_FALSE(oss.str().empty());
    EXPECT_NE(oss.str().find("HALLUCINATION"), std::string::npos);
}

TEST(HallucinationDashboardTest, PrintReportOnEmptyDashboardDoesNotCrash) {
    HallucinationDashboard dashboard;
    std::ostringstream oss;
    EXPECT_NO_THROW(dashboard.printReport(oss));
}

// ─────────────────────────────────────────────────────────────────────────────
// Thread-safety stress test
// ─────────────────────────────────────────────────────────────────────────────

TEST(HallucinationDashboardTest, ConcurrentRecordingIsSafe) {
    HallucinationDashboardConfig cfg;
    cfg.window_size = 100;
    HallucinationDashboard dashboard(cfg);

    constexpr int THREADS = 8;
    constexpr int OPS_PER_THREAD = 50;

    std::vector<std::thread> threads;
    threads.reserve(THREADS);

    for (int t = 0; t < THREADS; ++t) {
        threads.emplace_back([&, t]() {
            for (int i = 0; i < OPS_PER_THREAD; ++i) {
                double score = (t % 2 == 0) ? 0.9 : 0.3;
                dashboard.recordFaithfulness(score, "q");
                (void)dashboard.hallucinationRate();
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    auto snap = dashboard.snapshot();
    EXPECT_EQ(snap.total_recorded, static_cast<size_t>(THREADS * OPS_PER_THREAD));
    EXPECT_GE(snap.hallucination_rate, 0.0);
    EXPECT_LE(snap.hallucination_rate, 1.0);
}
