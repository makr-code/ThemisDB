/**
 * @file ws_transport.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief WebSocket transport for CDC change event delivery.
 *
 * Manages named CDC subscriptions for WebSocket sessions.  Event delivery is
 * decoupled from network I/O: the server layer supplies two callbacks
 * (SendFn / CloseFn) so this class can be unit-tested without live sockets.
 */
class WsTransport {
public:
    /// Called to send a text message to a WebSocket session.
    using SendFn = std::function<void(const std::string& session_id,
                                     const std::string& message)>;

    /// Called to close a WebSocket session (e.g. on back-pressure overflow).
    using CloseFn = std::function<void(const std::string& session_id)>;

    /// Per-subscription filter options.
    struct SubscriptionFilter {
        std::string key_prefix;                          ///< Key prefix filter (empty = all)
        uint64_t from_sequence = 0;                      ///< Deliver events with sequence > this
        std::set<Changefeed::ChangeEventType> event_types; ///< Empty = all event types
    };

    /// Transport-level statistics.
    /// Note: total_overflow_closes maps to the Prometheus counter
    /// cdc_ws_overflow_total; total_events_delivered maps to
    /// cdc_ws_events_delivered_total.
    struct Stats {
        size_t active_sessions = 0;
        size_t total_subscriptions = 0;
        uint64_t total_events_delivered = 0;  ///< cdc_ws_events_delivered_total
        uint64_t total_overflow_closes = 0;   ///< cdc_ws_overflow_total
        uint64_t total_poll_cycles = 0;
    };

    /// Maximum queued events per session before the session is closed (WS 1011).
    static constexpr size_t kMaxPendingEvents = 1000;

    /// Default CDC polling interval in milliseconds.
    static constexpr uint32_t kDefaultPollIntervalMs = 500;

    /**
     * @brief Construct WsTransport.
     * @param changefeed  Changefeed instance (not owned; must outlive transport).
     * @param poll_interval_ms  Background polling interval in milliseconds.
     * @param metrics     Optional CDCMetrics instance for Prometheus counter
     *                    updates (ws_overflow_total, ws_events_delivered).
     *                    Not owned; must outlive transport if non-null.
     */
    explicit WsTransport(Changefeed* changefeed,
                         uint32_t poll_interval_ms = kDefaultPollIntervalMs,
                         cdc::CDCMetrics* metrics = nullptr);

    ~WsTransport();

    // Non-copyable, non-movable (owns a timer and atomic state).
    WsTransport(const WsTransport&) = delete;
    WsTransport& operator=(const WsTransport&) = delete;

    // ── Session lifecycle ──────────────────────────────────────────────────

    /**
     * @brief Register a new WebSocket session with the transport.
     * @param session_id  Unique session identifier (e.g. UUID string).
     */
    void addSession(const std::string& session_id);

    /**
     * @brief Unregister a WebSocket session and remove all its subscriptions.
     * @param session_id  Session to remove.
     */
    void removeSession(const std::string& session_id);

    // ── Subscription management ────────────────────────────────────────────

    /**
     * @brief Add a named CDC subscription for a session.
     *
     * If @p sub_id already exists for the session the subscription is
     * replaced (re-subscribe semantics).
     *
     * @param session_id  Owning session.
     * @param sub_id      Client-provided subscription identifier.
     * @param filter      Event filter.
     */
    void subscribe(const std::string& session_id,
                   const std::string& sub_id,
                   const SubscriptionFilter& filter);

    /**
     * @brief Remove a named subscription from a session.
     * @param session_id  Owning session.
     * @param sub_id      Subscription identifier to remove.
     */
    void unsubscribe(const std::string& session_id, const std::string& sub_id);

    // ── Event delivery ─────────────────────────────────────────────────────

    /**
     * @brief Poll changefeed and deliver pending events to all subscribed sessions.
     *
     * Typically called from a background timer (see startPolling) or directly
     * from a test.  For each subscription, fetches new events and calls
     * @p send_fn to deliver them.  On back-pressure overflow @p close_fn is
     * called and the session is removed.
     *
     * @param send_fn   Callback to send a message; must be thread-safe.
     * @param close_fn  Callback to close a session; must be thread-safe.
     *                  May be null if overflow handling is not required.
     */
    void pollAndDeliver(const SendFn& send_fn, const CloseFn& close_fn = {});

    // ── Background polling ─────────────────────────────────────────────────

    /**
     * @brief Start background polling on the supplied io_context.
     *
     * Schedules a recurring timer that calls pollAndDeliver every
     * poll_interval_ms milliseconds.  The callbacks are captured for the
     * lifetime of the polling loop.
     *
     * @param ioc       Boost.Asio io_context (must remain alive while polling).
     * @param send_fn   Message send callback.
     * @param close_fn  Session close callback (may be null).
     */
    void startPolling(boost::asio::io_context& ioc,
                      SendFn send_fn,
                      CloseFn close_fn = {});

    /**
     * @brief Stop background polling.
     */
    void stopPolling();

    // ── Observability ──────────────────────────────────────────────────────

    /**
     * @brief Return a snapshot of current transport statistics.
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

    void scheduleNextPoll();
};

} // namespace cdc
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
