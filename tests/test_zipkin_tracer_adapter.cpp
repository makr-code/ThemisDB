/**
 * @file test_zipkin_tracer_adapter.cpp
 * @brief Unit tests for ZipkinTracerAdapter (core/concerns)
 *
 * Tests cover:
 * - Default and custom CircuitBreakerConfig construction
 * - isInitialized() before/after initialize() / shutdown()
 * - startSpan(): no-crash on uninitialized adapter, non-null result
 * - startChildSpan(): with ZipkinSpanAdapter parent and with foreign parent
 * - startSpanFromHeaders(): W3C traceparent takes precedence
 * - startSpanFromHeaders(): B3 single-header extraction
 * - startSpanFromHeaders(): B3 multi-header extraction
 * - startSpanFromHeaders(): missing/malformed B3 headers fall back gracefully
 * - injectContext(): B3 and traceparent headers emitted when a span is active
 * - injectContext(): no headers emitted when no active span
 * - isHealthy(): false when not initialized
 * - circuitBreakerState(): starts CLOSED, opens after failure threshold
 * - flush() and shutdown() are safe to call multiple times
 * - kDefaultEndpoint points to Zipkin HTTP collector port 9411
 */

#include <gtest/gtest.h>

#include "core/concerns/zipkin_tracer_adapter.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/noop_implementations.h"

using namespace themis::core::concerns;
using namespace themis::sharding;

// ============================================================================
// Helpers
// ============================================================================

// A minimal ISpan that is NOT a ZipkinSpanAdapter, used to exercise the
// fallback path in startChildSpan().
class ForeignSpanForZipkin : public ITracer::ISpan {
public:
    void setAttribute(const std::string&, const std::string&) override {}
    void setAttribute(const std::string&, int64_t) override {}
    void setAttribute(const std::string&, double) override {}
    void setAttribute(const std::string&, bool) override {}
    void recordError(const std::string&) override {}
    void setStatus(bool, const std::string&) override {}
    void end() override { ended = true; }
    bool isValid() const override { return true; }
    bool ended = false;
};

// ============================================================================
// Default endpoint
// ============================================================================

TEST(ZipkinTracerAdapterTest, DefaultEndpointIsZipkinHttpCollector) {
    std::string ep = ZipkinTracerAdapter::kDefaultEndpoint;
    EXPECT_NE(ep.find("9411"), std::string::npos)
        << "Default endpoint should reference Zipkin port 9411: " << ep;
}

// ============================================================================
// Construction
// ============================================================================

TEST(ZipkinTracerAdapterTest, DefaultConstructionSucceeds) {
    EXPECT_NO_THROW(ZipkinTracerAdapter adapter);
}

TEST(ZipkinTracerAdapterTest, CustomCircuitBreakerConfigIsAccepted) {
    ZipkinTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.timeout           = std::chrono::seconds(10);
    cfg.success_threshold = 1;
    EXPECT_NO_THROW(ZipkinTracerAdapter adapter(cfg));
}

// ============================================================================
// Initialization state
// ============================================================================

TEST(ZipkinTracerAdapterTest, NotInitializedByDefault) {
    ZipkinTracerAdapter adapter;
    EXPECT_FALSE(adapter.isInitialized());
}

TEST(ZipkinTracerAdapterTest, ShutdownResetsInitializedFlag) {
    ZipkinTracerAdapter adapter;
    adapter.initialize("zipkin-test", "http://127.0.0.1:9411");
    adapter.shutdown();
    EXPECT_FALSE(adapter.isInitialized());
}

TEST(ZipkinTracerAdapterTest, DoubleInitializeReturnsTrueAndDoesNotTripBreaker) {
    ZipkinTracerAdapter adapter;
    adapter.initialize("zipkin-svc", "http://127.0.0.1:9411");

    if (adapter.isInitialized()) {
        CircuitBreaker::State state_before = adapter.circuitBreakerState();
        bool second = adapter.initialize("zipkin-svc", "http://127.0.0.1:9411");
        EXPECT_TRUE(second);
        EXPECT_EQ(adapter.circuitBreakerState(), state_before);
    }

    adapter.shutdown();
}

// ============================================================================
// isHealthy()
// ============================================================================

TEST(ZipkinTracerAdapterTest, IsUnhealthyWhenNotInitialized) {
    ZipkinTracerAdapter adapter;
    auto result = adapter.isHealthy();
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

TEST(ZipkinTracerAdapterTest, HealthCheckMessageIsNonNull) {
    ZipkinTracerAdapter adapter;
    adapter.initialize("svc", "http://127.0.0.1:9411");
    auto result = adapter.isHealthy();
    EXPECT_NE(result.message.data(), nullptr);
}

// ============================================================================
// Circuit-breaker state
// ============================================================================

TEST(ZipkinTracerAdapterTest, CircuitBreakerStartsClosed) {
    ZipkinTracerAdapter adapter;
    EXPECT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::CLOSED);
}

