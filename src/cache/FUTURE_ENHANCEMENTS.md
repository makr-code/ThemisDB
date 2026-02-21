# Cache Module - Future Enhancements

## Scope

This document covers implementation-specific future enhancements for the Cache module (`src/cache/`), comprising `adaptive_query_cache.cpp` (multi-level L1/L2/L3 pipeline, 1,259 lines), `semantic_cache.cpp` (vector-similarity result caching, 294 lines), `bounded_lru_cache.cpp`, and `embedding_cache.cpp`. Enhancements to the underlying `storage/rocksdb_wrapper.h` (L3 backing store) and `utils/zstd_codec.h` (L2 compression) are out of scope except where the cache module controls their configuration. The Admin API, warmup, and tenant management features listed in the ROADMAP are the primary focus.

## Design Constraints

- `[ ]` L1 and L2 in-memory tiers must stay lock-free on the read path; no new `std::mutex` acquisitions may be introduced on `AdaptiveQueryCache::get()`.
- `[ ]` The `cache::CircuitBreaker` protecting L3 RocksDB must remain the sole fault-isolation mechanism for the persistence tier; new L3 features must check breaker state before every operation.
- `[ ]` Per-tenant quotas enforced via `config_.per_tenant_max_bytes` must not be bypassable by any new Admin API write path.
- `[ ]` Serialization format for L2 compressed entries (`zstd_codec`) and L3 RocksDB keys (`QUERY_CACHE_PREFIX`) must remain stable across minor versions; breaking format changes require a cache flush on upgrade.

## Required Interfaces

| Interface | Consumer | Notes |
|-----------|----------|-------|
| `AdaptiveQueryCache::get(key, tenant_id)` | AQL executor, server request handler | Must remain O(1) average on L1 hit path |
| `AdaptiveQueryCache::put(key, value, ttl, tenant_id)` | AQL executor | Rate limiter gating enforced before write |
| `cache::CircuitBreaker` state machine | `AdaptiveQueryCache` L3 path | CLOSED/OPEN/HALF_OPEN transitions must be observable via metrics |
| `SemanticCache::findSimilar(embedding, threshold)` | LLM query handler in `content/content_manager_llm.cpp` | Cosine threshold configurable per-call |
| `cache::RateLimiter` | `AdaptiveQueryCache::put()` | Token bucket; refill rate and burst configurable |
| `EmbeddingCache` | `SemanticCache`, vector search paths | Must evict LRU entries when `embedding_cache.cpp` capacity is reached |

## Planned Features

### Admin HTTP API for Cache Operations
**Priority:** High
**Target Version:** v1.7.0

Implement a REST Admin API (planned in ROADMAP Phase 3) that exposes cache inspection, per-key eviction, tenant quota status, and circuit breaker control. The API must be protected by the existing `auth::JWTValidator` with a dedicated `admin:cache` scope claim.

**Implementation Notes:**
- `[ ]` Create `cache_admin_handler.cpp`; register routes under `/v1/admin/cache/` in `src/server/http_server.cpp`.
- `[ ]` `GET /v1/admin/cache/stats` — returns JSON snapshot of `AdaptiveQueryCache::Metrics` (L1/L2/L3 hit rates, eviction counts, circuit breaker state, per-tenant quota usage).
- `[ ]` `DELETE /v1/admin/cache/key/{encoded_key}` — evict a specific entry from all tiers; base64-encode key in path.
- `[ ]` `DELETE /v1/admin/cache/tenant/{tenant_id}` — evict all entries for a tenant via L3 pattern-based invalidation (already implemented in `adaptive_query_cache.cpp`).
- `[ ]` `POST /v1/admin/cache/circuit-breaker/reset` — force circuit breaker back to CLOSED; requires `admin:cache:write` scope.
- `[ ]` `GET /v1/admin/cache/circuit-breaker` — returns current state, failure count, and last failure time.

**Performance Targets:**
- Admin API read endpoints (stats, circuit breaker) respond in < 5 ms regardless of L1 cache size.
- Tenant-level eviction of 100,000 L3 entries completes in < 2 s (uses existing iterator-based scan).

