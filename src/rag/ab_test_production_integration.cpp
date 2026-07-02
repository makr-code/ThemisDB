/**
 * @file ab_test_production_integration.cpp
 * @brief Production A/B Testing Integration Implementation
 */

#include "rag/ab_test_production_integration.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <spdlog/spdlog.h>

namespace themis::rag::learning {

// ============================================================================
// ABTestProductionRouter Implementation
// ============================================================================

struct ABTestProductionRouter::Impl {
    std::unordered_map<std::string, RoutingStats> routing_stats;
    mutable std::mutex stats_mutex;
};

ABTestProductionRouter::ABTestProductionRouter(
    std::shared_ptr<ABTestingFramework> ab_framework,
    std::shared_ptr<core::concerns::IMetrics> metrics)
    : ab_framework_(ab_framework), metrics_(metrics),
      impl_(std::make_unique<Impl>()) {}

ABTestProductionRouter::~ABTestProductionRouter() = default;

bool ABTestProductionRouter::isEligible(const EligibilityCriteria& criteria) {
    // Check temporal constraints
    auto now = std::chrono::system_clock::now();
    if (now < criteria.earliest_eligible_time || now > criteria.latest_eligible_time) {
        return false;
    }

    // Check region
    if (!criteria.allowed_regions.empty()) {
        auto it = std::find(criteria.allowed_regions.begin(),
                           criteria.allowed_regions.end(),
                           criteria.user_region);
        if (it == criteria.allowed_regions.end()) {
            return false;
        }
    }

    if (!criteria.blocked_regions.empty()) {
        auto it = std::find(criteria.blocked_regions.begin(),
                           criteria.blocked_regions.end(),
                           criteria.user_region);
        if (it != criteria.blocked_regions.end()) {
            return false;
        }
    }

    // Check customer status
    if (!criteria.has_opted_in_to_experiments) {
        return false;
    }

    return true;
}

bool ABTestProductionRouter::selectTreatmentForRequest(
    const std::string& test_id,
    const std::string& user_id,
    const EligibilityCriteria& criteria) {
    
    // Update stats
    {
        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
        auto& stats = impl_->routing_stats[test_id];
        stats.total_requests++;
        
        if (isEligible(criteria)) {
            stats.eligible_requests++;
        } else {
            stats.ineligible_requests++;
            return false;  // Ineligible users always get control
        }
    }

    // Apply traffic split
    bool use_treatment = ab_framework_->shouldUseTreatment(test_id, user_id);
    
    // Update assignment stats
    {
        std::lock_guard<std::mutex> lock(impl_->stats_mutex);
        auto& stats = impl_->routing_stats[test_id];
        if (use_treatment) {
            stats.treatment_assignments++;
        } else {
            stats.control_assignments++;
        }
        
        stats.eligibility_rate = 
            static_cast<double>(stats.eligible_requests) / stats.total_requests;
    }

    // Record metric
    if (metrics_) {
        core::concerns::IMetrics::Labels labels;
        labels["test_id"] = test_id;
        labels["variant"] = use_treatment ? "treatment" : "control";
        labels["eligible"] = "true";
        metrics_->incrementCounter("themisdb_ab_test_traffic_routed_total", 1, labels);
    }

    return use_treatment;
}

void ABTestProductionRouter::recordProductionObservation(
    const std::string& test_id,
    const std::string& user_id,
    bool was_treatment,
    double latency_ms,
    bool success,
    double custom_metric) {
    
    ab_framework_->recordObservation(test_id, was_treatment, success, custom_metric);

    if (metrics_) {
        core::concerns::IMetrics::Labels labels;
        labels["test_id"] = test_id;
        labels["variant"] = was_treatment ? "treatment" : "control";
        labels["success"] = success ? "true" : "false";
        
        metrics_->recordHistogram("themisdb_ab_test_latency_ms", latency_ms, labels);
        metrics_->incrementCounter("themisdb_ab_test_observations_total", 1, labels);
        
        if (custom_metric > 0.0) {
            metrics_->setGauge("themisdb_ab_test_custom_metric", custom_metric, labels);
        }
    }
}

ABTestProductionRouter::RoutingStats
ABTestProductionRouter::getRoutingStats(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(impl_->stats_mutex);
    auto it = impl_->routing_stats.find(test_id);
    if (it != impl_->routing_stats.end()) {
        return it->second;
    }
    return RoutingStats{};
}

// ============================================================================
// ABTestPromotionEngine Implementation
// ============================================================================

struct ABTestPromotionEngine::Impl {
    std::unordered_map<std::string, std::vector<PromotionDecision>> promotion_history;
    mutable std::mutex history_mutex;
};

ABTestPromotionEngine::ABTestPromotionEngine(
    std::shared_ptr<ABTestingFramework> ab_framework,
    std::shared_ptr<MLLearningMetricsCollector> metrics_collector,
    std::shared_ptr<core::concerns::IMetrics> prometheus_metrics)
    : ab_framework_(ab_framework), metrics_collector_(metrics_collector),
      prometheus_metrics_(prometheus_metrics),
      impl_(std::make_unique<Impl>()) {}

ABTestPromotionEngine::~ABTestPromotionEngine() = default;

ABTestPromotionEngine::PromotionDecision
ABTestPromotionEngine::evaluatePromotion(
    const std::string& test_id,
    const PromotionConfig& config) {
    
    auto result = ab_framework_->evaluateTest(test_id);
    auto now = std::chrono::system_clock::now();

    PromotionDecision decision;
    decision.decision_time = now;
    decision.treatment_samples = result.treatment_sample_count;
    decision.control_samples = result.control_sample_count;
    decision.p_value = result.p_value;
    decision.treatment_improvement = result.treatment_improvement;

    // Check minimum sample size
    if (result.control_sample_count < config.min_samples_for_decision ||
        result.treatment_sample_count < config.min_samples_for_decision) {
        decision.decision = DecisionType::CONTINUE_TEST;
        decision.reason = "Insufficient samples for decision (need " + 
                         std::to_string(config.min_samples_for_decision) + ")";
        return decision;
    }

    // Check statistical significance
    if (!result.is_significant || result.p_value > config.significance_level) {
        decision.decision = DecisionType::CONTINUE_TEST;
        decision.reason = "Not statistically significant (p=" + 
                         std::to_string(result.p_value) + ")";
        return decision;
    }

    // Check improvement threshold
    if (result.treatment_improvement < config.min_improvement_threshold) {
        decision.decision = DecisionType::CONTINUE_TEST;
        decision.reason = "Improvement below threshold (" + 
                         std::to_string(result.treatment_improvement * 100.0) + "%)";
        return decision;
    }

    // If treatment is worse, rollback
    if (result.treatment_improvement < 0.0) {
        decision.decision = DecisionType::ROLLBACK;
        decision.reason = "Treatment performs worse than control (" + 
                         std::to_string(result.treatment_improvement * 100.0) + "%)";
        
        if (prometheus_metrics_) {
            core::concerns::IMetrics::Labels labels;
            labels["test_id"] = test_id;
            labels["reason"] = "worse_performance";
            prometheus_metrics_->incrementCounter(
                "themisdb_ab_test_rollback_decisions_total", 1, labels);
        }
        
        return decision;
    }

    // Check if test duration is sufficient
    auto test_status = ab_framework_->getTestStatus(test_id);
    // Assume we have test duration tracking (could enhance framework)
    
    // Everything looks good - promote
    decision.decision = DecisionType::PROMOTE;
    decision.reason = "Statistically significant improvement (" + 
                     std::to_string(result.treatment_improvement * 100.0) + "%, p=" + 
                     std::to_string(result.p_value) + ")";

    if (prometheus_metrics_) {
        core::concerns::IMetrics::Labels labels;
        labels["test_id"] = test_id;
        labels["improvement"] = std::to_string(result.treatment_improvement);
        prometheus_metrics_->incrementCounter(
            "themisdb_ab_test_promotion_decisions_total", 1, labels);
    }

    // Record to metrics collector
    if (metrics_collector_) {
        auto trace_ctx = MLLearningMetricsCollector::createTraceContext();
        metrics_collector_->recordABTestState(
            test_id, "promoted", result.treatment_improvement, trace_ctx);
    }

    // Store in history
    {
        std::lock_guard<std::mutex> lock(impl_->history_mutex);
        impl_->promotion_history[test_id].push_back(decision);
    }

    return decision;
}

std::vector<ABTestPromotionEngine::PromotionDecision>
ABTestPromotionEngine::getPromotionHistory(const std::string& test_id) const {
    std::lock_guard<std::mutex> lock(impl_->history_mutex);
    auto it = impl_->promotion_history.find(test_id);
    if (it != impl_->promotion_history.end()) {
        return it->second;
    }
    return {};
}

// ============================================================================
// ABTestRollbackAutomator Implementation
// ============================================================================

struct ABTestRollbackAutomator::Impl {
    std::vector<RollbackEvent> rollback_events;
    mutable std::mutex events_mutex;
};

ABTestRollbackAutomator::ABTestRollbackAutomator(
    std::shared_ptr<ABTestPromotionEngine> promotion_engine,
    std::shared_ptr<core::concerns::IMetrics> metrics)
    : promotion_engine_(promotion_engine), metrics_(metrics),
      impl_(std::make_unique<Impl>()) {}

ABTestRollbackAutomator::~ABTestRollbackAutomator() = default;

bool ABTestRollbackAutomator::shouldTriggerRollback(
    const std::string& test_id,
    const RollbackConfig& config) {
    
    if (!metrics_) {
        return false;  // Can't make rollback decision without metrics
    }

    // In production, this would query real-time metrics from Prometheus or stats system
    // For now, we provide the framework for checking SLO violations
    
    spdlog::debug("Checking rollback conditions for test {}", test_id);
    
    // This would be implemented with actual metrics provider integration
    // Check error rate, latency, and other SLO metrics
    
    return false;
}

bool ABTestRollbackAutomator::triggerRollback(
    const std::string& test_id,
    const std::string& reason) {
    
    RollbackEvent event;
    event.test_id = test_id;
    event.timestamp = std::chrono::system_clock::now();
    event.reason = reason;
    event.was_automatic = false;  // This is manual

    {
        std::lock_guard<std::mutex> lock(impl_->events_mutex);
        impl_->rollback_events.push_back(event);
    }

    if (metrics_) {
        core::concerns::IMetrics::Labels labels;
        labels["test_id"] = test_id;
        labels["reason"] = reason;
        metrics_->incrementCounter("themisdb_ab_test_rollback_total", 1, labels);
    }

    spdlog::info("A/B test {} rolled back: {}", test_id, reason);
    
    return true;
}

std::vector<ABTestRollbackAutomator::RollbackEvent>
ABTestRollbackAutomator::getRollbackHistory() const {
    std::lock_guard<std::mutex> lock(impl_->events_mutex);
    return impl_->rollback_events;
}

}  // namespace themis::rag::learning
