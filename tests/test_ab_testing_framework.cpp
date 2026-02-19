/**
 * @file test_ab_testing_framework.cpp
 * @brief Unit tests for A/B Testing Framework
 */

#include <gtest/gtest.h>

#include "rag/ab_testing_framework.h"

using namespace themis::rag::learning;

class ABTestingFrameworkTest : public ::testing::Test {
  protected:
    void SetUp() override {
        framework_ = std::make_unique<ABTestingFramework>();

        // Setup default test configuration
        test_config_.test_id            = "test_model_v2";
        test_config_.component          = "lora_adapter";
        test_config_.traffic_split      = 0.1;
        test_config_.min_samples        = 30;
        test_config_.significance_level = 0.05;
        test_config_.min_improvement    = 0.02;
    }

    std::unique_ptr<ABTestingFramework> framework_;
    ABTestConfig test_config_;
};

TEST_F(ABTestingFrameworkTest, StartTest) {
    bool success = framework_->startTest(test_config_);
    EXPECT_TRUE(success);

    // Starting same test again should fail
    bool duplicate = framework_->startTest(test_config_);
    EXPECT_FALSE(duplicate);
}

TEST_F(ABTestingFrameworkTest, GetActiveTests) {
    EXPECT_TRUE(framework_->getActiveTests().empty());

    framework_->startTest(test_config_);
    auto active = framework_->getActiveTests();

    EXPECT_EQ(active.size(), 1);
    EXPECT_EQ(active[0], test_config_.test_id);
}

TEST_F(ABTestingFrameworkTest, RecordObservations) {
    framework_->startTest(test_config_);

    // Record observations for control group
    for (int i = 0; i < 50; i++) {
        bool success = (i % 2 == 0); // 50% success rate
        framework_->recordObservation(test_config_.test_id, false, success);
    }

    // Record observations for treatment group
    for (int i = 0; i < 50; i++) {
        bool success = (i < 40); // 80% success rate
        framework_->recordObservation(test_config_.test_id, true, success);
    }

    // Evaluate test
    auto result = framework_->evaluateTest(test_config_.test_id);

    EXPECT_EQ(result.sample_size_control, 50);
    EXPECT_EQ(result.sample_size_treatment, 50);
    EXPECT_NEAR(result.control_success_rate, 0.5, 0.01);
    EXPECT_NEAR(result.treatment_success_rate, 0.8, 0.01);
    EXPECT_NEAR(result.improvement, 0.3, 0.01);
}

TEST_F(ABTestingFrameworkTest, TrafficSplitting) {
    framework_->startTest(test_config_);

    // Test consistent assignment for same user
    std::string user_id = "user_123";
    bool first          = framework_->shouldUseTreatment(test_config_.test_id, user_id);
    bool second         = framework_->shouldUseTreatment(test_config_.test_id, user_id);

    EXPECT_EQ(first, second); // Same user should get same assignment

    // Test traffic split ratio
    int treatment_count = 0;
    int total_users     = 1000;

    for (int i = 0; i < total_users; i++) {
        std::string uid = "user_" + std::to_string(i);
        if (framework_->shouldUseTreatment(test_config_.test_id, uid)) {
            treatment_count++;
        }
    }

    double actual_split = static_cast<double>(treatment_count) / total_users;
    // Should be close to configured split (0.1 = 10%)
    EXPECT_NEAR(actual_split, test_config_.traffic_split, 0.05);
}

TEST_F(ABTestingFrameworkTest, StatisticalSignificance) {
    framework_->startTest(test_config_);

    // Small difference, should not be significant
    for (int i = 0; i < 100; i++) {
        framework_->recordObservation(test_config_.test_id, false, (i < 70)); // 70%
        framework_->recordObservation(test_config_.test_id, true, (i < 72));  // 72%
    }

    auto result = framework_->evaluateTest(test_config_.test_id);
    EXPECT_FALSE(result.is_significant); // Difference too small

    // Large difference with enough samples, should be significant
    ABTestConfig test2;
    test2.test_id     = "test_significant";
    test2.component   = "test";
    test2.min_samples = 30;

    framework_->startTest(test2);

    for (int i = 0; i < 100; i++) {
        framework_->recordObservation(test2.test_id, false, (i < 50)); // 50%
        framework_->recordObservation(test2.test_id, true, (i < 90));  // 90%
    }

    auto result2 = framework_->evaluateTest(test2.test_id);
    EXPECT_TRUE(result2.is_significant); // Large difference
    EXPECT_LT(result2.p_value, 0.05);
}

TEST_F(ABTestingFrameworkTest, CompleteTest) {
    framework_->startTest(test_config_);

    EXPECT_EQ(framework_->getTestStatus(test_config_.test_id), ABTestStatus::ACTIVE);

    // Promote test
    framework_->completeTest(test_config_.test_id, true);
    EXPECT_EQ(framework_->getTestStatus(test_config_.test_id), ABTestStatus::PROMOTED);

    // Start another test and roll it back
    ABTestConfig test2;
    test2.test_id   = "test_rollback";
    test2.component = "test";
    framework_->startTest(test2);

    framework_->completeTest(test2.test_id, false);
    EXPECT_EQ(framework_->getTestStatus(test2.test_id), ABTestStatus::ROLLED_BACK);
}

TEST_F(ABTestingFrameworkTest, CancelTest) {
    framework_->startTest(test_config_);

    EXPECT_EQ(framework_->getTestStatus(test_config_.test_id), ABTestStatus::ACTIVE);

    framework_->cancelTest(test_config_.test_id);
    EXPECT_EQ(framework_->getTestStatus(test_config_.test_id), ABTestStatus::CANCELLED);
}

TEST_F(ABTestingFrameworkTest, MultipleTests) {
    ABTestConfig test1;
    test1.test_id   = "test1";
    test1.component = "lora";

    ABTestConfig test2;
    test2.test_id   = "test2";
    test2.component = "prompt";

    ABTestConfig test3;
    test3.test_id   = "test3";
    test3.component = "retrieval";

    framework_->startTest(test1);
    framework_->startTest(test2);
    framework_->startTest(test3);

    auto active = framework_->getActiveTests();
    EXPECT_EQ(active.size(), 3);

    // Complete one test
    framework_->completeTest(test2.test_id, true);
    active = framework_->getActiveTests();
    EXPECT_EQ(active.size(), 2);
}

TEST_F(ABTestingFrameworkTest, EdgeCaseZeroSamples) {
    framework_->startTest(test_config_);

    // Evaluate with no observations
    auto result = framework_->evaluateTest(test_config_.test_id);

    EXPECT_EQ(result.sample_size_control, 0);
    EXPECT_EQ(result.sample_size_treatment, 0);
    EXPECT_FALSE(result.is_significant);
}
