/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            tracing_middleware.h                               ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-03-09 03:52:35                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     114                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb0423  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 2672276c0  2026-02-28  feat(api): add TracingMiddleware for X-Correlation-ID pro... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <string_view>

namespace themis {
namespace api {

/**
 * @brief Middleware that extracts or generates a request correlation ID and
 *        propagates it through all log lines via utils::Logger::setTraceContext().
 *
 * For every inbound HTTP request the middleware:
 *  1. Reads the `X-Correlation-ID` request header; generates a UUID v4 if absent.
 *  2. Calls utils::Logger::setTraceContext() so the correlation ID appears in every
 *     log line emitted on the current thread for the duration of the request.
 *  3. Stores the ID in a thread-local variable so applyToResponse() can inject it
 *     into the response header without needing the original request.
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
 * // In applyGovernanceHeaders():
 * const auto& id = TracingMiddleware::currentCorrelationId();
 * if (!id.empty()) res.set("X-Correlation-ID", id);
 * ```
 */
class TracingMiddleware {
public:
    TracingMiddleware() = default;
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
     *
     * @param incoming_id  Value of the incoming X-Correlation-ID header, or empty.
     * @return The correlation ID that will be echoed to the client.
     */
    std::string processRequest(std::string_view incoming_id) const;

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
};

} // namespace api
} // namespace themis
