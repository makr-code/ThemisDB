/**
 * @file distributed_trace_span.h
 * @brief DistributedTraceSpan class for Wave D Phase 2A observability.
 * @version 2.4.0
 * @date 2026-08-17
 *
 * Provides a W3C Trace Context-compliant span implementation with baggage
 * propagation, event recording, and low-overhead instrumentation for
 * distributed tracing in ThemisDB.
 *
 * Wave D Phase 2A Gate: W4A-TRACE-01 (overhead ≤ 2% vs baseline)
 *
 * @see https://www.w3.org/TR/trace-context/
 * @see tests/observability/test_otel_trace_overhead.cpp
 */

#pragma once

#include <string>
#include <map>
#include <vector>
#include <memory>
#include <chrono>
#include <cstdint>
#include <shared_mutex>

namespace themis {
namespace observability {

// Forward declarations
class DistributedTraceContext;

/**
 * @brief Event recorded within a span's lifetime.
 *
 * Span events are lightweight markers with a timestamp and optional attributes.
 * Useful for recording lifecycle events (e.g., "cache_hit", "network_retry").
 */
struct SpanEvent {
    /// Event name (e.g., "cache_hit", "retry").
    std::string name;

    /// Attributes associated with this event (e.g., {"retry_count", "3"}).
    std::map<std::string, std::string> attributes;

    /// Wall-clock timestamp of the event.
    std::chrono::system_clock::time_point timestamp;

    SpanEvent() : timestamp(std::chrono::system_clock::now()) {}

    /**
     * @brief Construct a span event with name and timestamp.
     * @param event_name Name of the event.
     */
    explicit SpanEvent(std::string event_name)
        : name(std::move(event_name)), timestamp(std::chrono::system_clock::now()) {}
};

/**
 * @brief Lightweight span status for error/exception tracking.
 *
 * The OTEL spec distinguishes between "Ok" (success), "Error" (expected failure),
 * and "Unset" (default). This enum aligns with that model.
 */
enum class SpanStatus : uint8_t {
    Unset = 0,  ///< Default, no status set.
    Ok = 1,     ///< Operation completed successfully.
    Error = 2,  ///< Operation failed with an error.
};

/**
 * @brief W3C Trace Context-compliant span for distributed tracing.
 *
 * DistributedTraceSpan captures operation-level telemetry across service
 * boundaries. It supports:
 * - W3C trace context propagation (parent-child relationships)
 * - Baggage items for cross-service metadata
 * - Event recording (lightweight lifecycle markers)
 * - Low-overhead instrumentation (RAII lifetime management)
 * - Thread-safe operations (all methods callable from any thread)
 *
 * ## Usage Pattern
 *
 * ```cpp
 * // Phase 2A: Create a root span for a database operation
 * auto tracer = DistributedTracingSDK(config);
 * auto root_ctx = DistributedTraceContext::createRoot();
 * auto span = std::make_shared<DistributedTraceSpan>(
 *     "database_query", root_ctx, tracer);
 *
 * // Add baggage (e.g., user_id, tenant context)
 * span->addBaggage("user_id", "user_12345");
 * span->addBaggage("tenant_id", "tenant_abc");
 *
 * // Record events during operation
 * span->addEvent("query_start");
 * // ... execute query ...
 * span->addEvent("query_end", {{"result_rows", "42"}});
 *
 * // Propagate context to downstream service
 * auto child_ctx = span->childContext("shard_router");
 * auto downstream_headers = child_ctx->toHttpHeaders(TraceContextFormat::W3C_TRACE_CONTEXT);
 *
 * // Set operation status
 * if (error_occurred) {
 *     span->setStatus(SpanStatus::Error, "Query timeout");
 * } else {
 *     span->setStatus(SpanStatus::Ok);
 * }
 * // Span automatically flushed when destroyed (RAII)
 * ```
 *
 * ## Performance (Wave D Phase 2A Gate)
 *
 * - Span creation: < 1 µs
 * - Event recording: < 100 ns
 * - Baggage insertion: < 500 ns
 * - Context extraction/propagation: < 2 µs
 * - **Overhead target: ≤ 2% vs Wave 7 baseline** (gate W4A-TRACE-01)
 *
 * @thread_safety All public methods are thread-safe. Multiple threads may
 *                call event-recording methods concurrently.
 */
class DistributedTraceSpan {
public:
    /**
     * @brief Construct a distributed trace span.
     *
     * @param operation_name Human-readable operation name (e.g., "database_query", "shard_router").
     * @param parent_context Parent trace context (for propagation). If nullptr, creates root span.
     * @param tracer_ref Reference to a DistributedTracingSDK instance for context management.
     *
     * @note The span acquires no locks at construction time and is safe to create
     *       in latency-critical paths.
     */
    DistributedTraceSpan(
        std::string operation_name,
        std::shared_ptr<DistributedTraceContext> parent_context = nullptr);

