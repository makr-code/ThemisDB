/**
 * @file AB_TEST_PRODUCTION_INTEGRATION_GUIDE.md
 * @brief Production A/B Testing Integration Guide
 * 
 * This document describes the production integration of the A/B testing framework
 * with live traffic routing, metrics collection, and automated promotion/rollback.
 */

# Production A/B Testing Integration Guide

## Overview

The Production A/B Testing Integration connects ThemisDB's A/B Testing Framework to real production traffic with:

- **Live Traffic Routing**: Real-time eligibility gating and consistent user assignment
- **Metrics-Driven Decisions**: Statistical promotion logic based on production metrics
- **Automated Rollback**: SLO-aware automatic rollback on performance degradation
- **Comprehensive Monitoring**: Prometheus metrics and audit trail for all decisions

## Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                  Production Request Stream                  │
└──────────────────┬──────────────────────────────────────────┘
                   │
                   ▼
        ┌──────────────────────────┐
        │ ABTestProductionRouter   │
        │                          │
        │ ✓ Eligibility Gating     │
        │ ✓ Traffic Splitting      │
        │ ✓ Live Observation       │
        │   Recording              │
        └────────┬─────────────────┘
                 │
        ┌────────┴─────────────────────────────┐
        │                                      │
        ▼                                      ▼
   ┌──────────┐                         ┌──────────────┐
   │ Control  │                         │ Treatment    │
   │ Variant  │                         │ Variant      │
   └────┬─────┘                         └──────┬───────┘
        │                                      │
        └──────────────┬───────────────────────┘
                       │
                       ▼
        ┌──────────────────────────────────┐
        │ Metrics Collection               │
        │ (Latency, Success, Custom)       │
        └────────┬─────────────────────────┘
                 │
        ┌────────┴──────────────────────────────────────┐
        │                                               │
        ▼                                               ▼
┌──────────────────────────┐          ┌──────────────────────────┐
│ ABTestPromotionEngine    │          │ ABTestRollbackAutomator  │
│                          │          │                          │
│ ✓ Statistical Analysis   │          │ ✓ SLO Monitoring         │
│ ✓ Improvement Threshold  │          │ ✓ Performance Guardrails │
│ ✓ Promotion Decision     │          │ ✓ Auto Rollback Trigger  │
└────────┬─────────────────┘          └────────┬─────────────────┘
         │                                     │
         └──────────────┬────────────────────┬─┘
                        │                    │
                        ▼                    ▼
        ┌──────────────────────────────────────────┐
        │   MLLearningMetricsCollector              │
        │ (Prometheus Metrics + Audit Trail)       │
        └──────────────────────────────────────────┘
```

## Core Components

### 1. ABTestProductionRouter

Routes production traffic between control and treatment variants with eligibility gating.

**Responsibilities:**
- Apply eligibility criteria (region, customer tier, opt-in status)
- Perform consistent user assignment using traffic split
- Record observations from production (latency, success, custom metrics)
- Expose routing statistics

**Usage Example:**

```cpp
auto router = std::make_unique<ABTestProductionRouter>(
    ab_framework, prometheus_metrics);

// For each production request
EligibilityCriteria criteria;
criteria.user_region = request.region();
criteria.is_paid_customer = user.is_premium();
criteria.has_opted_in_to_experiments = user.experiments_enabled();

bool use_treatment = router->selectTreatmentForRequest(
    "lora_v2_trial",      // test_id
    user.id(),            // user_id  
    criteria);

// Execute control or treatment variant...

// Record observation
router->recordProductionObservation(
    "lora_v2_trial",
    user.id(),
    use_treatment,
    request_latency_ms,
    request_succeeded,
    business_metric_value);  // Optional
```

**Eligibility Criteria:**

```cpp
struct EligibilityCriteria {
    // User attributes
    std::string user_region;
    std::string customer_tier;  // free, standard, premium, enterprise
    bool is_paid_customer = false;
    bool has_opted_in_to_experiments = true;
    
    // Temporal window
    std::chrono::system_clock::time_point earliest_eligible_time;
    std::chrono::system_clock::time_point latest_eligible_time;
    
    // Performance constraints
    double max_acceptable_latency_ms = 5000.0;
    bool requires_cache_hit = false;
    
