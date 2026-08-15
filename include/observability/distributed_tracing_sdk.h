/**
 * @file distributed_tracing_sdk.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟡 BETA (Phase 2 Observability Expansion)
 * @note Score: 0/100 (implementation in progress)
 * @note Status: Distributed tracing SDK for cross-service span propagation
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include <string>
#include <map>
#include <memory>
#include <vector>
#include <cstdint>
#include <chrono>

namespace themis {
namespace observability {

// Forward declarations
class ISpan;

/**
 * @brief Enumeration of supported trace context propagation formats.
 *
 * The distributed tracing SDK supports multiple industry-standard
 * trace context propagation formats for cross-service communication.
 *
 * @see https://www.w3.org/TR/trace-context/
 * @see https://www.jaegertracing.io/docs/latest/client-libraries/
 */
enum class TraceContextFormat {
    W3C_TRACE_CONTEXT,  ///< W3C Trace Context (traceparent + tracestate headers)
    JAEGER_BAGGAGE,     ///< Jaeger Baggage Header (uber-trace-id + jaeger-baggage)
    B3_SINGLE,          ///< Zipkin B3 Single Header (b3 header)
    B3_MULTI,           ///< Zipkin B3 Multi Header (x-b3-traceid, x-b3-spanid, etc.)
};

/**
 * @brief Baggage item metadata for trace propagation.
 *
 * Baggage is a set of user-defined key-value pairs that are propagated
 * across service boundaries as part of the trace context. Baggage is
 * useful for passing domain-specific metadata (e.g., tenant ID, request
 * priority) without creating explicit span attributes.
 */
struct BaggageItem {
    /// Key name (required, must be non-empty).
    std::string key;

    /// Value (required, can be empty).
    std::string value;

    /// Whether this baggage item was preserved from the parent context.
    bool inherited{false};
};

/**
 * @brief Distributed trace context snapshot for cross-service propagation.
 *
 * A DistributedTraceContext captures the essential information needed to
 * propagate a trace across service boundaries. It includes the trace ID,
 * span ID, baggage items, and trace flags. The context is immutable after
 * creation and safe to pass across service boundaries.
 *
 * ## Usage Pattern
 *
 * ```cpp
 * // Service A: Create a span and extract its context
 * auto span = tracer.startSpan("upstream-request");
 * auto ctx = tracer.extractDistributedContext(span.get());
 *
 * // Serialize the context into HTTP headers
 * auto headers = ctx.toHttpHeaders(TraceContextFormat::W3C_TRACE_CONTEXT);
 *
 * // Send HTTP request with propagated headers to Service B
 * http_client.post("/api/downstream", headers, body);
 *
 * // Service B: Receive the context and create a child span
 * auto headers_received = request.headers();
 * auto parent_ctx = DistributedTraceContext::fromHttpHeaders(
 *     headers_received, TraceContextFormat::W3C_TRACE_CONTEXT);
 * auto child_span = tracer.startChildSpan("downstream-handler", parent_ctx);
 * ```
 */
class DistributedTraceContext {
public:
    /**
     * @brief Create an empty/root trace context.
     * @return A new DistributedTraceContext with a fresh trace ID.
     */
    static std::shared_ptr<DistributedTraceContext> createRoot();

    /**
     * @brief Extract trace context from HTTP headers.
     *
     * Parses the supplied HTTP headers to extract trace context information
     * in the specified format. Supports W3C Trace Context, Jaeger Baggage,
     * B3 Single, and B3 Multi formats.
     *
     * @param headers HTTP headers (map of header name -> value).
     * @param format Propagation format to use for extraction.
     * @return A new DistributedTraceContext, or nullptr if headers are malformed.
     *
     * @note If headers do not contain valid trace context, a new root context
     *       is created with a fresh trace ID.
     */
    static std::shared_ptr<DistributedTraceContext> fromHttpHeaders(
        const std::map<std::string, std::string>& headers,
        TraceContextFormat format);

