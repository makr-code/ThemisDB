/*
 * Unit tests for ContextPropagation — context propagation across async boundaries.
 *
 * Covers:
 *   - ContextScope  : installs/restores current context on the calling thread
 *   - ContextPropagation::current() : returns the active context or nullptr
 *   - ContextPropagation::propagate() : flows context into a new async task
 *   - Nested scopes : inner scope shadows outer; outer restored on exit
 *   - Null context  : propagate() with no active context passes nullptr
 *   - Child isolation: attributes set inside async task are not visible in parent
 */

#include "core/concerns/context_propagation.h"
#include "core/concerns/i_context.h"
#include <gtest/gtest.h>
#include <atomic>
#include <string>
#include <thread>

using namespace themis::core::concerns;

// ---------------------------------------------------------------------------
// ContextPropagation::current() — no scope installed
// ---------------------------------------------------------------------------

TEST(ContextPropagationTest, CurrentIsNullWithNoScope) {
    // Without any ContextScope the thread-local should be nullptr.
    EXPECT_EQ(nullptr, ContextPropagation::current());
}

// ---------------------------------------------------------------------------
// ContextScope — basic install / restore
// ---------------------------------------------------------------------------

TEST(ContextScopeTest, InstallsContext) {
    auto ctx = SimpleContext::create("trace-1", "req-1");
    ASSERT_NE(nullptr, ctx);
    {
        ContextScope scope(ctx);
        EXPECT_EQ(ctx, ContextPropagation::current());
    }
    // Scope exited — should be restored to nullptr.
    EXPECT_EQ(nullptr, ContextPropagation::current());
}

TEST(ContextScopeTest, RestoresPreviousContextOnExit) {
    auto outer = SimpleContext::create("trace-outer", "req-outer");
    auto inner = SimpleContext::create("trace-inner", "req-inner");

    ContextScope outerScope(outer);
    EXPECT_EQ(outer, ContextPropagation::current());

    {
        ContextScope innerScope(inner);
        EXPECT_EQ(inner, ContextPropagation::current());
    }

    // Inner scope gone — outer should be current again.
    EXPECT_EQ(outer, ContextPropagation::current());
}

TEST(ContextScopeTest, NullContextScope) {
    // Installing nullptr should work and report nullptr.
    ContextScope scope(nullptr);
    EXPECT_EQ(nullptr, ContextPropagation::current());
}

// ---------------------------------------------------------------------------
// ContextScope — nested scopes
// ---------------------------------------------------------------------------

TEST(ContextScopeTest, ThreeLevelNesting) {
    auto a = SimpleContext::create("a", "");
    auto b = SimpleContext::create("b", "");
    auto c = SimpleContext::create("c", "");

    EXPECT_EQ(nullptr, ContextPropagation::current());

    ContextScope s_a(a);
    EXPECT_EQ(a, ContextPropagation::current());
    {
        ContextScope s_b(b);
        EXPECT_EQ(b, ContextPropagation::current());
        {
            ContextScope s_c(c);
            EXPECT_EQ(c, ContextPropagation::current());
        }
        EXPECT_EQ(b, ContextPropagation::current());
    }
    EXPECT_EQ(a, ContextPropagation::current());
}

// ---------------------------------------------------------------------------
// ContextPropagation::propagate() — context flows to async task
// ---------------------------------------------------------------------------

TEST(ContextPropagationTest, PropagateFlowsContextToAsyncTask) {
    auto ctx = SimpleContext::create("trace-async", "req-async");
    ASSERT_NE(nullptr, ctx);
    ctx->set(context_keys::kService, "test-service");

    ContextScope scope(ctx);

    auto fut = ContextPropagation::propagate([]() -> std::string {
        auto c = ContextPropagation::current();
        if (!c) return "";
        // Verify both trace_id and kService are propagated.
        auto trace = c->get(context_keys::kTraceId).value_or("missing-trace");
        auto svc   = c->get(context_keys::kService).value_or("missing-svc");
        return trace + "|" + svc;
    });

    EXPECT_EQ("trace-async|test-service", fut.get());
}

