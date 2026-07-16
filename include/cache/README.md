> **Build:** `cmake --preset release && cmake --build build/release`

# Cache Module — Public Headers

**Module Path:** `include/cache/`
**Implementation:** `../../src/cache/`

## Purpose

Public interfaces and declarations for ThemisDB's multi-layer caching subsystem.

## Canonical Module Documentation

`include/cache/` contains public header contracts. Canonical module behavior, architecture, and operations docs live in `src/cache/`:

- [`../../src/cache/README.md`](../../src/cache/README.md)
- [`../../src/cache/ARCHITECTURE.md`](../../src/cache/ARCHITECTURE.md)
- [`../../src/cache/SECURITY.md`](../../src/cache/SECURITY.md)
- [`../../src/cache/AUDIT.md`](../../src/cache/AUDIT.md)
- [`../../src/cache/PERFORMANCE_EXPECTATIONS.md`](../../src/cache/PERFORMANCE_EXPECTATIONS.md)
- [`../../src/cache/ROADMAP.md`](../../src/cache/ROADMAP.md)
- [`../../src/cache/FUTURE_ENHANCEMENTS.md`](../../src/cache/FUTURE_ENHANCEMENTS.md)
- [`../../src/cache/CHANGELOG.md`](../../src/cache/CHANGELOG.md)

## Header Files

| Header | Primary Class / Interface |
|--------|--------------------------|
| `cache_interfaces.h` | `ICache`, `ICacheProvider` — abstract cache interfaces |
| `cache_provider.h` | `CacheProvider` — factory/registry for cache implementations |
| `cache_metrics.h` | `CacheMetrics` — hit-rate, latency and eviction counters |
| `cache_partition.h` | `CachePartition` — namespace-isolated cache slice |
| `cache_replication.h` | `CacheReplication` — cross-node cache replication interface |
| `cache_replication_coordinator.h` | `CacheReplicationCoordinator` — orchestrates replication peers |
| `eviction_policy.h` | `IEvictionPolicy`, `EvictionPolicyFactory` |
| `arc_cache.h` | `ARCCache` — Adaptive Replacement Cache |
| `bounded_lru_cache.h` | `BoundedLRUCache` — capacity-capped LRU cache |
| `l1_tinylfu_cache.h` | `L1TinyLFUCache` — frequency-based L1 admission filter |
| `enhanced_query_cache.h` | `EnhancedQueryCache` — query-plan result cache |
| `adaptive_query_cache.h` | `AdaptiveQueryCache` — workload-adaptive query cache |
| `result_cache.h` | `ResultCache` — generic query result store |
| `semantic_cache.h` | `SemanticCache` — embedding-based semantic deduplication |
| `embedding_cache.h` | `EmbeddingCache` — vector embedding result cache |
| `predictive_prefetcher.h` | `PredictivePrefetcher` — ML-driven cache prefetcher |
| `distributed_cache_coordinator.h` | `DistributedCacheCoordinator` — cluster-wide cache coordination |
| `distributed_eviction.h` | `DistributedEvictionPolicy` — coordinated cross-node eviction |
| `redis_cache_coordinator.h` | `RedisCacheCoordinator` — Redis-backed cache coordination |
| `redis_tls_config.h` | `RedisTLSConfig` — TLS settings for Redis connections |
| `grpc_remote_cache_peer.h` | `GrpcRemoteCachePeer` — gRPC-backed remote cache peer |
| `request_coalescer.h` | `RequestCoalescer` — deduplicates concurrent identical cache fills |
| `adaptive_ttl_policy.h` | `AdaptiveTTLPolicy` — workload-driven TTL adjustment |
| `aligned_vector_allocator.h` | `AlignedVectorAllocator` — SIMD-aligned allocator for cache buffers |
| `cache_hit_rate_slo_monitor.h` | `CacheHitRateSLOMonitor` — alerting for hit-rate SLO violations |
| `l3_encryption_config.h` | `L3EncryptionConfig` — at-rest encryption settings for L3 tier |

## Usage

```cpp
#include "cache/adaptive_query_cache.h"

themis::cache::AdaptiveQueryCache::Config cfg;
cfg.enable_tenant_isolation = true;
cfg.per_tenant_max_bytes = 100 * 1024 * 1024;

themis::cache::AdaptiveQueryCache cache(cfg);
```

For full runtime usage examples (`put/get`, warmup, admin APIs), see [`../../src/cache/README.md`](../../src/cache/README.md).

## Key Configuration Surface

Important configuration entry points are declared in:

- `adaptive_query_cache.h` (`AdaptiveQueryCache::Config`)
- `redis_cache_coordinator.h` (`RedisCacheCoordinator::Config`)
- `cache_hit_rate_slo_monitor.h` (`CacheHitRateSloMonitor::Config`)
- `l3_encryption_config.h` (`L3EncryptionConfig`)

## Build

```cmake
cmake --preset release && cmake --build build/release --target themis-cache
```

## See Also

- [`../../src/cache/README.md`](../../src/cache/README.md) — implementation details

## Installation

```cmake
target_include_directories(your_target PRIVATE ${THEMISDB_INCLUDE_DIR})
```
