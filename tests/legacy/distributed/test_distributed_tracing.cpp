#include <gtest/gtest.h>
#include "utils/tracing.h"
#include "observability/metrics_collector.h"
#include <thread>
#include <chrono>

using namespace themis;
using namespace themis::observability;

/**
 * Test fixture for distributed tracing tests
 */
class DistributedTracingTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Initialize tracer - gracefully handle collector unavailability
        Tracer::initialize("themis-test", "http://localhost:4318");
        auto probe = Tracer::startSpan("distributed_tracing.probe");
        tracing_available_ = probe.isValid();
        probe.end();
        // Reset metrics for clean test state
        MetricsCollector::getInstance().reset();
    }
    
    void TearDown() override {
        // Cleanup after tests
        Tracer::shutdown();
    }

    bool tracing_available_ = false;
};

/**
 * Test basic span creation and attributes
 */
TEST_F(DistributedTracingTest, BasicSpanCreation) {
    auto span = Tracer::startSpan("test.operation");
    if (!span.isValid()) {
        GTEST_SKIP() << "Tracing disabled (OTLP collector not available)";
    }
    EXPECT_TRUE(span.isValid());
    
    span.setAttribute("test.key", "test.value");
    span.setAttribute("test.number", static_cast<int64_t>(42));
    span.setAttribute("test.double", 3.14);
    span.setAttribute("test.bool", true);
    
    span.setStatus(true, "Operation completed successfully");
    span.end();
}

/**
 * Test span lifecycle and RAII
 */
TEST_F(DistributedTracingTest, SpanLifecycle) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    int64_t initial_total = Tracer::getTotalSpans();
    int64_t initial_active = Tracer::getActiveSpans();
    
    {
        ScopedSpan span("test.scoped");
        span.setAttribute("test", "value");
        
        // Active spans should increase
        EXPECT_EQ(Tracer::getActiveSpans(), initial_active + 1);
    }
    // Span should be destroyed here
    
    // Total spans should increase, active should decrease
    EXPECT_EQ(Tracer::getTotalSpans(), initial_total + 1);
    EXPECT_EQ(Tracer::getActiveSpans(), initial_active);
}

/**
 * Test child span creation and context propagation
 */
TEST_F(DistributedTracingTest, ChildSpanCreation) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    auto parent = Tracer::startSpan("parent.operation");
    EXPECT_TRUE(parent.isValid());
    
    auto child = Tracer::startChildSpan("child.operation", parent);
    EXPECT_TRUE(child.isValid());
    
    parent.setAttribute("parent.attr", "parent_value");
    child.setAttribute("child.attr", "child_value");
    
    child.end();
    parent.end();
}

/**
 * Test error recording in spans
 */
TEST_F(DistributedTracingTest, ErrorRecording) {
    auto span = Tracer::startSpan("error.operation");
    
    try {
        // Simulate an error
        throw std::runtime_error("Test error");
    } catch (const std::exception& e) {
        span.recordError(e.what());
        span.setStatus(false, "Operation failed");
    }
    
    span.end();
}

/**
 * Test TracedSpan with automatic metrics recording
 */
TEST_F(DistributedTracingTest, TracedSpanMetrics) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    {
        TracedSpan span("test.traced_operation");
        span.setAttribute("operation.type", "test");
        
        // Simulate work
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        
        span.setStatus(true);
    }
    // Metrics should be recorded on destruction
    
    // Verify metrics were recorded (basic check)
    EXPECT_GT(Tracer::getTotalSpans(), 0);
}

/**
 * Test span duration measurement
 */
TEST_F(DistributedTracingTest, SpanDuration) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    auto span = Tracer::startSpan("duration.test");
    
    // Simulate work
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    double duration = span.durationMs();
    EXPECT_GE(duration, 50.0); // Should be at least 50ms
    EXPECT_LT(duration, 100.0); // But not too much more
    
    span.end();
}

/**
 * Test concurrent span creation
 */
