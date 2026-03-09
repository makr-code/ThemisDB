# Cache Module - Future Enhancements
<!-- Status: current | validated: 2026-03-09 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · docs/de/src/cache/README.md -->

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
- `[x]` Create `cache_admin_handler.cpp`; register routes under `/v1/admin/cache/` in `src/server/http_server.cpp`.
- `[x]` `GET /v1/admin/cache/stats` — returns JSON snapshot of `AdaptiveQueryCache::Metrics` (L1/L2/L3 hit rates, eviction counts, circuit breaker state, per-tenant quota usage).
- `[x]` `DELETE /v1/admin/cache/key/{encoded_key}` — evict a specific entry from all tiers; base64-encode key in path.
- `[x]` `DELETE /v1/admin/cache/tenant/{tenant_id}` — evict all entries for a tenant via L3 pattern-based invalidation (already implemented in `adaptive_query_cache.cpp`).
- `[x]` `POST /v1/admin/cache/circuit-breaker/reset` — force circuit breaker back to CLOSED; requires `admin:cache:write` scope.
- `[x]` `GET /v1/admin/cache/circuit-breaker` — returns current state, failure count, and last failure time.
- `[x]` `GET /v1/admin/cache/health` — returns per-tier status (L1/L2/L3), circuit breaker state, overall health flag.
- `[x]` `POST /v1/admin/cache/warmup` — load cache entries from NDJSON log file.
- `[x]` `POST /v1/admin/cache/snapshot` — export live cache entries to NDJSON file.
- `[x]` `GET /v1/admin/cache/tenants` — list all tenants with aggregated cache statistics.
- `[x]` `GET /v1/admin/cache/tenant/{tenant_id}/stats` — per-tenant statistics (bytes used, quota, hits/misses, evictions).
- `[x]` `PATCH /v1/admin/cache/tenant/{tenant_id}/quota` — update the cache size quota for a tenant.

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
- `[x]` Add `AdaptiveQueryCache::warmupFromLog(const std::string& log_path, size_t max_entries)` method.
- `[x]` Log format: newline-delimited JSON, each line `{"key":"<sha256>","value_b64":"<base64>","ttl_remaining_s":300,"tenant":"acme"}`.
- `[x]` Warmup path bypasses `cache::RateLimiter` (internal operation) but honours per-tenant quota checks.
- `[x]` Cap warmup at `config_.l1_max_entries / 2` to leave headroom for live traffic; excess entries go to L2.
- `[x]` Add `AdaptiveQueryCache::exportSnapshot(const std::string& out_path)` for pre-shutdown snapshot export.
- `[x]` Expose warmup progress via a Prometheus gauge `themis_cache_warmup_entries_loaded_total`.

**Performance Targets:**
- Warmup of 50,000 L1 entries from disk log in < 10 s on commodity SSD.
- Zero lock contention with concurrent live reads during warmup (use background thread with yield points).

---

### Adaptive TTL Tuning Based on Access Patterns
**Priority:** Medium
**Target Version:** v1.8.0
**Status:** ✅ Implemented

**Implementation Notes:**
- `[x]` Adaptive TTL is opt-in via `config_.enable_adaptive_ttl` (default: `false`).
- `[x]` Algorithm uses logarithmic scaling: `adaptive_ttl = base_ttl * (1 + log(access_count + 1) / scaling_factor)`, clamped to `[config_.adaptive_ttl_min_seconds, config_.adaptive_ttl_max_seconds]`.
- `[x]` Access count is incremented on each L1/L2 cache hit; TTL is recalculated and re-applied on the entry.
- `[x]` Parameters are configurable: `adaptive_ttl_min_seconds`, `adaptive_ttl_max_seconds`, `adaptive_ttl_scaling_factor`.
- `[?]` Deferred: per-tenant TTL disabling for tenants with strict data-freshness SLAs — the current implementation does not support per-tenant adaptive TTL opt-out. See Known Issues in `ROADMAP.md`.

**Performance Targets:**
- Adaptive tuning reduces L3 read rate by ≥ 20% for workloads with 80/20 hot-key distribution (measured via replay of production query logs).
- TTL recalculation overhead < 1% of cache hit path CPU time.

---

### Configurable Eviction Policies (LFU and ARC)
**Priority:** Medium
**Target Version:** v1.8.0

