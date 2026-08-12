/**
 * @file log_aggregator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.13
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "core/concerns/i_async_logger.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <functional>
#include <future>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

namespace themis {
namespace observability {

/**
 * @brief A single structured log entry retained in the in-process buffer.
 *
 * Every entry is serialisable to JSON for machine-readable ingestion.
 */
struct LogEntry {
    /// Wall-clock timestamp of the event.
    std::chrono::system_clock::time_point timestamp;

    /// Severity level (maps to ILogger::Level).
    core::concerns::ILogger::Level level{core::concerns::ILogger::Level::INFO};

    /// Human-readable message.
    std::string message;

    /// Arbitrary key/value metadata (query IDs, latencies, operator names — no PII).
    std::map<std::string, std::string> fields;

    /// Serialise to a single-line JSON string.
    std::string toJson() const;
};

/**
 * @brief Sink target for LogAggregator output.
 */
enum class LogSinkType {
    MEMORY,  ///< In-process ring buffer only (default; useful for testing)
    FILE,    ///< Append to a file on disk
    BOTH     ///< In-process ring buffer AND file
};

/**
 * @brief Configuration for LogAggregator.
 */
struct LogAggregatorConfig {
    /// Minimum severity level; records below this level are silently dropped.
    core::concerns::ILogger::Level min_level{core::concerns::ILogger::Level::INFO};

    /// Where to write log entries.
    LogSinkType sink_type{LogSinkType::MEMORY};

    /// Path for file sink (used when sink_type is FILE or BOTH).
    std::string file_path;

    /// Maximum number of LogEntry objects to retain in the in-process ring
    /// buffer.  0 disables in-process retention (file-only mode).
    size_t max_retained_entries{4096};

    /// Maximum number of pending async log tasks in the worker queue.
    /// When the queue is full, new async tasks are dropped and the overflow
    /// counter is incremented.  0 disables async queueing (async calls fall
    /// back to synchronous dispatch on the calling thread).
    size_t async_queue_max_size{8192};

    /// spdlog-compatible format pattern used when formatting lines for the
    /// file sink.  Empty string uses a sensible default.
    std::string format_pattern;

    /// When true, publish log-count gauges to MetricsCollector after each
    /// batch of log calls (controlled by publish_interval).
    bool publish_metrics{false};

    /// How often to flush counters to MetricsCollector.
    std::chrono::seconds publish_interval{10};
};

/**
 * @brief Counters exported by LogAggregator for observability.
 */
struct LogAggregatorStats {
    int64_t total_entries{0};        ///< Total entries accepted (not dropped)
    int64_t dropped_entries{0};      ///< Entries dropped due to level filter
    int64_t entries_by_level[6]{0, 0, 0, 0, 0, 0}; ///< Per-level accepted count
    int64_t async_queue_overflows{0}; ///< Async tasks dropped due to full queue
};

/**
 * @brief Structured-log aggregator and collector for the observability module.
 *
 * `LogAggregator` implements `core::concerns::IAsyncLogger` (which in turn
 * extends `ILogger`) and augments the base interface with:
 *   - In-process ring buffer of `LogEntry` structs (each serialisable to JSON)
 *   - Optional file sink (append-only, one JSON object per line)
 *   - Trace-context correlation via `logWithContext()` — every entry carries
 *     `trace_id`, `span_id`, and `request_id` from the caller's
 *     `TraceContext`, enabling end-to-end log/trace correlation
 *   - Per-level counters published to MetricsCollector as Prometheus gauges
 *     (`themis_log_entries_total{level="info"}`, etc.)
 *   - Optional callback hook (`onEntry`) for downstream sinks
 *     (e.g. SIEM forwarder, test spy)
 *   - Thread-safe; all public methods use a single mutex
 *
 * ### Design Notes
 * - Fields in `logStructured()` / `logWithContext()` must NOT contain PII
 *   (user data, query result values); only identifiers, latencies, and
 *   operator names are acceptable.
 * - The in-process buffer uses a `std::deque` capped at
 *   `config.max_retained_entries`; oldest entries are evicted when full.
 * - File output is flushed after every write to limit data loss.
 *
 * ### Usage
 * ```cpp
 * LogAggregatorConfig cfg;
 * cfg.min_level   = ILogger::Level::INFO;
 * cfg.sink_type   = LogSinkType::FILE;
 * cfg.file_path   = "/var/log/themisdb/structured.jsonl";
 * cfg.max_retained_entries = 2000;
 *
 * LogAggregator agg(cfg);
 * agg.info("Query executed");
 * agg.logStructured(ILogger::Level::WARN, "Slow query detected",
 *                   {{"query_id", "q-123"}, {"latency_ms", "450"}});
 *
 * // Retrieve buffered entries for diagnostics
 * for (const auto& e : agg.entries()) {
 *     std::cout << e.toJson() << "\n";
 * }
 * ```
 * ### Async streaming
 * `LogAggregator` also implements `core::concerns::IAsyncLogger`.  A dedicated
 * background worker thread drains a bounded in-process queue so that callers
 * on hot paths can fire-and-forget log records without blocking on sink I/O.
 *
 * ```cpp
 * // Fire-and-forget (discard future):
 * agg.infoAsync("Request accepted");
 *
 * // Await dispatch (e.g. in tests or before shutdown):
 * agg.logStructuredAsync(Level::WARN, "Slow query", {{"ms", "450"}}).get();
 * ```
 *
 * When the async queue is full (configurable via `async_queue_max_size`),
 * new tasks are dropped and `LogAggregatorStats::async_queue_overflows` is
 * incremented.  `shutdown()` drains all pending async tasks before returning.
 *
 */