TEST_F(DistributedTracingTest, ConcurrentSpans) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    const int num_threads = 10;
    const int spans_per_thread = 100;
    
    std::vector<std::thread> threads;
    int64_t initial_total = Tracer::getTotalSpans();
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([spans_per_thread]() {
            for (int j = 0; j < spans_per_thread; j++) {
                ScopedSpan span("concurrent.operation");
                span.setAttribute("thread_local", static_cast<double>(j));
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all spans were created
    EXPECT_EQ(Tracer::getTotalSpans(), initial_total + num_threads * spans_per_thread);
}

/**
 * Test span without tracing enabled (no-op mode)
 */
TEST_F(DistributedTracingTest, NoOpSpans) {
    // When tracing is not initialized or disabled, spans should still work as no-ops
    auto span = Tracer::startSpan("noop.operation");
    
    // Should not crash
    span.setAttribute("key", "value");
    span.recordError("error");
    span.setStatus(false);
    span.end();
    
    // In no-op mode, span may or may not be valid depending on compile flags
    // Just verify no crashes occurred
}

/**
 * Test nested span operations
 */
TEST_F(DistributedTracingTest, NestedSpans) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    auto root = Tracer::startSpan("root");
    root.setAttribute("level", static_cast<int64_t>(0));
    
    auto level1 = Tracer::startChildSpan("level1", root);
    level1.setAttribute("level", static_cast<int64_t>(1));
    
    auto level2 = Tracer::startChildSpan("level2", level1);
    level2.setAttribute("level", static_cast<int64_t>(2));
    
    level2.end();
    level1.end();
    root.end();
    
    // Verify spans were created
    EXPECT_GE(Tracer::getTotalSpans(), 3);
}

/**
 * Test metrics collector integration
 */
TEST_F(DistributedTracingTest, MetricsCollectorIntegration) {
    auto& collector = MetricsCollector::getInstance();
    
    // Record some span durations
    collector.recordSpanDuration("operation1", 10.5);
    collector.recordSpanDuration("operation2", 25.3);
    collector.recordActiveSpans(5);
    collector.recordTotalSpans(100);
    
    // Get Prometheus metrics
    std::string metrics = collector.getPrometheusMetrics();
    
    // Verify metrics contain expected data
    EXPECT_NE(metrics.find("trace_span_duration_ms"), std::string::npos);
    EXPECT_NE(metrics.find("trace_active_spans"), std::string::npos);
    EXPECT_NE(metrics.find("trace_total_spans"), std::string::npos);
}

/**
 * Test span move semantics
 */
TEST_F(DistributedTracingTest, SpanMoveSemantics) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    auto span1 = Tracer::startSpan("move.test");
    EXPECT_TRUE(span1.isValid());
    
    // Move span
    auto span2 = std::move(span1);
    EXPECT_TRUE(span2.isValid());
    
    // Original span should be invalid after move
    EXPECT_FALSE(span1.isValid());
    
    span2.end();
}

/**
 * Test multiple span lifecycles
 */
TEST_F(DistributedTracingTest, MultipleSpanLifecycles) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    int64_t initial = Tracer::getTotalSpans();
    
    for (int i = 0; i < 10; i++) {
        ScopedSpan span("loop.operation");
        span.setAttribute("iteration", static_cast<int64_t>(i));
    }
    
    EXPECT_EQ(Tracer::getTotalSpans(), initial + 10);
}

/**
 * Test span attributes with different types
 */
TEST_F(DistributedTracingTest, SpanAttributeTypes) {
    auto span = Tracer::startSpan("attributes.test");
    
    // String attributes
    span.setAttribute("string", "value");
    span.setAttribute("string.empty", "");
    
    // Integer attributes
    span.setAttribute("int.zero", static_cast<int64_t>(0));
    span.setAttribute("int.negative", static_cast<int64_t>(-123));
    span.setAttribute("int.positive", static_cast<int64_t>(456));
    
    // Double attributes
    span.setAttribute("double.zero", 0.0);
    span.setAttribute("double.negative", -3.14);
    span.setAttribute("double.positive", 2.718);
    
    // Boolean attributes
    span.setAttribute("bool.true", true);
    span.setAttribute("bool.false", false);
    
    span.end();
}

// ============================================================================
// W3C TraceContext propagation tests
// ============================================================================

/**
 * Test that a valid traceparent header creates a span (no crash / no-op fallback)
 */
TEST_F(DistributedTracingTest, W3CTraceparentHeaderAccepted) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    std::map<std::string, std::string> headers{
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"}
    };

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test that a missing traceparent header falls back to a regular root span
 */
TEST_F(DistributedTracingTest, W3CNoTraceparentFallsBack) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    std::map<std::string, std::string> headers{{"X-Request-ID", "req-1234"}};

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test that an empty header map falls back to a regular root span
 */
TEST_F(DistributedTracingTest, W3CEmptyHeadersFallsBack) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    std::map<std::string, std::string> headers;

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test that a malformed traceparent is ignored and a root span is still created
 */
TEST_F(DistributedTracingTest, W3CMalformedTraceparentIgnored) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    std::map<std::string, std::string> headers{
        {"traceparent", "not-a-valid-traceparent"}
    };

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test that traceparent and tracestate headers are both accepted
 */
