# RUNBOOK: Vector Search — Triage & Recovery

**Author:** ThemisDB Contributors
**Created:** 2026-09-16
**Last Updated:** 2026-09-16
**Status:** active

**Audience:** Database Operators, SREs, Vector-Search / AI Infra Team Lead
**Purpose:** Triage and recover from vector search index failures, recall degradation, high-latency queries, index rebuild failures, and memory pressure under large indices
**Severity:** Medium-High (affects ANN/embedding search; scalar query execution continues unimpacted)
**Estimated Duration:** 5 min - 2 hours (depending on failure class)

---

## Overview

This runbook guides operators through diagnosing and recovering from failures in the ThemisDB vector search subsystem (`src/vector_search/`). The subsystem has four primary operational surfaces:

1. **Index lifecycle** — HNSW / IVF index build, load, persist, and rebuild operations
2. **Query execution** — kNN search, distance computation, result ranking
3. **Recall quality** — approximate nearest-neighbour accuracy (p-recall@k)
4. **Resource management** — in-memory index footprint, build-time memory spikes

**Key Principles:**
- Vector search is an additive query capability; its failure must not block scalar query execution or replication
- Index corruption triggers automatic rebuild by default; operators must confirm rebuild completion before re-enabling high-QPS traffic
- Recall degradation (p-recall@k < 0.9) is a soft SLO breach, not a hard availability incident — alert but do not page unless combined with high latency
- Memory pressure during index build is expected for large corpora; apply back-pressure via the `vector_index.rebuild_batch_size` config key before escalating

**Wave D D1 Trace Span Cross-Links:**
- Span tag `vector.index.op` — captures HNSW insert, delete, and rebuild latencies
- Span tag `vector.search.recall` — captures per-query recall estimates from background sampling
- Span tag `vector.memory.rss_mb` — captures index memory watermark per collection

---

## Prerequisites Checklist

Before beginning any intervention, verify:

- [ ] Scalar query execution is operational (vector search is non-critical path for writes/reads)
- [ ] Access to server logs with `[VECTOR:*]` tag filtering
- [ ] Prometheus/Grafana dashboard showing vector search metrics (`vector_search_*`, `vector_index_*`)
- [ ] Knowledge of which index algorithm is configured (`hnsw` vs `ivf`) and its parameter set (M, ef_construction)
- [ ] Ability to trigger an index rebuild via admin API or config reload

---

## Quick Reference Table

| Log Pattern | Severity | First Response |
|---|---|---|
| `[VECTOR:IndexCorruption]` | Critical | Trigger index rebuild; halt writes during rebuild |
| `[VECTOR:RecallDegradation]` | Warning | Check index staleness; trigger background rebalance |
| `[VECTOR:QueryTimeout]` | Warning | Check ef_search parameter; scale query concurrency |
| `[VECTOR:MemoryPressure]` | Warning | Reduce batch size; schedule off-peak rebuild |
| `[VECTOR:RebuildFailure]` | High | Check disk space and memory; retry with reduced batch |

---

## Failure Scenarios

---

### Scenario 1: Index Unavailability / Index Corruption

**Symptoms:**
- Logs contain `[VECTOR:IndexCorruption]` tags
- kNN search returns empty results or throws with `"E5404: Index corruption detected"`
- Prometheus metric `vector_index_corrupt_total` is non-zero

**Log pattern to search for:**
```
[VECTOR:IndexCorruption] HNSW index integrity check failed — collection: <name>, error: ...
```

#### Step 1: Confirm Index State

```bash
# Search recent logs for corruption events
grep '\[VECTOR:IndexCorruption\]' /var/log/themisdb/themisdb.log | tail -20

# Confirm corruption metric
curl -s http://localhost:9090/api/v1/query?query=vector_index_corrupt_total | jq .
```

#### Step 2: Halt Write Traffic to Affected Collection

```bash
# Pause ingest to prevent further corruption propagation
# (use admin API or feature flag depending on deployment)
curl -X POST http://localhost:8080/admin/collections/<name>/pause-ingest
```

#### Step 3: Trigger Index Rebuild

```bash
# Trigger full index rebuild from WAL / document store
curl -X POST http://localhost:8080/admin/collections/<name>/rebuild-index \
  -H 'Content-Type: application/json' \
  -d '{"algorithm": "hnsw", "force": true}'
```

#### Step 4: Monitor Rebuild Progress

