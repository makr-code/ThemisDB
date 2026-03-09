# Cache Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready multi-level cache (L1/L2/L3) with all four implementation phases complete. All features including tenant management API, cache replication, SLO alerting, and distributed coordination are implemented. Unit test coverage tracking (target: >80%) is the only outstanding item (Issue: #1596).

## Completed ✅
- [x] Multi-level adaptive query cache (L1 in-memory, L2 compressed, L3 RocksDB-backed)
- [x] Semantic-aware query result caching with vector similarity lookups
- [x] LRU eviction with configurable cache size and TTL
- [x] Automatic cache invalidation on data changes
- [x] Per-entry size limits and validation (L1: 1KB, L2: 10KB, L3: configurable)
- [x] Circuit breaker for RocksDB fault isolation (CLOSED/OPEN/HALF_OPEN states)
- [x] Enhanced metrics: hits/misses, errors, compression ratios per tier
- [x] Retry logic with exponential backoff for RocksDB initialization
- [x] L3 pattern-based invalidation using iterator-based scans
- [x] Configuration validation on startup
- [x] Token bucket rate limiting
- [x] Tenant isolation and namespace enforcement
- [x] Per-tenant size quotas
- [x] GDPR-aware cache invalidation – PII purge propagation via `invalidatePII()` (GDPR Art. 17)
- [x] Admin API for cache operations and monitoring (Issue: #1577)
- [x] Cache warmup with bulk operations (Issue: #1578)
- [x] Tenant management API — list, stats, quota update via HTTP (Issue: #1579)
- [x] Health checks and cache diagnostics endpoint (Issue: #1580)
- [x] Adaptive TTL tuning based on access patterns (Issue: #1581)
- [x] Admin API: inspect, evict, and reload cache entries via HTTP (Issue: #1582)
- [x] Bulk warmup from query logs or snapshot (Issue: #1583)
- [x] Tenant-level cache statistics dashboard (`GET /v1/admin/cache/tenants`) (Issue: #1584)
- [x] Configurable eviction policies beyond LRU (LFU, ARC) via `eviction_policy.h` / `arc_cache.h` (Issue: #1585)
- [x] Cache hit rate SLO alerting via `CacheHitRateSLOMonitor` (Issue: #1586)
- [x] Predictive pre-fetching based on query history (Issue: #1589)
- [x] GDPR-aware cache invalidation (PII purge propagation) (Issue: #1591)
- [x] Cache replication for high-availability multi-node deployments (Issue: #1590, #1595)

## In Progress 🚧
- [I] Unit tests coverage > 80% (Issue: #1596)

## Implementation Phases

### Phase 1: Multi-Level Cache Core (Status: Completed)
- [x] Implemented L1 in-memory LRU cache with 1 KB per-entry size limit
- [x] Implemented L2 compressed cache with 10 KB per-entry size limit
- [x] Implemented L3 RocksDB-backed persistent cache with configurable size
- [x] Added automatic cache invalidation on data mutation events
- [x] Implemented circuit breaker for RocksDB fault isolation (CLOSED/OPEN/HALF_OPEN)
- [x] Added retry logic with exponential backoff for RocksDB initialization
- [x] Implemented L3 pattern-based invalidation via iterator-based key scans
- [x] Added per-tier metrics: hit rate, miss rate, error count, compression ratio

### Phase 2: Security and Tenant Isolation (Status: Completed)
- [x] Added startup configuration validation with descriptive error messages
- [x] Implemented token bucket rate limiting per tenant namespace
- [x] Enforced tenant namespace isolation preventing cross-tenant cache reads
- [x] Implemented per-tenant size quotas with configurable byte limits

### Phase 3: Operational Excellence and Admin API (Status: Complete ✅)
- [x] Implement Admin API for cache inspection, eviction, and reload (`server/cache_admin_api_handler.cpp`) (Issue: #1599)
- [x] Implement bulk warmup from query log snapshot (`cache/warmup.cpp`) (Issue: #1600)
- [x] Implement tenant management API (list tenants, per-tenant stats, quota updates) (Issue: #1601)
- [x] Add `/health` endpoint reporting per-tier status and circuit breaker state (Issue: #1602)
- [x] Implement adaptive TTL tuning based on per-key access frequency (Issue: #1603)

### Phase 4: Distributed Cache and Predictive Features (Status: Complete ✅)
- [x] Implement Redis-compatible distributed cache coordination protocol (Issue: #1592)
- [x] Add write-through cache mode for read-heavy workloads (Issue: #1593)
- [x] Implement predictive pre-fetching based on query sequence history (Issue: #1594)
- [x] Add cache replication for high-availability multi-node deployments (Issue: #1595)

## Production Readiness Checklist
- [I] Unit tests coverage > 80% (Issue: #1596)
- [x] Integration tests (L1/L2/L3 pipeline, circuit breaker, tenant isolation)
- [x] Performance benchmarks (Issue: #1597)
- [x] Security audit (tenant isolation, namespace enforcement)
- [x] Documentation complete (Issue: #1598)
- [x] API stability guaranteed for cache read/write/invalidate

## Known Issues & Limitations
- Unit test coverage tracking is ongoing (Issue: #1596); integration tests and all major pipelines are covered.
- Distributed cache coordination (`RedisCacheCoordinator`) requires an external Redis server; enable via `THEMIS_ENABLE_REDIS=ON` and link hiredis. The coordinator degrades gracefully when Redis is unavailable.
- Predictive pre-fetching is implemented (`PredictivePrefetcher`, opt-in via `enable_predictive_prefetch`); actual pre-warm scheduling is delegated to the caller.
- Cache entries are not encrypted at rest (L3); enable RocksDB encryption at the storage layer for at-rest protection.
- `invalidatePII()` does not yet automatically integrate with `PIIPseudonymizer::erasePII()`; callers must invoke it explicitly (see `FUTURE_ENHANCEMENTS.md`).

## Breaking Changes
- Admin API endpoints (`/v1/admin/cache/`) were introduced as new endpoints; non-breaking to existing cache read/write/invalidate API.
- Distributed cache configuration adds new optional fields for cluster mode; existing single-node configurations are unaffected.
