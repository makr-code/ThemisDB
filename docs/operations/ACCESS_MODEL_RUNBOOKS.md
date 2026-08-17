# Access Model Coordinator — Operator Runbooks

**Document Version:** 1.0.0  
**Module:** Access Model (Tier Coordination)  
**Target Audience:** Database operators, SREs  
**Date:** 2026-08-17  

---

## Overview

This runbook provides diagnostic and remediation procedures for the **Access Model Coordinator**, which manages automatic tier promotions/demotions between cache (L1/L2) and storage (COLD/WARM/HOT) tiers.

**Key Components:**
- **AccessCoordinator:** Central tier orchestrator
- **EvictionListener / PromotionListener:** Event interfaces from cache/storage
- **AgeBasedPolicy:** Unified aging rules for tier transitions
- **AccessMetrics:** Observability collectors

---

## Symptom-Based Troubleshooting

### Symptom 1: Promotions Not Happening (Hot Data Stuck in Cold Tier)

**Impact:** Query latency increase; cache miss rate high; storage bottleneck

**Detection Criteria:**
- Metric: `access_model_promotion_attempts_total == 0` (over 5-min window)
- Metric: `access_model_storage_access_hotspot_frequency <= 1.0 event/sec`
- Trace: No promotion events in logs for keys with >100 accesses
- Alert: Custom query: `rate(cache_query_latency_p99_ms[5m]) > baseline * 1.5`

**Investigation Steps:**

1. **Check Coordinator Status**
   ```bash
   # SSH to database node
   curl http://localhost:8080/debug/access_coordinator/status
   # Expected: {"status": "running", "worker_threads": 4, "event_queue_depth": <N>}
   ```

2. **Verify Policy Configuration**
   ```bash
   curl http://localhost:8080/debug/access_model/policy
   # Expected: age thresholds, size limits, promotion rules
   # Look for: age_threshold_secs, size_limit_bytes
   ```

3. **Check Tier Registration**
   ```bash
   curl http://localhost:8080/debug/access_model/tiers
   # Expected: All tiers (L1, L2, COLD, WARM, HOT) registered
   ```

4. **Review Logs**
   ```bash
   journalctl -u themisdb -n 1000 | grep -i "promotion\|demote" | tail -50
   # Look for: REJECTED reasons, DEFER reasons, policy conflicts
   ```

5. **Check Event Queue Depth**
   ```bash
   curl http://localhost:8080/metrics | grep event_queue_depth
   # Expected: <1000 (not backed up)
   # If >5000: coordinator workers are saturated
   ```

**Short-Term Fixes:**

1. **Increase Worker Threads**
   ```bash
   # Edit: /etc/themisdb/config.yaml
   access_coordinator:
     worker_threads: 8  # Increase from 4 to 8
   
   # Restart service
   sudo systemctl restart themisdb
   ```

2. **Relax Age Threshold**
   ```bash
   # Edit policy: lower promotion trigger age
   access_coordinator:
     age_policy:
       promotion_age_threshold_secs: 60  # Reduce from 300
   
   sudo systemctl restart themisdb
   ```

3. **Clear Event Queue Backlog**
   ```bash
   # Trigger manual drain
   curl -X POST http://localhost:8080/debug/access_model/drain_queue
   ```

**Long-Term Solution:**

- Review capacity planning: are worker threads undersized for workload?
- Analyze access patterns: are they uniformly distributed or highly skewed?
- Consider sharding coordinator state by tier for higher parallelism
- See: `docs/operations/ACCESS_MODEL_CAPACITY_PLANNING.md`

---

### Symptom 2: Worker Pool Stuck (Events Not Processing)

**Impact:** Tier transitions halt; cache/storage decouple; degraded performance

**Detection Criteria:**
- Metric: `access_model_event_queue_depth >= 10000` (sustained >1 min)
- Metric: `access_model_promotion_successes_total` not incrementing
- Trace: No worker activity logs (no "event processed" entries)
- Alert: `event_queue_depth > threshold AND seconds_since_last_event > 30`

