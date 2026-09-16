# RUNBOOK: Temporal Store — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Temporal Data Platform Team Lead
**Purpose:** Triage and recover from temporal snapshot failures, retention violations, CDC lag, and conflict storms
**Severity:** High (temporal store failures degrade bitemporal querying, history audits, and CDC-dependent pipelines)
**Estimated Duration:** 5 min – 3 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB temporal store module (`src/temporal/`). The module has five primary surfaces:

1. **Temporal query engine** — AS-OF, BETWEEN, system-versioned, bitemporal queries (`TemporalQueryEngine`)
2. **Snapshot manager** — epoch-based snapshot create/verify/restore (`SnapshotManager`)
3. **Retention manager** — retention policy enforcement, TTL sweep, tombstone creation (`RetentionManager`)
4. **CDC pipeline** — change-data capture event emission, lag tracking, ordering guarantees (`TemporalCdcPipeline`)
5. **Conflict resolver** — last-write-wins and application-defined resolution strategies (`TemporalConflictResolver`)

**Key Principles:**
- Snapshot creation is fail-closed; a failed snapshot does not corrupt the live data path
- Retention sweeps are bounded; a retention scan that exceeds its time budget is interrupted and rescheduled
- CDC events are strictly ordered per entity; out-of-order delivery is rejected, not silently reordered
- Conflict resolution is deterministic (LWW by default); custom resolvers must be idempotent

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Access to server logs with `[TEMPORAL:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing `temporal_*` metrics
- [ ] Knowledge of active retention policies and CDC consumer configurations
- [ ] Snapshot storage backend reachable and healthy

---

## Failure Scenarios

---

### Scenario 1: Snapshot Failure

**Symptoms:**
- Log pattern: `[TEMPORAL:SnapshotFailed] epoch=<N> reason=<reason>`
- Snapshot gap in `temporal_snapshot_epochs` metric
- Recovery from snapshot will be required for point-in-time restore

**Log patterns:**
```
[TEMPORAL:SnapshotFailed] epoch=1700000000 reason=storage_write_error
[TEMPORAL:SnapshotFailed] epoch=1700000000 reason=checksum_mismatch expected=<h> actual=<h>
[TEMPORAL:SnapshotFailed] epoch=1700000000 reason=timeout elapsed_ms=30500
```

#### Step 1: Identify Snapshot State
```bash
grep '\[TEMPORAL:SnapshotFailed\]' /var/log/themisdb/themisdb.log | tail -20
themis-admin temporal snapshot-list --last 10
```

#### Step 2: Retry Failed Snapshot
```bash
themis-admin temporal snapshot-create --epoch <epoch>
# Verify: log should show [TEMPORAL:SnapshotCreated] within 60 s
```

#### Step 3: Validate Snapshot Integrity
```bash
themis-admin temporal snapshot-verify --epoch <epoch>
# On checksum mismatch, delete and re-create:
themis-admin temporal snapshot-delete --epoch <epoch>
themis-admin temporal snapshot-create --epoch <epoch>
```

---

### Scenario 2: Retention Violation

**Symptoms:**
- Log pattern: `[TEMPORAL:RetentionViolation] policy=<name> entity_id=<id> age_days=<N>`
- Records beyond retention window still present in live query results
- `temporal_retention_sweep_lag_seconds` metric exceeding threshold

**Log patterns:**
```
[TEMPORAL:RetentionViolation] policy=30d entity_id=<id> age_days=45
[TEMPORAL:RetentionViolation] policy=<name> sweep_interrupted rows_remaining=<N>
```

#### Step 1: Check Retention Sweep State
```bash
grep '\[TEMPORAL:RetentionViolation\]' /var/log/themisdb/themisdb.log | tail -20
query-metrics --metric temporal_retention_sweep_lag_seconds --range 30m
```

#### Step 2: Trigger Manual Retention Sweep
```bash
themis-admin temporal retention-sweep --policy <name> --max-rows 100000
```

#### Step 3: Increase Sweep Budget
If sweep is consistently interrupted:
```bash
themis-admin config set temporal.retention.sweep_budget_ms 60000
themis-admin temporal reload-config
```

---

### Scenario 3: CDC Lag

**Symptoms:**
- Log pattern: `[TEMPORAL:CDCLag] pipeline=<name> lag_ms=<ms> threshold_ms=<T>`
- CDC consumers receiving events behind real-time by more than SLO threshold
- `temporal_cdc_lag_ms` metric exceeding SLO threshold

**Log patterns:**
```
[TEMPORAL:CDCLag] pipeline=<name> lag_ms=1500 threshold_ms=500
[TEMPORAL:CDCLag] pipeline=<name> reason=consumer_backpressure queue_depth=<N>
```

#### Step 1: Check CDC Pipeline State
```bash
grep '\[TEMPORAL:CDCLag\]' /var/log/themisdb/themisdb.log | tail -20
query-metrics --metric temporal_cdc_lag_ms --label pipeline --range 10m
```

#### Step 2: Scale CDC Consumers
```bash
themis-admin cdc add-consumer --pipeline <name> --count 2
```

#### Step 3: Throttle Write Rate (Temporary)
If consumer backpressure is from a write burst:
```bash
themis-admin config set temporal.cdc.max_events_per_batch 100
themis-admin temporal reload-config
```

---

### Scenario 4: Conflict Storm

**Symptoms:**
- Log pattern: `[TEMPORAL:ConflictStorm] entity_id=<id> conflicts_per_sec=<N>`
- High rate of concurrent bitemporal writes to the same entity
- `temporal_conflict_resolutions_total` counter very high; query latency increasing

**Log patterns:**
```
[TEMPORAL:ConflictStorm] entity_id=<id> conflicts_per_sec=500 threshold=100
[TEMPORAL:ConflictStorm] entity_id=<id> reason=hot_partition write_threads=<N>
```

#### Step 1: Identify Hot Entities
```bash
grep '\[TEMPORAL:ConflictStorm\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'entity_id=\S+' | sort | uniq -c | sort -rn | head -10
```

#### Step 2: Apply Write Serialization for Hot Entities
```bash
themis-admin temporal set-write-serialization --entity-id <id> --mode serialize
```

#### Step 3: Review Upstream Write Producers
If conflict storm is from an application-level bug (e.g., duplicate event replay):
```bash
# Identify top write producers for the hot entity
themis-admin temporal write-producers --entity-id <id> --top 5
# Throttle or pause the offending producer
themis-admin temporal pause-producer --producer-id <id>
```

---

### Scenario 5: History Query Degradation

**Symptoms:**
- Log pattern: `[TEMPORAL:QueryDegradation] query_type=<type> p99_ms=<ms> threshold_ms=<T>`
- AS-OF or BETWEEN queries taking longer than SLO
- `temporal_query_p99_ms` metric above threshold; index cache miss rate rising

**Log patterns:**
```
[TEMPORAL:QueryDegradation] query_type=AS_OF p99_ms=1500 threshold_ms=100
[TEMPORAL:QueryDegradation] query_type=BETWEEN reason=index_cache_miss rate=<pct>
```

#### Step 1: Check Query Cache State
```bash
query-metrics --metric temporal_index_cache_hit_rate --range 15m
grep '\[TEMPORAL:QueryDegradation\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Rebuild Temporal Index Cache
```bash
themis-admin temporal index-cache-rebuild --async
# Monitor: temporal_index_cache_hit_rate should recover within 2 min
```