TEST(ContextPropagationTest, PropagateCreatesChildContext) {
    // The propagated context is a *child* of the current context so that
    // writes inside the task do not affect the parent.
    auto parent = SimpleContext::create("parent-trace", "parent-req");
    parent->set(context_keys::kService, "svc");

    ContextScope scope(parent);

    auto fut = ContextPropagation::propagate([]() -> bool {
        auto child = ContextPropagation::current();
        if (!child) return false;

        // Parent attributes must be visible via the child.
        auto trace = child->get(context_keys::kTraceId);
        auto svc   = child->get(context_keys::kService);
        return trace.value_or("") == "parent-trace" &&
               svc.value_or("")   == "svc";
    });

    EXPECT_TRUE(fut.get());
}

TEST(ContextPropagationTest, ChildWritesDoNotAffectParent) {
    auto parent = SimpleContext::create("t", "r");
    ContextScope scope(parent);

    auto fut = ContextPropagation::propagate([]() {
        auto child = ContextPropagation::current();
        if (child) {
            child->set(context_keys::kOperation, "db.query");
        }
    });
    fut.get();

    // Parent must not have the child-only attribute.
    EXPECT_FALSE(parent->has(context_keys::kOperation));
}

TEST(ContextPropagationTest, PropagateWithNoCurrentContextPassesNullToTask) {
    // When there is no active scope, propagate() should pass nullptr.
    ASSERT_EQ(nullptr, ContextPropagation::current());

    auto fut = ContextPropagation::propagate([]() -> bool {
        return ContextPropagation::current() == nullptr;
    });

    EXPECT_TRUE(fut.get());
}

// ---------------------------------------------------------------------------
// ContextPropagation::propagate() — return value forwarding
// ---------------------------------------------------------------------------

TEST(ContextPropagationTest, PropagateForwardsReturnValue) {
    auto ctx = SimpleContext::create("t", "r");
    ContextScope scope(ctx);

    auto fut = ContextPropagation::propagate([]() -> int { return 42; });
    EXPECT_EQ(42, fut.get());
}

TEST(ContextPropagationTest, PropagateVoidReturnDoesNotThrow) {
    auto ctx = SimpleContext::create("t", "r");
    ContextScope scope(ctx);

    auto fut = ContextPropagation::propagate([]() {});
    EXPECT_NO_THROW(fut.get());
}

// ---------------------------------------------------------------------------
// Multi-thread isolation
// ---------------------------------------------------------------------------

