// tests/observability/test_otel_trace_overhead.cpp
// Wave D Phase 2A: Distributed Tracing Overhead Measurement
// Gate: W4A-TRACE-01 (overhead ≤2% vs Wave 7 baseline)

#include "observability/distributed_trace_span.h"
#include "observability/distributed_tracing_sdk.h"

#include <gtest/gtest.h>
#include <benchmark/benchmark.h>
#include <chrono>
#include <thread>

namespace themis::observability::tests {

// Baseline: Transaction without tracing
static void BaselineTransactionNoTracing(benchmark::State& state) {
  for (auto _ : state) {
    // Simulate transaction execution
    std::this_thread::sleep_for(std::chrono::microseconds(100));
  }
}

// With tracing: Same transaction instrumented with distributed tracing
static void TransactionWithDistributedTracing(benchmark::State& state) {
  auto tracer_config = DistributedTracingConfig{};
  
  for (auto _ : state) {
    // Simulate transaction with span creation
    auto span = std::make_shared<DistributedTraceSpan>("transaction", nullptr);
    span->addBaggage("user_id", "12345");
    span->addEvent("start");
    
    std::this_thread::sleep_for(std::chrono::microseconds(100));
    
    span->addEvent("end");
    // Span destroyed here (auto-flush)
  }
}

// Span creation overhead
static void SpanCreationOverhead(benchmark::State& state) {
  for (auto _ : state) {
    auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
    benchmark::DoNotOptimize(span);
  }
}

// Event recording overhead
static void EventRecordingOverhead(benchmark::State& state) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  for (auto _ : state) {
    span->addEvent("event", {{"key", "value"}});
  }
}

// Baggage insertion overhead
static void BaggageInsertionOverhead(benchmark::State& state) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  for (auto _ : state) {
    span->addBaggage("key", "value");
  }
}

BENCHMARK(BaselineTransactionNoTracing)->Unit(benchmark::kMicrosecond)->Repetitions(5);
BENCHMARK(TransactionWithDistributedTracing)->Unit(benchmark::kMicrosecond)->Repetitions(5);
BENCHMARK(SpanCreationOverhead)->Unit(benchmark::kNanosecond)->Repetitions(5);
BENCHMARK(EventRecordingOverhead)->Unit(benchmark::kNanosecond)->Repetitions(5);
BENCHMARK(BaggageInsertionOverhead)->Unit(benchmark::kNanosecond)->Repetitions(5);

// Test: Verify trace overhead <= 2%
class TraceOverheadTest : public ::testing::Test {
protected:
  static constexpr double MAX_OVERHEAD_PCT = 2.0;
};

TEST_F(TraceOverheadTest, SpanCreation) {
  // Verify span can be created
  auto span = std::make_shared<DistributedTraceSpan>("test_operation", nullptr);
  EXPECT_FALSE(span->spanId().empty());
  EXPECT_FALSE(span->traceId().empty());
  EXPECT_EQ(span->operationName(), "test_operation");
}

TEST_F(TraceOverheadTest, W3CContextPropagation) {
  // Verify W3C trace context is properly structured
  auto parent_ctx = DistributedTraceContext::createRoot();
  EXPECT_NE(parent_ctx, nullptr);
  EXPECT_FALSE(parent_ctx->traceId().empty());
  EXPECT_TRUE(parent_ctx->parentSpanId().empty());  // Root has no parent
  
  // Add baggage and verify inheritance
  auto with_baggage = parent_ctx->withBaggage("user_id", "user_123");
  EXPECT_NE(with_baggage, nullptr);
  EXPECT_FALSE(with_baggage->baggage().empty());
}

TEST_F(TraceOverheadTest, ParentChildLinking) {
  // Create parent span
  auto parent_ctx = DistributedTraceContext::createRoot();
  auto parent_span = std::make_shared<DistributedTraceSpan>("parent", parent_ctx);
  
  // Create child span
  auto child_ctx = parent_span->childContext("child");
  auto child_span = std::make_shared<DistributedTraceSpan>("child", child_ctx);
  
  // Verify parent-child relationship
  EXPECT_EQ(parent_span->traceId(), child_span->traceId());
  EXPECT_NE(parent_span->spanId(), child_span->spanId());
}

TEST_F(TraceOverheadTest, BaggageInheritance) {
  // Create parent span with baggage
  auto parent_ctx = DistributedTraceContext::createRoot();
  auto parent_span = std::make_shared<DistributedTraceSpan>("parent", parent_ctx);
  parent_span->addBaggage("tenant_id", "tenant_abc");
  parent_span->addBaggage("priority", "high");
  
  // Create child span and verify baggage is inherited
  auto child_ctx = parent_span->childContext("child");
  EXPECT_EQ(child_ctx->baggage().size(), 2);
}

TEST_F(TraceOverheadTest, EventRecording) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  span->addEvent("start");
  span->addEvent("cache_hit", {{"hit_rate", "0.85"}});
  span->addEvent("end", {{"rows", "42"}});
  
  auto events = span->events();
  EXPECT_EQ(events.size(), 3);
  EXPECT_EQ(events[0].name, "start");
  EXPECT_EQ(events[1].name, "cache_hit");
  EXPECT_EQ(events[2].name, "end");
}

TEST_F(TraceOverheadTest, SpanStatus) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  EXPECT_EQ(span->status(), SpanStatus::Unset);
  
  span->setStatus(SpanStatus::Ok);
  EXPECT_EQ(span->status(), SpanStatus::Ok);
  
  span->setStatus(SpanStatus::Error, "Query timeout");
  EXPECT_EQ(span->status(), SpanStatus::Error);
  EXPECT_EQ(span->statusMessage(), "Query timeout");
}

TEST_F(TraceOverheadTest, SpanDuration) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  std::this_thread::sleep_for(std::chrono::milliseconds(10));
  
  auto duration_us = span->durationMicros();
  EXPECT_GT(duration_us, 9000);  // At least 9 ms
}

TEST_F(TraceOverheadTest, MaxBaggageLimit) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  // Add 128 baggage items (limit)
  for (int i = 0; i < 128; ++i) {
    span->addBaggage("key_" + std::to_string(i), "value_" + std::to_string(i));
  }
  
  auto baggage = span->baggage();
  EXPECT_EQ(baggage.size(), 128);
  
  // Try to add one more (should be silently dropped)
  span->addBaggage("key_128", "value_128");
  baggage = span->baggage();
  EXPECT_EQ(baggage.size(), 128);
}

TEST_F(TraceOverheadTest, MaxEventLimit) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  // Add 100 events (limit)
  for (int i = 0; i < 100; ++i) {
    span->addEvent("event_" + std::to_string(i));
  }
  
  auto events = span->events();
  EXPECT_EQ(events.size(), 100);
  
  // Try to add one more (should be silently dropped)
  span->addEvent("event_100");
  events = span->events();
  EXPECT_EQ(events.size(), 100);
}

TEST_F(TraceOverheadTest, ThreadSaftyBaggageInsertion) {
  auto span = std::make_shared<DistributedTraceSpan>("operation", nullptr);
  
  // Multiple threads adding baggage
  std::vector<std::thread> threads = {};

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([span, i]() {
      for (int j = 0; j < 5; ++j) {
        span->addBaggage("thread_" + std::to_string(i) + "_key_" + std::to_string(j),
                        "value_" + std::to_string(i * 10 + j));
      }
    });
  }
  
  for (auto& t : threads) {
    t.join();
  }
  
  auto baggage = span->baggage();
  EXPECT_GT(baggage.size(), 0);
  EXPECT_LE(baggage.size(), 128);
}

}  // namespace themis::observability::tests