    /**
     * @brief Convert trace context to HTTP headers.
     *
     * Serializes the trace context into HTTP headers suitable for sending
     * to a downstream service.
     *
     * @param format Propagation format to use for serialization.
     * @return Map of header name -> value ready for HTTP transmission.
     *
     * @note The format of headers depends on @p format:
     *       - W3C_TRACE_CONTEXT: "traceparent" and "tracestate" headers
     *       - JAEGER_BAGGAGE: "uber-trace-id" and "jaeger-baggage" headers
     *       - B3_SINGLE: single "b3" header
     *       - B3_MULTI: "x-b3-traceid", "x-b3-spanid", "x-b3-sampled", etc.
     */
    std::map<std::string, std::string> toHttpHeaders(TraceContextFormat format) const;

    /**
     * @brief Get the trace ID as a hex string.
     * @return 32-character hex string (128-bit trace ID).
     */
    const std::string& traceId() const { return trace_id_; }

    /**
     * @brief Get the parent span ID as a hex string.
     * @return 16-character hex string (64-bit span ID), or empty if root context.
     */
    const std::string& parentSpanId() const { return parent_span_id_; }

    /**
     * @brief Get the trace state (W3C Trace Context tracestate value).
     * @return Comma-separated list of vendor-specific trace state key-value pairs.
     */
    const std::string& traceState() const { return trace_state_; }

    /**
     * @brief Get whether this trace is sampled.
     * @return true if the trace should be sampled, false otherwise.
     */
    bool isTraceSampled() const { return trace_sampled_; }

    /**
     * @brief Get baggage items associated with this trace context.
     * @return Immutable vector of baggage items.
     */
    const std::vector<BaggageItem>& baggage() const { return baggage_; }

    /**
     * @brief Add a baggage item to the trace context.
     *
     * Creates a new DistributedTraceContext with an additional baggage item.
     * The original context is unmodified.
     *
     * @param key Baggage key (required, non-empty).
     * @param value Baggage value (can be empty).
     * @return New DistributedTraceContext with the added baggage item.
     *
     * @note Maximum of 128 baggage items are allowed per context.
     *       If limit is exceeded, the oldest inherited baggage item is dropped.
     */
    std::shared_ptr<DistributedTraceContext> withBaggage(
        const std::string& key,
        const std::string& value) const;

    /**
     * @brief Get the timestamp when this context was created.
     * @return Wall-clock timestamp of context creation.
     */
    std::chrono::system_clock::time_point createdAt() const { return created_at_; }

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~DistributedTraceContext() = default;

protected:
    std::string trace_id_;           ///< 128-bit trace ID as hex string
    std::string parent_span_id_;     ///< Parent span ID, empty if root
    std::string trace_state_;        ///< W3C tracestate value
    bool trace_sampled_;             ///< Trace sampling flag
    std::vector<BaggageItem> baggage_;  ///< Baggage items for cross-service metadata
    std::chrono::system_clock::time_point created_at_;  ///< Context creation time
};

/**
 * @brief Configuration for the distributed tracing SDK.
 *
 * Specifies how the SDK should handle trace context propagation,
 * baggage handling, and upstream/downstream service interactions.
 */
struct DistributedTracingConfig {
    /// Default propagation format for serializing trace context.
    TraceContextFormat default_format{TraceContextFormat::W3C_TRACE_CONTEXT};

    /// Whether to inherit baggage from parent context.
    bool inherit_baggage{true};

    /// Maximum number of baggage items per trace context.
    std::size_t max_baggage_items{128};

    /// Whether to automatically propagate baggage in downstream calls.
    bool propagate_baggage{true};

    /// Service name for identifying this service in trace context (optional).
    std::string service_name;

    /// Whether to enable distributed trace correlation logging.
    bool enable_trace_correlation_logging{true};
};

/**
 * @brief Result of a distributed trace operation.
 *
 * Encapsulates the success/failure status of distributed tracing operations.
 */
struct DistributedTraceResult {
    /// Whether the operation succeeded.
    bool success{false};

    /// Error code if operation failed (0 = no error).
    int error_code{0};

    /// Human-readable error message if operation failed.
    std::string error_message;

    /// Optional additional context for debugging.
    std::string debug_context;
};

