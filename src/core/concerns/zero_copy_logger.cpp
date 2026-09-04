/**
 * @file zero_copy_logger.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=4, M=26, L=5
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "core/concerns/zero_copy_logger.h"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <ctime>
#include <spdlog/spdlog.h>

namespace themis {
namespace core {
namespace concerns {

// ---------------------------------------------------------------------------
// Thread-local pre-allocated format buffer
// ---------------------------------------------------------------------------

// One string per OS thread, reserved once, reused across log calls.
// Declared in the anonymous namespace so it's translation-unit-local.
namespace {
thread_local std::string tl_format_buffer;
thread_local bool tl_buffer_initialized = false;
} // namespace

// ---------------------------------------------------------------------------
// Construction
// ---------------------------------------------------------------------------

ZeroCopyLogger::ZeroCopyLogger(std::shared_ptr<spdlog::logger> logger, bool json_mode, std::size_t buffer_capacity)
    : logger_(logger ? std::move(logger) : utils::Logger::get()), json_mode_(json_mode),
      buffer_capacity_(buffer_capacity) {}

// ---------------------------------------------------------------------------
// shouldLog
// ---------------------------------------------------------------------------

bool ZeroCopyLogger::shouldLog(Level level) const noexcept {
    return logger_ && logger_->should_log(toSpdlogLevel(level));
}

// ---------------------------------------------------------------------------
// Zero-copy hot-path methods
// ---------------------------------------------------------------------------

void ZeroCopyLogger::logSV(Level level, std::string_view message) noexcept {
    if (!logger_) {
        return;
    }
    logger_->log(toSpdlogLevel(level), message);
}

void ZeroCopyLogger::traceSV(std::string_view message) noexcept {
    if (logger_) {
        logger_->trace(message);
    }
}

void ZeroCopyLogger::debugSV(std::string_view message) noexcept {
    if (logger_) {
        logger_->debug(message);
    }
}

void ZeroCopyLogger::infoSV(std::string_view message) noexcept {
    if (logger_) {
        logger_->info(message);
    }
}

void ZeroCopyLogger::warnSV(std::string_view message) noexcept {
    if (logger_) {
        logger_->warn(message);
    }
}

void ZeroCopyLogger::errorSV(std::string_view message) noexcept {
    if (logger_) {
        logger_->error(message);
    }
}

void ZeroCopyLogger::criticalSV(std::string_view message) noexcept {
    if (logger_) {
        logger_->critical(message);
    }
}

// ---------------------------------------------------------------------------
// logStructuredSV — zero-copy structured / JSON logging
// ---------------------------------------------------------------------------

void ZeroCopyLogger::logStructuredSV(Level level, std::string_view message,
                                     std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    if (!logger_) {
        return;
    }
    if (!logger_->should_log(toSpdlogLevel(level))) {
        return;
    }

    std::string &buf = formatBuffer();
    buf.clear();

    if (json_mode_.load(std::memory_order_relaxed)) {
        buildJsonInto(buf, level, message, fields);
    } else {
        buildPlainStructuredInto(buf, message, fields);
    }

    logger_->log(toSpdlogLevel(level), buf);
}

// ---------------------------------------------------------------------------
// ILogger overrides
// ---------------------------------------------------------------------------

void ZeroCopyLogger::logStructured(Level level, const std::string &message, const Fields &fields) {
    if (!logger_) {
        return;
    }
    if (!logger_->should_log(toSpdlogLevel(level))) {
        return;
    }

    std::string &buf = formatBuffer();
    buf.clear();

    if (json_mode_.load(std::memory_order_relaxed)) {
        // to avoid a second allocation. We write directly into buf.
        auto now = std::chrono::system_clock::now();
        auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        char ts_buf[32];
        std::time_t t = static_cast<std::time_t>(ms / 1000);
        struct tm tm_result{};
#ifdef _WIN32
        gmtime_s(&tm_result, &t);
#else
        gmtime_r(&t, &tm_result);
#endif
        std::strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%S", &tm_result);

        char ms_str[8];
        std::snprintf(ms_str, sizeof(ms_str), "%03lld", static_cast<long long>(ms % 1000));

        buf += "{\"ts\":\"";
        buf += ts_buf;
        buf += '.';
        buf += ms_str;
        buf += "Z\",\"level\":\"";
        jsonEscapeInto(buf, levelToString(level));
        buf += "\",\"message\":\"";
        jsonEscapeInto(buf, message);
        buf += '"';

        for (const auto &[k, v] : fields) {
            buf += ",\"";
            jsonEscapeInto(buf, k);
            buf += "\":\"";
            if (isPiiKey(k)) {
                buf += "[REDACTED]";
            } else {
                jsonEscapeInto(buf, v);
            }
            buf += '"';
        }
        buf += '}';
    } else {
        buf += message;
        for (const auto &[k, v] : fields) {
            buf += ' ';
            buf += k;
            buf += '=';
            if (isPiiKey(k)) {
                buf += "[REDACTED]";
            } else {
                buf += v;
            }
        }
    }

    logger_->log(toSpdlogLevel(level), buf);
}

// ---------------------------------------------------------------------------
// Configuration
// ---------------------------------------------------------------------------

void ZeroCopyLogger::setLevel(Level level) {
    if (logger_) {
        logger_->set_level(toSpdlogLevel(level));
    }
}

ILogger::Level ZeroCopyLogger::getLevel() const {
    if (!logger_) {
        return Level::INFO;
    }
    switch (logger_->level()) {
        case spdlog::level::trace:
            return Level::TRACE;
        case spdlog::level::debug:
            return Level::DEBUG;
        case spdlog::level::info:
            return Level::INFO;
        case spdlog::level::warn:
            return Level::WARN;
        case spdlog::level::err:
            return Level::ERROR;
        case spdlog::level::critical:
            return Level::CRITICAL;
        default:
            return Level::INFO;
    }
}

void ZeroCopyLogger::setPattern(const std::string &pattern) {
    if (logger_) {
        logger_->set_pattern(pattern);
    }
}

// ---------------------------------------------------------------------------
// Lifecycle
// ---------------------------------------------------------------------------

void ZeroCopyLogger::flush() noexcept {
    if (logger_) {
        logger_->flush();
    }
}

void ZeroCopyLogger::shutdown() noexcept {
    if (logger_) {
        logger_->flush();
        logger_.reset();
    }
}

ProbeResult ZeroCopyLogger::isHealthy() const {
    if (!logger_) {
        return ProbeResult::unhealthy("logger sink is null");
    }
    return ProbeResult::healthy();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

spdlog::level::level_enum ZeroCopyLogger::toSpdlogLevel(Level level) noexcept {
    switch (level) {
        case Level::TRACE:
            return spdlog::level::trace;
        case Level::DEBUG:
            return spdlog::level::debug;
        case Level::INFO:
            return spdlog::level::info;
        case Level::WARN:
            return spdlog::level::warn;
        case Level::ERROR:
            return spdlog::level::err;
        case Level::CRITICAL:
            return spdlog::level::critical;
    }
    return spdlog::level::info;
}

std::string &ZeroCopyLogger::formatBuffer() const noexcept {
    if (!tl_buffer_initialized) {
        tl_format_buffer.reserve(buffer_capacity_);
        tl_buffer_initialized = true;
    }
    return tl_format_buffer;
}

void ZeroCopyLogger::jsonEscapeInto(std::string &out, std::string_view s) {
    // Reserve a conservative lower bound to reduce repeated growth in hot paths.
    out.reserve(static_cast<int>(out.size()) + s.size());
    for (unsigned char c : s) {
        switch (c) {
            case '"':
                out += "\\\"";
                break;
            case '\\':
                out += "\\\\";
                break;
            case '\n':
                out += "\\n";
                break;
            case '\r':
                out += "\\r";
                break;
            case '\t':
                out += "\\t";
                break;
            default:
                if (c < 0x20) {
                    char buf[8]{};
                    std::snprintf(buf, sizeof(buf), "\\u%04x", static_cast<unsigned int>(c));
                    out += buf;
                } else {
                    out += static_cast<char>(c);
                }
        }
    }
}

bool ZeroCopyLogger::isPiiKey(std::string_view key) noexcept {
    // Case-insensitive substring search for known PII field name tokens.
    // Avoids std::regex to stay allocation-free on the hot path.
    static constexpr std::string_view kPiiTokens[]
        = {"password", "secret", "token", "email", "phone", "ssn", "credit_card"};

    // Build a lowercase copy of the key (stack buffer for keys ≤ 128 bytes).
    char lower_buf[128]{};
    const std::size_t n = key.size() < sizeof(lower_buf) - 1 ?static_cast<int>(key.size()) : sizeof(lower_buf) - 1;
    for (std::size_t i = 0; i < n; ++i) {
        const auto ch = static_cast<unsigned char>(key[i]);
        lower_buf[i] = static_cast<char>((ch >= 'A' && ch <= 'Z') ? (ch | 0x20) : ch);
    }
    lower_buf[n] = '\0';
    std::string_view lower_key(lower_buf, n);

    for (const auto &token : kPiiTokens) {
        if (lower_key.find(token) != std::string_view::npos) {
            return true;
        }
    }
    return false;
}

void ZeroCopyLogger::buildJsonInto(std::string &buf, Level level, std::string_view message,
                                   std::initializer_list<std::pair<std::string_view, std::string_view>> fields) const {
    auto now = std::chrono::system_clock::now();
    auto ms  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    char ts_buf[32];
    std::time_t t = static_cast<std::time_t>(ms / 1000);
    struct tm tm_result{};
#ifdef _WIN32
    gmtime_s(&tm_result, &t);
#else
    gmtime_r(&t, &tm_result);
#endif
    std::strftime(ts_buf, sizeof(ts_buf), "%Y-%m-%dT%H:%M:%S", &tm_result);

    char ms_str[8];
    std::snprintf(ms_str, sizeof(ms_str), "%03lld", static_cast<long long>(ms % 1000));

    buf += "{\"ts\":\"";
    buf += ts_buf;
    buf += '.';
    buf += ms_str;
    buf += "Z\",\"level\":\"";
    jsonEscapeInto(buf, levelToString(level));
    buf += "\",\"message\":\"";
    jsonEscapeInto(buf, message);
    buf += '"';

    for (const auto &[k, v] : fields) {
        buf += ",\"";
        jsonEscapeInto(buf, k);
        buf += "\":\"";
        if (isPiiKey(k)) {
            buf += "[REDACTED]";
        } else {
            jsonEscapeInto(buf, v);
        }
        buf += '"';
    }
    buf += '}';
}

void ZeroCopyLogger::buildPlainStructuredInto(
    std::string &buf, std::string_view message,
    std::initializer_list<std::pair<std::string_view, std::string_view>> fields) {
    buf += message;
    for (const auto &[k, v] : fields) {
        buf += ' ';
        buf += k;
        buf += '=';
        if (isPiiKey(k)) {
            buf += "[REDACTED]";
        } else {
            buf += v;
        }
    }
}

} // namespace concerns
} // namespace core
} // namespace themis
