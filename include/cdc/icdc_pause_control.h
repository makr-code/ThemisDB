/**
 * @file icdc_pause_control.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Reason a CDC stream was paused.
 *
 * Allows the admin interface and monitoring systems to distinguish between
 * operator-initiated pauses, backpressure-induced pauses, and pauses triggered
 * by incompatible schema evolution.
 */
enum class PauseReason {
    AdminRequest,     ///< Explicitly paused by an administrative action
    Backpressure,     ///< Paused because a Critical backpressure level was reached
    SchemaEvolution,  ///< Paused due to an incompatible schema change
};

// ── ICDCPauseControl ──────────────────────────────────────────────────────────

/**
 * @brief Abstract interface for atomic CDC stream pause / resume control.
 *
 * Thread-safety: all methods must be thread-safe in every implementation.
 *
 * Design constraints:
 *  - pause() and resume() are atomic; no events are delivered between a
 *    successful pause() call returning and resume() being called.
 *  - Events that arrive during a pause are buffered up to maxBufferBytes.
 *    When the buffer is full, pause() / event ingestion returns an error.
 *  - isPaused() is non-blocking; it reflects the current pause state without
 *    acquiring any heavyweight lock.
 */
class ICDCPauseControl {
public:
    virtual ~ICDCPauseControl() = default;

    /**
     * @brief Pause the stream.
     *
     * Subsequent change events are buffered rather than delivered until
     * resume() is called.  Calling pause() on an already-paused stream is
     * a no-op that returns true.
     *
     * @param reason  The reason for the pause (recorded for audit / monitoring).
     * @return true on success; false if the operation failed (e.g. buffer
     *         pre-condition check failed).
     */
    [[nodiscard]] virtual bool pause(PauseReason reason = PauseReason::AdminRequest) = 0;

    /**
     * @brief Resume the stream.
     *
     * All buffered events are flushed to subscribers before new events are
     * delivered.  Calling resume() on a running stream is a no-op that returns
     * true.
     *
     * @return true on success; false if the operation failed.
     */
    [[nodiscard]] virtual bool resume() = 0;

    /**
     * @brief Non-blocking query of the current pause state.
     *
     * @return true if the stream is currently paused.
     */
    [[nodiscard]] virtual bool isPaused() const = 0;

    /**
     * @brief Return the reason the stream was most recently paused.
     *
     * The return value is undefined when isPaused() == false; callers should
     * check isPaused() first.
     */
    [[nodiscard]] virtual PauseReason pauseReason() const = 0;

    /**
     * @brief Number of events currently buffered during the pause.
     */
    [[nodiscard]] virtual std::size_t bufferedEventCount() const = 0;
};

// ── InMemoryPauseControl ──────────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of ICDCPauseControl.
 *
 * Suitable for unit tests and standalone use.  Events pushed via
 * bufferEvent() during a pause are stored in an internal deque.
 * drainBuffer() returns them in FIFO order after resume() is called.
 *
 * maxBufferBytes is enforced on the serialised JSON representation of
 * buffered events; each event is counted as its toJson().dump() size.
 */
class InMemoryPauseControl : public ICDCPauseControl {
public:
    static constexpr std::size_t kDefaultMaxBufferBytes = 64 * 1024 * 1024; // 64 MiB

    explicit InMemoryPauseControl(
        std::size_t max_buffer_bytes = kDefaultMaxBufferBytes)
        : max_buffer_bytes_(max_buffer_bytes) {}

    // ── ICDCPauseControl ─────────────────────────────────────────────────────

    bool pause(PauseReason reason = PauseReason::AdminRequest) override {
        std::unique_lock<std::mutex> lk(mutex_);
        if (paused_.load(std::memory_order_relaxed)) {
            return true; // already paused — no-op
        }
        reason_ = reason;
        paused_.store(true, std::memory_order_release);
        return true;
    }

    bool resume() override {
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
        std::unique_lock<std::mutex> lk(mutex_);
        return reason_;
    }

    std::size_t bufferedEventCount() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return buffer_.size();
    }

    // ── InMemoryPauseControl-specific API ────────────────────────────────────

    /**
     * @brief Buffer an event during a pause.
     *
     * @return true if the event was buffered; false if the buffer is full
     *         (max_buffer_bytes exceeded) or the stream is not paused.
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
     * @brief Drain and return all buffered events in FIFO order.
     *
     * Clears the internal buffer.  Should be called after resume() to
     * replay buffered events to subscribers.
     */
    std::deque<Changefeed::ChangeEvent> drainBuffer() {
        std::unique_lock<std::mutex> lk(mutex_);
        std::deque<Changefeed::ChangeEvent> out;
        out.swap(buffer_);
        buffered_bytes_ = 0;
        return out;
    }

    /**
     * @brief Block until the stream is resumed or the timeout elapses.
     *
     * @return true if the stream was resumed; false on timeout.
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
