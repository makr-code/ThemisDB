/**
 * @file PRODUCTION_ML_PIPELINE_RUNBOOK.md
 * @brief Operations Runbook for Production ML Pipeline
 *
 * This document provides step-by-step operational procedures for running,
 * monitoring, and troubleshooting the production continuous learning and ML pipeline.
 *
 * @version 1.0
 * @date 2026-07-02
 * @author ThemisDB Operations Team
 */

# Production ML Pipeline Operations Runbook

## Table of Contents

1. [Quick Start (5 minutes)](#quick-start)
2. [Daily Operations](#daily-operations)
3. [Common Scenarios](#common-scenarios)
4. [Troubleshooting](#troubleshooting)
5. [Emergency Procedures](#emergency-procedures)
6. [Escalation Matrix](#escalation-matrix)

---

## Quick Start (5 minutes)

### Verify System is Running

```bash
# Check HTTP server status
curl -s http://localhost:8080/health | jq .

# Expected output:
# { "status": "healthy", "uptime_seconds": 12345 }

# Check ML orchestrator status
curl -s http://localhost:8080/api/ml/status | jq .

# Expected output:
# {
#   "orchestrator_running": true,
#   "loops_active": 4,
#   "last_loop_execution": "2026-07-02T15:35:00Z",
#   "signal_providers_wired": true
# }

# Check metrics endpoint
curl -s http://localhost:9091/metrics | grep "themisdb_ml" | head -5
```

### Enable ML Pipeline (if disabled)

```bash
# Start the continuous learning loop
curl -X POST http://localhost:8080/api/ml/start

# Enable A/B testing
curl -X POST http://localhost:8080/api/ab/enable -d '{"traffic_split": 0.1}'

# Verify enabled
curl -s http://localhost:8080/api/ml/status | jq '.orchestrator_running'
# Expected: true
```

---

## Daily Operations

### Morning Checklist (5 minutes)

```bash
#!/bin/bash
# daily_checklist.sh

echo "=== ML Pipeline Daily Checklist ==="
echo ""

# 1. Check system health
echo "1. System Health:"
curl -s http://localhost:8080/health | jq '.status'

# 2. Check signal providers
echo "2. Signal Providers:"
curl -s http://localhost:8080/api/ml/signals | jq '.providers'

# 3. Check loop execution status
echo "3. Loop Status:"
curl -s http://localhost:8080/api/ml/loops | jq '.[] | {loop_id, last_execution, success_rate}'

# 4. Check feedback collection
echo "4. Feedback Rate (entries/day):"
curl -s http://localhost:9091/metrics | grep "themisdb_ml_feedback_entries_total" | tail -1

# 5. Check active A/B tests
echo "5. Active A/B Tests:"
curl -s http://localhost:8080/api/ab/tests | jq '.tests[] | {test_id, traffic_split, duration_hours}'

# 6. Check recent improvements
echo "6. Recent Improvements:"
curl -s http://localhost:8080/api/ml/improvements | jq '.recent[] | {timestamp, component, improvement_percent}'

# 7. Alert summary
echo "7. Critical Alerts:"
curl -s http://localhost:9091/alerts | jq '.[] | select(.severity == "CRITICAL") | {alert, value}'
```

### Afternoon Check-in (5 minutes)

```bash
#!/bin/bash
# afternoon_check.sh

echo "=== Afternoon Status Update ==="

# Check if any loops have failed
FAILED_LOOPS=$(curl -s http://localhost:8080/api/ml/loops | jq '.[] | select(.status == "failed") | .loop_id')

if [ -n "$FAILED_LOOPS" ]; then
    echo "⚠️  FAILED LOOPS DETECTED: $FAILED_LOOPS"
    echo "Action: Run 'troubleshoot_loop.sh <loop_id>'"
else
    echo "✓ All loops executing successfully"
fi

# Check signal provider health
PROVIDER_HEALTH=$(curl -s http://localhost:8080/api/ml/signals | jq '.provider_health')
echo "Signal Provider Health: $PROVIDER_HEALTH"

# Check metrics ingestion rate
METRICS_RATE=$(curl -s http://localhost:9091/metrics | grep "scrape_duration_seconds" | tail -1)
echo "Metrics Ingestion: OK"

# Check A/B test status
ACTIVE_TESTS=$(curl -s http://localhost:8080/api/ab/tests | jq '.tests | length')
echo "Active A/B Tests: $ACTIVE_TESTS"
```

### Weekly Review (30 minutes)

```bash
#!/bin/bash
# weekly_review.sh

echo "=== Weekly ML Pipeline Review ==="
echo "Date: $(date)"
echo ""

# 1. Loop Performance Summary
echo "## Loop Execution Performance"
curl -s http://localhost:8080/api/ml/metrics/weekly | jq '{
  loop_1: .loops.LOOP_1 | {success_rate, avg_duration_ms, total_executions},
  loop_2: .loops.LOOP_2 | {success_rate, avg_duration_ms, total_executions},
  loop_3: .loops.LOOP_3 | {success_rate, avg_duration_ms, total_executions},
  loop_4: .loops.LOOP_4 | {success_rate, avg_duration_ms, total_executions}
}'

# 2. Model Improvement Trends
echo ""
echo "## Model Improvement Trends"
curl -s http://localhost:8080/api/ml/metrics/weekly | jq '{
  accuracy_improvement_percent: .model.accuracy_change,
  retraining_count: .model.retraining_count,
  adapters_deployed: .model.new_adapters,
  avg_inference_latency_ms: .model.avg_latency_ms
}'

# 3. A/B Testing Summary
echo ""
echo "## A/B Testing Summary"
curl -s http://localhost:8080/api/ab/metrics/weekly | jq '{
  tests_completed: .completed_count,
  tests_promoted: .promoted_count,
  tests_rolled_back: .rolled_back_count,
  avg_test_duration_days: .avg_duration_days,
  total_improvement_percent: .cumulative_improvement_percent
}'

# 4. Feedback Statistics
echo ""
echo "## Feedback Collection"
curl -s http://localhost:8080/api/ml/feedback/stats | jq '{
  total_entries: .total_entries,
  daily_rate: .daily_rate,
  positive_ratio: .positive_ratio,
  manual_corrections: .corrections_count
}'

# 5. System Capacity
echo ""
echo "## System Capacity & Usage"
curl -s http://localhost:8080/api/ml/capacity | jq '{
  feedback_storage_gb: .feedback_storage_gb,
  metrics_storage_gb: .metrics_storage_gb,
  projected_storage_needed_gb: .projected_monthly_growth_gb,
  memory_usage_gb: .memory_usage_gb
}'

# Export for record-keeping
curl -s http://localhost:8080/api/ml/metrics/weekly > "./reports/ml_metrics_$(date +%Y%m%d).json"
echo ""
echo "Report saved to: ./reports/ml_metrics_$(date +%Y%m%d).json"
```

---

## Common Scenarios

### Scenario 1: Triggering Manual Loop Execution

**Use Case:** You want to manually trigger a learning loop outside the normal cadence.

```bash
#!/bin/bash
# trigger_loop.sh <loop_number>

LOOP=$1
if [ -z "$LOOP" ]; then
    echo "Usage: ./trigger_loop.sh <1|2|3|4>"
    exit 1
fi

echo "Triggering Loop $LOOP..."

case $LOOP in
    1)
        curl -X POST http://localhost:8080/api/ml/triggers/loop1 \
            -H "Content-Type: application/json" \
            -d '{"query_id": "manual_trigger_'$(date +%s)'", "force": true}'
        ;;
    2)
        curl -X POST http://localhost:8080/api/ml/triggers/loop2 \
            -H "Content-Type: application/json" \
            -d '{"force": true}'
        ;;
    3)
        curl -X POST http://localhost:8080/api/ml/triggers/loop3 \
            -H "Content-Type: application/json" \
            -d '{"force": true}'
        ;;
    4)
        curl -X POST http://localhost:8080/api/ml/triggers/loop4 \
            -H "Content-Type: application/json" \
            -d '{"force": true}'
        ;;
esac

# Monitor execution
sleep 2
curl -s http://localhost:8080/api/ml/loops/$LOOP | jq '{
  loop_id: .loop_id,
  status: .status,
  last_execution: .last_execution,
  result: .last_result
}'
```

### Scenario 2: Promoting an A/B Test

**Use Case:** A treatment variant shows improvement and you want to promote it to 100% traffic.

```bash
#!/bin/bash
# promote_ab_test.sh <test_id>

TEST_ID=$1
if [ -z "$TEST_ID" ]; then
    echo "Usage: ./promote_ab_test.sh <test_id>"
    exit 1
fi

echo "Promoting A/B Test: $TEST_ID"

# Get current test metrics
echo "Current Test Metrics:"
curl -s http://localhost:8080/api/ab/tests/$TEST_ID | jq '{
  test_id: .test_id,
  traffic_split: .traffic_split,
  treatment_success_rate: .treatment_success_rate,
  control_success_rate: .control_success_rate,
  improvement_percent: .improvement_percent,
  p_value: .p_value
}'

# Promote to 100% traffic
echo ""
echo "Promoting treatment to 100% traffic..."
RESULT=$(curl -X POST http://localhost:8080/api/ab/promote \
    -H "Content-Type: application/json" \
    -d '{
        "test_id": "'$TEST_ID'",
        "reason": "manual_promotion_operator_review",
        "operator": "'$(whoami)'"
    }')

echo "Promotion Result:"
echo $RESULT | jq '.promotion_result | {status, timestamp, new_traffic_split}'

# Verify promotion
sleep 2
echo ""
echo "Verification:"
curl -s http://localhost:8080/api/ab/tests/$TEST_ID | jq '.traffic_split'
```

### Scenario 3: Rolling Back a Failed A/B Test

**Use Case:** A treatment variant is causing performance degradation and needs rollback.

```bash
#!/bin/bash
# rollback_ab_test.sh <test_id> <reason>

TEST_ID=$1
REASON=$2

if [ -z "$TEST_ID" ] || [ -z "$REASON" ]; then
    echo "Usage: ./rollback_ab_test.sh <test_id> <reason>"
    echo "Reasons: degraded_latency, reduced_accuracy, increased_errors, manual_review"
    exit 1
fi

echo "ROLLING BACK A/B Test: $TEST_ID"
echo "Reason: $REASON"

# Get current metrics before rollback
echo ""
echo "Pre-Rollback Metrics:"
curl -s http://localhost:8080/api/ab/tests/$TEST_ID | jq '{
  treatment_success_rate,
  treatment_latency_p95_ms,
  treatment_error_rate
}'

# Execute rollback
echo ""
echo "Executing rollback..."
ROLLBACK=$(curl -X POST http://localhost:8080/api/ab/rollback \
    -H "Content-Type: application/json" \
    -d '{
        "test_id": "'$TEST_ID'",
        "reason": "'$REASON'",
        "operator": "'$(whoami)'",
        "force": false
    }')

echo "Rollback Result:"
echo $ROLLBACK | jq '.rollback_result | {status, completed_at, reverted_to}'

# Verify rollback completed
sleep 3
echo ""
echo "Post-Rollback Status:"
curl -s http://localhost:8080/api/ab/tests/$TEST_ID | jq '{
  traffic_split,
  status: .status,
  control_active: .control_is_active
}'

echo ""
echo "✓ Rollback Complete"
```

### Scenario 4: Inspecting Adapter Versions

**Use Case:** You want to see what adapter versions are deployed and their performance.

```bash
#!/bin/bash
# inspect_adapters.sh

echo "=== Deployed Adapter Versions ==="
echo ""

# Get all adapters
ADAPTERS=$(curl -s http://localhost:8080/api/ml/adapters | jq '.adapters[].adapter_id' -r)

for ADAPTER_ID in $ADAPTERS; do
    echo "Adapter: $ADAPTER_ID"
    
    # Get version details
    curl -s http://localhost:8080/api/ml/adapters/$ADAPTER_ID | jq '{
        current_version: .current_version,
        deployment_status: .deployment_status,
        deployed_at: .deployed_at,
        accuracy: .performance.accuracy,
        inference_latency_ms: .performance.inference_latency_ms,
        error_rate: .performance.error_rate,
        A_B_test_id: .ab_test_id
    }'
    
    echo ""
done

# Get version history
echo "=== Version History ==="
curl -s http://localhost:8080/api/ml/adapters/history | jq '.history[] | {
  timestamp,
  adapter_id,
  from_version,
  to_version,
  event_type,
  reason
}'
```

---

## Troubleshooting

### Problem 1: Signal Provider Not Connected

**Symptoms:**
- Loops execute but signal_source shows "fallback_missing"
- Prometheus metrics show high fallback_missing count

**Investigation:**

```bash
#!/bin/bash
# diagnose_signal_provider.sh

echo "=== Signal Provider Diagnostics ==="

# Check if providers are wired
echo "Provider Status:"
curl -s http://localhost:8080/api/ml/signals | jq '.providers'

# Check error counts
echo ""
echo "Provider Failure Counts:"
curl -s http://localhost:9091/metrics | grep "provider_failures_total"

# Check HTTP server logs
echo ""
echo "Recent Logs (last 50 lines):"
tail -50 /var/log/themisdb/server.log | grep -i "provider\|signal"

# Try manual provider call (if accessible)
echo ""
echo "Available Providers:"
curl -s http://localhost:8080/api/ml/signals/available | jq '.'
```

**Solutions:**

```bash
# Solution 1: Restart HTTP server
systemctl restart themisdb-http-server

# Solution 2: Reinitialize signal providers
curl -X POST http://localhost:8080/api/ml/signals/reinitialize

# Solution 3: Check dependent services
# Are these services running?
systemctl status themisdb-bao-optimizer
systemctl status themisdb-workload-optimizer
systemctl status themisdb-feedback-collector
```

### Problem 2: Loop Execution Taking Too Long

**Symptoms:**
- Loop execution time exceeds SLO (e.g., > 30 seconds for Loop 1)
- Alert: ML_LOOP_EXECUTION_TIMEOUT

**Investigation:**

```bash
#!/bin/bash
# diagnose_slow_loop.sh <loop_id>

LOOP_ID=$1
if [ -z "$LOOP_ID" ]; then
    echo "Usage: ./diagnose_slow_loop.sh <LOOP_1|LOOP_2|LOOP_3|LOOP_4>"
    exit 1
fi

echo "=== Diagnosing Slow Loop: $LOOP_ID ==="

# Check recent execution times
echo "Recent Execution Times:"
curl -s http://localhost:9091/metrics | \
    grep "themisdb_ml_loop_duration_ms_bucket{loop_id=\"$LOOP_ID\"" | tail -5

# Check if loop is compute-bound
echo ""
echo "System Resources:"
top -b -n 1 | head -12

# Check database connectivity
echo ""
echo "Database Status:"
curl -s http://localhost:8080/api/db/status | jq '.database'

# Check thread pool
echo ""
echo "Thread Pool Status:"
curl -s http://localhost:8080/api/ml/threads | jq '.'

# Check error rates
echo ""
echo "Error Rate:"
curl -s http://localhost:9091/metrics | \
    grep "themisdb_ml_loop_errors_total{loop_id=\"$LOOP_ID\""
```

**Solutions:**

```bash
# Solution 1: Increase thread pool size
curl -X POST http://localhost:8080/api/ml/configure \
    -d '{"thread_pool_size": 16}'

# Solution 2: Disable other loops temporarily
curl -X POST http://localhost:8080/api/ml/loops/LOOP_2/disable
curl -X POST http://localhost:8080/api/ml/loops/LOOP_3/disable

# Solution 3: Scale up resources
# Allocate more CPU/memory to ThemisDB process

# Solution 4: Check for database contention
curl -s http://localhost:8080/api/db/locks | jq '.active_locks'
```

### Problem 3: Feedback Collection Not Working

**Symptoms:**
- Feedback entries not increasing
- Loop 4 guardrail always fails due to low feedback count

**Investigation:**

```bash
#!/bin/bash
# diagnose_feedback.sh

echo "=== Feedback Collection Diagnostics ==="

# Check feedback API endpoint
echo "Feedback API Status:"
curl -s http://localhost:8080/health | jq '.endpoints.feedback'

# Check feedback entry count
echo ""
echo "Feedback Entry Count:"
curl -s http://localhost:8080/api/ml/feedback/count

# Check feedback storage
echo ""
echo "Feedback Storage Status:"
curl -s http://localhost:8080/api/ml/feedback/storage | jq '{
  total_entries,
  storage_used_gb,
  storage_available_gb
}'

# Check recent feedback entries
echo ""
echo "Recent Feedback Entries:"
curl -s http://localhost:8080/api/ml/feedback/recent?limit=5 | jq '.entries[] | {
  interaction_id,
  timestamp,
  feedback_type,
  confidence
}'
```

**Solutions:**

```bash
# Solution 1: Verify feedback API is accessible
curl -X POST http://localhost:8080/api/feedback \
    -H "Content-Type: application/json" \
    -d '{
        "interaction_id": "test_feedback_'$(date +%s)'",
        "feedback_type": "POSITIVE",
        "confidence": 0.9
    }'

# Solution 2: Restart feedback collector
systemctl restart themisdb-feedback-collector

# Solution 3: Check RocksDB storage
# Verify RocksDB is not corrupted
themisdb-admin db verify

# Solution 4: Manually log test feedback
for i in {1..10}; do
    curl -X POST http://localhost:8080/api/feedback \
        -d '{"interaction_id": "test_'$i'", "feedback_type": "POSITIVE"}'
done
```

---

## Emergency Procedures

### Emergency Shutdown (Last Resort)

```bash
#!/bin/bash
# emergency_shutdown.sh
# Use only if system is in unstable state

echo "⚠️  EMERGENCY SHUTDOWN INITIATED"
echo "Timestamp: $(date)"

# 1. Stop accepting new requests
curl -X POST http://localhost:8080/api/ml/stop-accepting-requests

# 2. Complete in-flight operations (30 second timeout)
sleep 5
echo "Waiting for in-flight operations to complete..."
sleep 25

# 3. Stop learning loops
curl -X POST http://localhost:8080/api/ml/stop

# 4. Stop A/B testing
curl -X POST http://localhost:8080/api/ab/disable

# 5. Graceful shutdown
systemctl stop themisdb-http-server

# 6. Kill remaining processes if necessary
sleep 5
pkill -9 themisdb_server || true

# 7. Verify stopped
sleep 2
echo "Service Status After Shutdown:"
systemctl status themisdb-http-server || echo "Service successfully stopped"
```

### Recovery Procedure

```bash
#!/bin/bash
# recovery_procedure.sh

echo "=== ML Pipeline Recovery ==="

# 1. Check system health
echo "1. Pre-recovery checks..."
systemctl status themisdb-http-server || echo "Service not running"
systemctl status themisdb-feedback-collector || echo "Feedback service not running"

# 2. Clean up any stale state
echo "2. Cleaning up stale state..."
rm -f /tmp/themisdb_ml_*.lock || true

# 3. Start services
echo "3. Starting services..."
systemctl start themisdb-feedback-collector
systemctl start themisdb-http-server

# 4. Wait for startup
sleep 10

# 5. Verify health
echo "4. Verifying health..."
curl -s http://localhost:8080/health | jq '.status'

# 6. Verify signal providers
echo "5. Verifying signal providers..."
curl -s http://localhost:8080/api/ml/signals | jq '.providers'

# 7. Start learning loops (conservative mode)
echo "6. Starting learning loops (conservative)..."
curl -X POST http://localhost:8080/api/ml/start \
    -d '{"conservative_mode": true, "ab_test_traffic_split": 0.0}'

# 8. Monitor for issues
echo "7. Monitoring... (60 seconds)"
for i in {1..12}; do
    echo "Check $i:"
    curl -s http://localhost:8080/api/ml/status | jq '.loops_active'
    sleep 5
done

echo "✓ Recovery Complete"
```

---

## Escalation Matrix

### Level 1: Local Investigation (you)

**Duration:** 15 minutes max  
**Tools:** Troubleshooting scripts above  
**Success Criteria:** Issue identified and solution attempted

```
Symptoms:
  - Single loop failing
  - Signal provider temporarily unavailable
  - Feedback collection rate degraded

Actions:
  1. Run diagnostic script for the component
  2. Check logs for error messages
  3. Attempt local fix (restart service, reinitialize)
  4. Verify fix worked
```

### Level 2: Team Escalation

**Duration:** 30-45 minutes  
**Team:** ML Pipeline Team Lead  
**Contact:** On call engineer  

**When to escalate:**
- Multiple loops failing
- Signal providers all unavailable
- A/B testing stalled for > 1 hour
- Feedback collection completely stopped

**Required Information:**
```bash
# Gather diagnostic bundle
mkdir -p /tmp/diagnostic_bundle
cp /var/log/themisdb/server.log /tmp/diagnostic_bundle/
curl -s http://localhost:9091/metrics > /tmp/diagnostic_bundle/metrics.txt
curl -s http://localhost:8080/api/ml/status > /tmp/diagnostic_bundle/ml_status.json
top -b -n 3 > /tmp/diagnostic_bundle/system_resources.txt
tar czf diagnostic_$(date +%s).tar.gz /tmp/diagnostic_bundle/
```

### Level 3: Critical Incident

**Duration:** Immediate response  
**Team:** Incident Commander + ML Team + DevOps  
**Escalation:** PagerDuty + Slack #themisdb-incidents  

**Automatic escalation triggers:**
- Multiple loops failed for > 5 minutes
- All signal providers unavailable
- Data corruption detected in feedback store
- Uncontrolled rollbacks happening

**Emergency contact chain:**
1. Team Lead: [phone/Slack]
2. Engineering Manager: [phone/Slack]
3. Incident Commander: [phone/Slack]

---

## Key Contacts

| Role | Name | Slack | Phone |
|------|------|-------|-------|
| ML Ops Lead | @ml_ops_lead | #ml-ops | 555-0001 |
| On-Call (Weekday) | @on_call_weekday | @dm | 555-0002 |
| On-Call (Weekend) | @on_call_weekend | @dm | 555-0003 |
| Incident Commander | @incident_commander | @dm | 555-0911 |

---

## Quick Reference

### Important URLs

```
HTTP Server Health:     http://localhost:8080/health
ML Status:              http://localhost:8080/api/ml/status
Metrics Endpoint:       http://localhost:9091/metrics
Grafana Dashboard:      http://grafana.local:3000
Prometheus:             http://prometheus.local:9090
Log Files:              /var/log/themisdb/server.log
Configuration:          /etc/themisdb/ml_config.yaml
```

### Key Metrics

```prometheus
# Watch these in Prometheus
themisdb_ml_loop_executions_total{status="success"}
themisdb_ml_loop_errors_total
themisdb_ml_loop_duration_ms
themisdb_ml_adapter_deployments_total
themisdb_ml_feedback_entries_total
themisdb_ml_ab_test_promotion_total
```

### Configuration File

```yaml
# /etc/themisdb/ml_config.yaml
continuous_learning:
  enabled: true
  min_feedback_samples: 100
  min_accuracy_drop: 0.05
  retraining_interval: 86400  # seconds

signal_providers:
  bao_miss_rate: enabled
  workload_drift: enabled
  feedback_count: enabled

ab_testing:
  enabled: true
  initial_traffic_split: 0.1
  min_improvement_threshold: 0.02
  auto_rollback_enabled: true

observability:
  prometheus_enabled: true
  tracing_enabled: true
  metrics_port: 9091
```

---

**Document Version:** 1.0  
**Last Updated:** 2026-07-02  
**Maintained By:** ML Operations Team  
**Status:** ACTIVE
