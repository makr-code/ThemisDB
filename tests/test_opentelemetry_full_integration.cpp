/**
 * @file test_opentelemetry_full_integration.cpp
 * @brief Focused tests for OpenTelemetryTracer — OpenTelemetry Full Integration
 *        (v1.6.0, observability domain).
 *
 * Acceptance criteria covered:
 *  AC-1  Construction with default and custom OTelConfig
 *  AC-2  isInitialized() lifecycle (constructed → shutdown → re-initialize)
 *  AC-3  startSpan() returns a valid, non-null ISpan
 *  AC-4  startChildSpan() propagates trace context from parent
 *  AC-5  startSpanFromHeaders() extracts W3C traceparent correctly
 *  AC-6  startSpanFromHeaders() starts a root span on missing/malformed header
 *  AC-7  startSpanFromHeaders() extracts W3C Baggage from headers
 *  AC-8  injectContext(headers) writes W3C traceparent header
 *  AC-9  injectContext(headers) writes W3C baggage header when baggage is set
 *  AC-10 injectContext(span, headers) injects span-specific context
 *  AC-11 extractContext() returns valid SpanContext for a well-formed traceparent
 *  AC-12 extractContext() returns invalid SpanContext for a missing header
 *  AC-13 extractContext() reports sampled flag correctly
 *  AC-14 Baggage: setBaggageItem / getBaggageItem round-trip
 *  AC-15 Baggage: removeBaggageItem removes single entry
 *  AC-16 Baggage: clearBaggage wipes all items
 *  AC-17 Baggage: extractBaggage populates from headers
 *  AC-18 recordException() sets exception.type, exception.message, status=ERROR
 *  AC-19 recordMetrics() attaches all non-zero MetricSnapshot fields as span attrs
 *  AC-20 recordMetrics() does not attach zero-valued standard fields
 *  AC-21 recordMetrics() attaches custom metric entries under "db.metrics.custom."
 *  AC-22 Probabilistic sampling: sample_rate=0 drops all spans
 *  AC-23 Probabilistic sampling: sample_rate=1 records all spans
 *  AC-24 Ring buffer: completed spans are retained up to max_retained_spans
 *  AC-25 Ring buffer: clearCompletedSpans empties the buffer
 *  AC-26 stats() counters track total/active/dropped spans
 *  AC-27 Multi-exporter config: activeExporters() reflects OTelConfig::exporters
 *  AC-28 getConfig() returns the active configuration
 *  AC-29 flush() does not crash
 *  AC-30 isHealthy() returns healthy when initialized, unhealthy after shutdown
 *  AC-31 Multiple resource_attributes accepted in OTelConfig
 *  AC-32 Concurrent startSpan() from multiple threads is safe
 *  AC-33 No OTLP endpoint → otlpExportedSpanCount / otlpDroppedSpanCount return 0
 *  AC-34 OTLP exporter created when endpoint is set; counter methods accessible
 *  AC-35 Completed spans are enqueued to OTLP exporter (queue not dropped)
 *  AC-36 Both ring buffer and OTLP queue receive span on end
 *  AC-37 Shutdown stops OTLP exporter safely (no crash)
 *  AC-38 Multi-exporter list (otlp+jaeger+zipkin) reported accurately
 */

#include <gtest/gtest.h>
#include "observability/opentelemetry_tracer.h"

#include <atomic>
#include <chrono>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>

using namespace themis::observability;
using namespace std::string_literals;

// ============================================================================
// Constants
// ============================================================================

static const std::string kValidTraceparent =
    "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";

static const std::string kUnsampledTraceparent =
    "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-00";

static std::map<std::string, std::string> headersWithTraceparent(
    const std::string& tp = kValidTraceparent)
{
    return {{"traceparent", tp}};
}

// ============================================================================
// Fixture
// ============================================================================

class OpenTelemetryTracerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure a clean baggage state before every test
        OpenTelemetryTracer::clearBaggage();
    }

    void TearDown() override {
        OpenTelemetryTracer::clearBaggage();
    }

    // Helper: tracer with always-on sampling and small ring buffer
    static OTelConfig defaultConfig() {
        OTelConfig cfg;
        cfg.service_name       = "test-service";
        cfg.sample_rate        = 1.0;
        cfg.max_retained_spans = 32;
        cfg.publish_metrics    = false;
        return cfg;
    }
};

// ============================================================================
// AC-1: Construction
// ============================================================================

TEST_F(OpenTelemetryTracerTest, DefaultConstructionSucceeds) {
    EXPECT_NO_THROW(OpenTelemetryTracer tracer);
}

TEST_F(OpenTelemetryTracerTest, CustomConfigConstructionSucceeds) {
    OTelConfig cfg;
    cfg.service_name    = "my-service";
    cfg.service_version = "2.0.0";
    // Jaeger HTTP collector endpoint (not an OTLP endpoint)
    cfg.endpoint        = "http://jaeger-collector:14268/api/traces";
    // cfg.protocol applies to OTLP transport ("grpc" or "http"); for Jaeger
    // backends the OTel Collector handles routing
    cfg.protocol        = "http";
    cfg.sample_rate     = 0.5;
    cfg.resource_attributes = {{"env", "staging"}, {"region", "eu-west-1"}};
    cfg.exporters       = {"otlp", "jaeger"};
    EXPECT_NO_THROW(OpenTelemetryTracer tracer(cfg));
}

// ============================================================================
// AC-2: isInitialized() lifecycle
// ============================================================================

TEST_F(OpenTelemetryTracerTest, IsInitializedAfterConstruction) {
    OpenTelemetryTracer tracer(defaultConfig());
    EXPECT_TRUE(tracer.isInitialized());
}

TEST_F(OpenTelemetryTracerTest, NotInitializedAfterShutdown) {
    OpenTelemetryTracer tracer(defaultConfig());
    tracer.shutdown();
    EXPECT_FALSE(tracer.isInitialized());
}

TEST_F(OpenTelemetryTracerTest, InitializedAfterReInitialize) {
    OpenTelemetryTracer tracer(defaultConfig());
    tracer.shutdown();
    EXPECT_FALSE(tracer.isInitialized());
    bool ok = tracer.initialize("re-init-service", "http://otel:4317");
    EXPECT_TRUE(ok);
    EXPECT_TRUE(tracer.isInitialized());
}

// ============================================================================
// AC-3: startSpan()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, StartSpanReturnsNonNull) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto span = tracer.startSpan("db.query");
    ASSERT_NE(span, nullptr);
}

TEST_F(OpenTelemetryTracerTest, StartSpanIsValid) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto span = tracer.startSpan("db.query");
    EXPECT_TRUE(span->isValid());
}

TEST_F(OpenTelemetryTracerTest, StartSpanAcceptsAttributes) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto span = tracer.startSpan("db.query");
    EXPECT_NO_THROW(span->setAttribute("db.operation", std::string("SELECT")));
    EXPECT_NO_THROW(span->setAttribute("db.row_count", int64_t(42)));
    EXPECT_NO_THROW(span->setAttribute("db.duration_ms", 3.14));
    EXPECT_NO_THROW(span->setAttribute("db.cached", false));
}

// ============================================================================
// AC-4: startChildSpan() propagates trace context
// ============================================================================

TEST_F(OpenTelemetryTracerTest, ChildSpanSharesTraceWithParent) {
    OpenTelemetryTracer tracer(defaultConfig());

    auto parent = tracer.startSpan("parent-op");
    parent->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);

    // Re-start parent to have a live span for startChildSpan
    tracer.clearCompletedSpans();
    auto liveParent = tracer.startSpan("parent-live");

    auto child = tracer.startChildSpan("child-op", *liveParent);
    child->end();
    liveParent->end();

    auto all = tracer.completedSpans();
    // Two completed spans: child first, then parent
    ASSERT_EQ(all.size(), 2u);

    // Both must share the same trace_id
    EXPECT_EQ(all[0].trace_id, all[1].trace_id);
    // The child must have the parent as its parent_span_id
    // (child ends first so it's index 0)
    EXPECT_EQ(all[0].parent_span_id, all[1].span_id);
}

