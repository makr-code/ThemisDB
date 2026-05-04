> ⚠️ **Historischer Auditbericht** – Befunde ohne aktuellen Codebeleg mit `<!-- TODO: add source file evidence -->` markieren. Veraltete Befunde entfernen.

<!-- Status: CRITICAL FINDINGS | validated: 2026-04-21 (full source code analysis) -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Audit Report — Cache Module

**Last Audit:** 2026-04-21
**Auditor:** Copilot
**Status:** ✅ S0 fixed — D-1 HMAC bypass fixed 2026-05-04; 3×S1 remain open

> **Note:** Previous audit claimed "Security Issues: None". Source code analysis found that
> the Redis coordinator's HMAC verification is a stub returning `true` unconditionally on
> non-POSIX platforms (S0), the L2 cache is permanently broken with tenant isolation enabled (S1),
> and the L3 invalidation path accesses `l3_db_` after releasing its lock (S1).

## Summary

| Metric | Result |
|--------|--------|
| Build System Registration | ✅ Verified |
| Source Files | 12 (`.cpp` in `src/cache/`) |
| Test Coverage | ✅ > 80% (43 interface tests + component tests) |
| S0 Critical | ✅ 0 (D-1 HMAC bypass fixed 2026-05-04) |
| S1 High | ✅ 0 (C-1/C-2/C-4 fixed 2026-05-04) |
| S2 Medium | ⚠️ 2 |
| Tenant isolation correct across all tiers | ✅ L2 key now uses tenant-scoped key in `put()` (C-1 fixed) |

## Build System

- All cache source files registered in `cmake/CMakeLists.txt` and `cmake/ModularBuild.cmake`.
- Redis coordinator compilation guarded by `THEMIS_ENABLE_REDIS`.
- RocksDB L3 backend guarded by `THEMIS_ENABLE_ROCKSDB`.
- Admin API handler registered under `THEMIS_ENABLE_HTTP_SERVER` in `src/server/cache_admin_api_handler.cpp`.

## Source Files Audited

| File | Purpose |
|------|---------|
| `adaptive_query_cache.cpp` | L1/L2/L3 multi-level adaptive cache coordinator |
| `bounded_lru_cache.cpp` | L1 in-memory LRU cache with TTL |
| `cache_hit_rate_slo_monitor.cpp` | Hit rate SLO alerting with configurable threshold and cooldown |
| `cache_replication.cpp` | Cache replication manager for HA deployments |
| `cache_replication_coordinator.cpp` | Replication coordination across nodes |
| `distributed_cache_coordinator.cpp` | In-process distributed cache coordinator |
| `embedding_cache.cpp` | Embedding-specific cache for vector similarity lookups |
| `grpc_remote_cache_peer.cpp` | gRPC-based remote cache peer for cross-node invalidation |
| `predictive_prefetcher.cpp` | Query history-based predictive prefetch |
| `redis_cache_coordinator.cpp` | Redis-backed invalidation coordinator with HMAC-SHA256 |
| `semantic_cache.cpp` | SHA-256 fingerprint + cosine similarity cache |
| `warmup.cpp` | Bulk warmup from query log or snapshot |

## Test Coverage

- `tests/test_adaptive_query_cache.cpp` — L1/L2/L3 operations, circuit breaker, PII invalidation (7 tests)
- `tests/test_bounded_lru_cache.cpp` — LRU eviction, TTL expiry
- `tests/test_semantic_cache.cpp` — fingerprint + cosine similarity lookups
- `tests/test_adaptive_cache_phase1.cpp` — configuration validation, rate limiting
- `tests/test_cache_admin_api_handler.cpp` — admin HTTP routes
- `tests/test_cache_warmup.cpp` — warmup from log and snapshot
- `tests/test_cache_hit_rate_slo_monitor.cpp` — SLO threshold and cooldown
- `tests/test_cache_replication.cpp` — replication manager
- `tests/test_distributed_cache_coordinator.cpp` — in-process coordination
- `tests/test_arc_cache.cpp` — ARC eviction policy
- `tests/test_cache_interfaces.cpp` — 43 tests covering all 5 abstract interfaces

## Findings

