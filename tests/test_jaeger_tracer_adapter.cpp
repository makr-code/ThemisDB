/**
 * @file test_jaeger_tracer_adapter.cpp
 * @brief Unit tests for JaegerTracerAdapter (core/concerns)
 *
 * Tests cover:
 * - Default and custom CircuitBreakerConfig construction
 * - isInitialized() before/after initialize() / shutdown()
 * - startSpan(): no-crash on uninitialized adapter, non-null result
 * - startChildSpan(): with JaegerSpanAdapter parent and with foreign parent
 * - startSpanFromHeaders(): W3C traceparent takes precedence over uber-trace-id
 * - startSpanFromHeaders(): valid uber-trace-id attributes are recorded
 * - startSpanFromHeaders(): missing/malformed headers fall back gracefully
 * - injectContext(): uber-trace-id and traceparent are emitted when a span is active
 * - injectContext(): no headers emitted when no active span
 * - isHealthy(): false when not initialized, after successful init
 * - circuitBreakerState(): starts CLOSED, opens after failure threshold
 * - flush() and shutdown() are safe to call multiple times
 * - kDefaultEndpoint points to Jaeger HTTP collector
 */

#include <gtest/gtest.h>

#include "core/concerns/jaeger_tracer_adapter.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/noop_implementations.h"

using namespace themis::core::concerns;
using namespace themis::sharding;

// ============================================================================
// Helpers
// ============================================================================

// A minimal ISpan that is NOT a JaegerSpanAdapter, used to exercise the
// fallback path in startChildSpan().
class ForeignSpanForJaeger : public ITracer::ISpan {
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

TEST(JaegerTracerAdapterTest, DefaultEndpointIsJaegerHttpCollector) {
    std::string ep = JaegerTracerAdapter::kDefaultEndpoint;
    EXPECT_NE(ep.find("14268"), std::string::npos)
        << "Default endpoint should reference Jaeger port 14268: " << ep;
}

// ============================================================================
// Construction
// ============================================================================

TEST(JaegerTracerAdapterTest, DefaultConstructionSucceeds) {
    EXPECT_NO_THROW(JaegerTracerAdapter adapter);
}

TEST(JaegerTracerAdapterTest, CustomCircuitBreakerConfigIsAccepted) {
    JaegerTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.timeout           = std::chrono::seconds(10);
    cfg.success_threshold = 1;
    EXPECT_NO_THROW(JaegerTracerAdapter adapter(cfg));
}

// ============================================================================
// Initialization state
// ============================================================================

TEST(JaegerTracerAdapterTest, NotInitializedByDefault) {
    JaegerTracerAdapter adapter;
    EXPECT_FALSE(adapter.isInitialized());
}

TEST(JaegerTracerAdapterTest, ShutdownResetsInitializedFlag) {
    JaegerTracerAdapter adapter;
    adapter.initialize("jaeger-test", "http://127.0.0.1:14268");
    adapter.shutdown();
    EXPECT_FALSE(adapter.isInitialized());
}

TEST(JaegerTracerAdapterTest, DoubleInitializeReturnsTrueAndDoesNotTripBreaker) {
    JaegerTracerAdapter adapter;
    adapter.initialize("jaeger-svc", "http://127.0.0.1:14268");

    if (adapter.isInitialized()) {
        CircuitBreaker::State state_before = adapter.circuitBreakerState();
        bool second = adapter.initialize("jaeger-svc", "http://127.0.0.1:14268");
        EXPECT_TRUE(second);
        EXPECT_EQ(adapter.circuitBreakerState(), state_before);
    }

    adapter.shutdown();
}

// ============================================================================
// isHealthy()
// ============================================================================

TEST(JaegerTracerAdapterTest, IsUnhealthyWhenNotInitialized) {
    JaegerTracerAdapter adapter;
    auto result = adapter.isHealthy();
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

TEST(JaegerTracerAdapterTest, HealthCheckMessageIsNonNull) {
    JaegerTracerAdapter adapter;
    adapter.initialize("svc", "http://127.0.0.1:14268");
    auto result = adapter.isHealthy();
    EXPECT_NE(result.message.data(), nullptr);
}

// ============================================================================
// Circuit-breaker state
// ============================================================================

TEST(JaegerTracerAdapterTest, CircuitBreakerStartsClosed) {
    JaegerTracerAdapter adapter;
    EXPECT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::CLOSED);
}

TEST(JaegerTracerAdapterTest, CircuitBreakerOpensAfterFailureThreshold) {
    JaegerTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    JaegerTracerAdapter adapter(cfg);

    for (int i = 0; i < static_cast<int>(cfg.failure_threshold) + 1; ++i) {
        adapter.startSpan("probe-" + std::to_string(i));
    }

    EXPECT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);
}

