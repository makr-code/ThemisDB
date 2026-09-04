/**
 * @file spdlog_logger_adapter.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_logger.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <memory>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <regex>
#include <ctime>
#include <cstdio>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Spdlog adapter implementation of ILogger.
 * 
 * Wraps the existing spdlog-based logger to implement the ILogger interface.
 * When json_mode_ is enabled, logStructured() / logWithContext() emit
 * single-line JSON objects with PII redaction applied to field values.
 * In plain-text mode the adapter preserves the fields as key=value pairs so
 * callers still get structured correlation data without requiring JSON sinks.
 */
class SpdlogLoggerAdapter : public ILogger {
public:
    explicit SpdlogLoggerAdapter(std::shared_ptr<spdlog::logger> logger = nullptr,
                                 bool json_mode = false)
        : logger_(logger ? logger : utils::Logger::get()),
          json_mode_(json_mode) {}

    void log(Level level, const std::string& message) override {
        switch (level) {
            case Level::TRACE: trace(message); break;
            case Level::DEBUG: debug(message); break;
            case Level::INFO: info(message); break;
            case Level::WARN: warn(message); break;
            case Level::ERROR: error(message); break;
            case Level::CRITICAL: critical(message); break;
        }
    }

    void trace(const std::string& message) override {
        if (logger_) {
          logger_->trace(message);
        }
    }

    void debug(const std::string& message) override {
        if (logger_) {
          logger_->debug(message);
        }
    }

    void info(const std::string& message) override {
        if (logger_) {
          logger_->info(message);
        }
    }

    void warn(const std::string& message) override {
        if (logger_) {
          logger_->warn(message);
        }
    }

    void error(const std::string& message) override {
        if (logger_) {
          logger_->error(message);
        }
    }

    void critical(const std::string& message) override {
        if (logger_) {
          logger_->critical(message);
        }
    }

    /**
     * @brief Emit a structured log line.
     *
     * In JSON mode the adapter builds a single-line JSON object with
     * timestamp, level, message, and each field serialized as a property.
     * In plain-text mode the adapter emits the message followed by key=value
     * pairs, still applying redaction to sensitive fields.
     */
    void logStructured(Level level,
                       const std::string& message,
                       const Fields& fields = {}) override {
        if (!logger_) {
          return;
        }
        if (json_mode_) {
            std::string json = buildJsonLine(level, message, fields);
            // Use the raw (no-format) log call to avoid double-escaping
            logger_->log(toSpdlogLevel(level), json);
        } else {
            // Fallback: plain text with key=value pairs
            std::ostringstream oss = {};
            oss << message;
            for (const auto& [k, v] : fields) {
                oss << " " << k << "=" << redact(k, v);
            }
            logger_->log(toSpdlogLevel(level), oss.str());
        }
    }

    /**
     * @brief Emit a structured log line with trace/span/request context.
     *
     * In JSON mode the three correlation IDs are emitted as explicit fields
     * immediately after "message", guaranteeing they appear even when
     * @p fields does not contain them.
     *
     * In plain-text mode a `[trace=…][span=…][req=…]` prefix is prepended to
     * the message so operators can `grep` for a trace-id without a log query
     * language.
     */
    void logWithContext(Level level,
                        const std::string& message,
                        const TraceContext& ctx,
                        const Fields& fields = {}) override {
        if (!logger_) {
          return;
        }
        if (json_mode_) {
            // Merge correlation IDs first so they appear before user fields
            // when iterating a sorted map in buildJsonLine().
            Fields merged = {};
            if (!ctx.trace_id.empty()) {
              merged["trace_id"]   = ctx.trace_id;
            }
            if (!ctx.span_id.empty()) {
              merged["span_id"]    = ctx.span_id;
            }
            if (!ctx.request_id.empty()) {
              merged["request_id"] = ctx.request_id;
            }
            // User-supplied fields may override the above if they share a key.
            merged.insert(fields.begin(), fields.end());
            std::string json = buildJsonLine(level, message, merged);
            logger_->log(toSpdlogLevel(level), json);
        } else {
            // Plain text: prepend correlation prefix for easy grep.
            std::ostringstream oss = {};
            if (!ctx.trace_id.empty())
                oss << "[trace=" << ctx.trace_id << "]";
            if (!ctx.span_id.empty())
                oss << "[span=" << ctx.span_id << "]";
            if (!ctx.request_id.empty())
                oss << "[req=" << ctx.request_id << "]";
            std::string prefix = oss.str();
            std::string full_msg = prefix.empty() ? message
                                                  : prefix + " " + message;
            for (const auto& [k, v] : fields) {
                full_msg += " " + k + "=" + redact(k, v);
            }
            logger_->log(toSpdlogLevel(level), full_msg);
        }
    }

