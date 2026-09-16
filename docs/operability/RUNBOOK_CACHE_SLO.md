# RUNBOOK: Cache SLO — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Cache/Infrastructure Team Lead
**Purpose:** Triage and recover from cache SLO breaches, coordinator degradation, tenant isolation incidents, replication lag, and LRU memory pressure
**Severity:** High (cache failures degrade query latency and throughput; fail-closed semantics prevent data inconsistency)
**Estimated Duration:** 5 min - 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB cache pipeline (`src/cache/`). The cache pipeline has five primary failure surfaces:

1. **SLO breach** — hit-rate or p99 latency falls outside the configured threshold (`CacheHitRateSloMonitor`)
2. **Distributed coordinator degradation** — quorum loss, partial-backend failure, or Redis/gRPC peer unavailability
3. **Tenant isolation incident** — cross-tenant key collision or per-tenant quota breach
4. **Replication lag incident** — p99 replication delivery > 500 ms, fan-out worker stall
5. **LRU memory pressure** — OOM risk from eviction rate spike or unbounded entry growth

**Key Principles:**
- Cache failures are fail-closed: degraded backends deny operations rather than silently returning stale data
- Tenant namespaces are strictly isolated; a `TenantViolation` class means cross-tenant access was attempted
- Circuit breakers protect coordinator paths; operators must manually reset after root cause is resolved
- Wave D D1 trace spans are emitted for all SLO breach and coordinator events (see `[CACHE:*]` log tags)

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Direct query execution is operational (cache degradation is not query-blocking)
- [ ] Access to server logs with `[CACHE:*]` tag filtering (e.g. `grep '\[CACHE:' /var/log/themisdb/themisdb.log`)
- [ ] Prometheus/Grafana dashboard showing cache metrics (`cache_hit_rate`, `cache_eviction_total`, `cache_replication_lag_p99`)
- [ ] Knowledge of which coordinator backend is configured (`Redis`, `gRPC`, `LocalBus`)
- [ ] Circuit breaker state accessible via admin API or log scan

---

## Failure Scenarios

---

### Scenario 1: Cache SLO Breach (Hit-Rate < Threshold or Eviction Spike)

**Symptoms:**
- Logs contain `[CACHE:SLOBreach]` structured tag or `slo_breach` JSON event
- Hit-rate metric `cache_hit_rate` drops below configured warning (default 0.60) or critical (default 0.40) threshold
- Eviction rate spike observable in `cache_eviction_total` counter

**Log patterns to search for:**
```
[CACHE:SLOBreach] event=slo_breach slo_threshold=0.6000 actual_hit_rate=0.4321 level=WARNING
{"event":"slo_breach","slo_threshold":0.6000,"actual_hit_rate":0.4321,"level":"WARNING","cache_name":"adaptive_query_cache"}
Cache hit rate SLO violation [WARNING]: hit_rate=0.432 (threshold=0.600) total_requests=5000
```

#### Step 1: Confirm SLO Breach and Level

```bash
# Search for structured SLO breach events
grep '\[CACHE:SLOBreach\]\|"event":"slo_breach"' /var/log/themisdb/themisdb.log | tail -20

# Check current hit-rate from admin API
themisdb-admin cache slo-status --json | jq '.hit_rate, .violation_level'
```

Expected output when breached:
```json
{ "hit_rate": 0.432, "violation_level": "WARNING", "total_requests": 5000 }
```

#### Step 2: Diagnose Root Cause

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| Hit-rate drops suddenly | Cache capacity too small / working set grew | Increase `l1_max_entries` / `l2_max_entries` config |
| Hit-rate drops after deployment | New query patterns miss cache fingerprints | Check warm-up logs; trigger manual warmup |
| Eviction spike with no traffic increase | Memory pressure from other tenants | Check per-tenant quota; enable tenant isolation |
| Hit-rate low but latency OK | Cache bypassed (circuit breaker open) | See Scenario 2 |

#### Step 3: Increase Cache Capacity (Temporary)

```bash
# View current capacity config
themisdb-admin config get cache.l1_max_entries cache.l2_max_entries

# Increase L1 capacity temporarily
themisdb-admin config set cache.l1_max_entries 2048 --apply-live

# Trigger manual warmup for recently evicted keys
themisdb-admin cache warmup --tenant-id <tenant_id> --recent-queries 1000
```

#### Step 4: Validate Recovery

```bash
# Confirm hit-rate recovers
grep '"event":"slo_breach"' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5

# Check that SLO violation resolves
themisdb-admin cache slo-status --json | jq '.violation_level'
```

**Decision Point:**
- ✅ **Recovery confirmed:** Hit-rate ≥ warning threshold, no new `slo_breach` events → incident closed
- ⚠ **Hit-rate still low:** Check warm-up backlog; consider promoting recent cache misses manually
- ❌ **Hit-rate critical and sustained:** Escalate; check coordinator connectivity (Scenario 2)

