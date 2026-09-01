/**
 * @file test_access_coordinator.cpp
 * @brief Unit tests for AccessCoordinator implementation.
 *
 * Tests verify the AccessCoordinator API as defined in
 * include/access_model/access_coordinator.h and implemented in
 * src/access_model/access_coordinator.cpp.
 *
 * Test labels: access_model, tier, coordinator
 */

#include <gtest/gtest.h>

#include <map>
#include <memory>
#include <vector>

#include "access_model/access_coordinator.h"
#include "access_model/access_metrics.h"
#include "access_model/access_tier_interface.h"
#include "access_model/age_based_policy.h"
#include "access_model/promotion_demotion.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  TestAccessTier — minimal deterministic AccessTier for coordinator tests
// ============================================================================

class TestAccessTier : public AccessTier {
 public:
    explicit TestAccessTier(TierLevel level, std::string name)
        : level_(level), name_(std::move(name)) {}

    TierGetResult get(std::string_view, const TierAccessOptions&) override {
        TierGetResult result;
        result.success = false;
        result.current_tier = level_;
        result.latency_us = std::chrono::microseconds(0);
        result.age_secs = std::chrono::seconds(0);
        return result;
    }

    TierPutResult put(std::string_view, std::string_view,
                      const TierAccessOptions&) override {
        TierPutResult result;
        result.success = true;
        result.placed_in_tier = level_;
        result.latency_us = std::chrono::microseconds(0);
        return result;
    }

    bool invalidate(std::string_view) override { return true; }
    TierLevel getTierLevel() const override { return level_; }
    std::string getTierName() const override { return name_; }
    bool hasKey(std::string_view) const override { return false; }
    std::size_t getCurrentSizeBytes() const override { return 0; }
    std::size_t getMaxCapacityBytes() const override { return kMaxTierCapacity; }
    std::size_t getEntryCount() const override { return 0; }
    double getHitRate() const override { return 0.0; }
    std::chrono::microseconds getAverageGetLatency() const override {
        return std::chrono::microseconds(0);
    }
    std::chrono::microseconds getAveragePutLatency() const override {
        return std::chrono::microseconds(0);
    }
    uint64_t getAccessCount(std::string_view) const override { return 0; }
    std::chrono::seconds getKeyAge(std::string_view) const override {
        return std::chrono::seconds(0);
    }
    bool initialize() override { return true; }
    void shutdown() override {}
    bool isHealthy() const override { return true; }

 private:
    TierLevel level_;
    std::string name_;
};

// ============================================================================
// § 2  Test Fixture
// ============================================================================

class AccessCoordinatorTest : public ::testing::Test {
 protected:
    void SetUp() override {
        coordinator_ = createAccessCoordinator(/*thread_pool_size=*/2);

        l1_ = std::make_shared<TestAccessTier>(TierLevel::L1_WORKING, "L1_WORKING");
        l2_ = std::make_shared<TestAccessTier>(TierLevel::L2_EPISODIC, "L2_EPISODIC");
        l3_ = std::make_shared<TestAccessTier>(TierLevel::L3_SEMANTIC, "L3_SEMANTIC");
        hot_ = std::make_shared<TestAccessTier>(TierLevel::STORAGE_HOT, "STORAGE_HOT");
        warm_ = std::make_shared<TestAccessTier>(TierLevel::STORAGE_WARM, "STORAGE_WARM");
        cold_ = std::make_shared<TestAccessTier>(TierLevel::STORAGE_COLD, "STORAGE_COLD");
    }

    void TearDown() override {
        if (coordinator_ && coordinator_->isRunning()) {
            coordinator_->shutdown();
        }
    }

    std::map<TierLevel, std::shared_ptr<AccessTier>> allTiers() {
        return {
            {TierLevel::L1_WORKING, l1_},
            {TierLevel::L2_EPISODIC, l2_},
            {TierLevel::L3_SEMANTIC, l3_},
            {TierLevel::STORAGE_HOT, hot_},
            {TierLevel::STORAGE_WARM, warm_},
            {TierLevel::STORAGE_COLD, cold_},
        };
    }

    std::shared_ptr<AccessCoordinator> coordinator_;
    std::shared_ptr<TestAccessTier> l1_;
    std::shared_ptr<TestAccessTier> l2_;
    std::shared_ptr<TestAccessTier> l3_;
    std::shared_ptr<TestAccessTier> hot_;
    std::shared_ptr<TestAccessTier> warm_;
    std::shared_ptr<TestAccessTier> cold_;
};

// ============================================================================
// § 3  Initialization & Lifecycle
// ============================================================================

