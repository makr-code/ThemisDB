/**
 * @file test_ab_test_production_integration.cpp
 * @brief Unit tests for production A/B testing integration
 */

#include <gtest/gtest.h>
#include <memory>
#include <thread>

#include "rag/ab_test_production_integration.h"
#include "rag/ab_testing_framework.h"
#include "core/concerns/metrics.h"

using namespace themis::rag::learning;

// Mock metrics implementation for testing
class MockMetrics : public core::concerns::IMetrics {
public:
    void incrementCounter(const std::string& name, double value,
                         const IMetrics::Labels& labels) override {
        counters_[name] = value;
    }

    void setGauge(const std::string& name, double value,
                 const IMetrics::Labels& labels) override {
        gauges_[name] = value;
    }

    void recordHistogram(const std::string& name, double value,
                        const IMetrics::Labels& labels) override {
        histograms_[name].push_back(value);
    }

    double getCounterValue(const std::string& name) const {
        auto it = counters_.find(name);
        return it != counters_.end() ? it->second : 0.0;
    }

    double getGaugeValue(const std::string& name) const {
        auto it = gauges_.find(name);
        return it != gauges_.end() ? it->second : 0.0;
    }

private:
    std::unordered_map<std::string, double> counters_;
    std::unordered_map<std::string, double> gauges_;
    std::unordered_map<std::string, std::vector<double>> histograms_;
};

class ABTestProductionRouterTest : public ::testing::Test {
protected:
    void SetUp() override {
        ab_framework_ = std::make_unique<ABTestingFramework>();
        metrics_ = std::make_shared<MockMetrics>();
        router_ = std::make_unique<ABTestProductionRouter>(ab_framework_, metrics_);

        // Setup default test config
        test_config_.test_id = "test_treatment_v2";
        test_config_.component = "LoRA";
        test_config_.traffic_split = 0.1;
        test_config_.min_samples = 100;
        test_config_.significance_level = 0.05;
        test_config_.min_improvement = 0.02;

        // Start test
        ASSERT_TRUE(ab_framework_->startTest(test_config_));
    }

    std::unique_ptr<ABTestingFramework> ab_framework_;
    std::shared_ptr<MockMetrics> metrics_;
    std::unique_ptr<ABTestProductionRouter> router_;
    ABTestConfig test_config_;
};

// ============================================================================
// ABTestProductionRouter Tests
// ============================================================================

TEST_F(ABTestProductionRouterTest, EligibleUserGetsRooted) {
    EligibilityCriteria criteria;
    criteria.user_region = "us-east";
    criteria.has_opted_in_to_experiments = true;
    criteria.earliest_eligible_time = std::chrono::system_clock::now() -
                                      std::chrono::hours(1);
    criteria.latest_eligible_time = std::chrono::system_clock::now() +
                                    std::chrono::hours(1);

    bool use_treatment = router_->selectTreatmentForRequest(
        test_config_.test_id, "user_123", criteria);

    auto stats = router_->getRoutingStats(test_config_.test_id);
    EXPECT_EQ(stats.total_requests, 1);
    EXPECT_EQ(stats.eligible_requests, 1);
    EXPECT_EQ(stats.ineligible_requests, 0);
}

TEST_F(ABTestProductionRouterTest, IneligibleUserGetsControl) {
    EligibilityCriteria criteria;
    criteria.has_opted_in_to_experiments = false;  // Not opted in

    bool use_treatment = router_->selectTreatmentForRequest(
        test_config_.test_id, "user_123", criteria);

    EXPECT_FALSE(use_treatment);  // Should always get control

    auto stats = router_->getRoutingStats(test_config_.test_id);
    EXPECT_EQ(stats.ineligible_requests, 1);
}

TEST_F(ABTestProductionRouterTest, TrafficSplitRespected) {
    EligibilityCriteria criteria;
    criteria.user_region = "us-east";
    criteria.has_opted_in_to_experiments = true;
    criteria.earliest_eligible_time = std::chrono::system_clock::now() -
                                      std::chrono::hours(1);
    criteria.latest_eligible_time = std::chrono::system_clock::now() +
                                    std::chrono::hours(1);

    int treatment_count = 0;
    const int total_users = 1000;

    for (int i = 0; i < total_users; i++) {
        std::string user_id = "user_" + std::to_string(i);
        bool use_treatment = router_->selectTreatmentForRequest(
            test_config_.test_id, user_id, criteria);
        if (use_treatment) {
            treatment_count++;
        }
    }

    double actual_split = static_cast<double>(treatment_count) / total_users;
    // Should be close to 10%
    EXPECT_NEAR(actual_split, test_config_.traffic_split, 0.05);
}

