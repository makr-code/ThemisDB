<!-- Status: current | validated: 2026-10-31 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/cache/ARCHITECTURE.md -->
<!-- AI_Used: true | AI_Models: unknown | AI_Contributor: GitHub Copilot Coding Agent | Human_Review: false | Human_Author: ThemisDB Team
Exact model backend not verifiable from final document text alone. -->

# Cache Module — Public Header Architecture

**Module Path:** `include/cache/`
**Implementation:** `../../src/cache/`
**Canonical architecture doc:** [`../../src/cache/ARCHITECTURE.md`](../../src/cache/ARCHITECTURE.md)

---

## 1. Overview

`include/cache/` defines the **public cache, invalidation, and distributed-coordination interfaces** for ThemisDB. The directory currently contains 33 `.h`/`.hpp` files covering query/result and embedding caches, policies, tenant-aware partitioning, warmup/prefetch helpers, coordination, and cache observability. A header's presence does not imply that every backend or feature is enabled at runtime.

For runtime composition details — cache tiers, replication/coordination internals, and SLO monitoring — see:
→ [`../../src/cache/ARCHITECTURE.md`](../../src/cache/ARCHITECTURE.md)

---

## 2. Header Groups

### 2.1 Core Cache Facades and Policies

| Header | Public Type | Purpose |
|--------|------------|---------|
| `adaptive_query_cache.h`, `enhanced_query_cache.h`, `result_cache.h` | Query cache types | Query/result cache facades |
| `embedding_cache.h`, `semantic_cache.h` | Embedding/semantic cache types | Embedding-aware cache contracts |
| `arc_cache.h`, `bounded_lru_cache.h`, `l1_tinylfu_cache.h` | Local cache types | Bounded in-memory cache tiers |
| `adaptive_ttl_policy.h`, `eviction_policy.h` | Policy types | TTL and eviction control |
| `cache_contract.h`, `cache_eviction_policy.h` | `CacheFailureClass`, eviction policy types | Shared failure classification and cache eviction policies |
| `eviction.h`, `eviction_listener.h` | `EvictionScheduler`, listener interfaces | Eviction scheduling and event notification |
| `lru_cache.h`, `lru_cache.hpp` | `LRUCache` | Generic LRU cache declaration and template implementation |

### 2.2 Isolation and Local Structure

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cache_partition.h`, `cache_provider.h`, `cache_interfaces.h` | Partition/provider interfaces | Tenant-aware partitioning and provider abstraction |
| `aligned_vector_allocator.h`, `request_coalescer.h` | Utility types | Allocation and duplicate-request coalescing |
| `cache_manager.h` | `CacheManager` | Cache management and event surfaces |
| `l3_encryption_config.h`, `redis_tls_config.h` | Config types | Secure cache backend configuration |

### 2.3 Distributed Coordination and Replication

| Header | Public Type | Purpose |
|--------|------------|---------|
| `distributed_cache_coordinator.h`, `redis_cache_coordinator.h` | Coordinator types | Distributed invalidation and coordination |
| `cache_replication.h`, `cache_replication_coordinator.h` | Replication types | Cache replication intent and orchestration |
| `distributed_eviction.h`, `grpc_remote_cache_peer.h` | Remote/distributed types | Cross-node eviction and peer transport |

### 2.4 Warmup, Prefetch, and Observability

| Header | Public Type | Purpose |
|--------|------------|---------|
| `predictive_prefetcher.h` | `PredictivePrefetcher` | Predictive warmup/prefetch |
| `cache_metrics.h` | `CacheMetrics` | Cache hit/miss and operational counters |
| `cache_hit_rate_slo_monitor.h` | `CacheHitRateSloMonitor` | Hit-rate and latency SLO monitoring |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::cache` | Cache, invalidation, and coordination types |

---

## 4. Public Contract Notes

- Cache facade headers remain public so query, server, and application layers can embed cache operations directly.
- Provider, partition, and coordinator interfaces expose explicit boundaries between local cache tiers and distributed invalidation/replication backends.
- Backend configuration headers are public; their presence does not establish that encryption or TLS is active in a particular deployment.
- SLO and metrics headers provide stable observability surfaces for runtime operations and admin endpoints.