    /**
     * @brief Virtual destructor (RAII cleanup).
     *
     * Automatically flushes any pending events/baggage to the tracer backend
     * when the span is destroyed.
     */
    virtual ~DistributedTraceSpan();

    // Prevent copying (spans are move-only)
    DistributedTraceSpan(const DistributedTraceSpan&) = delete;
    DistributedTraceSpan& operator=(const DistributedTraceSpan&) = delete;

    // Enable moving
    DistributedTraceSpan(DistributedTraceSpan&&) noexcept = default;
    DistributedTraceSpan& operator=(DistributedTraceSpan&&) noexcept = default;

    /**
     * @brief Get the span ID (unique within the trace).
     * @return 16-character hex string (64-bit span ID).
     */
    const std::string& spanId() const { return span_id_; }

    /**
     * @brief Get the trace ID (shared across all spans in the trace).
     * @return 32-character hex string (128-bit trace ID).
     */
    const std::string& traceId() const { return trace_id_; }

    /**
     * @brief Get the operation name.
     * @return Human-readable operation name passed at construction.
     */
    const std::string& operationName() const { return operation_name_; }

    /**
     * @brief Get the start time of the span.
     * @return Wall-clock timestamp of span creation.
     */
    std::chrono::system_clock::time_point startTime() const { return start_time_; }

    /**
     * @brief Add a baggage item to this span (inherited by child spans).
     *
     * Baggage items are lightweight key-value pairs that propagate to child
     * spans. Useful for passing context like user_id, tenant_id, or request
     * priority without creating explicit span attributes.
     *
     * @param key Baggage key (required, non-empty).
     * @param value Baggage value (can be empty).
     *
     * @note Thread-safe; multiple threads may add baggage concurrently.
     * @note Maximum 128 baggage items per span (oldest inherited items are dropped).
     * @note Baggage propagates automatically to child contexts created via childContext().
     */
    void addBaggage(const std::string& key, const std::string& value);

    /**
     * @brief Record an event within the span's lifetime.
     *
     * Events are lightweight markers (e.g., "cache_hit", "retry") with optional
     * attributes and an automatic timestamp.
     *
     * @param event_name Name of the event (e.g., "cache_hit", "timeout_recovered").
     * @param attributes Optional key-value pairs describing the event (e.g., {{"retry_count", "3"}}).
     *
     * @note Thread-safe; multiple threads may record events concurrently.
     * @note Events are stored inline; maximum 100 events per span.
     *
     * ## Example
     *
     * ```cpp
     * span->addEvent("query_start");
     * span->addEvent("cache_hit", {{"hit_rate", "0.85"}});
     * span->addEvent("query_end", {{"duration_ms", "42"}, {"rows", "10000"}});
     * ```
     */
    void addEvent(const std::string& event_name, const std::map<std::string, std::string>& attributes = {});

