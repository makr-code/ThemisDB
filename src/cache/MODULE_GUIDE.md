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

# Cache Module Guide

Exact model backend not verifiable from final document text alone.

## Audience and module role

This guide is the entry point for developers integrating or changing ThemisDB cache behavior. `src/cache` implements cache internals; public C++ declarations are in `include/cache`. It owns cache reuse, bounded storage, eviction, expiry, warmup/prefetch, coordination, and cache-specific observability. Query planning, authorization decisions, embedding generation, and source-of-truth data remain owned by their respective modules.

## Choose the right surface

| Need | Start here |
|---|---|
| Reuse query results across configured tiers | [`AdaptiveQueryCache`](../../include/cache/adaptive_query_cache.h) and [API.md](./API.md) |
| Simple bounded key/value cache | [`BoundedLRUCache`](../../include/cache/bounded_lru_cache.h) |
| Reuse a result by vector similarity | [`EmbeddingCache`](../../include/cache/embedding_cache.h) and [EMBEDDING.md](./EMBEDDING.md) |
| Exact prompt + parameter result key in RocksDB | [`SemanticCache`](../../include/cache/semantic_cache.h) |
| Distribute entries or invalidations | [`ICacheCoordinator`](../../include/cache/cache_replication_coordinator.h) and [`RedisCacheCoordinator`](../../include/cache/redis_cache_coordinator.h) |
| Inspect cache hit rate and latency SLOs | [`CacheHitRateSloMonitor`](../../include/cache/cache_hit_rate_slo_monitor.h) |

## Integration sequence

1. Decide whether results may be shared across callers. If not, enable tenant isolation and use tenant-aware fingerprint, `get`, `put`, and invalidation calls consistently.
2. Set explicit entry counts, size bounds, TTLs, backend path, and optional feature switches for the deployment. Validate the `AdaptiveQueryCache::Config` before construction.
3. Define freshness and invalidation behavior with the writer of the underlying data. A TTL alone may not satisfy freshness requirements.
4. If using a distributed coordinator, decide which operation is local and which is asynchronous, configure the backend and retry/timeout behavior, and monitor partial/dropped delivery.
5. Test the positive path, miss/expiry, malformed input, tenant boundary, backend degradation, and recovery path before enabling the feature in production.

## Developer workflow and evidence

- Read [ARCHITECTURE.md](./ARCHITECTURE.md), [IMPLEMENTATION.md](./IMPLEMENTATION.md), and the binding [`cache_contract.h`](../../include/cache/cache_contract.h) before changing cache semantics.
- Focused test sources live in [`tests/cache`](../../tests/cache); CMake registers each as a module-focused test. Examples include `test_cache_tenant_isolation_hardening`, `test_cache_coordinator_degradation`, `test_cache_contract_hardening`, and `test_cache_warmup`.
- The benchmark suite is [`bench_adaptive_query_cache.cpp`](../../benchmarks/query/bench_adaptive_query_cache.cpp) and [`bench_embedding_cache_performance.cpp`](../../benchmarks/cache/bench_embedding_cache_performance.cpp). Follow the [benchmark runbook](../../benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md); benchmark results are not inferred from code defaults.
- Keep roadmap status in [ROADMAP.md](./ROADMAP.md), future work in [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md), and historical changes in [CHANGELOG.md](./CHANGELOG.md).

## Documentation map

- [Architecture and failure domains](./ARCHITECTURE.md)
- [Research questions and evaluation](./RESEARCH.md)
- [Policy models](./MODELING.md)
- [Embedding cache](./EMBEDDING.md)
- [Implementation map](./IMPLEMENTATION.md)
- [Public API summary](./API.md)
- [Incident response](./RUNBOOK.md)
- [Operations and observability](./OPERATIONS.md)
- [Trust boundaries](./TRUST.md)
- [Security controls](./SECURITY.md)

## Boundaries and caveats

Cache contents are derived data and may be stale, evicted, missing, or inaccessible when a backend is degraded. Cache hits must not replace authorization or source-data validation. Tenant separation, distributed consistency, and durability depend on the specific API and configuration; they are not universal properties of every cache class.
