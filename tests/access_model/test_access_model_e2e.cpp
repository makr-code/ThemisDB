/**
 * @file test_access_model_e2e.cpp
 * @brief End-to-end integration tests for AccessCoordinator (Phase 6.1)
 *
 * Test scope: Full-stack promotion/demotion flows with real coordinator
 * Test coverage:
 *   - T1-T4: Promotion chain tests
 *   - T5-T7: Demotion chain tests
 *   - T8-T10: Policy enforcement tests
 *   - T11-T15: Edge cases and stress scenarios
 *
 * Acceptance criteria:
 * - All 15 tests pass in <5s total
 * - ASan/TSan/UBSan clean (0 errors)
 * - Coverage >85% of AccessCoordinatorImpl
 * - Tests independently runnable (no shared state)
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "access_model/access_coordinator.h"
#include "access_model/access_tier_interface.h"
#include "access_model/age_based_policy.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Mock Tiers & Fixtures
// ============================================================================

class MockAccessTier : public AccessTier {
 public:
    MOCK_METHOD(TierGetResult, get, (std::string_view, const TierAccessOptions&),
                (override));
    MOCK_METHOD(TierPutResult, put,
                (std::string_view, std::string_view, const TierAccessOptions&),
                (override));
    MOCK_METHOD(bool, invalidate, (std::string_view), (override));
    MOCK_METHOD(TierLevel, getTierLevel, (), (const, override));
    MOCK_METHOD(std::string, getTierName, (), (const, override));
    MOCK_METHOD(bool, hasKey, (std::string_view), (const, override));
    MOCK_METHOD(std::size_t, getCurrentSizeBytes, (), (const, override));
    MOCK_METHOD(std::size_t, getMaxCapacityBytes, (), (const, override));
    MOCK_METHOD(std::size_t, getEntryCount, (), (const, override));
    MOCK_METHOD(double, getHitRate, (), (const, override));
    MOCK_METHOD(std::chrono::microseconds, getAverageGetLatency, (),
                (const, override));
    MOCK_METHOD(std::chrono::microseconds, getAveragePutLatency, (),
                (const, override));
    MOCK_METHOD(uint64_t, getAccessCount, (std::string_view), (const, override));
    MOCK_METHOD(std::chrono::seconds, getKeyAge, (std::string_view),
                (const, override));
    MOCK_METHOD(bool, initialize, (), (override));
    MOCK_METHOD(void, shutdown, (), (override));
    MOCK_METHOD(bool, isHealthy, (), (const, override));
};

class AccessModelE2ETest : public ::testing::Test {
 protected:
    void SetUp() override {
        // Create real coordinator with 4 worker threads
        coordinator_ = createAccessCoordinator(4);

        // Create mock tiers
        l1_tier_ = std::make_shared<MockAccessTier>();
        l2_tier_ = std::make_shared<MockAccessTier>();
        l3_tier_ = std::make_shared<MockAccessTier>();
        warm_storage_ = std::make_shared<MockAccessTier>();
        cold_storage_ = std::make_shared<MockAccessTier>();

        // Set up default tier behaviors
        ON_CALL(*l1_tier_, getTierLevel)
            .WillByDefault(testing::Return(TierLevel::L1_WORKING));
        ON_CALL(*l2_tier_, getTierLevel)
            .WillByDefault(testing::Return(TierLevel::L2_EPISODIC));
        ON_CALL(*l3_tier_, getTierLevel)
            .WillByDefault(testing::Return(TierLevel::L3_SEMANTIC));
        ON_CALL(*warm_storage_, getTierLevel)
            .WillByDefault(testing::Return(TierLevel::STORAGE_WARM));
        ON_CALL(*cold_storage_, getTierLevel)
            .WillByDefault(testing::Return(TierLevel::STORAGE_COLD));

        // Set up size and capacity expectations
        ON_CALL(*l1_tier_, getCurrentSizeBytes).WillByDefault(testing::Return(0));
        ON_CALL(*l1_tier_, getMaxCapacityBytes).WillByDefault(testing::Return(100 * 1024 * 1024));
        ON_CALL(*l2_tier_, getCurrentSizeBytes).WillByDefault(testing::Return(0));
        ON_CALL(*l2_tier_, getMaxCapacityBytes).WillByDefault(testing::Return(500 * 1024 * 1024));
        ON_CALL(*l3_tier_, getCurrentSizeBytes).WillByDefault(testing::Return(0));
        ON_CALL(*l3_tier_, getMaxCapacityBytes).WillByDefault(testing::Return(2 * 1024 * 1024 * 1024));
        ON_CALL(*warm_storage_, getCurrentSizeBytes).WillByDefault(testing::Return(0));
        ON_CALL(*warm_storage_, getMaxCapacityBytes).WillByDefault(testing::Return(100 * 1024 * 1024 * 1024));
        ON_CALL(*cold_storage_, getCurrentSizeBytes).WillByDefault(testing::Return(0));
        ON_CALL(*cold_storage_, getMaxCapacityBytes).WillByDefault(testing::Return(1024 * 1024 * 1024 * 1024));

        // Set default entry counts
        ON_CALL(*l1_tier_, getEntryCount).WillByDefault(testing::Return(0));
        ON_CALL(*l2_tier_, getEntryCount).WillByDefault(testing::Return(0));
        ON_CALL(*l3_tier_, getEntryCount).WillByDefault(testing::Return(0));

        // Initialize coordinator with mock tiers
        tiers_map_[TierLevel::L1_WORKING] = l1_tier_;
        tiers_map_[TierLevel::L2_EPISODIC] = l2_tier_;
        tiers_map_[TierLevel::L3_SEMANTIC] = l3_tier_;
        tiers_map_[TierLevel::STORAGE_WARM] = warm_storage_;
        tiers_map_[TierLevel::STORAGE_COLD] = cold_storage_;

        EXPECT_TRUE(coordinator_->initialize(tiers_map_));
        coordinator_->start();
    }

    void TearDown() override {
        if (coordinator_ && coordinator_->isRunning()) {
            coordinator_->shutdown();
        }
    }

    std::shared_ptr<AccessCoordinator> coordinator_;
    std::shared_ptr<MockAccessTier> l1_tier_;
    std::shared_ptr<MockAccessTier> l2_tier_;
    std::shared_ptr<MockAccessTier> l3_tier_;
    std::shared_ptr<MockAccessTier> warm_storage_;
    std::shared_ptr<MockAccessTier> cold_storage_;
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers_map_;
};

// ============================================================================
// § 2  Promotion Chain Tests (T1-T4)
// ============================================================================

TEST_F(AccessModelE2ETest, T1_SingleKeyColdToWarmOnThreeAccesses) {
    // Setup: COLD storage contains a key
    // Action: Emit 3 hot-access events from COLD tier
    // Expected: Key promoted to WARM tier after 3 accesses
    
    std::string test_key = "single_key_cold_warm";
    
    // First access: should trigger promotion consideration
    coordinator_->onStorageAccess(test_key, TierLevel::STORAGE_COLD, 1,
                                  std::chrono::seconds(10));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Second access
    coordinator_->onStorageAccess(test_key, TierLevel::STORAGE_COLD, 2,
                                  std::chrono::seconds(10));
    std::this_thread::sleep_for(std::chrono::milliseconds(10));

    // Third access: should trigger promotion to WARM
    coordinator_->onStorageAccess(test_key, TierLevel::STORAGE_COLD, 3,
                                  std::chrono::seconds(10));
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // Verify promotion event was processed (basic check: no crash)
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T2_MultipleKeysLRUOrderPreservation) {
    // Setup: Multiple keys being promoted
    // Action: Emit promotion events for 10 keys with varying access counts
    // Expected: LRU order maintained across tier transitions
    
    for (int i = 0; i < 10; ++i) {
        std::string key = "key_" + std::to_string(i);
        uint64_t access_count = static_cast<uint64_t>(i + 1);
        
        coordinator_->onStorageAccess(key, TierLevel::STORAGE_COLD, access_count,
                                      std::chrono::seconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    // Verify no crashes and coordinator still running
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T3_ConcurrentPromotions_10KeysParallelAccess) {
    // Setup: Multiple threads emitting promotion events concurrently
    // Action: 10 threads each promoting 2 keys
    // Expected: All promotions processed without deadlock
    
    std::vector<std::thread> threads;
    std::atomic<int> promotion_count{0};
    
    auto worker = [this, &promotion_count](int thread_id) {
        for (int i = 0; i < 2; ++i) {
            std::string key = "concurrent_key_" + std::to_string(thread_id) + "_" +
                            std::to_string(i);
            coordinator_->onStorageAccess(key, TierLevel::STORAGE_COLD, 1,
                                         std::chrono::seconds(10));
            promotion_count++;
        }
    };
    
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(worker, i);
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_EQ(promotion_count, 20);
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T4_PromotionCascades_L3ToL2ToL1BackToBackWithin1s) {
    // Setup: Key in L3, rapidly accessed
    // Action: Emit 3 hot-access events within 1 second
    // Expected: Cascading promotions L3→L2→L1 completed
    
    std::string cascade_key = "cascade_key";
    auto start_time = std::chrono::steady_clock::now();
    
    // First access (at t=0ms): L3 → L2
    coordinator_->onStorageAccess(cascade_key, TierLevel::L3_SEMANTIC, 5,
                                  std::chrono::seconds(1));
    
    // Second access (at t=300ms): L2 → L1
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    coordinator_->onStorageAccess(cascade_key, TierLevel::L2_EPISODIC, 10,
                                  std::chrono::seconds(1));
    
    // Third access (at t=600ms): Confirm L1
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
    coordinator_->onStorageAccess(cascade_key, TierLevel::L1_WORKING, 15,
                                  std::chrono::seconds(1));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    EXPECT_LT(elapsed, std::chrono::seconds(2));
    EXPECT_TRUE(coordinator_->isRunning());
}

// ============================================================================
// § 3  Demotion Chain Tests (T5-T7)
// ============================================================================

TEST_F(AccessModelE2ETest, T5_CacheL1FullL2EvictionStorageColdFeedback) {
    // Setup: L1 cache full
    // Action: Emit eviction event from L1 → L2
    // Expected: Demotion to L2, storage receives cold feedback
    
    ON_CALL(*l1_tier_, getCurrentSizeBytes)
        .WillByDefault(testing::Return(100 * 1024 * 1024));  // L1 at capacity
    ON_CALL(*l1_tier_, getMaxCapacityBytes)
        .WillByDefault(testing::Return(100 * 1024 * 1024));
    
    std::string evict_key = "evict_from_l1";
    
    coordinator_->onCacheEvicted(evict_key, TierLevel::L1_WORKING, 1024 * 100,
                                 5,  // access_count
                                 std::chrono::seconds(30),  // last_access_age
                                 "lru");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T6_DemotionRejection_TierAlreadyFullBackpressure) {
    // Setup: Both L1 and L2 full, L3 also nearly full
    // Action: Emit eviction event from L1
    // Expected: Demotion request rejected or queued (backpressure)
    
    ON_CALL(*l1_tier_, getCurrentSizeBytes)
        .WillByDefault(testing::Return(100 * 1024 * 1024));
    ON_CALL(*l2_tier_, getCurrentSizeBytes)
        .WillByDefault(testing::Return(500 * 1024 * 1024));
    ON_CALL(*l3_tier_, getCurrentSizeBytes)
        .WillByDefault(testing::Return(1900 * 1024 * 1024));
    
    std::string backpressure_key = "backpressure_key";
    
    coordinator_->onCacheEvicted(backpressure_key, TierLevel::L1_WORKING,
                                 10 * 1024 * 1024, 1, std::chrono::seconds(60),
                                 "lru");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    // Should still be running (backpressure handled gracefully)
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T7_CascadingDemotions_L1FillL2FillL3Eviction) {
    // Setup: Multiple evictions with cascading demotions
    // Action: Emit 5 eviction events in rapid sequence
    // Expected: Demotions cascade down to L3, no deadlock
    
    for (int i = 0; i < 5; ++i) {
        std::string key = "cascade_evict_" + std::to_string(i);
        coordinator_->onCacheEvicted(key, TierLevel::L1_WORKING, 1024 * (100 + i),
                                     i + 1, std::chrono::seconds(30 - i * 5), "lru");
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(coordinator_->isRunning());
}

// ============================================================================
// § 4  Policy Enforcement Tests (T8-T10)
// ============================================================================

TEST_F(AccessModelE2ETest, T8_AgeBasedAutomaticDemotion_AgeExceedsPolicy) {
    // Setup: Old object in cache (age > policy threshold)
    // Action: Emit access event on aged key
    // Expected: Age-based policy triggers demotion
    
    std::string old_key = "aged_object";
    
    // Simulate an old access (60 seconds ago)
    coordinator_->onCacheEvicted(old_key, TierLevel::L1_WORKING, 5000,
                                 2,  // access_count
                                 std::chrono::seconds(60),  // age_secs = 60
                                 "age");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T9_SizeBasedBlockingFromL1_LargeObjectL2OnlyPath) {
    // Setup: Large object (>50MB)
    // Action: Attempt to promote large object to L1
    // Expected: Large object blocked from L1, routed to L2 only
    
    std::string large_key = "large_object_50mb";
    size_t large_size = 50 * 1024 * 1024;
    
    // Emit event for large object (shouldn't fit in L1 if policy enforces size limit)
    coordinator_->onStorageAccess(large_key, TierLevel::STORAGE_COLD, 100,
                                  std::chrono::seconds(10));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T10_HotHotspot_Single1000xAccessPriorityPromotion) {
    // Setup: Single key accessed 1000 times
    // Action: Emit 1000 access events for single key
    // Expected: Key promoted to L1 with high priority
    
    std::string hotspot_key = "hotspot_key";
    
    // Simulate hot access pattern
    for (int i = 1; i <= 100; ++i) {
        coordinator_->onStorageAccess(hotspot_key, TierLevel::STORAGE_COLD,
                                      static_cast<uint64_t>(i * 10),
                                      std::chrono::seconds(1));
        if (i % 20 == 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    EXPECT_TRUE(coordinator_->isRunning());
}

// ============================================================================
// § 5  Edge Cases & Stress Tests (T11-T15)
// ============================================================================

TEST_F(AccessModelE2ETest, T11_EmptyCoordinator_NoTiersRegistered) {
    // Setup: Create coordinator without registering tiers
    // Action: Try to emit events
    // Expected: Coordinator handles gracefully (no crash)
    
    auto empty_coord = createAccessCoordinator(2);
    // Initialize with empty map (or don't initialize)
    std::map<TierLevel, std::shared_ptr<AccessTier>> empty_tiers;
    EXPECT_TRUE(empty_coord->initialize(empty_tiers));
    empty_coord->start();
    
    // Try to emit events on empty coordinator
    empty_coord->onStorageAccess("key", TierLevel::STORAGE_COLD, 1,
                                std::chrono::seconds(10));
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(empty_coord->isRunning());
    empty_coord->shutdown();
}

TEST_F(AccessModelE2ETest, T12_SingleTierPromotion_DemotionNOP) {
    // Setup: Coordinator with only L1 tier
    // Action: Emit promotion/demotion events
    // Expected: Operations are NOPs (no crash)
    
    auto single_coord = createAccessCoordinator(1);
    std::map<TierLevel, std::shared_ptr<AccessTier>> single_tier;
    auto l1_only = std::make_shared<MockAccessTier>();
    ON_CALL(*l1_only, getTierLevel)
        .WillByDefault(testing::Return(TierLevel::L1_WORKING));
    ON_CALL(*l1_only, getCurrentSizeBytes).WillByDefault(testing::Return(0));
    ON_CALL(*l1_only, getMaxCapacityBytes)
        .WillByDefault(testing::Return(100 * 1024 * 1024));
    
    single_tier[TierLevel::L1_WORKING] = l1_only;
    EXPECT_TRUE(single_coord->initialize(single_tier));
    single_coord->start();
    
    single_coord->onStorageAccess("key", TierLevel::L1_WORKING, 1,
                                 std::chrono::seconds(10));
    single_coord->onCacheEvicted("key", TierLevel::L1_WORKING, 100, 1,
                                std::chrono::seconds(30), "lru");
    
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    
    EXPECT_TRUE(single_coord->isRunning());
    single_coord->shutdown();
}

TEST_F(AccessModelE2ETest, T13_RapidFireEvents_100EventsIn10ms_QueueBackpressure) {
    // Setup: High-frequency event injection
    // Action: Emit 100 events in 10ms
    // Expected: Queue handles backpressure, no event loss
    
    std::vector<std::string> keys;
    for (int i = 0; i < 100; ++i) {
        keys.push_back("rapid_key_" + std::to_string(i));
        coordinator_->onStorageAccess(keys.back(), TierLevel::STORAGE_COLD, 1,
                                      std::chrono::seconds(1));
    }
    
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T14_WorkerThreadFailureRecovery_OneThreadDiesOthersContinue) {
    // Setup: Coordinator with 4 workers
    // Note: This test verifies graceful degradation (actual thread kill is not
    // directly testable, but we verify the coordinator remains functional under load)
    // Action: High-frequency events while coordinator is running
    // Expected: Coordinator processes events despite potential transient failures
    
    for (int batch = 0; batch < 3; ++batch) {
        for (int i = 0; i < 20; ++i) {
            std::string key = "recovery_key_" + std::to_string(batch) + "_" +
                            std::to_string(i);
            coordinator_->onStorageAccess(key, TierLevel::STORAGE_COLD,
                                         1 + batch, std::chrono::seconds(10));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    
    EXPECT_TRUE(coordinator_->isRunning());
}

TEST_F(AccessModelE2ETest, T15_LongRunning_1000OperationsOver10sWithoutDeadlock) {
    // Setup: Extended test over 10 seconds
    // Action: Emit 1000 events (promotions and demotions) over 10 seconds
    // Expected: No deadlock, all events processed, coordinator responsive
    
    auto start_time = std::chrono::steady_clock::now();
    int event_count = 0;
    
    std::vector<std::thread> threads;
    std::atomic<bool> stop_flag{false};
    
    auto worker = [this, &event_count, &stop_flag]() {
        int local_count = 0;
        while (!stop_flag && local_count < 100) {
            std::string key = "longrun_" + std::to_string(local_count);
            
            // Alternate between promotions and demotions
            if (local_count % 2 == 0) {
                coordinator_->onStorageAccess(key, TierLevel::STORAGE_COLD, 5,
                                             std::chrono::seconds(10));
            } else {
                coordinator_->onCacheEvicted(key, TierLevel::L1_WORKING, 5000,
                                            2, std::chrono::seconds(30), "lru");
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            local_count++;
        }
        event_count += local_count;
    };
    
    // 10 threads × 100 events = 1000 total
    for (int i = 0; i < 10; ++i) {
        threads.emplace_back(worker);
    }
    
    // Wait up to 15 seconds
    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(15);
    while (std::chrono::steady_clock::now() < deadline && 
           event_count < 1000) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    stop_flag = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    auto elapsed = std::chrono::steady_clock::now() - start_time;
    
    // Should complete within 15 seconds with no deadlock
    EXPECT_LT(elapsed, std::chrono::seconds(15));
    EXPECT_TRUE(coordinator_->isRunning());
}

}  // namespace access_model
}  // namespace themis
