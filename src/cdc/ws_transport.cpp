/**
 * @file ws_transport.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=2, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


/**
 * ThemisDB - CDC WebSocket Transport (implementation)
 *
 * See include/cdc/ws_transport.h for class documentation.
 *
 * Copyright (c) 2025 ThemisDB Project
 * SPDX-License-Identifier: Apache-2.0
 */

#ifdef THEMIS_ENABLE_WEBSOCKET

#include "cdc/ws_transport.h"
#include "utils/logger.h"
#include <nlohmann/json.hpp>

namespace themis {
namespace cdc {

using json = nlohmann::json;

// ── Construction / destruction ────────────────────────────────────────────────

WsTransport::WsTransport(Changefeed* changefeed, uint32_t poll_interval_ms,
                         cdc::CDCMetrics* metrics)
    : changefeed_(changefeed)
    , poll_interval_ms_(poll_interval_ms)
    , metrics_(metrics)
{
    THEMIS_INFO("WsTransport created (poll_interval={}ms)", poll_interval_ms_);
}

WsTransport::~WsTransport() {
    stopPolling();
}

// ── Session lifecycle ─────────────────────────────────────────────────────────

void WsTransport::addSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (sessions_.find(session_id) == sessions_.end()) {
        sessions_.emplace(session_id, Session{session_id, {}, 0});
        THEMIS_INFO("WsTransport: session registered: {}", session_id);
    }
}

void WsTransport::removeSession(const std::string& session_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it != sessions_.end()) {
        THEMIS_INFO("WsTransport: session removed: {} ({} subscriptions)",
                    session_id, it->second.subscriptions.size());
        sessions_.erase(it);
    }
}

// ── Subscription management ───────────────────────────────────────────────────

void WsTransport::subscribe(const std::string& session_id,
                            const std::string& sub_id,
                            const SubscriptionFilter& filter) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        THEMIS_WARN("WsTransport::subscribe: unknown session {}", session_id);
        return;
    }

    Subscription sub;
    sub.id = sub_id;
    sub.filter = filter;
    // last_sent_sequence is set to from_sequence so the first poll retrieves
    // events with sequence strictly greater than from_sequence.
    sub.last_sent_sequence = filter.from_sequence;

    it->second.subscriptions[sub_id] = std::move(sub);
    THEMIS_INFO("WsTransport: session {} subscribed '{}' (prefix='{}', from_seq={}, event_types={})",
                session_id, sub_id, filter.key_prefix, filter.from_sequence,
                filter.event_types.size());
}

void WsTransport::unsubscribe(const std::string& session_id,
                              const std::string& sub_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = sessions_.find(session_id);
    if (it == sessions_.end()) {
        return;
    }
    if (it->second.subscriptions.erase(sub_id)) {
        THEMIS_INFO("WsTransport: session {} unsubscribed '{}'", session_id, sub_id);
    }
}

// ── Event delivery ────────────────────────────────────────────────────────────

