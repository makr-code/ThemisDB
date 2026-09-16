/**
 * @file trace_context.h
 * @brief Lightweight distributed-tracing context and span emitter for the base module.
 * @version 1.0.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Status: Wave D delivery — distributed tracing integration
 *
 * Provides a zero-external-dependency tracing abstraction suitable for the
 * base module runtime.  The design follows OpenTelemetry concepts (trace ID,
 * span ID, parent span ID) but is implemented with C++17 std-only primitives
 * so that the base module retains its "no external dependencies" contract.
 *
 * Key types:
 *  - @c TraceContext  — immutable value object carrying trace/span IDs.
 *  - @c SpanEmitter   — injectable callback interface for span events.
 *  - @c ScopedSpan    — RAII helper that records start/end events via the
 *                       injected emitter.
 *  - @c NoOpSpanEmitter — default no-op implementation; zero overhead when
 *                          tracing is not configured.
 *
 * Usage (instrumented call site):
 * @code
 *   // obtain an emitter from the owning subsystem (may be NoOp) — store by value
 *   SpanEmitter emitter = manager.spanEmitter();
 *
 *   TraceContext ctx = TraceContext::generate("hot_reload_manager.reloadModule");
 *   ScopedSpan span(ctx, emitter);
 *   // ... do work ...
 *   // span end is recorded automatically on scope exit
 * @endcode
 *
 * Thread safety: @c TraceContext is a plain value type (copyable, movable).
 *               @c ScopedSpan must not be shared across threads.
 *               @c SpanEmitter implementations must be thread-safe if the
 *               emitter is shared across threads.
 */

#pragma once

#include <atomic>
#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace themis {
namespace modules {

// =============================================================================
// TraceContext — immutable span identity value object
// =============================================================================

/**
 * @brief Immutable value object carrying the identity of a single tracing span.
 *
 * IDs are represented as 64-bit unsigned integers (compatible with
 * OpenTelemetry W3C trace-context encoding).  @c generate() produces a new
 * root context with a unique trace ID; @c child() derives a child span that
 * shares the parent's trace ID.
 *
 * Ownership contract: @c TraceContext is cheap to copy (two uint64_t + one
 * uint64_t parent ID + one std::string operation name ≤ SSO threshold).
 */
struct TraceContext {
    /// @brief Unique trace identifier (shared across all spans in one trace).
    uint64_t trace_id    = 0;

    /// @brief Identifier of this span within the trace.
    uint64_t span_id     = 0;

    /// @brief Span ID of the parent span; 0 if this is a root span.
    uint64_t parent_span_id = 0;

    /// @brief Human-readable operation name (e.g. "hot_reload_manager.reloadModule").
    std::string operation_name;

    // -------------------------------------------------------------------------
    // Factory helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Generate a new root @c TraceContext with a unique trace ID and
     *        span ID.
     *
     * @param operation  Operation name for this span.
     * @return New root @c TraceContext.
     */
    static TraceContext generate(std::string_view operation) {
        TraceContext ctx;
        ctx.trace_id        = nextId();
        ctx.span_id         = nextId();
        ctx.parent_span_id  = 0;
        ctx.operation_name  = std::string(operation);
        return ctx;
    }

    /**
     * @brief Derive a child @c TraceContext that shares this span's trace ID.
     *
     * The child receives a fresh span ID; its parent_span_id is set to this
     * span's @c span_id.
     *
     * @param operation  Operation name for the child span.
     * @return Child @c TraceContext.
     */
    TraceContext child(std::string_view operation) const {
        TraceContext ctx;
        ctx.trace_id        = trace_id;
        ctx.span_id         = nextId();
        ctx.parent_span_id  = span_id;
        ctx.operation_name  = std::string(operation);
        return ctx;
    }

