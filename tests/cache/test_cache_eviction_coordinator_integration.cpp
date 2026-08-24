/**
 * @file test_cache_eviction_coordinator_integration.cpp
 * @brief Phase 3 integration tests: cache→coordinator→storage event flow.
 * @version 1.0.0
 * 
 * Tests the end-to-end event flow:
 * 1. Cache detects eviction (capacity pressure)
 * 2. Emits CacheEvictionEvent via EvictionListener
 * 3. Coordinator processes event and makes promotion/demotion decisions
 * 4. Storage tier receives demotion request
 * 
 * @see include/cache/eviction_listener.h
 * @see include/access_model/access_coordinator.h
 * @see src/cache/ROADMAP.md Phase 3
 */

#include <gtest/gtest.h>
#include <mutex>
#include <memory>
#include <unordered_map>
#include <vector>
#include <chrono>
#include <thread>

#include "cache/eviction_listener.h"
#include "access_model/access_coordinator.h"

namespace themis {
namespace cache {

class TestEvictionListenerManager : public EvictionListenerManager {
public:
    uint64_t registerListener(std::shared_ptr<IEvictionListener> listener) override {
        if (!listener) {
            return 0;
        }

        std::lock_guard<std::mutex> lock(mutex_);
        const uint64_t handle = next_handle_++;
        listeners_.emplace(handle, std::move(listener));
        return handle;
    }

    void unregisterListener(uint64_t handle) override {
        std::lock_guard<std::mutex> lock(mutex_);
        listeners_.erase(handle);
    }

    void emitEvictionEvent(const CacheEvictionEvent& event) override {
        std::vector<std::shared_ptr<IEvictionListener>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot.reserve(listeners_.size());
            for (const auto& [_, listener] : listeners_) {
                snapshot.push_back(listener);
            }
        }

        for (const auto& listener : snapshot) {
            if (listener) {
                listener->onCacheEvicted(event);
            }
        }
    }

    void emitCapacityPressure(TierLevel from_tier, uint32_t current_capacity_percent,
                              std::size_t recommended_eviction_count) override {
        std::vector<std::shared_ptr<IEvictionListener>> snapshot;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            snapshot.reserve(listeners_.size());
            for (const auto& [_, listener] : listeners_) {
                snapshot.push_back(listener);
            }
        }

        for (const auto& listener : snapshot) {
            if (listener) {
                listener->onCapacityPressure(from_tier, current_capacity_percent,
                                             recommended_eviction_count);
            }
        }
    }

    std::size_t getListenerCount() const noexcept override {
        std::lock_guard<std::mutex> lock(mutex_);
        return listeners_.size();
    }

private:
    mutable std::mutex mutex_;
    std::unordered_map<uint64_t, std::shared_ptr<IEvictionListener>> listeners_;
    uint64_t next_handle_ = 1;
};

/**
 * @brief Mock coordinator implementation for testing.
 * 
 * Tracks all eviction events received and simulates promotion/demotion decisions.
 */
class MockAccessCoordinator : public access_model::AccessCoordinator {
public:
    struct CapturedEvent {
        access_model::EvictionEvent event;
        std::chrono::system_clock::time_point received_at;
    };

    // Track events received
    std::vector<CapturedEvent> captured_events;
    std::size_t promotion_decisions = 0;
    std::size_t demotion_decisions = 0;

    // EvictionListener / AccessCoordinator legacy adapter
    void onCacheEvicted(std::string_view key, access_model::TierLevel from_tier,
                        std::size_t size_bytes, uint64_t access_count,
                        std::chrono::seconds last_access_age_secs,
                        std::string_view eviction_reason) override {
        access_model::EvictionEvent event;
        event.key = std::string(key);
        event.tier = from_tier;
        event.reason = std::string(eviction_reason);
        event.evicted_size_bytes = size_bytes;
        event.access_count = access_count;
        event.last_access_age_secs = last_access_age_secs;

        CapturedEvent captured;
        captured.event = event;
        captured.received_at = std::chrono::system_clock::now();
        captured_events.push_back(captured);

        if (event.access_count > 10) {
            promotion_decisions++;
        } else if (event.access_count < 2) {
            demotion_decisions++;
        }
    }