#### Step 3: Increase Cache Size
```bash
themis-admin config set temporal.index_cache.max_entries 1000000
themis-admin temporal reload-config
```

---

## Alert → Runbook Mapping

| Alert Name | Log Pattern | Runbook Scenario |
|------------|-------------|------------------|
| `temporal_snapshot_failed` | `[TEMPORAL:SnapshotFailed]` | Scenario 1 |
| `temporal_retention_violation` | `[TEMPORAL:RetentionViolation]` | Scenario 2 |
| `temporal_cdc_lag_high` | `[TEMPORAL:CDCLag]` | Scenario 3 |
| `temporal_conflict_storm` | `[TEMPORAL:ConflictStorm]` | Scenario 4 |
| `temporal_query_degradation` | `[TEMPORAL:QueryDegradation]` | Scenario 5 |

---

## Escalation Path

1. **L1 (Operator):** Apply runbook steps; resolve within 30 min
2. **L2 (SRE):** Escalate if snapshot failure persists or CDC lag > SLO for 15 min
3. **L3 (Temporal Data Platform):** Engage for conflict-resolution strategy changes or retention policy redesign

---

## Related Documentation

- `src/temporal/ROADMAP.md` — Wave D operability items
- `include/temporal/temporal_api_contract.h` — Frozen v1.x query contract
- `tests/integration/test_temporal_store_soak.cpp` — Wave D soak tests
- `tests/temporal/test_temporal_highcardinality_stress.cpp` — Wave D stress tests
- `benchmarks/temporal/bench_temporal_dedicated_gates.cpp` — Dedicated performance gates
