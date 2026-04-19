<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Changelog — Cache Module

All notable changes to the Cache module are documented here.
The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/).

## [Unreleased]
*(All planned features are implemented — see `FUTURE_ENHANCEMENTS.md` for long-horizon items.)*

## [1.9.0] — 2026-03-22
### Changed
- **Lock-free L1 read path**: `l1_mutex_` promoted from `std::mutex` to `std::shared_mutex`; `AdaptiveQueryCache::get()` now holds a `std::shared_lock` on the L1 tier, allowing concurrent reads without serialisation.
- `L1Entry` fields (`created_at_ms`, `last_accessed_ms`, `access_count`, `ttl_seconds`, `window_start_ms`, `window_count`, `expired_flag`) converted to `std::atomic`; copy/move constructors deleted.
- `l1_cache_` value type changed from `L1Entry` to `std::unique_ptr<L1Entry>` to allow stable pointer access under a shared lock.
- `l1_eviction_strategy_` calls (insert/remove/select/clear/getName) now protected by a dedicated `l1_eviction_mutex_` (`std::mutex`) separate from `l1_mutex_`.
- Lazy L1 expiry: expired entries are marked via CAS on `expired_flag` and purged during the next write-path eviction pass instead of erasing under a shared lock.
- `onAccess()` removed from the L1 hot read path; access frequency tracked atomically via `access_count.fetch_add`.

## [1.7.0] — 2026-03-09
### Added
- Public cache abstraction interfaces: `IEvictionPolicy`, `ICacheAdminOps`, `ICacheWarmup`, `IGDPRPurgeHook`, `ITTLAdapter` in `include/cache/cache_interfaces.h`
- `CacheInterfacesTests` with 43 unit tests for all 5 interfaces and value types
- `DELETE /v1/admin/cache/pii/{pii_uuid}` admin endpoint calling `invalidatePII()`; requires `admin:cache:write` scope
- Auto-trigger `invalidatePII()` from `PIIPseudonymizer::erasePII()` via `registerCacheInvalidator()` callback
- HMAC-SHA256 signed invalidation messages for `RedisCacheCoordinator`: `Config::hmac_secret`, `computeHmac()`/`verifyHmac()`; unsigned messages rejected when secret is configured

### Fixed
- Cross-tenant cache reads now return `nullopt` instead of leaking data when tenant ID mismatch occurs

## [1.6.0] — 2026-02-01
### Added
- Cache replication for HA multi-node deployments: `CacheReplicationManager`, `InProcessCacheCoordinator`, `RedisCacheCoordinator` (Issues #1590, #1595)
- Cache hit rate SLO alerting: `CacheHitRateSloMonitor` with configurable threshold, cooldown, and alert callback (Issue #1586)
- Predictive pre-fetching based on query history: `PredictivePrefetcher` with `recordQueryAccess()`/`getCandidates()` (Issue #1589)
- Configurable eviction policies beyond LRU: `EvictionPolicy` enum `{LRU, LFU, ARC}`; `ARCCache<K,V>` with `makeEvictionStrategy()` factory (Issue #1585)
- Embedding-specific cache (`src/cache/embedding_cache.cpp`)
- `CacheReplicationCoordinator` (`src/cache/cache_replication_coordinator.cpp`)

## [1.5.0] — 2026-01-15
### Added
- GDPR-aware cache invalidation: `invalidatePII(pii_uuid)` (GDPR Art. 17) with `pii_key_index_` reverse map and `pii_ref:` L3 sentinel keys; 7 unit tests (Issue #1591)
- `put()` override accepting `pii_uuids` list for PII-annotated cache entries
- Admin API for cache inspection, eviction, and reload: routes under `/v1/admin/cache/` (Issue #1577)
- Cache warmup from query logs or snapshot: `warmupFromLog()` + `exportSnapshot()` (Issue #1578)
- Tenant management API: list, stats, quota update via HTTP (Issue #1579)
- Health endpoint: `GET /v1/admin/cache/health` reporting per-tier status and circuit breaker state (Issue #1580)
- Adaptive TTL tuning: `calculateAdaptiveTTL(access_count)` with logarithmic scaling (Issue #1581)
- Tenant-level cache statistics dashboard: `GET /v1/admin/cache/tenants` (Issue #1584)

## [1.0.0] — 2024-01-01
### Added
- Multi-level adaptive query cache: L1 in-memory LRU (1 KB/entry), L2 zstd/lz4 compressed (10 KB/entry), L3 RocksDB-backed persistent (`adaptive_query_cache.h/cpp`)
- Semantic-aware query result caching with SHA-256 fingerprint and cosine similarity (`semantic_cache.h/cpp`)
- `BoundedLRUCache` with configurable max entries and TTL (`bounded_lru_cache.h/cpp`)
- Automatic cache invalidation via `invalidate(pattern)` with L3 iterator-based prefix scan
- Circuit breaker for RocksDB fault isolation (CLOSED/OPEN/HALF_OPEN) with retry logic
- Per-entry size limits and per-tier metrics (hits, misses, errors, compression ratios)
- Tenant isolation: tenant-scoped `get()`/`put()` with per-tenant size quotas (default 100 MB)
- Token bucket rate limiting per tenant namespace
- Configuration validation on startup with typed exception hierarchy
