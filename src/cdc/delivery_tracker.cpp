/**
 * @file delivery_tracker.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "cdc/delivery_tracker.h"
#include "utils/logger.h"

namespace themis {
namespace cdc {

DeliveryTracker::DeliveryTracker(DeliveryTrackerConfig config,
                                 RedeliveryCallback callback)
    : config_(std::move(config))
    , redelivery_callback_(std::move(callback))
{
}

DeliveryTracker::~DeliveryTracker() {
    stop();
}

/**
 * @brief Start.
 * @details Calls: exchange(), std::thread(), THEMIS_INFO(), count().
 */
void DeliveryTracker::start() {
    if (running_.exchange(true)) {
        return; // already running
    }

    redelivery_thread_ = std::thread(&DeliveryTracker::redeliveryThreadFunc, this);
    THEMIS_INFO("DeliveryTracker: background redelivery thread started "
                "(ack_timeout={}ms, recheck_interval={}ms)",
                std::chrono::duration_cast<std::chrono::milliseconds>(config_.ack_timeout).count(),
                std::chrono::duration_cast<std::chrono::milliseconds>(config_.recheck_interval).count());
}

/**
 * @brief Stop.
 * @details Calls: exchange(), notify_all(), joinable(), join(), THEMIS_INFO().
 */
void DeliveryTracker::stop() {
    if (!running_.exchange(false)) {
        return; // not running
    }

    cv_.notify_all();

    if (redelivery_thread_.joinable()) {
        redelivery_thread_.join();
    }

    THEMIS_INFO("DeliveryTracker: background redelivery thread stopped");
}

/**
 * @brief Track Delivery.
 * @param[in] consumer_id Identifier of the consumer.
 * @param[in] events Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: empty(), lock(), size(), THEMIS_WARN(), std::chrono::steady_clock::now(), emplace(), std::move(), THEMIS_DEBUG().
 */
bool DeliveryTracker::trackDelivery(const std::string& consumer_id,
                                     const std::vector<Changefeed::ChangeEvent>& events) {
    if (events.empty()) {
        return true;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    ConsumerState& state = consumers_[consumer_id];

    // Check pending limit before adding
    if (config_.max_pending_per_consumer > 0 &&
        state.pending.size() + events.size() > config_.max_pending_per_consumer) {
        THEMIS_WARN("DeliveryTracker: consumer '{}' pending limit ({}) would be exceeded "
                    "(current={}, adding={}); rejecting delivery",
                    consumer_id, config_.max_pending_per_consumer,
                    state.pending.size(),events.size());
        return false;
    }

    auto now = std::chrono::steady_clock::now();
    for (const auto& ev : events) {
        PendingEvent pending;
        pending.event = ev;
        pending.delivered_at = now;
        pending.attempt = 1;
        state.pending.emplace(ev.sequence, std::move(pending));
    }
    state.total_delivered += events.size();

    THEMIS_DEBUG("DeliveryTracker: tracked {} event(s) for consumer '{}' (pending={})",
                 events.size(), consumer_id,state.pending.size());
    return true;
}

/**
 * @brief Acknowledge.
 * @param[in] consumer_id Identifier of the consumer.
 * @param[in] sequence Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: lock(), find(), end(), erase(), THEMIS_DEBUG(), size().
 */
bool DeliveryTracker::acknowledge(const std::string& consumer_id, uint64_t sequence) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto cit = consumers_.find(consumer_id);
    if (cit == consumers_.end()) {
        return false;
    }

    ConsumerState& state = cit->second;
    auto eit = state.pending.find(sequence);
    if (eit == state.pending.end()) {
        return false;
    }

    state.pending.erase(eit);
    state.total_acknowledged++;

    THEMIS_DEBUG("DeliveryTracker: consumer '{}' acknowledged sequence {} (pending={})",
                 consumer_id, sequence,state.pending.size());
    return true;
}

/**
 * @brief Acknowledge Up To.
 * @param[in] consumer_id Identifier of the consumer.
 * @param[in] up_to_sequence Input parameter.
 * @return Return value.
 * @details Calls: lock(), find(), end(), begin(), erase(), THEMIS_DEBUG(), size().
 */
size_t DeliveryTracker::acknowledgeUpTo(const std::string& consumer_id,
                                         uint64_t up_to_sequence) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto cit = consumers_.find(consumer_id);
    if (cit == consumers_.end()) {
        return 0;
    }

    ConsumerState& state = cit->second;

    // Erase all pending events with sequence <= up_to_sequence
    size_t removed = 0;
    auto it = state.pending.begin();
    while (it != state.pending.end() && it->first <= up_to_sequence) {
        it = state.pending.erase(it);
        removed++;
    }

    state.total_acknowledged += removed;

    THEMIS_DEBUG("DeliveryTracker: consumer '{}' cumulative-acked up to seq {} "
                 "(removed={}, pending={})",
                 consumer_id, up_to_sequence, removed,state.pending.size());
    return removed;
}

