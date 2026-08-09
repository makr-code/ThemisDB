// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_run_summary_focused.cpp
 * @brief Focused tests for ScraperRunSummary and ScraperRunSummaryCollector.
 *
 * Test IDs: SCR-25 through SCR-28
 * No file I/O, no network, deterministic only.
 *
 * Coverage:
 *   SCR-25  Empty session → isHealthy() == false
 *   SCR-26  Counter accuracy after a mixed-fault event sequence
 *   SCR-27  toLogLine() is non-empty and contains "succeeded="
 *   SCR-28  isHealthy() logic: succeeded>0 AND failed_eval==0 required
 *
 * @see include/scraper/scraper_run_summary.h
 */

#include "scraper/scraper_run_summary.h"
#include "gtest/gtest.h"

#include <string>

namespace themis {
namespace scraper {
namespace test {

// ============================================================================
// SCR-25 — Empty session → isHealthy() == false
// ============================================================================

TEST(ScraperRunSummary, SCR25_EmptySessionIsUnhealthy) {
    ScraperRunSummary s{};
    // A default-constructed summary has succeeded == 0 → not healthy.
    EXPECT_FALSE(s.isHealthy())
        << "An empty run summary must not be considered healthy";
}

// ============================================================================
// SCR-26 — Counter accuracy after mixed fault events
// ============================================================================

TEST(ScraperRunSummary, SCR26_CounterAccuracyMixedEvents) {
    ListeningScraperDiagnosticSink sink;
    ScraperRunSummaryCollector     collector;
    collector.attach(sink);

    // 2× kFetchFailed  → failed_fetch += 2
    sink.emit(makeDiagnosticEvent(ScraperError::kFetchFailed,
                                  "https://example.com/a", "fetch error 1"));
    sink.emit(makeDiagnosticEvent(ScraperError::kFetchFailed,
                                  "https://example.com/b", "fetch error 2"));

    // 1× kParseError   → failed_parse += 1
    sink.emit(makeDiagnosticEvent(ScraperError::kParseError,
                                  "https://example.com/c", "parse error"));

    // 1× kEvaluationFailed → failed_eval += 1
    sink.emit(makeDiagnosticEvent(ScraperError::kEvaluationFailed,
                                  "https://example.com/d", "eval failed"));

    // 1× kMetadataWriteFailed → failed_write += 1
    sink.emit(makeDiagnosticEvent(ScraperError::kMetadataWriteFailed,
                                  "https://example.com/e", "write failed"));

    // 3 pages successfully written
    collector.recordSuccess(3);

    // Run-level stats
    collector.setRunStats(10, 5000);

    const auto s = collector.summary();

    EXPECT_EQ(s.failed_fetch,  2u) << "Two kFetchFailed events must increment failed_fetch by 2";
    EXPECT_EQ(s.failed_parse,  1u) << "One kParseError event must increment failed_parse by 1";
    EXPECT_EQ(s.failed_eval,   1u) << "One kEvaluationFailed event must increment failed_eval by 1";
    EXPECT_EQ(s.failed_write,  1u) << "One kMetadataWriteFailed event must increment failed_write by 1";
    EXPECT_EQ(s.pages_written, 3u) << "recordSuccess(3) must set pages_written to 3";
    EXPECT_EQ(s.total_urls,   10u) << "setRunStats(10, …) must set total_urls to 10";
    EXPECT_EQ(s.run_duration_ms, 5000u) << "setRunStats(…, 5000) must set run_duration_ms to 5000";
}

// ============================================================================
// SCR-27 — toLogLine() is non-empty and contains "succeeded="
// ============================================================================

TEST(ScraperRunSummary, SCR27_ToLogLineNonEmptyContainsSucceeded) {
    ScraperRunSummary s;
    s.total_urls      = 20;
    s.succeeded       = 15;
    s.failed_fetch    = 2;
    s.failed_parse    = 1;
    s.failed_eval     = 0;
    s.failed_write    = 0;
    s.skipped_burst   = 2;
    s.pages_written   = 30;
    s.run_duration_ms = 12345;

    const std::string line = s.toLogLine();

    EXPECT_FALSE(line.empty())
        << "toLogLine() must not return an empty string";
    EXPECT_NE(line.find("succeeded="), std::string::npos)
        << "toLogLine() must contain the \"succeeded=\" key";
}

// ============================================================================
// SCR-28 — isHealthy() requires succeeded>0 AND failed_eval==0
// ============================================================================

TEST(ScraperRunSummary, SCR28_IsHealthyLogic) {
    // Case A: succeeded > 0 and failed_eval == 0 → healthy
    ScraperRunSummary s1{};
    s1.succeeded  = 5;
    s1.failed_eval = 0;
    EXPECT_TRUE(s1.isHealthy())
        << "succeeded=5, failed_eval=0 must be healthy";

    // Case B: succeeded > 0 but failed_eval > 0 → unhealthy (fail-closed)
    ScraperRunSummary s2{};
    s2.succeeded  = 5;
    s2.failed_eval = 1;
    EXPECT_FALSE(s2.isHealthy())
        << "succeeded=5, failed_eval=1 must NOT be healthy (fail-closed)";

    // Case C: succeeded == 0 and failed_eval == 0 → unhealthy (nothing done)
    ScraperRunSummary s3{};
    s3.succeeded  = 0;
    s3.failed_eval = 0;
    EXPECT_FALSE(s3.isHealthy())
        << "succeeded=0, failed_eval=0 must NOT be healthy (no successes)";
}

} // namespace test
} // namespace scraper
} // namespace themis
