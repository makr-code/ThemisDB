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
    auto _trace_span_##__LINE__ = std::make_shared<themis::observability::DistributedTraceSpan>( \
        operation_name, parent_context); \
    themis::observability::TraceContextGuard _trace_guard_##__LINE__( \
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
    auto _trace_span_##__LINE__ = std::make_shared<themis::observability::DistributedTraceSpan>( \
        operation_name, themis::observability::getCurrentTraceContext()); \
    themis::observability::TraceContextGuard _trace_guard_##__LINE__( \
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
    auto _trace_span_##__LINE__ = std::make_shared<themis::observability::DistributedTraceSpan>( \
        operation_name, themis::observability::getCurrentTraceContext()); \
    themis::observability::TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

#if defined(THEMIS_ENABLE_TRACING)
inline void recordTraceEvent(std::string_view event_name,
                             std::initializer_list<std::pair<std::string, std::string>> attrs = {}) {
    auto* span = getCurrentSpan();
    if (span) {
        std::map<std::string, std::string> attributes;
        for (const auto& [key, value] : attrs) {
            attributes.emplace(key, value);
        }
        span->addEvent(std::string(event_name), attributes);
    }
}

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
#define TRACE_EVENT(event_name, ...) \
    do { \
        auto* _span = themis::observability::getCurrentSpan(); \
        if (_span) { \
            _span->addEvent((event_name) __VA_OPT__(,) __VA_ARGS__); \
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
        auto* _span = themis::observability::getCurrentSpan(); \
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
        auto* _span = themis::observability::getCurrentSpan(); \
        if (_span) { \
            _span->setStatus((status), ##__VA_ARGS__); \
        } \
    } while (0)

// ============================================================================
// AI Module Trace Instrumentation — Wave D D2
// ============================================================================

/**
 * @brief RAII scope guard for AI module pipeline tracing (Wave D D2).
 *
 * Records a span covering any AI module pipeline operation.  Inherits the
 * caller's trace context so spans appear as child operations in the parent
 * trace tree (e.g. an incoming AQL query that triggers inference).
 *
 * Span names follow OpenTelemetry semantic conventions:
 * - "ai.plugin.generate" — AIPluginGenerator::generatePlugin()
 * - "llm.infer"          — LLMAQLHandler::executeInfer()
 * - "llm.rag"            — LLMAQLHandler::executeRAG()
 * - "llm.embed"          — LLMAQLHandler::executeEmbed()
 *
 * ## Usage
 *
 * ```cpp
 * Result<GeneratedPlugin> AIPluginGenerator::generatePlugin(...) {
 *     TRACE_SCOPE_AI("ai.plugin.generate");
 *     // ... pipeline ...
 * }
 *
 * std::string LLMAQLHandler::executeInfer(...) {
 *     TRACE_SCOPE_AI("llm.infer");
 *     // ... pipeline ...
 * }
 * ```
 *
 * @param operation_name Span operation name following OTel conventions.
 *
 * @note Overhead is ~1 µs per span creation (within Wave D budget).
 * @note The span is automatically ended when the enclosing scope exits (RAII).
 * @see docs/operability/RUNBOOK_AI_GENERATION.md for Stats counter semantics.
 * @see src/ai/WAVE_D_ROADMAP.md — D2: Observability Expansion.
 */
#define TRACE_SCOPE_AI(operation_name) \
    auto _trace_span_##__LINE__ = std::make_shared<themis::observability::DistributedTraceSpan>( \
        operation_name, themis::observability::getCurrentTraceContext()); \
    themis::observability::TraceContextGuard _trace_guard_##__LINE__( \
        _trace_span_##__LINE__.get(), \
        _trace_span_##__LINE__->childContext(operation_name));

#else

inline void recordTraceEvent(std::string_view,
                             std::initializer_list<std::pair<std::string, std::string>> = {}) {}

#define TRACE_SCOPE_COORDINATOR(operation_name, parent_context) do { } while (false)
#define TRACE_SCOPE_SHARD_ROUTER(operation_name) do { } while (false)
#define TRACE_SCOPE_WAL_SHIPPER(operation_name) do { } while (false)
#define TRACE_EVENT(event_name, ...) do { } while (false)
#define TRACE_BAGGAGE(key, value) do { } while (false)
#define TRACE_SET_STATUS(status, ...) do { } while (false)
#define TRACE_SCOPE_AI(operation_name) do { } while (false)

#endif

} // namespace observability
} // namespace themis