/**
 * @brief Distributed tracing SDK for cross-service span propagation.
 *
 * The DistributedTracingSDK provides utilities for:
 * - Extracting trace context from incoming requests
 * - Propagating trace context to outgoing requests
 * - Managing baggage items across service boundaries
 * - Supporting multiple trace propagation formats (W3C, Jaeger, B3)
 *
 * This SDK is designed to integrate seamlessly with the OpenTelemetryTracer
 * to provide end-to-end distributed tracing capabilities.
 *
 * ## Error Codes (Observability Phase 2 Extension)
 *
 * - DTI_INVALID_TRACE_CONTEXT = 10
 * - DTI_BAGGAGE_OVERFLOW = 11
 * - DTI_UNSUPPORTED_FORMAT = 12
 * - DTI_HEADER_PARSE_ERROR = 13
 * - DTI_CONTEXT_PROPAGATION_FAILED = 14
 * - DTI_INTERNAL_ERROR = 15
 */
class DistributedTracingSDK {
public:
    /**
     * @brief Construct a distributed tracing SDK instance.
     * @param config Configuration for the SDK.
     */
    explicit DistributedTracingSDK(const DistributedTracingConfig& config = {});

    /**
     * @brief Virtual destructor for safe polymorphic deletion.
     */
    virtual ~DistributedTracingSDK() = default;

    /**
     * @brief Extract trace context from incoming HTTP headers.
     *
     * Parses HTTP headers to extract trace context information in the
     * configured or explicitly-specified format. If no valid trace context
     * is found, creates a new root context.
     *
     * @param headers HTTP headers from incoming request.
     * @param format Optional override for propagation format (uses config default if omitted).
     * @return Extracted or newly-created DistributedTraceContext.
     *
     * @throws std::invalid_argument if headers contain a malformed trace ID.
     *
     * @note This function is thread-safe.
     */
    std::shared_ptr<DistributedTraceContext> extractContextFromHeaders(
        const std::map<std::string, std::string>& headers,
        const TraceContextFormat* format = nullptr);

    /**
     * @brief Propagate trace context to outgoing HTTP headers.
     *
     * Serializes a trace context into HTTP headers suitable for sending
     * to a downstream service. Optionally includes baggage items if
     * propagate_baggage is enabled in the configuration.
     *
     * @param context Trace context to propagate.
     * @param format Optional override for propagation format (uses config default if omitted).
     * @return Map of HTTP headers ready for transmission.
     *
     * @note This function is thread-safe.
     */
    std::map<std::string, std::string> propagateContextToHeaders(
        const std::shared_ptr<DistributedTraceContext>& context,
        const TraceContextFormat* format = nullptr);

    /**
     * @brief Verify that trace context meets format requirements.
     *
     * Validates that a DistributedTraceContext is well-formed and can
     * be safely propagated to downstream services.
     *
     * @param context Trace context to validate.
     * @return DistributedTraceResult with success status and error details.
     *
     * @note This function is thread-safe.
     */
    DistributedTraceResult validateTraceContext(
        const std::shared_ptr<DistributedTraceContext>& context);

    /**
     * @brief Create a child trace context from a parent context.
     *
     * Creates a new DistributedTraceContext with the same trace ID but
     * a new parent span ID, ready for propagation to a downstream service.
     *
     * @param parent_context The parent trace context.
     * @param new_span_id The span ID for the new span (generated if empty).
     * @return New DistributedTraceContext ready for downstream propagation.
     *
     * @throws std::invalid_argument if parent_context is invalid.
     *
     * @note This function is thread-safe.
     */
    std::shared_ptr<DistributedTraceContext> createChildContext(
        const std::shared_ptr<DistributedTraceContext>& parent_context,
        const std::string& new_span_id = "");

    /**
     * @brief Get the current configuration.
     * @return Reference to the SDK configuration.
     */
    const DistributedTracingConfig& configuration() const { return config_; }

    /**
     * @brief Set a new configuration.
     * @param config New configuration to apply.
     *
     * @note Configuration changes are thread-safe but do not affect
     *       already-extracted contexts.
     */
    void setConfiguration(const DistributedTracingConfig& config) { config_ = config; }

private:
    DistributedTracingConfig config_;
    mutable std::shared_mutex config_mutex_;

    // Helper methods
    std::string generateTraceId();
    std::string generateSpanId();
};

} // namespace observability
} // namespace themis