TEST_F(ABTestProductionRouterTest, RecordProductionObservation) {
    EligibilityCriteria criteria;
    criteria.user_region = "us-east";
    criteria.has_opted_in_to_experiments = true;
    criteria.earliest_eligible_time = std::chrono::system_clock::now() -
                                      std::chrono::hours(1);
    criteria.latest_eligible_time = std::chrono::system_clock::now() +
                                    std::chrono::hours(1);

    bool was_treatment = router_->selectTreatmentForRequest(
        test_config_.test_id, "user_123", criteria);

    // Record observations
    router_->recordProductionObservation(
        test_config_.test_id, "user_123", was_treatment,
        25.5,  // latency ms
        true,  // success
        0.95   // custom metric
    );

    // Verify metrics were recorded
    EXPECT_GT(
        static_cast<MockMetrics*>(metrics_.get())->getCounterValue(
            "themisdb_ab_test_observations_total"),
        0.0);
}

TEST_F(ABTestProductionRouterTest, RegionFiltering) {
    EligibilityCriteria criteria;
    criteria.user_region = "eu-west";
    criteria.has_opted_in_to_experiments = true;
    criteria.allowed_regions = {"us-east", "us-west"};  // EU not allowed
    criteria.earliest_eligible_time = std::chrono::system_clock::now() -
                                      std::chrono::hours(1);
    criteria.latest_eligible_time = std::chrono::system_clock::now() +
                                    std::chrono::hours(1);

    bool use_treatment = router_->selectTreatmentForRequest(
        test_config_.test_id, "user_456", criteria);

    EXPECT_FALSE(use_treatment);  // Should get control due to region filter
}

TEST_F(ABTestProductionRouterTest, TemporalFiltering) {
    EligibilityCriteria criteria;
    criteria.user_region = "us-east";
    criteria.has_opted_in_to_experiments = true;
    criteria.earliest_eligible_time = std::chrono::system_clock::now() +
                                      std::chrono::hours(1);  // In future
    criteria.latest_eligible_time = std::chrono::system_clock::now() +
                                    std::chrono::hours(2);

    bool use_treatment = router_->selectTreatmentForRequest(
        test_config_.test_id, "user_789", criteria);

    EXPECT_FALSE(use_treatment);  // Should get control due to temporal window
}

// ============================================================================
// ABTestPromotionEngine Tests
// ============================================================================

class ABTestPromotionEngineTest : public ::testing::Test {
protected:
    void SetUp() override {
        ab_framework_ = std::make_unique<ABTestingFramework>();
        metrics_ = std::make_shared<MockMetrics>();
        metrics_collector_ = MLLearningMetricsCollector::getInstance();
        
        promotion_engine_ = std::make_unique<ABTestPromotionEngine>(
            ab_framework_, metrics_collector_, metrics_);

        test_config_.test_id = "test_promotion";
        test_config_.component = "LoRA";
        test_config_.traffic_split = 0.1;
        test_config_.min_samples = 30;
        test_config_.significance_level = 0.05;
        test_config_.min_improvement = 0.02;

        ASSERT_TRUE(ab_framework_->startTest(test_config_));
    }

    std::unique_ptr<ABTestingFramework> ab_framework_;
    std::shared_ptr<MockMetrics> metrics_;
    std::shared_ptr<MLLearningMetricsCollector> metrics_collector_;
    std::unique_ptr<ABTestPromotionEngine> promotion_engine_;
    ABTestConfig test_config_;
};

TEST_F(ABTestPromotionEngineTest, InsufficientSamplesReturnsContinue) {
    ABTestPromotionEngine::PromotionConfig config;
    config.min_samples_for_decision = 1000;

    // Record only a few observations
    for (int i = 0; i < 10; i++) {
        ab_framework_->recordObservation(test_config_.test_id, false, true, 0.9);
        ab_framework_->recordObservation(test_config_.test_id, true, true, 0.91);
    }

    auto decision = promotion_engine_->evaluatePromotion(
        test_config_.test_id, config);

    EXPECT_EQ(decision.decision,
              ABTestPromotionEngine::DecisionType::CONTINUE_TEST);
}