**Investigation Steps:**

1. **Check Worker Thread Status**
   ```bash
   curl http://localhost:8080/debug/access_coordinator/workers
   # Expected: all threads "running" with last_activity timestamp recent
   ```

2. **Detect Deadlock**
   ```bash
   # Capture stack traces
   sudo gdb attach $(pgrep themisdb)
   thread apply all bt
   # Look for: all threads blocked on mutex, condition_variable
   ```

3. **Monitor System Resources**
   ```bash
   top -p $(pgrep themisdb) -n 5
   # Check: CPU utilization (should be >50% if processing)
   # Check: memory growth (stable or increasing?)
   ```

4. **Review Recent Logs for Errors**
   ```bash
   journalctl -u themisdb -n 2000 --priority=err
   # Look for: segfaults, unhandled exceptions, resource exhaustion
   ```

**Short-Term Fixes:**

1. **Graceful Restart**
   ```bash
   # Trigger shutdown + restart
   sudo systemctl restart themisdb
   # Monitor: event queue should drain during shutdown
   ```

2. **Manual Worker Wake**
   ```bash
   curl -X POST http://localhost:8080/debug/access_model/wake_workers
   # Forces worker threads to check queue
   ```

3. **Drain Event Queue to File**
   ```bash
   curl http://localhost:8080/debug/access_model/queue_snapshot > /tmp/queue_dump.json
   # Analyze: are events valid? corrupted? duplicated?
   ```

**Long-Term Solution:**

- Implement deadlock detection (periodic watchdog)
- Add timeout enforcement on worker operations
- Consider work-stealing scheduler for better throughput
- See: `docs/architecture/ACCESS_MODEL_CONCURRENCY_DESIGN.md`

---

### Symptom 3: Memory Spike in Coordinator

**Impact:** OOM risk; process killed by kernel; service unavailable

**Detection Criteria:**
- Metric: `access_model_memory_bytes > 200MB` (baseline 50MB)
- Alert: RSS growth > 50MB in 1 minute
- Symptom: Application restart required after OOM

**Investigation Steps:**

1. **Check Event Queue Size**
   ```bash
   curl http://localhost:8080/metrics | grep -E "event_queue|queue_depth"
   # If queue_depth > 1M: backlog causing memory buildup
   ```

2. **Capture Memory Profile**
   ```bash
   # Install flamegraph tools
   sudo apt-get install linux-tools-common
   
   # Profile memory
   sudo perf record -F 99 -p $(pgrep themisdb) --call-graph=dwarf -- sleep 30
   sudo perf script > out.perf
   # Analyze: which function allocated most memory?
   ```

3. **Check for Memory Leaks**
   ```bash
   # Run under valgrind (on non-prod)
   valgrind --leak-check=full --show-leak-kinds=all ./themisdb
   # Look for: "still reachable", "definitely lost"
   ```

**Short-Term Fixes:**

1. **Drain Queue Aggressively**
   ```bash
   # Temporarily increase worker throughput
   curl -X POST http://localhost:8080/debug/access_model/flush_queue?timeout_ms=5000
   ```

2. **Clear Metrics Buffer**
   ```bash
   # Reset in-memory metrics (if retained long)
   curl -X POST http://localhost:8080/debug/access_model/reset_metrics
   ```

3. **Reduce Max Queue Depth**
   ```bash
   # Edit: /etc/themisdb/config.yaml
   access_coordinator:
     max_queue_depth: 50000  # Reduce from 100000
   
   sudo systemctl restart themisdb
   ```

**Long-Term Solution:**

- Implement memory backpressure: reject promotions if queue > threshold
- Add periodic queue compaction
- Investigate event size: are event payloads too large?
- See: `docs/operations/ACCESS_MODEL_MEMORY_TUNING.md`

---

### Symptom 4: Promotion Latency Spike

