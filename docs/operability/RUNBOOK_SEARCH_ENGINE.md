# Runbook: Search Engine — Operator Remediation Guide

<!-- Status: current | Created: 2026-09-16 | Wave D delivery -->
<!-- Links: src/search/ROADMAP.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md -->

## Overview

This runbook provides operator procedures for the five most critical incident
classes in the ThemisDB **Search Engine** (inverted index, full-text ranking,
faceted search, distributed shard mesh, and reindex pipeline).  Each scenario
includes detection signals, root-cause analysis steps, and remediation actions.

**Module error codes:** SE-5000–SE-5199  
**Log prefix format:** `[SEARCH:<IncidentClass>]`

---

## Scenario 1 — Index Corruption

**Log pattern:** `[SEARCH:IndexCorruption]`

### Detection

```
[SEARCH:IndexCorruption] shard_id=<S> segment_id=<G> checksum_expected=<H1> checksum_actual=<H2>
```

- Alert fires when a segment checksum fails on open or merge (error code SE-5000).
- Grafana panel: *Search / Index Health* → `index_corruption_total` counter increments.
- Symptom: queries return unexpected empty result sets or throw `SE-5001` errors.

### Root Cause Analysis

1. Identify the affected shard and segment:
   ```bash
   grep 'IndexCorruption' /var/log/themisdb/search.log | tail -20
   ```
2. Check disk health on the node hosting the shard:
   ```bash
   dmesg | grep -i 'error\|fail\|bad sector' | tail -20
   ```
3. Check for interrupted writes: look for `[SEARCH:ReindexStall]` or
   `[SEARCH:WALSyncFailed]` events before the corruption timestamp.
4. Verify replica shard health: `GET /admin/shards/<S>/health`.

### Remediation

| Step | Action |
|------|--------|
| 1 | Take the corrupted shard offline: `POST /admin/shards/<S>/offline`. |
| 2 | Trigger replica promotion to restore read availability. |
| 3 | Schedule a full segment rebuild on the affected shard: `POST /admin/index/<S>/rebuild`. |
| 4 | After rebuild completes, verify checksum: `POST /admin/index/<S>/verify`. |
| 5 | Return shard online: `POST /admin/shards/<S>/online`. |

### Escalation

Escalate to on-call engineering if the rebuild fails or if > 1 shard is affected
simultaneously.  Capture `themis-diag dump search-index` before escalating.

---

## Scenario 2 — Ranking Model Stall

**Log pattern:** `[SEARCH:RankingStall]`

### Detection

```
[SEARCH:RankingStall] ranker_id=<R> stall_ms=<T> queue_depth=<N> query_id=<ID>
```

- Alert fires when the ranking pipeline has not produced a result in > 2 000 ms
  with pending work (error code SE-5050).
- Symptom: searches return with no ranked results; `[SEARCH:RankingFallback]`
  events appear in the log.

### Root Cause Analysis

1. Check ranking model load: `GET /admin/search/ranker/stats`.
2. Look for GPU/CPU fallback events:
   ```bash
   grep 'RankingStall\|RankingFallback\|GPUError' /var/log/themisdb/search.log | tail -30
   ```
3. Check for OOM on the ranking worker process.
4. Verify the ranking model file is present and unmodified:
   ```bash
   md5sum /data/themisdb/models/search_ranker.bin
   ```

### Remediation

| Step | Action |
|------|--------|
| 1 | Restart the ranking worker pool: `POST /admin/search/ranker/restart`. |
| 2 | Force CPU fallback while GPU issues are investigated: `PATCH /admin/search/ranker {"device":"cpu"}`. |
| 3 | If model file is missing/corrupted: redeploy from artefact store. |
| 4 | If the stall recurs under GPU load: reduce `search.ranker_batch_size`. |

---

## Scenario 3 — Facet Explosion

**Log pattern:** `[SEARCH:FacetExplosion]`

### Detection

```
[SEARCH:FacetExplosion] query_id=<ID> facet_key=<K> cardinality=<N> limit=<M> elapsed_ms=<T>
```

- Alert fires when `cardinality > search.facet_cardinality_limit` (default 100 000)
  (error code SE-5080).
- Symptom: facet queries return incomplete results or time out.

### Root Cause Analysis

1. Identify the offending facet key from the log entry.
2. Check the actual cardinality: `GET /admin/index/facets/<KEY>/cardinality`.
3. Determine whether the high-cardinality key is intentional (e.g. user IDs used
   as facets) or a schema design issue.

### Remediation

