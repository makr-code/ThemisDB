# RUNBOOK: Analytics Pipeline — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Analytics Platform Team Lead
**Purpose:** Triage and recover from analytics pipeline failures, performance degradation, and resource exhaustion events
**Severity:** High (analytics pipeline failures degrade real-time and OLAP query capability; time-series data may be lost or delayed)
**Estimated Duration:** 5 min - 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB analytics pipeline (`src/analytics/`). The pipeline has six primary surfaces:

1. **Time-series ingestion** — metric ingestion → window accumulation → aggregation flush (`streaming_window.cpp`, `aggregation.cpp`)
2. **Columnar execution** — columnar scan, predicate pushdown, batch projection (`columnar_execution.cpp`)
3. **Aggregation pipeline** — SUM/COUNT/AVG/MIN/MAX result aggregation with overflow protection (`result_aggregator.cpp`)
4. **Query execution** — OLAP plan lookup, CEP pattern evaluation, distributed merge (`olap.cpp`, `cep_engine.cpp`)
5. **Window rollup** — tumbling/sliding/hopping/session window management with eviction (`streaming_window.cpp`)
6. **Metric cardinality** — high-cardinality label set management, eviction, and bounded registry (`streaming_window.cpp`, `distributed_analytics.cpp`)

**Key Principles:**
- The analytics pipeline is fail-closed; malformed inputs and unsupported operations return error codes, not silent silences
- Circuit breakers protect distributed analytics coordinator paths from cascading failures
- Streaming windows are bounded by runtime limits (`max_open_windows`, `max_records_per_window`, `max_records_per_session`); eviction is expected behavior under load
- Optional backends (Arrow, ONNX, TF Serving) degrade gracefully; their unavailability does not block the core analytics path

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Core database query execution is operational (analytics pipeline is a secondary tier)
- [ ] Access to server logs with `[ANALYTICS:*]`, `[STREAMING:*]`, `[COLUMNAR:*]`, `[AGGREGATION:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing analytics metrics (`analytics_*`, `streaming_window_*`, `columnar_scan_*`)
- [ ] Knowledge of which optional backends are configured (Arrow Flight, ONNX, TF Serving)
- [ ] Circuit breaker state accessible via admin API or log scan for `distributed_analytics`

---

## Failure Scenarios

---

### Scenario 1: Time-Series Ingestion Backlog

**Symptoms:**
- Logs contain `[STREAMING:WindowEviction]` at high rate
- `analytics_streaming_window_evictions_total` counter rising steeply
- Write latency for time-series metrics increasing; ingestion throughput dropping below 10 000 ops/sec
- `[STREAMING:BackPressure]` log tags visible (DROP or BLOCK mode active)

**Log patterns:**
```
[STREAMING:WindowEviction] window_id=<id> evicted=<N> reason=max_records_exceeded
[STREAMING:BackPressure] mode=DROP ingest_rate=<rate> queue_depth=<depth>
[ANALYTICS:IngestBacklog] pending_events=<N> lag_ms=<lag>
```

#### Step 1: Confirm Backlog State

```bash
# Check for eviction events in the last 5 minutes
grep '\[STREAMING:WindowEviction\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | tail -30

# Check backpressure mode (DROP = data loss; BLOCK = producer stall)
grep '\[STREAMING:BackPressure\]' /var/log/themisdb/themisdb.log | tail -10

# Check current ingestion throughput
query-metrics --metric analytics_ingest_ops_per_sec --range 30m --window 1m
```

Expected output when backlogged:
```
[STREAMING:BackPressure] mode=DROP ingest_rate=45000 queue_depth=8192
[STREAMING:WindowEviction] window_id=ts_metric_0042 evicted=512 reason=max_records_exceeded
```

#### Step 2: Diagnose Root Cause

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| Evictions + DROP mode | Ingest rate exceeds `max_records_per_window` | Increase `max_records_per_window` or scale ingest workers |
| BLOCK mode, producer stall | Downstream aggregation pipeline slow | Profile aggregation latency; check Scenario 3 |
| Backlog only during batch jobs | Scheduled ETL saturating ingest path | Throttle ETL jobs or stagger them off peak |
| Backlog growing monotonically | Aggregation flush not keeping pace | Check flush latency; verify window size is appropriate |

#### Step 3: Tune Runtime Limits (Temporary Mitigation)

```bash
# View current window limits
themisdb-admin config get analytics.streaming.max_records_per_window
themisdb-admin config get analytics.streaming.max_open_windows