    // Region filtering
    std::vector<std::string> allowed_regions;
    std::vector<std::string> blocked_regions;
};
```

**Safety Properties:**
- Ineligible users always get control (fail-safe)
- Consistent assignment: same user always gets same variant
- Comprehensive metrics collection for debugging

### 2. ABTestPromotionEngine

Makes automated promotion/rollback decisions based on statistical analysis.

**Decision Types:**

- **PROMOTE**: Treatment is statistically significantly better (p < threshold)
- **ROLLBACK**: Treatment is worse or violates SLO criteria
- **CONTINUE_TEST**: Need more data or inconclusive results
- **ERROR**: Insufficient data or test not found

**Configuration:**

```cpp
ABTestPromotionEngine::PromotionConfig config;
config.min_improvement_threshold = 0.02;        // 2% improvement
config.significance_level = 0.05;               // p-value threshold
config.min_samples_for_decision = 1000;         // Min samples
config.max_latency_percentile_99 = 100.0;       // ms
config.max_error_rate = 0.01;                   // 1%
config.min_test_duration = std::chrono::hours(24);
config.max_test_duration = std::chrono::hours(72);
```

**Usage:**

```cpp
auto promotion_engine = std::make_unique<ABTestPromotionEngine>(
    ab_framework, metrics_collector, prometheus_metrics);

// Evaluate test periodically
auto decision = promotion_engine->evaluatePromotion(
    "lora_v2_trial", config);

if (decision.decision == ABTestPromotionEngine::DecisionType::PROMOTE) {
    // Deploy treatment to production
    deployTreatment(decision.reason);
} else if (decision.decision == ABTestPromotionEngine::DecisionType::ROLLBACK) {
    // Rollback treatment
    rollbackTreatment(decision.reason);
}
```

**History Tracking:**

```cpp
auto history = promotion_engine->getPromotionHistory("lora_v2_trial");
for (const auto& decision : history) {
    std::cout << "Decision: " << (int)decision.decision
              << " Improvement: " << (decision.treatment_improvement * 100) << "%"
              << " p-value: " << decision.p_value
              << " Reason: " << decision.reason << std::endl;
}
```

### 3. ABTestRollbackAutomator

Monitors production metrics and automatically triggers rollback on SLO violations.

**Configuration:**

```cpp
ABTestRollbackAutomator::RollbackConfig config;
config.error_rate_threshold = 0.05;             // Rollback if > 5%
config.latency_increase_threshold = 0.20;       // Rollback if > 20%
config.evaluation_window = std::chrono::seconds(60);
config.burn_rate_window = std::chrono::seconds(300);
config.error_rate_burn_rate = 10.0;  // Rollback if burns budget 10x faster
```

**Usage:**

```cpp
auto rollback_automator = std::make_unique<ABTestRollbackAutomator>(
    promotion_engine, prometheus_metrics);

// Periodic check (e.g., every minute)
if (rollback_automator->shouldTriggerRollback("lora_v2_trial", config)) {
    rollback_automator->triggerRollback("lora_v2_trial", 
        "SLO violation: error rate exceeded");
}

// Manual rollback if needed
rollback_automator->triggerRollback("lora_v2_trial", 
    "Manual: customer impact detected");
```

**Rollback History:**

```cpp
auto history = rollback_automator->getRollbackHistory();
for (const auto& event : history) {
    std::cout << "Test: " << event.test_id
              << " Time: " << event.timestamp
              << " Reason: " << event.reason
              << " Auto: " << event.was_automatic << std::endl;
}
```

## Prometheus Metrics

### Traffic Routing Metrics

```
# Total requests routed through A/B test
themisdb_ab_test_traffic_routed_total{test_id="...", variant="control|treatment", eligible="true"}

# Routing statistics
themisdb_ab_test_eligibility_rate{test_id="..."}

# Request latency distribution
themisdb_ab_test_latency_ms{test_id="...", variant="control|treatment"}

# Observations recorded
themisdb_ab_test_observations_total{test_id="...", variant="control|treatment", success="true|false"}
```

### Promotion Decision Metrics

```
# Promotion decisions made
themisdb_ab_test_promotion_decisions_total{test_id="...", improvement="..."}

# Rollback decisions made
themisdb_ab_test_rollback_decisions_total{test_id="...", reason="..."}

# A/B test state changes
themisdb_ml_ab_test_state_changes_total{test_id="...", status="running|promoted|rolled_back"}

