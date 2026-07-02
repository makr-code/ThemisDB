/**
 * @file ab_test_production_integration.h
 * @brief Production A/B Testing Integration with Live Traffic and Metrics
 *
 * Connects A/B testing framework to real production traffic, metrics collection,
 * and automated promotion/rollback decisions.
 */

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "ab_testing_framework.h"
#include "ml_learning_metrics_collector.h"
#include "core/concerns/metrics.h"

namespace themis::rag::learning {

/**
 * @brief Real-time eligibility criteria for A/B test participant
 *
 * Determines whether a user/request is eligible for A/B test participation
 * based on production criteria (region, user tier, customer status, etc.)
 */
struct EligibilityCriteria {
    // User/Request attributes
    std::string user_region;
    std::string customer_tier;  // free, standard, premium, enterprise
    bool is_paid_customer = false;
    bool has_opted_in_to_experiments = true;
    
    // Temporal constraints
    std::chrono::system_clock::time_point earliest_eligible_time;
    std::chrono::system_clock::time_point latest_eligible_time;
    
    // Performance constraints
    double max_acceptable_latency_ms = 5000.0;  // Reject if latency exceeds this
    bool requires_cache_hit = false;
    
    // Customization
    std::vector<std::string> allowed_regions;
    std::vector<std::string> blocked_regions;
};

/**
 * @brief Live traffic routing decision for A/B tests
 *
 * Determines which variant (control/treatment) should be served for a request,
 * with eligibility gating and fallback to control on any safety concern.
 */
class ABTestProductionRouter {
public:
    ABTestProductionRouter(
        std::shared_ptr<ABTestingFramework> ab_framework,
        std::shared_ptr<core::concerns::IMetrics> metrics);
    ~ABTestProductionRouter();

    /**
     * @brief Determine which variant to serve for a request
     *
     * Applies eligibility gating, traffic splitting, and safety checks.
     * Returns treatment if all checks pass, otherwise returns control.
     *
     * @param test_id A/B test identifier
     * @param user_id User/session identifier for consistent assignment
     * @param criteria Eligibility criteria for this request
     * @return true if treatment should be used, false for control
     */
    bool selectTreatmentForRequest(
        const std::string& test_id,
        const std::string& user_id,
        const EligibilityCriteria& criteria);

    /**
     * @brief Record observation from production traffic
     *
     * @param test_id A/B test identifier
     * @param user_id User/session identifier
     * @param was_treatment true if user received treatment
     * @param latency_ms Request latency in milliseconds
     * @param success true if request succeeded
     * @param custom_metric Optional business metric value (e.g., score)
     */
    void recordProductionObservation(
        const std::string& test_id,
        const std::string& user_id,
        bool was_treatment,
        double latency_ms,
        bool success,
        double custom_metric = 0.0);

    /**
     * @brief Get routing statistics for a test
     */
    struct RoutingStats {
        size_t total_requests = 0;
        size_t eligible_requests = 0;
        size_t ineligible_requests = 0;
        size_t treatment_assignments = 0;
        size_t control_assignments = 0;
        double eligibility_rate = 0.0;
    };

    RoutingStats getRoutingStats(const std::string& test_id) const;

private:
    bool isEligible(const EligibilityCriteria& criteria);
    
    std::shared_ptr<ABTestingFramework> ab_framework_;
    std::shared_ptr<core::concerns::IMetrics> metrics_;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Automatic promotion/rollback decisions based on production metrics
 *
 * Monitors A/B test results and automatically promotes treatment or
 * initiates rollback based on statistical significance and SLO compliance.
 */
class ABTestPromotionEngine {
public:
    ABTestPromotionEngine(
        std::shared_ptr<ABTestingFramework> ab_framework,
        std::shared_ptr<MLLearningMetricsCollector> metrics_collector,
        std::shared_ptr<core::concerns::IMetrics> prometheus_metrics);
    ~ABTestPromotionEngine();

    /**
     * @brief Configuration for promotion/rollback decisions
     */
    struct PromotionConfig {
        // Statistical criteria
        double min_improvement_threshold = 0.02;  // 2% improvement
        double significance_level = 0.05;  // p-value threshold
        size_t min_samples_for_decision = 1000;
        
