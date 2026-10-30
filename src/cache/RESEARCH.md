---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Cache Research Notes

Exact model backend not verifiable from final document text alone.

## Purpose and evidence boundary

This note records the measurable questions and evidence sources for engineering work on `src/cache`. It is for cache maintainers evaluating cache policy, query reuse, embedding reuse, warmup, and distributed invalidation. It reports no performance or retrieval-quality result by itself.

The implementation evidence is in [`adaptive_query_cache.cpp`](adaptive_query_cache.cpp), [`embedding_cache.cpp`](embedding_cache.cpp), [`semantic_cache.cpp`](semantic_cache.cpp), and the public declarations under [`include/cache`](../../include/cache). Test evidence is in [`tests/cache`](../../tests/cache); release performance evidence is defined by [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md) and the [cache benchmark runbook](../../benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md). The binding v1.x invariants are in [`cache_contract.h`](../../include/cache/cache_contract.h).

## Questions and assumptions

1. **Tier placement:** Does selecting an L1/L2/L3 tier from entry size, access behavior, and configuration improve the target workload without exceeding its memory and latency budgets? The adaptive cache has bounded L1/L2 capacities and a RocksDB-backed L3; write-through is an explicit option with a different write cost.
2. **Approximate embedding reuse:** What similarity threshold gives acceptable reuse for a particular embedding model and query corpus? `EmbeddingCache` defaults to cosine similarity and threshold `0.95`, but that default is not evidence that different models, dimensions, or applications share a safe threshold.
3. **Prefetch value:** Do observed successive fingerprints predict future accesses often enough to offset extra cache work and memory? `PredictivePrefetcher` records bounded transition counts; prediction value must be measured against the workload rather than presumed.
4. **Distributed invalidation:** How often do asynchronous invalidation deliveries become partial or delayed, and do operators see the resulting degradation promptly? Local completion and successful remote delivery are separate outcomes.

These are evaluation questions, not claims that a workload-specific model has already been trained or that cache hits are always semantically safe.

## Methods and evaluation

- Compare cache-enabled and cache-disabled paths with the same deterministic query/result inputs and tenant configuration.
- For adaptive cache policy, measure hit/miss counts and p50/p95/p99 latency for L1, L2, L3, misses, tenant-aware operations, invalidation, and concurrent access.
- For embedding reuse, report embedding model/version, dimension, metric, threshold, corpus, duplicate/near-duplicate labeling method, hit rate, and false-reuse cases. A cache hit at or above the configured threshold is not a substitute for application-level correctness review.
- For prefetch, use a chronological workload split: learn transition counts on the earlier segment and measure candidate hit ratio, unnecessary prefetches, overhead bytes, and per-tenant behavior on a later segment. Keep A/B group assignment and time-of-day weighting settings fixed and recorded.
- For replication, inject peer failures and verify local operation behavior, failed-peer diagnostics, bounded retry/drop behavior, and tenant-scoped invalidation.

The existing focused suites include [`test_cache_tenant_isolation_hardening.cpp`](../../tests/cache/test_cache_tenant_isolation_hardening.cpp), [`test_cache_coordinator_degradation.cpp`](../../tests/cache/test_cache_coordinator_degradation.cpp), and [`test_cache_efficiency_multilevel.cpp`](../../tests/cache/test_cache_efficiency_multilevel.cpp). The release benchmark cases and allowed regression tolerance are listed in [`PERFORMANCE_EXPECTATIONS.md`](PERFORMANCE_EXPECTATIONS.md); use the benchmark runbook for reproducible collection and manifest checks.

## Trade-offs and limits

- Larger capacities and longer TTLs can increase hit opportunities but retain more data and may extend stale-result exposure.
- Approximate similarity saves exact-match constraints but can return an unsuitable result when the threshold or embeddings are not calibrated for the workload.
- A vector index reduces search work when available; the embedding implementation can instead scan entries when no index is active, changing the cost profile with cache size.
- Replication and prefetch add work beyond the local lookup path. Queue limits and retry caps bound some work but do not guarantee every peer receives an event.
- Benchmark results are hardware-, compiler-, corpus-, and configuration-specific. No numeric latency or accuracy claim should be copied from these notes without the associated run artifacts.

## Reproducibility record

For any reported conclusion, preserve the code revision, build preset, hardware, cache configuration, dataset/workload description, benchmark JSON, manifest, and generated gate report under the existing benchmark evidence process. Mark unavailable L3 cases as skipped as the runbook requires; do not treat a skipped case as a passing measurement.

## Related documentation

- [Modeling assumptions](./MODELING.md)
- [Embedding cache behavior](./EMBEDDING.md)
- [Performance expectations](./PERFORMANCE_EXPECTATIONS.md)
- [Module guide](./MODULE_GUIDE.md)
