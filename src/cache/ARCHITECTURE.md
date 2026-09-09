# Architecture - Cache Module

<!-- Status: current | validated: 2026-09-09 -->
<!-- Links: README.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md -->

## Overview

The cache module composes query and embedding cache behaviors into bounded, observable runtime layers. It provides multi-tier data reuse with tenant isolation, invalidation/warmup flows, and distributed coordination for multi-node scenarios.

## Main Execution Planes

1. Multi-tier cache plane
- adaptive query cache through L1/L2/L3 style layers
- embedding and semantic cache execution paths

2. Isolation and policy plane
- tenant-aware keying and quota-aware runtime checks
- invalidation, circuit behavior, and bounded failure semantics

3. Coordination and replication plane
- in-process and remote coordination support
- replication event distribution and consistency hooks

4. Operations and observability plane
- warmup and predictive prefetch paths
- SLO monitoring and runtime metrics surfaces

## Core Contracts

| Contract | Behavior |
|---|---|
| cache facade interfaces | provide get/put/invalidate style cache semantics |
| isolation interfaces | enforce tenant and bounded access constraints |
| coordination interfaces | propagate invalidation/replication intents safely |
| observability interfaces | expose health/SLO and operational counters |

## Failure Semantics

- invalid or incompatible cache operations fail with structured error behavior.
- degraded coordination backends surface explicit runtime degradation.
- invalidation/warmup failures remain bounded and observable.

## Module Dependencies

### Direct Upstream Dependencies (this module uses)
| Module | Interface / File | Purpose |
|--------|-----------------|---------|
| utils | `include/utils/` (hashing, time, concurrency helpers) | Cache key hashing, TTL computation, and internal locking primitives |

### Direct Downstream Consumers (modules that use this module)
| Module | Via | Notes |
|--------|-----|-------|
| server | `include/cache/semantic_cache.h`, `include/cache/adaptive_query_cache.h`, `include/cache/cache_hit_rate_slo_monitor.h` | Semantic result reuse and SLO monitoring for HTTP query responses |
| query | `include/cache/adaptive_query_cache.h` | Plan-level result reuse before execution engine invocation |
| llm | `include/cache/semantic_cache.h`, `include/cache/embedding_cache.h` | Embedding and inference-result caching to reduce model call volume |
| sharding | `include/cache/bounded_lru_cache.h` | Bounded LRU shard-routing metadata cache for consistent-hash ring lookups |

## Integration Points

### Critical Integration: SemanticCache ↔ Server Query Path
**Files:** `include/cache/semantic_cache.h` ↔ `src/server/http_server.cpp`
**Contract:** `SemanticCache::lookup(embedding_key)` returns a cached result or `MISS`; on `MISS` the server executes the full query and calls `SemanticCache::insert()` with the result. Invalidation events (from storage writes) must precede any subsequent `lookup()` seeing stale data.
**Thread Safety:** `SemanticCache` is concurrency-safe via internal `std::shared_mutex`; `insert()` and `lookup()` may be called from any request thread.
**Failure Mode:** Cache backend degradation (Redis unavailable) falls back to `MISS` behavior; the server continues with full execution. A `cache.backend_errors` counter is incremented.

### Critical Integration: AdaptiveQueryCache ↔ Query Module
**Files:** `include/cache/adaptive_query_cache.h` ↔ `src/query/` execution entry point
**Contract:** `AdaptiveQueryCache::get(query_fingerprint)` is called before the query planner; a hit returns a pinned result set that the query layer forwards directly. `put()` is called post-execution with the result and a computed eviction score.
**Thread Safety:** Cache entries are reference-counted; concurrent reads are lock-free, writes serialised per bucket.
**Failure Mode:** Cache corruption or capacity overflow causes silent eviction; the query layer always falls through to execution on a miss.

### Critical Integration: CacheHitRateSloMonitor ↔ Server SLO Reporting
**Files:** `include/cache/cache_hit_rate_slo_monitor.h` ↔ `src/server/` metrics pipeline
**Contract:** `CacheHitRateSloMonitor::record(hit_or_miss)` updates a rolling hit-rate window; `checkSlo()` returns a breach signal consumed by the server's SLO alerting path.
**Thread Safety:** Counter updates are atomic; `checkSlo()` reads are linearisable.
**Failure Mode:** Monitor failure is non-fatal; SLO breach reporting may be delayed but cache operation continues.



- Verified files:
  - src/cache/adaptive_query_cache.cpp
  - src/cache/embedding_cache.cpp
  - src/cache/semantic_cache.cpp
  - src/cache/cache_replication.cpp
  - src/cache/distributed_cache_coordinator.cpp
  - src/cache/redis_cache_coordinator.cpp
  - src/cache/warmup.cpp
  - src/cache/cache_hit_rate_slo_monitor.cpp
- Verified architecture claims:
  - explicit multi-tier, isolation, coordination, and observability planes
  - bounded error behavior around distributed and warmup paths
  - dedicated cache-layer composition for query/embedding reuse