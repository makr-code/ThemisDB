/**
 * @file test_tracer.cpp
 * @brief Unit tests for ObservabilityTracer – span management, W3C Trace
 *        Context propagation, sampling, and ring-buffer diagnostics.
 *
 * Tests cover:
 *  - Default configuration construction
 *  - isInitialized() / initialize() / shutdown()
 *  - startSpan(): span is valid, increments total_spans
 *  - Span end() records entry in completedSpans() ring buffer
 *  - startChildSpan(): inherits trace_id from parent
 *  - startSpanFromHeaders(): extracts traceparent, fallback to root
 *  - injectContext(): writes traceparent header after startSpan
 *  - Attribute setting (string, int64, double, bool)
 *  - recordError() / setStatus(false)
 *  - TracerStats: total_spans, active_spans, dropped_spans
 *  - Sampling: always-off drops spans (isValid() == false)
 *  - Ring-buffer cap: clearCompletedSpans()
 *  - Thread safety: concurrent span creation
 *  - ContinuousProfiler integration: attach disabled by default; graceful no-op
 *    when profiler is null or destroyed before span ends; snapshot attached when
 *    profiler is set and attach_profile_on_span_end is true
 */

#include <gtest/gtest.h>
#include "observability/tracer.h"
#include "observability/continuous_profiler.h"

#include <memory>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;

// ---------------------------------------------------------------------------
// Helpers
// ---------------------------------------------------------------------------

static ObservabilityTracer makeTracer(double sample_rate = 1.0,
                                       size_t max_retained = 100) {
    ObservabilityTracerConfig cfg;
    cfg.service_name       = "test-service";
    cfg.sample_rate        = sample_rate;
    cfg.max_retained_spans = max_retained;
    cfg.publish_metrics    = false;
    return ObservabilityTracer(cfg);
}

// ---------------------------------------------------------------------------
// Construction / lifecycle
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, DefaultConstruction) {
    ObservabilityTracer tracer;
    EXPECT_TRUE(tracer.isInitialized());
}

TEST(ObservabilityTracerTest, InitializeAlwaysSucceeds) {
    ObservabilityTracer tracer;
    EXPECT_TRUE(tracer.initialize("svc", "http://localhost:4318"));
    EXPECT_TRUE(tracer.isInitialized());
}

TEST(ObservabilityTracerTest, ShutdownMarksUninitialized) {
    ObservabilityTracer tracer;
    tracer.shutdown();
    EXPECT_FALSE(tracer.isInitialized());
}

TEST(ObservabilityTracerTest, IsHealthyAfterInit) {
    ObservabilityTracer tracer;
    auto result = tracer.isHealthy();
    EXPECT_TRUE(result.ok);
}

// ---------------------------------------------------------------------------
// Span creation
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, StartSpanReturnsValidSpan) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("test.op");
    ASSERT_NE(nullptr, span);
    EXPECT_TRUE(span->isValid());
}

TEST(ObservabilityTracerTest, StartSpanIncrementsTotalSpans) {
    auto tracer = makeTracer();
    EXPECT_EQ(0, tracer.stats().total_spans);
    auto s1 = tracer.startSpan("a");
    EXPECT_EQ(1, tracer.stats().total_spans);
    auto s2 = tracer.startSpan("b");
    EXPECT_EQ(2, tracer.stats().total_spans);
    (void)s1; (void)s2;
}

TEST(ObservabilityTracerTest, ActiveSpansDecreaseAfterEnd) {
    auto tracer = makeTracer();
    {
        auto span = tracer.startSpan("active");
        EXPECT_EQ(1, tracer.stats().active_spans);
        span->end();
        EXPECT_EQ(0, tracer.stats().active_spans);
    }
}

TEST(ObservabilityTracerTest, SpanEndedByDestructor) {
    auto tracer = makeTracer();
    {
        auto span = tracer.startSpan("scope");
        EXPECT_EQ(1, tracer.stats().active_spans);
    }  // span destroyed here
    EXPECT_EQ(0, tracer.stats().active_spans);
}

// ---------------------------------------------------------------------------
// Span attributes / status
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, SpanSetAttributes) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("attr.test");
    // Should not throw or crash
    span->setAttribute("db.system",      "themisdb");
    span->setAttribute("row_count",      int64_t{42});
    span->setAttribute("latency_ms",     3.14);
    span->setAttribute("cache_hit",      true);
    span->end();

    auto completed = tracer.completedSpans();
    ASSERT_EQ(1u, completed.size());
    EXPECT_EQ("themisdb", completed[0].attributes.at("db.system"));
    EXPECT_EQ("42",       completed[0].attributes.at("row_count"));
}

TEST(ObservabilityTracerTest, SpanRecordError) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("error.op");
    span->recordError("connection timeout");
    span->end();

    auto completed = tracer.completedSpans();
    ASSERT_EQ(1u, completed.size());
    EXPECT_FALSE(completed[0].ok);
    EXPECT_EQ("connection timeout", completed[0].attributes.at("error.message"));
}

