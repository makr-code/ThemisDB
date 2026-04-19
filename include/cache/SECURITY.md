<!-- Status: current | validated: 2026-04-06 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security — Cache Module Public Headers

**Module Path:** `include/cache/`
**Implementation Security:** `../../src/cache/SECURITY.md`

---

## Scope

Security considerations for the public cache header API surface. Covers tenant isolation,
GDPR erasure, Redis invalidation integrity, and cache-layer information disclosure risks.

---

## Threat Model

| Threat | Vector | Mitigation Header |
|--------|--------|------------------|
| Cross-tenant cache read | Missing tenant_id check in `get()` | `adaptive_query_cache.h` — `get(fp, tenant_id)` returns `nullopt` on mismatch |
| Cache poisoning via unsigned Redis message | Attacker sends forged invalidation | `redis_cache_coordinator.h` — HMAC-SHA256 required; unsigned messages rejected |
| PII data persistence beyond erasure request | Cache retains entry after GDPR request | `cache_interfaces.h` `IGDPRPurgeHook` — synchronous purge across L1/L2/L3 |
| Eviction policy side-channel | Timing-based cache occupancy inference | `IEvictionPolicy` contract does not expose occupancy to callers |
| Unbounded per-tenant cache memory | Tenant exhausting shared cache | `ICacheAdminOps` — `resize()` and per-tenant byte quota |
| L3 data-at-rest disclosure | Unencrypted RocksDB files | Operator-managed; documented limitation |
| Admin operations without authentication | Direct `ICacheAdminOps` access | `ICacheAdminOps` is only obtainable via authenticated admin accessor |
| Prefetch of evicted PII data | `PredictivePrefetcher` re-populating purged entry | `invalidatePII()` blacklists key in prefetch index |

---

## Security Controls

### Tenant Isolation
All cache interfaces key on `tenant_id`; cross-tenant reads return `nullopt` with no
error, preventing timing-based cross-tenant inference.

### GDPR Right to Erasure
`IGDPRPurgeHook::purge(PurgeDescriptor)` is synchronous and writes an audit log entry
before returning. L1, L2, and L3 are all purged atomically.

### HMAC-Signed Invalidation
`RedisCacheCoordinator` requires HMAC-SHA256 signing of all invalidation messages.
When `Config::hmac_secret` is set, unsigned or mismatched messages are rejected.

### Admin Operation Gating
`ICacheAdminOps` is only obtainable through an authenticated admin accessor, not from
the primary `ICache<K,V>` interface directly.

---

## Known Limitations

- L3 (RocksDB) encryption at rest is operator-managed; the header contract does not
  enforce column family encryption. Operators must configure RocksDB encryption before
  production use.
- Redis TLS is recommended but not enforced by the header contract; operators must
  configure Redis with TLS in high-security deployments.
- Semantic cache (`semantic_cache.h`) stores embedding vectors; embeddings may
  reconstruct approximate input data — treat as potentially sensitive.
