---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Cache API Guide

Exact model backend not verifiable from final document text alone.

## Purpose and API boundary

This guide helps C++ callers select the appropriate cache interface and understand its inputs and failure behavior. The public declarations in [`include/cache`](../../include/cache) are authoritative; this page summarizes the commonly used surfaces rather than replacing their headers or Doxygen contracts.

## Adaptive query results

Use [`AdaptiveQueryCache`](../../include/cache/adaptive_query_cache.h) when callers need tiering, tenant context, invalidation, statistics, or warmup/prefetch controls.

| Operation | Input and result | Important edge behavior |
|---|---|---|
| `generateFingerprint(query, params, tenant_id)` | Query text, JSON parameters, optional tenant; returns a fingerprint string | Include the same tenant context on generation and access when isolation is enabled. |
| `get(fingerprint, tenant_id)` | Fingerprint and tenant; returns an optional `CacheEntry` | Missing, expired, or inaccessible entries are not hits. |
| `put(fingerprint, query_params, result, tenant_id, pii_uuids)` | Query identity, result, optional tenant and PII identifiers; returns success | Input limits, configured quotas, cache configuration, and backend state apply. |
| `invalidate(pattern)`, `invalidateTenant(tenant_id)`, `invalidatePII(pii_uuid)` | Key pattern or explicit tenant/PII identifier; return number removed | Replication delivery, where enabled, is distinct from local removal. |
| `getStats()`, `getStatsByTier()`, `getHealthStatus()`, `getDetailedInfo()` | No input; return counters/status JSON or structures | Treat health and statistics as diagnostics, not authorization decisions. |

The inherited simple backend adapter (`get(key)`, `put(key, value, ttl)`, `remove`, `contains`) uses the empty/default tenant. It is not a substitute for tenant-aware calls.

## Other cache APIs

- [`BoundedLRUCache`](../../include/cache/bounded_lru_cache.h) implements `ICacheBackend<std::string, nlohmann::json>` with bounded entries, TTL, `get`, `put`, `remove`, `contains`, `clear`, and statistics.
- [`EmbeddingCache`](../../include/cache/embedding_cache.h) accepts an embedding vector for `query()` and stores query text, vector, and metadata through `store()`. The vector length must equal `Config::embedding_dim`; a mismatch yields no query result or a failed store. See [`EMBEDDING.md`](./EMBEDDING.md).
- [`SemanticCache`](../../include/cache/semantic_cache.h) exposes `put(prompt, params, response, metadata, ttl)`, `query(prompt, params)`, `getStats()`, `clearExpired()`, and `clear()`. It borrows the `TransactionDB` and optional column-family pointer supplied at construction; the caller owns those resources and must keep them alive.
- [`ICacheCoordinator`](../../include/cache/cache_replication_coordinator.h) publishes entry/invalidation events and registers callbacks. `isConnected()` and `getStats()` expose coordinator health, not an end-to-end delivery guarantee.
- [`CacheHitRateSloMonitor`](../../include/cache/cache_hit_rate_slo_monitor.h) evaluates supplied metrics and can record per-tier latency and tenant eviction rate. Threshold configuration is validated by `Config::validate()`.

## Contract constraints

The shared v1.x contract in [`cache_contract.h`](../../include/cache/cache_contract.h) specifies maximum key, value, and tenant-ID sizes, temporal bounds, failure classes, and fail-closed decisions. In particular:

- Empty tenant ID denotes the global namespace; isolation is not enabled by default in `AdaptiveQueryCache::Config`.
- A malformed key/value or tenant violation must not be interpreted as a hit.
- `DegradedBackend` and `InternalError` require fail-closed handling under the contract; `PartialDelivery` is separately reported.
- Do not assume that replication, persistence, or tenant isolation is active just because the relevant interface exists; configure and verify the feature.

## Integration validation

For callers using tenant-scoped paths, cover the same fingerprint/tenant on both read and write, cross-tenant miss behavior, tenant invalidation, and quota rejection. For persistent semantic cache use, verify RocksDB lifetime and column-family selection. For coordinator use, verify disconnected and partial-delivery diagnostics.

## Related documentation

- [Implementation map](./IMPLEMENTATION.md)
- [Integration guide](./MODULE_GUIDE.md)
- [Runbook](./RUNBOOK.md)
- [Trust boundaries](./TRUST.md)
