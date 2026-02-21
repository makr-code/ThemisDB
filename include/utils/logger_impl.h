/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            logger_impl.h                                      ║
  Version:         0.0.3                                              ║
  Last Modified:   2026-02-21 07:42:25                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   93.0/100                                       ║
    • Total Lines:     83                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

﻿#pragma once

#include <spdlog/spdlog.h>
#include <fmt/format.h>

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
