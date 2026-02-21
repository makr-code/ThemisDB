#pragma once

// Windows compatibility - undef macros that conflict with Logger::Level
#ifdef ERROR
#undef ERROR
#endif

#include "themis_export.h"
#include <string>
#include <memory>
#include <atomic>
#include <cstdint>

namespace spdlog { class logger; }

namespace themis {
namespace utils {

/**
 * @brief Log performance metrics – counts per level since last reset.
 */
struct LogMetrics {
    std::atomic<uint64_t> trace_count{0};
    std::atomic<uint64_t> debug_count{0};
    std::atomic<uint64_t> info_count{0};
    std::atomic<uint64_t> warn_count{0};
    std::atomic<uint64_t> error_count{0};
    std::atomic<uint64_t> critical_count{0};
    std::atomic<uint64_t> total_count{0};

    // Non-copyable due to atomics – provide a snapshot helper
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
        return {trace_count.load(), debug_count.load(), info_count.load(),
                warn_count.load(), error_count.load(), critical_count.load(),
                total_count.load()};
    }
    void reset() {
        trace_count = debug_count = info_count = warn_count =
            error_count = critical_count = total_count = 0;
    }
};

class THEMIS_BASE_API Logger {
public:
    enum class Level { TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL };

    // -----------------------------------------------------------------------
    // Initialisation variants
    // -----------------------------------------------------------------------

    /// Standard init: console + plain-text file sink with PII redaction.
    static void init(const std::string& log_file = "vccdb.log", Level level = Level::INFO);

    /// JSON-structured init: console + JSON-formatted file sink.
    /// Each log line is emitted as a JSON object (timestamp, level, message).
    static void initJson(const std::string& log_file = "vccdb.json.log",
                         Level level = Level::INFO);

    /// Rotating file init: creates a size-limited, rotating file sink.
    /// @param log_file       Base log-file path (suffixes are appended for rotated files).
    /// @param max_file_size  Maximum size of a single log file in bytes (default 10 MiB).
    /// @param max_files      Number of rotated files to retain (default 5).
    static void initRotating(const std::string& log_file = "vccdb.log",
                              std::size_t max_file_size = 10 * 1024 * 1024,
                              std::size_t max_files = 5,
                              Level level = Level::INFO);

    static void shutdown();
    static std::shared_ptr<spdlog::logger> get();

    // -----------------------------------------------------------------------
    // Runtime controls
    // -----------------------------------------------------------------------

    static void setLevel(Level level);
    static void setPattern(const std::string& pattern);

    /// Inject a trace-context ID into every subsequent log message prefix.
    /// Pass an empty string to clear the injected context.
    static void setTraceContext(const std::string& trace_id);

    // -----------------------------------------------------------------------
    // Helpers
    // -----------------------------------------------------------------------

    static Level     levelFromString(const std::string& lvl);
    static const char* levelToString(Level lvl);

    // -----------------------------------------------------------------------
    // Performance metrics
    // -----------------------------------------------------------------------

    /// Return a const reference to the live log-metrics counters.
    static const LogMetrics& getMetrics();
    /// Reset all log-metrics counters to zero.
    static void resetMetrics();

    // -----------------------------------------------------------------------
    // Logging methods (template, defined in logger_impl.h)
    // -----------------------------------------------------------------------

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
    static std::string trace_context_; ///< Optional trace-ID prefix for log correlation
    static std::mutex trace_context_mu_;

    static spdlog::level::level_enum toSpdlogLevel(Level level);
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
