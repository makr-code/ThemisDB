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

# Cache Incident Runbook

Exact model backend not verifiable from final document text alone.

## Purpose and safety

Use this runbook to diagnose cache hit-rate regressions, stale-result reports, tenant-isolation concerns, warmup failures, or coordinator degradation. It is for on-call operators and cache maintainers. Cache data is derived; do not weaken authorization or freshness checks to restore hit rate.

## Triage order

1. **Scope the impact.** Record affected service/node, cache class/tier, time window, tenant scope, recent deploy/configuration change, and whether symptoms are misses, stale values, latency, or write/invalidation errors.
2. **Read health and counters.** Inspect the adaptive cache's `getHealthStatus()`, `getStatsByTier()`, `getDetailedInfo()`, and `getCircuitBreakerStatus()` where exposed. Check SLO status through `CacheHitRateSloMonitor::getStatus()` / `getLastResult()`. For coordinators, inspect `isConnected()` and `getStats()`.
3. **Classify the failure.** Use the categories in [`cache_contract.h`](../../include/cache/cache_contract.h): malformed key/value, invalid operation, tenant violation, degraded backend, partial delivery, or internal error. Preserve the category and component in incident records.
4. **Check the dependency path.** For L3, verify the configured RocksDB path and process-level storage health. For Redis coordination, verify reachability, authentication/configuration, publish errors, reconnect counts, and subscriber status. For gRPC peers, compare healthy peer membership and failed-peer diagnostics.
5. **Compare with a focused regression.** Use the existing tests covering the suspected path in [`tests/cache`](../../tests/cache). Do not use a benchmark as a substitute for correctness or isolation verification.

## Symptom-specific response

### Hit rate falls or latency rises

- Compare tier hit/miss counts, entry count/capacity, expiry, evictions, and promotions/demotions across the same time interval.
- Check recent TTL, size limit, workload, or tier configuration changes before increasing capacity.
- If an L3 path is degraded, follow the database/storage owner runbook; do not delete or rewrite RocksDB files as a cache recovery shortcut.
- Escalate a sustained release-budget regression using the [cache benchmark runbook](../../benchmarks/cache/CACHE_BENCHMARK_RUNBOOK.md) and preserve the benchmark output.

### Stale data or cross-tenant result report

- Treat possible cross-tenant exposure as a security incident: stop relying on the affected cache path, preserve relevant logs/diagnostics, and escalate through the project security process.
- Verify `enable_tenant_isolation`, tenant key generation, and tenant context consistency on both writes and reads.
- Apply the narrowest supported invalidation (`invalidateTenant`, `invalidatePII`, or a reviewed pattern) after coordinating with the data owner. Confirm the local removal and any remote delivery state separately.
- Do not interpret cache flush success as proof that all remote replicas or backups were purged.

### Coordinator disconnected, partial delivery, or fanout drops

- Confirm local cache behavior and coordinator health independently; async replication does not make local success equivalent to all-peer delivery.
- Check `fanout_dropped`, `fanout_failed`, retry and publish-error counters, connection state, and failed peer identities where available.
- Restore the coordinator service/network/configuration through its owning operational procedure. Allow the implementation's bounded reconnect/retry behavior to work; avoid restart loops or manually injecting replication messages.
- Re-run the degraded-coordinator tests and verify counters/health return to normal before declaring recovery.

### Warmup did not complete

- Check `WarmupResult.ok`, its error, loaded/skipped/total counts, input file accessibility, and whether the file contains valid NDJSON records with SHA-256-format keys and supported fields.
- Confirm configured worker count, cache limits, tenant quota, and available storage. Small logs run single-threaded; warming intentionally reserves headroom in L1.
- Correct the source log or configuration and retry with a bounded input. Do not treat skipped entries as loaded.

## Rollback and escalation

Roll back the most recent cache configuration or deployment change if the issue began immediately afterward and rollback does not disable a required security control. Keep tenant isolation and fail-closed protections enabled. Escalate backend incidents to the RocksDB/Redis/gRPC owner and security incidents to the project security contact; include time range, failure class, sanitized diagnostics, peer/tier status, and actions already taken.

## Related references

- [Operations checklist and telemetry](./OPERATIONS.md)
- [Trust and incident boundaries](./TRUST.md)
- [Production requirements](./PRODUCTION_REQUIREMENTS.md)
- [Security overview](./SECURITY.md)