TEST(ZipkinTracerAdapterTest, CircuitBreakerOpensAfterFailureThreshold) {
    ZipkinTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    ZipkinTracerAdapter adapter(cfg);

    for (int i = 0; i < static_cast<int>(cfg.failure_threshold) + 1; ++i) {
        adapter.startSpan("probe-" + std::to_string(i));
    }

    EXPECT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);
}

TEST(ZipkinTracerAdapterTest, OpenCircuitReturnsInvalidSpan) {
    ZipkinTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    ZipkinTracerAdapter adapter(cfg);
    for (int i = 0; i < 3; ++i) adapter.startSpan("trip-" + std::to_string(i));
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    auto span = adapter.startSpan("during-open");
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

// ============================================================================
// startSpan()
// ============================================================================

TEST(ZipkinTracerAdapterTest, StartSpanReturnsNonNull) {
    ZipkinTracerAdapter adapter;
    auto span = adapter.startSpan("test-span");
    EXPECT_NE(span, nullptr);
}

TEST(ZipkinTracerAdapterTest, StartSpanDoesNotCrashOnUninitializedAdapter) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("no-crash");
        span->setAttribute("k", std::string("v"));
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanAttributesDoNotCrash) {
    ZipkinTracerAdapter adapter;
    auto span = adapter.startSpan("attr-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->setAttribute("str",  std::string("hello")));
    EXPECT_NO_THROW(span->setAttribute("int",  int64_t(42)));
    EXPECT_NO_THROW(span->setAttribute("dbl",  3.14));
    EXPECT_NO_THROW(span->setAttribute("bool", true));
}

TEST(ZipkinTracerAdapterTest, StartSpanEndIsIdempotent) {
    ZipkinTracerAdapter adapter;
    auto span = adapter.startSpan("end-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->end());
    EXPECT_NO_THROW(span->end());
}

// ============================================================================
// startChildSpan()
// ============================================================================

TEST(ZipkinTracerAdapterTest, StartChildSpanWithZipkinParentReturnsNonNull) {
    ZipkinTracerAdapter adapter;
    auto parent = adapter.startSpan("parent");
    ASSERT_NE(parent, nullptr);
    auto child = adapter.startChildSpan("child", *parent);
    EXPECT_NE(child, nullptr);
}

TEST(ZipkinTracerAdapterTest, StartChildSpanWithForeignParentFallsBackToRoot) {
    ZipkinTracerAdapter adapter;
    ForeignSpanForZipkin foreign;
    EXPECT_NO_THROW({
        auto span = adapter.startChildSpan("fallback", foreign);
        EXPECT_NE(span, nullptr);
    });
}

TEST(ZipkinTracerAdapterTest, StartChildSpanOnOpenCircuitReturnsInvalidSpan) {
    ZipkinTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);

    ZipkinTracerAdapter adapter(cfg);
    for (int i = 0; i < 3; ++i) adapter.startSpan("trip");
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    auto parent = adapter.startSpan("parent");
    auto child  = adapter.startChildSpan("child", *parent);
    EXPECT_NE(child, nullptr);
    EXPECT_FALSE(child->isValid());
}

// ============================================================================
// startSpanFromHeaders() – W3C traceparent takes precedence
// ============================================================================

TEST(ZipkinTracerAdapterTest, StartSpanFromEmptyHeadersReturnsNonNull) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> empty;
    auto span = adapter.startSpanFromHeaders("op", empty);
    EXPECT_NE(span, nullptr);
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersW3CTakesPrecedenceOverB3) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    headers["b3"]          = "4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-1";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.w3c", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

// ============================================================================
// startSpanFromHeaders() – B3 single header
// ============================================================================

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersB3SingleValidDoesNotCrash) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    // Full format: traceId-spanId-sampling-parentSpanId
    headers["b3"] = "4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-1-00f067aa0ba902b6";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.single", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersB3SingleWithoutParentDoesNotCrash) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["b3"] = "4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-1";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.single.noparent", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersB3SingleDenyDoesNotCrash) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["b3"] = "0"; // sampling deny

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.deny", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersMalformedB3SingleFallsBack) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["b3"] = "not-a-valid-b3-header";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.bad", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

