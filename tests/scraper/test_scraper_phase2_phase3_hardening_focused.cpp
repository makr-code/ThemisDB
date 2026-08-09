// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_scraper_phase2_phase3_hardening_focused.cpp
 * @brief Phase 2+3 focused hardening tests for the scraper module.
 *
 * Test IDs: SCR-09 through SCR-16
 * No file I/O, no network, deterministic only.
 *
 * Coverage:
 *   SCR-09  faultClassOf() — all ScraperError codes map to expected fault class
 *   SCR-10  defaultSeverityOf() — fail-closed errors map to kFatal; others kError/kInfo
 *   SCR-11  makeDiagnosticEvent() — returned event fields match inputs
 *   SCR-12  ListeningScraperDiagnosticSink::emit() — records events and calls listeners
 *   SCR-13  ListeningScraperDiagnosticSink concurrency — two threads emit without data race
 *   SCR-14  NullScraperDiagnosticSink — silently accepts all events
 *   SCR-15  Diagnostics listener broadcast — multiple listeners all receive same event
 *   SCR-16  ScraperDiagnosticEvent timestamp monotonicity
 *
 * @see include/scraper/scraper_diagnostics.h
 * @see src/scraper/ROADMAP.md — Phase 2+3 items
 */

#include "gtest/gtest.h"
#include "scraper/scraper_diagnostics.h"