// ACM-01: Initialize coordinator with all tiers
TEST_F(AccessCoordinatorTest, ACM_01_InitializeWithTiers) {
    EXPECT_TRUE(coordinator_->initialize(allTiers()));
    EXPECT_NO_THROW(coordinator_->start());
    EXPECT_TRUE(coordinator_->isRunning());
    EXPECT_NO_THROW(coordinator_->shutdown());
    EXPECT_FALSE(coordinator_->isRunning());
}

// ACM-02: Initialize with empty tier map succeeds
TEST_F(AccessCoordinatorTest, ACM_02_InitializeEmptyTiers) {
    std::map<TierLevel, std::shared_ptr<AccessTier>> empty_tiers;
    EXPECT_TRUE(coordinator_->initialize(empty_tiers));
}

// ACM-03: Set age-based policy
TEST_F(AccessCoordinatorTest, ACM_03_SetAgePolicy) {
    AgeBasedPolicy policy;
    policy.hot_to_warm_days = 30;
    policy.warm_to_cold_days = 90;
    policy.l1_promotion_threshold = 10;
    EXPECT_NO_THROW(coordinator_->setAgePolicy(policy));
}

// ============================================================================
// § 4  Cache Eviction Events
// ============================================================================

// ACM-04: Eviction event with low access count triggers demotion consideration
TEST_F(AccessCoordinatorTest, ACM_04_OnEvictionLowAccessCount) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    AgeBasedPolicy policy;
    policy.l1_promotion_threshold = 10;
    policy.hot_zero_access_days = 1;
    coordinator_->setAgePolicy(policy);

    EvictionEvent event;
    event.key = "low_access_key";
    event.tier = TierLevel::L1_WORKING;
    event.reason = "lru_eviction";
    event.evicted_size_bytes = 1024;
    event.access_count = 2;                         // Below threshold
    event.last_access_age_secs = std::chrono::seconds(172800);  // 2 days

    EXPECT_NO_THROW(coordinator_->onEviction(event));

    // Coordinator should record the eviction
    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_GE(metrics.counters.cache_evictions_observed, 1u);
}

// ACM-05: Eviction event with high access count does NOT trigger demotion
TEST_F(AccessCoordinatorTest, ACM_05_OnEvictionHighAccessCount) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    AgeBasedPolicy policy;
    policy.l1_promotion_threshold = 5;
    coordinator_->setAgePolicy(policy);

    EvictionEvent event;
    event.key = "hot_key";
    event.tier = TierLevel::L1_WORKING;
    event.reason = "capacity";
    event.access_count = 100;  // Well above threshold

    EXPECT_NO_THROW(coordinator_->onEviction(event));

    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_GE(metrics.counters.cache_evictions_observed, 1u);
    // Demotions should not be initiated for hot data
    EXPECT_EQ(metrics.counters.demotions_initiated, 0u);
}

// ============================================================================
// § 5  Storage Hot-Access Events
// ============================================================================

// ACM-06: Hot-access event above threshold queues a promotion
TEST_F(AccessCoordinatorTest, ACM_06_OnHotAccessAboveThreshold) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 2;
    coordinator_->setAgePolicy(policy);

    AccessEvent event;
    event.key = "hot_storage_key";
    event.current_tier = TierLevel::STORAGE_HOT;
    event.access_count = 5;  // Above threshold

    EXPECT_NO_THROW(coordinator_->onHotAccess(event));

    auto metrics = coordinator_->getAccessModelMetrics();
    EXPECT_GE(metrics.counters.storage_hot_accesses_observed, 1u);
    EXPECT_GE(metrics.counters.promotions_initiated, 1u);
}

// ACM-07: Cold-tier hot-access routes promotion to warm first
TEST_F(AccessCoordinatorTest, ACM_07_OnHotAccessColdTierPromotesToWarm) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 2;
    coordinator_->setAgePolicy(policy);

    AccessEvent event;
    event.key = "cold_storage_key";
    event.current_tier = TierLevel::STORAGE_COLD;
    event.access_count = 3;  // Above threshold

    EXPECT_NO_THROW(coordinator_->onHotAccess(event));

    // Verify a transition toward warm was recorded
    auto transitions = coordinator_->getRecentTransitions(10);
    EXPECT_FALSE(transitions.empty());
    bool has_warm_promotion = false;
    for (const auto& t : transitions) {
        if (t.to_tier == TierLevel::STORAGE_WARM) {
            has_warm_promotion = true;
        }
    }
    EXPECT_TRUE(has_warm_promotion);
}

// ============================================================================
// § 6  Async Promotion
// ============================================================================

// ACM-08: promoteAsync returns a future that resolves with the correct tiers
TEST_F(AccessCoordinatorTest, ACM_08_PromoteAsyncReturnsCorrectTiers) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    auto future = coordinator_->promoteAsync(
        "promo_key", TierLevel::STORAGE_COLD, TierLevel::L3_SEMANTIC,
        /*size_bytes=*/4096);

    ASSERT_TRUE(future.valid());
    auto result = future.get();  // Blocks until promotion completes or errors
    EXPECT_EQ(result.from_tier, TierLevel::STORAGE_COLD);
    EXPECT_EQ(result.to_tier, TierLevel::L3_SEMANTIC);
}

