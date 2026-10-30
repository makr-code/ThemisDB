---
Author: "ThemisDB Team"
Created: "2026-09-21"
Last Updated: "2026-10-30"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Security - Access Model Module

Report vulnerabilities via the project-level SECURITY.md.

## Security Scope

`access_model` is an in-process tier coordination library, not an authentication,
authorization, tenant-isolation, or durable audit subsystem. The application must
authorize callers, validate tier adapters, and protect diagnostic data.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| Unauthorized or cross-tenant transition | No identity or tenant authorization is performed; listener callers are trusted. |
| Queue-driven memory exhaustion | `pending_tasks_` is an unbounded `std::queue`; no `max_queue_depth` setting or queue-depth gauge exists. |
| Work loss during shutdown | Workers exit when `running_` becomes false and do not drain queued tasks. |
| Missing audit evidence | The default logger catches and suppresses logging exceptions; records are best-effort, not a durable audit trail. |
| Sensitive identifiers in logs | Event keys are included in structured log messages; restrict and redact logs. |
| Invalid tier or policy configuration | `initialize()` copies the map without validating its entries. `setAgePolicy()` does not call `AgeBasedPolicy::isValid()`. |
| False success or data integrity assumption | Current worker processing reports success without invoking tier `get`/`put`; results do not prove data movement. |

## Controls Present

- Coordinator state and task-queue access use a mutex; worker threads are joined
  during shutdown.
- `AgeBasedPolicy::isValid()` is available for caller-side validation of
  selected threshold relationships.
- Structured logs and correlation IDs support diagnosis when logging succeeds.
- `TraceContextManager::ScopedContext` restores the previous thread-local
  context when its scope ends.

These controls do not establish authorization, bounded resource use, fail-closed
tier registration, or durable audit guarantees.

## Security Follow-ups

- Validate the tier map and policy before accepting production events.
- Add bounded queue/backpressure behavior and define shutdown handling for queued work.
- Implement real tier transfer semantics and surface backend failures before treating results as successful.
- Ensure application-level authorization, log access controls, and key redaction are in place.

## Sourcecode Verification (Module: access_model/security)

- Verified files:
  - `src/access_model/access_coordinator.cpp`
  - `src/access_model/access_model_logging.cpp`
  - `src/access_model/access_model_trace.cpp`
  - `src/access_model/age_based_policy.cpp`
  - `include/access_model/access_model_trace.h`
  - `include/access_model/access_coordinator.h`
- Source behavior is summarized in [`TRUST.md`](./TRUST.md); the queue is
  unbounded, tier initialization is not validated, logging is best-effort, and
  current worker results do not execute data transfers.

---

**AI provenance:** Exact model backend not verifiable from final document text alone.