std::vector<Changefeed::ChangeEvent>
DeliveryTracker::getPendingRedelivery(const std::string& consumer_id,
                                       std::optional<std::chrono::milliseconds> timeout_override) {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    auto cit = consumers_.find(consumer_id);
    if (cit == consumers_.end()) {
        return {};
    }

    ConsumerState& state = cit->second;
    auto now = std::chrono::steady_clock::now();
    const auto effective_timeout = timeout_override.value_or(config_.ack_timeout);

    std::vector<Changefeed::ChangeEvent> to_redeliver;
    std::vector<uint64_t> to_expire;

    for (auto& [seq, pending] : state.pending) {
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
            now - pending.delivered_at);

        if (elapsed >= effective_timeout) {
            if (config_.max_redelivery_attempts > 0 &&
                pending.attempt >= config_.max_redelivery_attempts) {
                // Max attempts reached — expire the event
                THEMIS_WARN("DeliveryTracker: consumer '{}' event seq={} expired after "
                            "{} attempt(s)",
                            consumer_id, seq, pending.attempt);
                to_expire.push_back(seq);
                state.total_expired++;
            } else {
                to_redeliver.push_back(pending.event);
                pending.delivered_at = now; // reset timer for next round
                pending.attempt++;
                state.total_redeliveries++;
            }
        }
    }

    for (uint64_t seq : to_expire) {
        state.pending.erase(seq);
    }

    if (!to_redeliver.empty()) {
        THEMIS_INFO("DeliveryTracker: redelivering {} event(s) to consumer '{}'",
                    to_redeliver.size(), consumer_id);
    }

    return to_redeliver;
}

/**
 * @brief Remove Consumer.
 * @param[in] consumer_id Identifier of the consumer.
 * @details Calls: lock(), erase(), THEMIS_INFO().
 */
void DeliveryTracker::removeConsumer(const std::string& consumer_id) {
    std::lock_guard<std::mutex> lock(mutex_);
    size_t erased = consumers_.erase(consumer_id);
    if (erased > 0) {
        THEMIS_INFO("DeliveryTracker: removed consumer '{}'", consumer_id);
    }
}

std::optional<ConsumerDeliveryStats>
DeliveryTracker::getStats(const std::string& consumer_id) const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = consumers_.find(consumer_id);
    if (it == consumers_.end()) {
        return std::nullopt;
    }

    const ConsumerState& state = it->second;
    ConsumerDeliveryStats stats;
    stats.consumer_id = consumer_id;
    stats.pending_count = state.pending.size();
    stats.total_delivered = state.total_delivered;
    stats.total_acknowledged = state.total_acknowledged;
    stats.total_redeliveries = state.total_redeliveries;
    stats.total_expired = state.total_expired;
    return stats;
}

std::vector<ConsumerDeliveryStats> DeliveryTracker::getAllStats() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<ConsumerDeliveryStats> result = {};

    result.reserve(consumers_.size());
    for (const auto& [consumer_id, state] : consumers_) {
        ConsumerDeliveryStats stats;
        stats.consumer_id = consumer_id;
        stats.pending_count = state.pending.size();
        stats.total_delivered = state.total_delivered;
        stats.total_acknowledged = state.total_acknowledged;
        stats.total_redeliveries = state.total_redeliveries;
        stats.total_expired = state.total_expired;
        result.push_back(std::move(stats));
    }
    return result;
}

size_t DeliveryTracker::consumerCount() const {
    /**
     * @brief Lock.
     * @param[in] mutex_ Input parameter.
     * @return Return value.
     */
    std::lock_guard<std::mutex> lock(mutex_);
    return consumers_.size();
}

/**
 * @brief ===== Background Redelivery Thread =====
 * @details Calls: THEMIS_DEBUG(), load(), lock(), wait_for(), checkAndRedeliver(), THEMIS_ERROR(), what().
 */

void DeliveryTracker::redeliveryThreadFunc() {
    THEMIS_DEBUG("DeliveryTracker: redelivery thread running");

    while (running_.load(std::memory_order_relaxed)) {
        {
            std::unique_lock<std::mutex> lock(cv_mutex_);
            cv_.wait_for(lock, config_.recheck_interval, [this] {
                return !running_.load(std::memory_order_relaxed);
            });
        }

        if (!running_.load(std::memory_order_relaxed)) {
            break;
        }

        try {
            checkAndRedeliver();
        } catch (const std::exception& e) {
            THEMIS_ERROR("DeliveryTracker: error in redelivery thread: {}", e.what());
        }
    }

    THEMIS_DEBUG("DeliveryTracker: redelivery thread exiting");
}

/**
 * @brief Check And Redeliver.
 * @details Calls: lock(), reserve(), size(), push_back(), getPendingRedelivery(), empty(), redelivery_callback_().
 */
void DeliveryTracker::checkAndRedeliver() {
    if (!redelivery_callback_) {
        return;
    }

    // Collect consumer IDs under lock, then call getPendingRedelivery per consumer
    // (getPendingRedelivery acquires the lock itself).
    std::vector<std::string> consumer_ids;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        consumer_ids.reserve(consumers_.size());
        for (const auto& [id, _] : consumers_) {
            consumer_ids.push_back(id);
        }
    }

    for (const auto& cid : consumer_ids) {
        auto events = getPendingRedelivery(cid);
        if (!events.empty()) {
            redelivery_callback_(cid, events);
        }
    }
}

} // namespace cdc
} // namespace themis
