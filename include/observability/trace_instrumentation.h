/**
 * @file trace_instrumentation.h
 * @brief Wave D Phase 2A+D2: Trace instrumentation macros for key components.
 * @version 2.5.0
 * @date 2026-09-16
 *
 * Provides RAII-based trace instrumentation macros for:
 * - Coordinator (distributed consensus operations)
 * - ShardRouter (cross-shard routing decisions)
 * - WALShipper (write-ahead log replication)
 * - AIPluginGenerator (AI plugin generation pipeline) — Wave D D2
 * - LLMAQLHandler inference and RAG paths — Wave D D2
 *
 * Designed for minimal overhead (< 100 ns per span creation).
 *
 * Wave D Phase 2A Gate: W4A-TRACE-01 (overhead ≤ 2%)
 */

#pragma once

#include "observability/distributed_trace_span.h"
#include "observability/distributed_tracing_sdk.h"

#include <memory>
#include <string>

namespace themis {
namespace observability {

/**
 * @brief Get the global distributed tracing SDK instance.
 *
 * Lazily initializes on first call. Thread-safe.
 * Returns a reference to the singleton DistributedTracingSDK.
 *
 * @return Reference to global DistributedTracingSDK.
 */
DistributedTracingSDK& getGlobalTracingSDK();

/**
 * @brief Get the current distributed trace context (thread-local).
 *
 * Returns the trace context associated with the current thread's active span.
 * If no span is active, returns nullptr.
 *
 * @return Current trace context, or nullptr if no span is active.
 */
std::shared_ptr<DistributedTraceContext> getCurrentTraceContext();

/**
 * @brief Set the current distributed trace context (thread-local).
 *
 * Updates the trace context for the current thread. Used internally by
 * TRACE_SCOPE_* macros to propagate context through nested operations.
 *
 * @param ctx New trace context to activate (can be nullptr to clear).
 */
void setCurrentTraceContext(std::shared_ptr<DistributedTraceContext> ctx);

/**
 * @brief Get the current active span (thread-local).
 *
 * Returns the span associated with the innermost active TRACE_SCOPE on the
 * current thread. Returns nullptr if no scope is active.
 *
 * @return Pointer to active DistributedTraceSpan, or nullptr.
 */
DistributedTraceSpan* getCurrentSpan();

/**
 * @brief Set the current active span (thread-local).
 *
 * Used internally by TraceContextGuard. Do not call directly.
 *
 * @param span Raw pointer to span owned by the active guard (may be nullptr).
 */
void setCurrentSpan(DistributedTraceSpan* span);

/**
 * @brief RAII guard that activates a span scope and restores the previous
 *        context and span on destruction (including on exception paths).
 *
 * Used by TRACE_SCOPE_* macros to guarantee correct nesting — without this
 * guard the thread-local context would be left pointing at a destroyed span
 * after the scope exits.
 */
class TraceContextGuard {
public:
    /**
     * @brief Activate @p span as the current span and @p child_ctx as the
     *        current trace context for the duration of the enclosing scope.
     *
     * @param span       Active span (must outlive this guard; owned by caller).
     * @param child_ctx  Child trace context produced by the span.
     */
    TraceContextGuard(DistributedTraceSpan* span,
                      std::shared_ptr<DistributedTraceContext> child_ctx)
        : prev_ctx_(getCurrentTraceContext())
        , prev_span_(getCurrentSpan())
    {
        setCurrentTraceContext(std::move(child_ctx));
        setCurrentSpan(span);
    }

    /// Restores the previous context and span on scope exit (RAII).
    ~TraceContextGuard() {
        setCurrentSpan(prev_span_);
        setCurrentTraceContext(std::move(prev_ctx_));
    }

