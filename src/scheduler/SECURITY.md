---
Author: ThemisDB Team
Created: 2026-10-31
Last Updated: 2026-11-01
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Team
Source level: 1
SOT domain: security
Canonical references: include/scheduler/task_scheduler.h; src/scheduler/task_scheduler.cpp; include/scheduler/task_audit_manager.h; src/scheduler/task_audit_manager.cpp
Related issue: #7376; DOC-WEEKLY-2026-44
---

Exact model backend not verifiable from final document text alone.

# Scheduler security notes

Report vulnerabilities through the project-level [SECURITY.md](../../SECURITY.md).
This page records controls visible in scheduler source; it is not a threat-model
approval or a claim that deployment authorization is complete.

## Source-visible controls

- Task registration checks `task:register`; removal checks `task:unregister`;
  enable, disable, and update check `task:update`; manual and DAG execution
  check `task:execute`. Function registration and removal require
  `task:register_function` and the `system_admin` role.
- `TaskScheduler::RequestContext` carries permissions and roles.
  `hasPermission` and `hasRole` deny when context is absent. HTTP routes and
  trusted in-process services must provide a scoped context with only the
  operation's required permissions. `ScopedRequestContext` restores the prior
  thread context at scope exit.
- Distributed scheduler dispatch checks a shared gate maintained by
  `DistributedTaskCoordinator`; manual, scheduled, CDC-triggered, and DAG
  dispatch fail closed until healthy leadership and fail closed on detected
  leadership/health loss. Standalone schedulers with no distributed coordinator
  remain local.
- CDC predicates reject malformed syntax, unknown fields, and unsupported
  operators during configuration validation; runtime parse/evaluation errors
  reject the event rather than matching by default.
- `sandbox_execution`, audit logging, anomaly detection, result storage, and
  GDPR masking are configurable. Sandbox execution and result storage default
  to disabled; audit/anomaly behavior can also be disabled by configuration.
- `TaskAuditConfig` exposes audit/security logging, HMAC, retention, archival,
  and corruption-recovery settings. These options do not by themselves prove
  that a deployment enables or correctly provisions them.

## Limits and follow-up

- The scheduler implementation does not establish a `THEMIS_PRODUCTION_MODE`
  or `THEMIS_ENVIRONMENT` gate. No production mode should be inferred from this
  module.
- Existing callers that relied on absent-context allow behavior must migrate to
  explicit `RequestContext` scopes. This is a deliberate behavior change.
- A deployment review should verify request-context propagation, API
  authentication/authorization, audit configuration and key management,
  sandbox policy, storage lifetime, and rate-limit behavior for its own
  configuration.

**TODO:** security and deployment owners must define and review the normative
production controls. This source review cannot supply those requirements.
