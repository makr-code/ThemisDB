# RUNBOOK: RAG Pipeline

<!-- Wave D operability runbook — approved 2026-09-16 -->
<!-- Cross-links: src/rag/ROADMAP.md Wave D contribution; D1 trace spans -->

## Overview

This runbook covers the five most operator-critical failure scenarios for the
ThemisDB RAG pipeline.  Each scenario lists detection signals, diagnostic
steps, remediation actions, and the expected log pattern used for alerting.

Trace span prefix: `D1-RAG-*` (emitted by the RAG module for Wave D
distributed tracing; see `docs/operability/WAVE_D_ROADMAP.md §D1`).

---

## Scenario 1 — Chunk Index Unavailability

**Log pattern:** `[RAG:IndexUnavailable]`

### Symptoms

- RAG queries return empty result sets or `IndexErrorCode::IndexUnavailable`.
- Alert fires on `rag_chunk_index_available == 0` Prometheus gauge.
- Trace span `D1-RAG-INDEX` shows `status=unavailable`.

### Diagnosis

1. Check the index process or shard health:
   ```
   ctest -R "IndexSoak" -L wave_d --output-on-failure
   ```
2. Inspect the most recent log lines for the pattern:
   ```
   grep "\[RAG:IndexUnavailable\]" /var/log/themis/rag.log | tail -20
   ```
3. Confirm the index file path and permissions:
   ```
   ls -lh /var/themis/data/chunk_index/
   ```
4. Check for disk-full conditions:
   ```
   df -h /var/themis/data/
   ```

### Remediation

1. If the index directory is missing or corrupted, trigger a rebuild:
   ```
   themis-admin index rebuild --module rag --shard all
   ```
2. If disk space is exhausted, free space and restart the index service.
3. If index initialization fails at startup, verify `THEMIS_RAG_INDEX_PATH`
   environment variable points to a writable directory.
4. Escalate to the index team if rebuild does not complete within 30 minutes.

### Rollback

Revert to the prior index snapshot:
```
themis-admin index restore --snapshot latest-known-good
```

---

## Scenario 2 — Recall Degradation

**Log pattern:** `[RAG:RecallDegradation]`

### Symptoms

- `WikiEvalStats::recall_at_k` at k=5 or k=10 drops below 0.8.
- Alert fires on `rag_recall_at_k{k="10"} < 0.8` Prometheus rule.
- Trace span `D1-RAG-RECALL` shows `recall=<value>`.
- Soak test `RAGSoak_ChunkRetrievalStability` fails in CI.

### Diagnosis

1. Run the focused recall evaluation:
   ```
   ctest -R "RAGSoak_ChunkRetrievalStability" --output-on-failure
   ```
2. Check recent index rebuild or embedding model change events:
   ```
   themis-admin audit log --module rag --event index_rebuild --since 24h
   ```
3. Compare embedding model version against the baseline:
   ```
   themis-admin model info --component embedding
   ```
4. Inspect BM25+/HNSW fusion weights in `WikiIndexConfig`:
   - `bm25_k1`, `bm25_b`, `bm25_delta`, `rrf_k`

### Remediation

1. If embedding model was recently changed, roll back:
   ```
   themis-admin model rollback --component embedding --to previous
   ```
2. If HNSW `ef_search` was reduced, restore to baseline:
   ```
   themis-admin config set wiki_index.hnsw_ef_search=128
   ```
3. Trigger a full index re-evaluation against the golden query set:
   ```
   themis-admin eval run --suite golden_rag_queries
   ```

---

## Scenario 3 — LLM Judge Timeout

**Log pattern:** `[RAG:JudgeTimeout]`

### Symptoms

- `LLMJudgeIntegration::evaluate()` returns `llm_unavailable` for all requests.
- Alert fires on `rag_llm_judge_timeout_total > 0`.
- Trace span `D1-RAG-JUDGE` shows `status=timeout`.
- Soak test `RAGSoak_LLMJudgeReliability` reports false positives in CI.

### Diagnosis

1. Verify the LLM backend endpoint is reachable:
   ```
   curl -fsS http://llm-backend:8080/health || echo "LLM backend unreachable"
   ```
2. Check the judge timeout configuration:
   ```
   themis-admin config get rag.llm_judge_timeout_ms
   ```
3. Inspect the judge integration log:
   ```
   grep "\[RAG:JudgeTimeout\]" /var/log/themis/rag.log | tail -20
   ```
4. Confirm `THEMIS_ENABLE_LLM_JUDGE` is set and the backend pointer is non-null.

### Remediation

