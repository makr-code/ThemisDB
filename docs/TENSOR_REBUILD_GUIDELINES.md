## Status: Stale – Archivierungskandidat
> **Hinweis (2026-08-12):** Diese Datei enthält TODO/FIXME/STALE/TBD/PLACEHOLDER-Marker und wird als Archivierungskandidat geführt. Inhalte wurden nicht gelöscht. Für den aktuellen Stand bitte kanonische Quellen und den [Root-Index](00_DOCUMENTATION_INDEX.md) konsultieren.
<!-- stale-marker: DOC-WEEKLY-2026-33 -->


> **⚠️ STATUS: STALE – Archivierungskandidat**
> Dieser Inhalt enthält veraltete TODO/FIXME/PLACEHOLDER-Marker und wird im nächsten Archiv-Run nach `docs/ARCHIVED/` verschoben.
> Bitte nicht als aktuelle Referenz nutzen. Inventar: [DOCS_INVENTORY_2026-Q3.md](Audit/DOCS_INVENTORY_2026-Q3.md)

---

# Tensor Artifact Rebuild Guidelines and Thresholds

**Status:** Engineering Reference  
**Date:** 2026-07-05  
**Related Issue:** makr-code/ThemisDB#5433

---

## Purpose

Provide concrete, measurable thresholds and decision rules for when and how to rebuild distributed tensor artifacts. Complements policy documents with operational guidance.

---

## 1. Rebuild Thresholds

### 1.1 Staleness Thresholds (by Artifact Class)

| Artifact Class | Max Age Before Yellow | Max Age Before Red | Typical Refresh Interval |
|---|---|---|---|
| **Primary Tensor (LoRA)** | 1 hour | 24 hours | 6 hours (if updated) |
| **Adapter (LLM specific)** | Never stale*¹ | — | Immutable once materialized |
| **Shard Summary** | 5 minutes | 30 minutes | 2 minutes |
| **Routing Tensor** | 10 minutes | 60 minutes | 5 minutes |
| **Factor Cache** | 1 hour | 24 hours | Background rebuild |
| **Derived Summary** | 30 minutes | 4 hours | On-demand or lazy |
| **Query Cache** | 1 second | 10 seconds | Query-specific invalidation |

*¹ Adapters are immutable snapshots; marked stale only on model switch

### 1.2 Freshness Debt Thresholds

**Freshness Debt Calculation**:
```
Debt = Σ(artifact_i.staleness_duration × artifact_i.query_impact_weight)
```

| Threshold | Status | Action |
|---|---|---|
| 0–5% | Green | Normal operation; no action |
| 5–20% | Yellow | Log warning; schedule background refresh |
| >20% | Red | Immediate action: invalidate or rebuild |

**Example**:
- Artifact A: stale for 10 min, impact weight 0.5 → contributes 5 units
- Artifact B: stale for 30 min, impact weight 0.2 → contributes 6 units
- Total debt = 11 units; if max capacity = 50, debt ratio = 22% → **RED**

### 1.3 Update Delta Thresholds

| Condition | Action | Rationale |
|---|---|---|
| Delta age <50ms | **Hold**, batch more deltas | Reduce patch overhead |
| Delta age 50–200ms | **Apply** if no other pending | Balance latency vs. batching |
| Delta age >200ms | **Must apply** immediately | Update staleness exceeds tolerance |
| Delta queue depth >10 | **Apply** oldest deltas | Queue backing up |
| Delta total size >100MB | **Apply** all | Risk of large bulk apply |

### 1.4 Rank-Cap Breach Threshold

**Trigger**: `artifact.rank > shard.rank_capacity`

**Actions** (in priority order):
1. Try **rebalance** to shard with larger capacity (if available)
2. Try **partial refit** to reduce rank below capacity
3. Try **snapshot rebuild** to shard cluster in different topology
4. **Fail** with explicit error; require manual intervention

### 1.5 Worker Crash Recovery Thresholds

| Time Since Last Heartbeat | Action |
|---|---|
| <5 seconds | Wait; transient network delay expected |
| 5–30 seconds | Alert; initiate crash recovery |
| >30 seconds | Assume crash; trigger full recovery |
| >300 seconds | Mark worker dead; evict from cluster |

---

## 2. Rebuild Mode Selection Decision Tree