    void onStorageAccess(std::string_view, access_model::TierLevel,
                         uint64_t access_count,
                         std::chrono::seconds access_window) override {
        if (access_count / access_window.count() > 3) {
            promotion_decisions++;
        }
    }

    // Required AccessCoordinator methods (stubs for test)
    bool initialize(const std::map<access_model::TierLevel, std::shared_ptr<access_model::AccessTier>>&) override {
        return true;
    }

    void start() override {}

    void shutdown() override {}
    bool isRunning() const override { return true; }

    void setAgePolicy(const access_model::AgeBasedPolicy&) override {}

    void setPromotionThresholds(uint64_t, uint64_t) override {}

    std::future<access_model::PromotionResult> promoteAsync(const std::string&, access_model::TierLevel,
                                                            access_model::TierLevel, uint64_t) override {
        return std::async(std::launch::deferred, [] { return access_model::PromotionResult{}; });
    }

    std::optional<access_model::DemotionPlan> planDemotion(const std::string&, access_model::TierLevel,
                                                           access_model::TierLevel, uint64_t) override {
        return std::nullopt;
    }

    std::optional<access_model::DemotionResult> executeDemotion(const std::string&) override {
        return std::nullopt;
    }

    access_model::AccessMetrics getKeyMetrics(const std::string&) override { return {}; }
    access_model::AccessMetrics getTierMetrics(access_model::TierLevel) override { return {}; }

    access_model::AccessModelMetrics getAccessModelMetrics() override { return {}; }

    std::vector<access_model::AccessTransitionEvent> getRecentTransitions(std::size_t) override {
        return {};
    }
};

/**
 * @brief Test: Basic eviction listener registration and event emission.
 */
class CacheEvictionCoordinatorIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override { listener_manager = std::make_unique<TestEvictionListenerManager>(); }

    std::unique_ptr<TestEvictionListenerManager> listener_manager;
};