    /// @brief Return true when this context carries a valid (non-zero) span ID.
    [[nodiscard]] bool isValid() const noexcept { return span_id != 0; }

private:
    /// @brief Thread-safe monotonic ID counter.
    static uint64_t nextId() noexcept {
        static std::atomic<uint64_t> counter{1};
        return counter.fetch_add(1, std::memory_order_relaxed);
    }
};

// =============================================================================
// SpanEvent — payload emitted at span start/end
// =============================================================================

/**
 * @brief Payload passed to @c SpanEmitter callbacks.
 */
struct SpanEvent {
    /// @brief The trace context this event belongs to.
    TraceContext context;

    /// @brief Monotonic wall-clock timestamp of the event.
    std::chrono::steady_clock::time_point timestamp;

    /// @brief True when this is a span-start event; false for span-end.
    bool is_start = true;

    /// @brief Optional error code (0 = success).  Set on span-end for errors.
    int error_code = 0;

    /// @brief Optional error detail string.
    std::string error_detail;
};

// =============================================================================
// SpanEmitter — injectable observer interface
// =============================================================================

/**
 * @brief Injectable span emitter callback.
 *
 * Callers receive span events via the @c operator() overload.  The default
 * type alias accepts @c std::function<void(const SpanEvent&)> so any lambda,
 * function pointer, or callable can be used.
 *
 * Thread safety: implementations shared across threads must be internally
 * synchronized.
 */
using SpanEmitter = std::function<void(const SpanEvent&)>;

/**
 * @brief Return a no-op @c SpanEmitter that discards all events.
 *
 * Used as the default emitter when tracing is not configured, ensuring
 * zero overhead from tracing instrumentation in production paths.
 */
inline SpanEmitter noOpSpanEmitter() noexcept {
    return [](const SpanEvent&) noexcept {};
}

// =============================================================================
// ScopedSpan — RAII span lifetime helper
// =============================================================================

/**
 * @brief RAII helper that records span start on construction and span end on
 *        destruction.
 *
 * @code
 *   TraceContext ctx = TraceContext::generate("module_loader.loadModule");
 *   ScopedSpan span(ctx, emitter);
 *   // ... do work ...
 *   // if an error occurred, mark it before destruction:
 *   span.setError(BASE_LOADER_INIT_FAILED::code, "init returned -1");
 *   // destructor emits span-end with the error detail
 * @endcode
 *
 * @note ScopedSpan is non-copyable, non-movable.  It must not outlive the
 *       @c SpanEmitter reference it holds.
 */
class ScopedSpan {
public:
    /**
     * @brief Construct and emit a span-start event.
     * @param ctx      Trace context for this span.
     * @param emitter  Emitter to receive span events (must outlive ScopedSpan).
     */
    explicit ScopedSpan(const TraceContext& ctx, SpanEmitter& emitter)
        : ctx_(ctx), emitter_(emitter), error_code_(0) {
        SpanEvent ev;
        ev.context   = ctx_;
        ev.timestamp = std::chrono::steady_clock::now();
        ev.is_start  = true;
        emitter_(ev);
    }

    ~ScopedSpan() noexcept {
        SpanEvent ev;
        ev.context      = ctx_;
        ev.timestamp    = std::chrono::steady_clock::now();
        ev.is_start     = false;
        ev.error_code   = error_code_;
        ev.error_detail = std::move(error_detail_);
        emitter_(ev);
    }

    /// @brief Mark this span as having ended with an error.
    /// @param code    Base module taxonomy error code.
    /// @param detail  Human-readable detail string.
    void setError(int code, std::string detail) noexcept {
        error_code_   = code;
        error_detail_ = std::move(detail);
    }

    // Non-copyable, non-movable.
    ScopedSpan(const ScopedSpan&)            = delete;
    ScopedSpan& operator=(const ScopedSpan&) = delete;
    ScopedSpan(ScopedSpan&&)                 = delete;
    ScopedSpan& operator=(ScopedSpan&&)      = delete;

private:
    TraceContext  ctx_;
    SpanEmitter&  emitter_;
    int           error_code_;
    std::string   error_detail_;
};

} // namespace modules
} // namespace themis
