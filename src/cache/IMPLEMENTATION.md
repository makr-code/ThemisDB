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

# Cache Implementation Notes

Exact model backend not verifiable from final document text alone.

## Purpose and source map

This document is an implementation map for maintainers. Public contracts live under `include/cache/`; internal implementations live in `src/cache/`. The v1.x behavioral constraints are defined by [`cache_contract.h`](../../include/cache/cache_contract.h), while [`ARCHITECTURE.md`](./ARCHITECTURE.md) describes module boundaries.

| Concern | Implementation |
|---|---|
| Adaptive query tiering, validation, stats, snapshot/warmup, prefetch integration | [`adaptive_query_cache.cpp`](adaptive_query_cache.cpp) |
| Bounded in-memory LRU | [`bounded_lru_cache.cpp`](bounded_lru_cache.cpp) |
| Embedding similarity lookup | [`embedding_cache.cpp`](embedding_cache.cpp) |
| Prompt/parameter keyed persistent semantic results | [`semantic_cache.cpp`](semantic_cache.cpp) |
| Eviction policy and listener behavior | [`cache_eviction_policy.cpp`](cache_eviction_policy.cpp), [`eviction.cpp`](eviction.cpp), [`eviction_listener.cpp`](eviction_listener.cpp) |
| Replication events and local/remote fanout | [`cache_replication.cpp`](cache_replication.cpp), [`cache_replication_coordinator.cpp`](cache_replication_coordinator.cpp) |
| Redis pub/sub coordinator | [`redis_cache_coordinator.cpp`](redis_cache_coordinator.cpp) |
| gRPC peer adapter | [`grpc_remote_cache_peer.cpp`](grpc_remote_cache_peer.cpp) |
| Hit-rate/latency SLO and alert integration | [`cache_hit_rate_slo_monitor.cpp`](cache_hit_rate_slo_monitor.cpp) |

## Adaptive query cache lifecycle

1. Callers derive a fingerprint with `generateFingerprint()` and pass query parameters, result, tenant context, and optional PII identifiers to the rich `get()` / `put()` APIs.
2. Reads search configured tiers and update per-tier hit or miss/promotion statistics. L1/L2 have explicit count, size, and TTL configuration; L3 is backed by RocksDB.
3. Writes are bounded by configuration. With write-through enabled, a value is written to all applicable tiers; otherwise placement follows the implementation's tier policy and entry constraints.
4. Invalidation can target a pattern, tenant, or PII identifier. If replication is configured, coordination events are sent through the selected coordinator.
5. Optional controls include rate limiting, backpressure, circuit breaking, adaptive TTL, warmup, prefetch, and replication. Their `Config` switches are not all enabled by default.

The simple `ICacheBackend<std::string, nlohmann::json>` adapter is explicitly documented in the header as using the empty/default tenant. Production integrations that require tenant isolation should use the rich API and supply the tenant context consistently.

## Persistence, expiry, and concurrency

- `SemanticCache` computes a SHA-256 key from the prompt concatenated with serialized parameters and stores JSON values in the caller-supplied RocksDB database/column family. The database and column-family pointers are non-owning; their owners must outlive the cache.
- Semantic entries use TTL metadata and can be expired by explicit `clearExpired()` or the optional background expiry thread. The destructor signals and joins that thread.
- `EmbeddingCache` uses an internal pimpl and a shared mutex: reads can share the lock, while store/clear operations take the exclusive lock. Its vector-index path and fallback scan are described in [`EMBEDDING.md`](./EMBEDDING.md).
- Do not assume all cache types share one persistence, threading, or invalidation mechanism. Check the owning class header before adding cross-type behavior.

## Failure handling invariants

Use the stable failure taxonomy in [`cache_contract.h`](../../include/cache/cache_contract.h): malformed key/value, invalid operation, tenant violation, degraded backend, partial delivery, and internal error. Invalid input must not be converted to a successful cache hit. Backend and coordinator failures must remain diagnosable; the contract does not permit unbounded retry or silent success for unsafe operations.

Replication is asynchronous for remote peers. The local operation may complete while peer delivery is queued, retried, dropped at capacity, or ultimately failed. Surface that distinction through coordinator statistics and diagnostics; do not claim remote consistency from local cache success.

## Validation map

The focused cache tests are generated as CMake targets from [`tests/cache/CMakeLists.txt`](../../tests/cache/CMakeLists.txt). Relevant suites cover tenant isolation, coordinator degradation, multilevel behavior, warmup, replication, contract hardening, and SLO monitoring. Performance expectations and benchmark case names are listed in [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md).

## Related documentation

- [Public API guide](./API.md)
- [Module integration guide](./MODULE_GUIDE.md)
- [Operations and recovery](./OPERATIONS.md)
- [Trust boundaries](./TRUST.md)