`bounded_lru_cache.cpp` implements LRU only. For scan-heavy workloads (large analytics queries visiting all cache entries), LRU causes cache pollution. Add Least-Frequently-Used (LFU) and Adaptive Replacement Cache (ARC) policies selectable via config.

**Implementation Notes:**
- `[x]` Define `EvictionPolicy` enum `{LRU, LFU, ARC}` in `cache/eviction_policy.h`.
- `[x]` Implement `LFUCache<K,V>` using a frequency-bucket doubly-linked list (O(1) insert/evict).
- `[x]` Implement `ARCCache<K,V>`: maintain T1 (recency), T2 (frequency), B1, B2 ghost lists; adapt partition `p` on hits/misses.
- `[x]` `AdaptiveQueryCache::Config` gains `l1_eviction_policy` and `l2_eviction_policy` fields (default: `LRU` for backward compatibility).
- `[x]` `bounded_lru_cache.cpp` is refactored to implement the `IEvictionCache<K,V>` interface; `LFUCache` and `ARCCache` implement the same interface.

**Performance Targets:**
- ARC policy achieves ≥ 10% higher hit rate than LRU on a scan-heavy TPC-H query replay workload.
- LFU/ARC overhead vs LRU ≤ 5% on `AdaptiveQueryCache::get()` microbenchmark.

---

### GDPR-Aware Cache Invalidation (PII Purge Propagation) ✅ Implemented
**Priority:** High
**Target Version:** v1.7.0 — **Status: DONE**

Implements GDPR Art. 17 ("Right to Erasure") propagation from the storage layer to the cache layer.
When `PIIPseudonymizer::erasePII()` is called for a data-subject record, any cached query result
that contains that subject's data must also be purged immediately from all three cache tiers.

**Implemented in `adaptive_query_cache.cpp` / `adaptive_query_cache.h`:**
- `[x]` Extended `put(fingerprint, params, result, tenant_id, pii_uuids = {})` with an optional
  `pii_uuids` vector. When non-empty, the cache key is registered in a per-UUID reverse index
  (`pii_key_index_`, mutex-protected) for L1/L2, and a `pii_ref:{uuid}:{fingerprint}` sentinel
  key is written to RocksDB for L3.
- `[x]` Added `invalidatePII(const std::string& pii_uuid)` which:
  - Reads and clears the L1/L2 reverse-index set for the UUID in a single lock acquisition.
  - Purges matching L1 and L2 entries using the eviction-strategy hooks.
  - Scans the `pii_ref:{uuid}:` prefix in RocksDB, deletes both the sentinel keys and the
    corresponding `query_cache:{fingerprint}` data entries.
  - Respects the L3 circuit breaker; logs a warning when the breaker is open.
  - Emits a structured `THEMIS_INFO` log after every call for operational traceability.
    (Formal GDPR audit entries are written by the caller before invoking this method.)
- `[x]` `clear()` updated to also flush `pii_key_index_` and all `pii_ref:` L3 entries.
- `[x]` 7 unit tests added in `tests/test_adaptive_query_cache.cpp`.

**Remaining follow-up items:**
- `[ ]` Integrate `invalidatePII()` call into `PIIPseudonymizer::erasePII()` so cache purge
  happens automatically on every erasure without requiring caller coordination.
- `[ ]` Expose `DELETE /v1/admin/cache/pii/{pii_uuid}` admin endpoint.

---

### Write-Through Cache Mode
**Priority:** Low
**Target Version:** v2.0.0
**Status:** ✅ Implemented (PR open)

For read-heavy workloads with restart-safety requirements, L1/L2 in-memory entries can be simultaneously persisted to L3 (RocksDB) at write time, eliminating the need for warmup-from-log after a restart.

**Implementation Notes:**
- `[x]` `AdaptiveQueryCache::Config` gains `enable_write_through = false` (opt-in, backward-compatible).
- `[x]` `put()` calls private `writeThroughToL3(fingerprint, params, result, now_ms, ttl_seconds)` after a successful L1 or L2 write.
- `[x]` `writeThroughToL3()` checks the L3 circuit breaker before every write, records `write_through_total` / `write_through_errors` in `CacheMetrics`, and is a no-op when `l3_db_` is null.
- `[x]` The write-through call is performed **outside** the L1/L2 `std::mutex` scope so that RocksDB disk I/O does not block concurrent in-memory reads.
- `[x]` `getDetailedInfo()` exposes `write_through.{enabled, total, errors}` for monitoring.
- `[x]` Initialization log message indicates when write-through mode is active.

