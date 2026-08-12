/**
 * @file logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
#include <random>
#include <string>
#include <unordered_map>

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

/** @brief Logger. */
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
    static Level getLevel();
    static void setPattern(const std::string& pattern);
    static void setTraceContext(const std::string& trace_id);
    static std::string getTraceContext();

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
    inline static std::shared_ptr<spdlog::logger> logger_{};
    static LogMetrics metrics_;
    inline static std::string trace_context_{};
    inline static std::mutex trace_context_mu_{};
    inline static bool json_mode_ = false;

    static LogMetrics& metricsStorage();
    static spdlog::level::level_enum toSpdlogLevel(Level level);
};

// ---------------------------------------------------------------------------
// SampledLogger
// ---------------------------------------------------------------------------

/**
 * @brief Configuration for sampled logging.
 */
struct SampledLoggerConfig {
    double trace_sample_rate = 0.01;  ///< 1%
    double debug_sample_rate = 0.10;  ///< 10%
    double info_sample_rate  = 1.00;  ///< 100%
    double warn_sample_rate  = 1.00;  ///< 100%
    double error_sample_rate = 1.00;  ///< 100%
    double burst_rate = 100.0;        ///< token-bucket rate (calls/s per site)
    double burst_size = 10.0;         ///< initial token budget
};

/**
 * @brief Logger decorator that samples and rate-limits high-frequency log calls.
 *
 * Uses per-(file, line, level) token-bucket rate limiters to prevent log flooding.
 * Suppressed call counts are tracked for observability.
 * Audit log calls (via AuditLogger) MUST bypass this decorator.
 *
 * v1.5.0: Initial implementation
 */
class THEMIS_BASE_API SampledLogger {
public:
    explicit SampledLogger(std::shared_ptr<Logger> underlying,
                           SampledLoggerConfig cfg = {});
    ~SampledLogger();

    /// Log with sampling; file/line identify the call-site bucket.
    void log(Logger::Level level, const std::string& msg,
             const char* file = __builtin_FILE(),
             int line = __builtin_LINE());

    /// Total suppressed calls since construction.
    uint64_t suppressed_total() const;

    /// Reset suppression counters (for tests).
    void reset_stats();

    /// Hot-reload config at runtime.
    void set_config(SampledLoggerConfig cfg);

private:
    struct Bucket;

    bool should_log(Logger::Level level, const char* file, int line);

    std::shared_ptr<Logger>  underlying_;
    SampledLoggerConfig      cfg_;
    std::atomic<uint64_t>    suppressed_{0};
    mutable std::mutex       buckets_mutex_;
    std::unordered_map<std::string, std::unique_ptr<Bucket>> buckets_;
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
