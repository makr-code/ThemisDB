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

# Cache Models and Policies

Exact model backend not verifiable from final document text alone.

## Purpose and scope

This page describes the state and decision models implemented by the cache module. It is for developers changing tier selection, eviction, TTL, embedding similarity, or prefetch policy. The cache module models reuse and access history; it does not determine whether an underlying query result is authorized or still valid after writes.

## Adaptive tier model

[`AdaptiveQueryCache`](../../include/cache/adaptive_query_cache.h) exposes three logical levels:

| Level | Role | Implementation constraints |
|---|---|---|
| HOT / L1 | Small, fast in-memory tier | Entry count, per-entry size, TTL, and LRU/LFU/ARC selection are configurable. |
| WARM / L2 | Larger in-memory tier with compression support | Entry count, per-entry size, TTL, and eviction policy are configurable. |
| COLD / L3 | Persistent RocksDB tier | TTL and database path are configurable; external database availability affects the operation. |

The public `CacheStats` records per-tier hits, misses, evictions, promotions, and demotions. Entry size limits, circuit breaker, rate limiting, backpressure, write-through, tenant isolation, adaptive TTL, prefetch, and replication are separately configured; several are disabled by default. Configuration is not an implicit deployment policy: production limits and enabled controls must be selected for the workload.

Tiering is a reuse and resource-placement decision, not a consistency protocol. Writes that can make cached results stale require an appropriate invalidation path. The current module contract defines key/value bounds, temporal limits, failure classifications, and coordinator semantics in [`cache_contract.h`](../../include/cache/cache_contract.h).

## Access and eviction models

The base eviction interface receives access, insert, and removal events and returns a key to evict. Adaptive L1/L2 configuration selects among LRU, LFU, and ARC policies; `BoundedLRUCache` maintains a bounded map and recency list. Frequency weighting is configurable for the adaptive cache. These policies approximate future value from recent or frequent access; they do not prove that retained entries are fresh or useful.

Adaptive TTL, when enabled, is bounded by the configured minimum and maximum. The workload-driven TTL interface represents access frequency, age of last access, and write ratio; its configured maximum is a hard cap. Treat TTL values as an operational freshness window, not a substitute for invalidation after writes.

## Prefetch model

[`PredictivePrefetcher`](../../include/cache/predictive_prefetcher.h) stores counts of observed transitions from a source fingerprint to successor fingerprints. It bounds tracked source keys, successors per source, predictions, and the minimum transition count. Optional configuration adds confidence filtering, time-of-day weighting, and A/B accounting. History is associated with a tenant or the empty global namespace.

This is a bounded first-order transition model, not a general language model or a guarantee of future access. Candidate generation is only useful if callers measure actual prefetch hits, generated candidates, and overhead. Persistence hooks use the configured RocksDB wrapper; `clear()` removes the in-memory model state.

## Embedding similarity model

`EmbeddingCache` accepts vectors of exactly `embedding_dim` elements. The indexed path uses the vector-index manager configured with cosine distance, requests the nearest item, converts cosine distance to similarity, and accepts a result only when the similarity meets `similarity_threshold`. If vector-index initialization leaves no active index, the implementation scans entries and calculates cosine similarity. This implementation detail means index availability changes lookup cost and must be included in performance evaluation.

The similarity threshold is an application decision. Incorrect dimensions are rejected as misses; expired candidates are not valid hits. The cache does not re-embed queries or evaluate semantic correctness of a returned `query_text`.

## Evaluation criteria

For every model or policy change, compare against the existing release baseline using the cases in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md). Include hit rate and per-tier latency, p95/p99, memory/entry counts, prefetch overhead and usefulness, tenant isolation, and invalidation behavior. The [research notes](./RESEARCH.md) define a reproducible evaluation outline; benchmark evidence belongs in the existing result/manifest process rather than being inferred from defaults.

## Related documentation

- [Research and evaluation questions](./RESEARCH.md)
- [Embedding cache contract](./EMBEDDING.md)
- [Implementation map](./IMPLEMENTATION.md)
- [Cache contract header](../../include/cache/cache_contract.h)
