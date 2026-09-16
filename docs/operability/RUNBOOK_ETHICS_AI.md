# Runbook: Ethics AI — Wave D Operability

<!-- Runbook: ethics_ai | Wave D | validated: 2026-09-16 -->
<!-- Links: src/ethics_ai/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `ethics_ai`
module. Use it to diagnose and remediate compliance gate failures, decision
timeouts, context assembly failures, policy violations, and EU AI Act
Article 13/22 gate incidents.

> **Important:** EU AI Act compliance gate items require legal review before
> closure. Do not mark EU AI Act compliance gates as satisfied without
> documented legal sign-off.

---

## Scenario 1 — Compliance Gate Failed

**Log pattern:** `[ETHICS:ComplianceGateFailed]`

### Symptoms
- Compliance gate evaluation fails or returns an inconclusive result.
- `[ETHICS:ComplianceGateFailed]` emitted with `gate_name`, `policy_id`, and `error_code` fields.
- Ethics decision pipeline may be blocked; fail-closed behavior enforces denial.

### Diagnosis
1. Confirm compliance gate failures:
   ```
   grep '\[ETHICS:ComplianceGateFailed\]' /var/log/themisdb/ethics_ai.log
   ```
2. Identify `gate_name` (e.g., `EU_AI_ACT_ART_13`, `DSGVO_ART_5`) and `policy_id`.
3. Check `legal_db` availability:
   ```
   themisdb-admin ethics check-legal-db
   ```
4. Review `ethics_compliance_gate_failure_total` metric.

### Remediation
1. If `legal_db` is unavailable: the gate must fail-closed; do NOT bypass.
   Restore `legal_db` connectivity: `themisdb-admin ethics restore-legal-db`.
2. If gate config is misconfigured: review `ethics_ai.compliance_gates` config and correct.
3. Confirm gate returns to PASS state after legal_db restoration.
4. Document the incident in the compliance incident log.

### Escalation
All compliance gate failures MUST be escalated to the legal and compliance team regardless
of duration.

---

## Scenario 2 — Decision Timeout

**Log pattern:** `[ETHICS:DecisionTimeout]`

### Symptoms
- Ethics decision evaluation exceeds configured timeout.
- `[ETHICS:DecisionTimeout]` emitted with `dilemma_id`, `school_count`, `elapsed_ms`, and `timeout_ms` fields.
- Decision may be aborted; fallback verdict (ABSTAIN or DEFER) may be applied.

### Diagnosis
1. Confirm decision timeouts:
   ```
   grep '\[ETHICS:DecisionTimeout\]' /var/log/themisdb/ethics_ai.log
   ```
2. Identify `school_count` and `elapsed_ms` vs. `timeout_ms`.
3. Check discourse mode (LAYERED_FULL vs. LAYERED_FAST) for affected decisions.
4. Review `ethics_decision_timeout_total` metric.

### Remediation
1. For LAYERED_FULL timeouts: consider switching to LAYERED_FAST for non-critical decisions.
2. Increase decision timeout: `ethics_ai.decision_timeout_ms` (must not exceed legal SLO).
3. Check school availability; ABSTAIN entries are recorded for unavailable schools per EU Art.13.
4. Confirm decisions complete within timeout after adjustment.

### Escalation
Escalate to the ethics engineering team if timeout rate exceeds 0.1% or if LAYERED_FULL
decisions are systematically timing out.

---

## Scenario 3 — Context Assembly Failed

**Log pattern:** `[ETHICS:ContextAssemblyFailed]`

### Symptoms
- Ethics context assembly for a decision round fails.
- `[ETHICS:ContextAssemblyFailed]` emitted with `context_id`, `assembly_stage`, and `error_code` fields.
- Decision may proceed with incomplete context; quality may be reduced.

### Diagnosis
1. Confirm context assembly failures:
   ```
   grep '\[ETHICS:ContextAssemblyFailed\]' /var/log/themisdb/ethics_ai.log
   ```
2. Identify `assembly_stage` (norm_retrieval, school_routing, profile_loading).
3. For norm retrieval failures: check legal_db connectivity (see Scenario 1).
4. Review `ethics_context_assembly_failure_total` metric.

### Remediation
1. For norm retrieval failures: restore `legal_db`; `legal_db_unavailable=true` flag is set in
   MetaVerdict per EU Art.13 contract.
2. For school routing failures: check school availability and routing config.
3. Confirm context assembly completes for subsequent decisions.

### Escalation
Escalate to the ethics engineering team if assembly failure rate exceeds 0.1% or if
legal_db remains unavailable.

---

## Scenario 4 — Policy Violation

**Log pattern:** `[ETHICS:PolicyViolation]`

### Symptoms
- A decision or context violates a configured ethics policy.
- `[ETHICS:PolicyViolation]` emitted with `policy_id`, `violation_type`, `dilemma_id` fields.
- Decision may be blocked; audit log entry is written per EU Art.13 requirements.

### Diagnosis
1. Confirm policy violations:
   ```
   grep '\[ETHICS:PolicyViolation\]' /var/log/themisdb/ethics_ai.log
   ```
2. Identify `policy_id` and `violation_type`.
3. Review the associated audit log entry:
   ```
   themisdb-admin ethics show-audit --dilemma-id <id>
   ```
4. Review `ethics_policy_violation_total` metric.

### Remediation
1. Do NOT bypass policy violations without legal review.
2. Escalate to the compliance team for all policy violations.
3. If the violation is a false positive: document and request policy rule review.
4. All audit log entries for violations are immutable and MUST be retained.

### Escalation
All policy violations MUST be escalated to the legal and compliance team immediately.

---

## Scenario 5 — EU AI Act Article 13/22 Compliance Gate Pending

**Log pattern:** `[ETHICS:ComplianceGateFailed]` with `gate_name=EU_AI_ACT_ART_13`

### Symptoms
- EU AI Act Art. 13/22 compliance gate has not been confirmed as green.
- Any compliance claim in documentation is premature.
- Wave D sign-off for EU AI Act compliance is blocked.

### Diagnosis
1. Check gate status:
   ```
   themisdb-admin ethics show-compliance-gate --gate EU_AI_ACT_ART_13
   ```
2. Determine if legal review has been completed (`[~]` in ROADMAP = pending legal review).
3. Review benchmark baseline for `GATE-EUAI-AUDIT-01`.

### Remediation
1. Do NOT mark gate as `[x]` without documented legal sign-off.
2. Engage legal team for EU AI Act Art. 13/22 review.
3. After legal sign-off: run `GATE-EUAI-AUDIT-01` benchmark on representative hardware.
4. Update `src/ethics_ai/ROADMAP.md` gate item to `[x]` only after legal approval.

### Escalation
Escalate to the legal team and ethics engineering lead. Do NOT ship any EU AI Act
Art. 13/22 compliance claims before this gate is confirmed green with legal sign-off.