class LogAggregator : public core::concerns::IAsyncLogger {
public:
    using Fields   = core::concerns::ILogger::Fields;
    using Level    = core::concerns::ILogger::Level;
    using TraceCtx = core::concerns::TraceContext;

    /// Callback type invoked (under the mutex) for every accepted entry.
    using EntryCallback = std::function<void(const LogEntry&)>;

    explicit LogAggregator(const LogAggregatorConfig& config = LogAggregatorConfig{});
    ~LogAggregator() override;

    // Non-copyable
    LogAggregator(const LogAggregator&) = delete;
    LogAggregator& operator=(const LogAggregator&) = delete;

    // -----------------------------------------------------------------------
    // ILogger interface
    // -----------------------------------------------------------------------

    void log(Level level, const std::string& message) override;
    void trace(const std::string& message) override;
    void debug(const std::string& message) override;
    void info(const std::string& message) override;
    void warn(const std::string& message) override;
    void error(const std::string& message) override;
    void critical(const std::string& message) override;

    void logStructured(Level level,
                       const std::string& message,
                       const Fields& fields = {}) override;

    void logWithContext(Level level,
                        const std::string& message,
                        const TraceCtx& ctx,
                        const Fields& fields = {}) override;

    void setLevel(Level level) override;
    Level getLevel() const override;
    void setPattern(const std::string& pattern) override;

    void flush() noexcept override;
    void shutdown() noexcept override;

    core::concerns::ProbeResult isHealthy() const override;

    // -----------------------------------------------------------------------
    // IAsyncLogger overrides — worker-thread-backed async dispatch
    // -----------------------------------------------------------------------

    /**
     * @brief Post a log record to the async worker queue.
     *
     * Returns a future that resolves once the record has been processed by the
     * background worker thread and written to all configured sinks.  When the
     * queue is full the record is dropped, `stats().async_queue_overflows` is
     * incremented, and the future resolves immediately.
     */
    std::future<void> logAsync(Level level, std::string_view message) override;

    /**
     * @brief Post a structured log record to the async worker queue.
     *
     * Like `logAsync()` but includes key/value @p fields in the entry.
     */
    std::future<void> logStructuredAsync(Level level,
                                          std::string_view message,
                                          const Fields& fields = {}) override;

    /**
     * @brief Post a trace-context-correlated log record to the async worker
     *        queue.
     *
     * Injects `trace_id`, `span_id`, and `request_id` from @p ctx as fields
     * and dispatches asynchronously.
     */
    std::future<void> logWithContextAsync(Level level,
                                           std::string_view message,
                                           const TraceCtx& ctx,
                                           const Fields& fields = {});

    // -----------------------------------------------------------------------
    // Extended API
    // -----------------------------------------------------------------------

    /**
     * @brief Return a copy of all buffered log entries.
     *
     * Entries are in chronological order (oldest first).  The buffer is
     * capped at `config.max_retained_entries`; older entries are evicted.
     */
    std::vector<LogEntry> entries() const;

    /**
     * @brief Return buffered entries at or above @p min_level.
     *
     * Convenience wrapper that filters the result of entries().
     */
    std::vector<LogEntry> entriesAtLevel(Level min_level) const;

    /**
     * @brief Remove all buffered entries.
     *
     * Does not affect the file sink (already written records are not removed).
     */
    void clear();

    /** @brief Total number of entries currently in the buffer. */
    size_t size() const;

    /** @brief Return current aggregator statistics. */
    LogAggregatorStats stats() const;

    /**
     * @brief Register a callback invoked for every accepted log entry.
     *
     * The callback is called under the internal mutex; it must not call
     * any LogAggregator method to avoid deadlock.
     *
     * @param cb Callback to register.  Pass nullptr to remove the callback.
     */
    void setEntryCallback(EntryCallback cb);

    /** @brief Return the active configuration. */
    LogAggregatorConfig getConfig() const;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace observability
} // namespace themis