# Treatment improvement percentage
themisdb_ml_ab_test_improvement_percent{test_id="...", status="..."}
```

## Integration with ContinuousLearningOrchestrator

The production integration is designed to work seamlessly with ContinuousLearningOrchestrator:

```cpp
// In ContinuousLearningOrchestrator initialization
auto router = std::make_unique<ABTestProductionRouter>(
    ab_framework_, prometheus_metrics_);
auto promotion_engine = std::make_unique<ABTestPromotionEngine>(
    ab_framework_, metrics_collector_, prometheus_metrics_);
auto rollback_automator = std::make_unique<ABTestRollbackAutomator>(
    promotion_engine, prometheus_metrics_);

// Per-request (in RAG handler)
bool use_treatment = router->selectTreatmentForRequest(
    "adapter_v2_test", request.user_id(), criteria);

// Execute variant and collect metrics...

router->recordProductionObservation(
    "adapter_v2_test", request.user_id(), use_treatment,
    latency_ms, success, business_metric);

// Periodic check (every hour)
auto decision = promotion_engine->evaluatePromotion(
    "adapter_v2_test", promotion_config);
if (decision.decision == ABTestPromotionEngine::DecisionType::PROMOTE) {
    promoteAdapter("adapter_v2");
}

// Continuous monitoring
if (rollback_automator->shouldTriggerRollback(
    "adapter_v2_test", rollback_config)) {
    rollback_automator->triggerRollback("adapter_v2_test", 
        "SLO violation");
}
```

## Production Runbook

### Starting a New A/B Test

1. Create test configuration with clear success metrics
2. Set eligibility criteria (regions, customer tiers, temporal window)
3. Configure traffic split (typically 5-10% for new changes)
4. Start test via `ABTestingFramework::startTest()`
5. Wire routing and observation recording in request path
6. Monitor metrics dashboard

### Decision Cadence

- **Every Hour**: Evaluate promotion (check statistical significance)
- **Every Minute**: Check SLO violations for rollback
- **Per-Request**: Route traffic and record observations

### Safe Promotion

1. **Sufficient Samples**: Ensure minimum sample size (typically 1000+)
2. **Significance**: p-value < 0.05 (95% confidence)
3. **Minimum Improvement**: Require measurable improvement (2%+)
4. **SLO Compliance**: Latency and error rate within bounds
5. **Manual Approval** (optional): Require human sign-off

### Emergency Rollback

1. **Automatic Detection**: SLO violations trigger immediate rollback
2. **Manual Trigger**: Operators can force rollback anytime
3. **Audit Trail**: All rollback events logged to Prometheus
4. **Post-Incident**: Review root cause and improve guardrails

## Monitoring Dashboard

Key Prometheus queries for Grafana dashboard:

```promql
# Overall A/B test status
count(themisdb_ab_test_traffic_routed_total)

# Treatment success rate
rate(themisdb_ab_test_observations_total{success="true"}[5m])

# Latency comparison
avg(themisdb_ab_test_latency_ms{variant="control"}) vs
avg(themisdb_ab_test_latency_ms{variant="treatment"})

# Eligibility rate
rate(themisdb_ab_test_traffic_routed_total{eligible="true"}[5m]) /
rate(themisdb_ab_test_traffic_routed_total[5m])

# Rollback events
increase(themisdb_ab_test_rollback_total[1h])
```

## Security & Safety Considerations

1. **Eligibility Gating**: Users who don't consent never see experiments
2. **Fail-Safe**: Ineligible users always default to control
3. **Audit Trail**: All decisions logged with timestamps and reasons
4. **Rate Limiting**: Automatic rollback if error rate spikes
5. **Manual Override**: Operators can stop tests anytime
6. **Data Privacy**: No sensitive user data collected in metrics

## Performance Impact

- **Routing Overhead**: < 1ms per request (hash-based assignment)
- **Metrics Collection**: < 2ms per request (batched writes)
- **Memory Usage**: < 10MB per active test (sample storage)
- **Thread Safety**: Lock-free reads where possible, coarse locks for updates

## Future Enhancements

- [ ] Bandit algorithms for adaptive traffic splitting
- [ ] Multi-armed bandits for more sophisticated allocation
- [ ] Automated confidence interval calculation
- [ ] Integration with feature flags for safer rollout
- [ ] Distributed A/B testing across multiple regions
- [ ] Real-time anomaly detection for faster rollback
- [ ] User cohort analysis (demographic-based insights)
