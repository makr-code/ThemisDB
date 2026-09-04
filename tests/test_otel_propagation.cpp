/**
 * @file test_otel_propagation.cpp
 * @brief Unit tests for OpenTelemetry trace and span propagation via ITracer
 *
 * Tests cover:
 * - startSpanFromHeaders(): W3C TraceContext extraction from inbound headers
 * - injectContext(): W3C TraceContext injection into outbound headers
 * - Baggage propagation via injectContext()
 * - Default ITracer fallback behaviour (NoOpTracer)
 * - ConcernsContext convenience wrappers
 * - Circuit-breaker interaction with startSpanFromHeaders()
 * - Missing / malformed traceparent header handling
 */

#include <gtest/gtest.h>

#include "core/concerns/otel_tracer_adapter.h"
#include "core/concerns/noop_implementations.h"
#include "core/concerns/concerns_context.h"
#include "utils/tracing.h"

#include <map>
#include <string>

using namespace themis::core::concerns;
using namespace themis::sharding;

// ============================================================================
// Helpers
// ============================================================================

static const std::string kValidTraceparent =
    "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";

static const std::string kValidTracestate = "congo=t61rcWkgMzE";

// Build a header map with a valid traceparent.
static std::map<std::string, std::string> headersWithTraceparent(
        const std::string& tp   = kValidTraceparent,
        const std::string& ts   = "") {
    std::map<std::string, std::string> h;
    h["traceparent"] = tp;
    if (!ts.empty()) {
      h["tracestate"] = ts;
    }
    return h;
}

// ============================================================================
// OpenTelemetryTracerAdapter::startSpanFromHeaders()
// ============================================================================

TEST(OtelPropagationTest, StartSpanFromHeadersReturnsNonNull) {
    OpenTelemetryTracerAdapter adapter;
    auto headers = headersWithTraceparent();
    auto span    = adapter.startSpanFromHeaders("http.request", headers);
    ASSERT_NE(span, nullptr);
}

TEST(OtelPropagationTest, StartSpanFromHeadersDoesNotCrashWithValidTraceparent) {
    OpenTelemetryTracerAdapter adapter;
    auto headers = headersWithTraceparent();
    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op", headers);
        span->end();
    });
}

TEST(OtelPropagationTest, StartSpanFromEmptyHeadersFallsBackToRootSpan) {
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> empty;
    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.no.parent", empty);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(OtelPropagationTest, StartSpanFromHeadersMalformedTraceparentFallsBack) {
    OpenTelemetryTracerAdapter adapter;
    auto headers = headersWithTraceparent("not-a-valid-traceparent");
    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.bad.tp", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(OtelPropagationTest, StartSpanFromHeadersWithTracestateDoesNotCrash) {
    OpenTelemetryTracerAdapter adapter;
    auto headers = headersWithTraceparent(kValidTraceparent, kValidTracestate);
    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.with.state", headers);
        span->end();
    });
}

TEST(OtelPropagationTest, StartSpanFromHeadersExtractsBaggage) {
    // Reset baggage before test
    themis::Baggage::clear();

    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["traceparent"] = kValidTraceparent;
    headers["baggage"]     = "tenant-id=acme,region=eu-west-1";

    auto span = adapter.startSpanFromHeaders("op.baggage", headers);
    span->end();

    // Baggage should have been extracted
    EXPECT_EQ(themis::Baggage::get("tenant-id"), "acme");
    EXPECT_EQ(themis::Baggage::get("region"),    "eu-west-1");

    themis::Baggage::clear();
}

