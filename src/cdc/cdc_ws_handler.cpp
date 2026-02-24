/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cdc_ws_handler.cpp                                 ║
  Module:          cdc                                                ║
  Description:     CDC WebSocket subscription manager for the         ║
                   /v2/cdc/stream endpoint with at-least-once         ║
                   delivery guarantees                                 ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "cdc/cdc_ws_handler.h"
#include "utils/logger.h"

#include <chrono>

namespace themis {
namespace cdc {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// handleFrame
// ---------------------------------------------------------------------------

std::vector<json> CdcWebSocketHandler::handleFrame(const json& frame) {
    std::vector<json> responses;

    const std::string action = frame.value("action", "");
    const std::string id     = frame.value("id", "");

    if (action == "subscribe") {
        if (id.empty()) {
            responses.push_back({{"action", "error"},
                                  {"message", "subscribe requires 'id'"}});
            return responses;
        }

        Subscription sub;
        sub.id = id;

        // "collection" maps to key_prefix as "collection:" (convention used
        // across all CDC endpoints to scope events to a document collection).
        if (frame.contains("collection") && frame["collection"].is_string()) {
            sub.key_prefix = frame["collection"].get<std::string>() + ":";
        }
        // An explicit key_prefix overrides the collection-derived one.
        if (frame.contains("key_prefix") && frame["key_prefix"].is_string()) {
            sub.key_prefix = frame["key_prefix"].get<std::string>();
        }

        // Parse optional event_types filter.
        if (frame.contains("event_types") && frame["event_types"].is_array()) {
            for (const auto& et : frame["event_types"]) {
                if (!et.is_string()) continue;
                const std::string s = et.get<std::string>();
                if (s == "PUT")
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
                else if (s == "DELETE")
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_DELETE);
                else if (s == "TRANSACTION_COMMIT")
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT);
                else if (s == "TRANSACTION_ROLLBACK")
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK);
            }
        }

        // Optional: resume from a specific sequence.
        if (frame.contains("from_sequence") && frame["from_sequence"].is_number_unsigned()) {
            const uint64_t from_seq = frame["from_sequence"].get<uint64_t>();
            // last_acked and last_sent are set so the first poll fetches from
            // from_seq onwards (listEvents excludes the from_sequence itself).
            sub.last_acked_sequence = (from_seq > 0) ? (from_seq - 1) : 0;
            sub.last_sent_sequence  = sub.last_acked_sequence;
        }

        const std::string key_prefix_copy = sub.key_prefix;
        {
            std::lock_guard<std::mutex> lock(mu_);
            subscriptions_[id] = std::move(sub);
        }

        THEMIS_INFO("CdcWebSocketHandler: subscription '{}' created (prefix='{}')",
                    id, key_prefix_copy);

        responses.push_back({{"action", "subscribed"}, {"id", id}});
    }
    else if (action == "unsubscribe") {
        if (id.empty()) {
            responses.push_back({{"action", "error"},
                                  {"message", "unsubscribe requires 'id'"}});
            return responses;
        }

        bool removed;
        {
            std::lock_guard<std::mutex> lock(mu_);
            removed = subscriptions_.erase(id) > 0;
        }

        THEMIS_INFO("CdcWebSocketHandler: subscription '{}' {}",
                    id, removed ? "removed" : "not found");

        responses.push_back({{"action", "unsubscribed"}, {"id", id}});
    }
    else if (action == "ack") {
        if (id.empty()) {
            responses.push_back({{"action", "error"},
                                  {"message", "ack requires 'id'"}});
            return responses;
        }

        const uint64_t acked_seq = frame.value("sequence", uint64_t(0));

        {
            std::lock_guard<std::mutex> lock(mu_);
            auto it = subscriptions_.find(id);
            if (it != subscriptions_.end()) {
                auto& sub = it->second;
                if (acked_seq > sub.last_acked_sequence) {
                    // Drain the pending-ack queue up to acked_seq.
                    while (!sub.pending_ack.empty()) {
                        const uint64_t seq =
                            sub.pending_ack.front().frame.value("sequence", uint64_t(0));
                        if (seq <= acked_seq) {
                            sub.pending_ack.pop_front();
                        } else {
                            break;
                        }
                    }
                    sub.last_acked_sequence = acked_seq;
                    THEMIS_DEBUG("CdcWebSocketHandler: ack '{}' up to seq {}",
                                 id, acked_seq);
                }
            }
        }
        // No response frame for ack; the client is only notified on error.
    }
    else {
        responses.push_back({{"action", "error"},
                              {"message", "unknown action: " + action}});
    }

    return responses;
}

