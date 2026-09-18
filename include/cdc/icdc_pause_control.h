/**
 * @file icdc_pause_control.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Pause/Resume Control Interface
 *
 * Provides atomic stream suspension and resumption for CDC change feeds.
 * Consumers (e.g. admin API, backpressure manager, schema evolution handler)
 * call pause() to halt event delivery and resume() to restart it.  Events
 * arriving during a pause are buffered up to maxBufferBytes; overflow returns
 * an error.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <stdexcept>
#include <string>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── PauseReason ───────────────────────────────────────────────────────────────

enum class PauseReason {
    AdminRequest,     ///< Explicitly paused by an administrative action
    Backpressure,     ///< Paused because a Critical backpressure level was reached
    SchemaEvolution,  ///< Paused due to an incompatible schema change
};

// ── ICDCPauseControl ──────────────────────────────────────────────────────────

class ICDCPauseControl {
public:
    /**
     * @brief ICDCPause Control.
     * @return Return value.
     */
    virtual ~ICDCPauseControl() = default;

    [[nodiscard]] virtual bool pause(PauseReason reason = PauseReason::AdminRequest) = 0;

    [[nodiscard]] virtual bool resume() = 0;

    [[nodiscard]] virtual bool isPaused() const = 0;

    [[nodiscard]] virtual PauseReason pauseReason() const = 0;

    [[nodiscard]] virtual std::size_t bufferedEventCount() const = 0;
};

// ── InMemoryPauseControl ──────────────────────────────────────────────────────

class InMemoryPauseControl : public ICDCPauseControl {
public:
    static constexpr std::size_t kDefaultMaxBufferBytes = 64 * 1024 * 1024; // 64 MiB

    explicit InMemoryPauseControl(
        std::size_t max_buffer_bytes = kDefaultMaxBufferBytes)
        : max_buffer_bytes_(max_buffer_bytes) {}

    // ── ICDCPauseControl ─────────────────────────────────────────────────────

    bool pause(PauseReason reason = PauseReason::AdminRequest) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        if (paused_.load(std::memory_order_relaxed)) {
            return true; // already paused — no-op
        }
        reason_ = reason;
        paused_.store(true, std::memory_order_release);
        return true;
    }

    bool resume() override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        if (!paused_.load(std::memory_order_relaxed)) {
            return true; // already running — no-op
        }
        paused_.store(false, std::memory_order_release);
        buffered_bytes_ = 0;
        cv_.notify_all();
        return true;
    }

    bool isPaused() const override {
        return paused_.load(std::memory_order_acquire);
    }

    PauseReason pauseReason() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return reason_;
    }

    std::size_t bufferedEventCount() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return buffer_.size();
    }

    /**
     * @brief ── InMemoryPauseControl-specific API ────────────────────────────────────
     * @param[in] event Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lk(), load(), toJson(), dump(), size(), push_back().
     */

    bool bufferEvent(const Changefeed::ChangeEvent& event) {
        std::unique_lock<std::mutex> lk(mutex_);
        if (!paused_.load(std::memory_order_relaxed)) {
            return false;
        }
        const std::string serialised = event.toJson().dump();
        if (buffered_bytes_ + serialised.size() > max_buffer_bytes_) {
            return false; // buffer full
        }
        buffered_bytes_ += serialised.size();
        buffer_.push_back(event);
        return true;
    }

    /**
     * @brief Drain Buffer.
     * @return Return value.
     * @details Calls: lk(), swap().
     */
    std::deque<Changefeed::ChangeEvent> drainBuffer() {
        std::unique_lock<std::mutex> lk(mutex_);
        std::deque<Changefeed::ChangeEvent> out;
        out.swap(buffer_);
        buffered_bytes_ = 0;
        return out;
    }

    /**
     * @brief Wait For Resume.
     * @param[in] timeout Input parameter.
     * @return True when the operation succeeds.
     * @details Calls: lk(), wait_for(), load().
     */
    bool waitForResume(std::chrono::milliseconds timeout) {
        std::unique_lock<std::mutex> lk(mutex_);
        return cv_.wait_for(lk, timeout,
            [this] { return !paused_.load(std::memory_order_relaxed); });
    }

private:
    mutable std::mutex              mutex_;
    std::condition_variable         cv_;
    std::atomic<bool>               paused_{false};
    PauseReason                     reason_{PauseReason::AdminRequest};
    std::deque<Changefeed::ChangeEvent> buffer_;
    std::size_t                     buffered_bytes_{0};
    std::size_t                     max_buffer_bytes_;
};

} // namespace cdc
} // namespace themis