# Increase max records per window (doubles memory per window — validate first)
themisdb-admin config set analytics.streaming.max_records_per_window 20000 --apply-live

# Switch from DROP to BLOCK to prevent data loss (if producers can tolerate backpressure)
themisdb-admin config set analytics.streaming.backpressure_mode BLOCK --apply-live
```

#### Step 4: Validate Recovery

```bash
# Confirm eviction rate returns to baseline (near zero under normal load)
query-metrics --metric analytics_streaming_window_evictions_total_rate --range 30m --window 1m

# Confirm ingestion throughput ≥ 10 000 ops/sec
query-metrics --metric analytics_ingest_ops_per_sec --range 10m --window 1m
```

**Decision Point:**
- ✅ **Eviction rate drops to baseline, throughput ≥ 10 000 ops/sec:** Incident resolved
- ⚠ **Eviction rate reduced but non-zero:** Acceptable if within agreed SLO — monitor for 15 min
- ❌ **Throughput still degraded:** Escalate to Analytics Platform team; consider reducing cardinality (Scenario 6)

---

### Scenario 2: Columnar Scan OOM (Out-of-Memory)

**Symptoms:**
- Process OOM kill or `std::bad_alloc` logged by the analytics worker
- `analytics_columnar_scan_batch_bytes` metric spiking before OOM
- `[COLUMNAR:OOMRisk]` log tags visible during large scan batches

**Log patterns:**
```
[COLUMNAR:OOMRisk] batch_size_bytes=<N> available_memory_bytes=<M> threshold_pct=90
[COLUMNAR:ScanAborted] reason=memory_limit_exceeded query_id=<id>
terminate called after throwing an instance of 'std::bad_alloc'
```

#### Step 1: Identify the Scan That Triggered OOM

```bash
# Find the query that preceded the OOM
grep '\[COLUMNAR:OOMRisk\]\|\[COLUMNAR:ScanAborted\]' /var/log/themisdb/themisdb.log | tail -20

# Check scan batch size at time of failure
grep 'batch_size_bytes' /var/log/themisdb/themisdb.log | tail -10

# Check available memory trend
query-metrics --metric process_resident_memory_bytes --range 1h --window 5m
```

#### Step 2: Apply Immediate Mitigation

```bash
# Reduce columnar scan batch size to limit peak memory allocation
themisdb-admin config set analytics.columnar.max_batch_size_rows 1024 --apply-live

# Enable memory-safe scan mode (adds bounds check before allocation)
themisdb-admin config set analytics.columnar.memory_safe_mode true --apply-live
```

#### Step 3: Identify the Offending Query

```bash
# Extract query IDs from COLUMNAR:ScanAborted events
grep '\[COLUMNAR:ScanAborted\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'query_id=\K[^\s]+' | sort | uniq -c | sort -rn | head -10

# Review the queries associated with high memory usage
themisdb-admin query-log show --query-id <id> --include-plan
```

#### Step 4: Long-term Remediation

1. Add a per-query memory quota in the query plan (`max_scan_memory_bytes` hint)
2. Add projection pushdown to reduce scan width before materialization
3. Validate fix with load test: `ctest -R analytics_highcardinality_stress --label-regex stress`

**Decision Point:**
- ✅ **OOM resolved, scan completes within memory budget:** Adjust limits permanently in config
- ❌ **OOM recurs on the same query:** Block the query class; review schema (large unindexed columns)

---

### Scenario 3: Aggregation Pipeline Failure

**Symptoms:**
- Aggregation results missing or returning `AGGREGATION_OVERFLOW` error codes
- `analytics_aggregation_overflow_total` counter rising
- `[AGGREGATION:Overflow]` log tags visible with specific accumulator details

**Log patterns:**
```
[AGGREGATION:Overflow] accumulator=<name> value=<v> running_sum=<s> overflows=<N>
[AGGREGATION:PartialResultDropped] shard_id=<id> reason=overflow_guard
[ANALYTICS:PipelineFailure] stage=aggregation error_code=AGGREGATION_OVERFLOW
```

#### Step 1: Identify the Overflow Source

```bash
# Find overflow events and the affected accumulators
grep '\[AGGREGATION:Overflow\]' /var/log/themisdb/themisdb.log | \
  grep -oP 'accumulator=\K[^\s]+' | sort | uniq -c | sort -rn | head -10

