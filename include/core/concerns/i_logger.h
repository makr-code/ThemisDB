/**
 * @file i_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

struct TraceContext {
    std::string trace_id;    ///< OpenTelemetry trace id (hex string)
    std::string span_id;     ///< OpenTelemetry span id (hex string)
    std::string request_id;  ///< Per-request/RPC correlation id

    bool empty() const noexcept {
        return trace_id.empty() && span_id.empty() && request_id.empty();
    }
};

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

    /**
     * @brief ILogger.
     * @return Return value.
     */
    virtual ~ILogger() = default;

    // -----------------------------------------------------------------------
    // Core logging methods
    // -----------------------------------------------------------------------

    /**
     * @brief Log.
     * @param[in] level Input parameter.
     * @param[in] message Input parameter.
     */
    virtual void log(Level level, const std::string& message) = 0;

    /**
     * @brief Trace.
     * @param[in] message Input parameter.
     */
    virtual void trace(const std::string& message) = 0;

    /**
     * @brief Debug.
     * @param[in] message Input parameter.
     */
    virtual void debug(const std::string& message) = 0;

    /**
     * @brief Info.
     * @param[in] message Input parameter.
     */
    virtual void info(const std::string& message) = 0;

    /**
     * @brief Warn.
     * @param[in] message Input parameter.
     */
    virtual void warn(const std::string& message) = 0;

    /**
     * @brief Error.
     * @param[in] message Input parameter.
     */
    virtual void error(const std::string& message) = 0;

    /**
     * @brief Critical.
     * @param[in] message Input parameter.
     */
    virtual void critical(const std::string& message) = 0;

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

    virtual void logWithContext(Level level,
                                const std::string& message,
                                const TraceContext& ctx,
                                const Fields& fields = {}) {
        Fields merged = fields;
        if (!ctx.trace_id.empty()) {
          merged["trace_id"]   = ctx.trace_id;
        }
        if (!ctx.span_id.empty()) {
          merged["span_id"]    = ctx.span_id;
        }
        if (!ctx.request_id.empty()) {
          merged["request_id"] = ctx.request_id;
        }
        logStructured(level, message, merged);
    }

    // -----------------------------------------------------------------------
    // Configuration methods
    // -----------------------------------------------------------------------

    /**
     * @brief Set Level.
     * @param[in] level Input parameter.
     */
    virtual void setLevel(Level level) = 0;

    [[nodiscard]] virtual Level getLevel() const = 0;

    /**
     * @brief Set Pattern.
     * @param[in] pattern Input parameter.
     */
    virtual void setPattern(const std::string& pattern) = 0;

    // Lifecycle hooks
    virtual void flush() noexcept {}

    virtual void shutdown() noexcept {}

    virtual ProbeResult isHealthy() const { return ProbeResult::healthy(); }

    // -----------------------------------------------------------------------
    // Helper methods
    // -----------------------------------------------------------------------

    /**
     * @brief Level From String.
     * @param[in] level Input parameter.
     * @return Return value.
     */
    static Level levelFromString(const std::string& level);

    /**
     * @brief Level To String.
     * @param[in] level Input parameter.
     * @return Pointer to the result.
     */
    static const char* levelToString(Level level);
};

} // namespace concerns
} // namespace core
} // namespace themis
