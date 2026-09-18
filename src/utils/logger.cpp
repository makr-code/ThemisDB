/**
 * @file logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
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
/**
 * @brief Json Escape Trace Id.
 * @param[in] s Input parameter.
 * @return Return value.
 * @details Calls: reserve(), size(), std::snprintf().
 */
std::string jsonEscapeTraceId(const std::string& s) {
    std::string out = {};
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


/**
 * @brief To Spdlog Level.
 * @param[in] level Input parameter.
 * @return Return value.
 * @details Implements toSpdlogLevel without additional internal calls.
 */
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

/**
 * @brief Metrics Storage.
 * @return Return value.
 * @details Implements metricsStorage without additional internal calls.
 */
LogMetrics& Logger::metricsStorage() {
    return metrics_;
}


/**
 * @brief Init.
 * @param[in] log_file Input parameter.
 * @param[in] level Input parameter.
 * @details Calls: begin(), end(), set_level(), toSpdlogLevel(), set_pattern(), spdlog::set_default_logger(), info(), themis::utils::makeErrorContext().
 */
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
        // Fail-open: degrade to stderr so callers are never silently unlogged.
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::LOG_INITIALIZATION_FAILED,
            "spdlog sink initialization failed – falling back to stderr; log_file=" +
                log_file + "; error=" + ex.what(),
            "Logger::init",
            themis::utils::ErrorSeverity::Error,
            true);
        themis::utils::logErrorWithContext(ctx);
        std::cerr << "[Logger::init] Log initialization failed (sink unavailable): "
                  << ex.what() << " – falling back to stderr\n";
    }
}


/**
 * @brief Init Json.
 * @param[in] log_file Input parameter.
 * @param[in] level Input parameter.
 * @details Calls: begin(), end(), set_level(), toSpdlogLevel(), set_pattern(), spdlog::set_default_logger(), info(), themis::utils::makeErrorContext().
 */
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
        // Fail-open: emit a structured diagnostic and fall back to stderr.
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::LOG_INITIALIZATION_FAILED,
            "spdlog JSON sink initialization failed – falling back to stderr; log_file=" +
                log_file + "; error=" + ex.what(),
            "Logger::initJson",
            themis::utils::ErrorSeverity::Error,
            true);
        themis::utils::logErrorWithContext(ctx);
        std::cerr << "[Logger::initJson] JSON log initialization failed: "
                  << ex.what() << " – falling back to stderr\n";
    }
}


/**
 * @brief Init Rotating.
 * @param[in] log_file Input parameter.
 * @param[in] max_file_size Input parameter.
 * @param[in] max_files Input parameter.
 * @param[in] level Input parameter.
 * @details Calls: begin(), end(), set_level(), toSpdlogLevel(), set_pattern(), spdlog::set_default_logger(), info(), themis::utils::makeErrorContext().
 */
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
        // Fail-open: emit structured diagnostic and fall back to stderr.
        auto ctx = themis::utils::makeErrorContext(
            themis::utils::ErrorCode::LOG_INITIALIZATION_FAILED,
            "spdlog rotating sink initialization failed – falling back to stderr; log_file=" +
                log_file + "; error=" + ex.what(),
            "Logger::initRotating",
            themis::utils::ErrorSeverity::Error,
            true);
        themis::utils::logErrorWithContext(ctx);
        std::cerr << "[Logger::initRotating] Rotating log initialization failed: "
                  << ex.what() << " – falling back to stderr\n";
    }
}


/**
 * @brief Shutdown.
 * @details Calls: flush(), reset().
 */
void Logger::shutdown() {
    if (logger_) {
        logger_->flush();
        spdlog::shutdown();
        logger_.reset();
        json_mode_ = false;
    }
}


/**
 * @brief Get.
 * @return Return value.
 * @details Calls: init().
 */
std::shared_ptr<spdlog::logger> Logger::get() {
    if (!logger_) {
        init();
    }
    return logger_;
}

/**
 * @brief Set Level.
 * @param[in] level Input parameter.
 * @details Calls: init(), set_level(), toSpdlogLevel().
 */
void Logger::setLevel(Level level) {
    if (!logger_) { init(); }
    logger_->set_level(toSpdlogLevel(level));
}

/**
 * @brief Get Level.
 * @return Return value.
 * @details Calls: init(), level().
 */
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

/**
 * @brief Set Pattern.
 * @param[in] pattern Input parameter.
 * @details Calls: init(), set_pattern().
 */
void Logger::setPattern(const std::string& pattern) {
    if (!logger_) { init(); }
    logger_->set_pattern(pattern);
}

/**
 * @brief Set Trace Context.
 * @param[in] trace_id Identifier of the trace.
 * @details Calls: lk(), empty(), set_pattern(), jsonEscapeTraceId().
 */
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

/**
 * @brief Get Trace Context.
 * @return Return value.
 * @details Calls: lk().
 */
std::string Logger::getTraceContext() {
    std::lock_guard<std::mutex> lk(trace_context_mu_);
    return trace_context_;
}


/**
 * @brief Get Metrics.
 * @return Return value.
 * @details Calls: metricsStorage().
 */
const LogMetrics& Logger::getMetrics() {
    return metricsStorage();
}

/**
 * @brief Reset Metrics.
 * @details Calls: metricsStorage(), reset().
 */
void Logger::resetMetrics() {
    metricsStorage().reset();
}


/**
 * @brief Level From String.
 * @param[in] lvl Input parameter.
 * @return Return value.
 * @details Calls: tolower().
 */
Logger::Level Logger::levelFromString(const std::string& lvl) {
    std::string s = lvl;
    for (auto& c : s) {
      c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    }
    if (s == "trace") {
      return Level::TRACE;
    }
    if (s == "debug") {
      return Level::DEBUG;
    }
    if (s == "info") {
      return Level::INFO;
    }
    if (s == "warn" || s == "warning") {
      return Level::WARN;
    }
    if (s == "error" || s == "err") {
      return Level::ERROR;
    }
    if (s == "critical" || s == "crit") {
      return Level::CRITICAL;
    }
    return Level::INFO;
}

/**
 * @brief Level To String.
 * @param[in] lvl Input parameter.
 * @return Pointer to the result.
 * @details Implements levelToString without additional internal calls.
 */
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