    void setLevel(Level level) override {
        if (!logger_) {
          return;
        }
        logger_->set_level(toSpdlogLevel(level));
    }

    Level getLevel() const override {
        if (!logger_) {
          return Level::INFO;
        }
        switch (logger_->level()) {
            case spdlog::level::trace: return Level::TRACE;
            case spdlog::level::debug: return Level::DEBUG;
            case spdlog::level::info: return Level::INFO;
            case spdlog::level::warn: return Level::WARN;
            case spdlog::level::err: return Level::ERROR;
            case spdlog::level::critical: return Level::CRITICAL;
            default: return Level::INFO;
        }
    }

    void setPattern(const std::string& pattern) override {
        if (logger_) {
          logger_->set_pattern(pattern);
        }
    }

    // Lifecycle hooks
    void flush() noexcept override {
        if (logger_) {
          logger_->flush();
        }
    }

    void shutdown() noexcept override {
        if (logger_) {
            logger_->flush();
            logger_.reset();
        }
    }

    ProbeResult isHealthy() const override {
        if (!logger_) {
            return ProbeResult::unhealthy("logger sink is null");
        }
        return ProbeResult::healthy();
    }

    /**
     * @brief Enable or disable JSON-mode at runtime.
     * @param enabled When true, structured logs are emitted as JSON objects.
     */
    void setJsonMode(bool enabled) { json_mode_ = enabled; }

    /**
     * @brief Return whether JSON-mode is currently enabled.
     */
    bool jsonMode() const { return json_mode_; }

private:
    std::shared_ptr<spdlog::logger> logger_;
    bool json_mode_;

    static spdlog::level::level_enum toSpdlogLevel(Level level) {
        switch (level) {
            case Level::TRACE: return spdlog::level::trace;
            case Level::DEBUG: return spdlog::level::debug;
            case Level::INFO:  return spdlog::level::info;
            case Level::WARN:  return spdlog::level::warn;
            case Level::ERROR: return spdlog::level::err;
            case Level::CRITICAL: return spdlog::level::critical;
        }
        return spdlog::level::info;
    }

    /**
     * @brief JSON-escape a string value.
     */
    static std::string jsonEscape(const std::string& s) {
        std::string out = {};
        out.reserve(s.size() + 4);
        for (unsigned char c : s) {
            switch (c) {
                case '"':  out += "\\\""; break;
                case '\\': out += "\\\\"; break;
                case '\n': out += "\\n";  break;
                case '\r': out += "\\r";  break;
                case '\t': out += "\\t";  break;
                default:
                    if (c < 0x20) {
                        char buf[8];
                        std::snprintf(buf, sizeof(buf), "\\u%04x", c);
                        out += buf;
                    } else {
                        out += static_cast<char>(c);
                    }
            }
        }
        return out;
    }

    /**
     * @brief Redact PII-sensitive field values.
     *
     * Fields whose key contains "password", "secret", "token", "email",
     * "phone", or "ssn" (case-insensitive) are replaced with "[REDACTED]".
     */
    static std::string redact(const std::string& key, const std::string& value) {
        static const std::regex pii_re(
            "password|secret|token|email|phone|ssn|credit_card",
            std::regex::icase);
        if (std::regex_search(key, pii_re)) {
            return "[REDACTED]";
        }
        return value;
    }

    /**
     * @brief Build a single-line JSON log object.
     */
    std::string buildJsonLine(Level level,
                              const std::string& message,
                              const Fields& fields) const {
        // Timestamp in ISO-8601 millisecond precision
        auto now = std::chrono::system_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                       now.time_since_epoch()).count();
        char ts_buf[32];
        std::time_t t = static_cast<std::time_t>(ms / 1000);
        struct tm tm_result{};
#ifdef _WIN32
        gmtime_s(&tm_result, &t);
#else
        gmtime_r(&t, &tm_result);
#endif
        std::strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%S", &tm_result);

        std::ostringstream oss = {};
        oss << "{\"ts\":\"" << ts_buf << '.' 
            << std::setfill('0') << std::setw(3) << (ms % 1000)
            << "Z\",\"level\":\"" << jsonEscape(levelToString(level))
            << "\",\"message\":\"" << jsonEscape(message) << "\"";

        for (const auto& [k, v] : fields) {
            oss << ",\"" << jsonEscape(k) << "\":\"" << jsonEscape(redact(k, v)) << "\"";
        }
        oss << "}";
        return oss.str();
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
