/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cdc_ws_handler.h                                   ║
  Module:          cdc                                                ║
  Description:     CDC WebSocket subscription manager for the         ║
                   /v2/cdc/stream endpoint with at-least-once         ║
                   delivery guarantees                                 ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include "cdc/changefeed.h"
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>
#include <deque>
#include <set>
#include <mutex>
#include <vector>
#include <chrono>

namespace themis {
namespace cdc {

/**
 * @brief Manages named CDC subscriptions for a single WebSocket connection.
 *
 * Implements the /v2/cdc/stream subscription protocol:
 *
 *   subscribe    {"action":"subscribe","id":"sub-1","collection":"orders",
 *                 "key_prefix":"US-","event_types":["PUT","DELETE"]}
 *   unsubscribe  {"action":"unsubscribe","id":"sub-1"}
 *   ack          {"action":"ack","id":"sub-1","sequence":10042}
 *
 * Multiple named subscriptions may be active simultaneously on one connection.
 * Each subscription tracks its own last-acked and last-sent sequence numbers.
 *
 * At-least-once delivery:
 *   Events are placed in a per-subscription pending-ack queue when sent.  The
 *   client must send an ack frame to advance the acked offset.  If no ack is
 *   received within kRedeliveryTimeoutMs, all pending events are redelivered
 *   from the oldest unacknowledged one.  When the pending queue reaches
 *   kMaxPendingAck entries, new events are withheld until the queue drains.
 *
 * @see FUTURE_ENHANCEMENTS.md – design constraints and protocol specification
 * @see src/cdc/cdc_ws_handler.cpp – implementation
 */
class CdcWebSocketHandler {
public:
    /// Maximum pending unacked events per subscription before pausing delivery.
    static constexpr size_t kMaxPendingAck = 1000;

    /// Time after which unacked events are redelivered (milliseconds).
    static constexpr int64_t kRedeliveryTimeoutMs = 5000;

    CdcWebSocketHandler() = default;

    /**
     * @brief Process a client control frame.
     *
     * Handles subscribe, unsubscribe, and ack actions.
     *
     * @param frame  Parsed JSON frame from the client.
     * @return       Response frames to send back to the client (may be empty).
     */
    std::vector<nlohmann::json> handleFrame(const nlohmann::json& frame);

    /**
     * @brief Poll the Changefeed for new events across all subscriptions.
     *
     * Called periodically by the WebSocket polling loop.  For each
     * subscription that is not back-pressured, fetches new events since
     * last_sent_sequence and queues them as pending-ack.
     *
     * @param feed  Changefeed to poll (must outlive this call).
     * @return      Event frames ready to be written to the WebSocket.
     */
    std::vector<nlohmann::json> pollEvents(Changefeed& feed);

    /**
     * @brief Check for unacknowledged events that need redelivery.
     *
     * Redelivers events older than kRedeliveryTimeoutMs.  Resets the sent-at
     * timestamps so the timer restarts after each redelivery attempt.
     *
     * @return Event frames to redeliver (may be empty).
     */
    std::vector<nlohmann::json> checkRedelivery();

    /**
     * @brief Return true if at least one subscription is active.
     */
    bool hasSubscriptions() const;

private:
    struct PendingEvent {
        nlohmann::json frame;
        std::chrono::steady_clock::time_point sent_at;
    };

    struct Subscription {
        std::string id;
        std::string key_prefix;
        std::set<Changefeed::ChangeEventType> event_types;
        uint64_t last_acked_sequence{0};
        uint64_t last_sent_sequence{0};
        std::deque<PendingEvent> pending_ack;
    };

    static nlohmann::json buildEventFrame(const Changefeed::ChangeEvent& ev,
                                          const std::string& sub_id);

    std::unordered_map<std::string, Subscription> subscriptions_;
    mutable std::mutex mu_;
};

} // namespace cdc
} // namespace themis
