/**
 * @file test_access_coordinator.cpp
 * @brief Unit tests for AccessCoordinator implementation.
 *
 * Test labels: access_model, tier, coordinator
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <memory>
#include <vector>

#include "access_model/access_coordinator.h"
#include "access_model/access_tier_interface.h"
#include "access_model/age_based_policy.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Mocks
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

// ============================================================================
// § 2  Tests: Initialization
// ============================================================================

class AccessCoordinatorTest : public ::testing::Test {
 protected:
    void SetUp() override {
        coordinator_ = createAccessCoordinator(2);
        
        // Create mock tiers
        l1_ = std::make_shared<MockAccessTier>();
        l2_ = std::make_shared<MockAccessTier>();
        l3_ = std::make_shared<MockAccessTier>();
        hot_ = std::make_shared<MockAccessTier>();
    }

    std::shared_ptr<AccessCoordinator> coordinator_;
    std::shared_ptr<MockAccessTier> l1_;
    std::shared_ptr<MockAccessTier> l2_;
    std::shared_ptr<MockAccessTier> l3_;
    std::shared_ptr<MockAccessTier> hot_;
};

// ACM-01: Initialize coordinator with tier registry
TEST_F(AccessCoordinatorTest, ACM_01_InitializeWithTiers) {
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers{
        {TierLevel::L1_WORKING, l1_},
        {TierLevel::L2_EPISODIC, l2_},
        {TierLevel::L3_SEMANTIC, l3_},
        {TierLevel::STORAGE_HOT, hot_},
    };

    EXPECT_NO_THROW(coordinator_->initialize(tiers));
    EXPECT_NO_THROW(coordinator_->start());
    EXPECT_NO_THROW(coordinator_->shutdown());
}

// ACM-02: Set age-based policy
TEST_F(AccessCoordinatorTest, ACM_02_SetAgePolicy) {
    AgeBasedPolicy policy;
    policy.hot_to_warm_days = 30;
    policy.warm_to_cold_days = 90;
    policy.l1_promotion_threshold = 10;

    EXPECT_NO_THROW(coordinator_->setAgePolicy(policy));
}

// ACM-03: Handle cache eviction event
TEST_F(AccessCoordinatorTest, ACM_03_OnEviction) {
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers{
        {TierLevel::L1_WORKING, l1_},
    };
    coordinator_->initialize(tiers);
    coordinator_->start();

    EvictionEvent event{
        .key = "test_key",
        .tier = TierLevel::L1_WORKING,
        .reason = "lru_eviction",
        .evicted_size_bytes = 1024,
    };

    EXPECT_NO_THROW(coordinator_->onEviction(event));
    
    auto transitions = coordinator_->getRecentTransitions(10);
    EXPECT_GT(transitions.size(), 0);

    coordinator_->shutdown();
}

// ACM-04: Handle storage hot access event
TEST_F(AccessCoordinatorTest, ACM_04_OnHotAccess) {
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers{
        {TierLevel::STORAGE_HOT, hot_},
        {TierLevel::L3_SEMANTIC, l3_},
    };
    coordinator_->initialize(tiers);
    coordinator_->start();

    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 2;
    coordinator_->setAgePolicy(policy);

    AccessEvent event{
        .key = "test_key",
        .current_tier = TierLevel::STORAGE_HOT,
        .access_count = 3,
    };

    EXPECT_NO_THROW(coordinator_->onHotAccess(event));

    coordinator_->shutdown();
}

// ============================================================================
// § 3  Tests: Promotion
// ============================================================================

// ACM-05: Async promotion operation
TEST_F(AccessCoordinatorTest, ACM_05_PromoteAsync) {
    coordinator_->start();

    auto future = coordinator_->promoteAsync(
        "test_key",
        TierLevel::STORAGE_COLD,
        TierLevel::L3_SEMANTIC,
        {.max_wait_ms = std::chrono::milliseconds(100)});

    // Should complete without blocking
    EXPECT_NO_THROW({
        auto result = future.get();
        EXPECT_EQ(result.from_tier, TierLevel::STORAGE_COLD);
        EXPECT_EQ(result.to_tier, TierLevel::L3_SEMANTIC);
    });

    coordinator_->shutdown();
}

// ============================================================================
// § 4  Tests: Demotion Planning
// ============================================================================

// ACM-06: Plan demotion with grace period
TEST_F(AccessCoordinatorTest, ACM_06_PlanDemotion) {
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers{
        {TierLevel::L1_WORKING, l1_},
        {TierLevel::L2_EPISODIC, l2_},
    };
    coordinator_->initialize(tiers);

    auto plan = coordinator_->planDemotion(
        "test_key",
        TierLevel::L1_WORKING,
        TierLevel::L2_EPISODIC,
        "age_based");

    EXPECT_TRUE(plan.has_value());
    EXPECT_EQ(plan->key, "test_key");
    EXPECT_EQ(plan->from_tier, TierLevel::L1_WORKING);
    EXPECT_EQ(plan->to_tier, TierLevel::L2_EPISODIC);
}

// ACM-07: Execute demotion plan
TEST_F(AccessCoordinatorTest, ACM_07_ExecuteDemotion) {
    std::map<TierLevel, std::shared_ptr<AccessTier>> tiers{
        {TierLevel::L1_WORKING, l1_},
        {TierLevel::L2_EPISODIC, l2_},
    };
    coordinator_->initialize(tiers);

    auto plan = coordinator_->planDemotion(
        "test_key",
        TierLevel::L1_WORKING,
        TierLevel::L2_EPISODIC,
        "age_based");

    EXPECT_TRUE(plan.has_value());

    auto result = coordinator_->executeDemotion(plan->plan_id);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->from_tier, TierLevel::L1_WORKING);
    EXPECT_EQ(result->to_tier, TierLevel::L2_EPISODIC);
}

// ============================================================================
// § 5  Tests: Metrics & Observability
// ============================================================================

// ACM-08: Retrieve metrics
TEST_F(AccessCoordinatorTest, ACM_08_GetMetrics) {
    coordinator_->start();

    auto metrics = coordinator_->getAccessModelMetrics();

    EXPECT_GE(metrics.counters.cache_evictions_observed, 0);
    EXPECT_GE(metrics.counters.promotions_initiated, 0);
    EXPECT_GE(metrics.counters.demotions_initiated, 0);

    coordinator_->shutdown();
}

}  // namespace access_model
}  // namespace themis
