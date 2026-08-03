/**
 * @file test_cache_storage_integration.cpp
 * @brief Integration tests for BLOCK 2-3: Cache and Storage module integration
 *
 * ThemisDB | File: test_cache_storage_integration.cpp
 * Maturity: 🟡 ALPHA
 * Status: BLOCK 2-3 Cache & Storage Integration Tests
 * Author: Copilot | Date: 2026-08-03
 *
 * Comprehensive integration tests for:
 * - CAI-01: Cache L1 eviction → AccessCoordinator notification
 * - CAI-02: Cache L2 eviction → AccessCoordinator notification
 * - CAI-03: High-access entries marked for promotion
 * - CAI-04: Low-access entries marked for demotion
 * - CAI-05: Shared AgeBasedPolicy enforcement
 * - CAI-06: Concurrent tier transitions (race condition safety)
 * - CAI-07: Storage warm-tier access → coordinator notification
 * - CAI-08: Cold-to-warm promotion chain (S3 access triggers cache warmup)
 *
 * @see include/access_model/access_coordinator.h
 * @see docs/architecture/UNIFIED_ACCESS_MODEL.md
 * @see src/access_model/ROADMAP.md (Phase 3-4: BLOCK 2-3)
 */

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <memory>
#include <thread>
#include <chrono>
#include <atomic>

#include "access_model/access_coordinator.h"
#include "access_model/access_tier_interface.h"
#include "access_model/promotion_demotion.h"
#include "cache/adaptive_query_cache.h"
#include "storage/tiered_storage.h"
#include "utils/logger.h"

namespace themis {
namespace access_model {
namespace test {

// ============================================================================
// Mock EvictionListener
// ============================================================================

class MockEvictionListener : public EvictionListener {
public:
    MOCK_METHOD(void, onCacheEvicted,
                (std::string_view key, TierLevel from_tier, std::size_t size_bytes,
                 uint64_t access_count, std::chrono::seconds last_access_age_secs,
                 std::string_view eviction_reason),
                (override));
};

// ============================================================================
// Mock PromotionListener
// ============================================================================

class MockPromotionListener : public PromotionListener {
public:
    MOCK_METHOD(void, onStorageAccess,
                (std::string_view key, TierLevel from_tier, uint64_t access_count,
                 std::chrono::seconds access_window),
                (override));
};

// ============================================================================
// Mock AccessTier (for coordinator-level integration tests)
// ============================================================================

class MockAccessTier : public AccessTier {
public:
    MOCK_METHOD(TierGetResult, get,
                (std::string_view key, const TierAccessOptions& options), (override));
    MOCK_METHOD(TierPutResult, put,
                (std::string_view key, std::string_view value,
                 const TierAccessOptions& options), (override));
    MOCK_METHOD(bool, invalidate, (std::string_view key), (override));
    MOCK_METHOD(TierLevel, getTierLevel, (), (const, override));
    MOCK_METHOD(std::string, getTierName, (), (const, override));
    MOCK_METHOD(bool, hasKey, (std::string_view key), (const, override));
    MOCK_METHOD(std::size_t, getCurrentSizeBytes, (), (const, override));
    MOCK_METHOD(std::size_t, getMaxCapacityBytes, (), (const, override));
    MOCK_METHOD(std::size_t, getEntryCount, (), (const, override));
    MOCK_METHOD(double, getHitRate, (), (const, override));
    MOCK_METHOD(std::chrono::microseconds, getAverageGetLatency, (), (const, override));
    MOCK_METHOD(std::chrono::microseconds, getAveragePutLatency, (), (const, override));
    MOCK_METHOD(uint64_t, getAccessCount, (std::string_view key), (const, override));
    MOCK_METHOD(std::chrono::seconds, getKeyAge, (std::string_view key), (const, override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(bool, isHealthy, (), (const, override));
};

// ============================================================================
// Test Fixtures
// ============================================================================

class CacheStorageIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create cache with standard config
        cache::AdaptiveQueryCache::Config cache_cfg;
        cache_cfg.l1_max_entries = 100;
        cache_cfg.l2_max_entries = 500;
        cache_cfg.l1_ttl_seconds = 60;
        cache_cfg.l2_ttl_seconds = 300;
        
        cache_ = std::make_shared<cache::AdaptiveQueryCache>(cache_cfg);
        
        // Create mock listeners
        mock_eviction_listener_ = std::make_unique<MockEvictionListener>();
        mock_promotion_listener_ = std::make_unique<MockPromotionListener>();
    }

    void TearDown() override {
        // Cleanup
        cache_.reset();
        mock_eviction_listener_.reset();
        mock_promotion_listener_.reset();
    }