TEST(ContextPropagationTest, EachThreadHasItsOwnCurrentContext) {
    // Two threads install different contexts concurrently; each should see
    // only its own context.
    auto ctx1 = SimpleContext::create("thread1-trace", "thread1-req");
    auto ctx2 = SimpleContext::create("thread2-trace", "thread2-req");

    std::atomic<bool> t1_ok{false};
    std::atomic<bool> t2_ok{false};

    std::thread t1([&]() {
        ContextScope scope(ctx1);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto c = ContextPropagation::current();
        t1_ok = c && c->get(context_keys::kTraceId).value_or("") == "thread1-trace";
    });

    std::thread t2([&]() {
        ContextScope scope(ctx2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        auto c = ContextPropagation::current();
        t2_ok = c && c->get(context_keys::kTraceId).value_or("") == "thread2-trace";
    });

    t1.join();
    t2.join();

    EXPECT_TRUE(t1_ok.load());
    EXPECT_TRUE(t2_ok.load());
    // Main thread's context is still nullptr.
    EXPECT_EQ(nullptr, ContextPropagation::current());
}

// ---------------------------------------------------------------------------
// ContextScope — exception safety
// ---------------------------------------------------------------------------

TEST(ContextScopeTest, RestoredAfterException) {
    auto ctx = SimpleContext::create("t", "r");

    EXPECT_EQ(nullptr, ContextPropagation::current());
    try {
        ContextScope scope(ctx);
        EXPECT_EQ(ctx, ContextPropagation::current());
        throw std::runtime_error("test");
    } catch (...) {
        // Scope destructed by stack unwinding.
    }

    // Context must be restored to nullptr despite the exception.
    EXPECT_EQ(nullptr, ContextPropagation::current());
}

// ---------------------------------------------------------------------------
// W3C TraceContext — kSpanId key and toTraceContext()
// ---------------------------------------------------------------------------

TEST(W3CTraceContextTest, SpanIdKeyStoresAndRetrieves) {
    auto ctx = SimpleContext::create("abc123", "req-1");
    ctx->set(context_keys::kSpanId, "def456");

    EXPECT_EQ("def456", ctx->get(context_keys::kSpanId).value_or(""));
}

TEST(W3CTraceContextTest, ToTraceContextIncludesSpanId) {
    auto ctx = SimpleContext::create("trace-id-val", "req-id-val");
    ctx->set(context_keys::kSpanId, "span-id-val");

    const auto tc = ctx->toTraceContext();

    EXPECT_EQ("trace-id-val", tc.trace_id);
    EXPECT_EQ("span-id-val",  tc.span_id);
    EXPECT_EQ("req-id-val",   tc.request_id);
}

TEST(W3CTraceContextTest, ToTraceContextSpanIdEmptyWhenNotSet) {
    auto ctx = SimpleContext::create("t", "r");
    // kSpanId is never set.
    EXPECT_TRUE(ctx->toTraceContext().span_id.empty());
}

TEST(W3CTraceContextTest, SpanIdInheritedByChild) {
    auto parent = SimpleContext::create("trace-abc", "req-42");
    parent->set(context_keys::kSpanId, "span-ff00");

    auto child = parent->createChild();
    EXPECT_EQ("span-ff00", child->get(context_keys::kSpanId).value_or(""));
    EXPECT_EQ("span-ff00", child->toTraceContext().span_id);
}

// ---------------------------------------------------------------------------
// W3C TraceContext — formatTraceparent()
// ---------------------------------------------------------------------------

TEST(W3CTraceContextTest, FormatTraceparentProducesCorrectHeader) {
    auto ctx = SimpleContext::create("4bf92f3577b34da6a3ce929d0e0e4736", "req-1");
    ctx->set(context_keys::kSpanId, "00f067aa0ba902b7");

    const auto tp = w3c_trace_context::formatTraceparent(*ctx);

    EXPECT_EQ("00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01", tp);
}

TEST(W3CTraceContextTest, FormatTraceparentEmptyWhenTraceIdMissing) {
    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kSpanId, "00f067aa0ba902b7");
    // kTraceId not set → empty result.
    EXPECT_TRUE(w3c_trace_context::formatTraceparent(*ctx).empty());
}

TEST(W3CTraceContextTest, FormatTraceparentEmptyWhenSpanIdMissing) {
    auto ctx = SimpleContext::create("4bf92f3577b34da6a3ce929d0e0e4736", "");
    // kSpanId not set → empty result.
    EXPECT_TRUE(w3c_trace_context::formatTraceparent(*ctx).empty());
}

// ---------------------------------------------------------------------------
// W3C TraceContext — parseTraceparent()
// ---------------------------------------------------------------------------

TEST(W3CTraceContextTest, ParseValidTraceparent) {
    const std::string tp = "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01";
    auto ctx = w3c_trace_context::parseTraceparent(tp);

    ASSERT_NE(nullptr, ctx);
    EXPECT_EQ("4bf92f3577b34da6a3ce929d0e0e4736",
              ctx->get(context_keys::kTraceId).value_or(""));
    EXPECT_EQ("00f067aa0ba902b7",
              ctx->get(context_keys::kSpanId).value_or(""));
}

TEST(W3CTraceContextTest, ParseTraceparentRejectsTooShort) {
    EXPECT_EQ(nullptr, w3c_trace_context::parseTraceparent("00-abc-def-01"));
}

