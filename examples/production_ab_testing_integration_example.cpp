/**
 * @file example_production_ab_testing_integration.cpp
 * @brief Example: Integrate Production A/B Testing with ContinuousLearningOrchestrator
 *
 * This example demonstrates how to use the production A/B testing integration
 * components within the ThemisDB RAG system.
 */

#include <iostream>
#include <memory>
#include <thread>
#include <chrono>

#include "rag/ab_testing_framework.h"
#include "rag/ab_test_production_integration.h"
#include "rag/continuous_learning_orchestrator.h"
#include "rag/ml_learning_metrics_collector.h"
#include "core/concerns/metrics.h"

using namespace themis::rag::learning;

// Mock metrics for the example
class ExampleMetrics : public core::concerns::IMetrics {
public:
    void incrementCounter(const std::string& name, double value,
                         const IMetrics::Labels& labels) override {
        std::cout << "Counter: " << name << " += " << value << std::endl;
    }

    void setGauge(const std::string& name, double value,
                 const IMetrics::Labels& labels) override {
        std::cout << "Gauge: " << name << " = " << value << std::endl;
    }

    void recordHistogram(const std::string& name, double value,
                        const IMetrics::Labels& labels) override {
        std::cout << "Histogram: " << name << " <- " << value << std::endl;
    }
};

void example_basic_production_routing() {
    std::cout << "\n=== Example 1: Basic Production Traffic Routing ===\n" << std::endl;

    // Setup
    auto ab_framework = std::make_unique<ABTestingFramework>();
    auto metrics = std::make_shared<ExampleMetrics>();
    auto router = std::make_unique<ABTestProductionRouter>(ab_framework, metrics);

    // Configure A/B test
    ABTestConfig test_config;
    test_config.test_id = "retrieval_v2_trial";
    test_config.component = "retrieval_system";
    test_config.traffic_split = 0.1;  // 10% to treatment
    test_config.min_samples = 500;
    test_config.significance_level = 0.05;
    test_config.min_improvement = 0.02;

    ASSERT(ab_framework->startTest(test_config), "Failed to start test");
    std::cout << "✓ A/B test started: " << test_config.test_id << std::endl;

    // Simulate production traffic
    std::cout << "\nSimulating production traffic..." << std::endl;
    for (int i = 0; i < 100; i++) {
        std::string user_id = "user_" + std::to_string(i);
        
        // Define eligibility criteria for this user
        EligibilityCriteria criteria;
        criteria.user_region = (i % 3 == 0) ? "eu-west" : "us-east";
        criteria.customer_tier = (i % 2 == 0) ? "premium" : "standard";
        criteria.is_paid_customer = (i % 3) != 0;
        criteria.has_opted_in_to_experiments = true;
        criteria.earliest_eligible_time = std::chrono::system_clock::now() - std::chrono::hours(1);
        criteria.latest_eligible_time = std::chrono::system_clock::now() + std::chrono::hours(1);
        criteria.allowed_regions = {};  // No region filter
        
        // Route request to treatment or control
        bool use_treatment = router->selectTreatmentForRequest(
            test_config.test_id, user_id, criteria);

        // Simulate request execution
        double latency_ms = 25.0 + (i % 10) * 5.0;  // 25-95ms
        bool success = (i % 20) != 0;  // 95% success rate
        double relevance_score = 0.85 + (i % 10) * 0.01;  // 0.85-0.94 relevance

        // Record observation
        router->recordProductionObservation(
            test_config.test_id, user_id, use_treatment,
            latency_ms, success, relevance_score);

        if (i % 25 == 0) {
            auto stats = router->getRoutingStats(test_config.test_id);
            std::cout << "  [" << i << " requests] Eligible: " << stats.eligible_requests
                     << "/" << stats.total_requests
                     << " Treatment: " << stats.treatment_assignments << std::endl;
        }
    }

    // Final statistics
    auto final_stats = router->getRoutingStats(test_config.test_id);
    std::cout << "\nFinal Statistics:" << std::endl;
    std::cout << "  Total Requests: " << final_stats.total_requests << std::endl;
    std::cout << "  Eligible: " << final_stats.eligible_requests << std::endl;
    std::cout << "  Eligibility Rate: " << (final_stats.eligibility_rate * 100) << "%" << std::endl;
    std::cout << "  Treatment Assignments: " << final_stats.treatment_assignments << std::endl;
}