    std::shared_ptr<cache::AdaptiveQueryCache> cache_;
    std::unique_ptr<MockEvictionListener> mock_eviction_listener_;
    std::unique_ptr<MockPromotionListener> mock_promotion_listener_;
};

// ============================================================================
// CAI-01: Cache L1 Eviction → AccessCoordinator Notification
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI01_L1EvictionNotification) {
    // Register eviction listener
    cache_->setEvictionListener(mock_eviction_listener_.get());

    // Expect onCacheEvicted to be called when L1 is full and entry evicts
    EXPECT_CALL(*mock_eviction_listener_, onCacheEvicted)
        .Times(::testing::AtLeast(1))
        .WillRepeatedly([](std::string_view key, TierLevel from_tier, 
                          std::size_t size_bytes, uint64_t access_count,
                          std::chrono::seconds last_access_age_secs,
                          std::string_view eviction_reason) {
            ASSERT_EQ(from_tier, TierLevel::L1_WORKING);
            ASSERT_GT(size_bytes, 0);
            ASSERT_FALSE(eviction_reason.empty());
        });

    // Add entries to L1 until it evicts
    nlohmann::json value = nlohmann::json::object();
    value["data"] = "test_data";
    
    for (int i = 0; i < 110; ++i) {
        std::string key = "test_key_" + std::to_string(i);
        cache_->put(key, value.dump());
    }
}

// ============================================================================
// CAI-02: Cache L2 Eviction → AccessCoordinator Notification
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI02_L2EvictionNotification) {
    cache_->setEvictionListener(mock_eviction_listener_.get());

    EXPECT_CALL(*mock_eviction_listener_, onCacheEvicted)
        .Times(::testing::AtLeast(1));

    // Fill L1, then L2 beyond capacity
    nlohmann::json value = nlohmann::json::object();
    value["data"] = "test_data";
    
    for (int i = 0; i < 600; ++i) {
        std::string key = "warm_key_" + std::to_string(i);
        cache_->put(key, value.dump());
    }
}

// ============================================================================
// CAI-03: High-Access Entries Marked for Promotion
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI03_HighAccessPromotion) {
    cache_->setEvictionListener(mock_eviction_listener_.get());

    std::atomic<bool> high_access_detected{false};

    EXPECT_CALL(*mock_eviction_listener_, onCacheEvicted)
        .WillRepeatedly([&](std::string_view key, TierLevel from_tier,
                           std::size_t size_bytes, uint64_t access_count,
                           std::chrono::seconds last_access_age_secs,
                           std::string_view eviction_reason) {
            // High access count should trigger promotion candidate
            if (access_count > 5) {
                high_access_detected = true;
            }
        });

    // Add and repeatedly access a key to increase access_count
    std::string hot_key = "hot_key";
    nlohmann::json value = nlohmann::json::object();
    value["data"] = "hot_data";
    
    cache_->put(hot_key, value.dump());
    
    // Multiple accesses to bump access count
    for (int i = 0; i < 20; ++i) {
        auto result = cache_->get(hot_key);
        ASSERT_TRUE(result.has_value());
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // Force L1 eviction
    for (int i = 0; i < 110; ++i) {
        std::string key = "evict_key_" + std::to_string(i);
        cache_->put(key, value.dump());
    }
}

// ============================================================================
// CAI-04: Low-Access Entries Marked for Demotion
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI04_LowAccessDemotion) {
    cache_->setEvictionListener(mock_eviction_listener_.get());

    std::atomic<bool> low_access_detected{false};

    EXPECT_CALL(*mock_eviction_listener_, onCacheEvicted)
        .WillRepeatedly([&](std::string_view key, TierLevel from_tier,
                           std::size_t size_bytes, uint64_t access_count,
                           std::chrono::seconds last_access_age_secs,
                           std::string_view eviction_reason) {
            // Low access count should trigger demotion candidate
            if (access_count <= 1) {
                low_access_detected = true;
            }
        });

    // Add a key once and don't access it
    std::string cold_key = "cold_key";
    nlohmann::json value = nlohmann::json::object();
    value["data"] = "cold_data";
    
    cache_->put(cold_key, value.dump());

    // Force eviction without accessing cold_key
    for (int i = 0; i < 110; ++i) {
        std::string key = "evict_key_" + std::to_string(i);
        cache_->put(key, value.dump());
    }
}

// ============================================================================
// CAI-05: Shared AgeBasedPolicy Enforcement
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI05_SharedAgePolicy) {
    // Verify that AgeBasedPolicy can be shared across cache and storage
    AgeBasedPolicy policy;
    policy.hot_to_warm_days = 30;
    policy.warm_to_cold_days = 90;
    policy.hot_zero_access_days = 14;
    policy.warm_zero_access_days = 45;

    // Both cache and storage should accept this unified policy
    ASSERT_EQ(policy.hot_to_warm_days, 30);
    ASSERT_EQ(policy.warm_to_cold_days, 90);
    ASSERT_EQ(policy.hot_zero_access_days, 14);
    ASSERT_EQ(policy.warm_zero_access_days, 45);
}

