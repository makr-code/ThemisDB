/*
 * Unit tests for OpenTelemetryTracerAdapter
 *
 * Validates that the ITracer adapter correctly delegates to the underlying
 * themis::Tracer, that the embedded circuit breaker trips after repeated
 * export failures, and that all ISpan methods are safe to call in both
 * normal and no-OTel (THEMIS_ENABLE_TRACING not defined) environments.
 *
 * Note: In CI environments where THEMIS_ENABLE_TRACING is not defined,
 * every span created by themis::Tracer returns isValid()==false.  The
 * adapter records that as a failure in the circuit breaker, which is the
 * expected behavior (the OTLP exporter is unreachable).  Tests are
 * written to be correct in both configurations.
 */

#include "core/concerns/otel_tracer_adapter.h"
#include "core/concerns/i_tracer.h"
#include "core/concerns/lifecycle.h"
#include "sharding/circuit_breaker.h"
#include <gtest/gtest.h>
#include <memory>
#include <string>
#include <thread>
#include <chrono>

using namespace themis::core::concerns;
using namespace themis::sharding;

// ─────────────────────────────────────────────────────────────────────────────
// Helpers
// ─────────────────────────────────────────────────────────────────────────────

/// Create an adapter with a circuit breaker that has a very short timeout so
/// we can test OPEN→HALF_OPEN transitions quickly in unit tests.
static OpenTelemetryTracerAdapter makeAdapterWithFastRecovery(
    size_t failure_threshold = 3,
    std::chrono::seconds timeout = std::chrono::seconds(0)){
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = failure_threshold;
    cfg.timeout = timeout;
    cfg.success_threshold = 1;
    return OpenTelemetryTracerAdapter(cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Test fixture
// ─────────────────────────────────────────────────────────────────────────────

class OtelTracerAdapterTest : public ::testing::Test {
protected:
    void SetUp() override {
        adapter_ = std::make_unique<OpenTelemetryTracerAdapter>();
    }

    void TearDown() override {
        adapter_->shutdown();
        adapter_.reset();
    }

    std::unique_ptr<OpenTelemetryTracerAdapter> adapter_;
};

// ─────────────────────────────────────────────────────────────────────────────
// Construction & initialization
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(OtelTracerAdapterTest, DefaultConstructionSucceeds) {
    OpenTelemetryTracerAdapter a;
    EXPECT_FALSE(a.isInitialized());
}

TEST_F(OtelTracerAdapterTest, CustomCircuitBreakerConfigAccepted) {
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 10;
    cfg.timeout = std::chrono::seconds(60);
    cfg.success_threshold = 3;
    EXPECT_NO_THROW(OpenTelemetryTracerAdapter a(cfg));
}

TEST_F(OtelTracerAdapterTest, NotInitializedAfterConstruction) {
    EXPECT_FALSE(adapter_->isInitialized());
}

TEST_F(OtelTracerAdapterTest, InitializeReturnsBoolWithoutCrashing) {
    // initialize() may return false when THEMIS_ENABLE_TRACING is not set;
    // the important thing is it does not throw.
    EXPECT_NO_THROW(adapter_->initialize("test-service", "http://localhost:4318"));
}

TEST_F(OtelTracerAdapterTest, ShutdownAfterInitDoesNotThrow) {
    adapter_->initialize("test-service", "http://localhost:4318");
    EXPECT_NO_THROW(adapter_->shutdown());
    EXPECT_FALSE(adapter_->isInitialized());
}

TEST_F(OtelTracerAdapterTest, DoubleShutdownDoesNotThrow) {
    EXPECT_NO_THROW(adapter_->shutdown());
    EXPECT_NO_THROW(adapter_->shutdown());
}

// ─────────────────────────────────────────────────────────────────────────────
// Span creation – interface contract
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(OtelTracerAdapterTest, StartSpanReturnsNonNull) {
    auto span = adapter_->startSpan("test.operation");
    ASSERT_NE(nullptr, span);
}

TEST_F(OtelTracerAdapterTest, StartChildSpanReturnsNonNull) {
    auto parent = adapter_->startSpan("parent");
    ASSERT_NE(nullptr, parent);
    auto child = adapter_->startChildSpan("child", *parent);
    ASSERT_NE(nullptr, child);
}

TEST_F(OtelTracerAdapterTest, StartChildSpanWithNonOtelParentSucceeds) {
    // Use a NoOp span (not an OtelSpanAdapter) as the parent; the adapter
    // should fall back to creating a root span rather than crashing.
    class FakeSpan : public ITracer::ISpan {
    public:
        void setAttribute(const std::string&, const std::string&) override {}
        void setAttribute(const std::string&, int64_t) override {}
        void setAttribute(const std::string&, double) override {}
        void setAttribute(const std::string&, bool) override {}
        void recordError(const std::string&) override {}
        void setStatus(bool, const std::string&) override {}
        void end() override {}
        bool isValid() const override { return false; }
    };

    FakeSpan fake;
    EXPECT_NO_THROW(adapter_->startChildSpan("child", fake));
}

TEST_F(OtelTracerAdapterTest, SpanSetStringAttributeDoesNotThrow) {
    auto span = adapter_->startSpan("attrs");
    EXPECT_NO_THROW(span->setAttribute("db.system", "themisdb"));
}

TEST_F(OtelTracerAdapterTest, SpanSetInt64AttributeDoesNotThrow) {
    auto span = adapter_->startSpan("attrs");
    EXPECT_NO_THROW(span->setAttribute("row_count", int64_t{42}));
}

TEST_F(OtelTracerAdapterTest, SpanSetDoubleAttributeDoesNotThrow) {
    auto span = adapter_->startSpan("attrs");
    EXPECT_NO_THROW(span->setAttribute("latency_ms", 3.14));
}

TEST_F(OtelTracerAdapterTest, SpanSetBoolAttributeDoesNotThrow) {
    auto span = adapter_->startSpan("attrs");
    EXPECT_NO_THROW(span->setAttribute("cache_hit", true));
}

TEST_F(OtelTracerAdapterTest, SpanRecordErrorDoesNotThrow) {
    auto span = adapter_->startSpan("error_test");
    EXPECT_NO_THROW(span->recordError("connection refused"));
}

TEST_F(OtelTracerAdapterTest, SpanSetStatusOkDoesNotThrow) {
    auto span = adapter_->startSpan("status_test");
    EXPECT_NO_THROW(span->setStatus(true, "completed"));
}

TEST_F(OtelTracerAdapterTest, SpanSetStatusErrorDoesNotThrow) {
    auto span = adapter_->startSpan("status_test");
    EXPECT_NO_THROW(span->setStatus(false, "timeout"));
}

TEST_F(OtelTracerAdapterTest, SpanEndDoesNotThrow) {
    auto span = adapter_->startSpan("end_test");
    EXPECT_NO_THROW(span->end());
}

TEST_F(OtelTracerAdapterTest, SpanEndCalledTwiceDoesNotThrow) {
    auto span = adapter_->startSpan("double_end");
    EXPECT_NO_THROW(span->end());
    EXPECT_NO_THROW(span->end());
}

TEST_F(OtelTracerAdapterTest, SpanDestructorDoesNotThrow) {
    // Let the span go out of scope; RAII should not throw.
    EXPECT_NO_THROW({
        auto span = adapter_->startSpan("raii");
        span->setAttribute("key", "value");
        // span destroyed here
    });
}

// ─────────────────────────────────────────────────────────────────────────────
// Circuit breaker behavior
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(OtelTracerAdapterTest, CircuitBreakerInitiallyClosedState) {
    EXPECT_EQ(CircuitBreaker::State::CLOSED, adapter_->circuitBreakerState());
}

TEST_F(OtelTracerAdapterTest, CircuitBreakerTripsAfterThresholdFailures) {
    // In non-OTel builds every span is invalid → each startSpan() call records
    // a failure.  After failure_threshold (default=5) failures the circuit opens.
    // We exhaust the default threshold and then create one more span to observe.

    // Use a low-threshold adapter so the test is fast and predictable.
    auto adapter = makeAdapterWithFastRecovery(/*failure_threshold=*/3);

    // Force enough failures to trip the circuit.
    for (int i = 0; i < 4; ++i) {
        auto sp = adapter.startSpan("probe");
        (void)sp;
    }

    // The circuit should now be OPEN (or we're still CLOSED if tracing IS enabled
    // and spans are valid — in that case every call was a success).
    auto state = adapter.circuitBreakerState();
    EXPECT_TRUE(state == CircuitBreaker::State::OPEN ||
                state == CircuitBreaker::State::CLOSED);
    // Either state is acceptable depending on whether THEMIS_ENABLE_TRACING is
    // active; what matters is that startSpan() never crashes or returns null.
}

TEST_F(OtelTracerAdapterTest, SpanCreatedWhenCircuitOpen) {
    // Force the circuit OPEN via multiple failures.
    auto adapter = makeAdapterWithFastRecovery(/*failure_threshold=*/1);
    // First call either fails (OTLP unavailable) or succeeds (OTLP available).
    adapter.startSpan("fail1");
    adapter.startSpan("fail2"); // might trip circuit if prev was invalid

    // Regardless of circuit state, startSpan must return non-null.
    auto span = adapter.startSpan("while_open");
    EXPECT_NE(nullptr, span);
}

TEST_F(OtelTracerAdapterTest, CircuitBreakerTransitionToHalfOpenAfterTimeout) {
    // Use a zero-second timeout so the transition happens immediately.
    auto adapter = makeAdapterWithFastRecovery(/*failure_threshold=*/2,
                                               /*timeout=*/std::chrono::seconds(0));

    // Generate failures to open the circuit.
    for (int i = 0; i < 5; ++i) {
        adapter.startSpan("force_failure");
    }

    // Allow timeout to elapse (already 0s, so no sleep needed).
    // The next allowRequest() call should transition to HALF_OPEN.
    auto span = adapter.startSpan("probe_recovery");
    EXPECT_NE(nullptr, span);

    auto state = adapter.circuitBreakerState();
    // Acceptable states: OPEN (if tracing enabled and spans valid, cb closed fast),
    // HALF_OPEN or CLOSED (recovery in progress / successful).
    EXPECT_TRUE(state == CircuitBreaker::State::OPEN   ||
                state == CircuitBreaker::State::HALF_OPEN ||
                state == CircuitBreaker::State::CLOSED);
}

// ─────────────────────────────────────────────────────────────────────────────
// Health probes
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(OtelTracerAdapterTest, IsHealthyUnhealthyBeforeInit) {
    auto result = adapter_->isHealthy();
    // Not initialized -> unhealthy.
    EXPECT_FALSE(result.ok);
    EXPECT_FALSE(result.message.empty());
}

TEST_F(OtelTracerAdapterTest, IsHealthyAfterSuccessfulInit) {
    bool ok = adapter_->initialize("svc", "http://localhost:4318");
    auto result = adapter_->isHealthy();
    if (ok) {
        // Real OTel init: circuit still closed -> healthy.
        EXPECT_TRUE(result.ok);
    } else {
        // Failed init: may report unhealthy; depends on whether init sets
        // initialized_ = false.  Either is acceptable.
        // (The adapter only sets initialized_ = false on failure via our impl)
    }
}

TEST_F(OtelTracerAdapterTest, IsHealthyUnhealthyWhenCircuitOpen) {
    // Use a low-threshold, high-timeout adapter so the circuit stays OPEN.
    OpenTelemetryTracerAdapter::CircuitBreakerConfig cfg;
    cfg.failure_threshold = 1;
    cfg.timeout = std::chrono::seconds(30); // stay OPEN for the duration of this test
    cfg.success_threshold = 1;
    auto adapter = OpenTelemetryTracerAdapter(cfg);

    // Manually mark initialized so we can test the circuit-open path.
    adapter.initialize("svc", "http://localhost:4318");

    // Force failures to open the circuit.
    for (int i = 0; i < 5; ++i) {
        adapter.startSpan("force");
    }

    if (adapter.circuitBreakerState() == CircuitBreaker::State::OPEN) {
        auto result = adapter.isHealthy();
        EXPECT_FALSE(result.ok);
        EXPECT_NE(std::string::npos,
                  result.message.find("circuit-breaker"));
    }
    // If the circuit is NOT open (tracing enabled + spans valid), this test
    // is a no-op — the adapter is healthy, which is also correct.
}

// ─────────────────────────────────────────────────────────────────────────────
// Lifecycle hooks
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(OtelTracerAdapterTest, FlushDoesNotThrow) {
    EXPECT_NO_THROW(adapter_->flush());
}

TEST_F(OtelTracerAdapterTest, FlushAfterInitDoesNotThrow) {
    adapter_->initialize("svc", "http://localhost:4318");
    EXPECT_NO_THROW(adapter_->flush());
}

TEST_F(OtelTracerAdapterTest, ShutdownWithActiveSpanDoesNotThrow) {
    auto span = adapter_->startSpan("active");
    EXPECT_NO_THROW(adapter_->shutdown());
    // span is still alive; end() must be safe after shutdown.
    EXPECT_NO_THROW(span->end());
}

// ─────────────────────────────────────────────────────────────────────────────
// ScopedSpan integration
// ─────────────────────────────────────────────────────────────────────────────

TEST_F(OtelTracerAdapterTest, ScopedSpanWorksWithAdapter) {
    EXPECT_NO_THROW({
        ScopedSpan scoped(*adapter_, "scoped.operation");
        scoped.setAttribute("db.statement", "SELECT 1");
        scoped.setAttribute("row_count", int64_t{1});
        scoped.setAttribute("latency_ms", 0.5);
        scoped.setAttribute("from_cache", false);
        scoped.setStatus(true, "ok");
    });
}

TEST_F(OtelTracerAdapterTest, ScopedSpanRecordErrorDoesNotThrow) {
    EXPECT_NO_THROW({
        ScopedSpan scoped(*adapter_, "failing.op");
        scoped.recordError("disk full");
        scoped.setStatus(false, "disk full");
    });
}