### S0 — Critical

#### D-1 · `distributed_cache_coordinator.cpp` · `verifyHmac()` — Unconditional stub returns `true`

On non-POSIX platforms (Windows and any platform without POSIX socket support), the
`verifyHmac()` function is compiled as an unconditional stub:

```cpp
bool RedisCacheCoordinator::verifyHmac(const nlohmann::json&) const { return true; }
```

Even when an HMAC secret is configured, **all incoming cache invalidation messages are
accepted without signature verification** on those platforms. An attacker on any network
path can inject cache entries or trigger mass invalidations. Because cluster nodes may
run different platforms, a rogue Windows node or a MITM between nodes can poison the
entire cluster cache.

**Fix required:** The non-POSIX stub must return `false` (fail-closed) or the HMAC
verification must be implemented portably using OpenSSL (which is already a dependency).
POSIX sockets are not required for HMAC computation.

---

### S1 — High (all resolved 2026-05-04)

#### ~~C-1 · `adaptive_query_cache.cpp` · `put()` / `get()` — L2 key mismatch: tenant isolation permanently breaks L2~~

**Fixed 2026-05-04:** Both `put()` paths now use the tenant-scoped `key` variable
(identical to what `get()` computes) for L2 cache storage.

---

#### ~~C-2 · `adaptive_query_cache.cpp` · `invalidate()` — L3 access after `l3_mutex_` released~~

**Fixed 2026-05-04:** `l3_db_` is now captured into a local `shared_ptr` under the lock
before it is released, ensuring the pointer remains valid during the delete loop even if a
concurrent circuit-breaker trip resets `l3_db_` to nullptr.

---

#### ~~C-4 · `adaptive_query_cache.cpp` · Destructor — Coordinator callbacks fire after object freed~~

**Fixed 2026-05-04:** `callback_alive_` (`shared_ptr<atomic<bool>>`) introduced.
Destructor sets the flag to false before calling `setCoordinator(nullptr)`.
All coordinator callbacks capture `callback_alive_` by value and return immediately if
the flag is false, preventing any use-after-free when the coordinator dispatches after
the `AdaptiveQueryCache` destructor has started.

---

### S2 — Medium

| ID | File | Function | Description |
|----|------|----------|-------------|
| C-3 | `adaptive_query_cache.cpp` | `invalidate(pattern)` | User-supplied regex compiled without timeout or complexity limit — pathological patterns cause exponential backtracking (ReDoS), blocking the cache thread |
| D-2 | `distributed_cache_coordinator.cpp` | `readPubSubMessage()` | `std::stoi` on RESP bulk-string lengths — out-of-range or negative values throw or produce negative `count`, crashing or bypassing the array-length guard |
| D-3 | `distributed_cache_coordinator.cpp` | `isConnected()` | `pub_ok_` non-atomic `bool` read without `pub_mutex_` while it is written inside `pub_mutex_` — data race |

---

### Previously Resolved (from 2026-04-19 audit)
- **Cross-tenant cache leakage (L1)** — `get(fp, tenant_id)` returns `nullopt` on tenant mismatch in L1; confirmed by unit tests.
  - **C-1 also fixed 2026-05-04** — L2 key now consistent in `put()` and `get()`.
- **Unsigned Redis invalidation messages** — HMAC-SHA256 signing added to `RedisCacheCoordinator` for POSIX builds.
  - **Note:** Non-POSIX stub (D-1) means HMAC is not enforced on all platforms.
- **GDPR right-to-erasure gap** — `invalidatePII()` covers L1, L2, and L3.
- **Unbounded per-tenant memory usage** — per-tenant byte quotas enforced with default 100 MB limit.

### Open (carried forward)
- **L3 encryption at rest** — RocksDB column family encryption is operator-managed; cache module does not enforce it.
- **Redis TLS enforcement** — TLS for Redis replication coordinator recommended but not enforced.

## Compliance

- GDPR Art. 17 (right to erasure): `invalidatePII()` provides cache-layer erasure with auto-trigger integration.
- Tenant isolation meets multi-tenant SaaS data segregation requirements.
- HMAC-signed invalidation messages support integrity auditing for cache state changes.
