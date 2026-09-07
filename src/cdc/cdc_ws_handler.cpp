/**
 * @file cdc_ws_handler.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=0, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/cdc_ws_handler.h"

#include <chrono>

#include "utils/logger.h"

namespace themis {
namespace cdc {

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// handleFrame
// ---------------------------------------------------------------------------

std::vector<json> CdcWebSocketHandler::handleFrame(const json &frame) {
    std::vector<json> responses;

    const std::string action      = frame.value("action", "");
    const std::string id          = frame.value("id", "");
    const std::string group_id    = frame.value("group_id", "");
    const std::string consumer_id = frame.value("consumer_id", "");

    if (action == "subscribe") {
        // Derive the subscription key.  Consumer-group subscriptions use
        // "{group_id}:{consumer_id}" when no explicit "id" is supplied.
        std::string sub_key = id;
        if (sub_key.empty()) {
            if (!group_id.empty()) {
                sub_key = group_id + ":" + consumer_id;
            } else {
                responses.push_back({{"action", "error"}, {"message", "subscribe requires 'id' or 'group_id'"}});
                return responses;
            }
        }

        Subscription sub;
        sub.id          = sub_key;
        sub.group_id    = group_id;
        sub.consumer_id = consumer_id;

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
            for (const auto &et : frame["event_types"]) {
                if (!et.is_string()) {
                    continue;
                }
                const std::string s = et.get<std::string>();
                if (s == "PUT") {
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_PUT);
                } else if (s == "DELETE") {
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_DELETE);
                } else if (s == "TRANSACTION_COMMIT") {
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_TRANSACTION_COMMIT);
                } else if (s == "TRANSACTION_ROLLBACK") {
                    sub.event_types.insert(Changefeed::ChangeEventType::EVENT_TRANSACTION_ROLLBACK);
                }
            }
        }

        // Consumer-group mode: resume from the group's durable committed offset.
        // This lets a consumer reconnect and continue from where the group left
        // off without scanning the full change log (v1.8.0 acceptance criterion).
        if (!group_id.empty() && group_manager_ != nullptr) {
            try {
                const uint64_t committed = group_manager_->getCommittedOffset(group_id);
                sub.last_acked_sequence  = committed;
                sub.last_sent_sequence   = committed;
                THEMIS_INFO("CdcWebSocketHandler: group subscription '{}' resuming from "
                            "committed_sequence+1={}",
                            sub_key, committed + 1);
            } catch (const std::exception &e) {
                // Group may not exist yet; deliver from the start of the log.
                THEMIS_WARN("CdcWebSocketHandler: could not read committed offset for "
                            "group '{}': {} — delivering from sequence 0",
                            group_id, e.what());
            }
        }
        // Explicit from_sequence overrides the group committed offset.
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
            subscriptions_[sub_key] = std::move(sub);
        }

        THEMIS_INFO("CdcWebSocketHandler: subscription '{}' created (prefix='{}', group='{}')", sub_key,
                    key_prefix_copy, group_id);

        responses.push_back({{"action", "subscribed"}, {"id", sub_key}});
    } else if (action == "unsubscribe") {
        // Accept either "id" or derive from "group_id:consumer_id"
        const std::string unsub_key = !id.empty() ? id : (!group_id.empty() ? (group_id + ":" + consumer_id) : "");
        if (unsub_key.empty()) {
            responses.push_back({{"action", "error"}, {"message", "unsubscribe requires 'id' or 'group_id'"}});
            return responses;
        }

        bool removed = {};
        {
            std::lock_guard<std::mutex> lock(mu_);
            removed = subscriptions_.erase(unsub_key) > 0;
        }

        THEMIS_INFO("CdcWebSocketHandler: subscription '{}' {}", unsub_key, removed ? "removed" : "not found");

        responses.push_back({{"action", "unsubscribed"}, {"id", unsub_key}});
    } else if (action == "ack") {
        // Accept ack by explicit "id" or by "group_id" (v1.8.0 group protocol).
        // When acking by group_id, advance the durable committed offset in
        // ConsumerGroupManager so the group resumes correctly after reconnect.
        const uint64_t acked_seq = frame.value("sequence", uint64_t(0));

        if (!group_id.empty()) {
            // Group-level ack: find the subscription for this group/consumer pair
            // and advance the durable committed offset.
            // When consumer_id is absent the key is "group_id:" — this mirrors
            // the subscribe path where an empty consumer_id also produces "group_id:".
            const std::string ack_key = group_id + ":" + consumer_id;

            std::string matched_key = {};
            {
                std::lock_guard<std::mutex> lock(mu_);
                // Fast path: exact key match (typical case: one subscription per group).
                if (subscriptions_.count(ack_key)) {
                    matched_key = ack_key;
                } else {
                    // Fallback scan: handles the case where the client acked by
                    // group_id only (no consumer_id in the ack frame).  A single
                    // WebSocket connection typically holds only a handful of
                    // subscriptions so O(n) is acceptable here.
                    for (auto &[k, s] : subscriptions_) {
                        if (s.group_id == group_id) {
                            matched_key = k;
                            break;
                        }
                    }
                }
                if (!matched_key.empty()) {
                    auto &sub = subscriptions_.at(matched_key);
                    if (acked_seq > sub.last_acked_sequence) {
                        while (!sub.pending_ack.empty()) {
                            const uint64_t seq = sub.pending_ack.front().frame.value("sequence", uint64_t(0));
                            if (seq <= acked_seq) {
                                sub.pending_ack.pop_front();
                            } else {
                                break;
                            }
                        }
                        sub.last_acked_sequence = acked_seq;
                        THEMIS_DEBUG("CdcWebSocketHandler: group ack '{}' up to seq {}", group_id, acked_seq);
                    }
                }
            }
            // Persist the committed offset durably after releasing the lock.
            if (!matched_key.empty() && group_manager_ != nullptr) {
                try {
                    group_manager_->commitOffset(group_id, acked_seq);
                } catch (const std::exception &e) {
                    THEMIS_ERROR("CdcWebSocketHandler: commitOffset failed for group '{}': {}", group_id, e.what());
                }
            }
            // No response frame for ack.
        } else {
            // Legacy id-based ack.
            if (id.empty()) {
                responses.push_back({{"action", "error"}, {"message", "ack requires 'id' or 'group_id'"}});
                return responses;
            }

            {
                std::lock_guard<std::mutex> lock(mu_);
                auto it = subscriptions_.find(id);
                if (it != subscriptions_.end()) {
                    auto &sub = it->second;
                    if (acked_seq > sub.last_acked_sequence) {
                        // Drain the pending-ack queue up to acked_seq.
                        while (!sub.pending_ack.empty()) {
                            const uint64_t seq = sub.pending_ack.front().frame.value("sequence", uint64_t(0));
                            if (seq <= acked_seq) {
                                sub.pending_ack.pop_front();
                            } else {
                                break;
                            }
                        }
                        sub.last_acked_sequence = acked_seq;
                        // If the subscription belongs to a group, also persist
                        // the durable committed offset.
                        if (!sub.group_id.empty() && group_manager_ != nullptr) {
                            try {
                                group_manager_->commitOffset(sub.group_id, acked_seq);
                            } catch (const std::exception &e) {
                                THEMIS_ERROR("CdcWebSocketHandler: commitOffset failed for "
                                             "group '{}': {}",
                                             sub.group_id, e.what());
                            }
                        }
                        THEMIS_DEBUG("CdcWebSocketHandler: ack '{}' up to seq {}", id, acked_seq);
                    }
                }
            }
        }
        // No response frame for ack; the client is only notified on error.
    } else {
        responses.push_back({{"action", "error"}, {"message", "unknown action: " + action}});
    }

    return responses;
}

// ---------------------------------------------------------------------------
// pollEvents
// ---------------------------------------------------------------------------

std::vector<json> CdcWebSocketHandler::pollEvents(Changefeed &feed) {
    std::vector<json> frames;

    std::lock_guard<std::mutex> lock(mu_);

    for (auto &[id, sub] : subscriptions_) {
        // Back-pressure: pause delivery when the pending-ack window is full.
        // Record the cdc_ws_overflow_total metric each time this fires.
        if (sub.pending_ack.size() >= static_cast<size_t>(max_pending_ack_)) {
            ws_overflow_total_.fetch_add(1, std::memory_order_relaxed);
            THEMIS_WARN("CdcWebSocketHandler: subscription '{}' pending_ack full "
                        "({}), pausing delivery (cdc_ws_overflow_total={})",
                        id, sub.pending_ack.size(), ws_overflow_total_.load(std::memory_order_relaxed));
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
            auto events    = feed.listEvents(opts);
            const auto now = std::chrono::steady_clock::now();

            for (const auto &ev : events) {
                // Re-check back-pressure within the batch.
                if (sub.pending_ack.size() >= static_cast<size_t>(max_pending_ack_)) {
                    break;
                }

                // Consumer-group mode: filter events to the consumer's partition.
                // Each event's key is hashed modulo consumer_count; only events
                // whose partition matches this consumer's partition are delivered.
                if (!sub.group_id.empty() && group_manager_ != nullptr) {
                    try {
                        if (!group_manager_->consumerHandlesKey(sub.group_id, sub.consumer_id, ev.key)) {
                            continue; // Belongs to a different consumer's partition
                        }
                    } catch (const std::exception &e) {
                        THEMIS_WARN("CdcWebSocketHandler: consumer partition check failed for "
                                    "subscription '{}' (group='{}', consumer='{}'): {}",
                                    id, sub.group_id, sub.consumer_id, e.what());
                        // Group may have been deleted; fall through to deliver anyway
                    } catch (const std::string &e) {
                        THEMIS_WARN("CdcWebSocketHandler: consumer partition check failed for "
                                    "subscription '{}' (group='{}', consumer='{}'): {}",
                                    id, sub.group_id, sub.consumer_id, e);
                        // Group may have been deleted; fall through to deliver anyway
                    } catch (const char *e) {
                        THEMIS_WARN("CdcWebSocketHandler: consumer partition check failed for "
                                    "subscription '{}' (group='{}', consumer='{}'): {}",
                                    id, sub.group_id, sub.consumer_id, (e ? e : "<null>"));
                        // Group may have been deleted; fall through to deliver anyway
                    }
                }

                auto event_frame = buildEventFrame(ev, id);
                sub.pending_ack.push_back({event_frame, now});
                sub.last_sent_sequence = ev.sequence;
                frames.push_back(std::move(event_frame));
            }
        } catch (const std::exception &e) {
            THEMIS_ERROR("CdcWebSocketHandler: poll error for subscription '{}': {}", id, e.what());
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

    for (auto &[id, sub] : subscriptions_) {
        if (sub.pending_ack.empty()) {
            continue;
        }

        const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(now - sub.pending_ack.front().sent_at);

        if (age >= timeout) {
            THEMIS_INFO("CdcWebSocketHandler: redelivering {} event(s) for "
                        "subscription '{}' (oldest={}ms)",
                        sub.pending_ack.size(), id, age.count());

            // Redeliver all pending events and reset their sent-at timestamps
            // so the redelivery timer restarts after this batch.
            for (auto &pending : sub.pending_ack) {
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

json CdcWebSocketHandler::buildEventFrame(const Changefeed::ChangeEvent &ev, const std::string &sub_id) {
    // Start from the canonical ChangeEvent JSON representation so the format
    // is identical to the SSE transport and matches the external contract
    // documented in FUTURE_ENHANCEMENTS.md.
    auto frame      = ev.toJson();
    frame["sub_id"] = sub_id;
    return frame;
}

} // namespace cdc
} // namespace themis