TEST(W3CTraceContextTest, ParseTraceparentRejectsAllZeroTraceId) {
    const std::string tp = "00-00000000000000000000000000000000-00f067aa0ba902b7-01";
    EXPECT_EQ(nullptr, w3c_trace_context::parseTraceparent(tp));
}

TEST(W3CTraceContextTest, ParseTraceparentRejectsAllZeroSpanId) {
    const std::string tp = "00-4bf92f3577b34da6a3ce929d0e0e4736-0000000000000000-01";
    EXPECT_EQ(nullptr, w3c_trace_context::parseTraceparent(tp));
}

TEST(W3CTraceContextTest, ParseTraceparentRejectsWrongTraceIdLength) {
    // trace-id has 31 chars instead of 32.
    const std::string tp = "00-4bf92f3577b34da6a3ce929d0e047-00f067aa0ba902b7-01";
    EXPECT_EQ(nullptr, w3c_trace_context::parseTraceparent(tp));
}

TEST(W3CTraceContextTest, ParseTraceparentRejectsWrongSpanIdLength) {
    // parent-id has 15 chars instead of 16.
    const std::string tp = "00-4bf92f3577b34da6a3ce929d0e0e4736-0f067aa0ba902b7-01";
    EXPECT_EQ(nullptr, w3c_trace_context::parseTraceparent(tp));
}

TEST(W3CTraceContextTest, ParseTraceparentRejectsEmptyString) {
    EXPECT_EQ(nullptr, w3c_trace_context::parseTraceparent(""));
}

// ---------------------------------------------------------------------------
// W3C TraceContext — round-trip encode/decode
// ---------------------------------------------------------------------------

TEST(W3CTraceContextTest, RoundTripFormatParse) {
    const std::string trace_id = "4bf92f3577b34da6a3ce929d0e0e4736";
    const std::string span_id  = "00f067aa0ba902b7";

    auto ctx = SimpleContext::create(trace_id, "");
    ctx->set(context_keys::kSpanId, span_id);

    const auto header = w3c_trace_context::formatTraceparent(*ctx);
    ASSERT_FALSE(header.empty());

    const auto parsed = w3c_trace_context::parseTraceparent(header);
    ASSERT_NE(nullptr, parsed);

    EXPECT_EQ(trace_id, parsed->get(context_keys::kTraceId).value_or(""));
    EXPECT_EQ(span_id,  parsed->get(context_keys::kSpanId).value_or(""));
}

// ---------------------------------------------------------------------------
// W3C TraceContext — propagation through async boundary
// ---------------------------------------------------------------------------

TEST(W3CTraceContextTest, TraceparentPropagatesAcrossAsyncBoundary) {
    // Install a context with W3C trace/span IDs.
    const std::string trace_id = "1234567890abcdef1234567890abcdef";
    const std::string span_id  = "fedcba0987654321";

    auto ctx = SimpleContext::create(trace_id, "req-99");
    ctx->set(context_keys::kSpanId, span_id);

    ContextScope scope(ctx);

    // Spawn async task — the child context inherits trace_id and span_id.
    auto fut = ContextPropagation::propagate([]() -> std::string {
        auto c = ContextPropagation::current();
        if (!c) return "";
        return w3c_trace_context::formatTraceparent(*c);
    });

    const auto result = fut.get();
    EXPECT_EQ("00-" + trace_id + "-" + span_id + "-01", result);
}

TEST(W3CTraceContextTest, ParsedContextInstalledViaScope) {
    // Simulate an inbound HTTP handler that receives a traceparent header.
    const std::string header =
        "00-aabbccddeeff00112233445566778899-0102030405060708-01";

    auto incoming = w3c_trace_context::parseTraceparent(header);
    ASSERT_NE(nullptr, incoming);

    ContextScope scope(incoming);

    EXPECT_EQ("aabbccddeeff00112233445566778899",
              ContextPropagation::current()->get(context_keys::kTraceId).value_or(""));
    EXPECT_EQ("0102030405060708",
              ContextPropagation::current()->get(context_keys::kSpanId).value_or(""));
}
