# Runbook: LLM Wiki Module

<!-- Wave D operability deliverable — llm_wiki module -->
<!-- Source: src/llm_wiki/ROADMAP.md § Wave D Contribution -->

**Module:** `src/llm_wiki/`, `plugins/themisdb_llm_wiki/`  
**Version:** 1.0.0 (Wave D, 2026-Q1)  
**Owner:** ThemisDB LLM Wiki Team  
**Labels:** `wave_d;operability;runbook`

---

## Overview

This runbook covers operator response procedures for the five most critical
LLM Wiki module incident classes. Each scenario includes detection signals
(log patterns and metrics), immediate mitigations, and escalation paths.

---

## Scenario 1 — Index Sync Failure

**Log pattern:** `[LLM_WIKI:IndexSyncFailed]`

### Detection

- Log line: `[LLM_WIKI:IndexSyncFailed] source=<S> article_id=<ID> reason=<R>`
- Metric: `llm_wiki_sync_failure_count_total`
- Alert: `LLMWikiIndexSyncFailureRate`

### Immediate actions

1. **Check source connectivity.** Verify the Wikipedia / wiki source endpoint
   is reachable from the LLM Wiki plugin host.
2. **Inspect sync error reason.** Common values: `SOURCE_TIMEOUT`, `PARSE_ERROR`,
   `RATE_LIMITED`. Each maps to a distinct remediation path.
3. **Enable retry with backoff.** Set `llm_wiki.sync_retry_enabled=true` and
   configure `llm_wiki.sync_retry_backoff_ms=<N>`.
4. **Switch to cached articles.** If the source is unavailable, serve stale
   cache entries by setting `llm_wiki.serve_stale_on_sync_failure=true`.

### Escalation

Sync failures affecting > 10 % of articles over a 30-minute window are P2.

---

## Scenario 2 — Semantic Search Degradation

**Log pattern:** `[LLM_WIKI:SemanticDegradation]`

### Detection

- Log line: `[LLM_WIKI:SemanticDegradation] query=<Q> latency_ms=<L> threshold_ms=<T>`
- Metric: `llm_wiki_semantic_search_latency_ms` p99 above threshold
- Alert: `LLMWikiSemanticLatencyHigh`

### Immediate actions

1. **Check embedding model health.** Run `themis_admin llm_wiki probe-embedding`
   to confirm the embedding service responds within SLA.
2. **Check vector index fragmentation.** High search latency is often caused by
   an un-compacted index; trigger compaction via
   `themis_admin llm_wiki compact-index`.
3. **Enable approximate search mode.** Lower `llm_wiki.ann_ef_search` to trade
   recall for latency during degradation.
4. **Scale search replicas** if load has increased beyond the configured shard
   capacity.

### Escalation

p99 latency > 5× baseline sustained for > 10 minutes is P2.

---

## Scenario 3 — Embedding Model Unavailable

**Log pattern:** `[LLM_WIKI:EmbeddingUnavailable]`

### Detection

- Log line: `[LLM_WIKI:EmbeddingUnavailable] model=<M> endpoint=<E> code=<C>`
- Metric: `llm_wiki_embedding_unavailable_count_total` nonzero
- Alert: `LLMWikiEmbeddingUnavailable`

### Immediate actions

1. **Confirm the embedding endpoint is running.** Check the embedding service
   health endpoint.
2. **Fall back to BM25 / keyword search.** Set
   `llm_wiki.fallback_search_mode=bm25` to allow queries to proceed without
   embeddings.
3. **Check model version.** A model version mismatch between the indexing and
   query path can produce `EmbeddingUnavailable`; verify
   `llm_wiki.embedding_model_version` matches the deployed model.
4. **Restart the embedding sidecar** if a transient startup failure is suspected.

### Escalation

Embedding unavailability affecting all queries is a P1 incident.

---

## Scenario 4 — Article Cache Overflow

**Log pattern:** `[LLM_WIKI:CacheOverflow]`

### Detection

- Log line: `[LLM_WIKI:CacheOverflow] cache_size_mb=<N> limit_mb=<L> evictions=<E>`
- Metric: `llm_wiki_cache_eviction_rate` above baseline
- Alert: `LLMWikiCacheEvictionHigh`

### Immediate actions

1. **Increase cache size limit.** Set `llm_wiki.cache_max_mb=<N>` to reduce
   eviction pressure.
2. **Enable LRU eviction.** Confirm `llm_wiki.cache_eviction_policy=lru` is
   active to ensure the most-requested articles are retained.
3. **Audit hot articles.** Use `themis_admin llm_wiki cache-stats` to identify
   articles that are evicted and re-fetched repeatedly.
4. **Pre-warm the cache** after a restart using the warm-up script:
   `scripts/llm_wiki/cache_warmup.sh`.

### Escalation

Cache overflow causing p95 search latency degradation beyond 3× baseline is P2.

---

## Scenario 5 — Wiki Source Timeout

**Log pattern:** `[LLM_WIKI:SourceTimeout]`

### Detection

- Log line: `[LLM_WIKI:SourceTimeout] source=<S> timeout_ms=<T> attempt=<A>`
- Metric: `llm_wiki_source_timeout_count_total`
- Alert: `LLMWikiSourceTimeout`

### Immediate actions

1. **Check upstream wiki source health.** Verify network connectivity and DNS
   resolution for the configured wiki source URL.
2. **Increase source timeout.** Set `llm_wiki.source_timeout_ms=<N>` if the
   source is legitimately slow under load.
3. **Enable caching of failed fetches.** Set
   `llm_wiki.cache_fetch_failure=true` to prevent thundering-herd retries.
4. **Switch to a mirror source.** If the primary source is consistently timing
   out, configure an alternative wiki mirror in
   `llm_wiki.source_fallback_url`.

### Escalation

Source timeouts persisting > 15 minutes with no recovery path are P2, requiring
an architecture review of the wiki source dependency.

---

## Related resources

- `src/llm_wiki/ROADMAP.md` — module roadmap and Wave D closure
- `tests/integration/test_llm_wiki_soak.cpp` — soak test
- `tests/llm_wiki/test_llm_wiki_highcardinality_stress.cpp` — stress test
- `benchmarks/llm_wiki/bench_llm_wiki_dedicated_gates.cpp` — LW-BM-01..04
