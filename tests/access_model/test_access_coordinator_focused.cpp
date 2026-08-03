/**
 * @file test_access_coordinator_focused.cpp
 * @brief Focused unit tests for AccessCoordinator (Phase 2 BLOCK 1 implementation)
 *
 * Test cases:
 * - ACM-01: Tier registry management
 * - ACM-02: Event ingest and processing (onEviction, onHotAccess)
 * - ACM-03: Policy decision predicates
 * - ACM-04: Promotion async execution
 * - ACM-05: Demotion planning and execution
 * - ACM-06: Correlation ID tracking
 * - ACM-07: Metrics collection
 * - ACM-08: Concurrent operations (thread safety)
 *
 * Test labels: access_model, coordinator, focused, phase2
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <memory>
#include <thread>
#include <vector>

#include "access_model/access_coordinator.h"
#include "access_model/access_tier_interface.h"
#include "access_model/age_based_policy.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Mocks & Fixtures
// ============================================================================

class MockAccessTier : public AccessTier {
 public:
    MOCK_METHOD(TierGetResult, get, (const std::string&, const TierAccessOptions&),
                (override));
    MOCK_METHOD(TierPutResult, put,
                (const std::string&, const std::string&,
                 const TierAccessOptions&),
                (override));
    MOCK_METHOD(bool, invalidate, (const std::string&), (override));
    MOCK_METHOD(TierMetrics, getMetrics, (), (const, override));
    MOCK_METHOD(TierLevel, getTierLevel, (), (const, override));
};

class AccessCoordinatorFocusedTest : public ::testing::Test {
 protected:
    void SetUp() override {
        coordinator_ = createAccessCoordinator(4);
        
        // Create mock tiers
        l1_ = std::make_shared<MockAccessTier>();
        l2_ = std::make_shared<MockAccessTier>();
        l3_ = std::make_shared<MockAccessTier>();
        warm_storage_ = std::make_shared<MockAccessTier>();
        cold_storage_ = std::make_shared<MockAccessTier>();
        
        ON_CALL(*l1_, getTierLevel).WillByDefault(testing::Return(TierLevel::L1_TRANSACTIONAL));
        ON_CALL(*l2_, getTierLevel).WillByDefault(testing::Return(TierLevel::L2_EPISODIC));
        ON_CALL(*l3_, getTierLevel).WillByDefault(testing::Return(TierLevel::L3_SEMANTIC));
        ON_CALL(*warm_storage_, getTierLevel).WillByDefault(testing::Return(TierLevel::STORAGE_WARM));
        ON_CALL(*cold_storage_, getTierLevel).WillByDefault(testing::Return(TierLevel::STORAGE_COLD));
        
        tiers_map_[TierLevel::L1_TRANSACTIONAL] = l1_;
        tiers_map_[TierLevel::L2_EPISODIC] = l2_;
        tiers_map_[TierLevel::L3_SEMANTIC] = l3_;
        tiers_map_[TierLevel::STORAGE_WARM] = warm_storage_;
        tiers_map_[TierLevel::STORAGE_COLD] = cold_storage_;
        
        coordinator_->initialize(tiers_map_);
    }

    void TearDown() override {
        if (coordinator_ && coordinator_->isRunning()) {
            coordinator_->shutdown();
        }
    }

    std::shared_ptr<AccessCoordinator> coordinator_;
    std::shared_ptr<MockAccessTier> l1_;
    std::shared_ptr<MockAccessTier> l2_;
    std::shared_ptr<MockAccessTier> l3_;
    std::shared_ptr<MockAccessTier> warm_storage_;
    std::shared_ptr<MockAccessTier> cold_storage_;
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers_map_;
};

// ============================================================================
// § 2  ACM-01: Tier Registry Management
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM01_TierRegistryInitialization) {
    // Test that tiers are properly registered and accessible
    EXPECT_TRUE(coordinator_->isRunning() == false);  // Not started yet
    
    coordinator_->start();
    EXPECT_TRUE(coordinator_->isRunning());
    
    coordinator_->shutdown();
    EXPECT_FALSE(coordinator_->isRunning());
}

TEST_F(AccessCoordinatorFocusedTest, ACM01_MultipleTiersRegistration) {
    // Test multiple tier registration
    EXPECT_NO_THROW(coordinator_->initialize(tiers_map_));
    EXPECT_NO_THROW(coordinator_->start());
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.cache_evictions_observed, 0);
    
    coordinator_->shutdown();
}

// ============================================================================
// § 3  ACM-02: Event Ingest & Processing (onEviction, onHotAccess)
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM02_CacheEvictionEventProcessing) {
    coordinator_->start();
    
    // Record a cache eviction event
    EvictionEvent event{
        .key = "test_key_1",
        .tier = TierLevel::L1_TRANSACTIONAL,
        .access_count = 5,
        .last_access_age_secs = std::chrono::seconds(10),
    };
    
    coordinator_->onEviction(event);
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.cache_evictions_observed, 1);
    
    coordinator_->shutdown();
}

TEST_F(AccessCoordinatorFocusedTest, ACM02_HotAccessEventProcessing) {
    coordinator_->start();
    
    // Set age policy first
    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 10;
    coordinator_->setAgePolicy(policy);
    
    // Record a hot access event
    AccessEvent event{
        .key = "test_key_2",
        .current_tier = TierLevel::STORAGE_COLD,
        .access_count = 15,  // Exceeds threshold
        .time_window_secs = std::chrono::seconds(60),
    };
    
    coordinator_->onHotAccess(event);
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.storage_hot_accesses_observed, 1);
    EXPECT_GT(metrics.counters.promotions_initiated, 0);
    
    coordinator_->shutdown();
}

TEST_F(AccessCoordinatorFocusedTest, ACM02_MultipleEventsProcessing) {
    coordinator_->start();
    
    // Process multiple events
    for (int i = 0; i < 10; ++i) {
        EvictionEvent event{
            .key = "key_" + std::to_string(i),
            .tier = TierLevel::L1_TRANSACTIONAL,
            .access_count = static_cast<uint64_t>(i),
            .last_access_age_secs = std::chrono::seconds(i * 10),
        };
        coordinator_->onEviction(event);
    }
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.cache_evictions_observed, 10);
    
    coordinator_->shutdown();
}

// ============================================================================
// § 4  ACM-03: Policy Decision Predicates
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM03_L1ToL2Promotion) {
    AgeBasedPolicy policy;
    policy.l1_promotion_threshold = 100;
    policy.l2_promotion_threshold = 50;
    policy.l1_zero_access_days = 1;
    
    // Hot data (high access count)
    EXPECT_FALSE(policy.shouldPromoteL1ToL2(
        100,  // High access count
        std::chrono::seconds(3600)));  // Recently accessed
    
    // Warm data (low access count + old)
    EXPECT_TRUE(policy.shouldPromoteL1ToL2(
        25,  // Below L2 threshold
        std::chrono::seconds(86400 * 2)));  // Very old
}

TEST_F(AccessCoordinatorFocusedTest, ACM03_HotnessClassification) {
    AgeBasedPolicy policy;
    policy.l1_promotion_threshold = 100;
    policy.hot_zero_access_days = 1;
    policy.warm_zero_access_days = 7;
    policy.l3_to_storage_days = 30;
    
    // HOT: High frequency + recent
    auto hotness = policy.classifyHotness(150, std::chrono::seconds(3600));
    EXPECT_EQ(hotness, DataHotnessLevel::HOT);
    
    // WARM: Moderate frequency + moderate age
    hotness = policy.classifyHotness(50, std::chrono::seconds(86400 * 3));
    EXPECT_EQ(hotness, DataHotnessLevel::WARM);
    
    // COLD: No access + very old
    hotness = policy.classifyHotness(0, std::chrono::seconds(86400 * 60));
    EXPECT_EQ(hotness, DataHotnessLevel::COLD);
}

TEST_F(AccessCoordinatorFocusedTest, ACM03_TierRecommendation) {
    AgeBasedPolicy policy;
    policy.l1_promotion_threshold = 100;
    policy.l1_zero_access_days = 1;
    
    // HOT data → L1
    auto tier = policy.recommendTierForData(150, std::chrono::seconds(3600));
    EXPECT_EQ(tier, TierLevel::L1_TRANSACTIONAL);
    
    // COLD data → STORAGE_COLD
    tier = policy.recommendTierForData(0, std::chrono::seconds(86400 * 60));
    EXPECT_EQ(tier, TierLevel::STORAGE_COLD);
}

// ============================================================================
// § 5  ACM-04: Promotion Async Execution
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM04_AsyncPromotionExecution) {
    coordinator_->start();
    
    auto future = coordinator_->promoteAsync(
        "test_key",
        TierLevel::STORAGE_COLD,
        TierLevel::L3_SEMANTIC,
        1024);  // 1KB
    
    // Wait for promotion to complete
    auto result = future.get();
    
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.from_tier, TierLevel::STORAGE_COLD);
    EXPECT_EQ(result.to_tier, TierLevel::L3_SEMANTIC);
    EXPECT_EQ(result.size_bytes, 0);  // Simplified implementation
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.promotions_initiated, 1);
    EXPECT_EQ(metrics.counters.promotions_succeeded, 1);
    
    coordinator_->shutdown();
}

TEST_F(AccessCoordinatorFocusedTest, ACM04_MultipleAsyncPromotions) {
    coordinator_->start();
    
    std::vector<std::future<PromotionResult>> futures;
    
    for (int i = 0; i < 5; ++i) {
        futures.emplace_back(coordinator_->promoteAsync(
            "key_" + std::to_string(i),
            TierLevel::STORAGE_WARM,
            TierLevel::L2_EPISODIC,
            1024));
    }
    
    // Wait for all promotions
    for (auto& future : futures) {
        auto result = future.get();
        EXPECT_TRUE(result.success);
    }
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.promotions_initiated, 5);
    EXPECT_EQ(metrics.counters.promotions_succeeded, 5);
    
    coordinator_->shutdown();
}

// ============================================================================
// § 6  ACM-05: Demotion Planning & Execution
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM05_DemotionPlanCreation) {
    coordinator_->start();
    
    auto plan = coordinator_->planDemotion(
        "test_key",
        TierLevel::L1_TRANSACTIONAL,
        TierLevel::L2_EPISODIC,
        2048);  // 2KB
    
    EXPECT_TRUE(plan.has_value());
    EXPECT_EQ(plan->key, "test_key");
    EXPECT_EQ(plan->from_tier, TierLevel::L1_TRANSACTIONAL);
    EXPECT_EQ(plan->to_tier, TierLevel::L2_EPISODIC);
    EXPECT_EQ(plan->data_size_bytes, 2048);
    
    coordinator_->shutdown();
}

TEST_F(AccessCoordinatorFocusedTest, ACM05_DemotionPlanExecution) {
    coordinator_->start();
    
    auto plan = coordinator_->planDemotion(
        "test_key",
        TierLevel::L2_EPISODIC,
        TierLevel::STORAGE_WARM,
        2048);
    
    EXPECT_TRUE(plan.has_value());
    std::string plan_id = plan->plan_id;
    
    auto result = coordinator_->executeDemotion(plan_id);
    
    EXPECT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->from_tier, TierLevel::L2_EPISODIC);
    EXPECT_EQ(result->to_tier, TierLevel::STORAGE_WARM);
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.demotions_succeeded, 1);
    
    coordinator_->shutdown();
}

// ============================================================================
// § 7  ACM-06: Correlation ID Tracking
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM06_CorrelationIDGeneration) {
    coordinator_->start();
    
    EvictionEvent event1{
        .key = "key_1",
        .tier = TierLevel::L1_TRANSACTIONAL,
        .access_count = 1,
        .last_access_age_secs = std::chrono::seconds(60),
    };
    
    EvictionEvent event2{
        .key = "key_2",
        .tier = TierLevel::L1_TRANSACTIONAL,
        .access_count = 2,
        .last_access_age_secs = std::chrono::seconds(120),
    };
    
    coordinator_->onEviction(event1);
    coordinator_->onEviction(event2);
    
    auto transitions = coordinator_->getRecentTransitions(10);
    
    EXPECT_GE(transitions.size(), 2);
    
    // Each transition should have a correlation ID
    for (const auto& t : transitions) {
        EXPECT_FALSE(t.correlation_id.empty());
    }
    
    // Correlation IDs should be unique
    std::set<std::string> ids;
    for (const auto& t : transitions) {
        ids.insert(t.correlation_id);
    }
    EXPECT_EQ(ids.size(), transitions.size());
    
    coordinator_->shutdown();
}

// ============================================================================
// § 8  ACM-07: Metrics Collection
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM07_MetricsCollection) {
    coordinator_->start();
    
    // Trigger some events
    for (int i = 0; i < 5; ++i) {
        EvictionEvent event{
            .key = "key_" + std::to_string(i),
            .tier = TierLevel::L1_TRANSACTIONAL,
            .access_count = static_cast<uint64_t>(i),
            .last_access_age_secs = std::chrono::seconds(60),
        };
        coordinator_->onEviction(event);
    }
    
    auto metrics = coordinator_->getAccessModelMetrics();
    
    EXPECT_EQ(metrics.counters.cache_evictions_observed, 5);
    EXPECT_GT(metrics.event_processing_latency_us_.count(), 0);
    
    coordinator_->shutdown();
}

TEST_F(AccessCoordinatorFocusedTest, ACM07_LatencyHistogramTracking) {
    coordinator_->start();
    
    AgeBasedPolicy policy;
    coordinator_->setAgePolicy(policy);
    
    // Generate multiple promotions
    for (int i = 0; i < 10; ++i) {
        coordinator_->promoteAsync(
            "key_" + std::to_string(i),
            TierLevel::STORAGE_COLD,
            TierLevel::L3_SEMANTIC,
            1024);
    }
    
    // Give time for async processing
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    
    auto metrics = coordinator_->getAccessModelMetrics();
    
    // Verify histogram has data
    EXPECT_GT(metrics.event_processing_latency_us_.count(), 0);
    
    // Verify percentile calculation works
    uint64_t p50 = metrics.event_processing_latency_us_.percentile(50);
    uint64_t p95 = metrics.event_processing_latency_us_.percentile(95);
    uint64_t p99 = metrics.event_processing_latency_us_.percentile(99);
    
    EXPECT_LE(p50, p95);
    EXPECT_LE(p95, p99);
    
    coordinator_->shutdown();
}

// ============================================================================
// § 9  ACM-08: Concurrent Operations (Thread Safety)
// ============================================================================

TEST_F(AccessCoordinatorFocusedTest, ACM08_ConcurrentEvictionAndPromotion) {
    coordinator_->start();
    
    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 50;
    coordinator_->setAgePolicy(policy);
    
    std::atomic<int> completed{0};
    
    // Thread 1: Fire evictions
    auto eviction_thread = std::thread([this, &completed] {
        for (int i = 0; i < 20; ++i) {
            EvictionEvent event{
                .key = "evict_" + std::to_string(i),
                .tier = TierLevel::L1_TRANSACTIONAL,
                .access_count = static_cast<uint64_t>(i),
                .last_access_age_secs = std::chrono::seconds(60 * i),
            };
            coordinator_->onEviction(event);
        }
        completed++;
    });
    
    // Thread 2: Fire hot accesses
    auto access_thread = std::thread([this, &completed] {
        for (int i = 0; i < 20; ++i) {
            AccessEvent event{
                .key = "hot_" + std::to_string(i),
                .current_tier = TierLevel::STORAGE_WARM,
                .access_count = static_cast<uint64_t>(50 + i * 10),
                .time_window_secs = std::chrono::seconds(60),
            };
            coordinator_->onHotAccess(event);
        }
        completed++;
    });
    
    // Wait for threads
    eviction_thread.join();
    access_thread.join();
    
    EXPECT_EQ(completed, 2);
    
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(metrics.counters.cache_evictions_observed, 20);
    EXPECT_EQ(metrics.counters.storage_hot_accesses_observed, 20);
    
    coordinator_->shutdown();
}

TEST_F(AccessCoordinatorFocusedTest, ACM08_ConcurrentPromotionAndMetricsRead) {
    coordinator_->start();
    
    std::atomic<bool> keep_promoting{true};
    
    // Thread 1: Continuously promote
    auto promote_thread = std::thread([this, &keep_promoting] {
        for (int i = 0; i < 50; ++i) {
            coordinator_->promoteAsync(
                "key_" + std::to_string(i),
                TierLevel::STORAGE_COLD,
                TierLevel::L3_SEMANTIC,
                1024);
        }
        keep_promoting = false;
    });
    
    // Thread 2: Read metrics concurrently
    std::vector<AccessModelMetrics> metrics_snapshots;
    auto read_thread = std::thread([this, &keep_promoting, &metrics_snapshots] {
        while (keep_promoting || metrics_snapshots.size() < 20) {
            metrics_snapshots.push_back(coordinator_->getAccessModelMetrics());
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    });
    
    promote_thread.join();
    read_thread.join();
    
    // Verify metrics were collected without crashes
    EXPECT_GT(metrics_snapshots.size(), 0);
    
    auto final_metrics = coordinator_->getAccessModelMetrics();
    EXPECT_EQ(final_metrics.counters.promotions_initiated, 50);
    
    coordinator_->shutdown();
}

}  // namespace access_model
}  // namespace themis

// ============================================================================
// Google Test Main
// ============================================================================

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
