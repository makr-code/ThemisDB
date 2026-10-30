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

# Cache Operations

Exact model backend not verifiable from final document text alone.

## Purpose and operating boundary

This page describes cache-specific production configuration and observability for operators. It complements, but does not override, [`PRODUCTION_REQUIREMENTS.md`](./PRODUCTION_REQUIREMENTS.md), [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md), or service-specific runbooks. The query/data owner defines freshness and invalidation requirements; the cache does not become the source of truth.

## Before enabling a cache path

- Set explicit count, entry-size, total-size, TTL, and per-tenant resource limits appropriate to the workload. Defaults are implementation defaults, not a production sizing decision.
- Decide whether tenant isolation is mandatory. `AdaptiveQueryCache::Config::enable_tenant_isolation` defaults to false; when enabled, tenant context must be used consistently for key generation, reads, writes, stats, and invalidation.
- Configure and validate RocksDB storage paths for L3 and persistent caches. Plan disk capacity, permissions, backup/retention, and ownership with the storage operator.
- For Redis or remote coordination, configure the endpoint, authentication and secret source, timeouts/reconnect behavior, peer set, and expected degraded-mode behavior. Do not assume coordination is enabled or connected from object construction alone.
- Set alert thresholds and minimum request count for the hit-rate/latency SLO monitor based on the deployment's own baseline.

## Signals to inspect

| Signal | Source | Operational use |
|---|---|---|
| L1/L2/L3 hits, misses, evictions, promotions, demotions | `AdaptiveQueryCache::getStats()` / `getStatsByTier()` | Identify tier placement, expiry, capacity pressure, or lookup changes. |
| Health and circuit-breaker state | `getHealthStatus()` / `getCircuitBreakerStatus()` | Distinguish normal misses from degraded backend behavior. |
| Tenant counts, quotas, eviction and isolation activity | tenant stats APIs and `CacheHitRateSloMonitor::recordTenantEvictionRate()` | Investigate quota pressure and tenant-scoped incidents. |
| Hit-rate and latency violations | `CacheHitRateSloMonitor` status/result and alert IDs | Detect sustained regressions; alerting requires the configured alert manager/integration. |
| Replication publish/receive errors, reconnects, fanout retries/drops/failures | Coordinator `getStats()` | Detect partial propagation; local completion does not prove remote delivery. |
| Warmup loaded/skipped/error counts and duration | `AdaptiveQueryCache::WarmupResult` | Confirm warmup outcome and identify bad records or quota limits. |

Use counters with their sampling interval and request volume. A zero hit rate with no evaluated requests is not a measured SLO breach; the monitor configuration includes `min_requests`.

## Routine checks

1. Review resource bounds and backend availability after config/deployment changes.
2. Check cache health, per-tier counters, and coordinator state during rollout and after dependency incidents.
3. Validate mapped performance cases against the release baseline using [`PERFORMANCE_EXPECTATIONS.md`](./PERFORMANCE_EXPECTATIONS.md) and the [benchmark runbook](../../benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md).
4. Preserve raw benchmark JSON, manifest comparison, hardware/runtime context, and regression follow-up evidence.
5. Confirm invalidation and tenant-isolation regressions remain covered by focused tests before changing related configuration.

## Safe recovery

- Prefer correcting the dependency, configuration, workload, or source-of-truth invalidation path over flushing all cache state.
- Use tenant- or PII-scoped invalidation when the incident scope is known; verify local and remote coordination outcomes separately.
- Use `resetCircuitBreaker()` only after the backend has been checked and the cause of the open state addressed.
- Treat warmup as bounded prepopulation. Review skipped/error counts before increasing concurrency or replaying a large log.
- Do not disable tenant checks, authorization at the caller boundary, or fail-closed behavior as a recovery measure.

For step-by-step incident handling, see [RUNBOOK.md](./RUNBOOK.md). For trust and security escalation, see [TRUST.md](./TRUST.md) and [SECURITY.md](./SECURITY.md).
