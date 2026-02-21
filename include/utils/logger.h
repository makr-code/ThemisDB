/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            logger.h                                           ║
  Version:         0.0.2                                              ║
  Last Modified:   2026-02-21 07:18:11                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   97.0/100                                       ║
    • Total Lines:     65                                             ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿#pragma once

// Windows compatibility - undef macros that conflict with Logger::Level
#ifdef ERROR
#undef ERROR
#endif

#include "themis_export.h"
#include <string>
#include <memory>

namespace spdlog { class logger; }

namespace themis {
namespace utils {

class THEMIS_BASE_API Logger {
public:
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };
    
    static void init(const std::string& log_file = "vccdb.log", Level level = Level::INFO);
    static void shutdown();
    static std::shared_ptr<spdlog::logger> get();
    // Runtime controls
    static void setLevel(Level level);
    static void setPattern(const std::string& pattern);
    // Helper to convert from string to Level; returns INFO on unknown
    static Level levelFromString(const std::string& lvl);
    static const char* levelToString(Level lvl);
    
    template<typename FormatString, typename... Args>
    static void trace(FormatString&& fmt, Args&&... args);
    
    template<typename FormatString, typename... Args>
    static void debug(FormatString&& fmt, Args&&... args);
    
    template<typename FormatString, typename... Args>
    static void info(FormatString&& fmt, Args&&... args);
    
    template<typename FormatString, typename... Args>
    static void warn(FormatString&& fmt, Args&&... args);
    
    template<typename FormatString, typename... Args>
    static void error(FormatString&& fmt, Args&&... args);
    
    template<typename FormatString, typename... Args>
    static void critical(FormatString&& fmt, Args&&... args);
    
private:
    static std::shared_ptr<spdlog::logger> logger_;
};

} // namespace utils
} // namespace themis

// Include implementation
#include "utils/logger_impl.h"

// Logging macros
#define THEMIS_TRACE(...) ::themis::utils::Logger::trace(__VA_ARGS__)
#define THEMIS_DEBUG(...) ::themis::utils::Logger::debug(__VA_ARGS__)
#define THEMIS_INFO(...) ::themis::utils::Logger::info(__VA_ARGS__)
#define THEMIS_WARN(...) ::themis::utils::Logger::warn(__VA_ARGS__)
#define THEMIS_ERROR(...) ::themis::utils::Logger::error(__VA_ARGS__)
#define THEMIS_CRITICAL(...) ::themis::utils::Logger::critical(__VA_ARGS__)
