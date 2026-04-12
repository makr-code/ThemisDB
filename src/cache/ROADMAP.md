# Cache Module Roadmap
<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · FUTURE_ENHANCEMENTS.md · docs/de/src/cache/README.md -->
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready multi-level cache (L1/L2/L3) with all four implementation phases complete. All features including tenant management API, cache replication, SLO alerting, and distributed coordination are implemented. All outstanding items are now resolved.

## Completed ✅
- [x] Multi-level adaptive query cache (L1 in-memory, L2 compressed, L3 RocksDB-backed) — `adaptive_query_cache.h/cpp`; `BoundedLRUCache` (L1), zstd/lz4 compressed L2, `RocksDBWrapper` L3; tests in `tests/test_adaptive_query_cache.cpp`
- [x] Semantic-aware query result caching with vector similarity lookups — `semantic_cache.h/cpp`; SHA-256 fingerprint + cosine similarity; tests in `tests/test_semantic_cache.cpp`
- [x] LRU eviction with configurable cache size and TTL — `bounded_lru_cache.h/cpp`; `BoundedLRUCache::Config::max_entries`, `::ttl`; tests in `tests/test_bounded_lru_cache.cpp`
- [x] Automatic cache invalidation on data changes — `AdaptiveQueryCache::invalidate(pattern)` in `adaptive_query_cache.h`; L3 iterator-based prefix scan; tests in `tests/test_adaptive_query_cache.cpp`
- [x] Per-entry size limits and validation (L1: 1KB, L2: 10KB, L3: configurable) — `AdaptiveQueryCache::Config::l1_max_entry_size`, `l2_max_entry_size`, `max_total_entry_size` in `adaptive_query_cache.h` line ~90–115
- [x] Circuit breaker for RocksDB fault isolation (CLOSED/OPEN/HALF_OPEN states) — `enable_circuit_breaker`, `cb_failure_threshold`, `cb_timeout_ms` in `adaptive_query_cache.h`; state reported via `getHealthStatus()["l3"]["circuit_breaker"]`
- [x] Enhanced metrics: hits/misses, errors, compression ratios per tier — `cache_metrics.h`; `CacheMetrics` struct with atomic counters `l1_hits`, `l2_hits`, `l3_hits`, `misses`, `compression_failures`, etc.
- [x] Retry logic with exponential backoff for RocksDB initialization — `adaptive_query_cache.cpp`; `initL3WithRetry()` private method; configurable via `Config::l3_retry_max_attempts`
- [x] L3 pattern-based invalidation using iterator-based scans — `AdaptiveQueryCache::invalidate(pattern)` uses RocksDB iterator scan in `adaptive_query_cache.cpp`
- [x] Configuration validation on startup — `AdaptiveQueryCache::Config::validate()` in `adaptive_query_cache.h`; constructor throws `std::invalid_argument` on invalid config; tests in `tests/test_adaptive_cache_phase1.cpp`
- [x] Token bucket rate limiting — `AdaptiveQueryCache::Config::enable_rate_limiting`, `max_requests_per_second` in `adaptive_query_cache.h`; `RateLimiter` struct in `adaptive_query_cache.h`; tests in `tests/test_adaptive_cache_phase1.cpp`
- [x] Tenant isolation and namespace enforcement — `enable_tenant_isolation` config flag; tenant-scoped `get(fp, tenant_id)` / `put(fp, params, result, tenant_id)` in `adaptive_query_cache.h`; cross-tenant access returns `nullopt`
- [x] Per-tenant size quotas — `Config::per_tenant_max_bytes` (default 100MB) in `adaptive_query_cache.h`; enforced in `put()` via `tenant_bytes_used_` map
- [x] GDPR-aware cache invalidation – PII purge propagation via `invalidatePII()` (GDPR Art. 17) — `AdaptiveQueryCache::invalidatePII(pii_uuid)` in `adaptive_query_cache.h` line ~360; `put(fp, params, result, tenant_id, pii_uuids)` override at line ~248; 7 unit tests in `tests/test_adaptive_query_cache.cpp`
- [x] Admin API for cache operations and monitoring (Issue: #1577) — `src/server/cache_admin_api_handler.cpp`; routes under `/v1/admin/cache/`; tests in `tests/test_cache_admin_api_handler.cpp`
- [x] Cache warmup with bulk operations (Issue: #1578) — `src/cache/warmup.cpp`; `AdaptiveQueryCache::warmupFromLog()` and `::exportSnapshot()` in `adaptive_query_cache.h`; tests in `tests/test_cache_warmup.cpp`
- [x] Tenant management API — list, stats, quota update via HTTP (Issue: #1579) — `GET /v1/admin/cache/tenants`, `GET /v1/admin/cache/tenant/{id}/stats`, `PATCH /v1/admin/cache/tenant/{id}/quota` in `src/server/cache_admin_api_handler.cpp`; `AdaptiveQueryCache::getTenantStats()`, `::getTenantStatsForTenant()`, `::updateTenantQuota()` in `adaptive_query_cache.h`
- [x] Health checks and cache diagnostics endpoint (Issue: #1580) — `GET /v1/admin/cache/health` in `src/server/cache_admin_api_handler.cpp`; `AdaptiveQueryCache::getHealthStatus()` in `adaptive_query_cache.h`
- [x] Adaptive TTL tuning based on access patterns (Issue: #1581) — `Config::enable_adaptive_ttl`, `adaptive_ttl_min_seconds`, `adaptive_ttl_max_seconds`, `adaptive_ttl_scaling_factor` in `adaptive_query_cache.h`; `calculateAdaptiveTTL(access_count)` private method; logarithmic-scaling formula
- [x] Admin API: inspect, evict, and reload cache entries via HTTP (Issue: #1582) — `GET /v1/admin/cache/stats`, `DELETE /v1/admin/cache/key/{key}`, `DELETE /v1/admin/cache/tenant/{id}`, `POST /v1/admin/cache/circuit-breaker/reset` in `src/server/cache_admin_api_handler.cpp`
- [x] Bulk warmup from query logs or snapshot (Issue: #1583) — `POST /v1/admin/cache/warmup`, `POST /v1/admin/cache/snapshot` in `src/server/cache_admin_api_handler.cpp`; `AdaptiveQueryCache::warmupFromLog()`, `::exportSnapshot()` in `adaptive_query_cache.h`
- [x] Tenant-level cache statistics dashboard (`GET /v1/admin/cache/tenants`) (Issue: #1584) — `GET /v1/admin/cache/tenants` in `src/server/cache_admin_api_handler.cpp` line ~472; returns per-tenant bytes, hits, misses, hit_rate, evictions
- [x] Configurable eviction policies beyond LRU (LFU, ARC) via `eviction_policy.h` / `arc_cache.h` (Issue: #1585) — `EvictionPolicy` enum `{LRU, LFU, ARC}` in `include/cache/eviction_policy.h`; `ARCCache<K,V>` in `include/cache/arc_cache.h`; `makeEvictionStrategy()` factory; tests in `tests/test_arc_cache.cpp`
- [x] Cache hit rate SLO alerting via `CacheHitRateSLOMonitor` (Issue: #1586) — `CacheHitRateSloMonitor` class in `include/cache/cache_hit_rate_slo_monitor.h`; `src/cache/cache_hit_rate_slo_monitor.cpp`; configurable threshold, cooldown, alert callback; tests in `tests/test_cache_hit_rate_slo_monitor.cpp`
- [x] Predictive pre-fetching based on query history (Issue: #1589) — `PredictivePrefetcher` class in `include/cache/predictive_prefetcher.h`; `src/cache/predictive_prefetcher.cpp`; `recordQueryAccess()`, `getCandidates()`; opt-in via `Config::enable_predictive_prefetch`
- [x] GDPR-aware cache invalidation (PII purge propagation) (Issue: #1591) — see `invalidatePII()` above; `pii_key_index_` reverse map + `pii_ref:` L3 sentinel keys; 7 unit tests
- [x] Cache replication for high-availability multi-node deployments (Issue: #1590, #1595) — `CacheReplicationManager` in `include/cache/cache_replication.h`; `InProcessCacheCoordinator` in `include/cache/distributed_cache_coordinator.h`; `RedisCacheCoordinator` in `include/cache/redis_cache_coordinator.h`; tests in `tests/test_cache_replication.cpp`, `tests/test_distributed_cache_coordinator.cpp`

## In Progress 🚧

## Completed (Follow-up) ✅
- [x] `DELETE /v1/admin/cache/pii/{pii_uuid}` admin endpoint – `src/server/cache_admin_api_handler.cpp`; calls `invalidatePII(pii_uuid)`; requires `admin:cache:write` scope
- [x] Auto-trigger `invalidatePII()` from `PIIPseudonymizer::erasePII()` via registered callback – `PIIPseudonymizer::registerCacheInvalidator()` in `include/utils/pii_pseudonymizer.h`
- [x] HMAC-SHA256 signed invalidation messages for `RedisCacheCoordinator` – `Config::hmac_secret` field; `computeHmac()` / `verifyHmac()` in `src/cache/redis_cache_coordinator.cpp`; unsigned messages rejected when secret configured
- [x] Public cache abstraction interfaces – `include/cache/cache_interfaces.h`; `IEvictionPolicy`, `ICacheAdminOps`, `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter` with value types `CacheStats`, `KeyFilter`, `WarmupStats`, `PurgeDescriptor`, `PurgeResult`, `AccessPattern`, `TTLAdapterConfig`
- [x] Unit tests coverage > 80% (Issue: #1596) — `tests/test_cache_interfaces.cpp`; 43 unit tests for all 5 interfaces and all value types; registered as `CacheInterfacesTests` in `tests/CMakeLists.txt`
- [x] `RedisCacheCoordinator` Async Pub/Sub Subscription Loop (v1.7.0) — exponential back-off reconnection (1 s → 30 s) with `cache.redis.reconnect` metric; `isConnected()` exposed in `GET /v1/admin/cache/health`; Windows stub replaced with `THEMIS_POSIX_SOCKETS` compile-time feature flag; noisy `THEMIS_WARN` in stub constructor downgraded to `THEMIS_DEBUG`; CI: `redis-cache-coordinator-async-loop-ci.yml`
- [x] Warmup: Parallel Bulk Load (v1.8.0, Issue: #244) — `src/cache/warmup.cpp` rewrites `warmupFromLog()` with N `std::async` workers (one per CPU core); `Config::max_parallel_workers` (default: `std::thread::hardware_concurrency()`); `WarmupResult::warmup_duration_ms` + `warmup_entries_per_second`; per-shard L1/L2 insertion under existing mutexes; atomic aggregate counters; API response extended with timing/throughput fields; 4 new tests in `tests/test_cache_warmup.cpp`; CI: `cache-warmup-parallel-bulk-load-ci.yml`
- [x] Lock-Free L1 Read Path (v1.9.0) — `l1_mutex_` → `std::shared_mutex`; `L1Entry` fields atomicised; `l1_cache_` stores `unique_ptr<L1Entry>`; `l1_eviction_mutex_` guards eviction strategy; lazy expiry via CAS on `expired_flag`; `onAccess()` removed from hot path
- [x] `RequestCoalescer` — real Singleflight implementation (Issue: #4580) (2026-04-12)
  - `include/cache/request_coalescer.h`; `promise/shared_future` inflight map
  - `fn()` called exactly once per concurrent in-flight key group; results broadcast to all waiters
  - Exception from `fn()` propagated as `success=false` + error message to all waiters
  - 14 focused tests (RC-01…RC-14) in `tests/test_request_coalescer.cpp`

## Implementation Phases

### Phase 1: Multi-Level Cache Core (Status: Completed)
- [x] Implemented L1 in-memory LRU cache with 1 KB per-entry size limit — `bounded_lru_cache.h/cpp`; `Config::max_entries`, `l1_max_entry_size = 1024` in `adaptive_query_cache.h`
- [x] Implemented L2 compressed cache with 10 KB per-entry size limit — zstd/lz4 compressed entries in `adaptive_query_cache.cpp`; `Config::l2_max_entry_size = 10240`
- [x] Implemented L3 RocksDB-backed persistent cache with configurable size — `RocksDBWrapper` integration via `Config::l3_db_path`; circuit breaker guards all L3 operations
- [x] Added automatic cache invalidation on data mutation events — `AdaptiveQueryCache::invalidate(pattern)` in `adaptive_query_cache.h`
- [x] Implemented circuit breaker for RocksDB fault isolation (CLOSED/OPEN/HALF_OPEN) — `Config::enable_circuit_breaker`, states reported in `getHealthStatus()`
- [x] Added retry logic with exponential backoff for RocksDB initialization — `initL3WithRetry()` in `adaptive_query_cache.cpp`
- [x] Implemented L3 pattern-based invalidation via iterator-based key scans — `invalidate(pattern)` uses RocksDB iterator in `adaptive_query_cache.cpp`
- [x] Added per-tier metrics: hit rate, miss rate, error count, compression ratio — `CacheMetrics` struct in `include/cache/cache_metrics.h`

### Phase 2: Security and Tenant Isolation (Status: Completed)
- [x] Added startup configuration validation with descriptive error messages — `Config::validate(std::string* error)` in `adaptive_query_cache.h`; constructor throws on failure
- [x] Implemented token bucket rate limiting per tenant namespace — `RateLimiter` struct in `adaptive_query_cache.h`; `Config::enable_rate_limiting`, `max_requests_per_second`
- [x] Enforced tenant namespace isolation preventing cross-tenant cache reads — `get(fp, tenant_id)` returns `nullopt` when tenant mismatch; `tenant_bytes_used_` map enforces quotas
- [x] Implemented per-tenant size quotas with configurable byte limits — `Config::per_tenant_max_bytes` (default 100MB); enforced in `put()` path

### Phase 3: Operational Excellence and Admin API (Status: Complete ✅)
- [x] Implement Admin API for cache inspection, eviction, and reload (`server/cache_admin_api_handler.cpp`) (Issue: #1599) — `src/server/cache_admin_api_handler.cpp`; routes under `/v1/admin/cache/`; tests in `tests/test_cache_admin_api_handler.cpp`
- [x] Implement bulk warmup from query log snapshot (`cache/warmup.cpp`) (Issue: #1600) — `src/cache/warmup.cpp`; `warmupFromLog()` + `exportSnapshot()` in `adaptive_query_cache.h`; tests in `tests/test_cache_warmup.cpp`
- [x] Implement tenant management API (list tenants, per-tenant stats, quota updates) (Issue: #1601) — `GET /v1/admin/cache/tenants`, `GET /v1/admin/cache/tenant/{id}/stats`, `PATCH /v1/admin/cache/tenant/{id}/quota`; `updateTenantQuota()` in `adaptive_query_cache.h`
- [x] Add `/health` endpoint reporting per-tier status and circuit breaker state (Issue: #1602) — `GET /v1/admin/cache/health` in `cache_admin_api_handler.cpp`; `getHealthStatus()` returns per-tier counts and circuit breaker state
- [x] Implement adaptive TTL tuning based on per-key access frequency (Issue: #1603) — `calculateAdaptiveTTL(access_count)` in `adaptive_query_cache.cpp`; `CacheEntry::access_count` field; logarithmic scaling

### Phase 4: Distributed Cache and Predictive Features (Status: Complete ✅)
- [x] Implement Redis-compatible distributed cache coordination protocol (Issue: #1592) — `RedisCacheCoordinator` in `include/cache/redis_cache_coordinator.h`; `src/cache/redis_cache_coordinator.cpp`; hiredis pub/sub; enable via `THEMIS_ENABLE_REDIS=ON`; tests in `tests/test_distributed_cache_coordinator.cpp`
- [x] Add write-through cache mode for read-heavy workloads (Issue: #1593) — `Config::enable_write_through` (opt-in) in `adaptive_query_cache.h`; `writeThroughToL3()` private method; `CacheMetrics::write_through_writes` counter
- [x] Implement predictive pre-fetching based on query sequence history (Issue: #1594) — `PredictivePrefetcher` in `include/cache/predictive_prefetcher.h`; `src/cache/predictive_prefetcher.cpp`; `recordQueryAccess()`, `getCandidates()`; opt-in via `Config::enable_predictive_prefetch`
- [x] Add cache replication for high-availability multi-node deployments (Issue: #1595) — `CacheReplicationManager` in `include/cache/cache_replication.h`; `InProcessCacheCoordinator` in `include/cache/distributed_cache_coordinator.h`; `src/cache/cache_replication.cpp`, `src/cache/cache_replication_coordinator.cpp`; tests in `tests/test_cache_replication.cpp`

### Phase 5: Concurrency Hardening — Lock-Free L1 Read Path (Status: Complete ✅)
- [x] Promote `l1_mutex_` from `std::mutex` to `std::shared_mutex`; all read paths use `std::shared_lock`, all write paths use `std::unique_lock` — `adaptive_query_cache.h/cpp`
- [x] Convert `L1Entry` fields to `std::atomic` and store entries as `std::unique_ptr<L1Entry>` in `l1_cache_` — deleted copy/move constructors prevent accidental value-type copies
- [x] Add `l1_eviction_mutex_` to serialise `l1_eviction_strategy_` calls independently of `l1_mutex_` — `adaptive_query_cache.h/cpp`
- [x] Implement lazy L1 expiry via CAS on `L1Entry::expired_flag`; expired entries purged during write-path eviction pass, not inline on `get()`
- [x] Remove `l1_eviction_strategy_->onAccess()` from hot read path; access frequency tracked via `access_count.fetch_add(1, relaxed)`

## Production Readiness Checklist
- [x] Unit tests coverage > 80% (Issue: #1596)
- [x] Integration tests (L1/L2/L3 pipeline, circuit breaker, tenant isolation)
- [x] Performance benchmarks (Issue: #1597)
- [x] Security audit (tenant isolation, namespace enforcement, HMAC-signed coordinator messages)
- [x] Documentation complete (Issue: #1598)
- [x] API stability guaranteed for cache read/write/invalidate
- [x] GDPR Art. 17 complete: `invalidatePII()` auto-triggered from `PIIPseudonymizer::erasePII()` and exposed via `DELETE /v1/admin/cache/pii/{pii_uuid}`

## Known Issues & Limitations
- Distributed cache coordination (`RedisCacheCoordinator`) requires an external Redis server; enable via `THEMIS_ENABLE_REDIS=ON` and link hiredis. The coordinator degrades gracefully when Redis is unavailable.
- Predictive pre-fetching is implemented (`PredictivePrefetcher`, opt-in via `enable_predictive_prefetch`); actual pre-warm scheduling is delegated to the caller.
- Cache entries are not encrypted at rest (L3); enable RocksDB encryption at the storage layer for at-rest protection.

## Breaking Changes
- Admin API endpoints (`/v1/admin/cache/`) were introduced as new endpoints; non-breaking to existing cache read/write/invalidate API.
- Distributed cache configuration adds new optional fields for cluster mode; existing single-node configurations are unaffected.
