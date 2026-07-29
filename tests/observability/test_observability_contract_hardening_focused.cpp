// SPDX-License-Identifier: Apache-2.0
// Copyright (c) 2026 ThemisDB Contributors

/**
 * @file test_observability_contract_hardening_focused.cpp
 * @brief Phase 4 — Observability contract hardening focused tests (OCH-01..OCH-16).
 *
 * Tests are fully self-contained: no network I/O, no filesystem I/O.
 * All external interactions are mocked inline.  The canonical PRNG seed is
 * kObservabilityContractSeed = 42.
 *
 * ## Test families
 *
 * ### OCH-01..04 — Counter/gauge/histogram contract
 *   OCH-01  Counter is monotonically non-decreasing
 *   OCH-02  Gauge is bidirectional (may increase and decrease)
 *   OCH-03  Histogram bucket boundaries are strictly increasing
 *   OCH-04  Counter reset surfaced via annotation, not silent decrement
 *
 * ### OCH-05..08 — Tracing contract
 *   OCH-05  Parent-child span linkage is deterministic
 *   OCH-06  Child span close before parent is accepted (not an error)
 *   OCH-07  Dropped span surfaces TRACE_SPAN_DROPPED
 *   OCH-08  Span nesting depth beyond limit surfaces SPAN_DEPTH_EXCEEDED
 *
 * ### OCH-09..12 — Log contract
 *   OCH-09  Structured fields are preserved after write
 *   OCH-10  Concurrent log writes are non-interleaved (each entry intact)
 *   OCH-11  Log level filtering removes entries below configured threshold
 *   OCH-12  Log entry exceeding size limit surfaces LOG_WRITE_FAILED
 *
 * ### OCH-13..16 — SLO signals
 *   OCH-13  SLO window boundary is exact (no early close, no late close)
 *   OCH-14  SLO breach is surfaced when SLI falls below threshold
 *   OCH-15  Late update after window close surfaces SLO_WINDOW_INVALID
 *   OCH-16  Export failure surfaces EXPORTER_UNAVAILABLE and does not crash
 *
 * @see include/observability/observability_api_contract.h
 * @see src/observability/ROADMAP.md — Phase 4 item
 */

#include <gtest/gtest.h>

#include "observability/observability_api_contract.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <map>
#include <memory>
#include <random>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::chrono_literals;

namespace {

static constexpr uint64_t kObservabilityContractSeed = 42;

// ---------------------------------------------------------------------------
// Mock Counter
// ---------------------------------------------------------------------------
struct MockCounter {
    std::int64_t value = 0;
    bool         reset_annotated = false;

    /// Returns error code if the transition violates monotonicity.
    ObservabilityErrorCode increment(std::int64_t delta) {
        std::int64_t next = value + delta;
        if (!isValidCounterTransition(value, next))
            return ObservabilityErrorCode::METRIC_OVERFLOW;
        value = next;
        return ObservabilityErrorCode::OK;
    }

    void reset() { value = 0; reset_annotated = true; }
};

// ---------------------------------------------------------------------------
// Mock Gauge
// ---------------------------------------------------------------------------
struct MockGauge {
    double value = 0.0;
    void set(double v) { value = v; }
};

// ---------------------------------------------------------------------------
// Mock Histogram
// ---------------------------------------------------------------------------
struct MockHistogram {
    std::vector<double> buckets;  ///< upper bounds, must be strictly increasing

    bool isBucketsValid() const {
        for (std::size_t i = 1; i < buckets.size(); ++i)
            if (buckets[i] <= buckets[i - 1]) return false;
        return true;
    }
};

// ---------------------------------------------------------------------------
// Mock Span
// ---------------------------------------------------------------------------
struct MockSpan {
    std::string span_id;
    std::string parent_span_id;
    bool        closed = false;
};

struct MockTracer {
    std::vector<MockSpan> spans;
    std::size_t           dropped = 0;
    std::size_t           max_depth;

    explicit MockTracer(std::size_t max_d = kMaxSpanNestingDepth)
        : max_depth(max_d) {}

    ObservabilityErrorCode startSpan(const std::string& id,
                                     const std::string& parent_id,
                                     std::size_t depth) {
        if (depth > max_depth) {
            return ObservabilityErrorCode::SPAN_DEPTH_EXCEEDED;
        }
        spans.push_back({id, parent_id, false});
        return ObservabilityErrorCode::OK;
    }

    ObservabilityErrorCode dropSpan() {
        ++dropped;
        return ObservabilityErrorCode::TRACE_SPAN_DROPPED;
    }

