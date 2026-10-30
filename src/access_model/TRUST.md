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
cache/storage adapter -> listener callback -> AccessCoordinator -> tier registry
                                                        -> logger/metrics
```

The tier registry is supplied by the caller. `initialize()` copies the map but
does not validate null references, required tier coverage, tier health, or
ownership. Applications must validate adapters and lifetime before registering
listeners.

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

- Coordinator state and its task queue are guarded by a mutex; lifecycle
  methods start and join worker threads.
- Default logs and aggregate counters provide diagnostic evidence. The logger
  catches and suppresses logging exceptions, so logging is best-effort.
- `AgeBasedPolicy::isValid()` checks selected threshold relationships, but
  callers must invoke it; `setAgePolicy()` does not enforce it.
- Unknown, missing, or unhealthy tier configurations are not fail-closed at
  `initialize()`; the method stores the supplied map and returns true.
- The task queue is unbounded; no `max_queue_depth` enforcement or queue-depth
  gauge is implemented.
- The worker currently reports promotion success without invoking a tier
  operation; demotion execution likewise does not transfer data. Do not use
  coordinator outcomes as integrity or authorization evidence.
- Event payloads are not authenticated or tenant-scoped by this module.

These limits mean this library should not be treated as a security boundary or
as a durable audit system. The application must enforce permissions, validate
tier configuration, protect telemetry, and verify backend changes.

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