```bash
# Watch rebuild progress log
grep 'IndexRebuild\|rebuild_progress' /var/log/themisdb/themisdb.log | tail -30

# Prometheus: rebuild completion metric
curl -s 'http://localhost:9090/api/v1/query?query=vector_index_rebuild_complete_total' | jq .
```

#### Step 5: Resume Traffic

```bash
curl -X POST http://localhost:8080/admin/collections/<name>/resume-ingest
```

**Expected resolution time:** 10–60 min depending on corpus size.

---

### Scenario 2: Recall Degradation

**Symptoms:**
- Logs contain `[VECTOR:RecallDegradation]` tags
- Background recall sampling reports p-recall@10 < 0.9
- `vector_search_recall_p10` Prometheus metric drops below 0.9

**Log pattern to search for:**
```
[VECTOR:RecallDegradation] kNN recall degraded — collection: <name>, recall@10=0.XX, threshold=0.90
```

#### Step 1: Confirm Recall Metrics

```bash
grep '\[VECTOR:RecallDegradation\]' /var/log/themisdb/themisdb.log | tail -20
curl -s 'http://localhost:9090/api/v1/query?query=vector_search_recall_p10' | jq .
```

#### Step 2: Diagnose Root Cause

| Symptom | Likely Cause | Action |
|---|---|---|
| Gradual decline after large bulk insert | HNSW graph connectivity degraded | Trigger background rebalance or full rebuild |
| Sudden drop correlated with config change | ef_search lowered or M reduced | Revert config; re-evaluate parameter set |
| Collection-specific, not global | Segment fragmentation in that collection | Per-collection index rebuild |

#### Step 3: Increase ef_search Temporarily

```bash
# Bump ef_search to recover recall at the cost of higher latency
curl -X PATCH http://localhost:8080/admin/collections/<name>/config \
  -H 'Content-Type: application/json' \
  -d '{"vector_index": {"ef_search": 200}}'
```

#### Step 4: Schedule Background Rebalance

```bash
curl -X POST http://localhost:8080/admin/collections/<name>/rebalance-index
```

---

### Scenario 3: High-Latency Query (QueryTimeout)

**Symptoms:**
- Logs contain `[VECTOR:QueryTimeout]` tags
- p99 query latency exceeds 10 ms for 128-dim, k=10
- `vector_query_latency_p99_ms` Prometheus metric > 10 ms

**Log pattern to search for:**
```
[VECTOR:QueryTimeout] kNN query exceeded latency threshold — collection: <name>, latency_ms=XX.X, threshold_ms=10
```

#### Step 1: Confirm Latency Metrics

```bash
grep '\[VECTOR:QueryTimeout\]' /var/log/themisdb/themisdb.log | tail -20
curl -s 'http://localhost:9090/api/v1/query?query=histogram_quantile(0.99,rate(vector_query_latency_bucket[5m]))' | jq .
```

#### Step 2: Identify Bottleneck

| Check | Command | Expected |
|---|---|---|
| Query concurrency | `top -H -p $(pgrep themisdb)` | < 80% CPU saturation |
| Index size vs. RAM | `grep 'vector.memory.rss_mb' logs` | < 80% of available RAM |
| ef_search value | `curl .../admin/collections/<name>/config` | ≤ 128 for p99 < 10 ms |

#### Step 3: Reduce ef_search if Over-Configured

```bash
curl -X PATCH http://localhost:8080/admin/collections/<name>/config \
  -H 'Content-Type: application/json' \
  -d '{"vector_index": {"ef_search": 64}}'
```

#### Step 4: Scale Query Workers

```bash
# Increase query thread pool size (requires restart in current release)
# Edit config: vector_search.query_threads = <N>
systemctl restart themisdb
```

---

### Scenario 4: Index Rebuild Failure

**Symptoms:**
- Rebuild triggered but does not complete; logs show rebuild errors
- `vector_index_rebuild_failed_total` metric increments
- `[VECTOR:RebuildFailure]` log patterns present

**Log pattern to search for:**
```
[VECTOR:RebuildFailure] Index rebuild failed — collection: <name>, error: <OOM | disk full | timeout>
```

#### Step 1: Confirm Failure and Cause

```bash
grep '\[VECTOR:RebuildFailure\]' /var/log/themisdb/themisdb.log | tail -30
df -h /var/lib/themisdb  # Check disk space
free -h                  # Check available memory
```

#### Step 2: Mitigate by Reducing Rebuild Batch Size

```bash
curl -X PATCH http://localhost:8080/admin/config \
  -H 'Content-Type: application/json' \
  -d '{"vector_index": {"rebuild_batch_size": 1000}}'
```