**Impact:** Queries stalled during promotions; p95/p99 latency increase

**Detection Criteria:**
- Metric: `access_model_promotion_latency_p99_ms > 100` (gate: ≤50µs)
- Alert: `rate(slow_promotions[5m]) > 10 events/sec`
- Query: `SELECT count(*) FROM logs WHERE event="promotion" AND latency_ms > threshold`

**Investigation Steps:**

1. **Check Storage I/O**
   ```bash
   iostat -x 1 5 | grep sda  # Or appropriate storage device
   # Look for: high await time, low throughput (queue issues)
   ```

2. **Check Tier Contention**
   ```bash
   curl http://localhost:8080/debug/access_model/tier_contention
   # Expected: even distribution of accesses across tiers
   # Problem: one tier being hammered
   ```

3. **Review Slow Promotions**
   ```bash
   # Query logs for top-10 slowest promotions
   curl http://localhost:8080/debug/access_model/slow_operations?limit=10
   # Analyze: which tier pair? which keys? size distribution?
   ```

4. **Check Network Latency** (if storage is remote)
   ```bash
   ping -c 10 <storage_node>
   iperf3 -c <storage_node> -t 10
   # Look for: increased RTT, packet loss
   ```

**Short-Term Fixes:**

1. **Prioritize Small Promotions**
   ```bash
   # Edit policy to prefer quick wins
   access_coordinator:
     age_policy:
       max_promotion_size_bytes: 1048576  # Skip >1MB promotions
   ```

2. **Disable Promotions Temporarily**
   ```bash
   curl -X POST http://localhost:8080/debug/access_model/pause?duration_ms=300000
   # Pauses for 5 minutes while storage recovers
   ```

3. **Isolate Tier Contention**
   ```bash
   # Route traffic away from busy tier (manual override)
   curl -X POST http://localhost:8080/debug/access_model/route_override \
     -d '{"from_tier": "COLD", "to_tier": "WARM", "active": false}'
   ```

**Long-Term Solution:**

- Add tiered SSD/HDD separation for storage tiers
- Implement parallel promotion batching
- Consider pre-warming during off-peak
- See: `docs/operations/ACCESS_MODEL_PERFORMANCE_TUNING.md`

---

### Symptom 5: Policy Conflicts (Contradictory Decisions)

**Impact:** Unpredictable tier placement; hot data demoted; cold data promoted

**Detection Criteria:**
- Log: `promotion_decision=PROMOTE AND demotion_decision=DEMOTE` (same key, rapid)
- Metric: `access_model_policy_conflicts_total > 100` (over 5-min window)
- Alert: Config mismatch detected between cache and storage policies

**Investigation Steps:**

1. **Verify Policy Configuration**
   ```bash
   # Check coordinator policy
   curl http://localhost:8080/debug/access_model/policy | jq
   
   # Check cache policy
   curl http://localhost:8080/debug/cache/policy | jq
   
   # Check storage policy
   curl http://localhost:8080/debug/storage/policy | jq
   
   # Compare: age thresholds, size limits, promotion rules must align
   ```

2. **Review Recent Policy Changes**
   ```bash
   git log --oneline -- src/access_model/age_based_policy.* | head -5
   # Look for: recent changes that may have broken contract
   ```

3. **Query Conflict Events**
   ```bash
   curl 'http://localhost:8080/debug/access_model/conflicts?limit=20&since_minutes=60'
   # Analyze: which keys? which tier pairs? root cause?
   ```

**Short-Term Fixes:**

1. **Reset Policy to Defaults**
   ```bash
   curl -X POST http://localhost:8080/debug/access_model/reset_policy
   # Reverts to well-tested baseline configuration
   ```

2. **Synchronize Cache↔Storage Policies**
   ```bash
   # Copy storage policy to cache
   curl http://localhost:8080/debug/storage/policy | \
     jq '.age_threshold, .size_limit' | \
     curl -X POST http://localhost:8080/debug/cache/update_policy -d @-
   ```