```
┌─────────────────────────────────────────────────────────┐
│  Artifact becomes unavailable or stale                  │
└─────────────────────────────┬───────────────────────────┘
                              │
                    ┌─────────▼──────────┐
                    │ Is artifact LOST?  │
                    └─────┬──────────┬───┘
                          │          │
                         Yes        No
                          │          │
                ┌─────────▼─────┐   │
                │ Has REPLICA?  │   │
                └─┬──────────┬──┘   │
                  │Yes      No      │
         ┌────────▼─┐   ┌──▼────────▼─────────┐
         │ PATCH*   │   │ Has LINEAGE/SOURCE? │
         │Promote   │   └─┬──────────────┬────┘
         │replica   │     │Yes          No
         └──────────┘     │             │
                          │    ┌────────▼────────┐
                          │    │ FAIL & ALERT    │
                          │    │ Graph fallback  │
                          │    └─────────────────┘
                          │
                    ┌─────▼──────────────────────┐
                    │ Is PATCH applicable?       │
                    │ (small delta, base intact) │
                    └─────┬──────────────┬───────┘
                         Yes            No
                          │              │
              ┌───────────▼─┐   ┌───────▼──────────┐
              │ PATCH       │   │ Is PARTIAL      │
              │Apply delta  │   │ REFIT viable?   │
              │             │   │ (subset update) │
              └─────────────┘   └───┬──────────┬──┘
                                    │Yes      No
                              ┌─────▼──┐  ┌───▼────────────┐
                              │PARTIAL │  │ SNAPSHOT       │
                              │REFIT   │  │ REBUILD        │
                              │        │  │ (full rebuild) │
                              └────────┘  └────────────────┘

* PATCH used for promotion: copy replica to primary location
```

### 2.1 Rebuild Mode Decision Pseudocode

```python
def select_rebuild_mode(artifact, failure_reason):
    # 1. Check if artifact is completely lost
    if artifact.replica_count == 0:
        if has_lineage(artifact):
            return SNAPSHOT_REBUILD
        else:
            return FAIL_WITH_FALLBACK
    
    # 2. Check for corruption/mismatch
    if failure_reason == CORRUPTION:
        # Try to use clean replica
        clean_replicas = count_healthy_replicas(artifact)
        if clean_replicas > 0:
            return PATCH  # Promote from clean replica
        elif has_lineage(artifact):
            return SNAPSHOT_REBUILD
        else:
            return FAIL_WITH_FALLBACK
    
    # 3. Check for staleness
    if failure_reason == STALE:
        age = get_staleness_age(artifact)
        if age < 1_hour:
            return PATCH  # Refresh from exact source
        else:
            # Invalidate and lazy re-materialize
            return INVALIDATE
    
    # 4. Check for incompatibility (e.g., model-switch)
    if failure_reason == INCOMPATIBILITY:
        if has_lineage(artifact):
            return SNAPSHOT_REBUILD
        else:
            return FAIL_WITH_FALLBACK
    
    # 5. Check for partial factors/loss
    if failure_reason == PARTIAL_LOSS:
        factor_recovery_pct = count_available_factors(artifact) / total_factors
        if factor_recovery_pct > 0.7:
            return PARTIAL_REFIT  # Recover and recompute missing
        else:
            return SNAPSHOT_REBUILD
    
    # 6. Default to snapshot rebuild if available
    if has_lineage(artifact):
        return SNAPSHOT_REBUILD
    else:
        return FAIL_WITH_FALLBACK
```

---

## 3. Rebuild Priority Assignment

### 3.1 Priority Matrix

Based on artifact class and failure severity:

| Artifact Class | Single Loss | Multi-Loss | Corruption | Incompatibility |
|---|---|---|---|---|
| **Primary Tensor** | P1 (Critical) | P1 | P1 | P2 (High) |
| **Adapter** | P1 | P1 | P1 | P1 |
| **Shard Summary** | P2 | P2 | P2 | P2 |
| **Routing Tensor** | P2 | P2 | P2 | P3 (Medium) |
| **Derived Summary** | P3 | P3 | P3 | P3 |
| **Query Cache** | P4 (Low) | P4 | P4 | P4 |

### 3.2 Priority Escalation Rules

Escalate priority under these conditions:

| Condition | Escalate By | Example |
|---|---|---|
| Artifact has pending queries | +1 level | P2 → P1 |
| Multiple artifacts lost simultaneously | +1 level | P3 → P2 |
| Freshness debt > 20% | +2 levels | P4 → P2 |
| Critical path blocker | Max to P1 | Any → P1 |
| Long wait time (>10s) | +1 level | Escalate every 10s |

---

## 4. Rebuild Amplification Limits

### 4.1 Amplification Factor Targets

| Factor | Target Limit | Rationale |
|---|---|---|
| **Replication overhead** | 3–5× | Typical replication factor |
| **Network transfer** | 5–10× | Intra-shard < inter-shard cost |
| **Recomputation (CPU)** | 10–50× | CPU rebuild slower than copy |
| **Recomputation (GPU)** | 5–20× | GPU faster but resource-limited |
| **Lock contention delay** | 2–5× | Distributed lock overhead |
| **I/O amplification** | 2–3× | Cache effects |

