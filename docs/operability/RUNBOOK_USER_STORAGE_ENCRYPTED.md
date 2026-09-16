# Runbook: User Storage Encrypted — Wave D Operability

<!-- Runbook: user_storage_encrypted | Wave D | validated: 2026-09-16 -->
<!-- Links: src/user_storage_encrypted/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the
`user_storage_encrypted` module. Use it to diagnose and remediate
key-rotation failures, backend unavailability, decryption OOM events,
keystore corruption, and audit log overflow.

---

## Scenario 1 — Key Rotation Failure

**Log pattern:** `[ENC_STORAGE:KeyRotationFailed]`

### Symptoms
- Key rotation job reports error in scheduler logs.
- `[ENC_STORAGE:KeyRotationFailed]` emitted with `rotation_id` and `reason` fields.
- New writes may be blocked pending rotation completion.

### Diagnosis
1. Confirm rotation error in application logs:
   ```
   grep '\[ENC_STORAGE:KeyRotationFailed\]' /var/log/themisdb/user_storage_encrypted.log
   ```
2. Check `rotation_id`, `reason`, and `container_path` fields.
3. Verify gocryptfs binary and FUSE are available:
   ```
   which gocryptfs && gocryptfs --version
   ```
4. Confirm key derivation backend (Vault/HSM) is reachable from the host.

### Remediation
1. If the key derivation backend is unavailable, restore network connectivity
   and re-trigger the rotation via the scheduler API.
2. If the rotation left an inconsistent state (partial re-key), restore the
   container from the last known-good snapshot before re-attempting.
3. After resolution, confirm the log shows `[ENC_STORAGE:KeyRotationComplete]`.

### Escalation
Escalate to on-call security team if three consecutive rotation attempts fail
or if container data is inaccessible post-rotation.

---

## Scenario 2 — Encryption Backend Unavailable

**Log pattern:** `[ENC_STORAGE:BackendUnavailable]`

### Symptoms
- Mount or write operations return `BACKEND_UNAVAILABLE` error codes.
- `[ENC_STORAGE:BackendUnavailable]` emitted with `backend_type` and `error` fields.
- Container operations may be queued or rejected depending on fail-closed policy.

### Diagnosis
1. Confirm backend unavailability:
   ```
   grep '\[ENC_STORAGE:BackendUnavailable\]' /var/log/themisdb/user_storage_encrypted.log
   ```
2. Identify the `backend_type` (gocryptfs, FUSE, key-management service).
3. Check system FUSE module:
   ```
   lsmod | grep fuse
   modprobe fuse  # if missing
   ```
4. Check key-management service health (Vault/HSM endpoint status).

### Remediation
1. Restore the backend service or FUSE module.
2. Trigger a controlled remount of affected containers via the scheduler.
3. Confirm `[ENC_STORAGE:MountComplete]` appears in logs before resuming writes.

### Escalation
Escalate to infrastructure team if FUSE kernel module is unavailable or
if the key-management service is degraded beyond local remediation.

---

## Scenario 3 — Decryption OOM

**Log pattern:** `[ENC_STORAGE:DecryptionOOM]`

### Symptoms
- Decryption operations fail with out-of-memory errors.
- `[ENC_STORAGE:DecryptionOOM]` emitted with `container_path` and `rss_mb` fields.
- Application may return `DECRYPT_FAILED` to callers.

### Diagnosis
1. Confirm OOM events:
   ```
   grep '\[ENC_STORAGE:DecryptionOOM\]' /var/log/themisdb/user_storage_encrypted.log
   ```
2. Check current RSS of the ThemisDB process:
   ```
   ps -o pid,rss,comm -p $(pgrep themisdb)
   ```
3. Review recent memory trend in monitoring dashboards (target: `themisdb_process_rss_mb`).
4. Identify callers with abnormally large decryption payload sizes.

### Remediation
1. Reduce concurrent decryption parallelism via configuration (`enc_storage.max_concurrent_decrypts`).
2. Increase container-level memory limits if running in a constrained cgroup.
3. If OOM is caused by a specific caller, rate-limit or reject oversized payloads upstream.
4. Restart the process after confirming memory pressure has subsided.

### Escalation
Escalate to the capacity planning team if OOM events recur after parallelism reduction.

---

## Scenario 4 — Keystore Corruption

**Log pattern:** `[ENC_STORAGE:KeystoreCorruption]`

### Symptoms
- Key derivation or lookup returns checksum or integrity errors.
- `[ENC_STORAGE:KeystoreCorruption]` emitted with `keystore_path` and `checksum_expected` / `checksum_actual`.
- All operations on affected containers will fail closed until keystore is restored.

### Diagnosis
1. Confirm corruption event:
   ```
   grep '\[ENC_STORAGE:KeystoreCorruption\]' /var/log/themisdb/user_storage_encrypted.log
   ```
2. Record `keystore_path`, `checksum_expected`, and `checksum_actual` from the log.
3. Verify keystore file integrity on disk:
   ```
   sha256sum <keystore_path>
   ```
4. Check storage device health and filesystem errors:
   ```
   dmesg | grep -i 'error\|corruption\|EIO'
   ```

### Remediation
1. Do **not** attempt to repair the keystore in place.
2. Restore the keystore from the last known-good backup (encrypted backup path defined in config).
3. Re-derive any keys that were updated after the last backup via the key-management service.
4. Confirm `[ENC_STORAGE:KeystoreVerified]` appears in logs after restore.

### Escalation
Escalate immediately to the security team if keystore corruption is suspected
to be caused by a security incident (unauthorized write access, ransomware, etc.).

---

## Scenario 5 — Audit Log Overflow

**Log pattern:** `[ENC_STORAGE:AuditOverflow]`

### Symptoms
- Audit events are being dropped due to overflow in the audit log buffer.
- `[ENC_STORAGE:AuditOverflow]` emitted with `dropped_count` and `buffer_capacity` fields.
- Compliance posture may be affected if audit events are required for regulatory purposes.

### Diagnosis
1. Confirm overflow events:
   ```
   grep '\[ENC_STORAGE:AuditOverflow\]' /var/log/themisdb/user_storage_encrypted.log
   ```
2. Check `dropped_count` and `buffer_capacity`.
3. Verify audit log sink (file, syslog, SIEM) is reachable and not full:
   ```
   df -h /var/log/themisdb/
   ```
4. Check audit event production rate via monitoring dashboards.

### Remediation
1. Increase audit log buffer capacity in configuration (`enc_storage.audit_buffer_capacity`).
2. Scale up or fix the audit log sink (rotate logs, increase SIEM ingestion rate).
3. After buffer pressure is resolved, confirm `dropped_count` returns to `0` in subsequent log lines.
4. File an incident report documenting the period and count of dropped audit events for compliance records.

### Escalation
Escalate to the compliance team if dropped audit events cover a window required
for regulatory or security audit evidence.

---

## References

- `src/user_storage_encrypted/ROADMAP.md` — Wave D operability contribution
- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h` — error taxonomy
- `tests/integration/test_user_storage_encrypted_soak.cpp` — Wave D soak tests
- `tests/user_storage_encrypted/test_user_storage_encrypted_highcardinality_stress.cpp` — stress tests
- `benchmarks/user_storage_encrypted/bench_encrypted_storage_dedicated_gates.cpp` — benchmark gates
