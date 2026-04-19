> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Cache Module

> For reporting security vulnerabilities, see the project-level [SECURITY.md](../../../SECURITY.md).

## Security Scope

The Cache module stores query results in a multi-level cache (L1 in-memory, L2 compressed, L3 RocksDB). Security concerns focus on: tenant data isolation, GDPR-compliant PII invalidation, protection of the admin API, replay prevention for cache invalidation messages, and safe RocksDB fault handling.

## Threat Model

| Threat | Mitigation |
|--------|------------|
| Cross-tenant cache data leakage | Tenant namespace isolation: `get(fp, tenant_id)` returns `nullopt` on tenant mismatch; per-tenant size quotas enforced in `put()` |
| GDPR right-to-erasure non-compliance | `invalidatePII(pii_uuid)` removes all entries tagged with a PII UUID from L1, L2, and L3; auto-triggered from `PIIPseudonymizer::erasePII()` |
| Cache poisoning via unsigned invalidation | HMAC-SHA256 signed invalidation messages in `RedisCacheCoordinator`; unsigned messages rejected when `hmac_secret` is configured |
| Admin API unauthorized access | `DELETE /v1/admin/cache/pii/{pii_uuid}` requires `admin:cache:write` scope; all admin routes enforce auth middleware |
| RocksDB path traversal | L3 database path validated at startup; configuration validation rejects `..` components |
| Resource exhaustion via large batch puts | Per-entry size limits (L1: 1 KB, L2: 10 KB, L3: configurable); per-tenant byte quotas (default 100 MB) |
| Replay of stale cache invalidation messages | HMAC-MAC includes message timestamp; replay window configurable |
| Cache warming from malicious snapshots | `warmupFromLog()` validates entry format and tenant ownership before loading; oversized entries are rejected |

## Security Controls

### Tenant Isolation
- Every `get()` and `put()` operation requires a `tenant_id`; the cache key is namespaced by tenant.
- Cross-tenant reads return `nullopt` — not an error that could be used to confirm key existence.
- Per-tenant size quotas enforced in `put()` via `tenant_bytes_used_` map; quota violations return a cache-miss.

### GDPR PII Purge
- PII-tagged entries use `pii_key_index_` reverse map to track which cache keys are associated with each PII UUID.
- `invalidatePII(pii_uuid)` atomically removes entries from L1, L2, and L3 (via RocksDB iterator scan).
- `pii_ref:` sentinel keys in L3 allow purge even after L1/L2 eviction.
- Auto-trigger integration: `PIIPseudonymizer::registerCacheInvalidator()` ensures purge propagates automatically.

### Cache Invalidation Integrity
- Redis-based distributed invalidation uses HMAC-SHA256 (`computeHmac()`/`verifyHmac()`) to authenticate invalidation messages.
- Messages without a valid HMAC are silently rejected when a secret is configured.

### Admin API Authorization
- All admin routes (`/v1/admin/cache/`) require valid authentication tokens.
- PII purge endpoint requires `admin:cache:write` scope to prevent accidental or unauthorized data erasure.
- Circuit breaker reset endpoint requires `admin:cache:admin` scope.

## Data Handling

- Cache entries may contain query result data that includes PII or sensitive business data — governed by per-tenant configuration.
- L3 (RocksDB) stores compressed, serialized query results on disk; encryption at rest is operator-configured at the storage layer.
- Semantic cache vectors (cosine similarity lookups) contain embedding representations of queries — not raw document content.
- Cache warmup snapshots are treated as untrusted input: format-validated before loading.

## Known Limitations

- L3 (RocksDB) encryption at rest is not managed by the cache module; operators must configure filesystem or RocksDB column family encryption separately.
- Predictive prefetcher access patterns are stored in memory and not encrypted; these may reveal query behavior to privileged internal processes.
- SLO alerting callbacks are invoked in the same thread as the cache operation; slow alert handlers could affect cache latency.

## Dependency Security

| Dependency | Purpose | Notes |
|------------|---------|-------|
| RocksDB | L3 persistent cache storage | Keep patched; path injection hardened |
| Redis (optional) | Distributed cache invalidation coordination | TLS connection recommended in production |
| zstd / lz4 | L2 cache compression | Input bounds enforced; no arbitrary decompression |
