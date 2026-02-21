/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            logger_impl.h                                      ║
  Version:         0.0.22                                             ║
  Last Modified:   2026-02-21 19:28:55                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     90                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <fmt/format.h>
#include <spdlog/spdlog.h>

namespace themis {
namespace utils {

template<typename FormatString, typename... Args>
void Logger::trace(FormatString&& fmt, Args&&... args) {
    if (logger_) {
        logger_->trace(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        metrics_.trace_count.fetch_add(1, std::memory_order_relaxed);
        metrics_.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::debug(FormatString&& fmt, Args&&... args) {
    if (logger_) {
        logger_->debug(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        metrics_.debug_count.fetch_add(1, std::memory_order_relaxed);
        metrics_.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::info(FormatString&& fmt, Args&&... args) {
    if (logger_) {
        logger_->info(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        metrics_.info_count.fetch_add(1, std::memory_order_relaxed);
        metrics_.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::warn(FormatString&& fmt, Args&&... args) {
    if (logger_) {
        logger_->warn(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        metrics_.warn_count.fetch_add(1, std::memory_order_relaxed);
        metrics_.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::error(FormatString&& fmt, Args&&... args) {
    if (logger_) {
        logger_->error(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        metrics_.error_count.fetch_add(1, std::memory_order_relaxed);
        metrics_.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

template<typename FormatString, typename... Args>
void Logger::critical(FormatString&& fmt, Args&&... args) {
    if (logger_) {
        logger_->critical(fmt::runtime(std::forward<FormatString>(fmt)), std::forward<Args>(args)...);
        metrics_.critical_count.fetch_add(1, std::memory_order_relaxed);
        metrics_.total_count.fetch_add(1, std::memory_order_relaxed);
    }
}

} // namespace utils
} // namespace themis
