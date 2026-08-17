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

    /**
     * @brief Initialize logger with file sink
     * 
     * Creates a standard file logger that writes log entries to a single file.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param log_file Path to output log file (default: "vccdb.log")
     * @param level Logging level threshold (default: INFO)
     * 
     * @return void
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309):**
     * - ERR_AUDIT_LOG_WRITE_FAILED (7301): Cannot create/write to log file
     *   - Recovery: Falls back to stderr logging; non-fatal
     *   - Severity: WARNING
     *   - User Action: Check file path, permissions, and disk space
     * 
     * - ERR_AUDIT_PERMISSION_DENIED (7305): Insufficient permissions on log directory
     *   - Recovery: Logs to stderr instead
     *   - Severity: WARNING
     *   - User Action: Verify write permissions on log directory
     * 
     * - ERR_AUDIT_DISK_FULL (7306): Insufficient disk space for log file
     *   - Recovery: Falls back to stderr; disk space error is logged
     *   - Severity: WARNING
     *   - User Action: Free up disk space before reinitializing
     * 
     * **Initialization Semantics:**
     * - Replaces previous logger instance if already initialized
     * - Graceful degradation: all errors logged but initialization proceeds
     * - Stderr fallback always available if file logging fails
     * 
     * @thread_safety Thread-safe; previous logger instance is flushed before replacement
     * @performance O(1) initialization; first log call may incur file creation overhead
     * 
     * @see ErrorCode for complete error taxonomy
     * @see initJson() for JSON format logging
     * @see initRotating() for automatic log rotation
     */
    static void init(const std::string& log_file = "vccdb.log", Level level = Level::INFO);
    
    /**
     * @brief Initialize logger with JSON-formatted file sink
     * 
     * Creates a JSON file logger that writes structured JSON log entries.
     * Useful for log aggregation systems and automated parsing.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param log_file Path to output JSON log file (default: "vccdb.json.log")
     * @param level Logging level threshold (default: INFO)
     * 
     * @return void
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309):**
     * - ERR_AUDIT_LOG_WRITE_FAILED (7301): Cannot create/write to JSON log file
     *   - Recovery: Falls back to stderr logging; JSON formatting disabled
     *   - Severity: WARNING
     *   - User Action: Check file path, permissions, and disk space
     * 
     * - ERR_AUDIT_SERIALIZATION_FAILED (7302): JSON serialization error
     *   - Recovery: Logs entry as plain text instead
     *   - Severity: WARNING
     *   - User Action: Check for invalid characters in log messages
     * 
     * - ERR_AUDIT_PERMISSION_DENIED (7305): Insufficient permissions on log directory
     *   - Recovery: Falls back to stderr; JSON format is preserved
     *   - Severity: WARNING
     *   - User Action: Verify write permissions on log directory
     * 
     * **JSON Format Semantics:**
     * - Each log entry is a complete JSON object with: timestamp, level, message, context
     * - Structured output suitable for log aggregation (ELK, Splunk, CloudWatch)
     * - Invalid JSON in messages is escaped appropriately
     * 
     * @thread_safety Thread-safe; previous logger instance is flushed before replacement
     * @performance O(1) initialization; first JSON serialization incurs format overhead
     * 
     * @see ErrorCode for complete error taxonomy
     * @see init() for plain-text file logging
     * @see initRotating() for automatic log rotation with JSON format
     */
    static void initJson(const std::string& log_file = "vccdb.json.log", Level level = Level::INFO);
    
    /**
     * @brief Initialize logger with rotating file sink
     * 
     * Creates a file logger that automatically rotates log files when they reach
     * a maximum size. Old files are renamed with sequence numbers and oldest files
     * are pruned according to max_files limit.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param log_file Path to output log file (default: "vccdb.log")
     * @param max_file_size Maximum size per file before rotation (default: 10MB)
     * @param max_files Maximum number of rotated files to keep (default: 5)
     * @param level Logging level threshold (default: INFO)
     * 
     * @return void
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309):**
     * - ERR_AUDIT_LOG_WRITE_FAILED (7301): Cannot create/rotate log files
     *   - Recovery: Falls back to single-file mode (no rotation)
     *   - Severity: WARNING
     *   - User Action: Check file path, permissions, and disk space
     * 
     * - ERR_AUDIT_ROTATION_FAILED (7307): Log rotation failed (file rename/delete)
     *   - Recovery: Continues with existing file; rotation retried on next threshold
     *   - Severity: WARNING
     *   - User Action: Check file system permissions for old files
     * 
     * - ERR_AUDIT_PERMISSION_DENIED (7305): Insufficient permissions for rotation
     *   - Recovery: Falls back to single-file mode; newest entries may overwrite old
     *   - Severity: WARNING
     *   - User Action: Verify write permissions on log directory
     * 
     * - ERR_AUDIT_DISK_FULL (7306): Insufficient disk space for rotation
     *   - Recovery: Attempts cleanup; if unsuccessful, continues without rotation
     *   - Severity: WARNING
     *   - User Action: Free up disk space and increase cleanup retention policy
     * 
     * **Rotation Semantics:**
     * - Files are rotated when size >= max_file_size (checked at log write time)
     * - Old files are renamed: vccdb.1.log, vccdb.2.log, ... vccdb.max_files.log
     * - Files beyond max_files count are deleted (FIFO cleanup)
     * - Rotation is atomic; partial rotations are not possible
     * - Graceful degradation: if rotation fails, logging continues without rotation
     * 
     * @bounded_resources
     * - Total disk space: approximately max_file_size * max_files bytes
     * - Rotation cost: O(max_files) for file rename/delete operations
     * - Watermarks: cleanup triggered when sum exceeds max_file_size * (max_files - 1)
     * 
     * @thread_safety Thread-safe; rotation is protected by logger lock
     * @performance O(1) per log call; O(max_files) on rotation event
     * 
     * @see ErrorCode for complete error taxonomy
     * @see init() for single-file logging
     * @see initJson() for JSON-formatted rotating logs
     */
    static void initRotating(const std::string& log_file = "vccdb.log",
                             std::size_t max_file_size = 10 * 1024 * 1024,
                             std::size_t max_files = 5,
                             Level level = Level::INFO);

    /**
     * @brief Shutdown logger and flush pending messages
     * 
     * Closes all log sinks and flushes pending messages to disk.
     * Safe to call even if logger not initialized.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @return void
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309):**
     * - ERR_AUDIT_LOG_WRITE_FAILED (7301): Flush to file fails
     *   - Recovery: Attempts stderr flush; non-fatal
     *   - Severity: WARNING
     *   - User Action: Check disk space and file permissions
     * 
     * - ERR_AUDIT_CLEANUP_FAILED (7309): Logger cleanup fails
     *   - Recovery: Proceeds with cleanup; stderr remains available
     *   - Severity: WARNING
     *   - User Action: Check for resource leaks in sinks
     * 
     * **Shutdown Semantics:**
     * - All pending log entries flushed before shutdown
     * - All sinks are closed and released
     * - Thread-safe: subsequent log calls fail gracefully (logged to fallback)
     * - Idempotent: safe to call multiple times
     * 
     * @thread_safety Thread-safe; protects against concurrent log calls
     * @performance O(pending_messages) for final flush
     * 
     * @note Should be called during application shutdown
     * @see ErrorCode for complete error taxonomy
     */
    static void shutdown();
     
    static std::shared_ptr<spdlog::logger> get();

    /**
     * @brief Set logging level threshold
     * 
     * Changes the logging level for all subsequent log calls.
     * Levels below the threshold are silently discarded.
     * Phase 2.10: Comprehensive error contract documentation.
     * 
     * @param level Threshold level (TRACE, DEBUG, INFO, WARN, ERROR, CRITICAL)
     * 
     * @return void
     * 
     * @error_contract
     * **Phase 2.3 Error Codes (7300-7309):**
     * No error codes; operation always succeeds.
     * - Level changes are atomic (single write under mutex)
     * - In-flight log calls use previous level value
     * - Level change is non-blocking
     * 
     * **Level Semantics:**
     * - TRACE: Most verbose; includes debug flow tracking (1% sample rate default)
     * - DEBUG: Development-level diagnostics (10% sample rate default)
     * - INFO: General informational messages (100% logged)
     * - WARN: Warning conditions and recoverable errors (100% logged)
     * - ERROR: Error conditions and failures (100% logged)
     * - CRITICAL: Critical errors and system failures (100% logged)
     * 
     * @thread_safety Thread-safe; uses atomic write under mutex
     * @performance O(1) operation; no I/O overhead
     * 
     * @see Level for available levels
     * @see getLevel() to query current level
     * @see SampledLogger for rate-limiting at high volume
     */
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

    // Backwards-compatible capitalized aliases used across the codebase.
    template<typename FormatString, typename... Args>
    static void Trace(FormatString&& fmt, Args&&... args) { trace(std::forward<FormatString>(fmt), std::forward<Args>(args)...); }

    template<typename FormatString, typename... Args>
    static void Debug(FormatString&& fmt, Args&&... args) { debug(std::forward<FormatString>(fmt), std::forward<Args>(args)...); }

    template<typename FormatString, typename... Args>
    static void Info(FormatString&& fmt, Args&&... args) { info(std::forward<FormatString>(fmt), std::forward<Args>(args)...); }

    template<typename FormatString, typename... Args>
    static void Warn(FormatString&& fmt, Args&&... args) { warn(std::forward<FormatString>(fmt), std::forward<Args>(args)...); }

    template<typename FormatString, typename... Args>
    static void Error(FormatString&& fmt, Args&&... args) { error(std::forward<FormatString>(fmt), std::forward<Args>(args)...); }

    template<typename FormatString, typename... Args>
    static void Critical(FormatString&& fmt, Args&&... args) { critical(std::forward<FormatString>(fmt), std::forward<Args>(args)...); }

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