// ---------------------------------------------------------------------------
// pollEvents
// ---------------------------------------------------------------------------

std::vector<json> CdcWebSocketHandler::pollEvents(Changefeed& feed) {
    std::vector<json> frames;

    std::lock_guard<std::mutex> lock(mu_);

    for (auto& [id, sub] : subscriptions_) {
        // Back-pressure: pause delivery when the pending-ack window is full.
        // Record the cdc_ws_overflow_total metric each time this fires.
        if (sub.pending_ack.size() >= max_pending_ack_) {
            ws_overflow_total_.fetch_add(1, std::memory_order_relaxed);
            THEMIS_WARN("CdcWebSocketHandler: subscription '{}' pending_ack full "
                        "({}), pausing delivery (cdc_ws_overflow_total={})",
                        id, sub.pending_ack.size(),
                        ws_overflow_total_.load(std::memory_order_relaxed));
            continue;
        }

        Changefeed::ListOptions opts;
        opts.from_sequence = sub.last_sent_sequence;
        opts.limit         = 100;
        opts.long_poll_ms  = 0; // Non-blocking; caller drives the poll interval.
        if (!sub.key_prefix.empty()) {
            opts.key_prefix = sub.key_prefix;
        }
        if (!sub.event_types.empty()) {
            opts.event_types = sub.event_types;
        }

        try {
            auto events      = feed.listEvents(opts);
            const auto now   = std::chrono::steady_clock::now();

            for (const auto& ev : events) {
                // Re-check back-pressure within the batch.
                if (sub.pending_ack.size() >= max_pending_ack_) break;

                auto event_frame = buildEventFrame(ev, id);
                sub.pending_ack.push_back({event_frame, now});
                sub.last_sent_sequence = ev.sequence;
                frames.push_back(std::move(event_frame));
            }
        } catch (const std::exception& e) {
            THEMIS_ERROR("CdcWebSocketHandler: poll error for subscription '{}': {}",
                         id, e.what());
        }
    }

    return frames;
}

// ---------------------------------------------------------------------------
// checkRedelivery
// ---------------------------------------------------------------------------

std::vector<json> CdcWebSocketHandler::checkRedelivery() {
    std::vector<json> frames;

    const auto now     = std::chrono::steady_clock::now();
    const auto timeout = std::chrono::milliseconds(kRedeliveryTimeoutMs);

    std::lock_guard<std::mutex> lock(mu_);

    for (auto& [id, sub] : subscriptions_) {
        if (sub.pending_ack.empty()) continue;

        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - sub.pending_ack.front().sent_at);

        if (age >= timeout) {
            THEMIS_INFO("CdcWebSocketHandler: redelivering {} event(s) for "
                        "subscription '{}' (oldest={}ms)",
                        sub.pending_ack.size(), id, age.count());

            // Redeliver all pending events and reset their sent-at timestamps
            // so the redelivery timer restarts after this batch.
            for (auto& pending : sub.pending_ack) {
                pending.sent_at = now;
                frames.push_back(pending.frame);
            }
        }
    }

    return frames;
}

// ---------------------------------------------------------------------------
// hasSubscriptions
// ---------------------------------------------------------------------------

bool CdcWebSocketHandler::hasSubscriptions() const {
    std::lock_guard<std::mutex> lock(mu_);
    return !subscriptions_.empty();
}

// ---------------------------------------------------------------------------
// buildEventFrame (static helper)
// ---------------------------------------------------------------------------

json CdcWebSocketHandler::buildEventFrame(const Changefeed::ChangeEvent& ev,
                                          const std::string& sub_id) {
    // Start from the canonical ChangeEvent JSON representation so the format
    // is identical to the SSE transport and matches the external contract
    // documented in FUTURE_ENHANCEMENTS.md.
    auto frame = ev.toJson();
    frame["sub_id"] = sub_id;
    return frame;
}

} // namespace cdc
} // namespace themis