TEST_F(ABTestPromotionEngineTest, SmallImprovementReturnsContinue) {
    ABTestPromotionEngine::PromotionConfig config;
    config.min_samples_for_decision = 30;
    config.min_improvement_threshold = 0.10;  // 10% required

    // Record observations with small difference (1%)
    for (int i = 0; i < 100; i++) {
        ab_framework_->recordObservation(test_config_.test_id, false,
                                        (i < 70));  // 70% success
        ab_framework_->recordObservation(test_config_.test_id, true,
                                        (i < 71));  // 71% success (1% improvement)
    }

    auto decision = promotion_engine_->evaluatePromotion(
        test_config_.test_id, config);

    EXPECT_EQ(decision.decision,
              ABTestPromotionEngine::DecisionType::CONTINUE_TEST);
}

TEST_F(ABTestPromotionEngineTest, SignificantImprovementPromotes) {
    ABTestPromotionEngine::PromotionConfig config;
    config.min_samples_for_decision = 30;
    config.min_improvement_threshold = 0.02;  // 2% required

    // Record observations with significant difference (20%)
    for (int i = 0; i < 150; i++) {
        ab_framework_->recordObservation(test_config_.test_id, false,
                                        (i < 50));  // 50% success
        ab_framework_->recordObservation(test_config_.test_id, true,
                                        (i < 70));  // 70% success (20% improvement)
    }

    auto decision = promotion_engine_->evaluatePromotion(
        test_config_.test_id, config);

    EXPECT_EQ(decision.decision,
              ABTestPromotionEngine::DecisionType::PROMOTE);
}

TEST_F(ABTestPromotionEngineTest, PromotionHistoryTracked) {
    ABTestPromotionEngine::PromotionConfig config;
    config.min_samples_for_decision = 30;

    // Generate significant improvement
    for (int i = 0; i < 100; i++) {
        ab_framework_->recordObservation(test_config_.test_id, false,
                                        (i < 50));
        ab_framework_->recordObservation(test_config_.test_id, true,
                                        (i < 70));
    }

    auto decision1 = promotion_engine_->evaluatePromotion(
        test_config_.test_id, config);

    auto history = promotion_engine_->getPromotionHistory(
        test_config_.test_id);

    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].decision, decision1.decision);
}

// ============================================================================
// ABTestRollbackAutomator Tests
// ============================================================================

class ABTestRollbackAutomatorTest : public ::testing::Test {
protected:
    void SetUp() override {
        ab_framework_ = std::make_unique<ABTestingFramework>();
        metrics_ = std::make_shared<MockMetrics>();
        metrics_collector_ = MLLearningMetricsCollector::getInstance();
        
        promotion_engine_ = std::make_unique<ABTestPromotionEngine>(
            ab_framework_, metrics_collector_, metrics_);
        
        rollback_automator_ = std::make_unique<ABTestRollbackAutomator>(
            promotion_engine_, metrics_);

        test_config_.test_id = "test_rollback";
        test_config_.component = "LoRA";
        test_config_.traffic_split = 0.1;
    }

    std::unique_ptr<ABTestingFramework> ab_framework_;
    std::shared_ptr<MockMetrics> metrics_;
    std::shared_ptr<MLLearningMetricsCollector> metrics_collector_;
    std::unique_ptr<ABTestPromotionEngine> promotion_engine_;
    std::unique_ptr<ABTestRollbackAutomator> rollback_automator_;
    ABTestConfig test_config_;
};

TEST_F(ABTestRollbackAutomatorTest, ManualRollbackSucceeds) {
    bool success = rollback_automator_->triggerRollback(
        test_config_.test_id,
        "Manual test rollback");

    EXPECT_TRUE(success);

    auto history = rollback_automator_->getRollbackHistory();
    EXPECT_EQ(history.size(), 1);
    EXPECT_EQ(history[0].test_id, test_config_.test_id);
    EXPECT_EQ(history[0].reason, "Manual test rollback");
    EXPECT_FALSE(history[0].was_automatic);
}

TEST_F(ABTestRollbackAutomatorTest, MultipleRollbacksTracked) {
    rollback_automator_->triggerRollback(test_config_.test_id, "Reason 1");
    rollback_automator_->triggerRollback(test_config_.test_id, "Reason 2");

    auto history = rollback_automator_->getRollbackHistory();
    EXPECT_EQ(history.size(), 2);
}

}  // namespace
