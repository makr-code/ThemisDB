/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            logger.cpp                                         ║
  Version:         0.0.25                                             ║
  Last Modified:   2026-02-22 08:22:34                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     234                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "utils/logger.h"
#include "utils/pii_redacting_sink.h"
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <memory>
#include <iostream>
#include <mutex>

// Windows defines ERROR as a macro; undef it
#ifdef ERROR
#undef ERROR
#endif

namespace themis {
namespace utils {

std::shared_ptr<spdlog::logger> Logger::logger_;
LogMetrics Logger::metrics_;
std::string Logger::trace_context_;
std::mutex Logger::trace_context_mu_;

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
        logger_->info("Logger initialized");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Log initialization failed: " << ex.what() << std::endl;
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
        logger_->info("JSON logger initialized");
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "JSON log initialization failed: " << ex.what() << std::endl;
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
        logger_->info("Rotating logger initialized (max_size={}, max_files={})",
                      max_file_size, max_files);
    } catch (const spdlog::spdlog_ex& ex) {
        std::cerr << "Rotating log initialization failed: " << ex.what() << std::endl;
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

void Logger::setPattern(const std::string& pattern) {
    if (!logger_) { init(); }
    logger_->set_pattern(pattern);
}

void Logger::setTraceContext(const std::string& trace_id) {
    std::lock_guard<std::mutex> lk(trace_context_mu_);
    trace_context_ = trace_id;
    if (!logger_) { return; }
    if (trace_id.empty()) {
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] %v");
    } else {
        logger_->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%^%l%$] [thread %t] [trace:" +
                             trace_id + "] %v");
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// Performance metrics
// ─────────────────────────────────────────────────────────────────────────────

const LogMetrics& Logger::getMetrics() {
    return metrics_;
}

void Logger::resetMetrics() {
    metrics_.reset();
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