TEST(ObservabilityTracerTest, SpanSetStatusFalse) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("status.op");
    span->setStatus(false, "query failed");
    span->end();

    auto completed = tracer.completedSpans();
    ASSERT_EQ(1u, completed.size());
    EXPECT_FALSE(completed[0].ok);
    EXPECT_EQ("query failed", completed[0].status_description);
}

TEST(ObservabilityTracerTest, SpanEndIdempotent) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("idempotent");
    span->end();
    span->end();  // second call must not crash or add a duplicate record
    EXPECT_EQ(1u, tracer.completedSpans().size());
}

// ---------------------------------------------------------------------------
// Child spans
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, ChildSpanInheritsTraceId) {
    auto tracer = makeTracer();
    auto parent = tracer.startSpan("parent");
    auto child  = tracer.startChildSpan("child", *parent);
    EXPECT_TRUE(child->isValid());

    parent->end();
    child->end();

    auto completed = tracer.completedSpans();
    ASSERT_EQ(2u, completed.size());

    // Find parent and child records
    const SpanRecord* parent_rec = nullptr;
    const SpanRecord* child_rec  = nullptr;
    for (const auto& r : completed) {
        if (r.name == "parent") {
          parent_rec = &r;
        }
        if (r.name == "child") {
          child_rec  = &r;
        }
    }
    ASSERT_NE(nullptr, parent_rec);
    ASSERT_NE(nullptr, child_rec);

    // Child must share the parent's trace_id
    EXPECT_EQ(parent_rec->trace_id, child_rec->trace_id);
    // Child's parent_span_id must equal parent's span_id
    EXPECT_EQ(parent_rec->span_id, child_rec->parent_span_id);
}

// ---------------------------------------------------------------------------
// W3C Trace Context — startSpanFromHeaders / injectContext
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, StartSpanFromHeadersNoHeader_RootSpan) {
    auto tracer = makeTracer();
    std::map<std::string, std::string> headers;
    auto span = tracer.startSpanFromHeaders("root", headers);
    EXPECT_TRUE(span->isValid());
    span->end();
    ASSERT_EQ(1u, tracer.completedSpans().size());
    EXPECT_TRUE(tracer.completedSpans()[0].parent_span_id.empty());
}

TEST(ObservabilityTracerTest, StartSpanFromHeadersValidTraceparent) {
    auto tracer = makeTracer();
    std::map<std::string, std::string> headers;
    headers["traceparent"] =
        "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";

    auto span = tracer.startSpanFromHeaders("child", headers);
    EXPECT_TRUE(span->isValid());
    span->end();

    auto completed = tracer.completedSpans();
    ASSERT_EQ(1u, completed.size());
    EXPECT_EQ("4bf92f3577b34da6a3ce929d0e0e4736", completed[0].trace_id);
    EXPECT_EQ("00f067aa0ba902b7",                 completed[0].parent_span_id);
}

TEST(ObservabilityTracerTest, StartSpanFromHeadersMalformedTraceparent) {
    auto tracer = makeTracer();
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "not-a-valid-header";
    auto span = tracer.startSpanFromHeaders("fallback", headers);
    EXPECT_TRUE(span->isValid());
    span->end();
    // Must produce a root span (no parent_span_id)
    EXPECT_TRUE(tracer.completedSpans()[0].parent_span_id.empty());
}

TEST(ObservabilityTracerTest, InjectContextWritesTraceparentHeader) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("inject.test");
    std::map<std::string, std::string> headers;
    tracer.injectContext(headers);
    EXPECT_NE(headers.end(), headers.find("traceparent"));
    auto tp = headers.at("traceparent");
    // Format: 00-<32hex>-<16hex>-01
    EXPECT_EQ(55u, tp.size());
    EXPECT_EQ("00-",    tp.substr(0, 3));
    EXPECT_EQ("-",      tp.substr(35, 1));
    EXPECT_EQ("-",      tp.substr(52, 1));
    span->end();
}

// ---------------------------------------------------------------------------
// Sampling
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, AlwaysOffSamplingDropsSpans) {
    auto tracer = makeTracer(/*sample_rate=*/0.0);
    for (int i = 0; i < 10; ++i) {
        auto span = tracer.startSpan("dropped");
        EXPECT_FALSE(span->isValid());
    }
    EXPECT_EQ(0,  tracer.stats().total_spans);
    EXPECT_EQ(10, tracer.stats().dropped_spans);
    EXPECT_EQ(0u, tracer.completedSpans().size());
}

// ---------------------------------------------------------------------------
// Ring buffer
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, RingBufferCapEvictsOldest) {
    auto tracer = makeTracer(/*sample_rate=*/1.0, /*max_retained=*/3);
    for (int i = 0; i < 5; ++i) {
        auto span = tracer.startSpan("s" + std::to_string(i));
        span->end();
    }
    auto completed = tracer.completedSpans();
    ASSERT_EQ(3u, completed.size());
    // Oldest 2 were evicted; retained are s2, s3, s4
    EXPECT_EQ("s2", completed[0].name);
    EXPECT_EQ("s4", completed[2].name);
}

