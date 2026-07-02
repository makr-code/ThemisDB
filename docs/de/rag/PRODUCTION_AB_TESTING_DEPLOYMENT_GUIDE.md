/**
 * @file PRODUCTION_AB_TESTING_DEPLOYMENT_GUIDE.md
 * @brief Deployment Guide for Production A/B Testing System
 *
 * Step-by-step guide for deploying the A/B testing system to production,
 * including configuration, monitoring, and operational runbooks.
 */

# Production A/B Testing Deployment Guide

## Overview

This guide covers deploying ThemisDB's production A/B testing system for running experiments on LoRA adapters, retrieval systems, and prompt optimization in production traffic.

## Pre-Deployment Checklist

- [ ] Prometheus metrics collection working
- [ ] MLLearningMetricsCollector initialized
- [ ] Grafana dashboards created
- [ ] Alerting rules configured for SLO breaches
- [ ] On-call runbook reviewed
- [ ] Rollback procedure tested in staging
- [ ] Team trained on experiment workflow

## System Integration

### 1. Initialize Components (Application Startup)

```cpp
// In your application initialization code
#include "rag/ab_test_production_integration.h"

class RAGApplicationSetup {
public:
    void initialize() {
        // Initialize A/B testing framework
        ab_framework_ = std::make_unique<ABTestingFramework>();
        
        // Initialize production router
        router_ = std::make_unique<ABTestProductionRouter>(
            ab_framework_, prometheus_metrics_);
        
        // Initialize promotion engine
        promotion_engine_ = std::make_unique<ABTestPromotionEngine>(
            ab_framework_, metrics_collector_, prometheus_metrics_);
        
        // Initialize rollback automator
        rollback_automator_ = std::make_unique<ABTestRollbackAutomator>(
            promotion_engine_, prometheus_metrics_);
        
        // Start background monitoring thread
        monitoring_thread_ = std::thread([this]() {
            this->monitorAndDecide();
        });
    }

private:
    std::unique_ptr<ABTestingFramework> ab_framework_;
    std::unique_ptr<ABTestProductionRouter> router_;
    std::unique_ptr<ABTestPromotionEngine> promotion_engine_;
    std::unique_ptr<ABTestRollbackAutomator> rollback_automator_;
    std::thread monitoring_thread_;
};
```

### 2. Per-Request Integration (RAG Handler)

```cpp
// In your RAG request handler
class RAGRequestHandler {
    void handleRequest(const RAGRequest& request) {
        // Define eligibility criteria
        EligibilityCriteria criteria = buildEligibilityCriteria(request);
        
        // Route to control or treatment
        bool use_treatment = router_->selectTreatmentForRequest(
            "adapter_v2_trial", request.user_id(), criteria);
        
        // Select implementation
        auto* implementation = use_treatment 
            ? treatment_adapter_ 
            : control_adapter_;
        
        // Execute and measure
        auto start = std::chrono::high_resolution_clock::now();
        RAGResponse response = implementation->process(request);
        auto elapsed_ms = std::chrono::duration<double, std::milli>(
            std::chrono::high_resolution_clock::now() - start).count();
        
        // Record observation
        router_->recordProductionObservation(
            "adapter_v2_trial",
            request.user_id(),
            use_treatment,
            elapsed_ms,
            response.success,
            response.relevance_score);
    }

private:
    EligibilityCriteria buildEligibilityCriteria(const RAGRequest& request) {
        EligibilityCriteria criteria;
        criteria.user_region = request.user_region();
        criteria.customer_tier = request.customer_tier();
        criteria.is_paid_customer = request.is_paid();
        criteria.has_opted_in_to_experiments = 
            request.user_settings().experiments_enabled;
        
        // Temporal window (all day)
        auto now = std::chrono::system_clock::now();
        criteria.earliest_eligible_time = now - std::chrono::hours(24);
        criteria.latest_eligible_time = now + std::chrono::hours(24);
        
        return criteria;
    }
};
```

### 3. Periodic Monitoring (Background Thread)