**Total budget**: Product of applicable factors
- **Ideal case** (replica copy): ~1–2× (replication only)
- **Typical case** (network + CPU): ~50–100× (5 network × 10 CPU)
- **Worst case** (slow lineage + recompute): ~500–1000× (10 network × 50 CPU)

### 4.2 Amplification-Based Mode Selection

If estimated amplification exceeds limits:

```python
def should_defer_rebuild(artifact, mode):
    estimated_amp = estimate_amplification(artifact, mode)
    
    if estimated_amp > MAX_AMPLIFICATION:
        # Defer or downgrade
        if mode == SNAPSHOT_REBUILD:
            # Try PATCH instead
            return should_defer_rebuild(artifact, PATCH)
        elif mode == PARTIAL_REFIT:
            # Try PATCH instead
            return should_defer_rebuild(artifact, PATCH)
        else:
            # PATCH or INVALIDATE; accept
            return False
    
    return True
```

---

## 5. Batch Rebuild Policies

### 5.1 Batch Sizing

When multiple artifacts need rebuild, batch them to reduce overhead:

| Artifact Size | Batch Size | Rationale |
|---|---|---|
| <10MB | 100–500 | Small artifacts; batch aggressively |
| 10–100MB | 20–100 | Medium; reasonable batch |
| 100MB–1GB | 5–20 | Large; smaller batches to avoid overload |
| >1GB | 1–5 | Very large; mostly sequential |

### 5.2 Batch Scheduling

**Phase 1** (first 30 seconds):
- Rebuild P1 critical artifacts (max batch size)

**Phase 2** (30 seconds–5 minutes):
- Rebuild P2 high-priority artifacts

**Phase 3** (5–60 minutes):
- Rebuild P3 medium-priority (background)

**Phase 4** (background, continuous):
- Rebuild P4 low-priority (opportunistic)

### 5.3 Batch Coordination

```python
def schedule_batch_rebuild(artifacts_to_rebuild):
    # Sort by priority
    artifacts_to_rebuild.sort(key=lambda a: a.priority)
    
    # Group into batches
    batches = []
    for priority_level in [P1, P2, P3, P4]:
        artifacts_at_level = [a for a in artifacts_to_rebuild
                             if a.priority == priority_level]
        
        # Determine batch size
        batch_size = get_batch_size(max_artifact_size(artifacts_at_level))
        
        # Create batches
        for batch in chunked(artifacts_at_level, batch_size):
            batches.append({
                'priority': priority_level,
                'artifacts': batch,
                'phase': get_phase_for_priority(priority_level),
            })
    
    return batches
```

---

## 6. Freshness Debt Mitigation Strategies

### 6.1 Strategies (in Priority Order)

| Strategy | Trigger | Cost | Benefit |
|---|---|---|---|
| **Batch patch** | Debt 5–10% | Low | Reduce latency tail; keep fresh |
| **Lazy invalidate** | Debt 10–20% | Low | Deferred cost; triggers refresh |
| **Force snapshot** | Debt >20% | High | Immediate freshness guarantee |
| **Scale out** | Debt persistent >20% | Very High | Operator action; increase capacity |

### 6.2 Decision Tree

```python
def mitigate_freshness_debt(debt_ratio):
    if debt_ratio < 0.05:
        return  # Green; no action
    
    if debt_ratio < 0.10:
        # Yellow: batch patches for oldest stale artifacts
        trigger_batch_patch(oldest_n=5)
    
    elif debt_ratio < 0.20:
        # Yellow-Red: invalidate older stale artifacts
        invalidate_artifacts(staleness_threshold=1_hour)
    
    else:  # debt_ratio >= 0.20:
        # Red: force snapshot rebuild
        force_snapshot_rebuild_all_stale()
        alert(severity=ERROR, msg="Freshness debt critical")
        
        # If still high, escalate to operator
        if debt_ratio > 0.50:
            alert(severity=CRITICAL, msg="Tensor infrastructure scaling needed")
```

---

## 7. Operational Runbooks

### 7.1 Runbook: Responding to Yellow Threshold

**Condition**: Freshness debt 5–20%

**Steps**:
1. Identify stale artifacts: `metrics.get_stale_artifacts()`
2. Log summary: count, total staleness age, impact weights
3. Trigger batch refresh: `recovery_mgr.batch_refresh_stale(max_age=1hr)`
4. Monitor debt ratio; escalate if crossing to red
5. No operator intervention needed

**Expected outcome**: Debt returns to green within 10 minutes

### 7.2 Runbook: Responding to Red Threshold

**Condition**: Freshness debt >20%

**Steps**:
1. **ALERT**: Page on-call operator
2. Dump diagnostics:
   - Stale artifacts and ages
   - Queued updates and delays
   - Replica distribution
   - Worker health status
3. Execute immediate mitigation:
   ```
   recovery_mgr.force_snapshot_rebuild_all_stale()
   ```