TEST_F(DistributedTracingTest, W3CTracestatePassedThrough) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    std::map<std::string, std::string> headers{
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"},
        {"tracestate",  "vendor1=value1,vendor2=value2"}
    };

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test case-insensitive header lookup (HTTP headers are case-insensitive)
 */
TEST_F(DistributedTracingTest, W3CCaseInsensitiveHeaderLookup) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    // Both upper-case variants should work
    std::map<std::string, std::string> headers_upper{
        {"Traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"}
    };
    std::map<std::string, std::string> headers_mixed{
        {"TRACEPARENT", "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"}
    };

    int64_t before = Tracer::getTotalSpans();
    {
        auto s1 = Tracer::startSpanFromHeaders("req.upper", headers_upper);
        EXPECT_TRUE(s1.isValid());
        s1.end();
    }
    {
        auto s2 = Tracer::startSpanFromHeaders("req.allcaps", headers_mixed);
        EXPECT_TRUE(s2.isValid());
        s2.end();
    }
    EXPECT_EQ(Tracer::getTotalSpans(), before + 2);
}

/**
 * Test that all-zeros trace-id in traceparent is rejected (invalid per W3C spec)
 */
TEST_F(DistributedTracingTest, W3CAllZeroTraceIdRejected) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    // All-zeros trace-id is explicitly invalid per W3C TraceContext specification
    std::map<std::string, std::string> headers{
        {"traceparent", "00-00000000000000000000000000000000-00f067aa0ba902b7-01"}
    };

    int64_t before = Tracer::getTotalSpans();
    // Should fall back to a new root span (not a child of the invalid context)
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test that all-zeros parent-id in traceparent is rejected (invalid per W3C spec)
 */
TEST_F(DistributedTracingTest, W3CAllZeroParentIdRejected) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    // All-zeros parent-id is explicitly invalid per W3C TraceContext specification
    std::map<std::string, std::string> headers{
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01"}
    };

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid());
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

/**
 * Test that a traceparent that is too short is rejected
 */
TEST_F(DistributedTracingTest, W3CShortTraceparentRejected) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    std::map<std::string, std::string> headers{
        // Only 54 chars (one short of required 55)
        {"traceparent", "00-4bf92f3577b34da6a3ce929d0e0e473-00f067aa0ba902b7-01"}
    };

    int64_t before = Tracer::getTotalSpans();
    auto span = Tracer::startSpanFromHeaders("http_request", headers);
    EXPECT_TRUE(span.isValid()); // falls back to root span
    EXPECT_EQ(Tracer::getTotalSpans(), before + 1);
    span.end();
}

// ============================================================================
// Adaptive sampling strategy tests
// ============================================================================

/**
 * Test that adaptive strategy starts with full sampling at low rates
 */
TEST_F(DistributedTracingTest, AdaptiveSamplingFullRateWhenIdle) {
    SamplingStrategy::AdaptiveConfig cfg;
    cfg.max_spans_per_second = 1000.0;
    cfg.min_rate             = 0.01;
    cfg.window               = std::chrono::milliseconds{500};

    auto strategy = SamplingStrategy::adaptive(cfg);
    EXPECT_EQ(strategy.type(), SamplingStrategy::Type::ADAPTIVE);

    // At startup the effective rate should be 1.0 (no spans have been observed yet)
    EXPECT_DOUBLE_EQ(strategy.getEffectiveRate(), 1.0);

    // A handful of calls must all be sampled (rate << max_spans_per_second)
    int sampled = 0;
    for (int i = 0; i < 10; ++i) {
        if (strategy.shouldSample()) {
          ++sampled;
        }
    }
    EXPECT_EQ(sampled, 10);
}

/**
 * Test that adaptive strategy lowers the effective rate when the span rate
 * exceeds max_spans_per_second.
 */
TEST_F(DistributedTracingTest, AdaptiveSamplingReducesRateUnderHighLoad) {
    // Cap: 5 spans/s; 2-second window so we have plenty of time to fill it
    SamplingStrategy::AdaptiveConfig cfg;
    cfg.max_spans_per_second = 5.0;
    cfg.min_rate             = 0.01;
    cfg.window               = std::chrono::milliseconds{2000};

    auto strategy = SamplingStrategy::adaptive(cfg);

    // Record start so we can verify the window hasn't expired during the loop
    auto loop_start = std::chrono::steady_clock::now();

    // Pump 50,000 calls – far more than the 5/s cap within a 2-second window
    for (int i = 0; i < 50000; ++i) {
        strategy.shouldSample();
    }

    auto loop_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - loop_start).count();

    // If the loop itself took longer than the window we cannot make the assertion
    if (loop_elapsed_ms >= 2000) {
        GTEST_SKIP() << "Loop took too long for timing-sensitive test (" 
                     << loop_elapsed_ms << " ms)";
    }

    // Wait for the window to expire so the next call triggers rate adjustment
    std::this_thread::sleep_for(std::chrono::milliseconds{2100});

    // One more call to trigger the window rollover
    strategy.shouldSample();

    // Effective rate must have been reduced below 1.0
    double rate = strategy.getEffectiveRate();
    EXPECT_LT(rate, 1.0);

    // And it must not drop below min_rate
    EXPECT_GE(rate, cfg.min_rate);
}

