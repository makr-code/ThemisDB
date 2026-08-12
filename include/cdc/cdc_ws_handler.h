/**
 * @file cdc_ws_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "cdc/changefeed.h"
#include "cdc/consumer_group.h"
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
 * Consumer-group extension (v1.8.0):
 *   subscribe    {"action":"subscribe","group_id":"etl-workers",
 *                 "consumer_id":"worker-3","collection":"orders"}
 *   ack          {"action":"ack","group_id":"etl-workers","sequence":10042}
 *
 * When "group_id" is present, the handler uses a ConsumerGroupManager to:
 *   - resume delivery from the group's committed offset + 1 on reconnect
 *   - filter events to the consumer's assigned partition (key-hash mod N)
 *   - durably advance the committed offset on ack
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

    /**
     * @brief Construct with optional test overrides.
     *
     * @param max_pending_ack  Override for the pending-ack queue limit (default:
     *                         kMaxPendingAck).  Pass a small value in unit tests
     *                         to trigger back-pressure without 1,000 events.
     * @param group_manager    Optional ConsumerGroupManager for consumer-group
     *                         semantics.  When non-null, subscriptions that carry
     *                         a "group_id" field use it for partition filtering and
     *                         durable offset commit.  Pass nullptr (the default) for
     *                         plain fan-out mode (existing behaviour).
     */
    explicit CdcWebSocketHandler(size_t max_pending_ack = kMaxPendingAck,
                                  ConsumerGroupManager* group_manager = nullptr)
        : max_pending_ack_(max_pending_ack), group_manager_(group_manager) {}

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
     * @brief Return the total number of times delivery was paused due to
     *        back-pressure (pending-ack queue full).
     *
     * Exposed as the `cdc_ws_overflow_total` metric.
     */
    uint64_t getWsOverflowTotal() const {
        return ws_overflow_total_.load(std::memory_order_relaxed);
    }

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
        // Consumer-group fields (empty when not in group mode)
        std::string group_id;
        std::string consumer_id;
    };

    static nlohmann::json buildEventFrame(const Changefeed::ChangeEvent& ev,
                                          const std::string& sub_id);

    std::unordered_map<std::string, Subscription> subscriptions_;
    mutable std::mutex mu_;
    std::atomic<uint64_t> ws_overflow_total_{0};
    size_t max_pending_ack_{kMaxPendingAck};
    ConsumerGroupManager* group_manager_{nullptr};  ///< Not owned; may be nullptr
};

} // namespace cdc
} // namespace themis
