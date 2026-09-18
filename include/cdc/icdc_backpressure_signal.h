/**
 * @file icdc_backpressure_signal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Backpressure Signaling Interface
 *
 * Advisory flow-control interface for CDC event delivery.  Consumers that
 * cannot process events fast enough signal backpressure to the CDC layer,
 * which may reduce throughput or (when configured) trigger an automatic
 * ICDCPauseControl::pause() at the Critical level.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <functional>
#include <mutex>
#include <string>

namespace themis {
namespace cdc {

// ── BackpressureLevel ─────────────────────────────────────────────────────────

enum class BackpressureLevel {
    None     = 0, ///< Normal operation
    Low      = 1, ///< Light congestion
    Medium   = 2, ///< Moderate congestion; reduce batch sizes
    High     = 3, ///< Severe congestion; add inter-batch delays
    Critical = 4, ///< Emergency; trigger automatic pause if configured
};

// ── ICDCBackpressureSignal ────────────────────────────────────────────────────

class ICDCBackpressureSignal {
public:
    /**
     * @brief ICDCBackpressure Signal.
     * @return Return value.
     */
    virtual ~ICDCBackpressureSignal() = default;

    /**
     * @brief Signal Backpressure.
     * @param[in] level Input parameter.
     */
    virtual void signalBackpressure(BackpressureLevel level) = 0;

    /**
     * @brief Clear Backpressure.
     */
    virtual void clearBackpressure() = 0;

    /**
     * @brief Current Level.
     * @return Return value.
     */
    virtual BackpressureLevel currentLevel() const = 0;
};

// ── InMemoryBackpressureSignal ────────────────────────────────────────────────

class InMemoryBackpressureSignal : public ICDCBackpressureSignal {
public:
    using LevelCallback = std::function<void(BackpressureLevel /*new_level*/)>;

    explicit InMemoryBackpressureSignal(LevelCallback cb = {})
        : level_(BackpressureLevel::None), callback_(std::move(cb)) {}

    // ── ICDCBackpressureSignal ────────────────────────────────────────────────

    void signalBackpressure(BackpressureLevel level) override {
        BackpressureLevel prev = level_.exchange(level, std::memory_order_acq_rel);
        if (prev != level && callback_) {
            /**
             * @brief Lk.
             * @param[in] cb_mutex_ Input parameter.
             * @return Return value.
             */
            std::unique_lock<std::mutex> lk(cb_mutex_);
            if (callback_) {
              callback_(level);
            }
        }
    }

    void clearBackpressure() override {
        signalBackpressure(BackpressureLevel::None);
    }

    BackpressureLevel currentLevel() const override {
        return level_.load(std::memory_order_acquire);
    }

    /**
     * @brief ── Extra helpers ─────────────────────────────────────────────────────────
     * @param[in] cb Input parameter.
     * @details Calls: lk(), std::move().
     */

    void setCallback(LevelCallback cb) {
        std::unique_lock<std::mutex> lk(cb_mutex_);
        callback_ = std::move(cb);
    }

private:
    std::atomic<BackpressureLevel> level_;
    mutable std::mutex             cb_mutex_;
    LevelCallback                  callback_;
};

} // namespace cdc
} // namespace themis
