/**
 * @file i_async_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 93/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

#pragma once

#include "core/concerns/i_logger.h"
#include <future>
#include <string>
#include <string_view>

namespace themis {
namespace core {
namespace concerns {

class IAsyncLogger : public ILogger {
public:
    // -----------------------------------------------------------------------
    // Async logging methods
    // -----------------------------------------------------------------------

    /**
     * @brief Log Async.
     * @param[in] level Input parameter.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: std::async(), std::string(), log().
     */
    virtual std::future<void> logAsync(Level level, std::string_view message) {
        return std::async(std::launch::async,
            [this, level, msg = std::string(message)] {
                this->log(level, msg);
            });
    }

    /**
     * @brief Trace Async.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: logAsync().
     */
    virtual std::future<void> traceAsync(std::string_view message) {
        return logAsync(Level::TRACE, message);
    }

    /**
     * @brief Debug Async.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: logAsync().
     */
    virtual std::future<void> debugAsync(std::string_view message) {
        return logAsync(Level::DEBUG, message);
    }

    /**
     * @brief Info Async.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: logAsync().
     */
    virtual std::future<void> infoAsync(std::string_view message) {
        return logAsync(Level::INFO, message);
    }

    /**
     * @brief Warn Async.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: logAsync().
     */
    virtual std::future<void> warnAsync(std::string_view message) {
        return logAsync(Level::WARN, message);
    }

    /**
     * @brief Error Async.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: logAsync().
     */
    virtual std::future<void> errorAsync(std::string_view message) {
        return logAsync(Level::ERROR, message);
    }

    /**
     * @brief Critical Async.
     * @param[in] message Input parameter.
     * @return Return value.
     * @details Calls: logAsync().
     */
    virtual std::future<void> criticalAsync(std::string_view message) {
        return logAsync(Level::CRITICAL, message);
    }

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

    /**
     * @brief Noop future.
     * @return Return value.
     * @details Calls: std::async().
     */
    static std::future<void> noop_future() {
        return std::async(std::launch::deferred, [] {});
    }
};

} // namespace concerns
} // namespace core
} // namespace themis
