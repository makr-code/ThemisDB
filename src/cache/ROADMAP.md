# Cache Module Roadmap
<!-- Status: [ ] open  [~] in progress  [x] done  [I] Issue  [P] PR  [?] blocked  [!] unclear -->

## Current Status
Production-ready multi-level cache (L1/L2/L3) with Phase 1 and Phase 2 hardening complete. Phase 3 operational excellence features are in progress.

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

## In Progress 🚧
- [ ] Admin API for cache operations and monitoring (Target: Q2 2026)
- [ ] Cache warmup with bulk operations (Target: Q2 2026)
- [ ] Tenant management API (Target: Q2 2026)
- [ ] Health checks and cache diagnostics endpoint (Target: Q3 2026)
- [ ] Adaptive TTL tuning based on access patterns (Target: Q3 2026)

## Planned Features 📋

### Short-term (Next 3-6 months)
- [ ] Admin API: inspect, evict, and reload cache entries via HTTP
- [ ] Bulk warmup from query logs or snapshot
- [ ] Tenant-level cache statistics dashboard
- [ ] Configurable eviction policies beyond LRU (LFU, ARC)
- [ ] Cache hit rate SLO alerting

### Long-term (6-12 months)
- [ ] Distributed cache coordination across nodes (Redis-compatible protocol)
- [ ] Write-through cache mode for read-heavy workloads
- [ ] Predictive pre-fetching based on query history
- [ ] Cache replication for high-availability deployments
- [ ] GDPR-aware cache invalidation (PII purge propagation)

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

### Phase 3: Operational Excellence and Admin API (Status: In Progress)
- [~] Implement Admin API for cache inspection, eviction, and reload (`cache/admin_api.cpp`)
- [~] Implement bulk warmup from query log snapshot (`cache/warmup.cpp`)
- [~] Implement tenant management API (list tenants, per-tenant stats, quota updates)
- [~] Add `/health` endpoint reporting per-tier status and circuit breaker state
- [~] Implement adaptive TTL tuning based on per-key access frequency

### Phase 4: Distributed Cache and Predictive Features (Status: Planned)
- [ ] Implement Redis-compatible distributed cache coordination protocol
- [ ] Add write-through cache mode for read-heavy workloads
- [ ] Implement predictive pre-fetching based on query sequence history
- [ ] Add cache replication for high-availability multi-node deployments

## Production Readiness Checklist
- [ ] Unit tests coverage > 80%
- [x] Integration tests (L1/L2/L3 pipeline, circuit breaker, tenant isolation)
- [ ] Performance benchmarks
- [x] Security audit (tenant isolation, namespace enforcement)
- [ ] Documentation complete
- [x] API stability guaranteed for cache read/write/invalidate

## Known Issues & Limitations
- Admin API is not yet implemented (Phase 3 in progress)
- Adaptive TTL tuning is not yet active
- Distributed cache coordination requires external Redis or future built-in cluster mode
- Predictive pre-fetching is not yet implemented

## Breaking Changes
- Admin API will be introduced as a new endpoint (non-breaking to existing cache API)
- Distributed cache configuration will add new required fields for cluster mode
