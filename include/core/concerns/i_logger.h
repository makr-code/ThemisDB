#pragma once

#include "core/concerns/lifecycle.h"
#include <string>
#include <memory>
#include <map>
#include <utility>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Trace/request context for structured log correlation.
 *
 * Carries a trace_id and request_id that are injected into every structured
 * log event so individual log lines can be correlated with distributed traces
 * and originating HTTP requests.
 */
struct TraceContext {
    std::string trace_id;    ///< OpenTelemetry trace id (hex string)
    std::string request_id;  ///< Per-request/RPC correlation id

    bool empty() const noexcept { return trace_id.empty() && request_id.empty(); }
};

/**
 * @brief Abstract logger interface for dependency injection.
 * 
 * Provides a unified logging interface that can be implemented by various
 * logging backends (spdlog, console, no-op, etc.). Enables testing with
 * mock loggers and runtime switching of logging implementations.
 *
 * Structured logging extension:
 *   - logStructured() emits a JSON-formatted log line with arbitrary key/value
 *     fields, enabling machine-readable log ingestion.
 *   - logWithContext() additionally injects a TraceContext so every log line
 *     carries trace_id and request_id for end-to-end correlation.
 */
class ILogger {
public:
    using Fields = std::map<std::string, std::string>;

    enum class Level {
        TRACE,
        DEBUG,
        INFO,
        WARN,
        ERROR,
        CRITICAL
    };

    virtual ~ILogger() = default;

    // Core logging methods
    virtual void log(Level level, const std::string& message) = 0;
    
    virtual void trace(const std::string& message) = 0;
    virtual void debug(const std::string& message) = 0;
    virtual void info(const std::string& message) = 0;
    virtual void warn(const std::string& message) = 0;
    virtual void error(const std::string& message) = 0;
    virtual void critical(const std::string& message) = 0;

    /**
     * @brief Emit a structured (JSON) log line with arbitrary key/value fields.
     *
     * Implementations MUST produce a single-line JSON object containing at least
     * "level" and "message" keys, plus every entry in @p fields.
     * PII-sensitive field values SHOULD be redacted by the implementation before
     * they are written to the sink.
     *
     * Default implementation falls back to plain log() for backends that do not
     * override this method.
     */
    virtual void logStructured(Level level,
                               const std::string& message,
                               const Fields& fields = {}) {
        log(level, message);
    }

    /**
     * @brief Emit a structured log line with trace/request-id context injected.
     *
     * Equivalent to logStructured() but also injects ctx.trace_id and
     * ctx.request_id into the emitted JSON object so log lines can be
     * correlated with distributed traces.
     */
    virtual void logWithContext(Level level,
                                const std::string& message,
                                const TraceContext& ctx,
                                const Fields& fields = {}) {
        Fields merged = fields;
        if (!ctx.trace_id.empty())  merged["trace_id"]   = ctx.trace_id;
        if (!ctx.request_id.empty()) merged["request_id"] = ctx.request_id;
        logStructured(level, message, merged);
    }

    // Configuration methods
    virtual void setLevel(Level level) = 0;
    virtual Level getLevel() const = 0;
    virtual void setPattern(const std::string& pattern) = 0;

    // Lifecycle hooks
    /**
     * @brief Flush any buffered log records to the underlying sink.
     *
     * Must be called before process exit (or between test cases) to
     * ensure no messages are lost.  Default is a no-op for backends
     * that do not buffer.
     */
    virtual void flush() noexcept {}

    /**
     * @brief Shut down the logger and release resources.
     *
     * After shutdown(), all logging calls are silently dropped.
     * Default is a no-op.
     */
    virtual void shutdown() noexcept {}

    /**
     * @brief Probe whether the logging sink is reachable and healthy.
     *
     * @return ProbeResult with ok=true when the sink is accessible,
     *         ok=false with a descriptive message otherwise.
     */
    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }

    // Helper methods
    static Level levelFromString(const std::string& level);
    static const char* levelToString(Level level);
};

} // namespace concerns
} // namespace core
} // namespace themis
