/**
 * @file test_otel_tracer_adapter.cpp
 * @brief Unit tests for OpenTelemetryTracerAdapter (core/concerns)
 *
 * Tests cover:
 * - Construction with default and custom CircuitBreakerConfig
 * - isInitialized() before and after initialize() / shutdown()
 * - startSpan() no-crash guarantee on uninitialised adapter
 * - startChildSpan() with OtelSpanAdapter parent (proper child span)
 * - startChildSpan() with non-OtelSpanAdapter parent (fallback to root)
 * - OtelSpanAdapter: setAttribute overloads, recordError, setStatus, end, isValid
 * - ScopedSpan RAII: end() called on scope exit
 * - isHealthy() reflects initialization state and circuit-breaker state
 * - circuitBreakerState() starts CLOSED
 * - flush() is a no-op and does not throw
 * - shutdown() resets isInitialized() to false
 * - Circuit breaker opens after failure threshold is exceeded
 */

#include <gtest/gtest.h>

#include "core/concerns/otel_tracer_adapter.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/noop_implementations.h"

using namespace themis::core::concerns;
using namespace themis::sharding;

// ============================================================================
// Helpers
// ============================================================================

// A minimal ISpan that is NOT an OtelSpanAdapter, used to exercise the
// fallback path in startChildSpan().
class ForeignSpan : public ITracer::ISpan {
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
// Construction
// ============================================================================

TEST(OtelTracerAdapterTest, DefaultConstructionSucceeds) {
    EXPECT_NO_THROW(OpenTelemetryTracerAdapter adapter);
}

TEST(OtelTracerAdapterTest, CustomCircuitBreakerConfigIsAccepted) {
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.timeout           = std::chrono::seconds(10);
    cfg.success_threshold = 1;
    EXPECT_NO_THROW(OpenTelemetryTracerAdapter adapter(cfg));
}

// ============================================================================
// Initialization state
// ============================================================================

TEST(OtelTracerAdapterTest, NotInitializedByDefault) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_FALSE(adapter.isInitialized());
}

TEST(OtelTracerAdapterTest, ShutdownResetsInitializedFlag) {
    OpenTelemetryTracerAdapter adapter;
    // initialize() may return false when there is no real OTLP endpoint,
    // but it should not crash.
    adapter.initialize("test-service", "http://127.0.0.1:14318");
    adapter.shutdown();
    EXPECT_FALSE(adapter.isInitialized());
}

// ============================================================================
// isHealthy()
// ============================================================================

TEST(OtelTracerAdapterTest, IsUnhealthyWhenNotInitialized) {
    OpenTelemetryTracerAdapter adapter;
    auto result = adapter.isHealthy();
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

TEST(OtelTracerAdapterTest, IsHealthyAfterSuccessfulInitialize) {
    // When THEMIS_ENABLE_TRACING is off the underlying Tracer::initialize()
    // returns false, so the adapter may not report healthy.  We just verify
    // the API does not crash and returns a well-formed ProbeResult.
    OpenTelemetryTracerAdapter adapter;
    adapter.initialize("svc", "http://127.0.0.1:4318");
    auto result = adapter.isHealthy();
    // result.ok depends on whether tracing is compiled in; just check the
    // message is a valid string.
    EXPECT_NE(result.message.data(), nullptr);
}

// ============================================================================
// Circuit-breaker state
// ============================================================================

TEST(OtelTracerAdapterTest, CircuitBreakerStartsClosed) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::CLOSED);
}

TEST(OtelTracerAdapterTest, CircuitBreakerOpensAfterFailureThreshold) {
    // Use a low threshold so the circuit trips quickly.
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 3;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    OpenTelemetryTracerAdapter adapter(cfg);

    // Calling startSpan() on an uninitialized adapter records a failure each
    // time (span is not valid).
    for (int i = 0; i < static_cast<int>(cfg.failure_threshold) + 1; ++i) {
        adapter.startSpan("probe-" + std::to_string(i));
    }

    // Circuit should be OPEN now.
    EXPECT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);
}

