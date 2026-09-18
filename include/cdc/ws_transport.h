/**
 * @file ws_transport.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB - CDC WebSocket Transport
 *
 * Provides WebSocket-based delivery of CDC change events as an alternative
 * to the SSE-based SseConnectionManager.  Unlike SSE (server-push only),
 * WebSocket connections support bidirectional control frames so clients can
 * dynamically add or remove named subscriptions without reconnecting.
 *
 * Protocol (JSON frames):
 *   Client → Server  subscribe:
 *     {"action":"subscribe","id":"sub-1","key_prefix":"orders:",
 *      "from_sequence":0,"event_types":["PUT","DELETE"]}
 *
 *   Server → Client  ack:
 *     {"action":"subscribed","id":"sub-1"}
 *
 *   Client → Server  unsubscribe:
 *     {"action":"unsubscribe","id":"sub-1"}
 *
 *   Server → Client  change event (mirrors ChangeEvent::toJson()):
 *     {"type":"cdc_event","subscription_id":"sub-1","sequence":10042,
 *      "key":"orders:US-1234","value":"...","timestamp_ms":...}
 *
 * Back-pressure: when a session's pending-event counter reaches
 * kMaxPendingEvents the session is closed with WebSocket code 1011 and the
 * overflow counter is incremented.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#pragma once

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "cdc/changefeed.h"
#include "cdc/cdc_metrics.h"
#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <unordered_map>

namespace themis {
namespace cdc {

class WsTransport {
public:
    using SendFn = std::function<void(const std::string& session_id,
                                     const std::string& message)>;

    using CloseFn = std::function<void(const std::string& session_id)>;

    struct SubscriptionFilter {
        std::string key_prefix;                          ///< Key prefix filter (empty = all)
        uint64_t from_sequence = 0;                      ///< Deliver events with sequence > this
        std::set<Changefeed::ChangeEventType> event_types; ///< Empty = all event types
    };

    struct Stats {
        size_t active_sessions = 0;
        size_t total_subscriptions = 0;
        uint64_t total_events_delivered = 0;  ///< cdc_ws_events_delivered_total
        uint64_t total_overflow_closes = 0;   ///< cdc_ws_overflow_total
        uint64_t total_poll_cycles = 0;
    };

    static constexpr size_t kMaxPendingEvents = 1000;

    static constexpr uint32_t kDefaultPollIntervalMs = 500;

    explicit WsTransport(Changefeed* changefeed,
                         uint32_t poll_interval_ms = kDefaultPollIntervalMs,
                         cdc::CDCMetrics* metrics = nullptr);

    ~WsTransport();

    // Non-copyable, non-movable (owns a timer and atomic state).
    WsTransport(const WsTransport&) = delete;
    WsTransport& operator=(const WsTransport&) = delete;

    /**
     * @brief ── Session lifecycle ──────────────────────────────────────────────────
     * @param[in] session_id Identifier of the session.
     */

    void addSession(const std::string& session_id);

    /**
     * @brief Remove Session.
     * @param[in] session_id Identifier of the session.
     */
    void removeSession(const std::string& session_id);

    /**
     * @brief ── Subscription management ────────────────────────────────────────────
     * @param[in] session_id Identifier of the session.
     * @param[in] sub_id Identifier of the sub.
     * @param[in] filter Input parameter.
     */

    void subscribe(const std::string& session_id,
                   const std::string& sub_id,
                   const SubscriptionFilter& filter);

    /**
     * @brief Unsubscribe.
     * @param[in] session_id Identifier of the session.
     * @param[in] sub_id Identifier of the sub.
     */
    void unsubscribe(const std::string& session_id, const std::string& sub_id);

    // ── Event delivery ─────────────────────────────────────────────────────

    void pollAndDeliver(const SendFn& send_fn, const CloseFn& close_fn = {});

    // ── Background polling ─────────────────────────────────────────────────

    void startPolling(boost::asio::io_context& ioc,
                      SendFn send_fn,
                      CloseFn close_fn = {});

    /**
     * @brief Stop Polling.
     */
    void stopPolling();

    /**
     * @brief ── Observability ──────────────────────────────────────────────────────
     * @return Return value.
     */

    Stats getStats() const;

private:
    // ── Internal data structures ───────────────────────────────────────────

    struct Subscription {
        std::string id = {};
        SubscriptionFilter filter;
        uint64_t last_sent_sequence = 0; ///< Last delivered sequence number
    };

    struct Session {
        std::string id = {};
        std::unordered_map<std::string, Subscription> subscriptions; ///< sub_id → Subscription
        size_t pending_events = 0; ///< Back-pressure counter
    };

    // ── Members ────────────────────────────────────────────────────────────

    Changefeed* changefeed_;
    uint32_t poll_interval_ms_;
    cdc::CDCMetrics* metrics_; ///< Optional external metrics sink (not owned)

    mutable std::mutex mutex_;
    std::unordered_map<std::string, Session> sessions_; ///< session_id → Session

    // Background polling
    std::unique_ptr<boost::asio::steady_timer> poll_timer_;
    std::atomic<bool> polling_active_{false};
    SendFn bg_send_fn_;
    CloseFn bg_close_fn_;

    // Counters (atomic; updated without holding mutex_)
    std::atomic<uint64_t> total_events_delivered_{0};
    std::atomic<uint64_t> total_overflow_closes_{0};
    std::atomic<uint64_t> total_poll_cycles_{0};

    /**
     * @brief Schedule Next Poll.
     */
    void scheduleNextPoll();
};

} // namespace cdc
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