#include <atomic>
#include <cstdint>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace scraper {
namespace test {

// ============================================================================
// SCR-09 — faultClassOf() mapping
// ============================================================================

TEST(ScraperPhase23Hardening, SCR09_FaultClassMapping) {
    EXPECT_EQ(faultClassOf(ScraperError::kFetchFailed),
              ScraperFaultClass::kFetchPath);
    EXPECT_EQ(faultClassOf(ScraperError::kRenderTimeout),
              ScraperFaultClass::kRenderPath);
    EXPECT_EQ(faultClassOf(ScraperError::kParseError),
              ScraperFaultClass::kParsePath);
    EXPECT_EQ(faultClassOf(ScraperError::kEvaluationFailed),
              ScraperFaultClass::kEvaluatorPath);
    EXPECT_EQ(faultClassOf(ScraperError::kMetadataWriteFailed),
              ScraperFaultClass::kWritePath);
    EXPECT_EQ(faultClassOf(ScraperError::kSourceNotFound),
              ScraperFaultClass::kCrawlControl);
    EXPECT_EQ(faultClassOf(ScraperError::kPaginationLimit),
              ScraperFaultClass::kCrawlControl);
    EXPECT_EQ(faultClassOf(ScraperError::kInternalError),
              ScraperFaultClass::kInternal);
    EXPECT_EQ(faultClassOf(ScraperError::kSuccess),
              ScraperFaultClass::kInternal);
}

// ============================================================================
// SCR-10 — defaultSeverityOf() mapping
// ============================================================================

TEST(ScraperPhase23Hardening, SCR10_DefaultSeverityMapping) {
    // kSuccess → kInfo
    EXPECT_EQ(defaultSeverityOf(ScraperError::kSuccess),
              ScraperFaultSeverity::kInfo);

    // fail-closed errors → kFatal
    EXPECT_EQ(defaultSeverityOf(ScraperError::kEvaluationFailed),
              ScraperFaultSeverity::kFatal);
    EXPECT_EQ(defaultSeverityOf(ScraperError::kInternalError),
              ScraperFaultSeverity::kFatal);
    EXPECT_EQ(defaultSeverityOf(ScraperError::kMetadataWriteFailed),
              ScraperFaultSeverity::kFatal);

    // non-fail-closed errors → kError
    EXPECT_EQ(defaultSeverityOf(ScraperError::kFetchFailed),
              ScraperFaultSeverity::kError);
    EXPECT_EQ(defaultSeverityOf(ScraperError::kRenderTimeout),
              ScraperFaultSeverity::kError);
    EXPECT_EQ(defaultSeverityOf(ScraperError::kParseError),
              ScraperFaultSeverity::kError);
    EXPECT_EQ(defaultSeverityOf(ScraperError::kSourceNotFound),
              ScraperFaultSeverity::kError);
    EXPECT_EQ(defaultSeverityOf(ScraperError::kPaginationLimit),
              ScraperFaultSeverity::kError);
}

// ============================================================================
// SCR-11 — makeDiagnosticEvent() field population
// ============================================================================

TEST(ScraperPhase23Hardening, SCR11_MakeDiagnosticEventFields) {
    const std::string url = "https://example.com/doc";
    const std::string msg = "HTTP 503 – service unavailable";
    const ScraperError err = ScraperError::kFetchFailed;

    auto ev = makeDiagnosticEvent(err, url, msg);

    EXPECT_EQ(ev.error,       err);
    EXPECT_EQ(ev.source_url,  url);
    EXPECT_EQ(ev.message,     msg);
    EXPECT_EQ(ev.fault_class, ScraperFaultClass::kFetchPath);
    EXPECT_EQ(ev.severity,    ScraperFaultSeverity::kError);
    EXPECT_FALSE(ev.message.empty())
        << "makeDiagnosticEvent must not produce an event with empty message";
}

// ============================================================================
// SCR-12 — ListeningScraperDiagnosticSink records events and calls listener
// ============================================================================

TEST(ScraperPhase23Hardening, SCR12_ListeningSinkRecordsAndBroadcasts) {
    ListeningScraperDiagnosticSink sink;

    int listener_calls = 0;
    sink.addListener([&](const ScraperDiagnosticEvent& e) {
        ++listener_calls;
        EXPECT_EQ(e.error, ScraperError::kParseError);
    });

    auto ev = makeDiagnosticEvent(ScraperError::kParseError,
                                  "https://test.example/page",
                                  "Malformed HTML: unexpected tag");
    sink.emit(ev);

    EXPECT_EQ(sink.size(), 1u);
    EXPECT_EQ(listener_calls, 1);

    const auto snap = sink.snapshot();
    ASSERT_EQ(snap.size(), 1u);
    EXPECT_EQ(snap[0].error, ScraperError::kParseError);
    EXPECT_EQ(snap[0].fault_class, ScraperFaultClass::kParsePath);
}

// ============================================================================
// SCR-13 — ListeningScraperDiagnosticSink concurrency: no data race
// ============================================================================

TEST(ScraperPhase23Hardening, SCR13_ListeningSinkConcurrentEmit) {
    ListeningScraperDiagnosticSink sink;
    std::atomic<int> total{0};
    sink.addListener([&](const ScraperDiagnosticEvent&) { ++total; });

    constexpr int kPerThread = 50;
    auto worker = [&](ScraperError err) {
        for (int i = 0; i < kPerThread; ++i) {
            auto ev = makeDiagnosticEvent(err, "https://bench.example/",
                                          "concurrent emit test");
            sink.emit(ev);
        }
    };

    std::thread t1(worker, ScraperError::kFetchFailed);
    std::thread t2(worker, ScraperError::kParseError);
    t1.join();
    t2.join();

    EXPECT_EQ(sink.size(), static_cast<std::size_t>(2 * kPerThread));
    EXPECT_EQ(total.load(), 2 * kPerThread);
}

// ============================================================================
// SCR-14 — NullScraperDiagnosticSink silently absorbs all events
// ============================================================================

TEST(ScraperPhase23Hardening, SCR14_NullSinkAbsorbs) {
    NullScraperDiagnosticSink sink;
    // Must not throw or crash for any error code.
    for (auto err : {
             ScraperError::kSuccess,
             ScraperError::kFetchFailed,
             ScraperError::kRenderTimeout,
             ScraperError::kParseError,
             ScraperError::kEvaluationFailed,
             ScraperError::kMetadataWriteFailed,
             ScraperError::kSourceNotFound,
             ScraperError::kPaginationLimit,
             ScraperError::kInternalError,
         }) {
        auto ev = makeDiagnosticEvent(err, "https://null.example/", "null sink test");
        EXPECT_NO_THROW(sink.emit(ev));
    }
}

// ============================================================================
// SCR-15 — Multiple listeners all receive the same event
// ============================================================================

TEST(ScraperPhase23Hardening, SCR15_MultipleListenersBroadcast) {
    ListeningScraperDiagnosticSink sink;

    std::vector<ScraperError> received_a, received_b;
    sink.addListener([&](const ScraperDiagnosticEvent& e) {
        received_a.push_back(e.error);
    });
    sink.addListener([&](const ScraperDiagnosticEvent& e) {
        received_b.push_back(e.error);
    });

    const ScraperError errors[] = {
        ScraperError::kFetchFailed,
        ScraperError::kRenderTimeout,
        ScraperError::kEvaluationFailed,
    };
    for (auto e : errors) {
        sink.emit(makeDiagnosticEvent(e, "https://multi.example/", "broadcast"));
    }

    ASSERT_EQ(received_a.size(), 3u);
    ASSERT_EQ(received_b.size(), 3u);
    for (std::size_t i = 0; i < 3u; ++i) {
        EXPECT_EQ(received_a[i], received_b[i])
            << "Listener A and B must receive identical events in order";
    }
}

// ============================================================================
// SCR-16 — ScraperDiagnosticEvent default timestamp is not epoch
// ============================================================================

TEST(ScraperPhase23Hardening, SCR16_EventTimestampInitialised) {
    using clk = std::chrono::system_clock;

    // The default-constructed timestamp must be close to now (within 5 s).
    ScraperDiagnosticEvent ev;
    const auto now = clk::now();
    const auto delta = std::chrono::duration_cast<std::chrono::seconds>(
        now - ev.timestamp).count();

    // Positive: timestamp <= now.
    EXPECT_GE(delta, 0)
        << "Event timestamp must not be in the future";
    // Reasonable: timestamp is within 5 seconds of construction.
    EXPECT_LE(delta, 5)
        << "Event timestamp appears to be uninitialised (too far from now)";
}

} // namespace test
} // namespace scraper
} // namespace themis