**Performance Targets:**
- Write-through adds ≤ RocksDB sync write latency (typically < 2 ms on SSD) to `put()` calls; L1/L2 `get()` latency is unaffected (lock released before L3 write).
- `write_through_errors` should remain 0 under normal operating conditions.

---

### Distributed Cache Coordination (Redis-Compatible Protocol)
**Priority:** Low
**Target Version:** v2.0.0
**Status:** ✅ Implemented (PR open)

For multi-node deployments, L1/L2 caches are node-local, causing inconsistent results after writes. Added an optional distributed coordination layer that broadcasts cache entries and invalidation messages over a Redis pub/sub channel.

**Implementation Notes:**
- `[x]` Created `src/cache/redis_cache_coordinator.cpp`; implements `ICacheCoordinator` interface via `RedisCacheCoordinator` class.
- `[x]` Redis transport: uses `hiredis` synchronous API; publish connection pool size configurable (`config_.pool_size`, default 4); subscribe connection runs on a dedicated background thread.
- `[x]` `publishEntry()` serialises `ENTRY_PUT` messages as JSON and PUBLISHes to `{channel_prefix}:replication` channel; peer nodes pre-populate their L1/L2 caches via the registered `subscribeEntries` callback.
- `[x]` `publishInvalidation()` broadcasts `INVALIDATE` messages; peers evict matching entries from L1/L2 (L3 is shared RocksDB; skip).
- `[x]` Graceful degradation: if Redis connection fails, logs a warning and the local cache operation completes; background thread retries reconnection at `reconnect_interval_ms` intervals.
- `[x]` Self-echo prevention: each message carries a `node_id` field; receivers discard messages from their own node.
- `[~]` Native cluster bus variant (no Redis dependency): covered by existing `InProcessCacheCoordinator` for single-binary deployments; true cluster bus deferred.
- `[x]` Enable via `THEMIS_ENABLE_REDIS=ON` CMake option; compiles to a no-op stub without hiredis.
- `[x]` Unit tests in `tests/test_distributed_cache_coordinator.cpp` (graceful degradation, ICacheCoordinator compliance, stats, channel naming).

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

- `[x]` Admin API write endpoints (`DELETE`, `POST`) must require `admin:cache:write` JWT scope; read endpoints require `admin:cache:read`; scope checks enforced at handler entry before any cache mutation.
- `[ ]` Distributed coordinator must validate that invalidation messages carry the originating node's signed token to prevent cache-flush attacks from unauthenticated nodes.
- `[x]` `AdaptiveQueryCache::warmupFromLog()` must validate each entry's key is a valid SHA-256 hex string and value size does not exceed `config_.l1_max_entry_bytes` (currently 1 KB) before insertion.

---

## Scientific References (IEEE Format)

The following papers underpin the algorithms and design decisions in this module. References are grouped by the feature they support.

### Eviction Policies (LRU, LFU, ARC)

