/**
 * @file cdc_ws_handler.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

class CdcWebSocketHandler {
public:
    static constexpr size_t kMaxPendingAck = 1000;

    static constexpr int64_t kRedeliveryTimeoutMs = 5000;

    explicit CdcWebSocketHandler(size_t max_pending_ack = kMaxPendingAck,
                                  ConsumerGroupManager* group_manager = nullptr)
        : max_pending_ack_(max_pending_ack), group_manager_(group_manager) {}

    /**
     * @brief Handle Frame.
     * @param[in] frame Input parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> handleFrame(const nlohmann::json& frame);

    /**
     * @brief Poll Events.
     * @param[in,out] feed Input/output parameter.
     * @return Return value.
     */
    std::vector<nlohmann::json> pollEvents(Changefeed& feed);

    /**
     * @brief Check Redelivery.
     * @return Return value.
     */
    std::vector<nlohmann::json> checkRedelivery();

    uint64_t getWsOverflowTotal() const {
        return ws_overflow_total_.load(std::memory_order_relaxed);
    }

    /**
     * @brief Has Subscriptions.
     * @return True when the operation succeeds.
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

    /**
     * @brief Build Event Frame.
     * @param[in] ev Input parameter.
     * @param[in] sub_id Identifier of the sub.
     * @return Return value.
     */
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
