<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · ../../src/cache/ARCHITECTURE.md -->

# Cache Module — Public Header Architecture

**Module Path:** `include/cache/`
**Implementation:** `../../src/cache/`
**Canonical architecture doc:** [`../../src/cache/ARCHITECTURE.md`](../../src/cache/ARCHITECTURE.md)

---

## 1. Overview

`include/cache/` defines the **public multi-tier cache, invalidation, and distributed-coordination contract** for ThemisDB. The 26 headers cover query/result and embedding caches, adaptive TTL and eviction policies, tenant-aware partitioning, warmup/prefetch helpers, remote/distributed coordination, and cache observability.

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

### 2.2 Isolation and Local Structure

| Header | Public Type | Purpose |
|--------|------------|---------|
| `cache_partition.h`, `cache_provider.h`, `cache_interfaces.h` | Partition/provider interfaces | Tenant-aware partitioning and provider abstraction |
| `aligned_vector_allocator.h`, `request_coalescer.h` | Utility types | Allocation and duplicate-request coalescing |
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
| `cache_hit_rate_slo_monitor.h` | `CacheHitRateSLOMonitor` | Hit-rate SLO monitoring |

---

## 3. Namespace Layout

| Namespace | Scope |
|-----------|-------|
| `themis::cache` | Cache, invalidation, and coordination types |

---

## 4. Public Contract Notes

- Cache facade headers remain public so query, server, and application layers can embed cache operations directly.
- Provider, partition, and coordinator interfaces expose explicit boundaries between local cache tiers and distributed invalidation/replication backends.
- Security-relevant backend config headers remain public because deployers must configure encrypted/TLS-backed cache layers.
- SLO and metrics headers provide stable observability surfaces for runtime operations and admin endpoints.