---

### Scenario 2: Distributed Coordinator Degradation (Quorum Loss, Partial-Backend Failure)

**Symptoms:**
- Logs contain `[CACHE:CoordinatorDegraded]` or `DegradedBackend` failure class
- Coordinator transitions to `OPEN` circuit breaker state
- Cache operations returning `DegradedBackend` failure class (fail-closed semantics active)

**Log patterns:**
```
[CACHE:CoordinatorDegraded] backend=redis state=OPEN consecutive_failures=5
[CACHE:CoordinatorDegraded] backend=grpc peer=node-2:50051 unreachable timeout=30s
Cache hit rate SLO violation [CRITICAL]: hit_rate=0.210 (threshold=0.400) total_requests=8000
```

#### Step 1: Identify Degraded Backend

```bash
# Scan for coordinator degradation events
grep '\[CACHE:CoordinatorDegraded\]\|DegradedBackend' /var/log/themisdb/themisdb.log | tail -20

# Check circuit breaker state per backend
themisdb-admin cache coordinator-status --json | jq '.backends[] | {name, state, consecutive_failures}'
```

#### Step 2: Diagnose by Backend Type

| Backend | Symptom | Resolution |
|---------|---------|-----------|
| Redis | `ECONNREFUSED` on publish | Verify Redis process is running; check `redis-cli ping` |
| gRPC peer | Peer unreachable, all requests timeout | Check peer node health; verify TLS cert validity |
| LocalBus | Never degrades (always available) | If LocalBus fails, it is an in-process error — check memory |

#### Step 3: Reset Circuit Breaker

```bash
# After root cause is resolved, probe the coordinator
themisdb-admin cache coordinator-probe --backend redis --timeout 5s

# If probe succeeds, reset circuit breaker
themisdb-admin cache circuit-breaker-reset --backend redis

# Verify transition to CLOSED
grep 'circuit_breaker.*CLOSED\|coordinator.*CLOSED' /var/log/themisdb/themisdb.log | tail -5
```

#### Step 4: Partial-Backend Failure (some peers up, some down)

When `PartialDelivery` appears in logs (not `DegradedBackend`), some peers are healthy:

```bash
# Identify which peers failed
grep '"event":"replication_partial"\|PartialDelivery' /var/log/themisdb/themisdb.log | \
  jq -r '.failed_peers[]?' 2>/dev/null || \
  grep 'peer.*failed\|peer.*timeout' /var/log/themisdb/themisdb.log | tail -20

# Remove unhealthy peer from coordinator config (temporary)
themisdb-admin cache coordinator-remove-peer --peer node-2:50051

# Re-add after repair
themisdb-admin cache coordinator-add-peer --peer node-2:50051
```

**Decision Point:**
- ✅ **All backends healthy, breaker CLOSED:** Incident resolved → close
- ⚠ **One peer still down:** Continue with reduced quorum; file maintenance ticket
- ❌ **All backends down:** Cache fully degraded — query latency will degrade; escalate immediately

---

### Scenario 3: Tenant Isolation Incident (Cross-Tenant Key Collision, Quota Breach)

**Symptoms:**
- Logs contain `[CACHE:TenantViolation]` or `TenantViolation` failure class
- Logs contain `[CACHE:TenantQuotaBreach]` when per-tenant eviction rate exceeds threshold
- Audit trail shows access to keys outside tenant namespace

**Log patterns:**
```
[CACHE:TenantViolation] attempted_tenant=tenant_a actual_tenant=tenant_b key=<fingerprint>
[CACHE:TenantQuotaBreach] tenant_id=tenant_a eviction_rate=0.85 threshold=0.50 cache_name=adaptive_query_cache
```

#### Step 1: Confirm Isolation Event Type

```bash
# Check for TenantViolation events (security-relevant — alert immediately)
grep '\[CACHE:TenantViolation\]' /var/log/themisdb/themisdb.log | tail -20

# Check for quota breach events (operational — usually capacity-related)
grep '\[CACHE:TenantQuotaBreach\]' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Action by Event Type

**TenantViolation (cross-tenant key collision):**
1. Capture the `attempted_tenant`, `actual_tenant`, and `key` from the log line
2. Check if `enable_tenant_isolation` is enabled in cache config
3. If isolation is disabled: enable it immediately (`themisdb-admin config set cache.enable_tenant_isolation true`)
4. If isolation is enabled: this indicates a key-fingerprint collision — review tenant key-generation logic
5. File a security incident; notify the affected tenant

```bash
# Confirm tenant isolation is enabled
themisdb-admin config get cache.enable_tenant_isolation

