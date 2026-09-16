# Runbook: Security — Wave D Operability

<!-- Runbook: security | Wave D | validated: 2026-09-16 -->
<!-- Links: src/security/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `security`
module. Use it to diagnose and remediate policy inconsistencies, key rotation
failures, RBAC violations, and provider degradation incidents.

---

## Scenario 1 — Policy Inconsistent

**Log pattern:** `[SECURITY:PolicyInconsistent]`

### Symptoms
- Policy evaluation produces inconsistent results across RBAC/ABAC/RLS enforcement paths.
- `[SECURITY:PolicyInconsistent]` emitted with `subject`, `action`, `resource`, `expected_decision`,
  and `actual_decision` fields.
- Access control may be weaker or stronger than intended.

### Diagnosis
1. Confirm policy inconsistencies:
   ```
   grep '\[SECURITY:PolicyInconsistent\]' /var/log/themisdb/security.log
   ```
2. Identify `subject`, `action`, `resource`, and decision delta.
3. Compare policy rules across enforcement paths:
   ```
   themisdb-admin security show-policy --subject <subject>
   ```
4. Review `security_policy_inconsistency_total` metric.

### Remediation
1. Reconcile policy rules across RBAC, ABAC, and RLS layers.
2. For deny-by-default violations: verify `policy.default_deny=true` is set.
3. Apply corrected policy: `themisdb-admin security apply-policy --file <policy.json>`.
4. Confirm decision consistency after policy update.

### Escalation
All policy inconsistencies MUST be escalated to the security team immediately.

---

## Scenario 2 — Key Rotation Failed

**Log pattern:** `[SECURITY:KeyRotationFailed]`

### Symptoms
- Key rotation operation fails; active key may be at risk.
- `[SECURITY:KeyRotationFailed]` emitted with `key_id`, `provider_type`, and `error_code` fields.
- Services using the affected key may fail to authenticate or decrypt data.

### Diagnosis
1. Confirm key rotation failures:
   ```
   grep '\[SECURITY:KeyRotationFailed\]' /var/log/themisdb/security.log
   ```
2. Identify `provider_type` (Vault, HSM, PKI) and `error_code`.
3. Test provider connectivity:
   ```
   themisdb-admin security test-provider --provider <provider_type>
   ```
4. Review `security_key_rotation_failure_total` metric.

### Remediation
1. Restore provider connectivity; check network path and auth tokens.
2. Retry rotation: `themisdb-admin security rotate-key --key-id <id>`.
3. If provider is permanently degraded: trigger key-provider failover.
4. Confirm rotation completes and new key is active.

### Escalation
All key rotation failures MUST be escalated to the security team immediately regardless of
provider type.

---

## Scenario 3 — RBAC Violation

**Log pattern:** `[SECURITY:RBACViolation]`

### Symptoms
- A role-based access control check produces an unexpected allow or deny.
- `[SECURITY:RBACViolation]` emitted with `user_id`, `role_id`, `resource_id`, and `violation_type` fields.
- Unauthorized access may have been granted or legitimate access blocked.

### Diagnosis
1. Confirm RBAC violations:
   ```
   grep '\[SECURITY:RBACViolation\]' /var/log/themisdb/security.log
   ```
2. Identify `violation_type` (unexpected_allow, unexpected_deny).
3. Inspect role assignment for the affected user:
   ```
   themisdb-admin security show-roles --user-id <id>
   ```
4. Review `security_rbac_violation_total` metric.

### Remediation
1. For unexpected_allow: immediately revoke conflicting role or deny override rule.
2. For unexpected_deny: verify role assignment and policy merge rules.
3. Apply corrected role assignment: `themisdb-admin security assign-role --user <id> --role <role>`.
4. Confirm RBAC decisions are consistent after fix.

### Escalation
All unexpected_allow violations MUST be escalated to the security team immediately and
treated as a potential security incident.

---

## Scenario 4 — Provider Degraded

**Log pattern:** `[SECURITY:ProviderDegraded]`

### Symptoms
- Security provider (Vault, HSM, PKI) enters degraded state.
- `[SECURITY:ProviderDegraded]` emitted with `provider_type`, `degradation_reason`, and `last_healthy_ms` fields.
- Key operations and certificate validation may fail.

### Diagnosis
1. Confirm provider degradation:
   ```
   grep '\[SECURITY:ProviderDegraded\]' /var/log/themisdb/security.log
   ```
2. Identify `provider_type` and `degradation_reason`.
3. Test provider health:
   ```
   themisdb-admin security test-provider --provider <provider_type>
   ```
4. Review `security_provider_health_status` metric.

### Remediation
1. Restore provider health (restart Vault, HSM reconnect, PKI cert renewal).
2. Trigger provider failover if available: `themisdb-admin security failover-provider`.
3. Confirm provider health status returns to HEALTHY.
4. Document degradation incident and duration.

### Escalation
Escalate to the infrastructure and security teams immediately for any HSM degradation.

---

## Scenario 5 — Concurrent Policy Evaluation Under High Load

**Log pattern:** `[SECURITY:PolicyInconsistent]` with `concurrent_eval_count>N`

### Symptoms
- High-cardinality concurrent policy evaluations surface edge-case inconsistencies.
- `security_policy_eval_p99_ms` metric elevated.
- Deny-by-default enforcement may have race conditions under extreme concurrency.

### Diagnosis
1. Check concurrent evaluation depth:
   ```
   grep 'concurrent_eval_count' /var/log/themisdb/security.log | tail -10
   ```
2. Review policy merge rules for concurrent modification scenarios.
3. Check `policy.concurrent_eval.lock_strategy` (optimistic vs. pessimistic).

### Remediation
1. Switch to pessimistic locking under high concurrency:
   `security.policy.concurrent_eval.lock_strategy=pessimistic`.
2. Increase policy evaluation thread pool: `security.policy.eval_threads`.
3. Confirm inconsistency rate returns to zero.

### Escalation
Escalate to the security team for any confirmed inconsistency under concurrent load, as it
may represent a security control regression.
