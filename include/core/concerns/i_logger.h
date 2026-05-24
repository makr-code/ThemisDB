/*
 * ThemisDB | File: i_logger.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 88/100
 * Gap Summary: total=2; TODO=0, Stub=0, Unimpl=0, Mock=2, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "core/concerns/lifecycle.h"
#include <string>
#include <memory>
#include <map>
#include <utility>

#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Trace/request context for structured log correlation.
 *
 * Carries a trace_id, span_id, and request_id that are injected into every
 * structured log event so individual log lines can be correlated with
 * distributed traces and originating HTTP requests.
 */
struct TraceContext {
    std::string trace_id;    ///< OpenTelemetry trace id (hex string)
    std::string span_id;     ///< OpenTelemetry span id (hex string)
    std::string request_id;  ///< Per-request/RPC correlation id

    bool empty() const noexcept {
        return trace_id.empty() && span_id.empty() && request_id.empty();
    }
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

    // -----------------------------------------------------------------------
    // Core logging methods
    // -----------------------------------------------------------------------

    /**
     * @brief Emit a log record at the specified severity level.
     *
     * This is the single dispatch point used by all severity-specific
     * helpers.  Implementations must be thread-safe.
     *
     * @param level   Severity level for the message.
     * @param message Human-readable log text.
     */
    virtual void log(Level level, const std::string& message) = 0;

    /// @brief Log at TRACE level (most verbose, diagnostic detail).
    /// @param message Human-readable log text.
    virtual void trace(const std::string& message) = 0;

    /// @brief Log at DEBUG level (developer-facing diagnostic detail).
    /// @param message Human-readable log text.
    virtual void debug(const std::string& message) = 0;

    /// @brief Log at INFO level (normal operational events).
    /// @param message Human-readable log text.
    virtual void info(const std::string& message) = 0;

    /// @brief Log at WARN level (unexpected but recoverable condition).
    /// @param message Human-readable log text.
    virtual void warn(const std::string& message) = 0;

    /// @brief Log at ERROR level (failure that requires attention).
    /// @param message Human-readable log text.
    virtual void error(const std::string& message) = 0;

    /// @brief Log at CRITICAL level (severe failure, may require restart).
    /// @param message Human-readable log text.
    virtual void critical(const std::string& message) = 0;

    /**
     * @brief Emit a structured (JSON) log line with arbitrary key/value fields.
     *
     * Implementations MUST produce a single-line JSON object containing at least
     * "level" and "message" keys, plus every entry in @p fields.
     * PII-sensitive field values SHOULD be redacted by the implementation before
     * they are written to the sink.
     *
     * Default implementation appends fields as key=value pairs to the message
     * so that trace/span IDs injected via logWithContext() are not silently
     * dropped by backends that only override log().
     */
    virtual void logStructured(Level level,
                               const std::string& message,
                               const Fields& fields = {}) {
        if (fields.empty()) {
            log(level, message);
            return;
        }
        std::string full_msg = message;
        for (const auto& kv : fields) {
            full_msg += " " + kv.first + "=" + kv.second;
        }
        log(level, full_msg);
    }

    /**
     * @brief Emit a structured log line with trace/request-id context injected.
     *
     * Equivalent to logStructured() but also injects ctx.trace_id, ctx.span_id,
     * and ctx.request_id into the emitted JSON object so log lines can be
     * correlated with distributed traces.
     */
    virtual void logWithContext(Level level,
                                const std::string& message,
                                const TraceContext& ctx,
                                const Fields& fields = {}) {
        Fields merged = fields;
        if (!ctx.trace_id.empty())   merged["trace_id"]   = ctx.trace_id;
        if (!ctx.span_id.empty())    merged["span_id"]    = ctx.span_id;
        if (!ctx.request_id.empty()) merged["request_id"] = ctx.request_id;
        logStructured(level, message, merged);
    }

    // -----------------------------------------------------------------------
    // Configuration methods
    // -----------------------------------------------------------------------

    /**
     * @brief Set the minimum severity level; records below this level are dropped.
     * @param level New minimum level.
     */
    virtual void setLevel(Level level) = 0;

    /**
     * @brief Return the current minimum severity level.
     * @return Active minimum level.
     */
    [[nodiscard]] virtual Level getLevel() const = 0;

    /**
     * @brief Set the spdlog-compatible format pattern for log lines.
     *
     * Has no effect on backends that do not support format patterns.
     * @param pattern Format string, e.g. `"[%Y-%m-%d %H:%M:%S] [%^%l%$] %v"`.
     */
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

    // -----------------------------------------------------------------------
    // Helper methods
    // -----------------------------------------------------------------------

    /**
     * @brief Convert a case-insensitive string name to a Level enum value.
     *
     * Accepts "trace", "debug", "info", "warn", "error", "critical".
     * Returns Level::INFO for unrecognised strings.
     *
     * @param level String representation of the level.
     * @return Corresponding Level enum value.
     */
    static Level levelFromString(const std::string& level);

    /**
     * @brief Convert a Level enum value to its string name.
     * @param level Level to convert.
     * @return Null-terminated string (e.g. "info"), lifetime is static.
     */
    static const char* levelToString(Level level);
};

} // namespace concerns
} // namespace core
} // namespace themis
