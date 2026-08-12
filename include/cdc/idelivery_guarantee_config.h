/**
 * @file idelivery_guarantee_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB — CDC Delivery Guarantee Configuration Interface
 *
 * Configures at-least-once vs. exactly-once delivery semantics per CDC
 * listener registration.  The exactly-once mode requires the listener to
 * implement IIdempotentCDCListener to prevent duplicate processing.
 *
 * Design constraints (from FUTURE_ENHANCEMENTS.md):
 *  - DeliveryMode::ExactlyOnce requires IIdempotentCDCListener.
 *  - setDeduplicationWindow() configures the rolling dedup window for
 *    ExactlyOnce mode; default is 5 minutes.
 *  - Deduplication check per event ≤ 10 µs via a rolling hash window.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include <chrono>
#include <deque>
#include <functional>
#include <mutex>
#include <string>
#include <unordered_set>

#include "cdc/changefeed.h"

namespace themis {
namespace cdc {

// ── DeliveryMode ──────────────────────────────────────────────────────────────

/**
 * @brief Delivery semantics for a CDC listener.
 */
enum class DeliveryMode {
    AtLeastOnce, ///< Events may be delivered more than once on failure; consumer must be idempotent or tolerant.
    ExactlyOnce, ///< Events are delivered exactly once; requires IIdempotentCDCListener.
};

// ── IIdempotentCDCListener ────────────────────────────────────────────────────

/**
 * @brief Marker interface for listeners that support exactly-once delivery.
 *
 * Listeners registered with DeliveryMode::ExactlyOnce must implement this
 * interface.  The CDC layer will call isDuplicate() before delivering each
 * event and skip duplicates.
 *
 * Thread-safety: all methods must be thread-safe.
 */
class IIdempotentCDCListener {
public:
    virtual ~IIdempotentCDCListener() = default;

    /**
     * @brief Check whether the event with the given sequence has already been
     *        processed by this listener.
     *
     * Called by the CDC layer before delivery.  Must return in ≤ 10 µs.
     *
     * @param collection  The collection the event originates from.
     * @param sequence    Monotonic sequence number of the event.
     * @return true if the event is a duplicate and should be skipped.
     */
    virtual bool isDuplicate(const std::string& collection,
                             uint64_t           sequence) const = 0;

    /**
     * @brief Record that the event has been successfully processed.
     *
     * Called by the CDC layer after successful delivery.
     */
    virtual void markProcessed(const std::string& collection,
                               uint64_t           sequence) = 0;
};

// ── IDeliveryGuaranteeConfig ──────────────────────────────────────────────────

/**
 * @brief Abstract configuration interface for CDC delivery guarantees.
 *
 * Instances are created per listener registration and handed to the CDC layer.
 *
 * Thread-safety: all methods must be thread-safe.
 */
class IDeliveryGuaranteeConfig {
public:
    virtual ~IDeliveryGuaranteeConfig() = default;

    /**
     * @brief Set the delivery mode.
     *
     * Switching from ExactlyOnce to AtLeastOnce after the stream has started
     * is allowed; the dedup state is discarded.
     *
     * @param mode  The desired delivery mode.
     */
    virtual void setMode(DeliveryMode mode) = 0;

    /**
     * @brief Return the current delivery mode.
     */
    virtual DeliveryMode mode() const = 0;