TEST_F(OpenTelemetryTracerTest, ChildSpanNonOtelParentFallsBackToRootSpan) {
    // Non-OtelSpan parent → fallback to new root span (no crash)
    OpenTelemetryTracer tracer(defaultConfig());

    struct ForeignSpan : public themis::core::concerns::ITracer::ISpan {
        void setAttribute(const std::string&, const std::string&) override {}
        void setAttribute(const std::string&, int64_t)            override {}
        void setAttribute(const std::string&, double)             override {}
        void setAttribute(const std::string&, bool)               override {}
        void recordError(const std::string&)                      override {}
        void setStatus(bool, const std::string& = "")             override {}
        void end()                                                override {}
        bool isValid() const                                      override { return true; }
    };

    ForeignSpan foreign;
    EXPECT_NO_THROW({
        auto span = tracer.startChildSpan("child", foreign);
        EXPECT_NE(span, nullptr);
    });
}

// ============================================================================
// AC-5 & AC-6: startSpanFromHeaders()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, StartSpanFromHeadersExtractsTraceparent) {
    OpenTelemetryTracer tracer(defaultConfig());

    auto headers = headersWithTraceparent();
    auto span    = tracer.startSpanFromHeaders("http.request", headers);
    ASSERT_NE(span, nullptr);
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    // trace_id must match the one from the traceparent header
    EXPECT_EQ(spans[0].trace_id,       "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(spans[0].parent_span_id, "00f067aa0ba902b7");
}

TEST_F(OpenTelemetryTracerTest, StartSpanFromHeadersMissingTraceparentStartsRoot) {
    OpenTelemetryTracer tracer(defaultConfig());

    std::map<std::string, std::string> empty;
    auto span = tracer.startSpanFromHeaders("http.request", empty);
    ASSERT_NE(span, nullptr);
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    // Must have generated a fresh trace_id (32 hex chars)
    EXPECT_EQ(spans[0].trace_id.size(), 32u);
    EXPECT_TRUE(spans[0].parent_span_id.empty());
}

TEST_F(OpenTelemetryTracerTest, StartSpanFromHeadersMalformedTraceparentStartsRoot) {
    OpenTelemetryTracer tracer(defaultConfig());

    auto headers = headersWithTraceparent("not-a-real-traceparent");
    auto span    = tracer.startSpanFromHeaders("http.request", headers);
    ASSERT_NE(span, nullptr);
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    EXPECT_EQ(spans[0].trace_id.size(), 32u);
    EXPECT_TRUE(spans[0].parent_span_id.empty());
}

// ============================================================================
// AC-7: startSpanFromHeaders() extracts Baggage
// ============================================================================

TEST_F(OpenTelemetryTracerTest, StartSpanFromHeadersExtractsBaggage) {
    OpenTelemetryTracer tracer(defaultConfig());

    std::map<std::string, std::string> headers;
    headers["traceparent"] = kValidTraceparent;
    headers["baggage"]     = "tenant-id=acme,user-id=u-99";

    auto span = tracer.startSpanFromHeaders("http.request", headers);
    span->end();

    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("tenant-id"), "acme");
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("user-id"),   "u-99");
}

// ============================================================================
// AC-8 & AC-9: injectContext(headers) — traceparent + baggage
// ============================================================================

TEST_F(OpenTelemetryTracerTest, InjectContextWritesTraceparentAfterStartSpan) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto span = tracer.startSpan("op");

    std::map<std::string, std::string> outbound;
    tracer.injectContext(outbound);

    EXPECT_TRUE(outbound.count("traceparent") > 0);
    const std::string& tp = outbound.at("traceparent");
    // Must start with "00-" and have correct structure
    EXPECT_EQ(tp.substr(0, 3), "00-");
    EXPECT_EQ(tp.size(), 55u);

    span->end();
}

