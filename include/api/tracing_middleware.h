/*
 * ThemisDB | File: tracing_middleware.h | Version: 0.0.15 | Last Modified: 2026-05-20 17:13:04
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 94/100 | Lines: 141
 * Open Issues: TODOs=1, Stubs=1, Gaps=3, Unimpl=0, Mock=1, Sim=0, Debt=0
 * Gap Correlation: internal=3 | external_v3=n/a | delta=n/a | status=no_external_data
 * External Severity (v3): C=n/a, H=n/a, M=n/a
 * PR: #4219 feat(api): wire TracingMiddleware tests into CMake and add CI workf... (2026-03-14T17:59:32Z)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include <string>
#include <string_view>
#include <cstdint>

namespace themis {
namespace api {

// Forward declaration — avoids pulling <otlp_exporter.h> into every TU that
// includes tracing_middleware.h.
class OtlpExporter;

/**
 * @brief Middleware that extracts or generates a request correlation ID,
 *        propagates it through all log lines, and optionally exports finished
 *        request spans to an OpenTelemetry collector via OTLP/HTTP.
 *
 * For every inbound HTTP request the middleware:
 *  1. Reads the `X-Correlation-ID` request header; generates a UUID v4 if absent.
 *  2. Calls utils::Logger::setTraceContext() so the correlation ID appears in every
 *     log line emitted on the current thread for the duration of the request.
 *  3. Stores the ID in a thread-local variable so applyToResponse() can inject it
 *     into the response header without needing the original request.
 *  4. If an OtlpExporter is attached, records the request start time and enqueues
 *     a finished span when `finishSpan()` is called at the end of the request.
 *
 * Thread safety: all methods are safe to call concurrently from different threads;
 * the correlation ID context is per-thread (thread_local storage).
 *
 * ### Typical usage in the HTTP server
 * ```cpp
 * // In routeRequest():
 * auto corr_id = tracing_middleware_->processRequest(
 *     req.count("X-Correlation-ID") ? std::string_view(req["X-Correlation-ID"]) : "");
 * struct CorrelationIdGuard {
 *     ~CorrelationIdGuard() { TracingMiddleware::clearContext(); }
 * } _guard;
 *
 * // After dispatching the request and building the response:
 * tracing_middleware_->finishSpan("HTTP GET /v1/entity/{id}", http_status);
 *
 * // In applyGovernanceHeaders():
 * const auto& id = TracingMiddleware::currentCorrelationId();
 * if (!id.empty()) res.set("X-Correlation-ID", id);
 * ```
 */
class TracingMiddleware {
public:
    /**
     * @brief Construct without OTLP export (correlation-ID propagation only).
     */
    TracingMiddleware() = default;

    /**
     * @brief Construct with an optional OTLP exporter.
     *
     * @param exporter  Non-owning pointer to an OtlpExporter that has been
     *                  started by the caller.  May be nullptr to disable span
     *                  export while keeping correlation-ID propagation.
     */
    explicit TracingMiddleware(OtlpExporter* exporter);

    ~TracingMiddleware() = default;

    // Non-copyable, movable (for use in unique_ptr / direct members)
    TracingMiddleware(const TracingMiddleware&) = delete;
    TracingMiddleware& operator=(const TracingMiddleware&) = delete;
    TracingMiddleware(TracingMiddleware&&) = default;
    TracingMiddleware& operator=(TracingMiddleware&&) = default;

    /// HTTP header name used for the correlation ID.
    static constexpr std::string_view kCorrelationIdHeader = "X-Correlation-ID";

    /**
     * @brief Extract or generate a correlation ID and activate it for this thread.
     *
     * If @p incoming_id is non-empty it is used as-is; otherwise a UUID v4 is
     * generated.  The resulting ID is:
     *  - stored in thread-local storage (readable via currentCorrelationId()),
     *  - passed to utils::Logger::setTraceContext() so that all subsequent log
     *    lines on this thread carry the ID until clearContext() is called.
     * If an OtlpExporter is attached, the current wall-clock time is stored as
     * the span start time for this thread.
     *
     * @param incoming_id  Value of the incoming X-Correlation-ID header, or empty.
     * @return The correlation ID that will be echoed to the client.
     */
    std::string processRequest(std::string_view incoming_id) const;

    /**
     * @brief Record the end of the current request as a finished span and
     *        enqueue it for OTLP export.
     *
     * Must be called after the response status is known, before clearContext().
     * No-op if no OtlpExporter is attached.
     *
     * @param span_name    Human-readable operation name, e.g. "HTTP GET /v1/entity".
     * @param http_status  HTTP response status code (200, 404, 500, …).
     */
    void finishSpan(std::string_view span_name, int http_status = 0) const;

    /**
     * @brief Return the correlation ID stored in the current thread's context.
     *
     * Returns an empty string if processRequest() has not been called on this thread,
     * or after clearContext() has been called.
     */
    static const std::string& currentCorrelationId() noexcept;

    /**
     * @brief Clear the thread-local correlation ID and reset the logger pattern.
     *
     * Must be called after the response is sent so that the next request processed
     * by the same thread starts with a clean context.  Typically invoked via a
     * RAII guard at the start of routeRequest().
     */
    static void clearContext() noexcept;

    /**
     * @brief Generate a RFC 4122 UUID v4 string (e.g. "550e8400-e29b-41d4-a716-446655440000").
     *
     * Uses a per-thread random_generator backed by a secure PRNG.
     * Thread-safe; each call returns a unique 36-character hex string.
     */
    static std::string generateUuidV4();

private:
    OtlpExporter* exporter_ = nullptr;  ///< Non-owning; may be null.
};

} // namespace api
} // namespace themis