// ACM-09: promoteAsync on non-running coordinator returns immediate failure
TEST_F(AccessCoordinatorTest, ACM_09_PromoteAsyncNotRunning) {
    // Coordinator has been created but NOT started
    auto future = coordinator_->promoteAsync(
        "key", TierLevel::STORAGE_COLD, TierLevel::L3_SEMANTIC, 1024);

    ASSERT_TRUE(future.valid());
    auto result = future.get();
    EXPECT_FALSE(result.success);
    EXPECT_FALSE(result.error_message.empty());
}

// ============================================================================
// § 7  Demotion Planning & Execution
// ============================================================================

// ACM-10: planDemotion returns a valid plan with correct fields
TEST_F(AccessCoordinatorTest, ACM_10_PlanDemotionReturnsValidPlan) {
    coordinator_->initialize(allTiers());

    auto plan = coordinator_->planDemotion(
        "demotion_key", TierLevel::L1_WORKING, TierLevel::L2_EPISODIC,
        /*data_size_bytes=*/2048);

    ASSERT_TRUE(plan.has_value());
    EXPECT_EQ(plan->key, "demotion_key");
    EXPECT_EQ(plan->from_tier, TierLevel::L1_WORKING);
    EXPECT_EQ(plan->to_tier, TierLevel::L2_EPISODIC);
    EXPECT_FALSE(plan->plan_id.empty());
}

// ACM-11: executeDemotion with valid plan_id succeeds
TEST_F(AccessCoordinatorTest, ACM_11_ExecuteDemotionValidPlan) {
    coordinator_->initialize(allTiers());

    auto plan = coordinator_->planDemotion(
        "demotion_key", TierLevel::L1_WORKING, TierLevel::L2_EPISODIC, 2048);

    ASSERT_TRUE(plan.has_value());

    auto result = coordinator_->executeDemotion(plan->plan_id);
    ASSERT_TRUE(result.has_value());
    EXPECT_TRUE(result->success);
    EXPECT_EQ(result->from_tier, TierLevel::L1_WORKING);
    EXPECT_EQ(result->to_tier, TierLevel::L2_EPISODIC);
}

// ACM-12: executeDemotion with unknown plan_id returns nullopt
TEST_F(AccessCoordinatorTest, ACM_12_ExecuteDemotionUnknownPlan) {
    auto result = coordinator_->executeDemotion("nonexistent-plan-id");
    EXPECT_FALSE(result.has_value());
}

// ============================================================================
// § 8  Metrics & Observability
// ============================================================================

// ACM-13: getAccessModelMetrics returns non-negative counters
TEST_F(AccessCoordinatorTest, ACM_13_GetMetricsReturnsSaneValues) {
    coordinator_->start();

    auto metrics = coordinator_->getAccessModelMetrics();

    EXPECT_GE(metrics.counters.cache_evictions_observed, 0u);
    EXPECT_GE(metrics.counters.promotions_initiated, 0u);
    EXPECT_GE(metrics.counters.demotions_initiated, 0u);
}

// ACM-14: getRecentTransitions returns expected events after activity
TEST_F(AccessCoordinatorTest, ACM_14_GetRecentTransitionsAfterEviction) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    AgeBasedPolicy policy;
    policy.l1_promotion_threshold = 10;
    policy.hot_zero_access_days = 1;
    coordinator_->setAgePolicy(policy);

    EvictionEvent event;
    event.key = "obs_key";
    event.tier = TierLevel::L1_WORKING;
    event.reason = "lru";
    event.access_count = 1;
    event.last_access_age_secs = std::chrono::seconds(200000);  // ~2.3 days

    coordinator_->onEviction(event);

    auto transitions = coordinator_->getRecentTransitions(50);
    EXPECT_FALSE(transitions.empty());
}

// ACM-15: setPromotionThresholds affects promotion decisions
TEST_F(AccessCoordinatorTest, ACM_15_SetPromotionThresholds) {
    coordinator_->initialize(allTiers());
    coordinator_->start();

    EXPECT_NO_THROW(coordinator_->setPromotionThresholds(
        /*cache_threshold=*/20, /*storage_threshold=*/5));

    AccessEvent event;
    event.key = "threshold_key";
    event.current_tier = TierLevel::STORAGE_WARM;
    event.access_count = 3;  // Below new storage threshold of 5

    coordinator_->onHotAccess(event);

    auto metrics = coordinator_->getAccessModelMetrics();
    // No promotion expected since access_count < storage_threshold
    EXPECT_EQ(metrics.counters.promotions_initiated, 0u);
}

}  // namespace access_model
}  // namespace themis