    /**
     * @brief Set the acknowledgement timeout for at-least-once delivery.
     *
     * Events not acknowledged within this window are redelivered.
     * Only relevant for AtLeastOnce mode.
     *
     * @param timeout  Timeout duration (default 30 s).
     */
    virtual void setAckTimeout(std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Return the current acknowledgement timeout.
     */
    virtual std::chrono::milliseconds ackTimeout() const = 0;

    /**
     * @brief Set the deduplication window for exactly-once delivery.
     *
     * Events with a timestamp_ms older than (now − window) are considered
     * outside the dedup window and will never be flagged as duplicates.
     *
     * Only relevant for ExactlyOnce mode.
     *
     * @param window  Window duration (default 5 min).
     */
    virtual void setDeduplicationWindow(std::chrono::milliseconds window) = 0;

    /**
     * @brief Return the current deduplication window.
     */
    virtual std::chrono::milliseconds deduplicationWindow() const = 0;
};

// ── InMemoryDeliveryGuaranteeConfig ──────────────────────────────────────────

/**
 * @brief Thread-safe in-memory implementation of IDeliveryGuaranteeConfig.
 *
 * Suitable for unit tests and standalone use.
 */
class InMemoryDeliveryGuaranteeConfig : public IDeliveryGuaranteeConfig {
public:
    static constexpr auto kDefaultAckTimeout      = std::chrono::seconds(30);
    static constexpr auto kDefaultDedupWindow     = std::chrono::minutes(5);

    explicit InMemoryDeliveryGuaranteeConfig(
        DeliveryMode              mode    = DeliveryMode::AtLeastOnce,
        std::chrono::milliseconds ack_to  = kDefaultAckTimeout,
        std::chrono::milliseconds dedup_w = kDefaultDedupWindow)
        : mode_(mode), ack_timeout_(ack_to), dedup_window_(dedup_w) {}

    // ── IDeliveryGuaranteeConfig ─────────────────────────────────────────────

    void setMode(DeliveryMode mode) override {
        std::unique_lock<std::mutex> lk(mutex_);
        mode_ = mode;
    }

    DeliveryMode mode() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return mode_;
    }

    void setAckTimeout(std::chrono::milliseconds timeout) override {
        std::unique_lock<std::mutex> lk(mutex_);
        ack_timeout_ = timeout;
    }

    std::chrono::milliseconds ackTimeout() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return ack_timeout_;
    }

    void setDeduplicationWindow(std::chrono::milliseconds window) override {
        std::unique_lock<std::mutex> lk(mutex_);
        dedup_window_ = window;
    }

    std::chrono::milliseconds deduplicationWindow() const override {
        std::unique_lock<std::mutex> lk(mutex_);
        return dedup_window_;
    }

private:
    mutable std::mutex        mutex_;
    DeliveryMode              mode_;
    std::chrono::milliseconds ack_timeout_;
    std::chrono::milliseconds dedup_window_;
};

// ── InMemoryIdempotentListener ────────────────────────────────────────────────

/**
 * @brief Thread-safe in-memory IIdempotentCDCListener.
 *
 * Stores processed (collection, sequence) pairs in an unordered_set.
 * Optionally accepts a max_window_size to bound memory usage by evicting
 * the oldest entries (FIFO) when the window is full.
 */
class InMemoryIdempotentListener : public IIdempotentCDCListener {
public:
    explicit InMemoryIdempotentListener(std::size_t max_window_size = 100'000)
        : max_window_size_(max_window_size) {}

    bool isDuplicate(const std::string& collection,
                     uint64_t           sequence) const override
    {
        std::unique_lock<std::mutex> lk(mutex_);
        return processed_.count(makeKey(collection, sequence)) > 0;
    }

    void markProcessed(const std::string& collection,
                       uint64_t           sequence) override
    {
        std::unique_lock<std::mutex> lk(mutex_);
        const std::string key = makeKey(collection, sequence);
        if (processed_.count(key)) return; // already recorded
        // Evict oldest if window is full
        if (fifo_.size() >= max_window_size_) {
            processed_.erase(fifo_.front());
            fifo_.pop_front();
        }
        processed_.insert(key);
        fifo_.push_back(key);
    }

    std::size_t processedCount() const {
        std::unique_lock<std::mutex> lk(mutex_);
        return processed_.size();
    }

private:
    static std::string makeKey(const std::string& collection, uint64_t seq) {
        return collection + ":" + std::to_string(seq);
    }

    mutable std::mutex                mutex_;
    std::unordered_set<std::string>   processed_;
    std::deque<std::string>           fifo_;
    std::size_t                       max_window_size_;
};

} // namespace cdc
} // namespace themis