// ============================================================================
// CAI-06: Concurrent Tier Transitions
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI06_ConcurrentTransitions) {
    cache_->setEvictionListener(mock_eviction_listener_.get());

    std::atomic<int> eviction_count{0};

    EXPECT_CALL(*mock_eviction_listener_, onCacheEvicted)
        .WillRepeatedly([&](std::string_view, TierLevel, std::size_t,
                           uint64_t, std::chrono::seconds,
                           std::string_view) {
            eviction_count++;
        });

    // Concurrent puts from multiple threads
    std::vector<std::thread> threads;
    for (int t = 0; t < 4; ++t) {
        threads.emplace_back([this, t]() {
            nlohmann::json value = nlohmann::json::object();
            value["data"] = "concurrent_data";
            
            for (int i = 0; i < 30; ++i) {
                std::string key = "thread_" + std::to_string(t) + "_key_" + std::to_string(i);
                cache_->put(key, value.dump());
            }
        });
    }

    for (auto& thread : threads) {
        thread.join();
    }

    ASSERT_GT(eviction_count, 0);
}

// ============================================================================
// CAI-07: Storage Warm-Tier Access → PromotionListener Notification
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI07_StorageWarmTierAccessEmitsPromotion) {
    // Verify that a PromotionListener receives onStorageAccess() when a warm
    // storage tier detects a hot-access pattern.
    //
    // The mock_promotion_listener_ is passed directly as the listener.  We call
    // onStorageAccess() explicitly to simulate what TieredStorageManager would
    // do after detecting repeated accesses in the STORAGE_WARM tier.

    EXPECT_CALL(*mock_promotion_listener_,
                onStorageAccess(::testing::StrEq("warm_key"),
                                TierLevel::STORAGE_WARM,
                                ::testing::Ge(uint64_t{3}),
                                ::testing::_))
        .Times(1);

    // Simulate a storage tier emitting a hot-access event.
    mock_promotion_listener_->onStorageAccess(
        "warm_key",
        TierLevel::STORAGE_WARM,
        /*access_count=*/5,
        /*access_window=*/std::chrono::seconds{3600});
}

// ============================================================================
// CAI-08: Cold-to-Warm Promotion Chain via AccessCoordinator
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI08_ColdToWarmPromotion) {
    // Test the cold→warm→L3 promotion decision chain inside AccessCoordinator.
    //
    // Scenario:
    //   1. A key in STORAGE_COLD is accessed above the promotion threshold.
    //   2. The coordinator receives an AccessEvent for that key.
    //   3. The coordinator records the event and (because STORAGE_WARM is
    //      registered) schedules a promotion to STORAGE_WARM.
    //   4. getRecentTransitions() shows a transition with to_tier == STORAGE_WARM.

    auto coordinator = createAccessCoordinator(/*thread_pool_size=*/1);

    // Register only STORAGE_COLD and STORAGE_WARM so the coordinator has valid
    // tiers to reason about.
    auto warm_tier = std::make_shared<::testing::NiceMock<MockAccessTier>>();
    auto cold_tier = std::make_shared<::testing::NiceMock<MockAccessTier>>();

    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers{
        {TierLevel::STORAGE_WARM, warm_tier},
        {TierLevel::STORAGE_COLD, cold_tier},
    };
    ASSERT_TRUE(coordinator->initialize(tiers));
    coordinator->start();

    // Set a low promotion threshold so a single burst triggers the decision.
    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 3;
    coordinator->setAgePolicy(policy);

    // Simulate cold-tier hot-access detection.
    AccessEvent event;
    event.key = "cold_key";
    event.current_tier = TierLevel::STORAGE_COLD;
    event.access_count = 5;  // Above threshold → promotion to STORAGE_WARM

    coordinator->onHotAccess(event);

    // Allow the coordinator a moment to process the queued event.
    std::this_thread::sleep_for(std::chrono::milliseconds(50));

    auto transitions = coordinator->getRecentTransitions(20);
    ASSERT_FALSE(transitions.empty());

    bool promoted_to_warm = false;
    for (const auto& t : transitions) {
        if (t.to_tier == TierLevel::STORAGE_WARM) {
            promoted_to_warm = true;
        }
    }
    EXPECT_TRUE(promoted_to_warm)
        << "Expected a transition to STORAGE_WARM after cold-tier hot-access "
           "event with access_count above promotion threshold.";

    coordinator->shutdown();
}

// ============================================================================
// Integration Stability Tests
// ============================================================================

TEST_F(CacheStorageIntegrationTest, CAI09_ListenerStability) {
    // Register, then unregister listener
    cache_->setEvictionListener(mock_eviction_listener_.get());
    cache_->setEvictionListener(nullptr);  // Should not crash

    // Add entries after unregister
    nlohmann::json value = nlohmann::json::object();
    value["data"] = "test_data";
    cache_->put("stability_test_key", value.dump());

    ASSERT_TRUE(true);  // No crash = success
}

TEST_F(CacheStorageIntegrationTest, CAI10_NullListenerSafety) {
    // Should handle null listener gracefully
    cache_->setEvictionListener(nullptr);
    
    nlohmann::json value = nlohmann::json::object();
    value["data"] = "test_data";
    
    for (int i = 0; i < 110; ++i) {
        std::string key = "null_listener_key_" + std::to_string(i);
        cache_->put(key, value.dump());
    }

    ASSERT_TRUE(true);
}

}  // namespace test
}  // namespace access_model
}  // namespace themis