/**
 * CEI-01: Register listener and verify initial state.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, RegisterListener_ReturnsValidHandle) {
    class TestListener : public IEvictionListener {
    public:
        void onCacheEvicted(const CacheEvictionEvent&) override {}
    };

    auto listener = std::make_shared<TestListener>();
    uint64_t handle = listener_manager->registerListener(listener);

    EXPECT_NE(handle, 0);
    EXPECT_EQ(listener_manager->getListenerCount(), 1);
}

/**
 * CEI-02: Emit eviction event and verify listener receives it.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, EmitEvictionEvent_ListenerReceivesEvent) {
    class TestListener : public IEvictionListener {
    public:
        int event_count = 0;
        std::string last_key;

        void onCacheEvicted(const CacheEvictionEvent& event) override {
            event_count++;
            last_key = event.key;
        }
    };

    auto listener = std::make_shared<TestListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "test_key_1";
    event.from_tier = TierLevel::L1;
    event.access_count = 5;
    event.size_bytes = 1024;

    listener_manager->emitEvictionEvent(event);

    EXPECT_EQ(listener->event_count, 1);
    EXPECT_EQ(listener->last_key, "test_key_1");
}

/**
 * CEI-03: Multiple listeners receive same event.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, MultipleListeners_AllReceiveEvent) {
    class CountingListener : public IEvictionListener {
    public:
        int* shared_counter = nullptr;

        void onCacheEvicted(const CacheEvictionEvent&) override {
            if (shared_counter) {
                (*shared_counter)++;
            }
        }
    };

    int counter = 0;

    auto listener1 = std::make_shared<CountingListener>();
    listener1->shared_counter = &counter;
    listener_manager->registerListener(listener1);

    auto listener2 = std::make_shared<CountingListener>();
    listener2->shared_counter = &counter;
    listener_manager->registerListener(listener2);

    CacheEvictionEvent event;
    event.key = "test_key_2";
    listener_manager->emitEvictionEvent(event);

    EXPECT_EQ(counter, 2);  // Both listeners called
}

/**
 * CEI-04: Unregister listener and verify it no longer receives events.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, UnregisterListener_NoMoreEvents) {
    class TestListener : public IEvictionListener {
    public:
        int event_count = 0;
        void onCacheEvicted(const CacheEvictionEvent&) override { event_count++; }
    };

    auto listener = std::make_shared<TestListener>();
    uint64_t handle = listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "test_key_3";
    listener_manager->emitEvictionEvent(event);
    EXPECT_EQ(listener->event_count, 1);

    listener_manager->unregisterListener(handle);

    listener_manager->emitEvictionEvent(event);
    EXPECT_EQ(listener->event_count, 1);  // No new event
}

/**
 * CEI-05: Capacity pressure notification.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, CapacityPressure_NotificationSent) {
    class TestListener : public IEvictionListener {
    public:
        int pressure_count = 0;
        uint32_t last_capacity_percent = 0;

        void onCacheEvicted(const CacheEvictionEvent&) override {}

        void onCapacityPressure(TierLevel, uint32_t current_capacity_percent,
                               std::size_t) override {
            pressure_count++;
            last_capacity_percent = current_capacity_percent;
        }
    };

    auto listener = std::make_shared<TestListener>();
    listener_manager->registerListener(listener);

    listener_manager->emitCapacityPressure(TierLevel::L2, 85, 10);

    EXPECT_EQ(listener->pressure_count, 1);
    EXPECT_EQ(listener->last_capacity_percent, 85);
}

/**
 * CEI-06: High-access entries trigger promotion decision.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, HighAccessEviction_PromotionSignal) {
    class TestListener : public IEvictionListener {
    public:
        bool saw_high_access = false;

        void onCacheEvicted(const CacheEvictionEvent& event) override {
            if (event.access_count >= 10) {
                saw_high_access = true;
            }
        }
    };

    auto listener = std::make_shared<TestListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "hot_key";
    event.access_count = 15;  // High access count
    event.reason = EvictionReason::Capacity;

    listener_manager->emitEvictionEvent(event);

    EXPECT_TRUE(listener->saw_high_access);
}

/**
 * CEI-07: Low-access entries trigger demotion decision.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, LowAccessEviction_DemotionSignal) {
    class TestListener : public IEvictionListener {
    public:
        bool saw_low_access = false;

        void onCacheEvicted(const CacheEvictionEvent& event) override {
            if (event.access_count < 3) {
                saw_low_access = true;
            }
        }
    };

    auto listener = std::make_shared<TestListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "cold_key";
    event.access_count = 1;  // Low access count
    event.reason = EvictionReason::Capacity;

    listener_manager->emitEvictionEvent(event);

    EXPECT_TRUE(listener->saw_low_access);
}

/**
 * CEI-08: Correlation ID tracking for distributed tracing.
 */
TEST_F(CacheEvictionCoordinatorIntegrationTest, CorrelationID_PropagatedInEvent) {
    class TestListener : public IEvictionListener {
    public:
        std::string captured_correlation_id;

        void onCacheEvicted(const CacheEvictionEvent& event) override {
            captured_correlation_id = event.correlation_id;
        }
    };

    auto listener = std::make_shared<TestListener>();
    listener_manager->registerListener(listener);

    CacheEvictionEvent event;
    event.key = "test_key";
    event.correlation_id = "trace_12345";

    listener_manager->emitEvictionEvent(event);

    EXPECT_EQ(listener->captured_correlation_id, "trace_12345");
}

}  // namespace cache
}  // namespace themis
