/**
 * @file delivery_tracker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB CDC At-Least-Once Delivery Tracker
 *
 * Tracks in-flight change events delivered to consumers and provides
 * redelivery of unacknowledged events, implementing at-least-once delivery
 * semantics for CDC streams.
 *
 * Design:
 *  - Caller delivers events to a named consumer via trackDelivery().
 *  - Consumer acknowledges receipt via acknowledge().
 *  - Events not acknowledged within the configured ack_timeout are returned
 *    by getPendingRedelivery() so the caller can re-send them.
 *  - An optional redelivery callback is invoked by a background thread when
 *    events exceed their ack deadline.
 *
 * Thread-safety: all public methods are thread-safe.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#include "cdc/changefeed.h"

#include <chrono>
#include <functional>
#include <map>
#include <mutex>
#include <string>
#include <thread>
#include <condition_variable>
#include <atomic>
#include <vector>
#include <cstdint>
#include <optional>

namespace themis {
namespace cdc {

struct DeliveryTrackerConfig {
    std::chrono::milliseconds ack_timeout{30000};

    std::chrono::milliseconds recheck_interval{5000};

    uint32_t max_redelivery_attempts{0};

    size_t max_pending_per_consumer{10000};
};

struct ConsumerDeliveryStats {
    std::string consumer_id;
    size_t pending_count{0};           ///< Events awaiting acknowledgement
    uint64_t total_delivered{0};       ///< Cumulative events delivered
    uint64_t total_acknowledged{0};    ///< Cumulative events acknowledged
    uint64_t total_redeliveries{0};    ///< Cumulative redelivery attempts
    uint64_t total_expired{0};         ///< Events dropped after max_redelivery_attempts
};

class DeliveryTracker {
public:
    using RedeliveryCallback =
        std::function<void(const std::string& consumer_id,
                           const std::vector<Changefeed::ChangeEvent>& events)>;

    explicit DeliveryTracker(DeliveryTrackerConfig config = {},
                             RedeliveryCallback callback = nullptr);

    ~DeliveryTracker();

    // Non-copyable, non-movable (owns thread + mutex state)
    DeliveryTracker(const DeliveryTracker&) = delete;
    DeliveryTracker& operator=(const DeliveryTracker&) = delete;

    /**
     * @brief Start.
     */
    void start();

    /**
     * @brief Stop.
     */
    void stop();

    /**
     * @brief Track Delivery.
     * @param[in] consumer_id Identifier of the consumer.
     * @param[in] events Input parameter.
     * @return True when the operation succeeds.
     */
    bool trackDelivery(const std::string& consumer_id,
                       const std::vector<Changefeed::ChangeEvent>& events);

    /**
     * @brief Acknowledge.
     * @param[in] consumer_id Identifier of the consumer.
     * @param[in] sequence Input parameter.
     * @return True when the operation succeeds.
     */
    bool acknowledge(const std::string& consumer_id, uint64_t sequence);

    /**
     * @brief Acknowledge Up To.
     * @param[in] consumer_id Identifier of the consumer.
     * @param[in] up_to_sequence Input parameter.
     * @return Return value.
     */
    size_t acknowledgeUpTo(const std::string& consumer_id, uint64_t up_to_sequence);

    std::vector<Changefeed::ChangeEvent> getPendingRedelivery(
        const std::string& consumer_id,
        std::optional<std::chrono::milliseconds> timeout_override = std::nullopt);

    /**
     * @brief Remove Consumer.
     * @param[in] consumer_id Identifier of the consumer.
     */
    void removeConsumer(const std::string& consumer_id);

    /**
     * @brief Get Stats.
     * @param[in] consumer_id Identifier of the consumer.
     * @return Return value.
     */
    std::optional<ConsumerDeliveryStats> getStats(const std::string& consumer_id) const;

    /**
     * @brief Get All Stats.
     * @return Return value.
     */
    std::vector<ConsumerDeliveryStats> getAllStats() const;

    /**
     * @brief Consumer Count.
     * @return Return value.
     */
    size_t consumerCount() const;

private:
    struct PendingEvent {
        Changefeed::ChangeEvent event;
        std::chrono::steady_clock::time_point delivered_at;
        uint32_t attempt{1};  ///< Delivery attempt count (1 = first delivery)
    };

    struct ConsumerState {
        std::map<uint64_t, PendingEvent> pending;
        uint64_t total_delivered{0};
        uint64_t total_acknowledged{0};
        uint64_t total_redeliveries{0};
        uint64_t total_expired{0};
    };

    DeliveryTrackerConfig config_;
    RedeliveryCallback redelivery_callback_;

    mutable std::mutex mutex_;
    std::map<std::string, ConsumerState> consumers_;

    std::atomic<bool> running_{false};
    std::thread redelivery_thread_;
    std::condition_variable cv_;
    std::mutex cv_mutex_;

    /**
     * @brief Redelivery Thread Func.
     */
    void redeliveryThreadFunc();
    /**
     * @brief Check And Redeliver.
     */
    void checkAndRedeliver();
};

} // namespace cdc
} // namespace themis
