# Runbook: Config — Wave D Operability

<!-- Runbook: config | Wave D | validated: 2026-09-16 -->
<!-- Links: src/config/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `config`
module. Use it to diagnose and remediate config reload failures, file-watcher
stalls, config validation failures, and resolver fallback scenarios.

---

## Scenario 1 — Reload Failed

**Log pattern:** `[CONFIG:ReloadFailed]`

### Symptoms
- Config reload operation fails; running configuration may be stale.
- `[CONFIG:ReloadFailed]` emitted with `config_key`, `version`, and `error_code` fields.
- Services consuming the affected config key may behave unexpectedly.

### Diagnosis
1. Confirm reload failures:
   ```
   grep '\[CONFIG:ReloadFailed\]' /var/log/themisdb/config.log
   ```
2. Identify `config_key` and `error_code`.
3. Validate config file syntax:
   ```
   themisdb-admin config validate --file <path>
   ```
4. Review `config_reload_failure_total` metric.

### Remediation
1. Fix config file syntax errors if validation fails.
2. For permission errors: verify file ownership and ACLs.
3. Trigger manual reload: `themisdb-admin config reload --key <key>`.
4. Confirm `[CONFIG:ReloadFailed]` stops and affected services use updated config.

### Escalation
Escalate to the platform team if reload failures persist for more than 2 minutes or if
critical config keys are affected.

---

## Scenario 2 — File Watcher Stall

**Log pattern:** `[CONFIG:FileWatcherStall]`

### Symptoms
- File watcher stops delivering change events.
- `[CONFIG:FileWatcherStall]` emitted with `watch_path`, `stall_duration_ms` fields.
- Config hot-reload stops working; services may use stale configuration.

### Diagnosis
1. Confirm file-watcher stalls:
   ```
   grep '\[CONFIG:FileWatcherStall\]' /var/log/themisdb/config.log
   ```
2. Identify `watch_path` and `stall_duration_ms`.
3. Check kernel inotify limits:
   ```
   cat /proc/sys/fs/inotify/max_user_watches
   ```
4. Verify the watched path is accessible:
   ```
   ls -la <watch_path>
   ```

### Remediation
1. If inotify limit is exhausted: increase `fs.inotify.max_user_watches` and restart watcher.
2. If watched path is missing: restore the config directory or update watch path config.
3. Restart file watcher: `themisdb-admin config restart-watcher`.
4. Confirm change events are delivered after restart.

### Escalation
Escalate to the infrastructure team if inotify limits cannot be increased or if stall
persists after watcher restart.

---

## Scenario 3 — Validation Failed

**Log pattern:** `[CONFIG:ValidationFailed]`

### Symptoms
- Config validation rejects incoming configuration change.
- `[CONFIG:ValidationFailed]` emitted with `config_key`, `validator_name`, and `violation` fields.
- Config change is rejected; service continues using previous configuration.

### Diagnosis
1. Confirm validation failures:
   ```
   grep '\[CONFIG:ValidationFailed\]' /var/log/themisdb/config.log
   ```
2. Identify `validator_name` (schema, range, dependency) and `violation`.
3. Run validation in dry-run mode:
   ```
   themisdb-admin config validate --dry-run --file <new_config>
   ```
4. Review config schema documentation: `docs/config/CONFIG_SCHEMA.md`.

### Remediation
1. Correct the configuration change to satisfy the validator.
2. For range violations: adjust values to within documented allowed ranges.
3. For dependency violations: ensure dependent keys are consistent.
4. Re-apply the corrected configuration and confirm validation passes.

### Escalation
Escalate to the platform team if the validation rule appears to be incorrect and a schema
update may be needed.

---

## Scenario 4 — Resolver Fallback

**Log pattern:** `[CONFIG:ResolverFallback]`

### Symptoms
- Config resolver falls back to secondary resolution chain.
- `[CONFIG:ResolverFallback]` emitted with `config_key`, `primary_error`, and `fallback_resolver` fields.
- Performance may degrade under high resolver fallback rate.

### Diagnosis
1. Confirm resolver fallback activations:
   ```
   grep '\[CONFIG:ResolverFallback\]' /var/log/themisdb/config.log
   ```
2. Identify `primary_error` (e.g., `store_unavailable`, `key_not_found`) and `fallback_resolver`.
3. Test primary resolver connectivity:
   ```
   themisdb-admin config test-resolver --resolver primary
   ```
4. Review `config_resolver_fallback_total` metric.

### Remediation
1. Restore primary resolver: fix connectivity or restart resolver backend.
2. If fallback is acceptable: document and set `config.resolver.fallback_allowed=true`.
3. Monitor fallback throughput to ensure SLO is not breached.
4. Confirm primary resolver resumes after recovery.

### Escalation
Escalate to the platform team if primary resolver remains unavailable for more than 5 minutes.

---

## Scenario 5 — Concurrent File-Watcher and Reload Race Condition

**Log pattern:** `[CONFIG:FileWatcherStall]` and `[CONFIG:ReloadFailed]` in same time window

### Symptoms
- File watcher and reload operations interfere under high-churn config update rate.
- Both `FileWatcherStall` and `ReloadFailed` appear in overlapping time windows.
- Config consistency may be temporarily violated.

### Diagnosis
1. Check for overlapping stall and reload failure events:
   ```
   grep -E 'FileWatcherStall|ReloadFailed' /var/log/themisdb/config.log | head -40
   ```
2. Determine if file-write frequency exceeds watcher debounce threshold.
3. Review `config.watcher.debounce_ms` setting.

### Remediation
1. Increase watcher debounce: `config.watcher.debounce_ms=500`.
2. Serialize config updates through a single writer process.
3. Enable config change coalescing: `config.watcher.coalesce_changes=true`.
4. Confirm both metrics return to zero after configuration change.

### Escalation
Escalate to the platform team if config consistency cannot be restored within 10 minutes.
