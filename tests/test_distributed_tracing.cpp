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
        // Reset metrics for clean test state
        MetricsCollector::getInstance().reset();
    }
    
    void TearDown() override {
        // Cleanup after tests
        Tracer::shutdown();
    }
};

/**
 * Test basic span creation and attributes
 */
TEST_F(DistributedTracingTest, BasicSpanCreation) {
    auto span = Tracer::startSpan("test.operation");
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
    const int num_threads = 10;
    const int spans_per_thread = 100;
    
    std::vector<std::thread> threads;
    int64_t initial_total = Tracer::getTotalSpans();
    
    for (int i = 0; i < num_threads; i++) {
        threads.emplace_back([spans_per_thread]() {
            for (int j = 0; j < spans_per_thread; j++) {
                ScopedSpan span("concurrent.operation");
                span.setAttribute("thread_local", j);
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