1. If the LLM backend is down, disable the judge temporarily (fail-closed):
   ```
   themis-admin config set rag.llm_judge_enabled=false
   ```
   The pipeline will return `llm_unavailable` scores and continue serving.
2. Increase the judge timeout if the backend is slow:
   ```
   themis-admin config set rag.llm_judge_timeout_ms=5000
   ```
3. Restart the LLM backend if health check fails:
   ```
   systemctl restart themis-llm-backend
   ```
4. Re-enable the judge once the backend recovers:
   ```
   themis-admin config set rag.llm_judge_enabled=true
   ```

---

## Scenario 4 — Embedding Service Failure

**Log pattern:** `[RAG:EmbeddingFailed]`

### Symptoms

- Ingestion or query embedding calls return zero vectors or error codes.
- Alert fires on `rag_embedding_error_total > 0`.
- Trace span `D1-RAG-EMBED` shows `status=error`.
- New chunks are not indexed because embedding production fails.

### Diagnosis

1. Check the embedding service health:
   ```
   curl -fsS http://embedding-service:8081/health
   ```
2. Inspect embedding error logs:
   ```
   grep "\[RAG:EmbeddingFailed\]" /var/log/themis/rag.log | tail -30
   ```
3. Verify `THEMIS_EMBEDDING_SERVICE_URL` is set to the correct endpoint.
4. Confirm the embedding model is loaded:
   ```
   themis-admin model info --component embedding --verbose
   ```

### Remediation

1. If the embedding service is restarting, wait for readiness:
   ```
   themis-admin wait --service embedding --timeout 120s
   ```
2. If the service is misconfigured, update the endpoint:
   ```
   themis-admin config set rag.embedding_service_url=http://<host>:<port>
   ```
3. For persistent failures, fall back to the cached embedding path:
   ```
   themis-admin config set rag.embedding_cache_fallback=true
   ```
4. Trigger a re-ingestion of failed chunks after service recovery:
   ```
   themis-admin ingest retry --module rag --failed-only
   ```

---

## Scenario 5 — Query Throughput Degradation

**Log pattern:** `[RAG:ThroughputDegradation]`

### Symptoms

- RAG query throughput drops below 500 queries/sec (Wave D gate).
- Alert fires on `rag_query_throughput_qps < 500`.
- Trace span `D1-RAG-THROUGHPUT` shows `qps=<value>`.
- Soak test `RAGSoak_QueryThroughput` fails in CI.

### Diagnosis

1. Run the throughput soak test manually:
   ```
   THEMIS_SOAK_DURATION_MS=10000 ctest -R "RAGSoak_QueryThroughput" --output-on-failure
   ```
2. Profile the top RAG hot paths:
   ```
   themis-admin perf profile --module rag --duration 30s
   ```
3. Check for index lock contention:
   ```
   grep "lock_contention\|mutex_wait" /var/log/themis/rag.log | tail -20
   ```
4. Inspect Prometheus metrics for GC pressure or memory pressure:
   ```
   themis-admin metrics dump --filter "rag_" | grep -E "latency|queue|mem"
   ```

### Remediation

1. If the index is under heavy rebuild load, defer rebuilds to off-peak:
   ```
   themis-admin config set rag.index_rebuild_schedule="02:00"
   ```
2. If thread pool exhaustion is detected, increase the worker count:
   ```
   themis-admin config set rag.query_worker_threads=16
   ```
3. If memory pressure is causing GC pauses, reduce cache size:
   ```
   themis-admin config set rag.embedding_cache_max_bytes=536870912
   ```
4. If throughput does not recover within 10 minutes, escalate to the RAG
   module team via the on-call rotation.

---

## Escalation

| Severity | Condition | Action |
|----------|-----------|--------|
| P1 | IndexUnavailable for > 5 min | Page on-call; trigger rebuild |
| P1 | Recall < 0.5 for > 10 min | Page on-call; roll back index/model |
| P2 | JudgeTimeout > 30% of requests | Disable judge; notify LLM team |
| P2 | EmbeddingFailed > 10% | Switch to cache fallback; page embedding team |
| P3 | ThroughputDegradation < 500 qps | Increase workers; file incident |

---

## Related Resources

- Soak tests: `tests/integration/test_rag_pipeline_soak.cpp`
- Stress tests: `tests/rag/test_rag_highcardinality_stress.cpp`
- Wave D acceptance checklist: `docs/operability/WAVE_D_ACCEPTANCE_CHECKLIST.md`
- RAG module roadmap: `src/rag/ROADMAP.md`
- Architecture: `src/rag/ARCHITECTURE.md`
