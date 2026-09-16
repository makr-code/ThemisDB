# Runbook: Governance Module

<!-- Status: current | validated: 2026-09-16 | Wave D operability deliverable -->

## Purpose

Operator remediation guide for the ThemisDB `governance` module. Covers the
five most critical incident classes, their log patterns, triage steps, and
recommended remediation actions.

---

## Scenario 1 — Policy Failed

**Log pattern:** `[GOVERNANCE:PolicyFailed]`

**Symptoms**
- A governance policy evaluation fails to produce a result.
- Log lines contain `[GOVERNANCE:PolicyFailed]` with policy ID and failure reason.
- Operations that require policy approval are blocked; compliance gates cannot be passed.

**Triage**
1. Identify the failing policy ID and the operation context from the log.
2. Check policy engine health: `themis_admin governance policy status`.
3. Verify OPA (or the configured policy engine) is running and responsive.
4. Review recent policy deployments for syntax errors or invalid rule references.

**Remediation**
1. Reload the failing policy from the last validated snapshot:
   `themis_admin governance policy reload <id>`.
2. If the policy engine is unresponsive, restart it:
   `themis_admin governance policy-engine restart`.
3. Roll back the policy to the last known-good version:
   `themis_admin governance policy rollback <id>`.
4. Enable fail-open mode temporarily if operations must not be blocked:
   set `governance.fail_open: true` (requires senior approval).

**Escalation**
Policy failures that block production operations require immediate escalation
to the governance team — provide policy ID, failure class, and the operation
that was gated.

---

## Scenario 2 — Version Conflict

**Log pattern:** `[GOVERNANCE:VersionConflict]`

**Symptoms**
- Concurrent policy version updates produce a conflict.
- Log lines contain `[GOVERNANCE:VersionConflict]` with conflicting version IDs.
- Policy version history shows duplicate or forked version entries.

**Triage**
1. Identify the conflicting policy version IDs from the log.
2. Review the policy version history: `themis_admin governance policy version-history <id>`.
3. Determine which concurrent actors caused the conflict (check the audit trail).
4. Verify that the version locking mechanism is functioning correctly.

**Remediation**
1. Resolve the conflict by selecting the canonical version:
   `themis_admin governance policy version-resolve <id> --keep <version_id>`.
2. Unlock the policy for further updates: `themis_admin governance policy unlock <id>`.
3. If conflicts are recurring, enforce a single-writer workflow for critical policies.
4. Tighten version control access: restrict concurrent writers via RBAC.

**Escalation**
Version conflicts that cannot be resolved by the operator indicate a governance
coordination failure — escalate to the compliance team with the conflict details
and the audit trail.

---

## Scenario 3 — Review Timeout

**Log pattern:** `[GOVERNANCE:ReviewTimeout]`

**Symptoms**
- A policy review or approval workflow step times out without a decision.
- Log lines contain `[GOVERNANCE:ReviewTimeout]` with review ID and approver list.
- Policy deployment pipelines stall waiting for review completion.

**Triage**
1. Identify the timed-out review ID and the assigned approvers.
2. Check review workflow status: `themis_admin governance review status <id>`.
3. Determine whether approvers are unavailable or have not been notified.
4. Verify that the notification delivery system (email, Slack) is functioning.

**Remediation**
1. Reassign the review to available approvers:
   `themis_admin governance review reassign <id> --approvers <list>`.
2. Extend the review timeout if the approvers need more time:
   `themis_admin governance review extend <id> --hours 24`.
3. Escalate to the review manager if approvers cannot be reached.
4. In a declared emergency, invoke the emergency approval bypass with dual-sign-off:
   `themis_admin governance review emergency-approve <id>`.

**Escalation**
Review timeouts blocking critical policy deployments should be escalated to
the governance manager with the review ID, approval chain, and the deployment
urgency justification.

---

## Scenario 4 — Compliance Breach

**Log pattern:** `[GOVERNANCE:ComplianceBreach]`

**Symptoms**
- A governance check detects a compliance violation in a data operation or policy config.
- Log lines contain `[GOVERNANCE:ComplianceBreach]` with breach ID, regulation reference,
  and affected data scope.
- The system may automatically quarantine the affected operation.

**Triage**
1. Identify the breach ID, regulation reference, and affected scope from the log.
2. Determine whether the breach is a true positive or a false positive.
3. Check the compliance rule definition: `themis_admin governance compliance rule show <id>`.
4. Identify the operation or actor that triggered the breach.

**Remediation**
1. If a false positive, update the compliance rule to exclude the benign pattern:
   `themis_admin governance compliance rule update <id>`.
2. If a true positive, immediately quarantine the affected data:
   `themis_admin governance compliance quarantine <scope>`.
3. File a compliance incident report and notify the DPO/compliance officer.
4. Conduct a root-cause analysis and patch the responsible process.

**Escalation**
Any confirmed compliance breach must be immediately escalated to the compliance
officer and legal team — do not remediate without their involvement.

---

## Scenario 5 — Policy Hot-Reload Failure

**Log pattern:** `[GOVERNANCE:PolicyFailed]` during hot-reload

**Symptoms**
- A live policy hot-reload fails, leaving the policy engine in an indeterminate state.
- Log lines contain `[GOVERNANCE:PolicyFailed]` with `hot_reload` context tag.
- Existing policy evaluations may be using an inconsistent policy set.

**Triage**
1. Determine the current active policy version: `themis_admin governance policy active`.
2. Check whether the hot-reload left any policy partially applied.
3. Review the hot-reload log for the specific failure stage.
4. Verify that the new policy bundle was fully transferred before reload.

**Remediation**
1. Force a clean policy reload from the last verified bundle:
   `themis_admin governance policy reload --version <stable_version>`.
2. Verify the active policy set is correct and consistent:
   `themis_admin governance policy verify`.
3. Fix the broken policy bundle before retrying the hot-reload.
4. Use a maintenance window for the next policy reload if the bundle is large.

**Escalation**
A failed policy hot-reload that leaves the policy engine inconsistent requires
immediate escalation to the governance on-call — the compliance posture is
compromised until the policy set is restored.

---

## Reference

| Log Pattern                          | Severity | SLO Impact | Owner      |
|--------------------------------------|----------|------------|------------|
| `[GOVERNANCE:PolicyFailed]`          | Critical | Yes        | governance |
| `[GOVERNANCE:VersionConflict]`       | Medium   | Partial    | governance |
| `[GOVERNANCE:ReviewTimeout]`         | Medium   | Partial    | governance |
| `[GOVERNANCE:ComplianceBreach]`      | Critical | Yes        | governance |

---

*Wave D operability deliverable — see `src/governance/ROADMAP.md`.*