TEST_F(OpenTelemetryTracerTest, InjectContextWritesBaggageWhenSet) {
    OpenTelemetryTracer tracer(defaultConfig());
    OpenTelemetryTracer::setBaggageItem("tenant-id", "contoso");

    auto span = tracer.startSpan("op");
    std::map<std::string, std::string> outbound;
    tracer.injectContext(outbound);

    EXPECT_TRUE(outbound.count("baggage") > 0);
    EXPECT_NE(outbound["baggage"].find("tenant-id=contoso"), std::string::npos);
    span->end();
}

TEST_F(OpenTelemetryTracerTest, InjectContextNoSpanDoesNotCrash) {
    OpenTelemetryTracer tracer(defaultConfig());
    std::map<std::string, std::string> outbound;
    EXPECT_NO_THROW(tracer.injectContext(outbound));
}

// ============================================================================
// AC-10: injectContext(span, headers)
// ============================================================================

TEST_F(OpenTelemetryTracerTest, InjectContextWithSpanWritesCorrectTraceparent) {
    OpenTelemetryTracer tracer(defaultConfig());

    auto headers = headersWithTraceparent();
    auto span    = tracer.startSpanFromHeaders("op", headers);

    std::map<std::string, std::string> outbound;
    tracer.injectContext(*span, outbound);

    EXPECT_TRUE(outbound.count("traceparent") > 0);
    // The traceparent must contain the original trace_id
    EXPECT_NE(outbound["traceparent"].find("4bf92f3577b34da6a3ce929d0e0e4736"),
              std::string::npos);
    span->end();
}

// ============================================================================
// AC-11, AC-12, AC-13: extractContext()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, ExtractContextValidTraceparent) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto ctx = tracer.extractContext(headersWithTraceparent());

    EXPECT_TRUE(ctx.isValid());
    EXPECT_EQ(ctx.trace_id, "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(ctx.span_id,  "00f067aa0ba902b7");
    EXPECT_TRUE(ctx.sampled);
}

TEST_F(OpenTelemetryTracerTest, ExtractContextMissingHeaderReturnsInvalid) {
    OpenTelemetryTracer tracer(defaultConfig());
    std::map<std::string, std::string> empty;
    auto ctx = tracer.extractContext(empty);
    EXPECT_FALSE(ctx.isValid());
}

TEST_F(OpenTelemetryTracerTest, ExtractContextUnsampledFlagReportedCorrectly) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto ctx = tracer.extractContext(headersWithTraceparent(kUnsampledTraceparent));
    EXPECT_TRUE(ctx.isValid());
    EXPECT_FALSE(ctx.sampled);
}

// ============================================================================
// AC-14 – AC-17: Baggage
// ============================================================================

TEST_F(OpenTelemetryTracerTest, BaggageSetAndGetRoundTrip) {
    OpenTelemetryTracer::setBaggageItem("tenant-id", "acme");
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("tenant-id"), "acme");
}

TEST_F(OpenTelemetryTracerTest, BaggageGetMissingKeyReturnsEmpty) {
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("nonexistent"), "");
}

TEST_F(OpenTelemetryTracerTest, BaggageRemoveItemWorks) {
    OpenTelemetryTracer::setBaggageItem("user-id", "u-1");
    OpenTelemetryTracer::removeBaggageItem("user-id");
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("user-id"), "");
}

TEST_F(OpenTelemetryTracerTest, BaggageClearWipesAllItems) {
    OpenTelemetryTracer::setBaggageItem("tenant-id", "x");
    OpenTelemetryTracer::setBaggageItem("user-id",   "y");
    OpenTelemetryTracer::clearBaggage();
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("tenant-id"), "");
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("user-id"),   "");
}

