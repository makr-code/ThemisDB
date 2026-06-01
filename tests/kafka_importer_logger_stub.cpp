/*
 * ThemisDB | File: kafka_importer_logger_stub.cpp | Version: 0.0.13
 * Maturity: 🟢 PRODUCTION-READY | Score: 93/100
 * Gap Summary: total=6; TODO=1, Stub=4, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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

// This TU defines Logger static members for a focused test binary.
// Force export mode so definitions are valid on Windows.
#if defined(_WIN32) && !defined(THEMIS_BASE_EXPORTS)
#define THEMIS_BASE_EXPORTS
#endif

// Include only what we need: the Logger class declaration + spdlog types.
#include "utils/logger.h"

namespace themis {
namespace utils {

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