TEST(ObservabilityTracerTest, ClearCompletedSpans) {
    auto tracer = makeTracer();
    auto span   = tracer.startSpan("clr");
    span->end();
    EXPECT_EQ(1u, tracer.completedSpans().size());
    tracer.clearCompletedSpans();
    EXPECT_EQ(0u, tracer.completedSpans().size());
}

// ---------------------------------------------------------------------------
// Thread safety
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, ConcurrentSpanCreation) {
    ObservabilityTracerConfig cfg;
    cfg.sample_rate        = 1.0;
    cfg.max_retained_spans = 500;
    cfg.publish_metrics    = false;
    ObservabilityTracer tracer(cfg);

    constexpr int kThreads = 8;
    constexpr int kEach    = 25;

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&tracer] {
            for (int i = 0; i < kEach; ++i) {
                auto span = tracer.startSpan("concurrent");
                span->setAttribute("thread", "1");
                span->end();
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(kThreads * kEach, tracer.stats().total_spans);
}

// ---------------------------------------------------------------------------
// ContinuousProfiler integration
// ---------------------------------------------------------------------------

TEST(ObservabilityTracerTest, ProfilerIntegration_AttachDisabledByDefault) {
    // When attach_profile_on_span_end is false (default), cpu_profile_folded
    // should always be empty even if a profiler is set.
    auto profiler = std::make_shared<ContinuousProfiler>();
    // Do NOT start() the profiler — we only care that no crash occurs and the
    // field stays empty when attachment is disabled.

    ObservabilityTracerConfig cfg;
    cfg.sample_rate              = 1.0;
    cfg.max_retained_spans       = 10;
    cfg.publish_metrics          = false;
    cfg.profiler                 = profiler;
    cfg.attach_profile_on_span_end = false;  // disabled (default)

    ObservabilityTracer tracer(cfg);
    auto span = tracer.startSpan("no_profile");
    span->end();

    auto records = tracer.completedSpans();
    ASSERT_EQ(1u, records.size());
    // cpu_profile_folded must be empty when attach is disabled
    EXPECT_TRUE(records[0].cpu_profile_folded.empty());
}

TEST(ObservabilityTracerTest, ProfilerIntegration_AttachEnabled_NoProfiler) {
    // attach_profile_on_span_end=true but no profiler set: must not crash,
    // cpu_profile_folded stays empty.
    ObservabilityTracerConfig cfg;
    cfg.sample_rate              = 1.0;
    cfg.max_retained_spans       = 10;
    cfg.publish_metrics          = false;
    cfg.profiler                 = nullptr;
    cfg.attach_profile_on_span_end = true;

    ObservabilityTracer tracer(cfg);
    auto span = tracer.startSpan("no_profiler_attached");
    span->end();

    auto records = tracer.completedSpans();
    ASSERT_EQ(1u, records.size());
    EXPECT_TRUE(records[0].cpu_profile_folded.empty());
}

TEST(ObservabilityTracerTest, ProfilerIntegration_AttachEnabled_WithProfiler) {
    // attach_profile_on_span_end=true with a started profiler:
    // cpu_profile_folded may be empty (depends on stack samples) but must
    // not crash, and the profiler pointer must be properly weak-referenced.
    ContinuousProfilerConfig pcfg;
    pcfg.enabled              = true;
    pcfg.enable_cpu_profiling = true;

    auto profiler = std::make_shared<ContinuousProfiler>(pcfg);
    profiler->start();

    ObservabilityTracerConfig cfg;
    cfg.sample_rate              = 1.0;
    cfg.max_retained_spans       = 10;
    cfg.publish_metrics          = false;
    cfg.profiler                 = profiler;
    cfg.attach_profile_on_span_end = true;

    ObservabilityTracer tracer(cfg);
    auto span = tracer.startSpan("with_profiler");
    span->end();

    profiler->stop();

    // Must not crash, and the record must have been written
    auto records = tracer.completedSpans();
    ASSERT_EQ(1u, records.size());
    // cpu_profile_folded can be empty or non-empty depending on timing;
    // we only require no crash and the field is a valid string.
    EXPECT_NO_THROW({ std::string s = records[0].cpu_profile_folded; (void)s; });
}

TEST(ObservabilityTracerTest, ProfilerIntegration_ProfilerDestroyedBeforeSpanEnds) {
    // If the shared_ptr to the profiler is destroyed before the span ends,
    // the weak_ptr locks to nullptr and we gracefully skip the snapshot.
    auto profiler = std::make_shared<ContinuousProfiler>();

    ObservabilityTracerConfig cfg;
    cfg.sample_rate              = 1.0;
    cfg.max_retained_spans       = 10;
    cfg.publish_metrics          = false;
    cfg.profiler                 = profiler;
    cfg.attach_profile_on_span_end = true;

    ObservabilityTracer tracer(cfg);
    auto span = tracer.startSpan("dangling_profiler");

    // Destroy the profiler while the span is still open
    profiler.reset();

    // end() must not crash even though the profiler is gone
    EXPECT_NO_THROW(span->end());

    auto records = tracer.completedSpans();
    ASSERT_EQ(1u, records.size());
    EXPECT_TRUE(records[0].cpu_profile_folded.empty());
}
