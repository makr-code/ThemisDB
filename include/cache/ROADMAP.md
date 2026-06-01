> **Build:** `cmake --preset linux-release && cmake --build --preset linux-release`

<!-- Status: current | validated: 2026-06-01 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · ../../src/cache/ROADMAP.md -->

# Cache Module — Public Header Roadmap

**Module Path:** `include/cache/`
**Canonical implementation roadmap:** [`../../src/cache/ROADMAP.md`](../../src/cache/ROADMAP.md)

---

## Overview

Tracks public cache API contract stability, distributed-cache header coverage, and planned public entry points. Runtime implementation work remains in:

→ [`../../src/cache/ROADMAP.md`](../../src/cache/ROADMAP.md)

---

## Current Status

All 26 cache headers are present and cover adaptive query/result caches, embedding/semantic caching, tenant-aware partitioning, distributed invalidation/replication, and cache observability.

---

## Completed ✅

- [x] local/cache-facade headers — `adaptive_query_cache.h`, `enhanced_query_cache.h`, `result_cache.h`, `embedding_cache.h`, `semantic_cache.h`
- [x] policy/utility headers — `adaptive_ttl_policy.h`, `eviction_policy.h`, `request_coalescer.h`, `aligned_vector_allocator.h`
- [x] distributed coordination headers — `distributed_cache_coordinator.h`, `redis_cache_coordinator.h`, `cache_replication.h`, `cache_replication_coordinator.h`, `grpc_remote_cache_peer.h`
- [x] observability headers — `cache_metrics.h`, `cache_hit_rate_slo_monitor.h`, `predictive_prefetcher.h`

---

## In Progress

- [ ] Clarify degraded-backend and partial-coordination expectations across distributed coordinator headers (Target: 2026-Q3)
- [ ] Add stronger tenant-isolation and invalidation-failure guidance in public cache provider docs (Target: 2026-Q3)

---

## Planned

- [ ] `cache_incident.h` — shared incident/diagnostic DTO for invalidation and backend failures (Target: 2026-Q4)
- [ ] `cache_capability_profile.h` — backend and feature-capability summary contract (Target: 2026-Q4)
- [ ] Document benchmark-backed compatibility notes for cache hot paths and tenant-aware operations (Target: 2026-Q4)

---

## Breaking Change History

None in v1.x. Public cache headers must remain backward compatible within the active major line; contract changes require migration notes and changelog updates.