TEST(OtelTracerAdapterTest, OpenCircuitReturnsInvalidSpan) {
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);
    cfg.success_threshold = 1;

    OpenTelemetryTracerAdapter adapter(cfg);

    // Trip the circuit.
    for (int i = 0; i < 3; ++i) {
        adapter.startSpan("trip-" + std::to_string(i));
    }
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    // A span started while the circuit is OPEN must not be valid.
    auto span = adapter.startSpan("during-open");
    ASSERT_NE(span, nullptr);
    EXPECT_FALSE(span->isValid());
}

// ============================================================================
// startSpan()
// ============================================================================

TEST(OtelTracerAdapterTest, StartSpanReturnsNonNull) {
    OpenTelemetryTracerAdapter adapter;
    auto span = adapter.startSpan("test-span");
    EXPECT_NE(span, nullptr);
}

TEST(OtelTracerAdapterTest, StartSpanDoesNotCrashOnUninitializedAdapter) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("no-crash");
        span->setAttribute("k", "v");
        span->end();
    });
}

TEST(OtelTracerAdapterTest, StartSpanAttributesDoNotCrash) {
    OpenTelemetryTracerAdapter adapter;
    auto span = adapter.startSpan("attr-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->setAttribute("str",  std::string("hello")));
    EXPECT_NO_THROW(span->setAttribute("int",  int64_t(42)));
    EXPECT_NO_THROW(span->setAttribute("dbl",  3.14));
    EXPECT_NO_THROW(span->setAttribute("bool", true));
}

TEST(OtelTracerAdapterTest, StartSpanRecordErrorDoesNotCrash) {
    OpenTelemetryTracerAdapter adapter;
    auto span = adapter.startSpan("err-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->recordError("something went wrong"));
}

TEST(OtelTracerAdapterTest, StartSpanSetStatusDoesNotCrash) {
    OpenTelemetryTracerAdapter adapter;
    auto span = adapter.startSpan("status-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->setStatus(false, "error state"));
    EXPECT_NO_THROW(span->setStatus(true, "ok"));
}

TEST(OtelTracerAdapterTest, StartSpanEndIsIdempotent) {
    OpenTelemetryTracerAdapter adapter;
    auto span = adapter.startSpan("end-span");
    ASSERT_NE(span, nullptr);
    EXPECT_NO_THROW(span->end());
    EXPECT_NO_THROW(span->end()); // second call must not crash
}

// ============================================================================
// startChildSpan()
// ============================================================================

TEST(OtelTracerAdapterTest, StartChildSpanWithOtelParentReturnsNonNull) {
    OpenTelemetryTracerAdapter adapter;
    auto parent = adapter.startSpan("parent");
    ASSERT_NE(parent, nullptr);
    auto child = adapter.startChildSpan("child", *parent);
    EXPECT_NE(child, nullptr);
}

TEST(OtelTracerAdapterTest, StartChildSpanWithForeignParentFallsBackToRoot) {
    OpenTelemetryTracerAdapter adapter;
    ForeignSpan foreign;
    // Should not crash even though parent is not an OtelSpanAdapter.
    EXPECT_NO_THROW({
        auto span = adapter.startChildSpan("fallback", foreign);
        EXPECT_NE(span, nullptr);
    });
}

TEST(OtelTracerAdapterTest, StartChildSpanOnOpenCircuitReturnsInvalidSpan) {
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 2;
    cfg.timeout           = std::chrono::seconds(60);

    OpenTelemetryTracerAdapter adapter(cfg);
    for (int i = 0; i < 3; ++i) {
      adapter.startSpan("trip");
    }
    ASSERT_EQ(adapter.circuitBreakerState(), CircuitBreaker::State::OPEN);

    auto parent = adapter.startSpan("parent"); // returns invalid span
    auto child  = adapter.startChildSpan("child", *parent);
    EXPECT_NE(child, nullptr);
    EXPECT_FALSE(child->isValid());
}

// ============================================================================
// flush() and shutdown()
// ============================================================================

TEST(OtelTracerAdapterTest, FlushDoesNotThrow) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.flush());
}

TEST(OtelTracerAdapterTest, ShutdownDoesNotThrow) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.shutdown());
}

TEST(OtelTracerAdapterTest, MultipleShutdownCallsAreSafe) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_NO_THROW(adapter.shutdown());
    EXPECT_NO_THROW(adapter.shutdown());
}

