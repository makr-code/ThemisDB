#pragma once

#include "core/concerns/i_context.h"
#include <future>
#include <memory>
#include <string>
#include <string_view>
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

// ---------------------------------------------------------------------------
// W3C TraceContext helpers for async context propagation
// ---------------------------------------------------------------------------

namespace themis {
namespace core {
namespace concerns {

/**
 * @namespace themis::core::concerns::w3c_trace_context
 * @brief Utilities for encoding and decoding W3C Trace Context headers
 *        to/from `IContext`.
 *
 * These helpers connect the thread-local async context propagation mechanism
 * (`ContextPropagation` / `ContextScope`) with the W3C Trace Context Level 1
 * standard so that distributed traces can cross async/thread boundaries
 * without losing the upstream trace and span identifiers.
 *
 * ### Typical usage — inbound request
 * @code
 *   // At the HTTP handler entry point:
 *   auto ctx = w3c_trace_context::parseTraceparent(
 *       request.header("traceparent"));
 *   if (!ctx) ctx = SimpleContext::create();
 *   ContextScope scope(ctx);
 *
 *   // Spawn async work — W3C trace/span IDs propagate automatically.
 *   auto fut = ContextPropagation::propagate([]() { ... });
 * @endcode
 *
 * ### Typical usage — outbound call
 * @code
 *   // Inside any async task that has an active ContextScope:
 *   auto traceparent = w3c_trace_context::formatTraceparent(
 *       *ContextPropagation::current());
 *   if (!traceparent.empty()) {
 *       outboundHeaders["traceparent"] = traceparent;
 *   }
 * @endcode
 */
namespace w3c_trace_context {

/**
 * @brief Generate a W3C `traceparent` header value from an `IContext`.
 *
 * Reads `context_keys::kTraceId` (32 hex chars) and
 * `context_keys::kSpanId` (16 hex chars) from @p ctx and formats them as a
 * W3C Trace Context Level 1 `traceparent` header:
 * @code
 *   "00-{trace-id}-{span-id}-01"
 * @endcode
 *
 * The sampled flag is fixed at `01` (sampled).  If you need to propagate
 * the original flags, store them under `context_keys::kSpanId` or extend
 * this utility.
 *
 * @param ctx  Source context.  Must have `kTraceId` and `kSpanId` set.
 * @return     A `traceparent` header value string, or an empty string if
 *             either `kTraceId` or `kSpanId` is absent or empty.
 */
inline std::string formatTraceparent(const IContext& ctx) {
    const auto trace_id = ctx.get(context_keys::kTraceId);
    const auto span_id  = ctx.get(context_keys::kSpanId);
    if (!trace_id || trace_id->empty() || !span_id || span_id->empty()) {
        return {};
    }
    return "00-" + *trace_id + "-" + *span_id + "-01";
}

/**
 * @brief Parse a W3C `traceparent` header and create an `IContext` with the
 *        extracted trace and span identifiers.
 *
 * Parses the W3C Trace Context Level 1 format:
 * @code
 *   "{version}-{trace-id}-{parent-id}-{trace-flags}"
 *   e.g. "00-4bf92f3577b34da6a3ce929d0e0e4736-00f067aa0ba902b7-01"
 * @endcode
 *
 * On success returns a `SimpleContext` with:
 *   - `context_keys::kTraceId`  set to the 32-char trace-id
 *   - `context_keys::kSpanId`   set to the 16-char parent-id
 *
 * The returned context can be installed with `ContextScope` so that
 * `ContextPropagation::propagate()` automatically flows the W3C trace
 * context into every spawned async task.
 *
 * @param header  The value of an inbound `traceparent` HTTP header.
 * @return        An `IContextPtr` with `kTraceId` and `kSpanId` populated,
 *                or `nullptr` if @p header is absent, shorter than 55 chars,
 *                does not match the expected dash-separated format, has
 *                wrong field lengths, or carries all-zero trace / span IDs
 *                (which are invalid per the W3C spec).
 */
inline IContextPtr parseTraceparent(std::string_view header) {
    // Minimum valid length: "00-{32hex}-{16hex}-{2hex}" = 55 chars.
    if (header.size() < 55) return nullptr;

    const auto d1 = header.find('-');
    if (d1 == std::string_view::npos) return nullptr;

    const auto d2 = header.find('-', d1 + 1);
    if (d2 == std::string_view::npos) return nullptr;

    const auto d3 = header.find('-', d2 + 1);
    if (d3 == std::string_view::npos) return nullptr;

    const auto trace_id  = header.substr(d1 + 1, d2 - d1 - 1);
    const auto parent_id = header.substr(d2 + 1, d3 - d2 - 1);

    // W3C spec: trace-id must be 32 hex chars; parent-id must be 16 hex chars.
    if (trace_id.size()  != 32) return nullptr;
    if (parent_id.size() != 16) return nullptr;

    // W3C spec: all-zero IDs are invalid.
    if (trace_id  == "00000000000000000000000000000000") return nullptr;
    if (parent_id == "0000000000000000") return nullptr;

    auto ctx = SimpleContext::create();
    ctx->set(context_keys::kTraceId, trace_id);
    ctx->set(context_keys::kSpanId,  parent_id);
    return ctx;
}

} // namespace w3c_trace_context

} // namespace concerns
} // namespace core
} // namespace themis