**API Sketch:**
```json
// GET /v1/admin/cache/stats — example response
{
  "l1": { "entries": 4821, "hits": 982301, "misses": 12034, "evictions": 301 },
  "l2": { "entries": 41200, "hits": 5021, "misses": 8803, "compression_ratio_avg": 4.2 },
  "l3": { "hits": 1023, "misses": 6201, "circuit_breaker": "CLOSED", "errors": 0 },
  "rate_limiter": { "allowed": 998301, "throttled": 204 },
  "tenants": [
    { "id": "acme", "bytes_used": 10485760, "quota_bytes": 104857600 }
  ]
}
```

---

### Cache Warmup from Query Logs and Snapshots
**Priority:** High
**Target Version:** v1.7.0

After a node restart, L1 and L2 are cold; queries that were hot before restart incur L3 or full re-execution latency. Implement a warmup path that replays a query log or imports a cache snapshot to pre-populate L1/L2.

**Implementation Notes:**
- `[ ]` Add `AdaptiveQueryCache::warmupFromLog(const std::string& log_path, size_t max_entries)` method.
- `[ ]` Log format: newline-delimited JSON, each line `{"key":"<sha256>","value_b64":"<base64>","ttl_remaining_s":300,"tenant":"acme"}`.
- `[ ]` Warmup path bypasses `cache::RateLimiter` (internal operation) but honours per-tenant quota checks.
- `[ ]` Cap warmup at `config_.l1_max_entries / 2` to leave headroom for live traffic; excess entries go to L2.
- `[ ]` Add `AdaptiveQueryCache::exportSnapshot(const std::string& out_path)` for pre-shutdown snapshot export.
- `[ ]` Expose warmup progress via a Prometheus gauge `themis_cache_warmup_entries_loaded_total`.

**Performance Targets:**
- Warmup of 50,000 L1 entries from disk log in < 10 s on commodity SSD.
- Zero lock contention with concurrent live reads during warmup (use background thread with yield points).

---

### Adaptive TTL Tuning Based on Access Patterns
**Priority:** Medium
**Target Version:** v1.8.0

Currently TTL is set at `put()` time and never adjusted. Implement a background tuner that observes per-key access frequency and adjusts TTL on promotion from L2→L1 or L3→L2, extending it for frequently accessed entries and shortening it for cold ones.

**Implementation Notes:**
- `[ ]` Add `AccessFrequencyTracker` struct inside `adaptive_query_cache.cpp`; count accesses per key in a sliding 5-minute window using a circular buffer.
- `[ ]` Tuning policy: if key accessed ≥ 10 times in the window, multiply TTL by 1.5 (capped at `config_.max_ttl_seconds`); if ≤ 1 time, multiply by 0.5 (floored at `config_.min_ttl_seconds`).
- `[ ]` Run tuner in the existing `eviction_thread_` (already present in `adaptive_query_cache.cpp`); tune batch of up to 1,000 keys per eviction cycle.
- `[ ]` Expose tuner adjustments in metrics: `themis_cache_ttl_extended_total`, `themis_cache_ttl_shortened_total`.
- `[?]` Decision needed: whether adaptive TTL should be disabled per-tenant when tenant has strict data-freshness SLAs.

**Performance Targets:**
- Adaptive tuning reduces L3 read rate by ≥ 20% for workloads with 80/20 hot-key distribution (measured via replay of production query logs).
- Tuner loop overhead < 1% of eviction thread CPU time.

---

### Configurable Eviction Policies (LFU and ARC)
**Priority:** Medium
**Target Version:** v1.8.0

`bounded_lru_cache.cpp` implements LRU only. For scan-heavy workloads (large analytics queries visiting all cache entries), LRU causes cache pollution. Add Least-Frequently-Used (LFU) and Adaptive Replacement Cache (ARC) policies selectable via config.