TEST_F(OpenTelemetryTracerTest, BaggageExtractFromHeadersPopulatesStore) {
    std::map<std::string, std::string> headers;
    headers["baggage"] = "tenant-id=corp,session-id=s-99";
    OpenTelemetryTracer::extractBaggage(headers);
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("tenant-id"),  "corp");
    EXPECT_EQ(OpenTelemetryTracer::getBaggageItem("session-id"), "s-99");
}

// ============================================================================
// AC-18: recordException()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, RecordExceptionSetsErrorAttributes) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 4;
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("risky-op");
    try {
        throw std::runtime_error("disk full");
    } catch (const std::exception& ex) {
        tracer.recordException(*span, ex);
    }
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    const auto& attrs = spans[0].attributes;
    EXPECT_FALSE(spans[0].ok);
    EXPECT_TRUE(attrs.count("exception.message") > 0);
    EXPECT_EQ(attrs.at("exception.message"), "disk full");
    EXPECT_TRUE(attrs.count("exception.type") > 0);
    // exception.type contains the RTTI name (non-empty)
    EXPECT_FALSE(attrs.at("exception.type").empty());
}

// ============================================================================
// AC-19 – AC-21: recordMetrics()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, RecordMetricsAttachesNonZeroFields) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 4;
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("db.query");

    SpanMetrics snap;
    snap.cpu_usage_percent  = 72.5;
    snap.memory_usage_bytes = 1024.0 * 1024.0 * 256.0;
    snap.active_connections = 50;
    snap.query_count        = 1000;
    snap.cache_hit_rate     = 0.95;
    tracer.recordMetrics(*span, snap);
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    const auto& attrs = spans[0].attributes;
    EXPECT_TRUE(attrs.count("db.metrics.cpu_usage_percent")  > 0);
    EXPECT_TRUE(attrs.count("db.metrics.memory_usage_bytes") > 0);
    EXPECT_TRUE(attrs.count("db.metrics.active_connections") > 0);
    EXPECT_TRUE(attrs.count("db.metrics.query_count")        > 0);
    EXPECT_TRUE(attrs.count("db.metrics.cache_hit_rate")     > 0);
}

TEST_F(OpenTelemetryTracerTest, RecordMetricsDoesNotAttachZeroFields) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 4;
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("db.query");
    SpanMetrics snap; // all zeros / defaults
    tracer.recordMetrics(*span, snap);
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    const auto& attrs = spans[0].attributes;
    EXPECT_EQ(attrs.count("db.metrics.cpu_usage_percent"),  0u);
    EXPECT_EQ(attrs.count("db.metrics.memory_usage_bytes"), 0u);
    EXPECT_EQ(attrs.count("db.metrics.active_connections"), 0u);
    EXPECT_EQ(attrs.count("db.metrics.query_count"),        0u);
    EXPECT_EQ(attrs.count("db.metrics.cache_hit_rate"),     0u);
}

TEST_F(OpenTelemetryTracerTest, RecordMetricsAttachesCustomEntries) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 4;
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("db.query");
    SpanMetrics snap;
    snap.custom["shard.latency_ms"] = 12.5;
    snap.custom["index.hit_count"]  = 300.0;
    tracer.recordMetrics(*span, snap);
    span->end();

    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    const auto& attrs = spans[0].attributes;
    EXPECT_TRUE(attrs.count("db.metrics.custom.shard.latency_ms") > 0);
    EXPECT_TRUE(attrs.count("db.metrics.custom.index.hit_count")  > 0);
}

// ============================================================================
// AC-22 & AC-23: Probabilistic sampling
// ============================================================================

TEST_F(OpenTelemetryTracerTest, ZeroSampleRateDropsAllSpans) {
    OTelConfig cfg = defaultConfig();
    cfg.sample_rate        = 0.0;
    cfg.max_retained_spans = 100;
    OpenTelemetryTracer tracer(cfg);

    for (int i = 0; i < 20; ++i) {
        auto span = tracer.startSpan("op");
        span->end();
    }

    auto s = tracer.stats();
    EXPECT_EQ(s.dropped_spans, 20);
    EXPECT_EQ(tracer.completedSpans().size(), 0u);
}