3. **Disable Automatic Decisions Temporarily**
   ```bash
   curl -X POST http://localhost:8080/debug/access_model/auto_mode?enabled=false
   # Switch to manual promotions only
   ```

**Long-Term Solution:**

- Add configuration validation at startup (cross-module checks)
- Implement policy version contract (both parties agree)
- Add audit trail for policy changes
- See: `docs/architecture/ACCESS_MODEL_POLICY_CONTRACT.md`

---

## Preventive Monitoring

### Recommended Alerting Rules

| Alert | Metric | Threshold | Duration |
|-------|--------|-----------|----------|
| HighEventQueueDepth | `event_queue_depth` | >5000 | 2 min |
| LowPromotionRate | `promotion_successes_total` | 0 (no change) | 5 min |
| HighPromotionLatency | `promotion_latency_p99_ms` | >100 | 1 min |
| HighMemoryUsage | `memory_bytes` | >200MB | 2 min |
| WorkerThreadStuck | Last activity | >60s | immediate |
| PolicyConflicts | `policy_conflicts_total` | >100 in 5min | 5 min |

### Baseline Metrics (Healthy State)

| Metric | Healthy Baseline | Warning Threshold |
|--------|------------------|-------------------|
| Event queue depth | <1000 | >5000 |
| Promotion latency p95 | <50µs | >100µs |
| Promotion success rate | >99% | <95% |
| Memory usage | 50MB | >200MB |
| Worker activity | events every <100ms | >1000ms gap |
| Promotion rate | >100 events/sec | <10 events/sec |

### Weekly Health Check Procedure

```bash
#!/bin/bash
# Run weekly to verify coordinator health

echo "1. Checking coordinator status..."
curl http://localhost:8080/debug/access_coordinator/status

echo "2. Verifying all tiers registered..."
curl http://localhost:8080/debug/access_model/tiers | jq '.[] | {tier: .name, status: .status}'

echo "3. Checking recent error count..."
curl http://localhost:8080/metrics | grep -E "errors_total|failures"

echo "4. Verifying metrics collection..."
curl http://localhost:8080/metrics | head -20

echo "5. Performance gates validation..."
curl http://localhost:8080/debug/access_model/gates | jq '.[] | {gate: .id, status: .status, value: .value, target: .target}'

echo "✓ Health check complete"
```

---

## Escalation Procedures

### When to Contact Development Team

1. **Memory leak suspected** (OOM recurring despite mitigation)
2. **Deadlock detected** (workers stuck, unable to wake)
3. **Data loss** (events dropped, queue truncated)
4. **Policy contract violation** (incompatible config signatures)
5. **Performance regression** (gates failing, no config change)

**Escalation Template:**

```
Subject: Access Model Coordinator Issue — Immediate Investigation Required

Description:
  - Symptom: [specific metric/behavior]
  - Detection time: [timestamp]
  - Affected duration: [start-end timestamps]
  - Attempted mitigations: [list of fixes tried]
  - Current state: [is it ongoing? resolved temporarily?]

Evidence:
  - Metrics export: [file attached]
  - Log snapshot: [file attached, last 1000 lines]
  - Config snapshot: [file attached]

Required by: [date/time]
```

---

## Reference Documentation

- **Architecture:** `docs/architecture/UNIFIED_ACCESS_MODEL.md`
- **Configuration:** `docs/operations/ACCESS_MODEL_CONFIG_REFERENCE.md`
- **Performance Tuning:** `docs/operations/ACCESS_MODEL_PERFORMANCE_TUNING.md`
- **Capacity Planning:** `docs/operations/ACCESS_MODEL_CAPACITY_PLANNING.md`
- **Policy Contract:** `docs/architecture/ACCESS_MODEL_POLICY_CONTRACT.md`

---

**Document Status:** 2026-08-17 DRAFT  
**Last Reviewed:** TBD  
**Next Review:** Quarterly post-GA

