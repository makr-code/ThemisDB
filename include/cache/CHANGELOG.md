<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Cache Module Public Headers

All notable changes to public headers in `include/cache/`.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]

## [1.9.0] — 2026-03-22
### Changed
- `adaptive_query_cache.h`: `l1_mutex_` promoted to `std::shared_mutex`; `get()` path now
  supports concurrent reads via `std::shared_lock`
- `bounded_lru_cache.h`: `L1Entry` fields converted to `std::atomic`; value type changed
  to `std::unique_ptr<L1Entry>` for stable pointer access under shared lock
- `eviction_policy.h`: `onAccess()` removed from hot read path; `access_count` tracked
  atomically via `fetch_add`; dedicated `l1_eviction_mutex_` added for eviction strategy calls
- `l1_tinylfu_cache.h`: New header — TinyLFU admission filter with W-LRU eviction for L1

## [1.7.0] — 2026-03-09
### Added
- `cache_interfaces.h`: Five abstract interfaces — `IEvictionPolicy`, `ICacheAdminOps`,
  `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter` — with supporting value types
- `grpc_remote_cache_peer.h`: `GRPCRemoteCachePeer` for gRPC-based remote cache invalidation
- `request_coalescer.h`: `RequestCoalescer` for deduplicating concurrent identical cache misses
- `aligned_vector_allocator.h`: `AlignedVectorAllocator<T>` for SIMD-aligned vector storage

### Changed
- `redis_cache_coordinator.h`: HMAC-SHA256 signing added for all pub/sub invalidation messages

## [1.6.0] — 2026-02-01
### Added
- `cache_replication.h`, `cache_replication_coordinator.h`: HA replication headers
- `distributed_cache_coordinator.h`: `IDistributedCacheCoordinator`, `InProcessCacheCoordinator`
- `redis_cache_coordinator.h`: Redis pub/sub invalidation coordinator
- `cache_hit_rate_slo_monitor.h`: `CacheHitRateSloMonitor` with `SLOConfig`
- `predictive_prefetcher.h`: Query-sequence-based predictive prefetch
- `arc_cache.h`: ARC eviction policy header

## [1.5.0] — 2026-01-15
### Added
- Initial public header set: `adaptive_query_cache.h`, `bounded_lru_cache.h`,
  `semantic_cache.h`, `embedding_cache.h`, `eviction_policy.h`
- `cache_provider.h`, `enhanced_query_cache.h`, `result_cache.h`, `cache_metrics.h`