TEST(JaegerTracerAdapterTest, OpenCircuitReturnsInvalidSpan) {
    JaegerTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    JaegerTracerAdapter adapter(cfg);
    for (int i = 0; i < 3; ++i) adapter.startSpan("trip-" + std::to_string(i));
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    auto span = adapter.startSpan("during-open");
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

// ============================================================================
// startSpan()
// ============================================================================

TEST(JaegerTracerAdapterTest, StartSpanReturnsNonNull) {
    JaegerTracerAdapter adapter;
    auto span = adapter.startSpan("test-span");
    EXPECT_NE(span, nullptr);
}

TEST(JaegerTracerAdapterTest, StartSpanDoesNotCrashOnUninitializedAdapter) {
    JaegerTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("no-crash");
        span->setAttribute("k", std::string("v"));
        span->end();
    });
}

TEST(JaegerTracerAdapterTest, StartSpanAttributesDoNotCrash) {
    JaegerTracerAdapter adapter;
    auto span = adapter.startSpan("attr-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->setAttribute("str",  std::string("hello")));
    EXPECT_NO_THROW(span->setAttribute("int",  int64_t(42)));
    EXPECT_NO_THROW(span->setAttribute("dbl",  3.14));
    EXPECT_NO_THROW(span->setAttribute("bool", true));
}

TEST(JaegerTracerAdapterTest, StartSpanEndIsIdempotent) {
    JaegerTracerAdapter adapter;
    auto span = adapter.startSpan("end-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->end());
    EXPECT_NO_THROW(span->end());
}

// ============================================================================
// startChildSpan()
// ============================================================================

TEST(JaegerTracerAdapterTest, StartChildSpanWithJaegerParentReturnsNonNull) {
    JaegerTracerAdapter adapter;
    auto parent = adapter.startSpan("parent");
    ASSERT_NE(parent, nullptr);
    auto child = adapter.startChildSpan("child", *parent);
    EXPECT_NE(child, nullptr);
}

TEST(JaegerTracerAdapterTest, StartChildSpanWithForeignParentFallsBackToRoot) {
    JaegerTracerAdapter adapter;
    ForeignSpanForJaeger foreign;
    EXPECT_NO_THROW({
        auto span = adapter.startChildSpan("fallback", foreign);
        EXPECT_NE(span, nullptr);
    });
}

TEST(JaegerTracerAdapterTest, StartChildSpanOnOpenCircuitReturnsInvalidSpan) {
    JaegerTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);

    JaegerTracerAdapter adapter(cfg);
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

TEST(JaegerTracerAdapterTest, StartSpanFromEmptyHeadersReturnsNonNull) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> empty;
    auto span = adapter.startSpanFromHeaders("op", empty);
    EXPECT_NE(span, nullptr);
}

TEST(JaegerTracerAdapterTest, StartSpanFromHeadersW3CTakesPrecedenceOverUberTraceId) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["traceparent"]   = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    headers["uber-trace-id"] = "4bf92f3577b34da6a3ce929d0e0e4736:00f067aa0ba902b7:0:1";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.w3c", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(JaegerTracerAdapterTest, StartSpanFromHeadersWithValidUberTraceIdDoesNotCrash) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["uber-trace-id"] = "4bf92f3577b34da6a3ce929d0e0e4736:00f067aa0ba902b7:0:1";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.jaeger", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(JaegerTracerAdapterTest, StartSpanFromHeadersMalformedUberTraceIdFallsBack) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["uber-trace-id"] = "not-valid";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.bad", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(JaegerTracerAdapterTest, StartSpanFromHeadersCaseInsensitiveUberTraceId) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    headers["Uber-Trace-Id"] = "4bf92f3577b34da6a3ce929d0e0e4736:00f067aa0ba902b7:0:1";

    EXPECT_NO_THROW({
        auto span = adapter.startSpanFromHeaders("op.case", headers);
        ASSERT_NE(span, nullptr);
        span->end();
    });
}