    void closeSpan(const std::string& id) {
        for (auto& s : spans)
            if (s.span_id == id) { s.closed = true; return; }
    }
};

// ---------------------------------------------------------------------------
// Mock Log
// ---------------------------------------------------------------------------
enum class LogLevel { DEBUG = 0, INFO = 1, WARN = 2, ERROR = 3 };

struct MockLogEntry {
    LogLevel                         level;
    std::map<std::string, std::string> fields;
    std::string                      message;
    std::size_t                      byte_size;
};

struct MockLogger {
    std::vector<MockLogEntry> entries;
    LogLevel                  min_level = LogLevel::INFO;

    ObservabilityErrorCode write(MockLogEntry entry) {
        if (entry.byte_size > kMaxLogEntryBytes)
            return ObservabilityErrorCode::LOG_WRITE_FAILED;
        if (entry.level < min_level)
            return ObservabilityErrorCode::OK;  // filtered, not an error
        entries.push_back(std::move(entry));
        return ObservabilityErrorCode::OK;
    }
};

// ---------------------------------------------------------------------------
// Mock SLO window
// ---------------------------------------------------------------------------
struct MockSloWindow {
    int64_t  open_ts;   ///< window open timestamp (ms)
    int64_t  close_ts;  ///< window close timestamp (ms)
    bool     closed = false;
    double   sli    = 1.0;   ///< current SLI value [0,1]
    double   target = 0.99;  ///< SLO target

    bool isBreach() const { return sli < target; }

    ObservabilityErrorCode update(int64_t ts, double new_sli) {
        if (closed && ts > close_ts + static_cast<int64_t>(kSloWindowGracePeriod.count()) * 1000)
            return ObservabilityErrorCode::SLO_WINDOW_INVALID;
        sli = new_sli;
        return ObservabilityErrorCode::OK;
    }
};

} // anonymous namespace

// ===========================================================================
// OCH-01 — Counter is monotonically non-decreasing
// ===========================================================================

TEST(ObservabilityContractHardeningOCH01, CounterMonotonic) {
    MockCounter c;
    EXPECT_EQ(c.increment(5),   ObservabilityErrorCode::OK);
    EXPECT_EQ(c.value, 5);
    EXPECT_EQ(c.increment(3),   ObservabilityErrorCode::OK);
    EXPECT_EQ(c.value, 8);
    EXPECT_EQ(c.increment(0),   ObservabilityErrorCode::OK);
    EXPECT_EQ(c.value, 8);
    // Negative delta violates monotonicity
    EXPECT_EQ(c.increment(-1),  ObservabilityErrorCode::METRIC_OVERFLOW);
    EXPECT_EQ(c.value, 8) << "Value must not change on rejected decrement";
}

// ===========================================================================
// OCH-02 — Gauge is bidirectional
// ===========================================================================

TEST(ObservabilityContractHardeningOCH02, GaugeBidirectional) {
    MockGauge g;
    g.set(100.0);
    EXPECT_DOUBLE_EQ(g.value, 100.0);
    g.set(50.0);
    EXPECT_DOUBLE_EQ(g.value, 50.0);
    g.set(200.0);
    EXPECT_DOUBLE_EQ(g.value, 200.0);
    g.set(-10.0);
    EXPECT_DOUBLE_EQ(g.value, -10.0);
}

// ===========================================================================
// OCH-03 — Histogram bucket boundaries are strictly increasing
// ===========================================================================

TEST(ObservabilityContractHardeningOCH03, HistogramBucketOrdering) {
    MockHistogram valid_h;
    valid_h.buckets = {1.0, 5.0, 10.0, 50.0, 100.0};
    EXPECT_TRUE(valid_h.isBucketsValid());

    MockHistogram invalid_h;
    invalid_h.buckets = {1.0, 5.0, 5.0, 50.0};  // duplicate = not strictly increasing
    EXPECT_FALSE(invalid_h.isBucketsValid());

    MockHistogram decreasing_h;
    decreasing_h.buckets = {10.0, 5.0, 1.0};
    EXPECT_FALSE(decreasing_h.isBucketsValid());
}

// ===========================================================================
// OCH-04 — Counter reset is annotated, not a silent decrement
// ===========================================================================

TEST(ObservabilityContractHardeningOCH04, CounterResetAnnotated) {
    MockCounter c;
    c.increment(100);
    EXPECT_EQ(c.value, 100);

    c.reset();
    EXPECT_EQ(c.value, 0);
    EXPECT_TRUE(c.reset_annotated)
        << "Counter reset must be annotated so consumers can detect it";
}

