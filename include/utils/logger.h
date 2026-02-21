/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            logger.h                                           ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:30                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   92.0/100                                       ║
    • Total Lines:     153                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#ifdef ERROR
#undef ERROR
#endif

#include "themis_export.h"

#include <spdlog/common.h>

#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <string>

namespace spdlog {
class logger;
}

namespace themis {
namespace utils {

struct LogMetrics {
    std::atomic<uint64_t> trace_count{0};
    std::atomic<uint64_t> debug_count{0};
    std::atomic<uint64_t> info_count{0};
    std::atomic<uint64_t> warn_count{0};
    std::atomic<uint64_t> error_count{0};
    std::atomic<uint64_t> critical_count{0};
    std::atomic<uint64_t> total_count{0};

    struct Snapshot {
        uint64_t trace_count;
        uint64_t debug_count;
        uint64_t info_count;
        uint64_t warn_count;
        uint64_t error_count;
        uint64_t critical_count;
        uint64_t total_count;
    };

    Snapshot snapshot() const {
        return {
            trace_count.load(),
            debug_count.load(),
            info_count.load(),
            warn_count.load(),
            error_count.load(),
            critical_count.load(),
            total_count.load()
        };
    }

    void reset() {
        trace_count = 0;
        debug_count = 0;
        info_count = 0;
        warn_count = 0;
        error_count = 0;
        critical_count = 0;
        total_count = 0;
    }
};

class THEMIS_BASE_API Logger {
public:
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };

    static void init(const std::string& log_file = "vccdb.log", Level level = Level::INFO);
    static void initJson(const std::string& log_file = "vccdb.json.log", Level level = Level::INFO);
    static void initRotating(const std::string& log_file = "vccdb.log",
                             std::size_t max_file_size = 10 * 1024 * 1024,
                             std::size_t max_files = 5,
                             Level level = Level::INFO);

    static void shutdown();
    static std::shared_ptr<spdlog::logger> get();

    static void setLevel(Level level);
    static void setPattern(const std::string& pattern);
    static void setTraceContext(const std::string& trace_id);

    static Level levelFromString(const std::string& lvl);
    static const char* levelToString(Level lvl);

    static const LogMetrics& getMetrics();
    static void resetMetrics();

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
    static LogMetrics metrics_;
    static std::string trace_context_;
    static std::mutex trace_context_mu_;

    static spdlog::level::level_enum toSpdlogLevel(Level level);
};

} // namespace utils
} // namespace themis

#include "utils/logger_impl.h"

#define THEMIS_TRACE(...) ::themis::utils::Logger::trace(__VA_ARGS__)
#define THEMIS_DEBUG(...) ::themis::utils::Logger::debug(__VA_ARGS__)
#define THEMIS_INFO(...) ::themis::utils::Logger::info(__VA_ARGS__)
#define THEMIS_WARN(...) ::themis::utils::Logger::warn(__VA_ARGS__)
#define THEMIS_ERROR(...) ::themis::utils::Logger::error(__VA_ARGS__)
#define THEMIS_CRITICAL(...) ::themis::utils::Logger::critical(__VA_ARGS__)
