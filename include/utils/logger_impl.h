/**
 * @file logger_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace themis {
namespace utils {

template<typename FormatString, typename... Args>
void Logger::trace(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::trace)) {
        const auto message = fmt::vformat(fmt::string_view(fmt), fmt::make_format_args(args...));
        logger_->log(spdlog::level::trace, message);
        auto& metrics = metricsStorage();
        metrics.trace_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::debug(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::debug)) {
        const auto message = fmt::vformat(fmt::string_view(fmt), fmt::make_format_args(args...));
        logger_->log(spdlog::level::debug, message);
        auto& metrics = metricsStorage();
        metrics.debug_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::info(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::info)) {
        const auto message = fmt::vformat(fmt::string_view(fmt), fmt::make_format_args(args...));
        logger_->log(spdlog::level::info, message);
        auto& metrics = metricsStorage();
        metrics.info_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::warn(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::warn)) {
        const auto message = fmt::vformat(fmt::string_view(fmt), fmt::make_format_args(args...));
        logger_->log(spdlog::level::warn, message);
        auto& metrics = metricsStorage();
        metrics.warn_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::error(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::err)) {
        const auto message = fmt::vformat(fmt::string_view(fmt), fmt::make_format_args(args...));
        logger_->log(spdlog::level::err, message);
        auto& metrics = metricsStorage();
        metrics.error_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::critical(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::critical)) {
        const auto message = fmt::vformat(fmt::string_view(fmt), fmt::make_format_args(args...));
        logger_->log(spdlog::level::critical, message);
        auto& metrics = metricsStorage();
        metrics.critical_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace utils
} // namespace themis