        // SLO criteria (latency/error rate)
        double max_latency_percentile_99 = 100.0;  // milliseconds
        double max_error_rate = 0.01;  // 1% error rate
        
        // Safety criteria
        bool require_manual_approval = false;  // Require human approval before promotion
        std::chrono::hours min_test_duration{24};  // Run test for at least 24 hours
        std::chrono::hours max_test_duration{72};  // Stop test after 72 hours
    };

    /**
     * @brief Evaluate A/B test for promotion/rollback
     *
     * Returns decision based on production metrics and configured thresholds.
     *
     * @param test_id A/B test identifier
     * @param config Promotion configuration
     * @return Decision: PROMOTE, ROLLBACK, or CONTINUE_TEST
     */
    enum class DecisionType {
        PROMOTE,         // Treatment is significantly better, ready to deploy
        ROLLBACK,        // Treatment is worse or violates SLO, should rollback
        CONTINUE_TEST,   // Need more data or inconclusive results
        ERROR            // Insufficient data or test not found
    };

    struct PromotionDecision {
        DecisionType decision;
        double treatment_improvement = 0.0;  // Percentage improvement of treatment vs control
        double p_value = 1.0;  // Statistical significance
        size_t treatment_samples = 0;
        size_t control_samples = 0;
        std::string reason;  // Explanation of decision
        std::chrono::system_clock::time_point decision_time;
    };

    PromotionDecision evaluatePromotion(
        const std::string& test_id,
        const PromotionConfig& config);

    /**
     * @brief Get history of promotion decisions
     */
    std::vector<PromotionDecision> getPromotionHistory(const std::string& test_id) const;

private:
    std::shared_ptr<ABTestingFramework> ab_framework_;
    std::shared_ptr<MLLearningMetricsCollector> metrics_collector_;
    std::shared_ptr<core::concerns::IMetrics> prometheus_metrics_;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

/**
 * @brief Automatic rollback on SLO violations
 *
 * Monitors production metrics and automatically triggers rollback if
 * performance degradation is detected (latency, error rate, user satisfaction).
 */
class ABTestRollbackAutomator {
public:
    ABTestRollbackAutomator(
        std::shared_ptr<ABTestPromotionEngine> promotion_engine,
        std::shared_ptr<core::concerns::IMetrics> metrics);
    ~ABTestRollbackAutomator();

    /**
     * @brief Configuration for automatic rollback
     */
    struct RollbackConfig {
        // SLO thresholds
        double error_rate_threshold = 0.05;  // Rollback if error rate > 5%
        double latency_increase_threshold = 0.20;  // Rollback if latency increases > 20%
        
        // Time windows
        std::chrono::seconds evaluation_window{60};  // Evaluate last 60 seconds
        std::chrono::seconds burn_rate_window{300};  // Use 5-min window for burn-rate calc
        
        // Guardrails
        double error_rate_burn_rate = 10.0;  // Rollback if error rate burns budget 10x faster
        bool require_manual_approval = false;
        
        std::string rollback_reason_prefix = "Automatic SLO violation";
    };

    /**
     * @brief Check if rollback should be triggered
     *
     * Monitors current performance and determines if rollback is necessary.
     *
     * @param test_id A/B test identifier
     * @param config Rollback configuration
     * @return true if rollback should be triggered
     */
    bool shouldTriggerRollback(
        const std::string& test_id,
        const RollbackConfig& config);

    /**
     * @brief Manually trigger rollback
     *
     * @param test_id A/B test identifier
     * @param reason Human-readable reason for rollback
     * @return true if rollback was successful
     */
    bool triggerRollback(
        const std::string& test_id,
        const std::string& reason);

    /**
     * @brief Get rollback history
     */
    struct RollbackEvent {
        std::string test_id;
        std::chrono::system_clock::time_point timestamp;
        std::string reason;
        bool was_automatic;
    };

    std::vector<RollbackEvent> getRollbackHistory() const;

private:
    std::shared_ptr<ABTestPromotionEngine> promotion_engine_;
    std::shared_ptr<core::concerns::IMetrics> metrics_;
    
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace themis::rag::learning
