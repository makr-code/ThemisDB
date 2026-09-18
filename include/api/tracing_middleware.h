/**
 * @file tracing_middleware.h
 * @brief Request correlation ID extraction and span export middleware.
 *
 * @details Extracts or generates correlation IDs from inbound X-Correlation-ID
 * headers, propagates them through request handling, and optionally exports
 * finished request spans to an OpenTelemetry OTLP collector.
 *
 * Core responsibilities:
 *  1. Extract (or generate UUID v4) correlation ID from X-Correlation-ID header
 *  2. Store ID in thread-local context for access across handler chain
 *  3. Inject correlation ID into all subsequent log lines on the request thread
 *  4. Record span start/end times and export to OTLP collector (if configured)
 *  5. Clear context at end of request to avoid bleed-through to next request
 *
 * Thread-safety model:
 *  - Correlation ID context is thread-local (via `thread_local` storage)
 *  - Each request thread has independent correlation context
 *  - Safe to call processRequest(), finishSpan(), clearContext() concurrently
 *  - OtlpExporter (if attached) handles concurrent span enqueueing
 *
 * ### Typical usage in HTTP handler
 * ```cpp
 * // At start of request handling:
 * auto corr_id = tracing_middleware_->processRequest(
 *     req.headers.count("X-Correlation-ID") ? req.headers["X-Correlation-ID"] : "");
 *
 * // Inject into response header
 * response.set("X-Correlation-ID", corr_id);
 *
 * // After dispatching to handler, at end of request:
 * tracing_middleware_->finishSpan("HTTP GET /v1/entity", response_status);
 *
 * // Before handling next request on same thread:
 * TracingMiddleware::clearContext();
 * ```
 *
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
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

class TracingMiddleware {
public:
    TracingMiddleware() = default;

    /**
     * @brief Tracing Middleware.
     * @param[in,out] exporter Input/output parameter.
     * @return Return value.
     */
    explicit TracingMiddleware(OtlpExporter* exporter);

    ~TracingMiddleware() = default;

    // Non-copyable, movable (for use in unique_ptr / direct members)
    TracingMiddleware(const TracingMiddleware&) = delete;
    TracingMiddleware& operator=(const TracingMiddleware&) = delete;
    TracingMiddleware(TracingMiddleware&&) noexcept = default;
    TracingMiddleware& operator=(TracingMiddleware&&) noexcept = default;

    static constexpr std::string_view kCorrelationIdHeader = "X-Correlation-ID";

    /**
     * @brief Process Request.
     * @param[in] incoming_id Identifier of the incoming.
     * @return Return value.
     */
    std::string processRequest(std::string_view incoming_id) const;

    void finishSpan(std::string_view span_name, int http_status = 0) const;

    /**
     * @brief Current Correlation Id.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static const std::string& currentCorrelationId() noexcept;

    /**
     * @brief Clear Context.
     * @note Exception safety: noexcept.
     */
    static void clearContext() noexcept;

    /**
     * @brief Generate Uuid V4.
     * @return Return value.
     */
    static std::string generateUuidV4();

private:
    OtlpExporter* exporter_ = nullptr;  ///< Non-owning; may be null.
};

} // namespace api
} // namespace themis