# Enable if not already set
themisdb-admin config set cache.enable_tenant_isolation true --apply-live
```

**TenantQuotaBreach (per-tenant eviction rate exceeded):**

```bash
# View per-tenant eviction stats
themisdb-admin cache tenant-stats --tenant-id <tenant_id> --json | \
  jq '.eviction_rate, .quota_threshold'

# Increase tenant quota
themisdb-admin config set cache.tenant_eviction_rate_threshold 0.80 --apply-live

# Or flush and re-warm tenant cache to reduce eviction pressure
themisdb-admin cache flush --tenant-id <tenant_id>
themisdb-admin cache warmup --tenant-id <tenant_id> --recent-queries 500
```

**Decision Point:**
- ✅ **No TenantViolation events post-fix:** Isolation is working → close
- ⚠ **Quota breach recurring:** Increase per-tenant capacity or implement rate limiting
- ❌ **Cross-tenant key collision confirmed:** Escalate as security incident; audit all tenant namespaces

---

### Scenario 4: Replication Lag Incident (p99 > 500 ms, Fan-out Worker Stall)

**Symptoms:**
- Logs contain `[CACHE:ReplicationLag]` with p99 > 500 ms
- Fan-out worker queue depth growing (visible in metrics: `cache_replication_queue_depth`)
- Some peers not acknowledging invalidation events within `kMaxReplicationDeliveryMs` (30 000 ms)

**Log patterns:**
```
[CACHE:ReplicationLag] p99_us=612000 threshold_us=500000 peer=node-3:50051
[CACHE:ReplicationStall] fan_out_worker=stalled queue_depth=4500 worker_wake_interval=500ms
{"event":"latency_slo_breach","p99_ms":612.0,"threshold_ms":500.0,"level":"WARNING"}
```

#### Step 1: Confirm Lag and Identify Stalled Peers

```bash
# Check replication lag events
grep '\[CACHE:ReplicationLag\]\|\[CACHE:ReplicationStall\]' /var/log/themisdb/themisdb.log | tail -20

# Check current queue depth and worker state
themisdb-admin cache replication-status --json | \
  jq '.fan_out_workers[] | {id, state, queue_depth, last_wake_ms}'
```

#### Step 2: Diagnose Stall Type

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| Queue depth rising, worker wake interval at max | Fan-out worker blocked on slow peer | Check peer network latency; consider removing peer temporarily |
| p99 spike but queue small | Individual slow peer | Check `kMaxReplicationDeliveryMs`; reduce per-peer timeout |
| All peers slow simultaneously | Network congestion or broadcast storm | Check network metrics; reduce replication fan-out batch size |
| Worker completely stalled | Deadlock in fan-out coordinator | Restart cache replication worker via admin API |

#### Step 3: Restart Stalled Fan-out Worker

```bash
# Restart replication fan-out worker (non-destructive; in-flight events are replayed)
themisdb-admin cache replication-worker-restart

# Verify queue drains
watch -n 2 'themisdb-admin cache replication-status --json | jq ".fan_out_workers[].queue_depth"'
```

#### Step 4: Validate Recovery

```bash
# Confirm p99 drops below threshold
grep '"event":"latency_slo_breach"' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -5

# Confirm all peers are acknowledging
themisdb-admin cache replication-status --json | jq '.peers[] | {id, healthy, last_ack_ms}'
```

**Decision Point:**
- ✅ **p99 < 500 ms, all peers healthy:** Incident resolved → close
- ⚠ **One peer consistently slow:** Remove from fan-out temporarily; file maintenance ticket
- ❌ **All peers stalled, queue unbounded:** Emergency stop of replication; escalate to on-call infrastructure

---

### Scenario 5: LRU Memory Pressure (OOM Risk, Eviction Rate Spike)

**Symptoms:**
- `cache_eviction_total` rate rising sharply
- System memory approaching limit (check `free -m` or container memory metrics)
- `[CACHE:SLOBreach]` firing due to hit-rate drop caused by aggressive eviction

**Log patterns:**
```
[CACHE:EvictionSpike] eviction_rate=950/s baseline=50/s threshold=500/s
Cache hit rate SLO violation [WARNING]: hit_rate=0.55 (threshold=0.60)
```

#### Step 1: Confirm Memory Pressure

```bash
# Check system memory
free -m

# Check cache memory footprint
themisdb-admin cache memory-usage --json | \
  jq '.l1_bytes_used, .l2_bytes_used, .total_bytes_used, .capacity_percent'

# Check eviction rate
grep 'EvictionSpike\|eviction_rate' /var/log/themisdb/themisdb.log | tail -10
```

#### Step 2: Immediate Mitigation

```bash
# Reduce L1 max entry size to free memory immediately
themisdb-admin config set cache.l1_max_entry_size 32768 --apply-live  # 32 KiB

