/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_logger.cpp                                       ║
  Version:         0.0.41                                             ║
  Last Modified:   2026-04-14 11:32:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   98.0/100                                       ║
    • Total Lines:     60                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "core/concerns/i_logger.h"
#include <algorithm>
#include <cctype>

namespace themis {
namespace core {
namespace concerns {

ILogger::Level ILogger::levelFromString(const std::string& level) {
    std::string lower = level;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    
    if (lower == "trace") return Level::TRACE;
    if (lower == "debug") return Level::DEBUG;
    if (lower == "info") return Level::INFO;
    if (lower == "warn" || lower == "warning") return Level::WARN;
    if (lower == "error") return Level::ERROR;
    if (lower == "critical" || lower == "fatal") return Level::CRITICAL;
    
    return Level::INFO; // Default
}

const char* ILogger::levelToString(Level level) {
    switch (level) {
        case Level::TRACE: return "TRACE";
        case Level::DEBUG: return "DEBUG";
        case Level::INFO: return "INFO";
        case Level::WARN: return "WARN";
        case Level::ERROR: return "ERROR";
        case Level::CRITICAL: return "CRITICAL";
        default: return "UNKNOWN";
    }
}

} // namespace concerns
} // namespace core
} // namespace themis
