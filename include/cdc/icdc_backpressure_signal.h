/**
 * @file icdc_backpressure_signal.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Severity levels for consumer backpressure signals.
 *
 * Ordered from least (None) to most severe (Critical).  The CDC layer uses
 * these levels to adjust delivery throughput:
 *   None     — normal operation; no throttling.
 *   Low      — advisory; the CDC layer may log the condition.
 *   Medium   — the CDC layer reduces event batch sizes.
 *   High     — the CDC layer introduces inter-batch delays.
 *   Critical — the CDC layer calls ICDCPauseControl::pause(Backpressure)
 *              if a pause-control handle is registered.
 */
enum class BackpressureLevel {
    None     = 0, ///< Normal operation
    Low      = 1, ///< Light congestion
    Medium   = 2, ///< Moderate congestion; reduce batch sizes
    High     = 3, ///< Severe congestion; add inter-batch delays
    Critical = 4, ///< Emergency; trigger automatic pause if configured
};

// ── ICDCBackpressureSignal ────────────────────────────────────────────────────

/**
 * @brief Abstract advisory backpressure interface for CDC consumers.
 *
 * Implementations are expected to be thread-safe.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - Signals are advisory; the CDC layer may still deliver events when a
 *    signal is active, but reduces throughput at Medium/High levels.
 *  - Critical level triggers automatic ICDCPauseControl::pause() when
 *    a pause-control integration is configured.
 *  - signalBackpressure() is non-blocking; ≤ 1 µs overhead.
 *  - currentLevel() is non-blocking.
 */
class ICDCBackpressureSignal {
public:
    virtual ~ICDCBackpressureSignal() = default;

    /**
     * @brief Signal the current backpressure level to the CDC layer.
     *
     * Replaces the previous level; subsequent calls with a lower level
     * effectively clear the signal.
     *
     * @param level  The new backpressure level.
     */
    virtual void signalBackpressure(BackpressureLevel level) = 0;

    /**
     * @brief Clear backpressure; equivalent to signalBackpressure(None).
     */
    virtual void clearBackpressure() = 0;

    /**
     * @brief Return the current backpressure level without blocking.
     */
    virtual BackpressureLevel currentLevel() const = 0;
};

// ── InMemoryBackpressureSignal ────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of ICDCBackpressureSignal.
 *
 * Stores the current level atomically.  An optional callback is invoked
 * (under a light mutex) whenever the level changes, allowing integration
 * tests and the CDC layer to react to level changes immediately.
 */
class InMemoryBackpressureSignal : public ICDCBackpressureSignal {
public:
    using LevelCallback = std::function<void(BackpressureLevel /*new_level*/)>;

    explicit InMemoryBackpressureSignal(LevelCallback cb = {})
        : level_(BackpressureLevel::None), callback_(std::move(cb)) {}

    // ── ICDCBackpressureSignal ────────────────────────────────────────────────

    void signalBackpressure(BackpressureLevel level) override {
        BackpressureLevel prev = level_.exchange(level, std::memory_order_acq_rel);
        if (prev != level && callback_) {
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

    // ── Extra helpers ─────────────────────────────────────────────────────────

    /**
     * @brief Register or replace the level-change callback.
     *
     * Thread-safe; replaces any previously registered callback.
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