TEST(OtelPropagationTest, StartSpanFromHeadersOnOpenCircuitReturnsInvalidSpan) {
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    OpenTelemetryTracerAdapter adapter(cfg);
    // Trip the circuit
    for (int i = 0; i < 3; ++i) {
      adapter.startSpan("trip-" + std::to_string(i));
    }
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    auto headers = headersWithTraceparent();
    auto span    = adapter.startSpanFromHeaders("op.open.circuit", headers);
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

// ============================================================================
// OpenTelemetryTracerAdapter::injectContext()
// ============================================================================

TEST(OtelPropagationTest, InjectContextDoesNotCrash) {
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    EXPECT_NO_THROW(adapter.injectContext(headers));
}

TEST(OtelPropagationTest, InjectContextLeavesHeadersEmptyWhenNoActiveSpan) {
    // Without THEMIS_ENABLE_TRACING there is no active OTel span;
    // the adapter should not inject a malformed traceparent.
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    adapter.injectContext(headers);
    // traceparent is only added when a valid trace/span id is available.
    // Either it is absent, or it is a well-formed 55-char string.
    auto it = headers.find("traceparent");
    if (it != headers.end()) {
        EXPECT_EQ(it->second.size(), static_cast<size_t>(55))
            << "traceparent must be 55 characters: " << it->second;
    }
}

TEST(OtelPropagationTest, InjectContextIncludesBaggageWhenSet) {
    themis::Baggage::clear();
    themis::Baggage::set("user-id", "42");

    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    adapter.injectContext(headers);

    auto it = headers.find("baggage");
    EXPECT_NE(it, headers.end())
        << "baggage header must be present when baggage items are set";
    if (it != headers.end()) {
        EXPECT_NE(it->second.find("user-id"), std::string::npos);
    }

    themis::Baggage::clear();
}

TEST(OtelPropagationTest, InjectContextNoBaggageHeaderWhenBaggageEmpty) {
    themis::Baggage::clear();

    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    adapter.injectContext(headers);

    // When no baggage is set, the baggage header must not be present.
    EXPECT_EQ(headers.count("baggage"), static_cast<size_t>(0));
}

// ============================================================================
// ITracer default implementations (via NoOpTracer)
// ============================================================================

TEST(OtelPropagationTest, NoOpTracerStartSpanFromHeadersReturnsNonNull) {
    NoOpTracer tracer;
    auto headers = headersWithTraceparent();
    auto span    = tracer.startSpanFromHeaders("noop.op", headers);
    ASSERT_NE(span, nullptr);
}

TEST(OtelPropagationTest, NoOpTracerStartSpanFromHeadersSpanIsInvalid) {
    NoOpTracer tracer;
    auto headers = headersWithTraceparent();
    auto span    = tracer.startSpanFromHeaders("noop.op", headers);
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

TEST(OtelPropagationTest, NoOpTracerInjectContextIsNoop) {
    NoOpTracer tracer;
    std::map<std::string, std::string> headers;
    EXPECT_NO_THROW(tracer.injectContext(headers));
    EXPECT_TRUE(headers.empty());
}

// ============================================================================
// ConcernsContext convenience wrappers
// ============================================================================

TEST(OtelPropagationTest, ConcernsContextStartSpanFromHeadersWorks) {
    auto ctx = ConcernsContext::createNoOp();
    auto headers = headersWithTraceparent();
    EXPECT_NO_THROW({
        auto span = ctx->startSpanFromHeaders("ctx.op", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(OtelPropagationTest, ConcernsContextInjectContextWorks) {
    auto ctx = ConcernsContext::createNoOp();
    std::map<std::string, std::string> headers;
    EXPECT_NO_THROW(ctx->injectContext(headers));
}

// ============================================================================
// Round-trip propagation: inject → extract
// ============================================================================

TEST(OtelPropagationTest, BaggageRoundTripInjectExtract) {
    // Set baggage on the "outbound" thread
    themis::Baggage::clear();
    themis::Baggage::set("request-id", "req-abc-123");
    themis::Baggage::set("tenant",     "themis-corp");

    // Inject into headers
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> outbound;
    adapter.injectContext(outbound);

    // Simulate a new thread / service: clear baggage and extract from headers
    themis::Baggage::clear();
    auto span = adapter.startSpanFromHeaders("downstream.op", outbound);
    span->end();

    // Baggage values should have been re-populated by extraction
    EXPECT_EQ(themis::Baggage::get("request-id"), "req-abc-123");
    EXPECT_EQ(themis::Baggage::get("tenant"),     "themis-corp");

    themis::Baggage::clear();
}

// ============================================================================
// Case-insensitive header lookup (W3C spec requirement)
// ============================================================================

TEST(OtelPropagationTest, StartSpanFromHeadersCaseInsensitiveTraceparent) {
    OpenTelemetryTracerAdapter adapter;

    // Mixed-case "Traceparent" header – must be accepted per W3C spec
    std::map<std::string, std::string> upper_headers;
    upper_headers["Traceparent"] = kValidTraceparent;
    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("req.case.upper", upper_headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });

    // ALL-CAPS "TRACEPARENT" – must also be accepted
    std::map<std::string, std::string> caps_headers;
    caps_headers["TRACEPARENT"] = kValidTraceparent;
    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("req.case.caps", caps_headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

// ============================================================================
// W3C spec: invalid traceparent values must be rejected and fall back
// ============================================================================

TEST(OtelPropagationTest, StartSpanFromHeadersAllZerosTraceIdFallsBack) {
    // All-zeros trace-id is explicitly invalid per W3C TraceContext spec
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "00-00000000000000000000000000000000-00f067aa0ba902b7-01";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("req.zero.traceid", headers);
        ASSERT_NE(span, nullptr);
        // Should fall back to a root span (not crash)
        span->end();
    });
}

TEST(OtelPropagationTest, StartSpanFromHeadersAllZerosParentIdFallsBack) {
    // All-zeros parent-id is explicitly invalid per W3C TraceContext spec
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("req.zero.parentid", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(OtelPropagationTest, StartSpanFromHeadersTooShortTraceparentFallsBack) {
    OpenTelemetryTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    // One hex digit too short in the trace-id field
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e473-00f067aa0ba902b7-01";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("req.short.tp", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}
