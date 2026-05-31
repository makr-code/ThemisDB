/*
 * ThemisDB | File: context_propagation.h | Version: 0.0.15 | Last Modified: 2026-05-20 19:53:17
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 201
 * Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2842 [core] Implement W3C TraceC... (2026-03-12) | #2678 feat(core): context propaga... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "core/concerns/i_context.h"
#include <future>
#include <memory>
#include <utility>

namespace themis {
namespace core {
namespace concerns {

// Forward declaration so ContextPropagation can declare ContextScope as friend.
class ContextScope;

/**
 * @brief Thread-local current-context storage for async boundary propagation.
 *
 * `ContextPropagation` enables automatic context propagation across async
 * and thread boundaries by maintaining a thread-local "current context"
 * pointer.  This removes the need to pass `IContextPtr` through every
 * function signature on hot paths.
 *
 * ### Typical usage
 *
 * #### Setting a context for the current request handler
 * @code
 *   // At the entry point of a request (HTTP handler, gRPC stub …)
 *   auto ctx = SimpleContext::create("trace-abc", "req-42");
 *   ContextScope scope(ctx);   // installs ctx as the current thread context
 *
 *   // Any code running on this thread can now call:
 *   auto current = ContextPropagation::current();  // returns ctx
 * @endcode
 *
 * #### Propagating context to a new thread / async task
 * @code
 *   // Inside the request handler (ctx is the current context):
 *   auto fut = ContextPropagation::propagate([]() {
 *       // Runs on a NEW thread, but current() still returns the parent ctx.
 *       auto ctx = ContextPropagation::current();
 *       ctx->get(context_keys::kTraceId);  // "trace-abc"
 *   });
 *   fut.get();
 * @endcode
 *
 * ### Thread safety
 *
 * The thread-local storage itself is inherently thread-safe (each thread
 * has its own slot).  `ContextScope` is not copyable or movable; it must
 * be used within a single thread's stack frame.
 */
class ContextPropagation {
public:
    /**
     * @brief Return the context installed for the calling thread.
     *
     * If no context has been installed via `ContextScope` the function
     * returns `nullptr`.
     *
     * @return The current `IContextPtr` or `nullptr`.
     */
    static IContextPtr current() noexcept {
        return current_;
    }

    /**
     * @brief Wrap a callable so the current thread's context is propagated
     *        into the new thread before the callable is invoked.
     *
     * Captures the current context by value (shared ownership) and
     * installs a child context via a `ContextScope` on the worker thread
     * before calling @p fn.  The child inherits all parent attributes and
     * may add or shadow keys without mutating the caller's context.
     *
     * @tparam Fn   Callable type (no-argument, any return type).
     * @param  fn   The work to run on a new thread.
     * @return A `std::future<>` that becomes ready when @p fn completes.
     *         Exceptions thrown by @p fn are propagated through the future.
     *
     * @code
     *   ContextScope scope(ctx);
     *   auto fut = ContextPropagation::propagate([] {
     *       auto c = ContextPropagation::current();
     *       // c is a child of `ctx` with all parent attributes visible
     *   });
     *   fut.get();
     * @endcode
     */
    template <typename Fn>
    static auto propagate(Fn&& fn) -> std::future<std::invoke_result_t<Fn>>;

    // Non-instantiable static utility class.
    ContextPropagation() = delete;

private:
    friend class ContextScope;

    /// Thread-local pointer to the active context (defined in context_propagation.cpp).
    static thread_local IContextPtr current_;
};

// ---------------------------------------------------------------------------
// ContextScope — defined before propagate() body so it can be used in the
// template implementation below.
// ---------------------------------------------------------------------------

/**
 * @brief RAII guard that installs an `IContextPtr` as the current thread
 *        context and restores the previous one on destruction.
 *
 * `ContextScope` follows standard RAII semantics: the previous context is
 * always restored when the scope exits, whether by normal return or by
 * exception.
 *
 * Scopes may be nested: inner scopes shadow the outer context for their
 * lifetime.
 *
 * @code
 *   auto root = SimpleContext::create("t-1", "r-1");
 *   {
 *       ContextScope outer(root);
 *       // ContextPropagation::current() == root
 *
 *       auto child = root->createChild();
 *       child->set(context_keys::kOperation, "db.query");
 *       {
 *           ContextScope inner(child);
 *           // ContextPropagation::current() == child
 *       }
 *       // ContextPropagation::current() == root  (restored)
 *   }
 *   // ContextPropagation::current() == nullptr (restored)
 * @endcode
 */
class ContextScope {
public:
    /**
     * @brief Install @p ctx as the current thread context.
     *
     * @param ctx Context to install.  May be `nullptr` to reset the
     *            current context while still ensuring proper restoration.
     */
    explicit ContextScope(IContextPtr ctx) noexcept
        : previous_(ContextPropagation::current_)
    {
        ContextPropagation::current_ = std::move(ctx);
    }

    /**
     * @brief Restore the previous context.
     */
    ~ContextScope() noexcept {
        ContextPropagation::current_ = std::move(previous_);
    }

    // Not copyable or movable — a scope guard must stay on one thread's stack.
    ContextScope(const ContextScope&)            = delete;
    ContextScope& operator=(const ContextScope&) = delete;
    ContextScope(ContextScope&&)                 = delete;
    ContextScope& operator=(ContextScope&&)      = delete;

private:
    IContextPtr previous_;
};

// ---------------------------------------------------------------------------
// ContextPropagation::propagate() — out-of-line template body
// (must appear after ContextScope is fully defined)
// ---------------------------------------------------------------------------

template <typename Fn>
auto ContextPropagation::propagate(Fn&& fn)
    -> std::future<std::invoke_result_t<Fn>>
{
    // Capture the caller's context; create a child so the async task can
    // add its own attributes without modifying the caller's context.
    IContextPtr parent = current_;
    IContextPtr child  = parent ? parent->createChild() : nullptr;

    return std::async(std::launch::async,
        [ctx = std::move(child), f = std::forward<Fn>(fn)]() mutable
            -> std::invoke_result_t<Fn>
        {
            // Install the propagated context for the duration of this task.
            ContextScope scope(ctx);
            return f();
        });
}

} // namespace concerns
} // namespace core
} // namespace themis
