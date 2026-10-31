---
Author: "ThemisDB Team"
Created: "2026-10-30"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Access Model Trust Boundaries

## Scope and trust model

This document describes the security-relevant behavior of the in-process tier
coordinator. `access_model` is not an authentication, authorization, tenant
isolation, encryption, or key-management subsystem. Its listener callbacks and
`AccessTier` implementations are trusted code in the application process.
Callers are responsible for authorization before sending events or exposing
coordinator methods.

The main boundary is:

```text
cache/storage adapter -> host-owned listener bridge -> AccessCoordinator
                                                   -> tier registry
                                                   -> logger/metrics
```

The cache and storage setters accept raw pointers to listener implementations.
`AccessCoordinator` does not implement `EvictionListener` or
`PromotionListener`; a host-owned bridge must forward callbacks to its public
event methods. The tier registry is supplied by the caller. `initialize()`
checks for non-null, healthy adapters whose reported tier matches the map key
and requires at least one cache and one storage tier. It does not initialize
adapters or own their lifetime. Applications must keep them alive while
callbacks or coordinator operations can run.

## Data exposure and correlation

The default structured logger writes keys, tier names, reasons, status,
latency, correlation ID, and thread ID for relevant events. Keys may be
customer-sensitive identifiers; restrict log access and redact keys before
external incident sharing. Do not put credentials, bearer tokens, or secrets in
keys, event reasons, or correlation IDs.

Correlation IDs are diagnostic strings. The coordinator generates counter-based
IDs for some events; `TraceContextManager` also supports random-looking IDs and
thread-local context. Neither implementation defines an authentication token
or cryptographic integrity mechanism. Do not use these IDs as secrets or
authorization evidence.

## Controls present and limitations

- Coordinator state and its task queue are guarded by a mutex. The configured
  queue limit also bounds pending demotion plans; default capacity is 10,000.
  Shutdown drains accepted tasks before joining workers.
- Default logs and aggregate counters provide diagnostic evidence. The logger
  catches and suppresses logging exceptions, so logging is best-effort.
- Invalid age-policy updates are rejected with `std::invalid_argument`.
- Promotion reads from the source and writes to the destination. Demotion writes
  before invalidating the source; a failed invalidation may leave duplicates,
  which are reported as failure. The operation has no cross-backend transaction,
  rollback, retry, or deduplication.
- Queue-full rejection is visible in aggregate counters/history; async callers
  also receive a failed future. The queue-depth gauge reports pending work, not
  in-flight workers.
- Event payloads are not authenticated or tenant-scoped by this module.

The library is not an authorization boundary or durable audit system. The
application must enforce permissions, initialize adapters, protect telemetry,
verify backend changes, and handle duplicate-copy recovery.

## Incident handling

On unexpected or missing transitions, preserve the application revision,
sanitized event fields, policy, tier map, lifecycle state, counters, and relevant
logs. Verify authoritative data directly through the owning cache/storage
backend. Stop further notifications at the adapter boundary if integrity is
uncertain; do not delete, demote, or promote source data based only on this
coordinator's success result. Escalate to the access-model and owning
cache/storage maintainers.

See [`SECURITY.md`](./SECURITY.md) for module security scope and
[`RUNBOOK.md`](./RUNBOOK.md) for response steps.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