# Flush cold entries (L3 tier only, if enabled)
themisdb-admin cache flush-tier --tier l3

# Check if max_total_entry_size is respected
themisdb-admin config get cache.max_total_entry_size
```

#### Step 3: Identify Large Entries

```bash
# Find oversized entries consuming disproportionate memory
themisdb-admin cache entry-stats --sort-by-size --limit 20 --json | \
  jq '.entries[] | {key_prefix: .key[0:32], size_bytes, tenant_id, tier}'
```

#### Step 4: Adjust Eviction Thresholds

```bash
# Lower trigger threshold to start eviction earlier (default 70%)
themisdb-admin config set cache.eviction_trigger_threshold_percent 60 --apply-live

# Increase safe threshold to be more aggressive about freeing memory
themisdb-admin config set cache.eviction_safe_threshold_percent 40 --apply-live

# Verify thresholds applied
themisdb-admin cache eviction-status --json | \
  jq '.trigger_threshold_percent, .safe_threshold_percent'
```

**Decision Point:**
- ✅ **Eviction rate normalizes, memory stable:** Incident resolved → adjust permanent capacity config
- ⚠ **Eviction rate still high but memory safe:** Monitor; schedule capacity planning review
- ❌ **OOM imminent (< 5% free):** Emergency: flush all cache tiers; restart service if necessary

---

## Diagnostic Commands Reference

### Log Tag Patterns (`[CACHE:*]`)

| Tag | Component | Severity |
|-----|-----------|---------|
| `[CACHE:SLOBreach]` | `CacheHitRateSloMonitor` | WARNING/CRITICAL |
| `[CACHE:TenantQuotaBreach]` | `CacheHitRateSloMonitor` | WARNING |
| `[CACHE:TenantViolation]` | All tenant-aware paths | CRITICAL |
| `[CACHE:CoordinatorDegraded]` | `DistributedCacheCoordinator`, `RedisCacheCoordinator` | HIGH |
| `[CACHE:ReplicationLag]` | `CacheReplicationCoordinator` | WARNING |
| `[CACHE:ReplicationStall]` | `CacheReplicationCoordinator` | CRITICAL |
| `[CACHE:EvictionSpike]` | `WeightedTieredLRUEvictionPolicy`, `BoundedLRUCache` | WARNING |

### Quick Health Commands

```bash
# One-shot health check
themisdb-admin cache health --json | jq '{hit_rate, violation_level, coordinator_state, replication_lag_p99_ms}'

# All active SLO alerts
themisdb-admin cache slo-status --json | jq '.active_alerts'

# Tenant isolation summary
themisdb-admin cache tenant-summary --json | jq '.tenants[] | {id, eviction_rate, quota_percent}'

# Replication summary
themisdb-admin cache replication-status --json | jq '{queue_depth: .total_queue_depth, healthy_peers: .healthy_peer_count}'

# Memory usage
themisdb-admin cache memory-usage --json | jq '.capacity_percent'
```

### Wave D D1 Trace Span Cross-Links

All cache SLO and coordinator events emit Wave D D1 trace spans under the `themis.cache` service prefix:

| Span Name | Trigger | Linked Log Tag |
|-----------|---------|---------------|
| `cache.slo.breach` | Hit-rate below threshold | `[CACHE:SLOBreach]` |
| `cache.tenant.quota_breach` | Per-tenant eviction rate exceeded | `[CACHE:TenantQuotaBreach]` |
| `cache.coordinator.degraded` | Circuit breaker OPEN | `[CACHE:CoordinatorDegraded]` |
| `cache.replication.lag` | p99 > 500 ms | `[CACHE:ReplicationLag]` |
| `cache.eviction.spike` | Eviction rate above baseline | `[CACHE:EvictionSpike]` |

To correlate trace spans with logs:
```bash
# Extract correlation_id from log line and look up in Jaeger/Zipkin
grep '\[CACHE:SLOBreach\]' /var/log/themisdb/themisdb.log | \
  grep -o '"correlation_id":"[^"]*"' | head -5
```

---

## Escalation Path

| Level | Condition | Action |
|-------|-----------|--------|
| L1 (Operator) | SLO breach < 30 min, single backend | Follow scenarios above |
| L2 (Team Lead) | SLO breach > 30 min, or tenant isolation incident | Page cache team lead |
| L3 (On-call) | All backends down, OOM imminent, or security incident | Page on-call infrastructure |

---

## Related Runbooks

- `RUNBOOK_ACCESS_MODEL_PROMOTION.md` — AccessCoordinator storage-tier promotion/demotion
- `RUNBOOK_AQL_ASSISTANCE.md` — AQL assistance pipeline triage
- `benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md` — Benchmark execution and gate validation

---

## Document Control

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0.0 | 2026-09-16 | ThemisDB Contributors | Initial Wave D delivery |