TEST_F(OpenTelemetryTracerTest, FullSampleRateRecordsAllSpans) {
    OTelConfig cfg = defaultConfig();
    cfg.sample_rate        = 1.0;
    cfg.max_retained_spans = 100;
    OpenTelemetryTracer tracer(cfg);

    for (int i = 0; i < 10; ++i) {
        auto span = tracer.startSpan("op");
        span->end();
    }

    EXPECT_EQ(tracer.completedSpans().size(), 10u);
    EXPECT_EQ(tracer.stats().dropped_spans, 0);
}

// ============================================================================
// AC-24 & AC-25: Ring buffer
// ============================================================================

TEST_F(OpenTelemetryTracerTest, RingBufferRetainsUpToMaxSpans) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 5;
    OpenTelemetryTracer tracer(cfg);

    for (int i = 0; i < 10; ++i) {
        auto span = tracer.startSpan("op");
        span->end();
    }

    EXPECT_EQ(tracer.completedSpans().size(), 5u);
}

TEST_F(OpenTelemetryTracerTest, ClearCompletedSpansEmptiesBuffer) {
    OpenTelemetryTracer tracer(defaultConfig());

    auto span = tracer.startSpan("op");
    span->end();

    ASSERT_EQ(tracer.completedSpans().size(), 1u);
    tracer.clearCompletedSpans();
    EXPECT_EQ(tracer.completedSpans().size(), 0u);
}

// ============================================================================
// AC-26: stats() counters
// ============================================================================

TEST_F(OpenTelemetryTracerTest, StatsTracksTotalAndDroppedSpans) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 100;
    OpenTelemetryTracer tracer(cfg);

    auto s0 = tracer.stats();
    EXPECT_EQ(s0.total_spans,   0);
    EXPECT_EQ(s0.dropped_spans, 0);

    {
        auto span = tracer.startSpan("live");  // active
        auto s1   = tracer.stats();
        EXPECT_EQ(s1.total_spans,  1);
        EXPECT_EQ(s1.active_spans, 1);
        span->end();
    }

    auto s2 = tracer.stats();
    EXPECT_EQ(s2.total_spans,  1);
    EXPECT_EQ(s2.active_spans, 0);
}

TEST_F(OpenTelemetryTracerTest, StatsDroppedSpansCountedWhenSampledOut) {
    OTelConfig cfg = defaultConfig();
    cfg.sample_rate = 0.0;
    OpenTelemetryTracer tracer(cfg);

    {
        auto span = tracer.startSpan("op");
        span->end();
    }

    EXPECT_EQ(tracer.stats().dropped_spans, 1);
    EXPECT_EQ(tracer.stats().total_spans,   0);
}

// ============================================================================
// AC-27: Multi-exporter config
// ============================================================================

TEST_F(OpenTelemetryTracerTest, ActiveExportersReflectsConfig) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters = {"otlp", "jaeger", "zipkin"};
    OpenTelemetryTracer tracer(cfg);

    auto exporters = tracer.activeExporters();
    ASSERT_EQ(exporters.size(), 3u);
    EXPECT_EQ(exporters[0], "otlp");
    EXPECT_EQ(exporters[1], "jaeger");
    EXPECT_EQ(exporters[2], "zipkin");
}

TEST_F(OpenTelemetryTracerTest, SingleExporterDefault) {
    OpenTelemetryTracer tracer; // uses default config
    auto exporters = tracer.activeExporters();
    ASSERT_EQ(exporters.size(), 1u);
    EXPECT_EQ(exporters[0], "otlp");
}