#### Step 3: Retry Rebuild

```bash
curl -X POST http://localhost:8080/admin/collections/<name>/rebuild-index \
  -d '{"algorithm": "hnsw", "force": true, "batch_size": 1000}'
```

#### Step 4: Escalate if Repeated Failures

If rebuild fails 3+ times, escalate to the AI/Infrastructure team lead with:
- Collection name and approximate vector count
- Available RAM and disk at time of failure
- Full `[VECTOR:RebuildFailure]` log extract

---

### Scenario 5: Memory Pressure Under Large Index

**Symptoms:**
- Logs contain `[VECTOR:MemoryPressure]` tags
- System RSS grows unbounded during build or query bursts
- OOM killer may terminate the process

**Log pattern to search for:**
```
[VECTOR:MemoryPressure] Vector index memory watermark exceeded — rss_mb=XXXX, threshold_mb=YYYY
```

#### Step 1: Confirm Memory Usage

```bash
grep '\[VECTOR:MemoryPressure\]' /var/log/themisdb/themisdb.log | tail -20

# Current process RSS
ps -o pid,rss,vsz -p $(pgrep themisdb)

# Prometheus memory metric
curl -s 'http://localhost:9090/api/v1/query?query=vector_index_memory_rss_mb' | jq .
```

#### Step 2: Enable Mmap-Backed Index (if available)

```bash
curl -X PATCH http://localhost:8080/admin/collections/<name>/config \
  -H 'Content-Type: application/json' \
  -d '{"vector_index": {"storage_mode": "mmap", "mmap_path": "/var/lib/themisdb/mmap"}}'
```

#### Step 3: Evict Unused Collections from Memory

```bash
curl -X POST http://localhost:8080/admin/collections/<name>/unload-index
```

#### Step 4: Set Memory Watermark Alert

Add to `prometheus/alerts/vector_search.yaml`:
```yaml
- alert: VectorIndexMemoryPressure
  expr: vector_index_memory_rss_mb > 8192
  for: 5m
  labels:
    severity: warning
  annotations:
    summary: "Vector index memory pressure on {{ $labels.collection }}"
    runbook: "docs/operability/RUNBOOK_VECTOR_SEARCH.md#scenario-5"
```

---

## Alert Rules Reference

| Alert Name | Metric | Threshold | Runbook Section |
|---|---|---|---|
| `VectorIndexCorrupt` | `vector_index_corrupt_total > 0` | Immediate | Scenario 1 |
| `VectorRecallDegradation` | `vector_search_recall_p10 < 0.9` | 5 min | Scenario 2 |
| `VectorQueryHighLatency` | `vector_query_latency_p99_ms > 10` | 5 min | Scenario 3 |
| `VectorRebuildFailed` | `vector_index_rebuild_failed_total > 0` | Immediate | Scenario 4 |
| `VectorIndexMemoryPressure` | `vector_index_memory_rss_mb > 8192` | 5 min | Scenario 5 |

---

## Wave D D1 Trace Span Reference

| Span Tag | Description | Expected Range |
|---|---|---|
| `vector.index.op=insert` | Single-vector HNSW insert | < 100 µs |
| `vector.index.op=search` | kNN search, 128-dim, k=10 | < 10 ms p99 |
| `vector.index.op=rebuild` | Full collection rebuild | Varies by corpus size |
| `vector.search.recall` | Background recall sample | ≥ 0.9 |
| `vector.memory.rss_mb` | Index RSS watermark | < 80% system RAM |

---

## Escalation Path

1. **L1 (Operator):** Follow Scenarios 1–5 above. Most issues resolve within 30 min.
2. **L2 (SRE / AI Infra Team):** Escalate rebuild failures, sustained recall < 0.8, or memory OOM events.
3. **L3 (Core Engineering):** Escalate index corruption that survives rebuild or HNSW graph invariant violations.

---

## Related Documentation

- `src/vector_search/ROADMAP.md` — module roadmap and Wave D closure
- `docs/operability/WAVE_D_ROADMAP.md` — Wave D evidence requirements
- `docs/operability/WAVE_D_SIGN_OFF.md` — Wave D sign-off checklist
- `tests/integration/test_vector_search_soak.cpp` — soak test (insert/query throughput, HNSW stability, concurrent recall)
- `tests/vector_search/test_vector_search_highcardinality_stress.cpp` — high-cardinality stress tests
- `benchmarks/vector_search/bench_vector_search_dedicated_gates.cpp` — p95/p99 benchmark gates (VS-BM-01 – VS-BM-04)
