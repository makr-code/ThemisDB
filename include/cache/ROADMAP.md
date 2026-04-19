<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · AUDIT.md · SECURITY.md · FUTURE_ENHANCEMENTS.md -->

# Roadmap — Cache Module Public Headers

**Module Path:** `include/cache/`
**Implementation Roadmap:** `../../src/cache/ROADMAP.md`

---

## Current Status

Public headers at v2.0.0. The L1 lock-free shared-mutex read path is now exposed via
`adaptive_query_cache.h`. All five abstract interfaces in `cache_interfaces.h` are
fully implemented and tested. TinyLFU L1 policy added. Phase 6 distribution
headers (`IDistributedEviction`, `ICachePartition`, `IAdaptiveTTLPolicy`) delivered
and covered by `tests/test_cache_phase6_interfaces.cpp`.

---

## Completed Features

- [x] `AdaptiveQueryCache` L1/L2/L3 multi-level façade
- [x] `IEvictionPolicy`, `ICacheAdminOps`, `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter`
- [x] `BoundedLRUCache` with `std::shared_mutex` L1 reads
- [x] `ARCCache` and `L1TinyLFUCache` eviction policy headers
- [x] `SemanticCache` and `EmbeddingCache`
- [x] `CacheHitRateSloMonitor` with configurable SLO and p50/p95/p99 latency tracking
- [x] `CacheReplicationManager` for HA
- [x] `RedisCacheCoordinator` with HMAC-SHA256 signing
- [x] `GRPCRemoteCachePeer` for gRPC remote invalidation
- [x] `RequestCoalescer` for concurrent miss deduplication — **real Singleflight implementation (2026-04-12)**
  - `promise/shared_future` inflight map; fn() called once per in-flight key
  - Exception from fn() propagated as `success=false` to all waiters
  - 14 tests (RC-01…RC-14) in `tests/test_request_coalescer.cpp`
- [x] `PredictivePrefetcher`
- [x] `AlignedVectorAllocator` for SIMD-aligned embedding storage
- [x] `IDistributedEviction` for cross-node coordinated eviction — `distributed_eviction.h`
- [x] `ICachePartition` for sharded per-tenant partition management — `cache_partition.h`
- [x] `IAdaptiveTTLPolicy` for stateful history-driven TTL adaptation — `adaptive_ttl_policy.h`

---

## Planned Features

- [x] Redis TLS enforcement at header level (Target: Q3 2026)
- [x] L3 encryption enforcement at header level (Target: Q3 2026)

---

## Implementation Phases

### Phase 1: Core Cache Interfaces
- [x] `AdaptiveQueryCache`, `BoundedLRUCache`, `SemanticCache`, `EmbeddingCache`

### Phase 2: Abstract Extension Interfaces
- [x] 5 interfaces in `cache_interfaces.h`

### Phase 3: Eviction Policy Headers
- [x] `ARCCache`, `L1TinyLFUCache`, `eviction_policy.h` factory

### Phase 4: Distributed & HA Headers
- [x] Replication, Redis coordinator, gRPC peer, request coalescer

### Phase 5: Lock-Free Read Path
- [x] `std::shared_mutex` L1 promotion in v1.9.0

### Phase 6: Distribution & Partitioning Headers
- [x] `IDistributedEviction` — cross-node eviction broadcast with `evict()`, `evictByPattern()`, `evictByTenant()`, `flush()`, listener registration, and observable stats
- [x] `ICachePartition` — sharded per-tenant partition manager with `assignTenant()`, `resize()`, `evictPartition()`, `getStats()`, and `getAllStats()`
- [x] `IAdaptiveTTLPolicy` — stateful TTL policy with access history tracking, `recordAccess()`, `computeTTL()`, `pruneHistory()`, and `AdaptiveTTLSuggestion` output

---

## Production Readiness Checklist

- [x] All 5 abstract interfaces have 43+ unit tests
- [x] GDPR purge hook present
- [x] HMAC-signed Redis invalidation
- [x] Lock-free L1 reads
- [x] Phase 6 distribution headers covered by `tests/test_cache_phase6_interfaces.cpp`
- [x] L3 encryption enforcement at header level (currently operator-managed)
- [x] Redis TLS enforcement at header level
