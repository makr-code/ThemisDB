/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            zero_copy_logger.h                                 ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 18:02:27                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     260                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 3279a3a190  2026-03-13  fix(core): audit fixes — atomic json_mode_, concurrent se... ║
    • c69bf14be2  2026-03-13  feat(core): Zero-Copy Logging — ZeroCopyLogger with strin... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_logger.h"
#include "utils/logger.h"
#include <spdlog/spdlog.h>
#include <atomic>
#include <memory>
#include <string_view>
#include <cstddef>
#include <utility>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief High-performance ILogger implementation with zero-copy string_view API.
 *
 * Reduces memory allocations in logging hot paths through two mechanisms:
 *
 *  1. **string_view hot-path API** (`logSV`, `traceSV`, `infoSV`, …):
 *     Accepts `std::string_view` directly and forwards to spdlog without
 *     constructing an intermediate `std::string`.  Callers that already hold
 *     a string literal, `std::string_view`, or `std::string` pay zero copy
 *     overhead for message dispatch.
 *
 *  2. **Pre-allocated thread-local format buffer**: The structured and JSON
 *     log paths (`logStructuredSV`, JSON mode) reuse a `std::string` buffer
 *     that is reserved once per thread at `kDefaultBufferCapacity` bytes.
 *     Subsequent calls `clear()` the buffer — retaining its heap capacity —
 *     so typical log lines cause no heap allocation on the hot path.
 *
 *  3. **Early level-check** (`shouldLog`): callers can guard expensive
 *     message construction behind a level check at zero virtual-dispatch cost.
 *
 * **Expected improvement:** 30–50 % reduction in logging overhead compared
 * to `SpdlogLoggerAdapter` for mixed plain-text / structured workloads.
 *
 * ## Thread safety
 * All public methods are thread-safe.  Each calling thread owns a distinct
 * thread-local format buffer, so concurrent log calls never contend on buffer
 * access.
 *
 * ## Backward compatibility
 * `ZeroCopyLogger` fully implements `ILogger`, so it can be used anywhere an
 * `ILogger` pointer or reference is expected.  The `const std::string&`
 * virtual overrides delegate to the `string_view` hot-path, so callers that
 * pass `std::string` objects pay at most a `string_view` construction (no
 * copy).
 *
 * ## Usage example
 * ```cpp
 * auto logger = std::make_unique<ZeroCopyLogger>(spdlog_logger);
 *
 * // Zero-copy hot path (string_view / literal):
 * logger->infoSV("Query executed");
 * logger->logStructuredSV(ILogger::Level::INFO, "insert done",
 *     {{"table", "users"}, {"rows", "1"}});
 *
 * // Guard expensive formatting behind level check:
 * if (logger->shouldLog(ILogger::Level::DEBUG)) {
 *     logger->debugSV(buildExpensiveDebugMessage());
 * }
 *
 * // Compatible with ILogger* / std::string callers (no extra copy):
 * ILogger* ilog = logger.get();
 * ilog->info("startup complete");
 * ```
 */
class ZeroCopyLogger : public ILogger {
public:
    /// Capacity (bytes) reserved in the thread-local format buffer on first
    /// use.  The buffer is never shrunk; messages longer than this capacity
    /// trigger a single reallocation on that call.
    static constexpr std::size_t kDefaultBufferCapacity = 4096;

    /**
     * @param logger           Underlying spdlog logger.  Defaults to the
     *                         global ThemisDB logger when nullptr.
     * @param json_mode        When true, `logStructuredSV` / `logStructured`
     *                         emit single-line JSON objects with PII redaction.
     * @param buffer_capacity  Initial reservation for the per-thread format
     *                         buffer.  Defaults to `kDefaultBufferCapacity`.
     */
    explicit ZeroCopyLogger(std::shared_ptr<spdlog::logger> logger = nullptr,
                            bool json_mode = false,
                            std::size_t buffer_capacity = kDefaultBufferCapacity);

    // =========================================================================
    // Zero-copy string_view hot-path API
    // =========================================================================

    /**
     * @brief Check whether the given level would be emitted.
     *
     * Use this to guard expensive message construction:
     * ```cpp
     * if (logger.shouldLog(ILogger::Level::DEBUG))
     *     logger.debugSV(computeExpensiveString());
     * ```
     */
    bool shouldLog(Level level) const noexcept;

    /// @brief Emit a log record at @p level without copying @p message.
    void logSV(Level level, std::string_view message) noexcept;

