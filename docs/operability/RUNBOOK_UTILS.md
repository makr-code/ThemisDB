# Runbook: Utils — Wave D Operability

<!-- Runbook: utils | Wave D | validated: 2026-09-16 -->
<!-- Links: src/utils/ROADMAP.md · docs/operability/README.md -->

## Overview

This runbook covers operator-critical incident scenarios for the `utils`
module. Use it to diagnose and remediate helper overload, privacy-audit
failures, fallback triggering, runtime errors, and crypto/key-management
helper contract violations.

---

## Scenario 1 — Helper Overload

**Log pattern:** `[UTILS:HelperOverload]`

### Symptoms
- Shared helper dispatch latency increases sharply.
- `[UTILS:HelperOverload]` emitted with `helper_name`, `queue_depth`, and `latency_ms` fields.
- Downstream modules (e.g., core, importers, content) may experience degraded throughput.

### Diagnosis
1. Confirm helper overload:
   ```
   grep '\[UTILS:HelperOverload\]' /var/log/themisdb/utils.log
   ```
2. Identify `helper_name` (e.g., `rate_limiter`, `thread_pool`, `pii_scanner`) and `queue_depth`.
3. Check `thread_pool_manager` active thread count and saturation metrics:
   ```
   grep 'thread_pool_stats' /var/log/themisdb/utils.log | tail -20
   ```
4. Review `utils_helper_dispatch_latency_p99_ms` metric in Prometheus.

### Remediation
1. If `thread_pool` is saturated: increase pool capacity in `utils.thread_pool.max_threads`.
2. If `rate_limiter` is congested: adjust token-bucket parameters or temporarily raise limit.
3. If `pii_scanner` is backlogged: enable parallel scanner slots via `utils.pii.max_parallel_slots`.
4. Confirm `[UTILS:HelperOverload]` stops appearing and latency returns to baseline.

### Escalation
Escalate to the platform team if helper queue depth remains above high-water mark for more
than 2 minutes or if p99 latency exceeds 200ms.

---

## Scenario 2 — Privacy Audit Failure

**Log pattern:** `[UTILS:PrivacyAuditFailed]`

### Symptoms
- Privacy audit pipeline fails to process records.
- `[UTILS:PrivacyAuditFailed]` emitted with `record_id`, `detector_name`, and `error_code` fields.
- PII detection or audit-log writes may be dropped.

### Diagnosis
1. Confirm privacy audit failures:
   ```
   grep '\[UTILS:PrivacyAuditFailed\]' /var/log/themisdb/utils.log
   ```
2. Identify `detector_name` (NER, regex, gazetteer) and `error_code`.
3. Check NER model availability and timeout settings:
   ```
   grep 'ner_detection_engine' /var/log/themisdb/utils.log | tail -20
   ```
4. Review `utils_privacy_audit_failure_total` metric.

### Remediation
1. If NER model is unavailable: verify model path and restart detector (`utils.pii.ner_model_path`).
2. If regex engine is timing out: review pattern list for ReDoS candidates; raise per-pattern
   timeout `utils.pii.regex_timeout_ms`.
3. If audit-log writes are failing: check `audit_logger` FD state and disk health.
4. Confirm `[UTILS:PrivacyAuditFailed]` stops appearing after remediation.

### Escalation
Escalate to the privacy engineering team if failure rate exceeds 0.1% for more than 1 minute
or if a legal PII retention deadline may be missed.

---

## Scenario 3 — Fallback Triggered

**Log pattern:** `[UTILS:FallbackTriggered]`

### Symptoms
- Runtime helper falls back to secondary implementation.
- `[UTILS:FallbackTriggered]` emitted with `helper_name`, `primary_error`, and `fallback_impl` fields.
- Performance may degrade; fallback implementations have lower throughput.

### Diagnosis
1. Confirm fallback activation:
   ```
   grep '\[UTILS:FallbackTriggered\]' /var/log/themisdb/utils.log
   ```
2. Identify `primary_error` (e.g., `zstd_unavailable`, `hkdf_key_not_found`).
3. Check library availability for the affected helper:
   ```
   ldd /opt/themisdb/libthemis_utils.so | grep -i zstd
   ```
4. Review `utils_fallback_activation_total` metric.

### Remediation
1. Restore primary implementation: reinstall missing library or restore config.
2. If fallback is acceptable long-term: document and update `utils.fallback.allowed_helpers`.
3. Monitor throughput under fallback to ensure SLO is not breached.
4. Confirm primary implementation resumes on next helper initialization.

### Escalation
Escalate to the platform team if the fallback state persists beyond 10 minutes or if
throughput under fallback drops below SLO threshold.

---

## Scenario 4 — Runtime Error

**Log pattern:** `[UTILS:RuntimeError]`

### Symptoms
- Unhandled runtime error in a shared utility path.
- `[UTILS:RuntimeError]` emitted with `component`, `error_code`, `stack_hint` fields.
- May cause cascading failures in dependent modules.

### Diagnosis
1. Confirm runtime errors:
   ```
   grep '\[UTILS:RuntimeError\]' /var/log/themisdb/utils.log
   ```
2. Identify `component` and `error_code` (e.g., `zstd_bomb_detected`, `lz4_overflow`).
3. Review error taxonomy reference: `include/utils/utils_error_codes.h` (codes 7300–7363).
4. Check for concurrent modification or resource exhaustion indicators.

### Remediation
1. For compression errors (7340–7349): check input data for decompression bomb patterns;
   enable bomb detection threshold `utils.compression.max_ratio=1024`.
2. For serialization errors (7350–7359): validate schema version compatibility.
3. For key/crypto errors (7360–7363): check HKDF key cache coherence and PKI cert pin validity.
4. Restart affected component with `systemctl restart themisdb-utils` if state is corrupt.

### Escalation
Escalate to the security team for any error in range 7360–7363 (key/crypto errors). Escalate
to the platform team for repeated runtime errors exceeding 10/minute.

---

## Scenario 5 — Crypto / Key-Management Contract Violation

**Log pattern:** `[UTILS:PrivacyAuditFailed]` with `error_code=7360..7363`

### Symptoms
- PKI or HKDF key operations fail with contract violation.
- `pki_client` cert pinning enforced; connection rejected due to mismatched certificate.
- `hkdf` key cache returns stale or unavailable key material.

### Diagnosis
1. Confirm crypto contract violations:
   ```
   grep 'error_code=736[0-3]' /var/log/themisdb/utils.log
   ```
2. Identify whether failure is in `pki_client` (cert pinning) or `hkdf_key_cache`.
3. For cert pinning failures: compare pinned SHA-256 against current server certificate.
4. For HKDF failures: check key cache TTL and provider connectivity.

### Remediation
1. For cert pinning: rotate pin to new certificate SHA-256 in `utils.pki.pinned_key_hash`.
2. For HKDF key cache miss: force cache refresh via `themisdb-admin utils refresh-key-cache`.
3. Confirm crypto operations resume with no error_code 7360–7363 in logs.
4. Update runbook with new pin value after rotation.

### Escalation
All key/crypto contract violations MUST be escalated to the security team immediately
regardless of duration or rate.