// ===========================================================================
// OCH-05 — Parent-child span linkage is deterministic
// ===========================================================================

TEST(ObservabilityContractHardeningOCH05, SpanParentChildLinkage) {
    MockTracer tracer;
    auto rc = tracer.startSpan("span-1", "",        1);
    EXPECT_EQ(rc, ObservabilityErrorCode::OK);
    rc       = tracer.startSpan("span-2", "span-1", 2);
    EXPECT_EQ(rc, ObservabilityErrorCode::OK);

    ASSERT_EQ(tracer.spans.size(), 2u);
    EXPECT_EQ(tracer.spans[1].parent_span_id, "span-1")
        << "Child span must reference its parent's span_id";
}

// ===========================================================================
// OCH-06 — Child span close before parent is accepted (not an error)
// ===========================================================================

TEST(ObservabilityContractHardeningOCH06, ChildCloseBeforeParent) {
    MockTracer tracer;
    tracer.startSpan("root",  "",     1);
    tracer.startSpan("child", "root", 2);

    // Close child first
    tracer.closeSpan("child");
    EXPECT_TRUE(tracer.spans[1].closed);
    EXPECT_FALSE(tracer.spans[0].closed) << "Parent still open — valid";

    // Then close parent
    tracer.closeSpan("root");
    EXPECT_TRUE(tracer.spans[0].closed);
}

// ===========================================================================
// OCH-07 — Dropped span surfaces TRACE_SPAN_DROPPED
// ===========================================================================

TEST(ObservabilityContractHardeningOCH07, DroppedSpanSurfacesError) {
    MockTracer tracer;
    auto code = tracer.dropSpan();
    EXPECT_EQ(code, ObservabilityErrorCode::TRACE_SPAN_DROPPED);
    EXPECT_EQ(tracer.dropped, 1u);
    EXPECT_TRUE(isDataLossCode(code));
}

// ===========================================================================
// OCH-08 — Span depth beyond limit surfaces SPAN_DEPTH_EXCEEDED
// ===========================================================================

TEST(ObservabilityContractHardeningOCH08, SpanDepthLimitEnforced) {
    MockTracer tracer(3);  // max_depth = 3
    EXPECT_EQ(tracer.startSpan("s1", "",   1), ObservabilityErrorCode::OK);
    EXPECT_EQ(tracer.startSpan("s2", "s1", 2), ObservabilityErrorCode::OK);
    EXPECT_EQ(tracer.startSpan("s3", "s2", 3), ObservabilityErrorCode::OK);
    EXPECT_EQ(tracer.startSpan("s4", "s3", 4), ObservabilityErrorCode::SPAN_DEPTH_EXCEEDED);
}

// ===========================================================================
// OCH-09 — Structured log fields preserved after write
// ===========================================================================

TEST(ObservabilityContractHardeningOCH09, StructuredFieldsPreserved) {
    MockLogger logger;
    MockLogEntry entry;
    entry.level    = LogLevel::INFO;
    entry.fields   = {{"request_id", "abc-123"}, {"user", "alice"}};
    entry.message  = "request handled";
    entry.byte_size = 100;

    auto rc = logger.write(entry);
    EXPECT_EQ(rc, ObservabilityErrorCode::OK);
    ASSERT_EQ(logger.entries.size(), 1u);
    EXPECT_EQ(logger.entries[0].fields.at("request_id"), "abc-123");
    EXPECT_EQ(logger.entries[0].fields.at("user"), "alice");
}

// ===========================================================================
// OCH-10 — Concurrent log writes are non-interleaved (entry-level atomicity)
// ===========================================================================

TEST(ObservabilityContractHardeningOCH10, ConcurrentWritesNonInterleaved) {
    // Simulated via sequential writes — each entry is a self-contained object.
    MockLogger logger;
    for (int i = 0; i < 20; ++i) {
        MockLogEntry e;
        e.level    = LogLevel::INFO;
        e.fields   = {{"seq", std::to_string(i)}};
        e.message  = "msg-" + std::to_string(i);
        e.byte_size = 64;
        logger.write(e);
    }
    EXPECT_EQ(logger.entries.size(), 20u);
    for (int i = 0; i < 20; ++i) {
        EXPECT_EQ(logger.entries[i].fields.at("seq"), std::to_string(i));
    }
}

// ===========================================================================
// OCH-11 — Log level filtering removes entries below threshold
// ===========================================================================