    /// @brief Log at TRACE level without copying @p message.
    void traceSV(std::string_view message) noexcept;

    /// @brief Log at DEBUG level without copying @p message.
    void debugSV(std::string_view message) noexcept;

    /// @brief Log at INFO level without copying @p message.
    void infoSV(std::string_view message) noexcept;

    /// @brief Log at WARN level without copying @p message.
    void warnSV(std::string_view message) noexcept;

    /// @brief Log at ERROR level without copying @p message.
    void errorSV(std::string_view message) noexcept;

    /// @brief Log at CRITICAL level without copying @p message.
    void criticalSV(std::string_view message) noexcept;

    /**
     * @brief Emit a structured log line using `string_view` fields.
     *
     * In JSON mode the output is a single-line JSON object:
     * ```json
     * {"ts":"…","level":"INFO","message":"…","key":"value",…}
     * ```
     * In plain-text mode key=value pairs are appended after @p message.
     *
     * Uses the pre-allocated thread-local format buffer — no heap allocation
     * on the hot path once the buffer has been reserved.
     *
     * PII-sensitive field values (keys containing "password", "secret",
     * "token", "email", "phone", "ssn", or "credit_card") are redacted to
     * `"[REDACTED]"`.
     *
     * @param level    Severity level.
     * @param message  Human-readable log text (string_view — zero copy).
     * @param fields   Key/value pairs as an initializer list of string_view
     *                 pairs — no string construction required.
     */
    void logStructuredSV(
        Level level,
        std::string_view message,
        std::initializer_list<std::pair<std::string_view, std::string_view>> fields = {});

    // =========================================================================
    // ILogger overrides — delegate to string_view hot path (no copy)
    // =========================================================================

    void log(Level level, const std::string& message) override { logSV(level, message); }
    void trace(const std::string& message) override     { traceSV(message); }
    void debug(const std::string& message) override     { debugSV(message); }
    void info(const std::string& message) override      { infoSV(message); }
    void warn(const std::string& message) override      { warnSV(message); }
    void error(const std::string& message) override     { errorSV(message); }
    void critical(const std::string& message) override  { criticalSV(message); }

    /// Structured log with `std::map<string,string>` fields (ILogger compat).
    void logStructured(Level level,
                       const std::string& message,
                       const Fields& fields = {}) override;

    // =========================================================================
    // Configuration
    // =========================================================================

    void setLevel(Level level) override;
    Level getLevel() const override;
    void setPattern(const std::string& pattern) override;

    // =========================================================================
    // Lifecycle
    // =========================================================================

    void flush() noexcept override;
    void shutdown() noexcept override;
    ProbeResult isHealthy() const override;

    // =========================================================================
    // Accessors
    // =========================================================================

    /** Enable or disable JSON-mode structured logging at runtime.
     *
     *  Thread-safe: `setJsonMode` may be called concurrently with logging
     *  methods.  The `std::atomic<bool>` ensures a consistent view across
     *  threads.
     */
    void setJsonMode(bool enabled) noexcept { json_mode_.store(enabled, std::memory_order_relaxed); }
    bool jsonMode() const noexcept { return json_mode_.load(std::memory_order_relaxed); }

    /** Initial reservation of the thread-local format buffer (bytes). */
    std::size_t bufferCapacity() const noexcept { return buffer_capacity_; }

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::atomic<bool> json_mode_;
    std::size_t buffer_capacity_;

    static spdlog::level::level_enum toSpdlogLevel(Level level) noexcept;

    /// Return the thread-local format buffer, reserving capacity on first use.
    std::string& formatBuffer() const noexcept;

    /// Append a JSON-escaped version of @p s into @p out.
    static void jsonEscapeInto(std::string& out, std::string_view s);

    /// Return true if @p key names a PII-sensitive field.
    ///
    /// @note Keys longer than 128 bytes are scanned only up to the first 128
    ///       bytes. In practice, structured log field names should not exceed
    ///       this length; if they do, PII detection for those keys may produce
    ///       false negatives.
    static bool isPiiKey(std::string_view key) noexcept;

    /// Build a JSON log line into @p buf (does not allocate on hot path).
    void buildJsonInto(
        std::string& buf,
        Level level,
        std::string_view message,
        std::initializer_list<std::pair<std::string_view, std::string_view>> fields) const;

    /// Build a plain-text structured log line into @p buf.
    static void buildPlainStructuredInto(
        std::string& buf,
        std::string_view message,
        std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
};

} // namespace concerns
} // namespace core
} // namespace themis