/**
 * Test that the min_rate floor is respected even under extreme load.
 */
TEST_F(DistributedTracingTest, AdaptiveSamplingRespectsMinRate) {
    SamplingStrategy::AdaptiveConfig cfg;
    cfg.max_spans_per_second = 1.0;   // very low cap
    cfg.min_rate             = 0.05;
    cfg.window               = std::chrono::milliseconds{2000};

    auto strategy = SamplingStrategy::adaptive(cfg);

    auto loop_start = std::chrono::steady_clock::now();
    for (int i = 0; i < 100000; ++i) {
        strategy.shouldSample();
    }

    auto loop_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - loop_start).count();

    if (loop_elapsed_ms >= 2000) {
        GTEST_SKIP() << "Loop took too long for timing-sensitive test ("
                     << loop_elapsed_ms << " ms)";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{2100});
    strategy.shouldSample(); // trigger window rollover

    // Rate must have dropped AND must not go below min_rate
    double rate = strategy.getEffectiveRate();
    EXPECT_LT(rate, 1.0);
    EXPECT_GE(rate, cfg.min_rate);
}

/**
 * Test that getEffectiveRate() returns probability() for non-ADAPTIVE strategies.
 */
TEST_F(DistributedTracingTest, GetEffectiveRateNonAdaptive) {
    EXPECT_DOUBLE_EQ(SamplingStrategy::alwaysOn().getEffectiveRate(),  1.0);
    EXPECT_DOUBLE_EQ(SamplingStrategy::alwaysOff().getEffectiveRate(), 0.0);
    EXPECT_DOUBLE_EQ(SamplingStrategy::probability(0.3).getEffectiveRate(), 0.3);
    EXPECT_DOUBLE_EQ(SamplingStrategy::parentBased(0.5).getEffectiveRate(), 0.5);
}

/**
 * Test that adaptive strategy can be installed on the global Tracer at runtime.
 */
TEST_F(DistributedTracingTest, AdaptiveSamplingSetOnTracer) {
    if (!tracing_available_) { GTEST_SKIP() << "Tracing backend unavailable in this build/runtime"; }
    SamplingStrategy::AdaptiveConfig cfg;
    cfg.max_spans_per_second = 500.0;
    cfg.min_rate             = 0.02;

    Tracer::setSamplingStrategy(SamplingStrategy::adaptive(cfg));

    EXPECT_EQ(Tracer::getSamplingStrategy().type(), SamplingStrategy::Type::ADAPTIVE);

    // Spans can still be created after switching to adaptive mode
    auto span = Tracer::startSpan("adaptive.test");
    EXPECT_TRUE(span.isValid());
    span.end();

    // Restore default strategy
    Tracer::setSamplingStrategy(SamplingStrategy::alwaysOn());
}

/**
 * Test that copies of an adaptive strategy share the same rate-measurement state.
 */
TEST_F(DistributedTracingTest, AdaptiveSamplingCopiesShareState) {
    SamplingStrategy::AdaptiveConfig cfg;
    cfg.max_spans_per_second = 5.0;
    cfg.min_rate             = 0.01;
    cfg.window               = std::chrono::milliseconds{2000};

    auto s1 = SamplingStrategy::adaptive(cfg);
    auto s2 = s1; // copy – must share state

    // Drive the rate up via s1
    auto loop_start = std::chrono::steady_clock::now();
    for (int i = 0; i < 50000; ++i) {
      s1.shouldSample();
    }

    auto loop_elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - loop_start).count();

    if (loop_elapsed_ms >= 2000) {
        GTEST_SKIP() << "Loop took too long for timing-sensitive test ("
                     << loop_elapsed_ms << " ms)";
    }

    std::this_thread::sleep_for(std::chrono::milliseconds{2100});
    s1.shouldSample(); // rollover

    // s2 should see the same effective rate
    EXPECT_DOUBLE_EQ(s1.getEffectiveRate(), s2.getEffectiveRate());
}

