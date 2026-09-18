/**
 * @file zero_copy_logger.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.1
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 100/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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

class ZeroCopyLogger : public ILogger {
public:
    static constexpr std::size_t kDefaultBufferCapacity = 4096;

    explicit ZeroCopyLogger(std::shared_ptr<spdlog::logger> logger = nullptr,
                            bool json_mode = false,
                            std::size_t buffer_capacity = kDefaultBufferCapacity);

    /**
     * @brief ========================================================================= Zero-copy string_view hot-path API =========================================================================
     * @param[in] level Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */

    bool shouldLog(Level level) const noexcept;

    /**
     * @brief Log SV.
     * @param[in] level Input parameter.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void logSV(Level level, std::string_view message) noexcept;

    /**
     * @brief Trace SV.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void traceSV(std::string_view message) noexcept;

    /**
     * @brief Debug SV.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void debugSV(std::string_view message) noexcept;

    /**
     * @brief Info SV.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void infoSV(std::string_view message) noexcept;

    /**
     * @brief Warn SV.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void warnSV(std::string_view message) noexcept;

    /**
     * @brief Error SV.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void errorSV(std::string_view message) noexcept;

    /**
     * @brief Critical SV.
     * @param[in] message Input parameter.
     * @note Exception safety: noexcept.
     */
    void criticalSV(std::string_view message) noexcept;

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

    void setJsonMode(bool enabled) noexcept { json_mode_.store(enabled, std::memory_order_relaxed); }
    bool jsonMode() const noexcept { return json_mode_.load(std::memory_order_relaxed); }

    std::size_t bufferCapacity() const noexcept { return buffer_capacity_; }

private:
    std::shared_ptr<spdlog::logger> logger_;
    std::atomic<bool> json_mode_;
    std::size_t buffer_capacity_;

    /**
     * @brief To Spdlog Level.
     * @param[in] level Input parameter.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    static spdlog::level::level_enum toSpdlogLevel(Level level) noexcept;

    /**
     * @brief Format Buffer.
     * @return Return value.
     * @note Exception safety: noexcept.
     */
    std::string& formatBuffer() const noexcept;

    /**
     * @brief Json Escape Into.
     * @param[in,out] out Input/output parameter.
     * @param[in] s Input parameter.
     */
    static void jsonEscapeInto(std::string& out, std::string_view s);

    /**
     * @brief Is Pii Key.
     * @param[in] key Input parameter.
     * @return True when the operation succeeds.
     * @note Exception safety: noexcept.
     */
    static bool isPiiKey(std::string_view key) noexcept;

    void buildJsonInto(
        std::string& buf,
        Level level,
        std::string_view message,
        std::initializer_list<std::pair<std::string_view, std::string_view>> fields) const;

    static void buildPlainStructuredInto(
        std::string& buf,
        std::string_view message,
        std::initializer_list<std::pair<std::string_view, std::string_view>> fields);
};

} // namespace concerns
} // namespace core
} // namespace themis