# Check if overflows are concentrated in one metric family
grep 'AGGREGATION_OVERFLOW' /var/log/themisdb/themisdb.log | tail -20
```

#### Step 2: Diagnose Overflow Class

| Pattern | Likely Cause | Action |
|---------|-------------|--------|
| SUM overflow on counter metrics | Raw counters exceeding int64 range | Switch to rate aggregation (`sum_rate`) instead of raw sum |
| Overflow only on specific metric | Metric cardinality explosion with large values | Apply per-metric value clamp (Scenario 6) |
| Overflow after schema change | New column with unbounded float range | Add `max_value` constraint to schema |
| Transient spikes | Burst of anomalous sensor data | Enable anomaly filter before aggregation pipeline |

#### Step 3: Apply Overflow Guard

```bash
# Enable overflow protection mode (converts overflow to saturating arithmetic)
themisdb-admin config set analytics.aggregation.overflow_mode SATURATE --apply-live

# Alternatively, switch affected metrics to rate-based aggregation
themisdb-admin analytics reconfigure-metric <metric_name> --aggregation sum_rate
```

#### Step 4: Verify Aggregation Pipeline Health

```bash
# Confirm overflow events stop
query-metrics --metric analytics_aggregation_overflow_total_rate --range 10m --window 1m

# Verify pipeline results are being produced
query-metrics --metric analytics_aggregation_result_flush_total_rate --range 10m --window 1m
```

**Decision Point:**
- ✅ **Overflow events stop, results produced:** Adjust aggregation type permanently
- ❌ **Overflow persists after configuration change:** Escalate; input data may violate schema assumptions

---

### Scenario 4: Query Timeout Spike

**Symptoms:**
- Analytics queries timing out; `analytics_query_timeout_total` rising
- `[ANALYTICS:QueryTimeout]` log tags with query_id and elapsed_ms
- Query p99 latency exceeding 1 ms gate (ARG-02 benchmark regression)

**Log patterns:**
```
[ANALYTICS:QueryTimeout] query_id=<id> elapsed_ms=<N> threshold_ms=<T>
[ANALYTICS:PlanCacheMiss] query_id=<id> hash=<hash>
[ANALYTICS:DistributedMerge] shard_id=<id> merge_latency_ms=<N> timeout=true
```

#### Step 1: Identify the Timeout Class

```bash
# Count timeout events by type
grep '\[ANALYTICS:QueryTimeout\]' /var/log/themisdb/themisdb.log | \
  awk '{print $1, $2}' | cut -d: -f1,2 | sort | uniq -c | tail -20

# Check plan cache miss rate (misses force re-planning = higher latency)
query-metrics --metric analytics_plan_cache_miss_rate --range 1h --window 5m

# Check distributed merge latency (cross-shard merges add significant latency)
grep '\[ANALYTICS:DistributedMerge\]' /var/log/themisdb/themisdb.log | \
  grep 'timeout=true' | tail -10
```

#### Step 2: Mitigations by Timeout Class

**Plan cache misses:**
```bash
# Increase plan cache size
themisdb-admin config set analytics.olap.plan_cache_size 4096 --apply-live

# Warm cache with common queries after restart
themisdb-admin analytics warm-plan-cache --top-queries 100
```

**Distributed merge timeouts:**
```bash
# Increase distributed merge timeout (temporary; root cause is shard latency)
themisdb-admin config set analytics.distributed.merge_timeout_ms 5000 --apply-live

# Check circuit breaker state for slow/failed shards
grep 'circuit_breaker\|OPEN\|HALF_OPEN' /var/log/themisdb/themisdb.log | tail -10
```

**Query complexity:**
```bash
# Review query plans for expensive operations
themisdb-admin analytics explain-query --query-id <id> --include-cost