    TraceContextGuard(const TraceContextGuard&) = delete;
    TraceContextGuard& operator=(const TraceContextGuard&) = delete;

private:
    std::shared_ptr<DistributedTraceContext> prev_ctx_;
    DistributedTraceSpan* prev_span_;
};

// ============================================================================
// Trace Instrumentation Macros
// ============================================================================

/**
 * @brief RAII scope guard for distributed tracing.
 *
 * Records a span with the given operation name, automatically managing
 * context propagation and span lifetime. Designed for use in function scope.
 *
 * ## Usage
 *
 * ```cpp
 * void processCoordinatorRequest(const Request& req) {
 *     TRACE_SCOPE_COORDINATOR("process_request", req.trace_context);
 *     // Span is automatically created and associated with this scope
 *     // Operations within this scope are traced
 *     // Span is flushed when scope exits
 * }
 * ```
 *
 * @param operation_name Human-readable operation name (e.g., "consensus_round").
 * @param parent_context Parent trace context (for propagation). Can be nullptr for root spans.
 *
 * @note This macro creates a temporary variable; do NOT use in single-statement contexts.
 * @note RAII cleanup is guaranteed even under exceptions.
 * @note Overhead is ~1 µs per span creation (well within 2% budget for typical operations).
 */
#define TRACE_SCOPE_COORDINATOR(operation_name, parent_context) \
    auto _trace_span_##__LINE__ = std::make_shared<DistributedTraceSpan>( \
        operation_name, parent_context); \
    TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

/**
 * @brief RAII scope guard for ShardRouter tracing.
 *
 * Records a span for shard routing decisions and cross-shard operations.
 *
 * ## Usage
 *
 * ```cpp
 * RouteResult ShardRouter::route(const Query& q) {
 *     TRACE_SCOPE_SHARD_ROUTER("route_query");
 *     // Trace context automatically propagated to child operations
 *     // Decisions are recorded as events
 * }
 * ```
 *
 * @param operation_name Human-readable operation name (e.g., "route_query").
 */
#define TRACE_SCOPE_SHARD_ROUTER(operation_name) \
    auto _trace_span_##__LINE__ = std::make_shared<DistributedTraceSpan>( \
        operation_name, getCurrentTraceContext()); \
    TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

/**
 * @brief RAII scope guard for WALShipper tracing.
 *
 * Records a span for write-ahead log replication operations.
 *
 * ## Usage
 *
 * ```cpp
 * void WALShipper::shipLog(const WALSegment& seg) {
 *     TRACE_SCOPE_WAL_SHIPPER("ship_segment");
 *     // Log shipping operations are traced
 *     // Replication lag is recorded as baggage
 * }
 * ```
 *
 * @param operation_name Human-readable operation name (e.g., "ship_segment").
 */
#define TRACE_SCOPE_WAL_SHIPPER(operation_name) \
    auto _trace_span_##__LINE__ = std::make_shared<DistributedTraceSpan>( \
        operation_name, getCurrentTraceContext()); \
    TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

/**
 * @brief Record a tracing event with optional attributes.
 *
 * Adds an event to the current active span (if any). Events are lightweight
 * lifecycle markers (e.g., "cache_hit", "retry", "fallback").
 *
 * ## Usage
 *
 * ```cpp
 * TRACE_EVENT("cache_hit", {{"hit_rate", "0.85"}});
 * TRACE_EVENT("retry", {{"attempt", "2"}, {"backoff_ms", "100"}});
 * ```
 *
 * @param event_name Name of the event.
 * @param attrs Optional attributes (key-value map). Default is empty.
 *
 * @note Silently ignored if no span is currently active.
 * @note Overhead is ~100 ns per event (negligible impact on p99 latency).
 */
#define TRACE_EVENT(event_name, attrs) \
    do { \
        auto* _span = getCurrentSpan(); \
        if (_span) { \
            _span->addEvent((event_name), (attrs)); \
        } \
    } while (0)

/**
 * @brief Add baggage to the current trace context.
 *
 * Propagates a key-value pair across service boundaries (inherited by child spans).
 * Useful for passing correlation IDs, tenant context, or request priority.
 *
 * ## Usage
 *
 * ```cpp
 * TRACE_BAGGAGE("user_id", "user_12345");
 * TRACE_BAGGAGE("tenant_id", "tenant_abc");
 * TRACE_BAGGAGE("priority", "high");
 * ```
 *
 * @param key Baggage key.
 * @param value Baggage value.
 *
 * @note Maximum 128 baggage items per trace (enforced by DistributedTraceSpan).
 * @note Silently ignored if no span is currently active.
 */