void example_promotion_decision() {
    std::cout << "\n=== Example 2: Metrics-Driven Promotion Decision ===\n" << std::endl;

    // Setup
    auto ab_framework = std::make_unique<ABTestingFramework>();
    auto metrics = std::make_shared<ExampleMetrics>();
    auto metrics_collector = MLLearningMetricsCollector::getInstance();
    auto promotion_engine = std::make_unique<ABTestPromotionEngine>(
        ab_framework, metrics_collector, metrics);

    // Configure and start test
    ABTestConfig test_config;
    test_config.test_id = "prompt_optimization_trial";
    test_config.component = "prompt_system";
    test_config.traffic_split = 0.05;  // 5% to treatment
    test_config.min_samples = 100;
    
    ASSERT(ab_framework->startTest(test_config), "Failed to start test");
    std::cout << "✓ Promotion test started: " << test_config.test_id << std::endl;

    // Simulate enough observations for promotion
    std::cout << "\nSimulating observations with significant improvement..." << std::endl;
    
    // Control: 70% success rate
    for (int i = 0; i < 150; i++) {
        ab_framework->recordObservation(test_config.test_id, false, i < 105, 0.88);
    }
    std::cout << "✓ Control observations recorded (70% success)" << std::endl;

    // Treatment: 85% success rate (15% improvement)
    for (int i = 0; i < 150; i++) {
        ab_framework->recordObservation(test_config.test_id, true, i < 127, 0.92);
    }
    std::cout << "✓ Treatment observations recorded (85% success)" << std::endl;

    // Evaluate promotion
    ABTestPromotionEngine::PromotionConfig config;
    config.min_samples_for_decision = 100;
    config.min_improvement_threshold = 0.02;  // 2% required
    config.significance_level = 0.05;
    config.min_test_duration = std::chrono::hours(1);

    std::cout << "\nEvaluating promotion decision..." << std::endl;
    auto decision = promotion_engine->evaluatePromotion(test_config.test_id, config);

    std::cout << "Decision: ";
    switch (decision.decision) {
        case ABTestPromotionEngine::DecisionType::PROMOTE:
            std::cout << "PROMOTE ✓\n";
            break;
        case ABTestPromotionEngine::DecisionType::ROLLBACK:
            std::cout << "ROLLBACK\n";
            break;
        case ABTestPromotionEngine::DecisionType::CONTINUE_TEST:
            std::cout << "CONTINUE_TEST\n";
            break;
        case ABTestPromotionEngine::DecisionType::ERROR:
            std::cout << "ERROR\n";
            break;
    }

    std::cout << "  Treatment Improvement: " << (decision.treatment_improvement * 100) << "%" << std::endl;
    std::cout << "  P-value: " << decision.p_value << std::endl;
    std::cout << "  Samples (Control/Treatment): " << decision.control_samples << "/" 
              << decision.treatment_samples << std::endl;
    std::cout << "  Reason: " << decision.reason << std::endl;

    // Check history
    auto history = promotion_engine->getPromotionHistory(test_config.test_id);
    std::cout << "\nPromotion History (" << history.size() << " decisions):" << std::endl;
    for (const auto& h : history) {
        std::cout << "  - Decision " << (int)h.decision 
                 << ": " << h.reason << std::endl;
    }
}

void example_automatic_rollback() {
    std::cout << "\n=== Example 3: Automatic Rollback Management ===\n" << std::endl;

    // Setup
    auto ab_framework = std::make_unique<ABTestingFramework>();
    auto metrics = std::make_shared<ExampleMetrics>();
    auto metrics_collector = MLLearningMetricsCollector::getInstance();
    auto promotion_engine = std::make_unique<ABTestPromotionEngine>(
        ab_framework, metrics_collector, metrics);
    auto rollback_automator = std::make_unique<ABTestRollbackAutomator>(
        promotion_engine, metrics);

    // Start a test
    ABTestConfig test_config;
    test_config.test_id = "llm_adapter_v3_trial";
    ab_framework->startTest(test_config);
    
    std::cout << "✓ Test started: " << test_config.test_id << std::endl;

    // Simulate rollback scenarios
    std::cout << "\nScenario 1: Manual Rollback (Operator Decision)" << std::endl;
    bool success = rollback_automator->triggerRollback(
        test_config.test_id,
        "Customer impact detected: response quality degradation");
    std::cout << "  Rollback triggered: " << (success ? "SUCCESS" : "FAILED") << std::endl;

    std::cout << "\nScenario 2: Error Detection Rollback" << std::endl;
    success = rollback_automator->triggerRollback(
        test_config.test_id,
        "Error rate spike: 15% errors in last 5 minutes");
    std::cout << "  Rollback triggered: " << (success ? "SUCCESS" : "FAILED") << std::endl;

    // Display rollback history
    auto history = rollback_automator->getRollbackHistory();
    std::cout << "\nRollback History (" << history.size() << " events):" << std::endl;
    for (const auto& event : history) {
        std::cout << "  - Test: " << event.test_id << std::endl;
        std::cout << "    Reason: " << event.reason << std::endl;
        std::cout << "    Automatic: " << (event.was_automatic ? "Yes" : "No") << std::endl;
    }
}