# Apply query complexity limit to reject expensive queries early
themisdb-admin config set analytics.query.max_plan_cost 1000 --apply-live
```

#### Step 3: Verify Recovery Against p99 Gate

```bash
# Confirm query p99 latency returns to ≤ 1 ms (ARG-02 gate)
query-metrics --metric analytics_query_latency_p99_us --range 30m --window 1m
# Expected: ≤ 1000 µs
```

**Decision Point:**
- ✅ **p99 ≤ 1000 µs, no new timeout events:** Gate restored; incident closed
- ⚠ **p99 between 1–5 ms, timeout rate < 1%:** Monitor; acceptable degraded state
- ❌ **p99 > 5 ms or timeout rate > 5%:** Escalate to Analytics Platform team

---

### Scenario 5: Window Rollup Stall

**Symptoms:**
- Window flush count not increasing; data accumulating in windows without rollup
- `analytics_window_flush_total_rate` metric at zero or flat for > 5 min
- `[STREAMING:FlushStall]` log tags visible

**Log patterns:**
```
[STREAMING:FlushStall] window_id=<id> pending_events=<N> last_flush_ms_ago=<N>
[STREAMING:WindowFull] window_id=<id> size=<N> max=<max> flush_triggered=false
[ANALYTICS:PipelineStall] stage=window_rollup duration_s=<N>
```

#### Step 1: Identify Stalled Windows

```bash
# Find windows with the oldest last-flush timestamp
grep '\[STREAMING:FlushStall\]' /var/log/themisdb/themisdb.log | \
  sort -t= -k3 -rn | head -10

# Check if the window flush thread is alive
grep 'window.*flush.*thread\|FlushWorker' /var/log/themisdb/themisdb.log | tail -5
```

#### Step 2: Trigger Manual Flush

```bash
# Force flush all stalled windows
themisdb-admin analytics flush-windows --all --force

# Verify flush count increments
query-metrics --metric analytics_window_flush_total_rate --range 5m --window 1m
```

#### Step 3: Diagnose Stall Root Cause

| Symptom | Likely Cause | Action |
|---------|-------------|--------|
| Flush thread not running | Panic/crash in flush worker | Restart analytics subsystem |
| Flush triggered but not completing | Downstream aggregation blocked | Check Scenario 3 |
| Window count at `max_open_windows` | New windows blocked by limit | Increase `max_open_windows` or flush oldest |
| Time-based flush not triggering | Wall-clock drift in container | Verify system clock; check NTP sync |

#### Step 4: Verify Rollup Resumes

```bash
# Confirm flush rate returns to normal
query-metrics --metric analytics_window_flush_total_rate --range 10m --window 1m

# Confirm window count is decreasing (stalled windows being flushed)
query-metrics --metric analytics_open_windows_count --range 10m --window 1m
```

**Decision Point:**
- ✅ **Flush rate normal, open window count stabilizes:** Incident resolved
- ❌ **Flush worker crashed and not restarting:** Restart analytics subsystem; file bug report

---

### Scenario 6: Metric Cardinality Explosion

**Symptoms:**
- `analytics_distinct_metric_names_count` counter growing without bound
- Memory pressure from metric registry; possible OOM (see Scenario 2)
- `[ANALYTICS:CardinalityExplosion]` log tags with cardinality counts
- New metric names appearing from unexpected label permutations

**Log patterns:**
```
[ANALYTICS:CardinalityExplosion] registry_size=<N> threshold=<T> growth_rate=<R>/min
[ANALYTICS:MetricEvicted] metric_name=<name> reason=cardinality_limit_exceeded
[ANALYTICS:LabelSanitization] metric_name=<name> labels_dropped=<N>
```

#### Step 1: Identify the Cardinality Source

```bash
# Check current distinct metric count
query-metrics --metric analytics_distinct_metric_names_count --range 1h --window 5m

# Find top-growing metric name prefixes (identify the noisy metric family)
grep '\[ANALYTICS:CardinalityExplosion\]' /var/log/themisdb/themisdb.log | tail -10

# List top metric name prefixes by count
themisdb-admin analytics metric-registry stats --top-prefixes 20
```

#### Step 2: Apply Immediate Cardinality Limit

```bash
# Set hard cap on distinct metric names in registry
themisdb-admin config set analytics.metrics.max_cardinality 10000 --apply-live

# Enable LRU eviction for the metric registry (evict least-recently-used metrics)
themisdb-admin config set analytics.metrics.eviction_policy LRU --apply-live