```cpp
void RAGApplicationSetup::monitorAndDecide() {
    // Run every hour
    while (running_) {
        std::this_thread::sleep_for(std::chrono::hours(1));
        
        // Check each active test
        auto active_tests = ab_framework_->getActiveTests();
        for (const auto& test_id : active_tests) {
            // Evaluate promotion
            ABTestPromotionEngine::PromotionConfig config;
            config.min_samples_for_decision = 1000;
            config.min_improvement_threshold = 0.02;
            
            auto decision = promotion_engine_->evaluatePromotion(test_id, config);
            
            if (decision.decision == ABTestPromotionEngine::DecisionType::PROMOTE) {
                std::cout << "🚀 Promoting test " << test_id << std::endl;
                handlePromotion(test_id);
            } else if (decision.decision == ABTestPromotionEngine::DecisionType::ROLLBACK) {
                std::cout << "⚠️  Rolling back test " << test_id << std::endl;
                rollback_automator_->triggerRollback(test_id, decision.reason);
                handleRollback(test_id);
            }
        }
        
        // Check for SLO violations every minute
        std::this_thread::sleep_for(std::chrono::seconds(30));
        ABTestRollbackAutomator::RollbackConfig rollback_config;
        rollback_config.error_rate_threshold = 0.05;
        rollback_config.latency_increase_threshold = 0.20;
        
        for (const auto& test_id : active_tests) {
            if (rollback_automator_->shouldTriggerRollback(test_id, rollback_config)) {
                std::cout << "🚨 SLO violation for test " << test_id << std::endl;
                rollback_automator_->triggerRollback(
                    test_id, "Automatic SLO violation");
                handleRollback(test_id);
            }
        }
    }
}
```

## Prometheus Monitoring

### Dashboard Queries

Create a Grafana dashboard with these key metrics:

```promql
# Current A/B test traffic volume
rate(themisdb_ab_test_traffic_routed_total[5m])

# Treatment vs Control success rate
rate(themisdb_ab_test_observations_total{success="true", variant="treatment"}[5m]) /
rate(themisdb_ab_test_observations_total{variant="treatment"}[5m])

# Latency comparison
histogram_quantile(0.99, rate(themisdb_ab_test_latency_ms_bucket[5m]))

# Eligibility rate (% of users included in test)
rate(themisdb_ab_test_traffic_routed_total{eligible="true"}[5m]) /
rate(themisdb_ab_test_traffic_routed_total[5m])

# Rollback events
increase(themisdb_ab_test_rollback_total[1h])

# Promotion decision rate
rate(themisdb_ab_test_promotion_decisions_total[1h])
```

### Alert Rules

```yaml
groups:
  - name: ab_testing
    rules:
      - alert: ABTestHighErrorRate
        expr: |
          rate(themisdb_ab_test_observations_total{success="false", variant="treatment"}[5m]) > 0.05
        for: 5m
        annotations:
          summary: "A/B test treatment error rate > 5%"
          description: "Test {{ $labels.test_id }} has error rate {{ $value | humanizePercentage }}"

      - alert: ABTestHighLatency
        expr: |
          histogram_quantile(0.99, rate(themisdb_ab_test_latency_ms_bucket{variant="treatment"}[5m])) > 100
        for: 5m
        annotations:
          summary: "A/B test treatment latency p99 > 100ms"

      - alert: ABTestLowEligibility
        expr: |
          rate(themisdb_ab_test_traffic_routed_total{eligible="true"}[5m]) / 
          rate(themisdb_ab_test_traffic_routed_total[5m]) < 0.01
        for: 10m
        annotations:
          summary: "A/B test eligibility rate < 1%"
```

## Operational Runbook

### Starting a New A/B Test

1. **Define Test Requirements**
   ```yaml
   test_id: "adapter_v2_trial"
   component: "LoRA"
   traffic_split: 0.10  # 10% to treatment
   duration: 72h
   target_samples: 2000
   success_metric: "retrieval_relevance"
   slo_latency_p99: 100ms
   slo_error_rate: 1%
   ```

2. **Eligibility Criteria**
   ```cpp
   // Decide who participates
   allowed_regions: ["us-east", "us-west"]
   customer_tier: ["premium", "enterprise"]
   opt_in_required: true
   temporal_window: [2026-07-03T00:00Z, 2026-07-06T00:00Z]
   ```

3. **Create and Deploy**
   ```cpp
   ABTestConfig config;
   config.test_id = "adapter_v2_trial";
   config.traffic_split = 0.10;
   config.min_samples = 2000;
   ab_framework->startTest(config);
   ```

4. **Monitor**
   - Watch metrics dashboard hourly
   - Alert on SLO breaches
   - Check promotion readiness

### Promotion Workflow

**Hourly Check:**
1. Sufficient samples? (typically 1000+)
2. Statistically significant? (p < 0.05)
3. Minimum improvement? (e.g., 2%)
4. Within SLO? (latency, error rate)

**If All Checks Pass:**
1. Send alert to ops team
2. Review decision details
3. Approve promotion (manual or automatic)
4. Deploy treatment to production
5. Complete old test in system

