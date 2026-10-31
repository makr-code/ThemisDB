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

- Task registration checks `task:register` and validates applicable AQL,
  resource, cron, and CDC configuration. `executeTaskNow` checks
  `task:execute` and applies a per-task rate limit. Function registration checks
  `task:register_function` and the `system_admin` role.
- `TaskScheduler::RequestContext` carries permissions and roles. **Important:**
  `hasPermission` and `hasRole` fall back to allow when no request context is
  present. The header instructs callers to set context before audited
  operations. The invoking server/API boundary must therefore be reviewed; do
  not treat these helpers alone as an authorization boundary.
- `sandbox_execution`, audit logging, anomaly detection, result storage, and
  GDPR masking are configurable. Sandbox execution and result storage default
  to disabled; audit/anomaly behavior can also be disabled by configuration.
- `TaskAuditConfig` exposes audit/security logging, HMAC, retention, archival,
  and corruption-recovery settings. These options do not by themselves prove
  that a deployment enables or correctly provisions them.

## Limits and follow-up

- The coordination-health check in `executeTaskNow` is a commented placeholder;
  do not rely on it to block execution during coordinator failure.
- The scheduler implementation does not establish a `THEMIS_PRODUCTION_MODE`
  or `THEMIS_ENVIRONMENT` gate. No production mode should be inferred from this
  module.
- A deployment review should verify request-context propagation, API
  authentication/authorization, audit configuration and key management,
  sandbox policy, storage lifetime, and rate-limit behavior for its own
  configuration.

**TODO:** security and deployment owners must define and review the normative
production controls. This source review cannot supply those requirements.