# Verify registry size starts decreasing
query-metrics --metric analytics_distinct_metric_names_count --range 10m --window 1m
```

#### Step 3: Identify and Block the Offending Producer

```bash
# Find the client or job sending high-cardinality labels
grep 'CardinalityExplosion\|LabelSanitization' /var/log/themisdb/themisdb.log | \
  grep -oP 'client_id=\K[^\s]+' | sort | uniq -c | sort -rn | head -5

# Block the offending client temporarily at the ingest gateway
themisdb-admin ingest-gateway block-client --client-id <id> --reason cardinality_explosion
```

#### Step 4: Implement Label Allowlist (Long-term)

```bash
# Configure an allowed label key set (drops unknown labels at ingest)
themisdb-admin config set analytics.metrics.label_allowlist \
  "host,region,service,env,instance_type" --apply-live

# Verify label sanitization is active
grep '\[ANALYTICS:LabelSanitization\]' /var/log/themisdb/themisdb.log | tail -5
```

**Decision Point:**
- ✅ **Registry size stable, cardinality within limit:** Adjust cardinality cap permanently in config
- ⚠ **Eviction rate high but registry stable:** Acceptable if SLO allows some metric loss; investigate root cause
- ❌ **Cardinality still growing after limit + eviction:** Block the offending producer; escalate to data-producer team

---

## Troubleshooting Quick Reference

| Symptom | Log Tag | First Action |
|---------|---------|-------------|
| Ingestion throughput < 10 000 ops/sec | `[STREAMING:BackPressure]` | Check window limits, switch to BLOCK mode (Scenario 1) |
| Window evictions at high rate | `[STREAMING:WindowEviction]` | Increase `max_records_per_window` (Scenario 1) |
| OOM during columnar scan | `[COLUMNAR:OOMRisk]` | Reduce batch size, enable memory-safe mode (Scenario 2) |
| Aggregation overflow errors | `[AGGREGATION:Overflow]` | Switch to SATURATE mode or rate aggregation (Scenario 3) |
| Query p99 > 1 ms | `[ANALYTICS:QueryTimeout]` | Check plan cache hit rate, merge latency (Scenario 4) |
| Window flush stalled | `[STREAMING:FlushStall]` | Force flush, check flush worker health (Scenario 5) |
| Registry size growing unbounded | `[ANALYTICS:CardinalityExplosion]` | Apply cardinality cap, block noisy producer (Scenario 6) |

---

## Evidence & Logging Checklist

After incident resolution, collect:

- [ ] `analytics_ingest_throughput_p99.json` — ingest ops/sec pre- and post-incident
- [ ] `analytics_streaming_window_evictions.log` — Filtered eviction events during incident window
- [ ] `analytics_query_latency_p99_us.csv` — p50/p95/p99 query latency over incident window
- [ ] `analytics_aggregation_overflow_events.log` — Overflow event category distribution
- [ ] `analytics_window_flush_rate.csv` — Window flush rate over incident window
- [ ] `analytics_distinct_metric_names.csv` — Cardinality growth curve over incident window

Archive in: `evidence/analytics-pipeline-incidents/<date>-<issue-id>/`

---

## Quick Reference: Diagnostic Commands

```bash
# ── Streaming window health ─────────────────────────────────────────────────
grep '\[STREAMING:' /var/log/themisdb/themisdb.log | tail -30

# ── Columnar scan OOM risk ──────────────────────────────────────────────────
grep '\[COLUMNAR:OOMRisk\]\|\[COLUMNAR:ScanAborted\]' \
  /var/log/themisdb/themisdb.log | tail -20

# ── Aggregation overflow events ─────────────────────────────────────────────
grep '\[AGGREGATION:Overflow\]' /var/log/themisdb/themisdb.log | tail -20

# ── Query timeout diagnostics ────────────────────────────────────────────────
grep '\[ANALYTICS:QueryTimeout\]\|\[ANALYTICS:PlanCacheMiss\]' \
  /var/log/themisdb/themisdb.log | tail -30

# ── Window rollup stall detection ───────────────────────────────────────────
grep '\[STREAMING:FlushStall\]\|\[ANALYTICS:PipelineStall\]' \
  /var/log/themisdb/themisdb.log | tail -20

# ── Metric cardinality ───────────────────────────────────────────────────────
grep '\[ANALYTICS:CardinalityExplosion\]\|\[ANALYTICS:MetricEvicted\]' \
  /var/log/themisdb/themisdb.log | tail -20