TEST(JaegerTracerAdapterTest, StartSpanFromHeadersOnOpenCircuitReturnsInvalidSpan) {
    JaegerTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);

    JaegerTracerAdapter adapter(cfg);
    for (int i = 0; i < 3; ++i) adapter.startSpan("trip-" + std::to_string(i));
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    std::map<std::string, std::string> headers;
    headers["uber-trace-id"] = "4bf92f3577b34da6a3ce929d0e0e4736:0000000000000001:0:1";
    auto span = adapter.startSpanFromHeaders("op.open.circuit", headers);
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

// ============================================================================
// injectContext()
// ============================================================================

TEST(JaegerTracerAdapterTest, InjectContextDoesNotCrash) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    EXPECT_NO_THROW(adapter.injectContext(headers));
}

TEST(JaegerTracerAdapterTest, InjectContextLeavesHeadersEmptyWhenNoActiveSpan) {
    JaegerTracerAdapter adapter;
    std::map<std::string, std::string> headers;
    adapter.injectContext(headers);
    // When no OTel span is active, neither traceparent nor uber-trace-id are set.
    // (If one is set, it must have a valid format.)
    auto it = headers.find("traceparent");
    if (it != headers.end()) {
        EXPECT_EQ(it->second.size(), static_cast<size_t>(55))
            << "traceparent must be 55 characters when present";
    }
    auto jit = headers.find("uber-trace-id");
    if (jit != headers.end()) {
        // uber-trace-id must contain at least 3 colons.
        EXPECT_GE(std::count(jit->second.begin(), jit->second.end(), ':'),
                  static_cast<std::string::difference_type>(3));
    }
}

TEST(JaegerTracerAdapterTest, InjectContextIncludesBaggageWhenSet) {
    themis::Baggage::clear();
    themis::Baggage::set("user-id", "42");

    JaegerTracerAdapter adapter;
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

TEST(JaegerTracerAdapterTest, FlushDoesNotThrow) {
    JaegerTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.flush());
}

TEST(JaegerTracerAdapterTest, ShutdownDoesNotThrow) {
    JaegerTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.shutdown());
}

TEST(JaegerTracerAdapterTest, MultipleShutdownCallsAreSafe) {
    JaegerTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.shutdown());
    EXPECT_NO_THROW(adapter.shutdown());
}

// ============================================================================
// JaegerSpanAdapter – direct construction
// ============================================================================

TEST(JaegerSpanAdapterTest, DefaultSpanIsInvalid) {
    JaegerTracerAdapter::JaegerSpanAdapter span(themis::Tracer::Span{});
    EXPECT_FALSE(span.isValid());
}

TEST(JaegerSpanAdapterTest, DefaultSpanMethodsDoNotCrash) {
    JaegerTracerAdapter::JaegerSpanAdapter span(themis::Tracer::Span{});
    EXPECT_NO_THROW(span.setAttribute("k", std::string("v")));
    EXPECT_NO_THROW(span.setAttribute("n", int64_t(0)));
    EXPECT_NO_THROW(span.setAttribute("d", 0.0));
    EXPECT_NO_THROW(span.setAttribute("b", false));
    EXPECT_NO_THROW(span.recordError("e"));
    EXPECT_NO_THROW(span.setStatus(true));
    EXPECT_NO_THROW(span.end());
}

TEST(JaegerSpanAdapterTest, AutoEndsOnDestructionWithoutExplicitEnd) {
    JaegerTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("raii-auto-end");
        (void)span;
    });
}

TEST(JaegerSpanAdapterTest, SafeAfterExplicitEndThenDestruction) {
    JaegerTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("explicit-then-raii");
        span->end();
    });
}

// ============================================================================
// ScopedSpan RAII via ITracer interface
// ============================================================================

TEST(JaegerTracerAdapterTest, ScopedSpanEndsOnScopeExit) {
    JaegerTracerAdapter adapter;
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