**Decision Audit Trail:**
```
Promotion Decision: PROMOTE
Test ID: adapter_v2_trial
Time: 2026-07-03T14:00:00Z
Treatment Improvement: 8.5%
P-value: 0.0031
Samples (Control/Treatment): 2100 / 2150
SLO Status: ✓ All passed
Decision Reason: Statistically significant improvement with p=0.0031
```

### Rollback Workflow

**Automatic Triggers:**
- Error rate spike (> threshold for 5 minutes)
- Latency increase (> 20% from baseline)
- SLO burn-rate high (budget consumed 10x faster)

**Manual Triggers:**
- Operator detects quality issue
- Customer feedback indicates regression
- Related infrastructure issue detected

**Rollback Procedure:**
1. Immediate traffic redirect to control
2. Stop metrics collection for treatment
3. Record rollback event
4. Alert team with reason
5. Create post-incident review ticket
6. Analyze root cause

**Example Rollback Alert:**
```
🚨 ROLLBACK INITIATED
Test: adapter_v2_trial
Reason: Error rate exceeded threshold (6.2% > 5.0%)
Time: 2026-07-03T15:30:00Z
Duration in Production: 1h 30m
Impact: ~5000 users
Action: Traffic redirected to control
Follow-up: Investigate error spike
```

## Production Safeguards

### Traffic Capping
```cpp
// Never exceed traffic cap
const double MAX_TREATMENT_TRAFFIC = 0.30;  // Max 30% in production

if (treatment_assignments / total_requests > MAX_TREATMENT_TRAFFIC) {
    // Immediately stop treatment routing
    use_treatment = false;
}
```

### Minimum Duration
```cpp
// Run at least this long before promotion
const auto MIN_TEST_DURATION = std::chrono::hours(24);

if (now - test_start_time < MIN_TEST_DURATION) {
    return DecisionType::CONTINUE_TEST;
}
```

### SLO Guardrails
```cpp
// Hard stops on SLO violations
const double MAX_ERROR_RATE = 0.05;  // 5% errors
const double MAX_LATENCY_MS = 150.0;   // 150ms p99

if (current_error_rate > MAX_ERROR_RATE ||
    current_latency_p99 > MAX_LATENCY_MS) {
    triggerRollback("SLO violation");
}
```

## Disaster Recovery

### If Metrics System Fails
- Stop routing traffic to treatment
- Continue collecting to file
- Manual review when metrics restored

### If A Test Hangs
```bash
# Manually cancel via API
curl -X POST http://api.themisdb.io/ab/test/cancel \
  -d '{"test_id": "broken_test", "reason": "Stuck"}'
```

### If Decision Engine Fails
- Fallback to manual approvals
- Route 100% to control
- Alert engineering team

## Post-Deployment Validation

### Day 1 Checklist
- [ ] Metrics flowing to Prometheus
- [ ] Dashboard displays correct data
- [ ] Alerts firing correctly
- [ ] Test started successfully
- [ ] Traffic routed to treatment
- [ ] Observations recorded
- [ ] Promotion decision calculated

### Week 1 Review
- [ ] No unexpected errors or alerts
- [ ] Traffic split matches configuration
- [ ] Eligibility rate reasonable
- [ ] Latency impact acceptable
- [ ] Error rates normal
- [ ] Team confident in system

### Before First Production Promotion
- [ ] Results reviewed by data team
- [ ] Business stakeholders approved
- [ ] Rollback plan documented
- [ ] On-call engineer briefed
- [ ] Gradual rollout strategy ready

## Support & Troubleshooting

### Common Issues

**Q: Eligibility rate very low?**
A: Check criteria - may be too strict. Verify region/tier filters.

**Q: Promotion keeps saying "continue test"?**
A: Need more samples or larger improvement. Check thresholds.

**Q: Sudden rollback?**
A: Check metrics dashboard for SLO violations, error spikes.

**Q: Metrics not appearing?**
A: Verify Prometheus is scraping, metrics exporter running.

### Escalation Path
1. Check dashboard for obvious issues
2. Review application logs
3. Contact data platform team
4. Escalate to on-call engineer if needed

## References

- [Production Integration Guide](AB_TEST_PRODUCTION_INTEGRATION_GUIDE.md)
- [Prometheus Metrics](../../docs/de/rag/ML_OBSERVABILITY.md)
- [Statistical Methods](../../research/PROMPT_ENHANCEMENT_ENGINE_OPTIMIZATION_RESEARCH.md#3-rollback-safety)
