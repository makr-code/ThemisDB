# ThemisDB Cache Module

<!-- Status: PRODUCTION_CANDIDATE | Phase 1-4 complete | validated: 2026-08-10 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Module Purpose

The cache module provides multi-tier caching infrastructure for query and embedding workloads, including adaptive query cache paths, tenant-aware isolation, warmup/prefetch support, and distributed replication/coordination surfaces.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| adaptive_query_cache.cpp | multi-tier adaptive cache facade and policies |
| bounded_lru_cache.cpp | bounded in-memory LRU cache tier behavior |
| semantic_cache.cpp | semantic fingerprint/similarity cache support |
| embedding_cache.cpp | embedding-specific cache storage/query behavior |
| warmup.cpp | bulk warmup and preload paths |
| predictive_prefetcher.cpp | predictive prefetch candidate handling |
| cache_replication.cpp | cache replication event handling |
| cache_replication_coordinator.cpp | replication coordination in-process surfaces |
| distributed_cache_coordinator.cpp | node-local distributed coordination support |
| redis_cache_coordinator.cpp | Redis pub/sub coordination support |
| grpc_remote_cache_peer.cpp | remote cache peer transport integration |
| cache_hit_rate_slo_monitor.cpp | hit-rate SLO monitoring and alerting hooks |

## Scope

In scope:
- query and embedding caching runtime surfaces
- tenant-aware isolation and invalidation/warmup behavior
- distributed cache coordination and replication support
- cache observability and SLO-oriented monitoring paths

Out of scope:
- non-cache query planning/parsing ownership
- storage engine internals outside cache integration points
- business-domain policy logic outside cache runtime boundaries

## Runtime Behavior and Limits

- behavior depends on configured cache tiers, limits, and feature flags.
- distributed paths depend on selected coordination backends.
- cache operations return structured failure paths for invalid/degraded conditions.

## Sourcecode Verification (Module: cache/readme)

- Verified files:
  - src/cache/adaptive_query_cache.cpp
  - src/cache/bounded_lru_cache.cpp
  - src/cache/semantic_cache.cpp
  - src/cache/embedding_cache.cpp
  - src/cache/warmup.cpp
  - src/cache/predictive_prefetcher.cpp
  - src/cache/cache_replication.cpp
  - src/cache/cache_replication_coordinator.cpp
  - src/cache/distributed_cache_coordinator.cpp
  - src/cache/redis_cache_coordinator.cpp
  - src/cache/grpc_remote_cache_peer.cpp
  - src/cache/cache_hit_rate_slo_monitor.cpp
- Verified behavior surfaces:
  - multi-tier cache and tenant-aware paths
  - warmup/prefetch and distributed coordination/replication
  - hit-rate monitoring and operational observability
- Note:
  - forward planning is tracked in ROADMAP.md and FUTURE_ENHANCEMENTS.md
  - historical entries remain in CHANGELOG.md