# ── Combined analytics health check ─────────────────────────────────────────
themisdb-admin analytics health-check --verbose

# ── Live metric snapshot ─────────────────────────────────────────────────────
query-metrics \
  --metric analytics_ingest_ops_per_sec,\
analytics_query_latency_p99_us,\
analytics_streaming_window_evictions_total_rate,\
analytics_aggregation_overflow_total_rate,\
analytics_distinct_metric_names_count \
  --range 30m --window 1m
```

---

## Wave D — D1 Distributed Trace Span Cross-Links

> **Wave D Phase 2A dependency:** The trace span annotations below reference the `DistributedTraceSpan`
> framework planned in `docs/operability/WAVE_D_ROADMAP.md` §2A. Until Phase 2A implementation
> completes (Target: Q1 2027), the listed span names are reference identifiers for future
> instrumentation.

### Analytics Pipeline D1 Trace Spans

When the Phase 2A tracing SDK is available, the following operator actions map to trace spans:

| Pipeline Surface | D1 Span Name | Baggage Keys | Notes |
|---|---|---|---|
| Time-series ingest | `analytics.timeseries.ingest` | `metric_name`, `window_id`, `backpressure_mode` | Includes eviction child events |
| Columnar scan | `analytics.columnar.scan` | `batch_size_rows`, `predicate_complexity`, `memory_bytes` | OOM risk event set on high memory |
| Aggregation flush | `analytics.aggregation.flush` | `accumulator_name`, `window_size`, `overflow_guard` | Error status set on overflow |
| Query execution | `analytics.query.execute` | `query_hash`, `plan_cache_hit`, `elapsed_us`, `shard_count` | Child spans per shard merge |
| Window rollup | `analytics.window.rollup` | `window_type`, `flush_latency_ms`, `evicted_count` | Stall status set if no flush > 5 min |
| Distributed merge | `analytics.distributed.merge` | `shard_id`, `merge_latency_ms`, `circuit_breaker_state` | ERROR status on merge timeout |

### Querying Trace Spans (Phase 2A onwards)

```bash
# Find all analytics queries with p99 > 1 ms
otel-query --service analytics_pipeline \
  --operation analytics.query.execute \
  --baggage "elapsed_us>1000" --range 1h

# Find all window rollup stalls
otel-query --service analytics_pipeline \
  --operation analytics.window.rollup \
  --status STALL --range 24h --include-baggage

# Find all aggregation overflow events
otel-query --service analytics_pipeline \
  --operation analytics.aggregation.flush \
  --status ERROR --range 7d

# Cross-reference cardinality with ingest throughput
otel-metrics-join \
  --trace-operation analytics.timeseries.ingest \
  --metric analytics_distinct_metric_names_count \
  --window 5m
```

### Phase 2A Instrumentation Targets

Once Phase 2A is implemented, add trace points in:

- `src/analytics/streaming_window.cpp`: Wrap `updateWindow()`, `flushWindow()`, and eviction paths in
  `DistributedTraceSpan` with baggage `window_type`, `evicted_count`, `flush_latency_ms`
- `src/analytics/columnar_execution.cpp`: Wrap `computeColumnBatches()` in `DistributedTraceSpan`
  with baggage `batch_size_rows`, `memory_bytes`; set OOM risk event on threshold breach
- `src/analytics/result_aggregator.cpp`: Wrap `mergePartialResults()` in `DistributedTraceSpan`
  with baggage `accumulator_name`, `overflow_guard`; set ERROR status on overflow
- `src/analytics/distributed_analytics.cpp`: Add span events for circuit breaker state transitions
  (`CLOSED→OPEN`, `OPEN→HALF_OPEN`, `HALF_OPEN→CLOSED`) with baggage `shard_id`, `failure_count`

**Related Wave D documents:**
- `docs/operability/WAVE_D_ROADMAP.md` §2A — DistributedTraceSpan implementation plan
- `docs/operability/PHASE2A_DISTRIBUTED_TRACING_VERIFICATION.md` — Acceptance gate W4A-TRACE-01
- `src/analytics/ROADMAP.md` §Wave D — Operability dependency notes

---

**Runbook Version:** 1.0
**Last Updated:** 2026-09-16
**Owner:** Analytics Platform Team, Operations Team
**Next Review:** 2027-03-01 (post-Wave D Phase 2A delivery)
