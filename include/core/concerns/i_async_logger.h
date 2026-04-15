/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            i_async_logger.h                                   ║
  Version:         0.0.44                                             ║
  Last Modified:   2026-04-15 05:33:57                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     231                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "core/concerns/i_logger.h"
#include <future>
#include <string>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

/**
 * @brief Async variant of the logger interface for non-blocking log dispatch.
 *
 * `IAsyncLogger` extends `ILogger` with `std::future`-returning methods so
 * that callers on hot paths can fire-and-forget log records without blocking
 * on sink I/O.
 *
 * ## Default implementations
 *
 * Every async method has a default body that dispatches the corresponding
 * synchronous `ILogger` method on a separate thread via `std::async`.  Concrete
 * classes that already implement `ILogger` can therefore inherit from
 * `IAsyncLogger` and get working async variants for free:
 *
 * @code
 *   class MySpdlogLogger : public IAsyncLogger {
 *       // Only needs to implement the ILogger pure-virtual methods;
 *       // all IAsyncLogger methods work automatically via the defaults.
 *   };
 * @endcode
 *
 * High-throughput backends (e.g. lock-free ring buffers) can override the
 * defaults with a more efficient implementation.
 *
 * ## Thread Safety
 *
 * Returned futures represent independent asynchronous operations.  The
 * underlying `ILogger` implementation must be thread-safe if multiple async
 * calls can be in-flight concurrently.
 *
 * ## Usage
 *
 * @code
 *   IAsyncLogger& logger = ...;
 *
 *   // Fire-and-forget (discard future)
 *   logger.infoAsync("Request received");
 *
 *   // Await completion (e.g. in tests or before shutdown)
 *   auto f = logger.errorAsync("Fatal error occurred");
 *   f.get();
 * @endcode
 */
class IAsyncLogger : public ILogger {
public:
    // -----------------------------------------------------------------------
    // Async logging methods
    // -----------------------------------------------------------------------

    /**
     * @brief Log a message at the given level asynchronously.
     *
     * @param level   Severity level.
     * @param message Log text.
     * @return A future that becomes ready once the record has been dispatched
     *         to the underlying sink.
     */
    virtual std::future<void> logAsync(Level level, std::string_view message) {
        return std::async(std::launch::async,
            [this, level, msg = std::string(message)] {
                this->log(level, msg);
            });
    }

    /**
     * @brief Asynchronously log at TRACE level.
     * @param message Log text.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> traceAsync(std::string_view message) {
        return logAsync(Level::TRACE, message);
    }

    /**
     * @brief Asynchronously log at DEBUG level.
     * @param message Log text.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> debugAsync(std::string_view message) {
        return logAsync(Level::DEBUG, message);
    }

    /**
     * @brief Asynchronously log at INFO level.
     * @param message Log text.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> infoAsync(std::string_view message) {
        return logAsync(Level::INFO, message);
    }

    /**
     * @brief Asynchronously log at WARN level.
     * @param message Log text.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> warnAsync(std::string_view message) {
        return logAsync(Level::WARN, message);
    }

    /**
     * @brief Asynchronously log at ERROR level.
     * @param message Log text.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> errorAsync(std::string_view message) {
        return logAsync(Level::ERROR, message);
    }

    /**
     * @brief Asynchronously log at CRITICAL level.
     * @param message Log text.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> criticalAsync(std::string_view message) {
        return logAsync(Level::CRITICAL, message);
    }

    /**
     * @brief Asynchronously emit a structured log line.
     *
     * @param level   Severity level.
     * @param message Log text.
     * @param fields  Key/value metadata to include in the JSON output.
     * @return Future that resolves when the record is dispatched.
     */
    virtual std::future<void> logStructuredAsync(Level level,
                                                  std::string_view message,
                                                  const Fields& fields = {})
    {
        return std::async(std::launch::async,
            [this, level, msg = std::string(message), f = fields] {
                this->logStructured(level, msg, f);
            });
    }
};

// ---------------------------------------------------------------------------
// NoOpAsyncLogger
// ---------------------------------------------------------------------------

/**
 * @brief No-op async logger implementation.
 *
 * All sync methods are silent no-ops; all async methods return an already-
 * satisfied future (via `std::async(std::launch::deferred, ...)`) so tests
 * and benchmarks can call them without spawning real threads.
 *
 * Example (testing):
 * @code
 *   NoOpAsyncLogger logger;
 *   auto f = logger.infoAsync("silent");
 *   f.get(); // returns immediately
 * @endcode
 */
class NoOpAsyncLogger : public IAsyncLogger {
public:
    // ILogger sync methods (all no-ops)
    void log(Level, const std::string&) override {}
    void trace(const std::string&)    override {}
    void debug(const std::string&)    override {}
    void info(const std::string&)     override {}
    void warn(const std::string&)     override {}
    void error(const std::string&)    override {}
    void critical(const std::string&) override {}
    void setLevel(Level level) override { level_ = level; }
    Level getLevel() const override { return level_; }
    void setPattern(const std::string&) override {}
    void flush() noexcept override {}
    void shutdown() noexcept override {}
    ProbeResult isHealthy() const override { return ProbeResult::healthy(); }

    // IAsyncLogger overrides — deferred (no real thread spawned)
    std::future<void> logAsync(Level, std::string_view) override {
        return std::async(std::launch::deferred, [] {});
    }
    std::future<void> traceAsync(std::string_view)    override { return noop_future(); }
    std::future<void> debugAsync(std::string_view)    override { return noop_future(); }
    std::future<void> infoAsync(std::string_view)     override { return noop_future(); }
    std::future<void> warnAsync(std::string_view)     override { return noop_future(); }
    std::future<void> errorAsync(std::string_view)    override { return noop_future(); }
    std::future<void> criticalAsync(std::string_view) override { return noop_future(); }
    std::future<void> logStructuredAsync(Level, std::string_view, const Fields&) override {
        return noop_future();
    }

private:
    Level level_ = Level::INFO;

    static std::future<void> noop_future() {
        return std::async(std::launch::deferred, [] {});
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
