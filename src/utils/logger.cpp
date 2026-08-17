/**
 * @file logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=3, L=2
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


// This implementation TU defines Logger symbols, so force export semantics
// to keep declaration/definition DLL attributes consistent in all build modes.
#ifndef THEMIS_BASE_EXPORTS
#define THEMIS_BASE_EXPORTS
#endif

#include "utils/logger.h"
#include "utils/pii_redacting_sink.h"
#include "utils/error_contracts.h"
#include "utils/error_registry.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <iostream>
#include <mutex>
#include <cstdio>

// Windows defines ERROR as a macro; undef it
#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace utils {

LogMetrics Logger::metrics_{};

namespace {
/// Minimal JSON-string escape for embedding a value inside "…".
/// Only escapes characters that would break JSON: backslash and double-quote.
/// Control characters (< 0x20) are replaced with their \uXXXX representation.
std::string jsonEscapeTraceId(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 4);
    for (unsigned char c : s) {
        if (c == '"') {
            out += "\\\"";
        } else if (c == '\\') {
            out += "\\\\";
        } else if (c < 0x20) {
            char buf[8];
            std::snprintf(buf, sizeof(buf), "\\u%04X", static_cast<unsigned>(c));
            out += buf;
        } else {
            out += static_cast<char>(c);
        }
    }
    return out;
}
} // anonymous namespace

// ─────────────────────────────────────────────────────────────────────────────
// Private helper
// ─────────────────────────────────────────────────────────────────────────────

spdlog::level::level_enum Logger::toSpdlogLevel(Level level) {
    switch (level) {
        case Level::TRACE:    return spdlog::level::trace;
        case Level::DEBUG:    return spdlog::level::debug;
        case Level::INFO:     return spdlog::level::info;
        case Level::WARN:     return spdlog::level::warn;
        case Level::ERROR:    return spdlog::level::err;
        case Level::CRITICAL: return spdlog::level::critical;
        default:              return spdlog::level::info;
    }
}

LogMetrics& Logger::metricsStorage() {
    return metrics_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Standard init
// ─────────────────────────────────────────────────────────────────────────────

void Logger::init(const std::string& log_file, Level level) {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);

        auto pii_console_sink = std::make_shared<themis::utils::PIIRedactingSink>(console_sink);
        auto pii_file_sink    = std::make_shared<themis::utils::PIIRedactingSink>(file_sink);

        std::vector<spdlog::sink_ptr> sinks{pii_console_sink, pii_file_sink};
        logger_ = std::make_shared<spdlog::logger>("themis", sinks.begin(), sinks.end());

        logger_->set_level(toSpdlogLevel(level));
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");

        spdlog::set_default_logger(logger_);
        json_mode_ = false;
        logger_->info("Logger initialized");
    } catch (const spdlog::spdlog_ex& ex) {
        // Phase 2.3: Log initialization error with ErrorContext
        ErrorContext err_ctx(
            themis::errors::ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
            fmt::format("Logger initialization failed: {}", ex.what()),
            "Logger::init"
        );
        err_ctx.severity = ErrorSeverity::ERROR;
        err_ctx.is_recoverable = false;
        err_ctx.recovery_hint = "Check log file path and permissions; ensure log directory exists";
        
        std::cerr << "Log initialization failed: " << err_ctx.toJSON() << std::endl;
    } catch (const std::exception &ex) {
        ErrorContext err_ctx(
            themis::errors::ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
            fmt::format("Unexpected error in Logger::init: {}", ex.what()),
            "Logger::init[exception]"
        );
        err_ctx.severity = ErrorSeverity::CRITICAL;
        std::cerr << err_ctx.toJSON() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// JSON-structured init
// ─────────────────────────────────────────────────────────────────────────────

void Logger::initJson(const std::string& log_file, Level level) {
    try {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto file_sink    = std::make_shared<spdlog::sinks::basic_file_sink_mt>(log_file, true);

        auto pii_console_sink = std::make_shared<themis::utils::PIIRedactingSink>(console_sink);
        auto pii_file_sink    = std::make_shared<themis::utils::PIIRedactingSink>(file_sink);

        std::vector<spdlog::sink_ptr> sinks{pii_console_sink, pii_file_sink};
        logger_ = std::make_shared<spdlog::logger>("themis", sinks.begin(), sinks.end());

        logger_->set_level(toSpdlogLevel(level));
        // JSON-structured pattern: every line is a valid JSON object
        logger_->set_pattern(
            R"({"ts":"%Y-%m-%dT%H:%M:%S.%e","logger":"%n","level":"%l","thread":%t,"msg":"%v"})");

        spdlog::set_default_logger(logger_);
        json_mode_ = true;
        logger_->info("JSON logger initialized");
    } catch (const spdlog::spdlog_ex& ex) {
        // Phase 2.3: Log initialization error with ErrorContext
        ErrorContext err_ctx(
            themis::errors::ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
            fmt::format("JSON logger initialization failed: {}", ex.what()),
            "Logger::initJson"
        );
        err_ctx.severity = ErrorSeverity::ERROR;
        err_ctx.is_recoverable = false;
        err_ctx.recovery_hint = "Check JSON log file path and permissions";
        
        std::cerr << "JSON log initialization failed: " << err_ctx.toJSON() << std::endl;
    } catch (const std::exception &ex) {
        ErrorContext err_ctx(
            themis::errors::ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
            fmt::format("Unexpected error in Logger::initJson: {}", ex.what()),
            "Logger::initJson[exception]"
        );
        err_ctx.severity = ErrorSeverity::CRITICAL;
        std::cerr << err_ctx.toJSON() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Rotating-file init
// ─────────────────────────────────────────────────────────────────────────────

void Logger::initRotating(const std::string& log_file,
                           std::size_t max_file_size,
                           std::size_t max_files,
                           Level level) {
    try {
        auto console_sink  = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        auto rotating_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
            log_file, max_file_size, max_files);

        auto pii_console_sink  = std::make_shared<themis::utils::PIIRedactingSink>(console_sink);
        auto pii_rotating_sink = std::make_shared<themis::utils::PIIRedactingSink>(rotating_sink);

        std::vector<spdlog::sink_ptr> sinks{pii_console_sink, pii_rotating_sink};
        logger_ = std::make_shared<spdlog::logger>("themis", sinks.begin(), sinks.end());

        logger_->set_level(toSpdlogLevel(level));
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");

        spdlog::set_default_logger(logger_);
        json_mode_ = false;
        logger_->info("Rotating logger initialized (max_size={}, max_files={})",
                      max_file_size, max_files);
    } catch (const spdlog::spdlog_ex& ex) {
        // Phase 2.3: Log initialization error with ErrorContext
        ErrorContext err_ctx(
            themis::errors::ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
            fmt::format("Rotating logger initialization failed: {}", ex.what()),
            "Logger::initRotating"
        );
        err_ctx.severity = ErrorSeverity::ERROR;
        err_ctx.is_recoverable = false;
        err_ctx.recovery_hint = "Check log file path, permissions, and rotation parameters";
        
        std::cerr << "Rotating log initialization failed: " << err_ctx.toJSON() << std::endl;
    } catch (const std::exception &ex) {
        ErrorContext err_ctx(
            themis::errors::ErrorCode::ERR_AUDIT_LOG_WRITE_FAILED,
            fmt::format("Unexpected error in Logger::initRotating: {}", ex.what()),
            "Logger::initRotating[exception]"
        );
        err_ctx.severity = ErrorSeverity::CRITICAL;
        std::cerr << err_ctx.toJSON() << std::endl;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Shutdown
// ─────────────────────────────────────────────────────────────────────────────

void Logger::shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::shutdown();
        logger_.reset();
        json_mode_ = false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Accessors
// ─────────────────────────────────────────────────────────────────────────────

std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init();
    }
    return logger_;
}

void Logger::setLevel(Level level) {
    if (!logger_) { init(); }
    logger_->set_level(toSpdlogLevel(level));
}

Logger::Level Logger::getLevel() {
    if (!logger_) { init(); }
    switch (logger_->level()) {
        case spdlog::level::trace: return Level::TRACE;
        case spdlog::level::debug: return Level::DEBUG;
        case spdlog::level::info:  return Level::INFO;
        case spdlog::level::warn:  return Level::WARN;
        case spdlog::level::err:   return Level::ERROR;
        case spdlog::level::critical: return Level::CRITICAL;
        default: return Level::INFO;
    }
}

void Logger::setPattern(const std::string& pattern) {
    if (!logger_) { init(); }
    logger_->set_pattern(pattern);
}

void Logger::setTraceContext(const std::string& trace_id) {
    std::lock_guard<std::mutex> lk(trace_context_mu_);
    trace_context_ = trace_id;
    if (!logger_) { return; }
    if (json_mode_) {
        // Keep JSON format; inject trace_id as an additional JSON field when set.
        if (trace_id.empty()) {
            logger_->set_pattern(
                R"({"ts":"%Y-%m-%dT%H:%M:%S.%e","logger":"%n","level":"%l","thread":%t,"msg":"%v"})");
        } else {
            logger_->set_pattern(
                R"({"ts":"%Y-%m-%dT%H:%M:%S.%e","logger":"%n","level":"%l","thread":%t,"trace_id":")" +
                jsonEscapeTraceId(trace_id) +
                R"(","msg":"%v"})");
        }
    } else {
        if (trace_id.empty()) {
            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
        } else {
            logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] [trace:" +
                                 trace_id + "] %v");
        }
    }
}

std::string Logger::getTraceContext() {
    std::lock_guard<std::mutex> lk(trace_context_mu_);
    return trace_context_;
}

// ─────────────────────────────────────────────────────────────────────────────
// Performance metrics
// ─────────────────────────────────────────────────────────────────────────────

const LogMetrics& Logger::getMetrics() {
    return metricsStorage();
}

void Logger::resetMetrics() {
    metricsStorage().reset();
}

// ─────────────────────────────────────────────────────────────────────────────
// Level helpers
// ─────────────────────────────────────────────────────────────────────────────

Logger::Level Logger::levelFromString(const std::string& lvl) {
    std::string s = lvl;
    for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    if (s == "trace")                       return Level::TRACE;
    if (s == "debug")                       return Level::DEBUG;
    if (s == "info")                        return Level::INFO;
    if (s == "warn" || s == "warning")      return Level::WARN;
    if (s == "error" || s == "err")         return Level::ERROR;
    if (s == "critical" || s == "crit")     return Level::CRITICAL;
    return Level::INFO;
}

const char* Logger::levelToString(Level lvl) {
    switch (lvl) {
        case Level::TRACE:    return "trace";
        case Level::DEBUG:    return "debug";
        case Level::INFO:     return "info";
        case Level::WARN:     return "warn";
        case Level::ERROR:    return "error";
        case Level::CRITICAL: return "critical";
        default:              return "info";
    }
}

} // namespace utils
} // namespace themis