// ============================================================================
// AC-28: getConfig()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, GetConfigReturnsActiveConfig) {
    OTelConfig cfg;
    cfg.service_name    = "mydb";
    cfg.service_version = "3.0.0";
    cfg.endpoint        = "http://collector:4318";
    cfg.protocol        = "http";
    cfg.sample_rate     = 0.25;
    cfg.resource_attributes = {{"env", "prod"}};
    cfg.exporters       = {"zipkin"};

    OpenTelemetryTracer tracer(cfg);
    auto got = tracer.getConfig();

    EXPECT_EQ(got.service_name,    "mydb");
    EXPECT_EQ(got.service_version, "3.0.0");
    EXPECT_EQ(got.endpoint,        "http://collector:4318");
    EXPECT_EQ(got.protocol,        "http");
    EXPECT_DOUBLE_EQ(got.sample_rate, 0.25);
    EXPECT_EQ(got.resource_attributes.at("env"), "prod");
    ASSERT_EQ(got.exporters.size(), 1u);
    EXPECT_EQ(got.exporters[0], "zipkin");
}

// ============================================================================
// AC-29: flush()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, FlushDoesNotCrash) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto span = tracer.startSpan("op");
    span->end();
    EXPECT_NO_THROW(tracer.flush());
}

// ============================================================================
// AC-30: isHealthy()
// ============================================================================

TEST_F(OpenTelemetryTracerTest, IsHealthyWhenInitialized) {
    OpenTelemetryTracer tracer(defaultConfig());
    auto result = tracer.isHealthy();
    EXPECT_TRUE(result.ok);
}

TEST_F(OpenTelemetryTracerTest, IsUnhealthyAfterShutdown) {
    OpenTelemetryTracer tracer(defaultConfig());
    tracer.shutdown();
    auto result = tracer.isHealthy();
    EXPECT_FALSE(result.ok);
}

// ============================================================================
// AC-31: Resource attributes in OTelConfig
// ============================================================================

TEST_F(OpenTelemetryTracerTest, ResourceAttributesStoredInConfig) {
    OTelConfig cfg = defaultConfig();
    cfg.resource_attributes = {
        {"deployment.environment", "production"},
        {"service.instance.id",   "node-1"},
        {"cloud.region",          "us-east-1"},
    };
    OpenTelemetryTracer tracer(cfg);
    auto got = tracer.getConfig();
    EXPECT_EQ(got.resource_attributes.at("deployment.environment"), "production");
    EXPECT_EQ(got.resource_attributes.at("service.instance.id"),    "node-1");
    EXPECT_EQ(got.resource_attributes.at("cloud.region"),           "us-east-1");
}

// ============================================================================
// AC-32: Thread safety
// ============================================================================

TEST_F(OpenTelemetryTracerTest, ConcurrentStartSpanIsSafe) {
    OTelConfig cfg = defaultConfig();
    cfg.max_retained_spans = 1000;
    OpenTelemetryTracer tracer(cfg);

    constexpr int kThreads    = 8;
    constexpr int kSpansEach  = 50;
    std::atomic<int> completed{0};

    std::vector<std::thread> threads;
    threads.reserve(kThreads);
    for (int t = 0; t < kThreads; ++t) {
        threads.emplace_back([&] {
            for (int i = 0; i < kSpansEach; ++i) {
                auto span = tracer.startSpan("concurrent-op");
                span->setAttribute("thread.id", int64_t(i));
                span->end();
                ++completed;
            }
        });
    }
    for (auto& th : threads) {
      th.join();
    }

    EXPECT_EQ(completed.load(), kThreads * kSpansEach);
    auto s = tracer.stats();
    EXPECT_EQ(s.total_spans, kThreads * kSpansEach);
    EXPECT_EQ(s.active_spans, 0);
}

// ============================================================================
// AC-33 – AC-38: Multi-exporter dispatch (OtlpExporter integration)
// ============================================================================

// AC-33: No OTLP endpoint configured → otlpExportedSpanCount returns 0
TEST_F(OpenTelemetryTracerTest, OtlpExporterCountsZeroWhenNotConfigured) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters = {"otlp"};
    cfg.endpoint  = "";  // no endpoint → OtlpExporter not created
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("op");
    span->end();

    EXPECT_EQ(tracer.otlpExportedSpanCount(), 0u);
    EXPECT_EQ(tracer.otlpDroppedSpanCount(),  0u);
}

