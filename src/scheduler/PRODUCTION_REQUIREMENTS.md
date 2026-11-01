---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: module-behavior
Canonical references: include/scheduler/task_scheduler.h; src/scheduler/task_scheduler.cpp; include/scheduler/task_audit_manager.h; src/scheduler/task_audit_manager.cpp
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler production-operation notes

## Scope

This file distinguishes source-visible configuration from deployment
requirements. It does not certify the scheduler as production-ready. Normative
production controls require review by the owners of the server/API, security,
storage, and deployment configuration.

## Source-visible configuration

- `TaskScheduler::Config` defaults audit logging and anomaly detection to
  enabled, but permits disabling them. Result storage and sandboxed execution
  default to disabled. The effective behavior depends on the configuration
  passed to the constructor and, for result storage, the supplied storage
  object.
- `TaskAuditConfig` separately configures audit/security logging, anomaly
  detection, export, HMAC, retention, archival, and corruption recovery. The
  presence of a setting is not evidence that it is enabled or operationally
  provisioned.
- The task scheduler requires a `QueryEngine`; CDC-trigger tasks additionally
  require a `Changefeed`. The distributed coordinator stores non-owning
  references to its scheduler and coordinator; callers must preserve their
  lifetime.
- The `linux-release` preset exists in `CMakePresets.json`. Scheduler source
  registration is gated by `THEMIS_ENABLE_HTTP_SERVER`; this is build
  registration, not a deployment-mode check.

## Security-sensitive boundary

`TaskScheduler` checks permission names for selected operations, but its
permission helpers fall back to allow when a request context is absent. The
calling API must provide and clear the intended request context and enforce its
own authentication boundary. See [SECURITY.md](SECURITY.md).

## Unresolved deployment requirements

- **TODO:** Define required audit/security settings, HMAC-key provisioning,
  retention and archival policy, authorization context propagation, sandbox
  policy, and storage lifecycle with the relevant owners.
- **TODO:** Establish deployment-specific limits, timeouts, retries, and
  availability requirements from actual supported configuration and operations.
- **Assumption not made:** No environment variable or generic production-mode
  switch is documented here because none was verified in the scoped scheduler
  source.

Until these gaps are resolved, use this page as a code/configuration inventory,
not as a production readiness checklist.
