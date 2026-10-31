---
Author: "ThemisDB Team"
Created: "2026-10-31"
Last Updated: "2026-10-31"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Model_Attribution: "Exact model backend not verifiable from final document text alone."
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

# Runbook — Themis Core

## Purpose and safety

Use this page for first-response triage of `src/themis` build/gating,
module-load, verification, dependency, and wire-session incidents. The detailed
operator commands, alert mapping, and escalation contacts are maintained in
[`docs/operability/RUNBOOK_THEMIS_CORE.md`](../../docs/operability/RUNBOOK_THEMIS_CORE.md);
follow that runbook for recovery. Do not bypass license, signature, hash, or
dependency checks to restore service.

## Initial triage

1. Record incident time, affected node, release/build identifier, edition,
   platform, and recent module/configuration changes.
2. Preserve relevant server logs and the failing module or wire endpoint
   context before restarting or replacing anything.
3. Identify the failing boundary:

| Symptom | First checks |
|---|---|
| Feature unexpectedly denied | Compare compiled edition/feature flags with runtime license status; distinguish tier denial from expiry, invalid signature, and uninitialized gate. |
| Module load refused | Capture the explicit failure reason; check the expected hash, signature/key trust, dependency availability/order, and platform loader error independently. |
| Dependency initialization fails | Inspect the dependency graph for missing nodes, incompatible constraints, or cycles; do not force-open a dependency gate. |
| Wire requests stall or fail | Determine whether the issue is connection admission, malformed input, session timeout, or peer disconnect; preserve the protocol/session logs. |
| Build identity is unexpected | Compare the manifest and embedded revision/toolchain metadata using the build-info contract; verify the artifact came from the intended build. |

The operator reference documents `[THEMIS:LoadFailed]`,
`[THEMIS:WireSessionStall]`, `[THEMIS:TrustViolation]`,
`[THEMIS:GatingTimeout]`, and `[THEMIS:EditionGateFailed]` log patterns and
associated metrics. Confirm those signals in the deployed version rather than
assuming every version emits every metric.

## Recovery and rollback

- **License or edition denial:** correct the edition/license configuration or
  renew/refresh through the supported license process. If the license is
  invalid or the feature is not in the built edition, keep the feature
  disabled. Never edit runtime gate state to manufacture permission.
- **Hash/signature failure:** quarantine the artifact and compare it with the
  release source of truth. Redeploy a verified, correctly signed artifact
  through the normal release process; do not regenerate an expected hash from
  an untrusted file.
- **Dependency or loader failure:** restore the last known-good compatible
  module set or roll back the deployment as a unit. Do not load a dependent
  module while its required dependencies are unavailable.
- **Wire/session incident:** preserve logs and evidence, then apply the
  version-specific timeout or session recovery procedure in the operator
  runbook. Do not disable message validation to clear a backlog.
- **Rollback condition:** roll back when verification cannot be established,
  module activation would violate a dependency contract, or the wire service
  cannot safely serve requests. Re-verify the replacement release before
  reopening traffic.

## Escalation

Escalate suspected artifact tampering, unexplained trust decisions, repeated
signature failures, or any proposed security-control bypass to the security
owner. Escalate persistent load, dependency, or wire failures to the core
platform/SRE owners with the collected build identity, logs, module versions,
and reproduction conditions. Consult the detailed runbook for current severity
and contact procedures.

See [`OPERATIONS.md`](./OPERATIONS.md) for observability and readiness checks
and [`TRUST.md`](./TRUST.md) for security boundaries.