**Implementation Notes:**
- `[ ]` Define `EvictionPolicy` enum `{LRU, LFU, ARC}` in `cache/eviction_policy.h`.
- `[ ]` Implement `LFUCache<K,V>` using a frequency-bucket doubly-linked list (O(1) insert/evict).
- `[ ]` Implement `ARCCache<K,V>`: maintain T1 (recency), T2 (frequency), B1, B2 ghost lists; adapt partition `p` on hits/misses.
- `[ ]` `AdaptiveQueryCache::Config` gains `l1_eviction_policy` and `l2_eviction_policy` fields (default: `LRU` for backward compatibility).
- `[ ]` `bounded_lru_cache.cpp` is refactored to implement the `IEvictionCache<K,V>` interface; `LFUCache` and `ARCCache` implement the same interface.

**Performance Targets:**
- ARC policy achieves ≥ 10% higher hit rate than LRU on a scan-heavy TPC-H query replay workload.
- LFU/ARC overhead vs LRU ≤ 5% on `AdaptiveQueryCache::get()` microbenchmark.

---

### Distributed Cache Coordination (Redis-Compatible Protocol)
**Priority:** Low
**Target Version:** v2.0.0

For multi-node deployments, L1/L2 caches are node-local, causing inconsistent results after writes. Add an optional distributed coordination layer that broadcasts invalidation messages over a Redis pub/sub channel or a native ThemisDB cluster bus.

**Implementation Notes:**
- `[ ]` Create `distributed_cache_coordinator.cpp`; implement `ICacheCoordinator` interface with `publishInvalidation(key, tenant_id)` and `subscribeInvalidations(callback)`.
- `[ ]` Redis transport: use `hiredis` async API; connect pool size configurable (`config_.redis_pool_size`, default 4).
- `[ ]` On `AdaptiveQueryCache::invalidate()`, call `ICacheCoordinator::publishInvalidation()` if coordinator is registered.
- `[ ]` On receiving remote invalidation, evict matching key from L1 and L2 only (L3 is shared RocksDB; skip).
- `[ ]` Graceful degradation: if coordinator connection drops, log warning and continue with node-local invalidation only.
- `[!]` Native cluster bus variant (no Redis dependency) is unclear; defer design until clustering architecture is decided.

**Performance Targets:**
- Invalidation broadcast latency < 5 ms (p99) within a 3-node cluster on same LAN.
- Zero false-evictions from coordinator message replay (idempotent invalidation).

## Test Strategy

| Test Type | Coverage Target | Notes |
|-----------|----------------|-------|
| Unit | >80% new code | Test `LFUCache` and `ARCCache` with hit-rate oracle; test `AccessFrequencyTracker` with synthetic access patterns; mock `IEvictionCache` for Admin API handler tests |
| Integration | L1/L2/L3 pipeline with warmup; Admin API end-to-end | `tests/cache/cache_integration_test.cpp`; add warmup and evict-all-tenant tests |
| Performance | Hit rate regression ≤ 1% vs baseline on 10K-query replay | `benchmarks/cache_bench.cpp` with production query log fixture |

## Performance Targets

| Metric | Current | Target | Method |
|--------|---------|--------|--------|
| L1 cache get latency (hit) | ~200 ns | < 200 ns (no regression) | `benchmarks/cache_bench.cpp` microbench |
| L3 circuit-breaker trip latency | < 1 ms | < 500 µs | `tests/cache/circuit_breaker_test.cpp` |
| Warmup 50K entries from disk | N/A | < 10 s | `tests/cache/warmup_bench.cpp` |
| Tenant eviction 100K L3 entries | < 5 s (estimate) | < 2 s | `tests/cache/eviction_bench.cpp` |
| Adaptive TTL L3 read reduction | Baseline | ≥ 20% | Replay benchmark with hot-key fixture |

## Security / Reliability

- `[ ]` Admin API write endpoints (`DELETE`, `POST`) must require `admin:cache:write` JWT scope; read endpoints require `admin:cache:read`; scope checks enforced at handler entry before any cache mutation.
- `[ ]` Distributed coordinator must validate that invalidation messages carry the originating node's signed token to prevent cache-flush attacks from unauthenticated nodes.
- `[ ]` `AdaptiveQueryCache::warmupFromLog()` must validate each entry's key is a valid SHA-256 hex string and value size does not exceed `config_.l1_max_entry_bytes` (currently 1 KB) before insertion.
