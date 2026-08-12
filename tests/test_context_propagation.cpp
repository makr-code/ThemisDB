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