void WsTransport::pollAndDeliver(const SendFn& send_fn, const CloseFn& close_fn) {
    if (!changefeed_) {
        total_poll_cycles_++;
        return;
    }

    total_poll_cycles_++;

    // Phase 1: snapshot the active subscriptions (fast, under lock).
    struct QueryItem {
        std::string session_id = {};
        std::string sub_id = {};
        Changefeed::ListOptions opts;
    };
    std::vector<QueryItem> query_items;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (const auto& [sid, session] : sessions_) {
            for (const auto& [sub_id, sub] : session.subscriptions) {
                Changefeed::ListOptions opts;
                opts.from_sequence = sub.last_sent_sequence;
                opts.limit = 100;
                opts.long_poll_ms = 0; // non-blocking
                if (!sub.filter.key_prefix.empty()) {
                    opts.key_prefix = sub.filter.key_prefix;
                }
                if (!sub.filter.event_types.empty()) {
                    opts.event_types = sub.filter.event_types;
                }
                query_items.push_back({sid, sub_id, std::move(opts)});
            }
        }
    }

    if (query_items.empty()) {
        return;
    }

    // Phase 2: query changefeed without holding the mutex (may take time).
    struct QueryResult {
        std::string session_id = {};
        std::string sub_id = {};
        std::vector<Changefeed::ChangeEvent> events;
    };
    std::vector<QueryResult> results;

    for (auto& item : query_items) {
        try {
            auto events = changefeed_->listEvents(item.opts);
            if (!events.empty()) {
                results.push_back({item.session_id, item.sub_id, std::move(events)});
            }
        } catch (const std::exception& ex) {
            THEMIS_ERROR("WsTransport: changefeed query failed for session {} sub {}: {}",
                         item.session_id, item.sub_id, ex.what());
        }
    }

    if (results.empty()) {
        return;
    }

    // Phase 3: apply back-pressure checks and update sequence pointers (under lock).
    struct Delivery {
        std::string session_id = {};
        std::string sub_id = {};
        std::vector<Changefeed::ChangeEvent> events;
    };
    std::vector<Delivery> deliveries;
    std::vector<std::string> overflow_sessions;

    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto& result : results) {
            auto sit = sessions_.find(result.session_id);
            if (sit == sessions_.end()) {
                continue; // session was removed while we queried
            }
            Session& session = sit->second;

            auto sub_it = session.subscriptions.find(result.sub_id);
            if (sub_it == session.subscriptions.end()) {
                continue; // subscription was removed while we queried
            }

            // Back-pressure: close and remove sessions that cannot accept more events.
            if (session.pending_events + static_cast<int>(result.events.size()) > kMaxPendingEvents) {
                THEMIS_WARN("WsTransport: session {} back-pressure limit reached "
                            "(pending={}, new={}), closing with 1011",
                            result.session_id, session.pending_events,
                            result.events.size());
                if (std::find(overflow_sessions.begin(), overflow_sessions.end(),
                              result.session_id) == overflow_sessions.end()) {
                    overflow_sessions.push_back(result.session_id);
                }
                continue;
            }

            sub_it->second.last_sent_sequence = result.events.back().sequence;
            session.pending_events += result.events.size();
            deliveries.push_back({result.session_id, result.sub_id, std::move(result.events)});
        }
    }

    // Phase 4: deliver events outside the lock.
    for (auto& delivery : deliveries) {
        for (const auto& event : delivery.events) {
            // Each frame matches ChangeEvent::toJson() extended with transport metadata.
            json frame = event.toJson();
            frame["type"] = "cdc_event";
            frame["subscription_id"] = delivery.sub_id;

            try {
                send_fn(delivery.session_id, frame.dump());
                total_events_delivered_++;
                if (metrics_) {
                    metrics_->ws_events_delivered++;
                }
            } catch (const std::exception& ex) {
                THEMIS_ERROR("WsTransport: send_fn threw for session {}: {}",
                             delivery.session_id, ex.what());
            }
        }

        // Decrement the pending counter now that we have handed off the events.
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = sessions_.find(delivery.session_id);
            if (it != sessions_.end()) {
                const size_t delivered = delivery.events.size();
                if (it->second.pending_events >= delivered) {
                    it->second.pending_events -= delivered;
                } else {
                    it->second.pending_events = 0;
                }
            }
        }
    }

    // Phase 5: close and remove overflow sessions.
    for (const auto& sid : overflow_sessions) {
        total_overflow_closes_++;
        if (metrics_) {
            metrics_->ws_overflow_total++;
        }
        if (close_fn) {
            try {
                close_fn(sid);
            } catch (const std::exception& ex) {
                THEMIS_ERROR("WsTransport: close_fn threw for session {}: {}", sid, ex.what());
            }
        }
        removeSession(sid);
    }
}

// ── Background polling ────────────────────────────────────────────────────────

void WsTransport::startPolling(boost::asio::io_context& ioc,
                               SendFn send_fn,
                               CloseFn close_fn) {
    if (polling_active_.load()) {
        THEMIS_WARN("WsTransport: polling already active");
        return;
    }

    bg_send_fn_ = std::move(send_fn);
    bg_close_fn_ = std::move(close_fn);
    polling_active_ = true;
    poll_timer_ = std::make_unique<boost::asio::steady_timer>(ioc);

    THEMIS_INFO("WsTransport: background polling started (interval={}ms)", poll_interval_ms_);
    scheduleNextPoll();
}

void WsTransport::stopPolling() {
    if (polling_active_.exchange(false)) {
        if (poll_timer_) {
            poll_timer_->cancel();
            poll_timer_.reset();
        }
        THEMIS_INFO("WsTransport: background polling stopped");
    }
}

void WsTransport::scheduleNextPoll() {
    if (!polling_active_.load() || !poll_timer_) {
        return;
    }

    poll_timer_->expires_after(std::chrono::milliseconds(poll_interval_ms_));
    poll_timer_->async_wait([this](const boost::system::error_code& ec) {
        if (ec || !polling_active_.load()) {
            return;
        }
        pollAndDeliver(bg_send_fn_, bg_close_fn_);
        scheduleNextPoll();
    });
}

// ── Observability ─────────────────────────────────────────────────────────────

WsTransport::Stats WsTransport::getStats() const {
    std::lock_guard<std::mutex> lock(mutex_);

    size_t total_subs = 0;
    for (const auto& [sid, session] : sessions_) {
        total_subs += session.subscriptions.size();
    }

    return Stats{
        sessions_.size(),
        total_subs,
        total_events_delivered_.load(),
        total_overflow_closes_.load(),
        total_poll_cycles_.load()
    };
}

} // namespace cdc
} // namespace themis

#endif // THEMIS_ENABLE_WEBSOCKET