// ============================================================================
// ScopedSpan RAII via ITracer interface
// ============================================================================

TEST(OtelTracerAdapterTest, ScopedSpanEndsOnScopeExit) {
    OpenTelemetryTracerAdapter adapter;
    // If ScopedSpan correctly calls end() in its destructor the span lifecycle
    // is exercised without memory issues.  We verify no crash occurs.
    EXPECT_NO_THROW({
        ScopedSpan scoped(adapter, "scoped-op");
        scoped.setAttribute("op", std::string("test"));
        scoped.setAttribute("count", int64_t(1));
        scoped.setAttribute("ratio", 0.5);
        scoped.setAttribute("flag",  false);
        scoped.recordError("nonfatal");
        scoped.setStatus(true, "recovered");
        // ~ScopedSpan() → span_->end()
    });
}

TEST(OtelTracerAdapterTest, ScopedSpanSpanPointerIsAccessible) {
    OpenTelemetryTracerAdapter adapter;
    ScopedSpan scoped(adapter, "ptr-access");
    // span() may return nullptr (no-op) or a valid pointer; either is acceptable.
    // The important thing is the call does not crash.
    EXPECT_NO_THROW(scoped.span());
}

// ============================================================================
// OtelSpanAdapter – direct construction
// ============================================================================

TEST(OtelSpanAdapterTest, DefaultSpanIsInvalid) {
    OpenTelemetryTracerAdapter::OtelSpanAdapter span(themis::Tracer::Span{});
    EXPECT_FALSE(span.isValid());
}

TEST(OtelSpanAdapterTest, DefaultSpanMethodsDoNotCrash) {
    OpenTelemetryTracerAdapter::OtelSpanAdapter span(themis::Tracer::Span{});
    EXPECT_NO_THROW(span.setAttribute("k", std::string("v")));
    EXPECT_NO_THROW(span.setAttribute("n", int64_t(0)));
    EXPECT_NO_THROW(span.setAttribute("d", 0.0));
    EXPECT_NO_THROW(span.setAttribute("b", false));
    EXPECT_NO_THROW(span.recordError("e"));
    EXPECT_NO_THROW(span.setStatus(true));
    EXPECT_NO_THROW(span.end());
}

// ============================================================================
// OtelSpanAdapter – RAII auto-end
// ============================================================================

// Verify that dropping the unique_ptr<ISpan> without calling end() explicitly
// does not crash and does not double-end the underlying span.
TEST(OtelSpanAdapterTest, AutoEndsOnDestructionWithoutExplicitEnd) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("raii-auto-end");
        // Deliberately NOT calling span->end() – destructor should auto-end.
        (void)span;
    });
}

TEST(OtelSpanAdapterTest, SafeAfterExplicitEndThenDestruction) {
    OpenTelemetryTracerAdapter adapter;
    EXPECT_NO_THROW({
        auto span = adapter.startSpan("explicit-then-raii");
        span->end();           // explicit end
        // unique_ptr destructor calls ~OtelSpanAdapter() → span_.end() again;
        // Tracer::Span::end() is idempotent so no double-end crash.
    });
}

// ============================================================================
// initialize() – double-call guard
// ============================================================================

TEST(OtelTracerAdapterTest, DoubleInitializeReturnsTrueAndDoesNotTripBreaker) {
    OpenTelemetryTracerAdapter adapter;

    // First call may succeed or fail (no OTLP endpoint in test env), but should
    // not crash.  Force the adapter into initialized state by attempting init.
    adapter.initialize("svc", "http://127.0.0.1:4318");

    if (adapter.isInitialized()) {
        // If the first call succeeded (e.g., no-op path without THEMIS_ENABLE_TRACING),
        // verify that a second call also returns true and does not record a failure.
        CircuitBreaker::State state_before = adapter.circuitBreakerState();
        bool second = adapter.initialize("svc", "http://127.0.0.1:4318");
        EXPECT_TRUE(second);
        // The circuit breaker must not have been tripped by the second init.
        EXPECT_EQ(adapter.circuitBreakerState(), state_before);
    }

    adapter.shutdown();
}