void example_full_workflow() {
    std::cout << "\n=== Example 4: Full Production Workflow ===\n" << std::endl;

    // Setup complete system
    auto ab_framework = std::make_unique<ABTestingFramework>();
    auto metrics = std::make_shared<ExampleMetrics>();
    auto metrics_collector = MLLearningMetricsCollector::getInstance();
    
    auto router = std::make_unique<ABTestProductionRouter>(ab_framework, metrics);
    auto promotion_engine = std::make_unique<ABTestPromotionEngine>(
        ab_framework, metrics_collector, metrics);
    auto rollback_automator = std::make_unique<ABTestRollbackAutomator>(
        promotion_engine, metrics);

    // Define test
    ABTestConfig test_config;
    test_config.test_id = "full_pipeline_test";
    test_config.traffic_split = 0.2;  // 20% to treatment
    ab_framework->startTest(test_config);

    std::cout << "=== Phase 1: Traffic Collection ===" << std::endl;
    
    // Simulate 200 requests
    for (int i = 0; i < 200; i++) {
        std::string user_id = "user_" + std::to_string(i);
        
        EligibilityCriteria criteria;
        criteria.user_region = "us-east";
        criteria.has_opted_in_to_experiments = true;
        criteria.earliest_eligible_time = std::chrono::system_clock::now() - std::chrono::hours(1);
        criteria.latest_eligible_time = std::chrono::system_clock::now() + std::chrono::hours(1);

        bool use_treatment = router->selectTreatmentForRequest(
            test_config.test_id, user_id, criteria);

        // Simulate better performance for treatment
        bool success = use_treatment ? (i % 20 != 0) : (i % 25 != 0);
        double latency = use_treatment ? 20.0 : 25.0;  // Treatment is faster

        router->recordProductionObservation(
            test_config.test_id, user_id, use_treatment,
            latency, success, 0.9);
    }

    auto stats = router->getRoutingStats(test_config.test_id);
    std::cout << "✓ Collected " << stats.total_requests << " requests" << std::endl;
    std::cout << "  Treatment: " << stats.treatment_assignments 
             << " Control: " << stats.control_assignments << std::endl;

    std::cout << "\n=== Phase 2: Promotion Decision ===" << std::endl;

    ABTestPromotionEngine::PromotionConfig promo_config;
    promo_config.min_samples_for_decision = 30;
    promo_config.min_improvement_threshold = 0.02;

    auto decision = promotion_engine->evaluatePromotion(
        test_config.test_id, promo_config);

    std::cout << "Decision: ";
    if (decision.decision == ABTestPromotionEngine::DecisionType::PROMOTE) {
        std::cout << "PROMOTE treatment to production ✓" << std::endl;
    } else if (decision.decision == ABTestPromotionEngine::DecisionType::CONTINUE_TEST) {
        std::cout << "CONTINUE collecting data" << std::endl;
    } else {
        std::cout << "ROLLBACK treatment" << std::endl;
    }

    std::cout << "\n=== Phase 3: Monitoring & Safeguards ===" << std::endl;

    // Check if rollback needed
    ABTestRollbackAutomator::RollbackConfig rollback_config;
    rollback_config.error_rate_threshold = 0.10;

    if (rollback_automator->shouldTriggerRollback(test_config.test_id, rollback_config)) {
        std::cout << "⚠ SLO violation detected, triggering rollback" << std::endl;
        rollback_automator->triggerRollback(test_config.test_id, "SLO breach");
    } else {
        std::cout << "✓ No SLO violations detected" << std::endl;
    }

    std::cout << "\n=== Summary ===" << std::endl;
    std::cout << "Test: " << test_config.test_id << std::endl;
    std::cout << "Status: " << (decision.decision == ABTestPromotionEngine::DecisionType::PROMOTE
                               ? "Ready for Promotion" : "Monitoring") << std::endl;
}

}  // namespace

int main() {
    std::cout << "ThemisDB Production A/B Testing Integration Examples\n" << std::endl;
    std::cout << "====================================================\n" << std::endl;

    try {
        example_basic_production_routing();
        example_promotion_decision();
        example_automatic_rollback();
        example_full_workflow();

        std::cout << "\n✅ All examples completed successfully!\n" << std::endl;
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ Error: " << e.what() << std::endl;
        return 1;
    }
}