#define TRACE_BAGGAGE(key, value) \
    do { \
        auto* _span = getCurrentSpan(); \
        if (_span) { \
            _span->addBaggage((key), (value)); \
        } \
    } while (0)

/**
 * @brief Set the status of the current span.
 *
 * Records operation success or failure. If status is Error, the provided
 * message is recorded as the error description.
 *
 * ## Usage
 *
 * ```cpp
 * if (success) {
 *     TRACE_SET_STATUS(SpanStatus::Ok);
 * } else {
 *     TRACE_SET_STATUS(SpanStatus::Error, "Query timeout after 30s");
 * }
 * ```
 *
 * @param status SpanStatus enum value (Ok, Error, or Unset).
 * @param message Optional error message (used only if status == Error).
 */
#define TRACE_SET_STATUS(status, ...) \
    do { \
        auto* _span = getCurrentSpan(); \
        if (_span) { \
            _span->setStatus((status), ##__VA_ARGS__); \
        } \
    } while (0)

// ============================================================================
// AI Module Trace Instrumentation — Wave D D2
// ============================================================================

/**
 * @brief RAII scope guard for AIPluginGenerator::generatePlugin() tracing.
 *
 * Records a span covering the full plugin generation pipeline:
 * input validation → endpoint invocation → response parsing → safety gate.
 *
 * Span name follows the OpenTelemetry semantic convention for internal operations:
 * "ai.plugin.generate"
 *
 * ## Usage
 *
 * ```cpp
 * Result<GeneratedPlugin> AIPluginGenerator::generatePlugin(
 *     const PluginGenerationPrompt& prompt)
 * {
 *     TRACE_SCOPE_AI_GENERATE("ai.plugin.generate");
 *     // ... generation pipeline ...
 * }
 * ```
 *
 * @param operation_name Span operation name (e.g. "ai.plugin.generate").
 *
 * @note Overhead is ~1 µs per span creation (within Wave D budget).
 * @see docs/operability/RUNBOOK_AI_GENERATION.md for Stats counter semantics.
 */
#define TRACE_SCOPE_AI_GENERATE(operation_name) \
    auto _trace_span_##__LINE__ = std::make_shared<DistributedTraceSpan>( \
        operation_name, getCurrentTraceContext()); \
    TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

/**
 * @brief RAII scope guard for LLMAQLHandler inference/RAG path tracing.
 *
 * Records a span covering the full LLM inference or RAG pipeline, including
 * routing decisions, retry attempts, CAI gate execution, and result handling.
 *
 * Span name follows OpenTelemetry semantic conventions for LLM operations:
 * "llm.infer", "llm.rag", "llm.embed"
 *
 * ## Usage
 *
 * ```cpp
 * std::string LLMAQLHandler::executeInfer(const std::string& prompt, ...) {
 *     TRACE_SCOPE_AI_INFER("llm.infer");
 *     // ... inference pipeline ...
 * }
 *
 * std::string LLMAQLHandler::executeRAG(const std::string& query, ...) {
 *     TRACE_SCOPE_AI_INFER("llm.rag");
 *     // ... RAG pipeline ...
 * }
 * ```
 *
 * @param operation_name Span operation name (e.g. "llm.infer", "llm.rag").
 *
 * @note Overhead is ~1 µs per span creation (within Wave D budget).
 * @note Propagates the caller's trace context; child spans for retries inherit the same trace ID.
 */
#define TRACE_SCOPE_AI_INFER(operation_name) \
    auto _trace_span_##__LINE__ = std::make_shared<DistributedTraceSpan>( \
        operation_name, getCurrentTraceContext()); \
    TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

} // namespace observability
} // namespace themis
