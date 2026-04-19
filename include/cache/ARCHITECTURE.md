<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ROADMAP.md · AUDIT.md · SECURITY.md · FUTURE_ENHANCEMENTS.md -->

# Cache Module — Public Header Architecture

**Version:** 1.9.0
**Last Updated:** 2026-04-06
**Module Path:** `include/cache/`
**Implementation:** `../../src/cache/`

---

## 1. Overview

The `include/cache/` directory exposes public C++ headers for ThemisDB's multi-level adaptive
cache. The public API covers the primary cache interface, eviction policies, admin operations,
GDPR purge hooks, TTL adaptation, SLO monitoring, distributed coordination (Redis + in-process),
embedding cache, semantic cache, and gRPC remote cache peer communication.

---

## 2. Design Principles

- **Interface-Segregated Abstractions** – `cache_interfaces.h` defines five separate interfaces
  (`IEvictionPolicy`, `ICacheAdminOps`, `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter`) to
  allow selective implementation and testing.
- **Tenant Isolation** – all cache interfaces accept a `tenant_id`; cross-tenant access is a
  contract violation.
- **Lock-Free L1 Reads** – `adaptive_query_cache.h` and `bounded_lru_cache.h` support
  `std::shared_mutex` on the L1 tier for concurrent read access.
- **Circuit Breaker** – `adaptive_query_cache.h` exposes the circuit breaker state for L3
  (RocksDB) so callers can observe degraded-mode operation.
- **GDPR Compliance** – `cache_interfaces.h` `IGDPRPurgeHook` and `adaptive_query_cache.h`
  `invalidatePII()` provide right-to-erasure at the cache layer.

---

## 3. Interface Inventory

| Header | Classes / Interfaces | Purpose |
|--------|----------------------|---------|
| `adaptive_query_cache.h` | `AdaptiveQueryCache`, `CacheConfig` | Multi-level L1/L2/L3 cache façade |
| `cache_interfaces.h` | `IEvictionPolicy`, `ICacheAdminOps`, `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter` | Abstract cache extension interfaces |
| `bounded_lru_cache.h` | `BoundedLRUCache<K,V>` | L1 in-memory LRU cache with TTL |
| `arc_cache.h` | `ARCCache<K,V>` | ARC eviction policy implementation |
| `l1_tinylfu_cache.h` | `L1TinyLFUCache<K,V>` | TinyLFU admission + LRU eviction L1 |
| `eviction_policy.h` | `EvictionPolicy` enum, `makeEvictionStrategy()` | Eviction policy factory |
| `semantic_cache.h` | `SemanticCache` | Vector similarity-based semantic cache |
| `embedding_cache.h` | `EmbeddingCache` | Dedicated cache for embedding vectors |
| `cache_provider.h` | `ICacheProvider` | Cache provider injection interface |
| `enhanced_query_cache.h` | `EnhancedQueryCache` | Extended cache with TTL adaptation and metrics |
| `result_cache.h` | `IResultCache`, `ResultCacheEntry` | Query result cache interface |
| `cache_metrics.h` | `CacheMetrics` | Cache metric descriptors |
| `cache_hit_rate_slo_monitor.h` | `CacheHitRateSloMonitor`, `SLOConfig` | Hit-rate SLO alerting |
| `cache_replication.h` | `CacheReplicationManager` | HA cache replication |
| `cache_replication_coordinator.h` | `CacheReplicationCoordinator` | In-process replication coordination |
| `distributed_cache_coordinator.h` | `IDistributedCacheCoordinator`, `InProcessCacheCoordinator` | Distributed coordination |
| `redis_cache_coordinator.h` | `RedisCacheCoordinator` | Redis pub/sub invalidation coordinator |
| `grpc_remote_cache_peer.h` | `GRPCRemoteCachePeer` | gRPC-based remote cache peer |
| `request_coalescer.h` | `RequestCoalescer` | Concurrent request coalescing |
| `predictive_prefetcher.h` | `PredictivePrefetcher` | Query-history predictive prefetch |
| `aligned_vector_allocator.h` | `AlignedVectorAllocator<T>` | SIMD-aligned allocator for vector data |
| `cache_provider.h` | `ICacheProvider` | Dependency injection interface |

> **Implementation details:** `../../src/cache/`