[1] N. Megiddo and D. S. Modha, "ARC: A Self-Tuning, Low Overhead Replacement Cache," in *Proc. 2nd USENIX Conf. on File and Storage Technologies (FAST '03)*, San Francisco, CA, USA, 2003, pp. 115–130. [Online]. Available: https://www.usenix.org/legacy/events/fast03/tech/full_papers/megiddo/megiddo.pdf

[2] S. Jiang and X. Zhang, "LIRS: An Efficient Low Inter-Reference Recency Set Replacement Policy to Improve Buffer Cache Performance," *ACM SIGMETRICS Perform. Eval. Rev.*, vol. 30, no. 1, pp. 31–42, Jun. 2002, doi: 10.1145/511399.511340.

[3] G. Einziger and R. Friedman, "TinyLFU: A Highly Efficient Cache Admission Policy," in *Proc. 22nd Euromicro Int. Conf. on Parallel, Distributed, and Network-Based Processing (PDP)*, Turin, Italy, 2014, pp. 146–153, doi: 10.1109/PDP.2014.34.

[4] W. F. King, "Analysis of Demand Paging Algorithms," in *Proc. IFIP Congress*, Ljubljana, Yugoslavia, 1971, pp. 485–490.

[5] R. L. Mattson, J. Gecsei, D. R. Slutz, and I. L. Traiger, "Evaluation Techniques for Storage Hierarchies," *IBM Syst. J.*, vol. 9, no. 2, pp. 78–117, 1970, doi: 10.1147/sj.92.0078.

### Semantic Similarity Caching

[6] J. Johnson, M. Douze, and H. Jégou, "Billion-Scale Similarity Search with GPUs," *IEEE Trans. Big Data*, vol. 7, no. 3, pp. 535–547, Sep. 2021, doi: 10.1109/TBDATA.2019.2921572. *(Basis for FAISS-compatible vector similarity lookup in `SemanticCache`)*

[7] Y. A. Malkov and D. A. Yashunin, "Efficient and Robust Approximate Nearest Neighbor Search Using Hierarchical Navigable Small World Graphs," *IEEE Trans. Pattern Anal. Mach. Intell.*, vol. 42, no. 4, pp. 824–836, Apr. 2020, doi: 10.1109/TPAMI.2018.2889473. *(HNSW algorithm; relevant to embedding similarity lookup in `SemanticCache::findSimilar()`)*

### Adaptive TTL and Access-Pattern-Driven Caching

[8] A. Cidon, A. Eisenman, M. Alizadeh, and S. Katti, "Cliffhanger: Scaling Performance Cliffs in Web Memory Caches," in *Proc. 13th USENIX Symp. on Networked Systems Design and Implementation (NSDI '16)*, Santa Clara, CA, USA, 2016, pp. 379–392. [Online]. Available: https://www.usenix.org/system/files/conference/nsdi16/nsdi16-paper-cidon.pdf *(Adaptive cache management and TTL tuning under skewed access patterns)*

[9] C. Shi, B. Hua, and K. G. Shin, "ARCA: Adaptive Replacement Cache with Admission Policy," in *Proc. IEEE/IFIP Int. Conf. on Dependable Systems and Networks (DSN)*, 2010, pp. 251–260, doi: 10.1109/DSN.2010.5544298. *(Adaptive replacement and admission control relevant to `enable_adaptive_ttl` design)*

### Distributed Cache Coordination and Consistency

[10] J. Nishtala et al., "Scaling Memcache at Facebook," in *Proc. 10th USENIX Symp. on Networked Systems Design and Implementation (NSDI '13)*, Lombard, IL, USA, 2013, pp. 385–398. [Online]. Available: https://www.usenix.org/system/files/conference/nsdi13/nsdi13-final170_update.pdf *(Distributed invalidation protocol design; basis for `RedisCacheCoordinator` publish/subscribe invalidation model)*

[11] B. Fan, D. G. Andersen, M. Kaminsky, and M. D. Mitzenmacher, "Cuckoo Filter: Practically Better Than Bloom," in *Proc. 10th ACM Int. on Conference on Emerging Networking Experiments and Technologies (CoNEXT '14)*, Sydney, Australia, 2014, pp. 75–88, doi: 10.1145/2674005.2674994. *(Membership filter for cache coordination; potential future use in distributed invalidation)*

### GDPR-Aware Cache Invalidation

[12] N. Kaaniche, M. Laurent, and M. Belguith, "Privacy Enhancing Technologies for Big Data Analytics," *Int. J. Inf. Secur.*, vol. 18, no. 2, pp. 143–160, 2019, doi: 10.1007/s10207-018-0403-1. *(Privacy-by-design and data erasure in caching systems; relevant to `invalidatePII()` GDPR Art. 17 implementation)*

### Predictive Prefetching

[13] O. Shacham, M. Ding, C. H. Kim, M. Erez, S. A. Mahlke, and K. Olukotun, "Rethinking Prefetching in Data-Parallel Accelerators," in *Proc. 44th Annual IEEE/ACM Int. Symp. on Microarchitecture (MICRO)*, Porto Alegre, Brazil, 2011, pp. 53–64, doi: 10.1145/2155620.2155628. *(Prefetch accuracy vs. coverage trade-offs; informs `PredictivePrefetcher` candidate generation heuristic)*

[14] S. Srinath, O. Mutlu, H. Kim, and Y. N. Patt, "Feedback Directed Prefetching: Improving the Performance and Bandwidth-Efficiency of Hardware Prefetchers," in *Proc. 13th Int. Symp. on High-Performance Computer Architecture (HPCA)*, Phoenix, AZ, USA, 2007, pp. 63–74, doi: 10.1109/HPCA.2007.346180. *(Feedback-driven prefetch throttling; basis for prefetch hit-rate tracking in `PredictivePrefetcher::recordPrefetchHit()`)*
