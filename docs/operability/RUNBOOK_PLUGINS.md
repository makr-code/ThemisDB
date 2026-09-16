# Runbook: Plugins — Wave D Operability

<!-- Runbook: plugins | Wave D | validated: 2026-09-16 -->
<!-- Links: src/plugins/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `plugins`
module. Use it to diagnose and remediate plugin load failures, invalid
signatures, lifecycle stalls, and audit log overflow incidents.

---

## Scenario 1 — Load Failed

**Log pattern:** `[PLUGINS:LoadFailed]`

### Symptoms
- Plugin load operation fails; plugin remains in UNLOADED or ERROR state.
- `[PLUGINS:LoadFailed]` emitted with `plugin_id`, `error_code`, and `manifest_path` fields.
- Services depending on the plugin may degrade or fail closed.

### Diagnosis
1. Confirm load failures:
   ```
   grep '\[PLUGINS:LoadFailed\]' /var/log/themisdb/plugins.log
   ```
2. Identify `plugin_id` and `error_code`.
3. Check plugin manifest validity:
   ```
   themisdb-admin plugins validate-manifest --plugin-id <id>
   ```
4. Review `plugins_load_failure_total` metric.

### Remediation
1. Fix manifest errors if validation fails; re-deploy plugin package.
2. For ABI mismatch errors: rebuild plugin against the current ThemisDB ABI version.
3. For permission errors: verify plugin binary ACLs.
4. Trigger reload: `themisdb-admin plugins reload --plugin-id <id>`.
5. Confirm plugin transitions to LOADED state.

### Escalation
Escalate to the plugin engineering team if load failures persist across multiple reload
attempts or if the plugin is security-critical.

---

## Scenario 2 — Signature Invalid

**Log pattern:** `[PLUGINS:SignatureInvalid]`

### Symptoms
- Plugin manifest or binary signature fails validation.
- `[PLUGINS:SignatureInvalid]` emitted with `plugin_id`, `manifest_hash`, `expected_hash`, and `validation_stage` fields.
- Plugin is rejected; fail-closed behavior prevents execution.

### Diagnosis
1. Confirm signature validation failures:
   ```
   grep '\[PLUGINS:SignatureInvalid\]' /var/log/themisdb/plugins.log
   ```
2. Identify `validation_stage` (manifest, binary, capability_set).
3. Compare manifest hash against signing registry:
   ```
   themisdb-admin plugins show-signature --plugin-id <id>
   ```
4. Verify the signing key is current and not revoked.

### Remediation
1. Re-sign plugin with current signing key:
   `themisdb-admin plugins sign --plugin-id <id> --key-id <current_key>`.
2. If signing key is compromised: rotate key and re-sign all plugins.
3. Confirm signature validation passes after re-signing.
4. Write incident to security audit log.

### Escalation
All signature validation failures MUST be escalated to the security team immediately,
as they may indicate supply-chain tampering.

---

## Scenario 3 — Lifecycle Stall

**Log pattern:** `[PLUGINS:LifecycleStall]`

### Symptoms
- Plugin lifecycle transition hangs in an intermediate state.
- `[PLUGINS:LifecycleStall]` emitted with `plugin_id`, `current_state`, `target_state`, and `stall_duration_ms` fields.
- Other plugins depending on the stalled plugin may be blocked.

### Diagnosis
1. Confirm lifecycle stalls:
   ```
   grep '\[PLUGINS:LifecycleStall\]' /var/log/themisdb/plugins.log
   ```
2. Identify `current_state` (LOADING, UNLOADING) and `stall_duration_ms`.
3. Check for deadlock or resource contention in plugin registry:
   ```
   themisdb-admin plugins inspect --plugin-id <id>
   ```
4. Review `plugins_lifecycle_stall_total` metric.

### Remediation
1. Force lifecycle reset: `themisdb-admin plugins reset-state --plugin-id <id>`.
2. For persistent stalls: kill plugin process and reload:
   `themisdb-admin plugins force-unload --plugin-id <id> && themisdb-admin plugins reload --plugin-id <id>`.
3. Confirm plugin completes lifecycle transition within normal bounds.

### Escalation
Escalate to the plugin engineering team if lifecycle stall persists beyond 30 seconds or
if multiple plugins are simultaneously stalled.

---

## Scenario 4 — Audit Log Overflow

**Log pattern:** `[PLUGINS:AuditOverflow]`

### Symptoms
- Plugin operation audit log queue exceeds capacity.
- `[PLUGINS:AuditOverflow]` emitted with `queue_depth`, `drop_count`, and `oldest_entry_age_ms` fields.
- Audit trail may have gaps; compliance requirements may be at risk.

### Diagnosis
1. Confirm audit overflow:
   ```
   grep '\[PLUGINS:AuditOverflow\]' /var/log/themisdb/plugins.log
   ```
2. Review `drop_count` and `oldest_entry_age_ms`.
3. Check audit log storage health:
   ```
   df -h /var/log/themisdb/
   ```
4. Review `plugins_audit_drop_total` metric.

### Remediation
1. Increase audit queue capacity: `plugins.audit.max_queue_depth`.
2. Check and free disk space if storage is full.
3. Flush audit log to cold storage to clear backlog.
4. Confirm drop count returns to zero.

### Escalation
Escalate to the security and compliance team if any audit entries are dropped.

---

## Scenario 5 — Plugin Hot-Plug Under Concurrent Registry Operations

**Log pattern:** `[PLUGINS:LifecycleStall]` with `registry_contention=true`

### Symptoms
- Multiple plugins are hot-plugged simultaneously; registry contention causes stalls.
- `plugins_registry_lock_wait_ms` metric elevated.
- Some plugins experience delayed state transitions.

### Diagnosis
1. Check for concurrent hot-plug activity:
   ```
   grep 'registry_contention=true' /var/log/themisdb/plugins.log
   ```
2. Identify the number of simultaneous lifecycle transitions.
3. Review registry lock acquisition metrics.

### Remediation
1. Serialize hot-plug operations: enable `plugins.registry.serialize_lifecycle=true`.
2. Reduce concurrent hot-plug parallelism in deployment tooling.
3. Increase registry lock timeout: `plugins.registry.lock_timeout_ms=5000`.
4. Confirm stalls resolve after serialization.

### Escalation
Escalate to the plugin engineering team if serialization causes unacceptable deployment
latency.