| Step | Action |
|------|--------|
| 1 | Abort the offending query: `DELETE /admin/queries/<ID>`. |
| 2 | Add a `LIMIT` to the facet aggregation in the query. |
| 3 | Enable facet cardinality cap: `search.facet_cardinality_limit: 10000`. |
| 4 | Remove high-cardinality fields from the facet index configuration. |
| 5 | If intentional: shard the facet index or use approximate aggregation. |

---

## Scenario 4 — Query Timeout

**Log pattern:** `[SEARCH:QueryTimeout]`

### Detection

```
[SEARCH:QueryTimeout] query_id=<ID> elapsed_ms=<T> timeout_ms=<M> phase=<parser|ranker|retrieval>
```

- Alert fires when a search query exceeds `search.query_timeout_ms` (default 5 000 ms)
  (error code SE-5100).
- Grafana panel: *Search / p99 Latency* → spike above SLA line.

### Root Cause Analysis

1. Identify the phase where the timeout occurred (parser / ranker / retrieval).
2. Check shard response times: `GET /admin/shards/latency`.
3. Look for indexer contention: `[SEARCH:IndexWriteLock]` events.
4. Check for ranking model stall: see Scenario 2.

### Remediation

| Step | Action |
|------|--------|
| 1 | Increase `search.query_timeout_ms` if the workload legitimately requires it. |
| 2 | Scale out the retrieval layer (add shards or replicas). |
| 3 | Enable query result caching: `search.result_cache_enabled: true`. |
| 4 | Tune `search.max_candidates` downward to reduce ranker load. |
| 5 | Add an SLO-breach alert to detect timeout storms early. |

---

## Scenario 5 — Reindex Stall

**Log pattern:** `[SEARCH:ReindexStall]`

### Detection

```
[SEARCH:ReindexStall] index_job_id=<J> stalled_ms=<T> progress_pct=<P> last_doc_id=<D>
```

- Alert fires when an active reindex job has made no progress for > 30 s
  (error code SE-5150).
- Grafana panel: *Search / Reindex Progress* → progress percentage stops advancing.

### Root Cause Analysis

1. Check the reindex job status: `GET /admin/index/jobs/<J>`.
2. Look for downstream write errors:
   ```bash
   grep 'ReindexStall\|WriteError\|DiskFull' /var/log/themisdb/search.log | tail -20
   ```
3. Check disk space on all data nodes.
4. Look for lock contention with concurrent query traffic.

### Remediation

| Step | Action |
|------|--------|
| 1 | Abort the stalled job: `DELETE /admin/index/jobs/<J>`. |
| 2 | Free disk space if usage > 85 %. |
| 3 | Reschedule the reindex during a low-traffic window. |
| 4 | Reduce `search.reindex_batch_size` to lower lock contention. |
| 5 | If persistent: inspect `[SEARCH:IndexCorruption]` events (Scenario 1). |

---

## Quick Reference

| Scenario          | Log Pattern                   | Error Code | First Action                            |
|-------------------|-------------------------------|------------|-----------------------------------------|
| Index Corruption  | `[SEARCH:IndexCorruption]`    | SE-5000    | Take shard offline; rebuild segment     |
| Ranking Stall     | `[SEARCH:RankingStall]`       | SE-5050    | Restart ranker pool; force CPU fallback |
| Facet Explosion   | `[SEARCH:FacetExplosion]`     | SE-5080    | Abort query; set cardinality cap        |
| Query Timeout     | `[SEARCH:QueryTimeout]`       | SE-5100    | Scale out retrieval; tune timeout       |
| Reindex Stall     | `[SEARCH:ReindexStall]`       | SE-5150    | Abort job; free disk; reschedule        |

---

## Diagnostics Commands

```bash
# Dump current search engine state
themis-diag dump search

# List active search queries
curl -s http://localhost:8529/admin/search/queries?state=active | jq .

# Abort a specific search query
curl -s -X DELETE http://localhost:8529/admin/search/queries/<QUERY_ID>

# Check index health across all shards
curl -s http://localhost:8529/admin/index/health | jq .

# Rebuild a specific shard's index
curl -s -X POST http://localhost:8529/admin/index/<SHARD_ID>/rebuild

# List active reindex jobs
curl -s http://localhost:8529/admin/index/jobs | jq .

# Tail search engine log
tail -f /var/log/themisdb/search.log | grep '\[SEARCH:'
```

---

## Related Documents

- `src/search/ROADMAP.md` — module roadmap and Wave D closure
- `ARCHITECTURE.md` — search engine architecture
- `SECURITY.md` — authentication and authorization for search endpoints
- `tests/integration/test_search_engine_soak.cpp` — Wave D soak test
- `benchmarks/search/bench_search_dedicated_gates.cpp` — Wave D benchmark gates