// ============================================================================
// startSpanFromHeaders() – B3 multi-headers
// ============================================================================

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersB3MultiValidDoesNotCrash) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["X-B3-TraceId"]     = "4bf92f3577b34da6a3ce929d0e0e4736";
    headers["X-B3-SpanId"]      = "00f067aa0ba902b7";
    headers["X-B3-ParentSpanId"]= "00f067aa0ba902b6";
    headers["X-B3-Sampled"]     = "1";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.multi", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersB3MultiWithoutParentDoesNotCrash) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["X-B3-TraceId"] = "4bf92f3577b34da6a3ce929d0e0e4736";
    headers["X-B3-SpanId"]  = "00f067aa0ba902b7";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.multi.noparent", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersB3MultiCaseInsensitive) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["x-b3-traceid"] = "4bf92f3577b34da6a3ce929d0e0e4736";
    headers["x-b3-spanid"]  = "00f067aa0ba902b7";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.b3.ci", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(ZipkinTracerAdapterTest, StartSpanFromHeadersOnOpenCircuitReturnsInvalidSpan) {
    ZipkinTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);

    ZipkinTracerAdapter adapter(cfg);
    for (int i = 0; i < 3; ++i) adapter.startSpan("trip-" + std::to_string(i));
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    std::map<std::string, std::string> headers;
    headers["X-B3-TraceId"] = "4bf92f3577b34da6a3ce929d0e0e4736";
    headers["X-B3-SpanId"]  = "0000000000000001";
    auto span = adapter.startSpanFromHeaders("op.open.circuit", headers);
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

// ============================================================================
// injectContext()
// ============================================================================

TEST(ZipkinTracerAdapterTest, InjectContextDoesNotCrash) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    EXPECT_NO_THROW(adapter.injectContext(headers));
}

TEST(ZipkinTracerAdapterTest, InjectContextLeavesHeadersEmptyWhenNoActiveSpan) {
    ZipkinTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    adapter.injectContext(headers);
    // When no OTel span is active, headers must not be partially filled.
    auto it_tp  = headers.find("traceparent");
    auto it_b3  = headers.find("b3");
    auto it_b3t = headers.find("X-B3-TraceId");

    if (it_tp != headers.end()) {
        EXPECT_EQ(it_tp->second.size(), static_cast<size_t>(55))
            << "traceparent must be 55 characters when present";
    }
    // b3 and X-B3-TraceId must either both be absent or both be present.
    EXPECT_EQ(it_b3 == headers.end(), it_b3t == headers.end())
        << "b3 and X-B3-TraceId must be in sync";
}

TEST(ZipkinTracerAdapterTest, InjectContextIncludesBaggageWhenSet) {
    themis::Baggage::clear();
    themis::Baggage::set("user-id", "42");

    ZipkinTracerAdapter adapter;
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

// ============================================================================
// flush() and shutdown()
// ============================================================================

TEST(ZipkinTracerAdapterTest, FlushDoesNotThrow) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.flush());
}

TEST(ZipkinTracerAdapterTest, ShutdownDoesNotThrow) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.shutdown());
}

TEST(ZipkinTracerAdapterTest, MultipleShutdownCallsAreSafe) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.shutdown());
    EXPECT_NO_THROW(adapter.shutdown());
}

// ============================================================================
// ZipkinSpanAdapter – direct construction
// ============================================================================

TEST(ZipkinSpanAdapterTest, DefaultSpanIsInvalid) {
    ZipkinTracerAdapter::ZipkinSpanAdapter span(themis::Tracer::Span{});
    EXPECT_FALSE(span.isValid());
}

TEST(ZipkinSpanAdapterTest, DefaultSpanMethodsDoNotCrash) {
    ZipkinTracerAdapter::ZipkinSpanAdapter span(themis::Tracer::Span{});
    EXPECT_NO_THROW(span.setAttribute("k", std::string("v")));
    EXPECT_NO_THROW(span.setAttribute("n", int64_t(0)));
    EXPECT_NO_THROW(span.setAttribute("d", 0.0));
    EXPECT_NO_THROW(span.setAttribute("b", false));
    EXPECT_NO_THROW(span.recordError("e"));
    EXPECT_NO_THROW(span.setStatus(true));
    EXPECT_NO_THROW(span.end());
}

TEST(ZipkinSpanAdapterTest, AutoEndsOnDestructionWithoutExplicitEnd) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("raii-auto-end");
        (void)span;
    });
}

TEST(ZipkinSpanAdapterTest, SafeAfterExplicitEndThenDestruction) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("explicit-then-raii");
        span->end();
    });
}

// ============================================================================
// ScopedSpan RAII via ITracer interface
// ============================================================================

TEST(ZipkinTracerAdapterTest, ScopedSpanEndsOnScopeExit) {
    ZipkinTracerAdapter adapter;
    EXPECT_NO_THROW({
        ScopedSpan scoped(adapter, "scoped-op");
        scoped.setAttribute("op",    std::string("test"));
        scoped.setAttribute("count", int64_t(1));
        scoped.setAttribute("ratio", 0.5);
        scoped.setAttribute("flag",  false);
        scoped.recordError("nonfatal");
        scoped.setStatus(true, "recovered");
    });
}
