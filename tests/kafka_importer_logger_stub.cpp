/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            kafka_importer_logger_stub.cpp                     ║
  Version:         0.0.13                                             ║
  Last Modified:   2026-04-15 18:51:53                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   85.0/100                                       ║
    • Total Lines:     118                                            ║
    • Open Issues:     TODOs: 0, Stubs: 3                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2421634e7d  2026-03-13  fix(kafka): rewrite tests to use production KafkaImporter... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

/**
 * @file kafka_importer_logger_stub.cpp
 * @brief Minimal no-op stub for Logger static members used by kafka_importer.cpp
 *        when building test_kafka_importer_focused as a standalone binary.
 *
 * kafka_importer.cpp uses the THEMIS_INFO / THEMIS_WARN macros which expand to
 * Logger::info / Logger::warn.  These template methods (in logger_impl.h) check
 * `if (logger_)` before forwarding to spdlog.  Keeping logger_ null here makes
 * every log call a safe no-op – exactly what unit tests need.
 *
 * This stub intentionally avoids including pii_redacting_sink.h and the full
 * PII-detection chain so the focused test binary remains standalone.
 */

// Include only what we need: the Logger class declaration + spdlog types.
#include "utils/logger.h"

namespace themis {
namespace utils {

// ---------------------------------------------------------------------------
// Static member definitions
// ---------------------------------------------------------------------------
std::shared_ptr<spdlog::logger> Logger::logger_;          // null → log calls are no-ops
LogMetrics                      Logger::metrics_;          // default-initialized (atomic counters = 0)
std::string                     Logger::trace_context_;
std::mutex                      Logger::trace_context_mu_;
bool                            Logger::json_mode_   = false;

// ---------------------------------------------------------------------------
// Stub implementations of non-template Logger methods
// ---------------------------------------------------------------------------
spdlog::level::level_enum Logger::toSpdlogLevel(Level) {
    return spdlog::level::info;
}

void Logger::init(const std::string&, Level) {}
void Logger::initJson(const std::string&, Level) {}
void Logger::initRotating(const std::string&, size_t, size_t, Level) {}
void Logger::shutdown() {}

std::shared_ptr<spdlog::logger> Logger::get() { return nullptr; }

void Logger::setLevel(Level) {}

Logger::Level Logger::getLevel() { return Level::INFO; }

void Logger::setPattern(const std::string&) {}

void Logger::setTraceContext(const std::string& id) {
    std::lock_guard<std::mutex> lk(trace_context_mu_);
    trace_context_ = id;
}

std::string Logger::getTraceContext() {
    std::lock_guard<std::mutex> lk(trace_context_mu_);
    return trace_context_;
}

const LogMetrics& Logger::getMetrics() { return metrics_; }

void Logger::resetMetrics() {
    metrics_.trace_count.store(0);
    metrics_.debug_count.store(0);
    metrics_.info_count.store(0);
    metrics_.warn_count.store(0);
    metrics_.error_count.store(0);
    metrics_.critical_count.store(0);
    metrics_.total_count.store(0);
}

Logger::Level Logger::levelFromString(const std::string& lvl) {
    std::string s = lvl;
    for (auto& c : s) c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));
    if (s == "trace")    return Level::TRACE;
    if (s == "debug")    return Level::DEBUG;
    if (s == "warn" || s == "warning") return Level::WARN;
    if (s == "error")    return Level::ERROR;
    if (s == "critical") return Level::CRITICAL;
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
