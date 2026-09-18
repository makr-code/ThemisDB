/**
 * @file idelivery_guarantee_config.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.12
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

enum class DeliveryMode {
    AtLeastOnce, ///< Events may be delivered more than once on failure; consumer must be idempotent or tolerant.
    ExactlyOnce, ///< Events are delivered exactly once; requires IIdempotentCDCListener.
};

// ── IIdempotentCDCListener ────────────────────────────────────────────────────

class IIdempotentCDCListener {
public:
    /**
     * @brief IIdempotent CDCListener.
     * @return Return value.
     */
    virtual ~IIdempotentCDCListener() = default;

    /**
     * @brief Is Duplicate.
     * @param[in] collection Input parameter.
     * @param[in] sequence Input parameter.
     * @return True when the operation succeeds.
     */
    virtual bool isDuplicate(const std::string& collection,
                             uint64_t           sequence) const = 0;

    /**
     * @brief Mark Processed.
     * @param[in] collection Input parameter.
     * @param[in] sequence Input parameter.
     */
    virtual void markProcessed(const std::string& collection,
                               uint64_t           sequence) = 0;
};

// ── IDeliveryGuaranteeConfig ──────────────────────────────────────────────────

class IDeliveryGuaranteeConfig {
public:
    /**
     * @brief IDelivery Guarantee Config.
     * @return Return value.
     */
    virtual ~IDeliveryGuaranteeConfig() = default;

    /**
     * @brief Set Mode.
     * @param[in] mode Input parameter.
     */
    virtual void setMode(DeliveryMode mode) = 0;

    /**
     * @brief Mode.
     * @return Return value.
     */
    virtual DeliveryMode mode() const = 0;

    /**
     * @brief Set Ack Timeout.
     * @param[in] timeout Input parameter.
     */
    virtual void setAckTimeout(std::chrono::milliseconds timeout) = 0;

    /**
     * @brief Ack Timeout.
     * @return Return value.
     */
    virtual std::chrono::milliseconds ackTimeout() const = 0;

    /**
     * @brief Set Deduplication Window.
     * @param[in] window Input parameter.
     */
    virtual void setDeduplicationWindow(std::chrono::milliseconds window) = 0;

    /**
     * @brief Deduplication Window.
     * @return Return value.
     */
    virtual std::chrono::milliseconds deduplicationWindow() const = 0;
};

// ── InMemoryDeliveryGuaranteeConfig ──────────────────────────────────────────

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
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        mode_ = mode;
    }

    DeliveryMode mode() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return mode_;
    }

    void setAckTimeout(std::chrono::milliseconds timeout) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        ack_timeout_ = timeout;
    }

    std::chrono::milliseconds ackTimeout() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return ack_timeout_;
    }

    void setDeduplicationWindow(std::chrono::milliseconds window) override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        dedup_window_ = window;
    }

    std::chrono::milliseconds deduplicationWindow() const override {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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

class InMemoryIdempotentListener : public IIdempotentCDCListener {
public:
    explicit InMemoryIdempotentListener(std::size_t max_window_size = 100'000)
        : max_window_size_(max_window_size) {}

    bool isDuplicate(const std::string& collection,
                     uint64_t           sequence) const override
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return processed_.count(makeKey(collection, sequence)) > 0;
    }

    void markProcessed(const std::string& collection,
                       uint64_t           sequence) override
    {
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
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
        /**
         * @brief Lk.
         * @param[in] mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock<std::mutex> lk(mutex_);
        return processed_.size();
    }

private:
    /**
     * @brief Make Key.
     * @param[in] collection Input parameter.
     * @param[in] seq Input parameter.
     * @return Return value.
     * @details Calls: std::to_string().
     */
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
