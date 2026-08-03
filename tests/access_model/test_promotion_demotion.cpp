/**
 * @file test_promotion_demotion.cpp
 * @brief Unit tests for promotion/demotion data structures and operations.
 *
 * Test labels: access_model, promotion, demotion
 */

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <chrono>
#include <memory>

#include "access_model/age_based_policy.h"
#include "access_model/promotion_demotion.h"

namespace themis {
namespace access_model {

// ============================================================================
// § 1  Tests: Demotion Plan
// ============================================================================

class PromotionDemotionTest : public ::testing::Test {};

// APD-01: Create demotion plan with all fields
TEST_F(PromotionDemotionTest, APD_01_CreateDemotionPlan) {
    DemotionPlan plan{
        .plan_id = "plan-123",
        .key = "test_key",
        .from_tier = TierLevel::L1_WORKING,
        .to_tier = TierLevel::L2_EPISODIC,
        .reason = "age_based",
        .grace_period_secs = std::chrono::seconds(600),
        .data_size_bytes = 4096,
        .access_count_at_plan = 42,
        .is_scheduled = true,
    };

    EXPECT_EQ(plan.key, "test_key");
    EXPECT_EQ(plan.from_tier, TierLevel::L1_WORKING);
    EXPECT_EQ(plan.to_tier, TierLevel::L2_EPISODIC);
    EXPECT_EQ(plan.data_size_bytes, 4096);
    EXPECT_EQ(plan.access_count_at_plan, 42);
}

// APD-02: Demotion plan grace period calculation
TEST_F(PromotionDemotionTest, APD_02_GracePeriodCalculation) {
    auto now = std::chrono::system_clock::now();
    
    DemotionPlan plan{
        .plan_id = "plan-456",
        .key = "test_key",
        .from_tier = TierLevel::STORAGE_HOT,
        .to_tier = TierLevel::STORAGE_WARM,
        .reason = "age_based",
        .grace_period_secs = std::chrono::seconds(300),
        .created_at = now,
        .scheduled_execution_time = now + std::chrono::seconds(300),
    };

    auto expected_time = now + std::chrono::seconds(300);
    auto diff = std::chrono::duration_cast<std::chrono::milliseconds>(
        plan.scheduled_execution_time - expected_time);
    
    EXPECT_LE(abs(diff.count()), 10);  // Allow 10ms tolerance
}

// ============================================================================
// § 2  Tests: Demotion Result
// ============================================================================

// APD-03: Successful demotion result
TEST_F(PromotionDemotionTest, APD_03_SuccessfulDemotionResult) {
    DemotionResult result{
        .success = true,
        .error_message = "",
        .size_bytes = 8192,
        .from_tier = TierLevel::L2_EPISODIC,
        .to_tier = TierLevel::L3_SEMANTIC,
        .total_latency_ms = std::chrono::milliseconds(45),
        .correlation_id = "corr-789",
    };

    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.size_bytes, 8192);
    EXPECT_EQ(result.total_latency_ms.count(), 45);
    EXPECT_EQ(result.correlation_id, "corr-789");
}

// APD-04: Failed demotion result with error message
TEST_F(PromotionDemotionTest, APD_04_FailedDemotionResult) {
    DemotionResult result{
        .success = false,
        .error_message = "destination tier not available",
        .size_bytes = 0,
        .from_tier = TierLevel::STORAGE_WARM,
        .to_tier = TierLevel::STORAGE_COLD,
        .total_latency_ms = std::chrono::milliseconds(100),
        .correlation_id = "corr-999",
    };

    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error_message, "destination tier not available");
}

// ============================================================================
// § 3  Tests: Age-Based Policy
// ============================================================================

// APD-05: L1→L2 demotion decision
TEST_F(PromotionDemotionTest, APD_05_ShouldDemoteL1ToL2) {
    AgeBasedPolicy policy;
    policy.l1_zero_access_days = 1;

    // Not yet old enough
    EXPECT_FALSE(policy.shouldDemoteL1ToL2(43200));  // 12 hours

    // Old enough
    EXPECT_TRUE(policy.shouldDemoteL1ToL2(86401));  // > 24 hours
}

// APD-06: Hot→warm storage demotion decision
TEST_F(PromotionDemotionTest, APD_06_ShouldDemoteHotToWarm) {
    AgeBasedPolicy policy;
    policy.hot_zero_access_days = 14;
    policy.hot_to_warm_days = 30;

    // Too fresh
    EXPECT_FALSE(policy.shouldDemoteHotToWarm(604800, 604800));  // 7 days

    // Old by zero-access rule
    EXPECT_TRUE(policy.shouldDemoteHotToWarm(1209601, 0));  // > 14 days since access

    // Old by write time rule
    EXPECT_TRUE(policy.shouldDemoteHotToWarm(0, 2592001));  // > 30 days since write
}

// APD-07: Storage→cache promotion decision
TEST_F(PromotionDemotionTest, APD_07_ShouldPromoteStorageToCache) {
    AgeBasedPolicy policy;
    policy.storage_promotion_threshold = 3;

    EXPECT_FALSE(policy.shouldPromoteStorageToCache(1));
    EXPECT_FALSE(policy.shouldPromoteStorageToCache(2));
    EXPECT_TRUE(policy.shouldPromoteStorageToCache(3));
    EXPECT_TRUE(policy.shouldPromoteStorageToCache(5));
}

// APD-08: Policy description
TEST_F(PromotionDemotionTest, APD_08_PolicyDescription) {
    AgeBasedPolicy policy;
    policy.hot_to_warm_days = 30;
    policy.warm_to_cold_days = 90;

    auto desc = policy.describe();
    
    EXPECT_NE(desc.find("hot_to_warm_days"), std::string::npos);
    EXPECT_NE(desc.find("30"), std::string::npos);
    EXPECT_NE(desc.find("warm_to_cold_days"), std::string::npos);
    EXPECT_NE(desc.find("90"), std::string::npos);
}

}  // namespace access_model
}  // namespace themis
