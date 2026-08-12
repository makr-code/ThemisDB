/**
 * @file delivery_tracker.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Configuration for DeliveryTracker.
 */
struct DeliveryTrackerConfig {
    /// Time after which an unacknowledged event is eligible for redelivery.
    std::chrono::milliseconds ack_timeout{30000};

    /// How often the background redelivery thread checks for timed-out events.
    std::chrono::milliseconds recheck_interval{5000};

    /// Maximum number of redelivery attempts before an event is considered
    /// permanently failed (0 = unlimited).
    uint32_t max_redelivery_attempts{0};

    /// Maximum number of pending (unacknowledged) events tracked per consumer.
    /// New deliveries are rejected once the limit is reached (returns false).
    size_t max_pending_per_consumer{10000};
};

/**
 * @brief Statistics for a single consumer's delivery state.
 */
struct ConsumerDeliveryStats {
    std::string consumer_id;
    size_t pending_count{0};           ///< Events awaiting acknowledgement
    uint64_t total_delivered{0};       ///< Cumulative events delivered
    uint64_t total_acknowledged{0};    ///< Cumulative events acknowledged
    uint64_t total_redeliveries{0};    ///< Cumulative redelivery attempts
    uint64_t total_expired{0};         ///< Events dropped after max_redelivery_attempts
};

/**
 * @brief At-least-once delivery tracker for CDC change events.
 *
 * Example usage:
 * @code
 *   DeliveryTracker tracker(config, [&](const std::string& cid, const std::vector<Changefeed::ChangeEvent>& evts){
 *       // re-send evts to consumer cid
 *   });
 *   tracker.start();
 *
 *   // On event dispatch:
 *   tracker.trackDelivery("consumer-1", events);
 *
 *   // On consumer ACK received:
 *   tracker.acknowledge("consumer-1", sequence);
 *
 *   tracker.stop();
 * @endcode
 */
class DeliveryTracker {
public:
    /**
     * @brief Callback invoked when events require redelivery.
     *
     * Called from the background redelivery thread with (consumer_id, events_to_redeliver).
     */
    using RedeliveryCallback =
        std::function<void(const std::string& consumer_id,
                           const std::vector<Changefeed::ChangeEvent>& events)>;

    /**
     * @brief Construct a DeliveryTracker.
     *
     * @param config     Configuration parameters.
     * @param callback   Optional callback invoked for unacknowledged events.
     *                   If not provided, getPendingRedelivery() must be polled manually.
     */
    explicit DeliveryTracker(DeliveryTrackerConfig config = {},
                             RedeliveryCallback callback = nullptr);

    ~DeliveryTracker();

    // Non-copyable, non-movable (owns thread + mutex state)
    DeliveryTracker(const DeliveryTracker&) = delete;
    DeliveryTracker& operator=(const DeliveryTracker&) = delete;

    /**
     * @brief Start the background redelivery thread.
     *
     * Must be called before the tracker is used if a RedeliveryCallback was provided.
     * Safe to call multiple times (second call is a no-op).
     */
    void start();

    /**
     * @brief Stop the background redelivery thread.
     *
     * Waits for the thread to finish. After stop(), start() may be called again.
     */
    void stop();

    /**
     * @brief Record that a batch of events has been delivered to a consumer.
     *
     * Each event is stored as pending until acknowledged or expired.
     *
     * @param consumer_id  Opaque consumer identifier.
     * @param events       Events that were dispatched to the consumer.
     * @return true if all events were tracked; false if the per-consumer pending
     *         limit would be exceeded (delivery should be rejected or back-pressured).
     */
    bool trackDelivery(const std::string& consumer_id,
                       const std::vector<Changefeed::ChangeEvent>& events);

    /**
     * @brief Acknowledge receipt of a single event by sequence number.
     *
     * Removes the event from the pending set for the given consumer.
     *
     * @param consumer_id  Consumer that processed the event.
     * @param sequence     Sequence number of the acknowledged event.
     * @return true if the event was found and removed; false if it was unknown
     *         (already acked, expired, or never tracked).
     */
    bool acknowledge(const std::string& consumer_id, uint64_t sequence);

    /**
     * @brief Acknowledge all events up to and including a sequence number.
     *
     * Useful for cumulative acknowledgement (similar to TCP ACK).
     *
     * @param consumer_id      Consumer that processed events.
     * @param up_to_sequence   Highest sequence number to acknowledge (inclusive).
     * @return Number of events removed from the pending set.
     */
    size_t acknowledgeUpTo(const std::string& consumer_id, uint64_t up_to_sequence);

    /**
     * @brief Return events that have exceeded their ack timeout and need redelivery.
     *
     * Events exceeding max_redelivery_attempts (if > 0) are discarded rather
     * than returned.  Each call resets the delivery timestamp of returned events
     * so they are not returned again immediately.
     *
     * @param consumer_id      Consumer whose pending events to check.
     * @param timeout_override If provided, overrides the configured ack_timeout for
     *                         this call only (useful for per-request timeout control).
     * @return Events ready for redelivery (may be empty).
     */
    std::vector<Changefeed::ChangeEvent> getPendingRedelivery(
        const std::string& consumer_id,
        std::optional<std::chrono::milliseconds> timeout_override = std::nullopt);

    /**
     * @brief Remove all tracked state for a consumer (e.g. on disconnect).
     *
     * @param consumer_id  Consumer to remove.
     */
    void removeConsumer(const std::string& consumer_id);

    /**
     * @brief Return delivery statistics for a consumer.
     *
     * @param consumer_id  Consumer to query.
     * @return Stats, or nullopt if the consumer is unknown.
     */
    std::optional<ConsumerDeliveryStats> getStats(const std::string& consumer_id) const;

    /**
     * @brief Return statistics for all tracked consumers.
     */
    std::vector<ConsumerDeliveryStats> getAllStats() const;

    /**
     * @brief Return the number of currently tracked consumers.
     */
    size_t consumerCount() const;

private:
    struct PendingEvent {
        Changefeed::ChangeEvent event;
        std::chrono::steady_clock::time_point delivered_at;
        uint32_t attempt{1};  ///< Delivery attempt count (1 = first delivery)
    };

    struct ConsumerState {
        /// Pending events keyed by sequence number for O(log n) lookup.
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

    void redeliveryThreadFunc();
    void checkAndRedeliver();
};

} // namespace cdc
} // namespace themis
