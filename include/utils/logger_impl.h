/**
 * @file logger_impl.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 84/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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
        logger_->trace(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        auto& metrics = metricsStorage();
        metrics.trace_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::debug(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::debug)) {
        logger_->debug(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        auto& metrics = metricsStorage();
        metrics.debug_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::info(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::info)) {
        logger_->info(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        auto& metrics = metricsStorage();
        metrics.info_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::warn(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::warn)) {
        logger_->warn(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        auto& metrics = metricsStorage();
        metrics.warn_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::error(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::err)) {
        logger_->error(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        auto& metrics = metricsStorage();
        metrics.error_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::critical(FormatString&& fmt, Args&&... args) {
    if (logger_ && logger_->should_log(spdlog::level::critical)) {
        logger_->critical(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        auto& metrics = metricsStorage();
        metrics.critical_count.fetch_add(1, std::memory_order_relaxed);
        metrics.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace utils
} // namespace themis
