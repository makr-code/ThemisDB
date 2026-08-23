/**
 * @file test_distributed_tracing_sdk.cpp
 * @brief Focused regression tests for distributed tracing SDK (Phase 2, DTI-01..10).
 *
 * Test coverage:
 * - DTI-01: W3C Trace Context extraction and propagation
 * - DTI-02: Jaeger Baggage header parsing and serialization
 * - DTI-03: B3 Single header support
 * - DTI-04: B3 Multi header support
 * - DTI-05: Baggage item management and limits
 * - DTI-06: Child context creation from parent context
 * - DTI-07: Trace context validation
 * - DTI-08: Orphan span recovery (missing parent context)
 * - DTI-09: Concurrent context propagation
 * - DTI-10: Edge cases (empty headers, malformed context)
 */

#include "gtest/gtest.h"
#include "observability/distributed_tracing_sdk.h"
#include <mutex>
#include <thread>
#include <vector>
#include <map>

namespace themis {
namespace observability {

class DistributedTracingSDKTest : public ::testing::Test {
protected:
    DistributedTracingSDK sdk_;
};

// DTI-01: W3C Trace Context extraction and propagation
TEST_F(DistributedTracingSDKTest, W3CTraceContextExtraction) {
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    headers["tracestate"] = "congo=t61rcWZlbiBhIGZhbmN5IGZvciBjaGFu";

    TraceContextFormat fmt = TraceContextFormat::W3C_TRACE_CONTEXT;
    auto ctx = sdk_.extractContextFromHeaders(headers, &fmt);

    ASSERT_TRUE(ctx);
    EXPECT_EQ(ctx->traceId(), "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(ctx->parentSpanId(), "00f067aa0ba902b7");
    EXPECT_EQ(ctx->traceState(), "congo=t61rcWZlbiBhIGZhbmN5IGZvciBjaGFu");
    EXPECT_TRUE(ctx->isTraceSampled());
}

TEST_F(DistributedTracingSDKTest, W3CTraceContextPropagation) {
    // createRoot() already sets trace_sampled_ = true internally.
    auto ctx = DistributedTraceContext::createRoot();

    auto headers = ctx->toHttpHeaders(TraceContextFormat::W3C_TRACE_CONTEXT);

    ASSERT_TRUE(headers.count("traceparent"));
    EXPECT_EQ(headers["traceparent"].substr(0, 3), "00-");
    EXPECT_EQ(headers["traceparent"].length(), 55);  // 00-<32 hex>-<16 hex>-<2 hex>
}

// DTI-02: Jaeger Baggage header parsing and serialization
TEST_F(DistributedTracingSDKTest, JaegerBaggageExtraction) {
    std::map<std::string, std::string> headers;
    headers["uber-trace-id"] = "4bf92f3577b34da6a3ce929d0e0e4736:00f067aa0ba902b7:0:1";
    headers["jaeger-baggage"] = "tenant=acme,request_priority=high";

    TraceContextFormat fmt = TraceContextFormat::JAEGER_BAGGAGE;
    auto ctx = sdk_.extractContextFromHeaders(headers, &fmt);

    ASSERT_TRUE(ctx);
    EXPECT_EQ(ctx->traceId(), "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(ctx->parentSpanId(), "00f067aa0ba902b7");

    auto baggage = ctx->baggage();
    EXPECT_EQ(baggage.size(), 2);
    EXPECT_EQ(baggage[0].key, "tenant");
    EXPECT_EQ(baggage[0].value, "acme");
    EXPECT_TRUE(baggage[0].inherited);
}

// DTI-03: B3 Single header support
TEST_F(DistributedTracingSDKTest, B3SingleHeaderExtraction) {
    std::map<std::string, std::string> headers;
    headers["b3"] = "4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-1";

    TraceContextFormat fmt_b3s = TraceContextFormat::B3_SINGLE;
    auto ctx = sdk_.extractContextFromHeaders(headers, &fmt_b3s);

    ASSERT_TRUE(ctx);
    EXPECT_EQ(ctx->traceId(), "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(ctx->parentSpanId(), "00f067aa0ba902b7");
}

// DTI-04: B3 Multi header support
TEST_F(DistributedTracingSDKTest, B3MultiHeaderExtraction) {
    std::map<std::string, std::string> headers;
    headers["x-b3-traceid"] = "4bf92f3577b34da6a3ce929d0e0e4736";
    headers["x-b3-spanid"] = "00f067aa0ba902b7";
    headers["x-b3-sampled"] = "1";

    TraceContextFormat fmt_b3m = TraceContextFormat::B3_MULTI;
    auto ctx = sdk_.extractContextFromHeaders(headers, &fmt_b3m);

    ASSERT_TRUE(ctx);
    EXPECT_EQ(ctx->traceId(), "4bf92f3577b34da6a3ce929d0e0e4736");
    EXPECT_EQ(ctx->parentSpanId(), "00f067aa0ba902b7");
    EXPECT_TRUE(ctx->isTraceSampled());
}

// DTI-05: Baggage item management and limits
TEST_F(DistributedTracingSDKTest, BaggageItemManagement) {
    auto ctx = DistributedTraceContext::createRoot();

    // Add baggage items up to limit
    for (int i = 0; i < 128; ++i) {
        ctx = ctx->withBaggage("key" + std::to_string(i), "value" + std::to_string(i));
    }

    EXPECT_EQ(ctx->baggage().size(), 128);
}

TEST_F(DistributedTracingSDKTest, BaggageOverflowHandling) {
    auto ctx = DistributedTraceContext::createRoot();
    const auto max_baggage_items = sdk_.configuration().max_baggage_items;

    // Add baggage items beyond limit
    for (std::size_t i = 0; i < max_baggage_items + 22; ++i) {
        ctx = ctx->withBaggage("key" + std::to_string(i), "value" + std::to_string(i));
    }

    // Should not exceed limit
    EXPECT_LE(ctx->baggage().size(), max_baggage_items);
}

// DTI-06: Child context creation from parent context
TEST_F(DistributedTracingSDKTest, ChildContextCreation) {
    auto parent = DistributedTraceContext::createRoot();
    std::string original_trace_id = parent->traceId();

    auto child = sdk_.createChildContext(parent, "");

    ASSERT_TRUE(child);
    EXPECT_EQ(child->traceId(), original_trace_id);  // Same trace ID
    EXPECT_FALSE(child->parentSpanId().empty());  // Child gets a new span ID
    EXPECT_NE(child->parentSpanId(), parent->parentSpanId());
}

TEST_F(DistributedTracingSDKTest, ChildContextInheritsBaggage) {
    auto parent = DistributedTraceContext::createRoot();
    parent = parent->withBaggage("tenant", "acme");

    auto cfg = DistributedTracingConfig();
    cfg.inherit_baggage = true;
    DistributedTracingSDK sdk_with_baggage(cfg);

    auto child = sdk_with_baggage.createChildContext(parent);

    auto baggage = child->baggage();
    EXPECT_EQ(baggage.size(), 1);
    EXPECT_EQ(baggage[0].key, "tenant");
    EXPECT_TRUE(baggage[0].inherited);
}

// DTI-07: Trace context validation
TEST_F(DistributedTracingSDKTest, TraceContextValidation) {
    auto ctx = DistributedTraceContext::createRoot();

    auto result = sdk_.validateTraceContext(ctx);

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.error_code, 0);
}

TEST_F(DistributedTracingSDKTest, InvalidTraceContextValidation) {
    auto result = sdk_.validateTraceContext(nullptr);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code,
              10);
}

// DTI-08: Orphan span recovery (missing parent context)
TEST_F(DistributedTracingSDKTest, OrphanSpanRecovery) {
    std::map<std::string, std::string> empty_headers;

    // When no headers present, SDK should create a new root context
    TraceContextFormat fmt_w3c = TraceContextFormat::W3C_TRACE_CONTEXT;
    auto ctx = sdk_.extractContextFromHeaders(empty_headers, &fmt_w3c);

    ASSERT_TRUE(ctx);
    EXPECT_FALSE(ctx->traceId().empty());
    EXPECT_TRUE(ctx->parentSpanId().empty());  // Root context
}

// DTI-09: Concurrent context propagation
TEST_F(DistributedTracingSDKTest, ConcurrentContextPropagation) {
    std::vector<std::thread> threads;
    std::vector<std::shared_ptr<DistributedTraceContext>> contexts;
    std::mutex contexts_mutex;

    // Create and propagate contexts concurrently
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back([this, i, &contexts_mutex, &contexts]() {
            auto ctx = DistributedTraceContext::createRoot();
            ctx = ctx->withBaggage("thread_id", std::to_string(i));

            auto headers = ctx->toHttpHeaders(TraceContextFormat::W3C_TRACE_CONTEXT);
            TraceContextFormat fmt_w3c = TraceContextFormat::W3C_TRACE_CONTEXT;
            auto extracted = sdk_.extractContextFromHeaders(headers, &fmt_w3c);

            {
                std::lock_guard<std::mutex> lock(contexts_mutex);
                contexts.push_back(extracted);
            }
        });
    }

    for (auto& t : threads) {
        t.join();
    }

    EXPECT_EQ(contexts.size(), 10);
}

// DTI-10: Edge cases (empty headers, malformed context)
TEST_F(DistributedTracingSDKTest, EmptyHeadersHandling) {
    std::map<std::string, std::string> empty_headers;

    TraceContextFormat fmt_w3c = TraceContextFormat::W3C_TRACE_CONTEXT;
    auto ctx = sdk_.extractContextFromHeaders(empty_headers, &fmt_w3c);

    // Should create new root context
    ASSERT_TRUE(ctx);
    EXPECT_FALSE(ctx->traceId().empty());
}

TEST_F(DistributedTracingSDKTest, MalformedTraceparentHandling) {
    std::map<std::string, std::string> headers;
    headers["traceparent"] = "invalid-format-here";

    TraceContextFormat fmt_w3c = TraceContextFormat::W3C_TRACE_CONTEXT;
    auto ctx = sdk_.extractContextFromHeaders(headers, &fmt_w3c);

    // Should fall back to new root context
    ASSERT_TRUE(ctx);
    // If new context was created, trace_id should not match the invalid header
    EXPECT_NE(ctx->traceId(), "invalid");
}

TEST_F(DistributedTracingSDKTest, MalformedTraceIdValidation) {
    struct TestTraceContext final : DistributedTraceContext {
        void setTraceId(const std::string& value) { trace_id_ = value; }
        void setParentSpanId(const std::string& value) { parent_span_id_ = value; }
    };

    auto ctx = std::make_shared<TestTraceContext>();
    ctx->setTraceId("not-hex-at-all");
    ctx->setParentSpanId("0000000000000001");

    auto result = sdk_.validateTraceContext(ctx);

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_code,
              10);
}

} // namespace observability
} // namespace themis