    /**
     * @brief Record span attributes (simple key-value metadata).
     *
     * Attributes are optional metadata describing the operation (e.g., user_id, query type).
     * Unlike baggage, attributes are NOT propagated to child spans.
     *
     * @param key Attribute key.
     * @param value Attribute value (string representation).
     *
     * @note Thread-safe.
     * @note Maximum 100 attributes per span.
     */
    void setAttribute(const std::string& key, const std::string& value);

    /**
     * @brief Set the span's status (success or error).
     *
     * @param status SpanStatus enum value (Ok, Error, or Unset).
     * @param message Optional error message (only used if status == Error).
     *
     * @note Thread-safe; multiple calls update the status (last-write-wins).
     */
    void setStatus(SpanStatus status, const std::string& message = "");

    /**
     * @brief Get the current span status.
     * @return Current SpanStatus value.
     */
    SpanStatus status() const { return status_.load(); }

    /**
     * @brief Get the status message (if status == Error).
     * @return Error message, empty if status != Error.
     */
    std::string statusMessage() const;

    /**
     * @brief Create a child trace context for a downstream operation.
     *
     * Returns a new DistributedTraceContext with the same trace ID but a new
     * span ID, ready for propagation to a child operation. The child context
     * automatically inherits baggage from this span.
     *
     * @param child_operation_name Name of the child operation (e.g., "shard_router").
     * @return New DistributedTraceContext with inherited baggage.
     *
     * @note Thread-safe; can be called from multiple threads concurrently.
     */
    std::shared_ptr<DistributedTraceContext> childContext(const std::string& child_operation_name);

    /**
     * @brief Get all recorded events.
     * @return Immutable vector of SpanEvent (timeline order).
     *
     * @note Thread-safe read.
     */
    std::vector<SpanEvent> events() const;

    /**
     * @brief Get all baggage items.
     * @return Vector of {key, value} pairs currently in this span's baggage.
     *
     * @note Thread-safe read.
     */
    std::vector<std::pair<std::string, std::string>> baggage() const;

    /**
     * @brief Get all attributes.
     * @return Map of {key, value} pairs currently set as attributes.
     *
     * @note Thread-safe read.
     */
    std::map<std::string, std::string> attributes() const;

    /**
     * @brief Flush span to backend (async, non-blocking).
     *
     * Enqueues the span for export to the configured OTel backend. This is
     * called automatically by the destructor, but can be called explicitly
     * for checkpoint operations.
     *
     * @note Non-blocking; flush happens asynchronously.
     * @note Multiple flushes are safe; span is only exported once.
     * @note Thread-safe.
     */
    void flush();

    /**
     * @brief Get the span's duration (from start to current time).
     * @return Duration in microseconds.
     */
    uint64_t durationMicros() const;

private:
    std::string span_id_;                                    ///< Unique span ID (16-hex)
    std::string trace_id_;                                   ///< Trace ID (32-hex, shared across spans)
    std::string operation_name_;                             ///< Operation name
    std::chrono::system_clock::time_point start_time_;       ///< Span start time
    std::shared_ptr<DistributedTraceContext> parent_context_;  ///< Parent trace context

    std::atomic<SpanStatus> status_{SpanStatus::Unset};      ///< Span status
    mutable std::shared_mutex status_mutex_;
    std::string status_message_;                             ///< Error message (if status == Error)

    std::vector<SpanEvent> events_;                          ///< Recorded events
    mutable std::shared_mutex events_mutex_;

    std::map<std::string, std::string> baggage_;             ///< Baggage items
    mutable std::shared_mutex baggage_mutex_;

    std::map<std::string, std::string> attributes_;          ///< Span attributes
    mutable std::shared_mutex attributes_mutex_;

    std::atomic<bool> flushed_{false};                       ///< Whether span has been flushed

    // Helper methods
    std::string generateSpanId();
    void flushInternal();
};

} // namespace observability
} // namespace themis
