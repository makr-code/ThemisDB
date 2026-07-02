/**
 * @file PRODUCTION_ML_PIPELINE_DEPLOYMENT_CHECKLIST.md
 * @brief Production Deployment Checklist for Continuous Learning & ML Pipeline
 *
 * This document provides a comprehensive checklist for deploying the production-ready
 * continuous learning and ML pipeline for ThemisDB, including all signal wiring,
 * monitoring setup, and safety checks.
 *
 * @version 1.0
 * @date 2026-07-02
 * @author ThemisDB Team
 */

# Production ML Pipeline Deployment Checklist

## Overview

This checklist verifies that all components of the production-ready continuous learning
and ML pipeline are properly configured, wired, tested, and monitored before production
deployment.

**Estimated Setup Time:** 2-3 hours for initial deployment  
**Maintenance Cadence:** Daily (monitoring), Weekly (review)  
**On-Call Runbook:** See [PRODUCTION_ML_PIPELINE_RUNBOOK.md](#runbook)

---

## Pre-Deployment Phase (1-2 weeks before)

### Architecture Review
- [ ] ML pipeline architecture reviewed with stakeholders
- [ ] Signal providers identified and validated
- [ ] Feedback collection strategy documented
- [ ] A/B testing eligibility criteria defined
- [ ] Rollback procedures documented
- [ ] Success metrics and SLOs defined
  - [ ] Accuracy improvement target (e.g., >= 2%)
  - [ ] A/B test statistical significance threshold (e.g., p < 0.05)
  - [ ] Loop 1-4 trigger thresholds defined
  - [ ] Maximum rollback time SLO (e.g., <= 5 minutes)

### Dependency Verification
- [ ] ThemisDB core components available and tested
  - [ ] BaoOptimizer for HNSW miss-rate signal
  - [ ] WorkloadAdaptiveOptimizer for drift detection
  - [ ] FeedbackCollector for user feedback
- [ ] LoRA adapter framework deployed
  - [ ] IncrementalLoRATrainer operational
  - [ ] Model checkpointing working
  - [ ] Adapter registry initialized
- [ ] Prometheus/Grafana stack deployed
  - [ ] Prometheus scrape endpoints configured
  - [ ] Grafana dashboards created
  - [ ] Alert rules deployed

### Team Training
- [ ] On-call engineers trained on ML pipeline operations
- [ ] Runbook reviewed and walk-through completed
- [ ] Emergency contact procedures established
- [ ] Escalation paths defined

---

## Deployment Phase

### 1. Signal Provider Wiring

#### Loop 1: HNSW Query Miss-Rate Signal
```cpp
// ✓ Checkpoint: Bao optimizer miss-rate provider is wired
[ ] - [ ] BaoOptimizer::getMissRate() implemented and tested
    [ ] - [ ] Return value in range [0.0, 1.0]
    [ ] - [ ] No exceptions thrown
    [ ] - [ ] Response time < 10ms
    [ ] - [ ] Wired in ContinuousLearningOrchestrator::setHnswMissRateProvider()
    [ ] - [ ] Tested with wireLiveSignalProviders() in HTTP server
    [ ] - [ ] Fallback heuristic documented

[ ] - [ ] Validation test: TriggerLoop1WithRealMissRate
      - [ ] Supplies actual miss-rate from BaoOptimizer
      - [ ] Confirms signal_source = "live"
      - [ ] Verifies guardrail evaluation

[ ] - [ ] Production check: `curl http://localhost:8080/metrics | grep themisdb_ml_loop_1`
```

#### Loop 2: Workload Drift Signal
```cpp
// ✓ Checkpoint: Workload optimizer drift provider is wired
[ ] - [ ] WorkloadAdaptiveOptimizer::getProfileDrift() implemented and tested
    [ ] - [ ] Return value in range [0.0, 1.0]
    [ ] - [ ] Triggers on >= 0.1 drift threshold
    [ ] - [ ] Wired in ContinuousLearningOrchestrator::setWorkloadDriftProvider()
    [ ] - [ ] Tested with wireLiveSignalProviders() in HTTP server
    [ ] - [ ] Fallback heuristic documented (accuracy proxy)

[ ] - [ ] Validation test: TriggerLoop2WithDriftDetection
      - [ ] Simulates workload change (2+ adaptations)
      - [ ] Confirms drift >= 0.1
      - [ ] Verifies signal_source = "live"

[ ] - [ ] Production check: `curl http://localhost:8080/metrics | grep themisdb_ml_loop_2`
```

#### Loop 4: Feedback Entry Count Signal
```cpp
// ✓ Checkpoint: Feedback collector provider is wired
[ ] - [ ] FeedbackCollector::newEntryCount() implemented and tested
    [ ] - [ ] Returns count of new feedback entries since last call
    [ ] - [ ] Threshold check: >= 100 entries passes guardrail
    [ ] - [ ] Wired in ContinuousLearningOrchestrator::setFeedbackEntryCountProvider()
    [ ] - [ ] Tested with wireLiveSignalProviders() in HTTP server

[ ] - [ ] Validation test: LogFeedbackAndTriggerLoop4
      - [ ] Logs 100+ feedback entries
      - [ ] Triggers Loop 4
      - [ ] Confirms guardrail passed
      - [ ] Adapter version incremented if successful

[ ] - [ ] Production check: `curl http://localhost:8080/metrics | grep themisdb_ml_loop_4`
```

### 2. Component Registration

```cpp
[ ] - [ ] LoRA Adapters registered
    [ ] - [ ] registerLoRAAdapter() called for each active adapter
    [ ] - [ ] Adapter info strings set with version + domain
    [ ] - [ ] Example: "ThemisHelpLoRA_v1.0_legal"

[ ] - [ ] Retrieval systems registered
    [ ] - [ ] registerRetrievalSystem() called for main ANN index
    [ ] - [ ] Example: "vector_index_main"

[ ] - [ ] Prompt systems registered
    [ ] - [ ] registerPromptSystem() called for prompt library
    [ ] - [ ] Example: "prompt_library_v1"

[ ] - [ ] Knowledge gap detectors registered
    [ ] - [ ] registerKnowledgeGapDetector() called
    [ ] - [ ] Example: "gap_detector_v1"

[ ] - [ ] HTTP server bootstrap confirms all registrations
      - [ ] Check logs: `grep "registerLoRAAdapter\|registerRetrievalSystem" server.log`
```

### 3. Learning Loop Configuration

```cpp
[ ] - [ ] ContinuousLearningOrchestrator configured
    [ ] - [ ] min_feedback_samples = 100 (or appropriate for domain)
    [ ] - [ ] min_accuracy_drop = 0.05 (5%)
    [ ] - [ ] retraining_interval = 24h (or appropriate cadence)
    [ ] - [ ] enable_ab_testing = true
    [ ] - [ ] ab_test_traffic_split = 0.10 (10% for initial rollout)
    [ ] - [ ] min_improvement_threshold = 0.02 (2%)
    [ ] - [ ] enable_auto_rollback = true
    [ ] - [ ] enforce_live_providers = false (allow fallbacks initially)

[ ] - [ ] Loop execution parameters
    [ ] - [ ] Loop 1 trigger cooldown set (e.g., 1 hour)
    [ ] - [ ] Loop 2 trigger cooldown set (e.g., 2 hours)
    [ ] - [ ] Loop 3 trigger cooldown set (e.g., 6 hours)
    [ ] - [ ] Loop 4 trigger cooldown set (e.g., 12 hours)

[ ] - [ ] Learning loop started in HTTP server
      - [ ] Check: `curl http://localhost:8080/api/ml/status`
```

### 4. A/B Testing Setup

```cpp
[ ] - [ ] ABTestProductionRouter deployed
    [ ] - [ ] Eligibility criteria defined
      [ ] - [ ] Region-based eligibility
      [ ] - [ ] Customer tier-based eligibility
      [ ] - [ ] Opt-in status checking
    [ ] - [ ] Traffic splitting working
      [ ] - [ ] 10% treatment traffic for initial rollout
      [ ] - [ ] Consistent user assignment (hash-based)
    [ ] - [ ] Fallback to control on safety concerns

[ ] - [ ] ABTestPromotionEngine configured
    [ ] - [ ] Min samples threshold set (e.g., 1000)
    [ ] - [ ] Statistical significance threshold (e.g., p < 0.05)
    [ ] - [ ] Improvement threshold (e.g., 2%)
    [ ] - [ ] Promotion decision logging enabled

[ ] - [ ] ABTestRollbackAutomator configured
    [ ] - [ ] SLO thresholds defined
      [ ] - [ ] Max latency increase (e.g., +10%)
      [ ] - [ ] Max error rate increase (e.g., +1%)
      [ ] - [ ] Min user satisfaction (e.g., >= 0.80)
    [ ] - [ ] Rollback automation enabled
    [ ] - [ ] Alert rules triggered on rollback

[ ] - [ ] Manual promotion/rollback tested
      - [ ] `curl -X POST http://localhost:8080/api/ab/promote`
      - [ ] `curl -X POST http://localhost:8080/api/ab/rollback`
```

### 5. Feedback Collection Setup

```cpp
[ ] - [ ] User feedback collection API deployed
    [ ] - [ ] POST /api/feedback endpoint wired
    [ ] - [ ] Feedback types supported (POSITIVE, NEGATIVE, NEUTRAL)
    [ ] - [ ] Optional correction field for human fixes
    [ ] - [ ] Timestamp and interaction_id required

[ ] - [ ] Feedback storage configured
    [ ] - [ ] RocksDB storage available
    [ ] - [ ] Retention policy set (e.g., 90 days)
    [ ] - [ ] Backup schedule configured

[ ] - [ ] Implicit signal collection
    [ ] - [ ] Click tracking enabled (if applicable)
    [ ] - [ ] Dwell time tracking enabled
    [ ] - [ ] Session duration tracking enabled

[ ] - [ ] Feedback integration tests passing
      - [ ] Test: LogFeedbackAndVerifyCount
      - [ ] Test: TriggerRetrainingOnFeedbackThreshold
```

### 6. ML Observability Setup

```cpp
[ ] - [ ] MLLearningMetricsCollector initialized
    [ ] - [ ] getInstance() returns singleton
    [ ] - [ ] Prometheus registry configured
    [ ] - [ ] Metric namespace: themisdb_ml_*

[ ] - [ ] Prometheus metrics exported
    [ ] - [ ] Loop state transitions: themisdb_ml_loop_transitions_total
    [ ] - [ ] Loop durations: themisdb_ml_loop_duration_ms
    [ ] - [ ] Loop executions: themisdb_ml_loop_executions_total
    [ ] - [ ] Loop errors: themisdb_ml_loop_errors_total
    [ ] - [ ] Adapter deployments: themisdb_ml_adapter_deployments_total
    [ ] - [ ] Retraining progress: themisdb_ml_retraining_progress_percent
    [ ] - [ ] Model accuracy: themisdb_ml_model_accuracy
    [ ] - [ ] Inference latency: themisdb_ml_inference_latency_ms

[ ] - [ ] W3C Trace Context propagation enabled
    [ ] - [ ] Trace-ID headers set on all ML operations
    [ ] - [ ] Correlation IDs tracked through loops
    [ ] - [ ] Debugging via trace logs enabled

[ ] - [ ] Structured logging configured
    [ ] - [ ] Loop execution logged with phase, result, signal_source
    [ ] - [ ] Adapter updates logged with version, status, improvements
    [ ] - [ ] A/B test decisions logged with metrics, decision, reason

[ ] - [ ] Prometheus scrape endpoint working
      - [ ] `curl http://localhost:9091/metrics | grep themisdb_ml`
      - [ ] Verify metric types (counter, gauge, histogram)
```

### 7. Grafana Dashboards

```
[ ] - [ ] ML Pipeline Overview Dashboard
    [ ] - [ ] Loop state timeseries for all 4 loops
    [ ] - [ ] Loop execution success rate (%)
    [ ] - [ ] Average loop execution time (ms)
    [ ] - [ ] Current adapter version deployed

[ ] - [ ] Learning Loop Details Dashboard
    [ ] - [ ] Loop 1 HNSW miss-rate signal
    [ ] - [ ] Loop 2 workload drift signal
    [ ] - [ ] Loop 4 feedback entry count
    [ ] - [ ] Guardrail pass/fail rates

[ ] - [ ] A/B Testing Dashboard
    [ ] - [ ] Active A/B tests list
    [ ] - [ ] Traffic split visualization
    [ ] - [ ] Treatment vs control metrics
    [ ] - [ ] Promotion/rollback decision history

[ ] - [ ] Adapter Management Dashboard
    [ ] - [ ] Adapter version timeline
    [ ] - [ ] Model accuracy trends
    [ ] - [ ] Inference latency by adapter
    [ ] - [ ] Retraining progress

[ ] - [ ] Alerting Dashboard
    [ ] - [ ] Signal provider availability
    [ ] - [ ] Loop execution errors
    [ ] - [ ] Stalled retraining detection
    [ ] - [ ] SLO violations
```

### 8. Alert Rules Deployment

```
[ ] - [ ] Alert: ML_Loop_Provider_Unavailable
    [ ] - [ ] Triggers when signal provider not wired
    [ ] - [ ] Severity: WARNING
    [ ] - [ ] Action: Check HTTP server logs, restart if needed

[ ] - [ ] Alert: ML_Loop_Execution_Error
    [ ] - [ ] Triggers on loop execution failure
    [ ] - [ ] Severity: ERROR
    [ ] - [ ] Action: Review loop logs, check signal values

[ ] - [ ] Alert: ML_Retraining_Stalled
    [ ] - [ ] Triggers when retraining takes > 2x expected time
    [ ] - [ ] Severity: ERROR
    [ ] - [ ] Action: Check trainer logs, verify data pipeline

[ ] - [ ] Alert: ML_SLO_Violation
    [ ] - [ ] Triggers when loop execution time exceeds SLO
    [ ] - [ ] Severity: WARNING
    [ ] - [ ] Action: Review performance, scale if needed

[ ] - [ ] Alert: A/B_Test_Degradation
    [ ] - [ ] Triggers on treatment variant degradation
    [ ] - [ ] Severity: CRITICAL
    [ ] - [ ] Action: Manual review, may trigger auto-rollback
```

---

## Post-Deployment Phase (Continuous)

### Daily Checks (automated)

```
[ ] - [ ] All signal providers reporting
      - [ ] Bao miss-rate provider
      - [ ] Workload drift provider
      - [ ] Feedback count provider

[ ] - [ ] No provider failures in last 24h
      - [ ] Check: `grep "provider failed\|fallback_error" server.log | wc -l`
      - [ ] Expected: 0 (or documented exceptions)

[ ] - [ ] All loops executing successfully
      - [ ] Check: Loop 1-4 success rate >= 95%
      - [ ] Check: No stalled loops (last execution < 24h)

[ ] - [ ] Feedback collection active
      - [ ] Check: New feedback entries last 24h > 0
      - [ ] Check: Feedback rate increasing or stable

[ ] - [ ] Metrics being exported
      - [ ] Check: Prometheus scrape endpoint responding
      - [ ] Check: Metrics updated in last 5 minutes

[ ] - [ ] No critical alerts
      - [ ] Review Prometheus/Grafana alert dashboard
      - [ ] Acknowledge and investigate any critical alerts
```

### Weekly Reviews

```
[ ] - [ ] Loop success rates reviewed
      - [ ] Loop 1 success rate: ________%
      - [ ] Loop 2 success rate: ________%
      - [ ] Loop 3 success rate: ________%
      - [ ] Loop 4 success rate: ________%
      - [ ] Target: >= 90% for each loop

[ ] - [ ] Adapter improvement trends reviewed
      - [ ] Model accuracy trend: ________
      - [ ] Inference latency trend: ________
      - [ ] Retraining frequency: ________
      - [ ] Last successful retraining: ________

[ ] - [ ] A/B testing metrics reviewed
      - [ ] Active tests: ________
      - [ ] Tests promoted: ________
      - [ ] Tests rolled back: ________
      - [ ] Average test duration: ________

[ ] - [ ] Feedback quality reviewed
      - [ ] Feedback rate: ________ entries/day
      - [ ] Positive ratio: ________%
      - [ ] Manual corrections: ________
      - [ ] Gap detection results: ________

[ ] - [ ] System improvements documented
      - [ ] Notable performance improvements this week
      - [ ] Recommendations for next sprint
      - [ ] Any blocking issues or escalations
```

### Monthly Reviews

```
[ ] - [ ] Production readiness assessment
    [ ] - [ ] All components operational and stable
    [ ] - [ ] No unresolved critical issues
    [ ] - [ ] Performance meets or exceeds targets
    [ ] - [ ] Documentation up to date

[ ] - [ ] ML pipeline effectiveness review
    [ ] - [ ] Overall accuracy improvement: ________%
    [ ] - [ ] A/B test success rate: ________%
    [ ] - [ ] Average model improvement per retraining: ________%
    [ ] - [ ] User satisfaction impact: ________

[ ] - [ ] Capacity planning
    [ ] - [ ] Feedback storage usage: ________GB
    [ ] - [ ] Metrics storage usage: ________GB
    [ ] - [ ] Projected growth: ________
    [ ] - [ ] Scaling needed? [ ] Yes [ ] No

[ ] - [ ] Operational runbook review
    [ ] - [ ] Runbook up to date? [ ] Yes [ ] No
    [ ] - [ ] Team training current? [ ] Yes [ ] No
    [ ] - [ ] Emergency procedures practiced? [ ] Yes [ ] No
```

---

## Rollback Checklist (in case of issues)

### Immediate Rollback (< 5 minutes)

```
[ ] - [ ] CRITICAL ISSUE DETECTED
      - [ ] Alert: ________________
      - [ ] Severity: ________________
      - [ ] Timestamp: ________________

[ ] - [ ] Contact on-call engineer
      - [ ] On-call: ________________
      - [ ] Contact method: ________________
      - [ ] Response time: ________________

[ ] - [ ] Disable auto-promotion
      - [ ] curl -X POST http://localhost:8080/api/ml/disable-promotion

[ ] - [ ] Investigate root cause
      - [ ] Check server logs: tail -100 server.log
      - [ ] Check metrics: curl http://localhost:9091/metrics
      - [ ] Check HTTP server status: curl http://localhost:8080/health

[ ] - [ ] Execute rollback (if needed)
      - [ ] curl -X POST http://localhost:8080/api/ab/rollback
      - [ ] Verify control variant restored: curl http://localhost:8080/api/ab/status
      - [ ] Confirm success rate restored

[ ] - [ ] Document incident
      - [ ] Incident timestamp: ________________
      - [ ] Root cause: ________________
      - [ ] Resolution: ________________
      - [ ] Prevention: ________________
```

### Full Rollback (if orchestrator needs restart)

```
[ ] - [ ] Stop HTTP server gracefully
      - [ ] curl -X POST http://localhost:8080/shutdown
      - [ ] Wait for graceful shutdown (< 30s)

[ ] - [ ] Stop orchestrator
      - [ ] systemctl stop themisdb-ml-orchestrator

[ ] - [ ] Revert to previous stable version (if needed)
      - [ ] git checkout <stable-hash>
      - [ ] Rebuild: ./build.sh

[ ] - [ ] Restart in safe mode
      - [ ] Set enforce_live_providers = false
      - [ ] Set ab_test_traffic_split = 0.0 (no traffic)
      - [ ] systemctl start themisdb-ml-orchestrator

[ ] - [ ] Verify basic functionality
      - [ ] Check logs for errors: journalctl -u themisdb-ml-orchestrator
      - [ ] Verify HTTP server responsive: curl http://localhost:8080/health
      - [ ] Verify metrics available: curl http://localhost:9091/metrics

[ ] - [ ] Gradual recovery
      - [ ] Step 1: Enable with 1% A/B test traffic
      - [ ] Monitor for 1 hour
      - [ ] Step 2: Increase to 5% A/B test traffic
      - [ ] Monitor for 1 hour
      - [ ] Step 3: Increase to 10% A/B test traffic (production)
```

---

## Sign-Off

- [ ] **Architecture Review:** Approved by ________________ on ________________
- [ ] **Deployment Lead:** ________________ on ________________
- [ ] **On-Call Trained:** ________________ on ________________
- [ ] **Security Review:** Completed by ________________ on ________________
- [ ] **Production Ready:** Approved by ________________ on ________________

---

## References

- ContinuousLearningOrchestrator API: `include/rag/continuous_learning_orchestrator.h`
- A/B Testing Guide: `docs/de/rag/AB_TEST_PRODUCTION_INTEGRATION_GUIDE.md`
- ML Observability: `docs/de/rag/ML_OBSERVABILITY.md`
- Production Runbook: `PRODUCTION_ML_PIPELINE_RUNBOOK.md`
- Emergency Contacts: See internal wiki

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-02  
**Maintained By:** ThemisDB ML Team  
**Status:** ACTIVE
