/**
 * @file auth_event_bus.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.1.0
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 *
 * @note **Stub Implementation**: Provides a basic in-process pub/sub bus for auth events.
 *       For production use, consider integrating with Apache Kafka, Redis Streams, or
 *       cloud-native message brokers (Azure Service Bus, AWS SNS).
 */

#include "auth/auth_event_bus.h"
#include <algorithm>
#include <shared_mutex>
#include <spdlog/spdlog.h>

namespace themis {
namespace auth {

class DefaultAuthEventBus : public IAuthEventBus {
private:
    mutable std::shared_mutex subscribers_mutex_;
    std::vector<std::shared_ptr<IAuthEventSubscriber>> subscribers_;

public:
    DefaultAuthEventBus() = default;

    void publish(const AuthEvent& event) override {
        /**
         * @brief Lock.
         * @param[in] subscribers_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(subscribers_mutex_);
        for (const auto& subscriber : subscribers_) {
            try {
                subscriber->onAuthEvent(event);
            } catch (const std::exception& e) {
                spdlog::error("Auth event subscriber {} raised exception: {}",
                              subscriber->subscriberId(), e.what());
            }
        }
    }

    bool subscribe(std::shared_ptr<IAuthEventSubscriber> subscriber) override {
        if (!subscriber) {
          return false;
        }

        /**
         * @brief Lock.
         * @param[in] subscribers_mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock lock(subscribers_mutex_);
        // Check for duplicates
        for (const auto& existing : subscribers_) {
            if (existing->subscriberId() == subscriber->subscriberId()) {
                return false;  // Subscriber already registered
            }
        }
        subscribers_.push_back(subscriber);
        return true;
    }

    bool unsubscribe(const std::string& subscriber_id) override {
        /**
         * @brief Lock.
         * @param[in] subscribers_mutex_ Input parameter.
         * @return Return value.
         */
        std::unique_lock lock(subscribers_mutex_);
        auto it = std::remove_if(subscribers_.begin(), subscribers_.end(),
                                 [&subscriber_id](const auto& s) {
                                     return s->subscriberId() == subscriber_id;
                                 });
        if (it == subscribers_.end()) {
            return false;  // Not found
        }
        subscribers_.erase(it, subscribers_.end());
        return true;
    }

    size_t subscriberCount() const override {
        /**
         * @brief Lock.
         * @param[in] subscribers_mutex_ Input parameter.
         * @return Return value.
         */
        std::shared_lock lock(subscribers_mutex_);
        return subscribers_.size();
    }
};

// Global singleton instance
static std::shared_ptr<IAuthEventBus> g_auth_event_bus =
    std::make_shared<DefaultAuthEventBus>();

/**
 * @brief Get Auth Event Bus.
 * @return Return value.
 * @details Implements getAuthEventBus without additional internal calls.
 */
IAuthEventBus& getAuthEventBus() {
    return *g_auth_event_bus;
}

/**
 * @brief Set Auth Event Bus.
 * @param[in] bus Input parameter.
 * @details Implements setAuthEventBus without additional internal calls.
 */
void setAuthEventBus(std::shared_ptr<IAuthEventBus> bus) {
    if (bus) {
        g_auth_event_bus = bus;
    }
}

}  // namespace auth
}  // namespace themis