4. Monitor during rebuild:
   - Rebuild latency and progress
   - Worker resource utilization
   - Debt ratio trend
5. If rebuild doesn't complete in 300 seconds:
   - Check worker health
   - Look for network issues
   - Consider manual shard rebalance
6. Post-incident: Review why debt accumulated; adjust thresholds or capacity

### 7.3 Runbook: Worker Crash Recovery

**Condition**: Worker down >30 seconds

**Steps**:
1. **Detection**: Heartbeat timeout triggers alert
2. **Identification**: Determine which artifacts were on crashed worker
3. **Recovery procedure**:
   ```
   recovery_mgr.crash_recover(worker_id, wal_snapshot)
   ```
4. **Monitoring**:
   - Read WAL; identify pending operations
   - Classify operations (update, rebuild, GC)
   - Resume or rollback each category
   - Monitor progress; should complete <10s
5. **Verification**:
   - All artifacts should return to AVAILABLE or DEGRADED
   - Manifest versions consistent across shards
   - Replicas synchronized
6. **If recovery fails**:
   - Mark worker UNHEALTHY
   - Promote replicas from healthy workers
   - Trigger background rebuild for lost data

---

## 8. Operational Metrics and Monitoring

### 8.1 Key Metrics to Track

```prometheus
# Freshness metrics
tensor_freshness_debt_ratio          # Current debt ratio (0.0–1.0)
tensor_staleness_age_seconds_count   # Histogram of stale artifact ages
tensor_stale_artifacts_total         # Count of stale artifacts

# Rebuild metrics
tensor_rebuild_latency_seconds       # p50, p99, p999
tensor_rebuild_ops_total             # Total rebuild operations
tensor_rebuild_mode_total            # By mode: patch, refit, snapshot, invalidate
tensor_rebuild_amplification_ratio   # Actual vs. theoretical cost

# Recovery metrics
tensor_recovery_success_ratio        # Successful recoveries / total attempts
tensor_recovery_failures_total       # Failed recovery operations
tensor_crash_recoveries_total        # Crash recovery operations executed

# Capacity metrics
tensor_total_size_bytes              # Sum of all tensor artifacts
tensor_replication_overhead_ratio    # Actual storage / ideal
tensor_rank_cap_breaches_total       # Times rank exceeded shard capacity
```

### 8.2 Alert Rules

```yaml
groups:
  - name: tensor_recovery_alerts
    rules:
      - alert: TensorFreshnessDeptRed
        expr: tensor_freshness_debt_ratio > 0.20
        for: 5m
        annotations:
          summary: "Tensor freshness debt critical"
          action: "Execute recovery_mgr.force_snapshot_rebuild_all_stale()"
      
      - alert: TensorRebuildLatencyHigh
        expr: tensor_rebuild_latency_seconds{quantile="0.99"} > 300
        for: 10m
        annotations:
          summary: "Tensor rebuild p99 latency > 5 minutes"
          action: "Check shard cluster health, network latency"
      
      - alert: TensorRecoveryFailureRate
        expr: rate(tensor_recovery_failures_total[5m]) > 0.01
        for: 10m
        annotations:
          summary: "Tensor recovery failure rate > 1%"
          action: "Investigate recovery_manager logs"
```

---

## 9. Configuration Template

```yaml
# tensor_recovery_config.yaml

recovery_manager:
  # Staleness thresholds (seconds)
  staleness_thresholds:
    primary_tensor:
      yellow: 3600      # 1 hour
      red: 86400        # 24 hours
    
    shard_summary:
      yellow: 300       # 5 minutes
      red: 1800         # 30 minutes
  
  # Delta application
  delta_batching:
    max_age_ms: 200
    max_pending: 10
    max_size_bytes: 104857600  # 100 MB
  
  # Freshness debt
  freshness_debt:
    yellow_threshold: 0.05     # 5%
    red_threshold: 0.20        # 20%
    max_capacity_units: 1000
  
  # Rebuild
  rebuild:
    max_replication_factor: 3
    max_network_bandwidth_pct: 0.30
    batch_size_small: 100
    batch_size_large: 10
  
  # Worker crash recovery
  worker_crash:
    heartbeat_timeout_sec: 30
    wal_replay_timeout_sec: 60
    max_pending_ops: 10000

  # Rebuild scheduling
  rebuild_phases:
    p1_timeout_sec: 30
    p2_timeout_sec: 300
    p3_timeout_sec: 3600
```

---

## References

- `EPIC3_RECOVERY_STRATEGY.md` (High-level policy)
- `TENSOR_RECOVERY_PROCEDURES.md` (Implementation guidance)
- `TENSOR_RECOVERY_TEST_STRATEGY.md` (Test definitions)
- `DISTRIBUTED_TENSOR_SHARDING.md`
- `HARDWARE_REQUIREMENTS.md`