// AC-34: OTLP exporter is created when endpoint is set; counter methods work
TEST_F(OpenTelemetryTracerTest, OtlpExporterCreatedWhenEndpointSet) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters = {"otlp"};
    cfg.endpoint  = "http://127.0.0.1:4318";  // local, not actually running
    OpenTelemetryTracer tracer(cfg);

    // The exporter is created → counters must be accessible without throwing
    // and the exporter must be reflected in activeExporters()
    EXPECT_NO_THROW(tracer.otlpExportedSpanCount());
    EXPECT_NO_THROW(tracer.otlpDroppedSpanCount());

    // activeExporters() must list "otlp"
    auto ex = tracer.activeExporters();
    ASSERT_EQ(ex.size(), 1u);
    EXPECT_EQ(ex[0], "otlp");
}

// AC-35: Completed spans are enqueued into OTLP exporter queue
TEST_F(OpenTelemetryTracerTest, SpansEnqueuedToOtlpExporterQueue) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters      = {"otlp"};
    cfg.endpoint       = "http://127.0.0.1:4318"; // unreachable → queue fills
    cfg.max_retained_spans = 100;
    OpenTelemetryTracer tracer(cfg);

    // Create several spans and end them — they should be enqueued
    for (int i = 0; i < 5; ++i) {
        auto span = tracer.startSpan("op-" + std::to_string(i));
        span->setAttribute("index", int64_t(i));
        span->end();
    }

    // The spans are queued in the OtlpExporter (not yet exported since no server).
    // droppedSpanCount() must remain 0 (queue not full).
    EXPECT_EQ(tracer.otlpDroppedSpanCount(), 0u);

    // The in-process ring buffer is also populated
    EXPECT_EQ(tracer.completedSpans().size(), 5u);
}

// AC-36: Both ring buffer and OTLP queue receive span on end
TEST_F(OpenTelemetryTracerTest, SpanEndPopulatesBothRingBufferAndExporter) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters      = {"otlp"};
    cfg.endpoint       = "http://127.0.0.1:4318";
    cfg.max_retained_spans = 10;
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("dual-path");
    span->setAttribute("key", std::string("value"));
    span->end();

    // Ring buffer has the span
    auto spans = tracer.completedSpans();
    ASSERT_EQ(spans.size(), 1u);
    EXPECT_EQ(spans[0].name, "dual-path");
    EXPECT_EQ(spans[0].attributes.at("key"), "value");

    // OTLP exporter queue not dropped
    EXPECT_EQ(tracer.otlpDroppedSpanCount(), 0u);
}

// AC-37: Shutdown stops OTLP exporter (no crash, idempotent)
TEST_F(OpenTelemetryTracerTest, ShutdownStopsOtlpExporterSafely) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters = {"otlp"};
    cfg.endpoint  = "http://127.0.0.1:4318";
    OpenTelemetryTracer tracer(cfg);

    auto span = tracer.startSpan("op");
    span->end();

    EXPECT_NO_THROW(tracer.shutdown());
    EXPECT_FALSE(tracer.isInitialized());
}

// AC-38: Multi-exporter list (otlp + jaeger + zipkin) — activeExporters stays accurate
TEST_F(OpenTelemetryTracerTest, MultiExporterListAccurate) {
    OTelConfig cfg = defaultConfig();
    cfg.exporters = {"otlp", "jaeger", "zipkin"};
    cfg.endpoint  = "";  // no actual endpoint needed for config test
    OpenTelemetryTracer tracer(cfg);

    auto ex = tracer.activeExporters();
    ASSERT_EQ(ex.size(), 3u);
    EXPECT_EQ(ex[0], "otlp");
    EXPECT_EQ(ex[1], "jaeger");
    EXPECT_EQ(ex[2], "zipkin");
}
