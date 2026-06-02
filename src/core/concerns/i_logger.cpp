/*
 * ThemisDB | File: i_logger.cpp | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 99/100 | Lines: 66
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #2844 feat(core): add Prometheus ... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#include "core/concerns/i_logger.h"

#include <algorithm>
#include <cctype>

namespace themis {
namespace core {
namespace concerns {

ILogger::Level ILogger::levelFromString(const std::string &level) {
    std::string lower = level;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

    if (lower == "trace") {
        return Level::TRACE;
    }
    if (lower == "debug") {
        return Level::DEBUG;
    }
    if (lower == "info") {
        return Level::INFO;
    }
    if (lower == "warn" || lower == "warning") {
        return Level::WARN;
    }
    if (lower == "error") {
        return Level::ERROR;
    }
    if (lower == "critical" || lower == "fatal") {
        return Level::CRITICAL;
    }

    return Level::INFO; // Default
}

const char *ILogger::levelToString(Level level) {
    switch (level) {
        case Level::TRACE:
            return "TRACE";
        case Level::DEBUG:
            return "DEBUG";
        case Level::INFO:
            return "INFO";
        case Level::WARN:
            return "WARN";
        case Level::ERROR:
            return "ERROR";
        case Level::CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
    }
}

} // namespace concerns
} // namespace core
} // namespace themis