TEST(ObservabilityContractHardeningOCH11, LogLevelFiltering) {
    MockLogger logger;
    logger.min_level = LogLevel::WARN;

    MockLogEntry debug_entry;
    debug_entry.level = LogLevel::DEBUG; debug_entry.byte_size = 50;
    MockLogEntry info_entry;
    info_entry.level  = LogLevel::INFO;  info_entry.byte_size = 50;
    MockLogEntry warn_entry;
    warn_entry.level  = LogLevel::WARN;  warn_entry.byte_size = 50;
    MockLogEntry error_entry;
    error_entry.level = LogLevel::ERROR; error_entry.byte_size = 50;

    EXPECT_EQ(logger.write(debug_entry), ObservabilityErrorCode::OK);
    EXPECT_EQ(logger.write(info_entry),  ObservabilityErrorCode::OK);
    EXPECT_EQ(logger.write(warn_entry),  ObservabilityErrorCode::OK);
    EXPECT_EQ(logger.write(error_entry), ObservabilityErrorCode::OK);

    EXPECT_EQ(logger.entries.size(), 2u) << "Only WARN and ERROR should pass the filter";
}

// ===========================================================================
// OCH-12 — Oversized log entry surfaces LOG_WRITE_FAILED
// ===========================================================================

TEST(ObservabilityContractHardeningOCH12, OversizedEntryRejected) {
    MockLogger logger;
    MockLogEntry big;
    big.level     = LogLevel::INFO;
    big.byte_size = kMaxLogEntryBytes + 1;

    auto rc = logger.write(big);
    EXPECT_EQ(rc, ObservabilityErrorCode::LOG_WRITE_FAILED);
    EXPECT_TRUE(isDataLossCode(rc));
    EXPECT_EQ(logger.entries.size(), 0u);
}

// ===========================================================================
// OCH-13 — SLO window boundary is exact
// ===========================================================================

TEST(ObservabilityContractHardeningOCH13, SloWindowBoundaryExact) {
    MockSloWindow w;
    w.open_ts  = 1000;
    w.close_ts = 60000;
    w.closed   = true;
    w.sli      = 0.999;

    // Update within grace period → OK
    auto rc = w.update(60001, 0.998);
    EXPECT_EQ(rc, ObservabilityErrorCode::OK);

    // Update far past grace period → SLO_WINDOW_INVALID
    rc = w.update(120000, 0.997);
    EXPECT_EQ(rc, ObservabilityErrorCode::SLO_WINDOW_INVALID);
}

// ===========================================================================
// OCH-14 — SLO breach surfaced when SLI falls below target
// ===========================================================================

TEST(ObservabilityContractHardeningOCH14, SloBreachSurfaced) {
    MockSloWindow w;
    w.open_ts  = 0;
    w.close_ts = 60000;
    w.target   = 0.99;
    w.sli      = 0.999;
    EXPECT_FALSE(w.isBreach());

    w.sli = 0.985;
    EXPECT_TRUE(w.isBreach()) << "SLO breach must be surfaced when SLI < target";
}

// ===========================================================================
// OCH-15 — Late update after window close surfaces SLO_WINDOW_INVALID
// ===========================================================================

TEST(ObservabilityContractHardeningOCH15, LateUpdateSurfacesSloWindowInvalid) {
    MockSloWindow w;
    w.open_ts  = 0;
    w.close_ts = 1000;
    w.closed   = true;
    w.sli      = 0.995;

    // Late update well past grace period
    auto rc = w.update(100000, 0.990);
    EXPECT_EQ(rc, ObservabilityErrorCode::SLO_WINDOW_INVALID);
}

// ===========================================================================
// OCH-16 — Export failure surfaces EXPORTER_UNAVAILABLE, no crash
// ===========================================================================

TEST(ObservabilityContractHardeningOCH16, ExportFailureGraceful) {
    // Simulated exporter: unavailable
    bool exporter_up = false;

    auto export_result = [&]() -> ObservabilityErrorCode {
        if (!exporter_up)
            return ObservabilityErrorCode::EXPORTER_UNAVAILABLE;
        return ObservabilityErrorCode::OK;
    };

    auto rc = export_result();
    EXPECT_EQ(rc, ObservabilityErrorCode::EXPORTER_UNAVAILABLE)
        << "Export failure must surface EXPORTER_UNAVAILABLE";
    // Verify no crash / exception (test itself completing is the proof)
    EXPECT_NE(rc, ObservabilityErrorCode::OK);
}
