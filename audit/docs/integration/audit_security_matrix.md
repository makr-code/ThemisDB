# Audit-Security Integration Matrix

**Document Status:** Finalized (2026-08-18)  
**Wave:** C — Security Production Validation  
**Scope:** Audit ← Security Event Mapping  
**Target:** Audit compliance evidence collection from security module  

---

## Overview

This matrix documents the integration points between the Security module (authentication, authorization, policy, threat detection) and the Audit module (event logging, compliance tagging, forensic evidence).

The audit module receives security events and enriches them with:
1. Tamper-evidence chain (prev_hash → event_hash)
2. Compliance framework tags (ISO27001, GDPR, BSIC5, NIS2, SOC2)
3. Forensic metadata (timestamp, actor, resource, action, result)
4. Recovery markers (for audit trail reconstruction)

---

## Security Event Types → Audit Event Types

### Access Control Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| user_login_success | access_control_login | ISO27001:A.9.2 | actor, timestamp, mfa_status | session_id |
| user_login_failure | access_control_login_failure | ISO27001:A.9.2, NIS2 | actor, timestamp, failure_reason | retry_count |
| user_logout | access_control_logout | ISO27001:A.9.2 | actor, timestamp, session_duration | session_end |
| role_assignment | access_control_change | ISO27001:A.6.2 | actor, target_user, role_id, timestamp | prev_roles |
| privilege_escalation | access_control_privilege_escalation | ISO27001:A.6.2, NIS2 | actor, escalation_from, escalation_to | approval_trace |

### Authentication & Session Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| mfa_enabled | auth_config_change | ISO27001:A.9.4, GDPR:4.11 | actor, user, method, timestamp | prev_config |
| mfa_disabled | auth_config_change | ISO27001:A.9.4, GDPR:4.11 | actor, user, method, timestamp | prev_config |
| session_timeout | session_management | ISO27001:A.9.1 | session_id, actor, timeout_seconds | session_metadata |
| session_revoked | session_management | ISO27001:A.9.1, NIS2 | actor, revoker, reason, timestamp | session_trace |
| certificate_issued | certificate_lifecycle | ISO27001:A.10.1 | cert_id, subject, issuer, expiry | cert_chain |
| certificate_revoked | certificate_lifecycle | ISO27001:A.10.1, NIS2 | cert_id, reason, timestamp | revocation_proof |

### Cryptographic Key Management Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| key_generated | key_lifecycle_generation | ISO27001:A.10.1, BSIC5 | key_id, algorithm, key_size, timestamp | keying_material_hash |
| key_rotated | key_lifecycle_rotation | ISO27001:A.10.1, BSIC5, NIS2 | key_id, old_key_id, new_key_id, reason | rotation_timestamp |
| key_compromised | key_lifecycle_compromise | ISO27001:A.10.1, NIS2 | key_id, compromise_date, affected_data | mitigation_actions |
| key_destroyed | key_lifecycle_destruction | ISO27001:A.10.1, GDPR:4.5 | key_id, reason, timestamp, destruction_method | destruction_proof |
| key_backup_created | key_lifecycle_backup | ISO27001:A.10.1, BSIC5 | key_id, backup_location, hash, timestamp | backup_manifest |
| key_backup_restored | key_lifecycle_restore | ISO27001:A.10.1, NIS2 | key_id, restore_source, restore_date | restore_validation |

### Vault/HSM Provider Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| vault_connection_success | provider_connection_established | ISO27001:A.13.1, NIS2 | provider_type, endpoint, timestamp | connection_trace |
| vault_connection_failure | provider_connection_failed | ISO27001:A.13.1, NIS2 | provider_type, endpoint, error, timestamp | retry_attempt |
| vault_failover_triggered | provider_failover_initiated | NIS2, BSIC5 | from_provider, to_provider, reason, timestamp | failover_proof |
| hsm_failover_success | provider_failover_completed | NIS2, BSIC5 | from_provider, to_provider, keys_migrated | failover_validation |
| hsm_failover_failure | provider_failover_failed | NIS2, BSIC5 | from_provider, to_provider, error, timestamp | incident_trace |
| software_keystore_fallback | provider_degradation_fallback | ISO27001:A.10.1, NIS2 | fallback_reason, expected_provider, timestamp | degradation_alert |

### Policy & Data Protection Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| policy_created | policy_change_creation | ISO27001:A.5, GDPR:5.1 | policy_id, policy_content, creator, timestamp | policy_version |
| policy_updated | policy_change_update | ISO27001:A.5, GDPR:5.1 | policy_id, prev_version, new_version, reason | policy_diff |
| policy_deleted | policy_change_deletion | ISO27001:A.5, GDPR:5.1 | policy_id, deleter, reason, timestamp | policy_archive |
| policy_enforcement_enabled | policy_enforcement_change | ISO27001:A.5, NIS2 | policy_id, resource, enforcement_mode, timestamp | enforcement_trace |
| policy_conflict_detected | policy_evaluation_anomaly | ISO27001:A.5, GDPR:5.1 | conflict_policies, resolution, timestamp | conflict_proof |
| rls_filter_applied | data_filtering_rls | GDPR:2, BSIC5 | actor, resource, filter_criteria, rows_filtered | filtering_trace |
| abac_filter_applied | data_filtering_abac | GDPR:2, NIS2 | actor, attributes, filter_criteria, rows_filtered | attribute_trace |
| data_masking_applied | data_filtering_masking | GDPR:2, BSIC5 | actor, resource, fields_masked, mask_method | masking_trace |
| column_encryption_enabled | data_protection_encryption | ISO27001:A.10.1, GDPR:4.3 | table, columns, algorithm, timestamp | encryption_key_id |

### Threat Detection & Incident Response Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| sql_injection_attempt_detected | threat_detection_injection | ISO27001:A.12.6, NIS2 | query, pattern, actor, timestamp | query_trace |
| privilege_escalation_attempt | threat_detection_escalation | ISO27001:A.6.2, NIS2 | actor, escalation_path, timestamp | attempt_trace |
| unauthorized_access_attempt | threat_detection_unauthorized | ISO27001:A.9.4, NIS2 | actor, resource, reason_denied, timestamp | access_decision |
| suspicious_activity_detected | threat_detection_anomaly | ISO27001:A.12.4, NIS2 | activity_type, anomaly_score, actor, timestamp | baseline_comparison |
| incident_confirmed | incident_response_confirmed | NIS2, BSIC5 | incident_id, severity, confirmed_date | incident_metadata |
| incident_response_initiated | incident_response_action | NIS2, BSIC5 | incident_id, response_type, responder, timestamp | response_trace |
| incident_resolved | incident_response_closure | NIS2, BSIC5 | incident_id, resolution, timestamp, post_mortem | closure_evidence |

### Compliance & Regulatory Events

| Security Event | Audit Event Type | Compliance Tags | Forensic Metadata | Recovery |
|---|---|---|---|---|
| gdpr_data_export_request | compliance_data_subject_request | GDPR:3, 6 | subject_id, request_date, export_scope | export_timestamp |
| gdpr_data_deletion_request | compliance_data_deletion | GDPR:5, 17 | subject_id, deletion_scope, confirmed_date | deletion_proof |
| data_retention_policy_applied | compliance_retention | GDPR:5, BSIC5 | data_type, retention_period, expiry_date | retention_trace |
| audit_log_retention_hold | compliance_hold | GDPR:17, SOC2 | hold_reason, hold_duration, held_by | hold_proof |
| compliance_audit_executed | compliance_verification | ISO27001, SOC2 | audit_scope, auditor, audit_date | audit_report_link |

---

## Compliance Framework Query Schema

### ISO 27001:2022 (Information Security Management)

**Audit Query Pattern:**

```sql
SELECT event_type, actor, resource, timestamp
FROM audit_events
WHERE compliance_tags CONTAINS 'ISO27001:A.*'
  AND event_date >= '2026-01-01'
ORDER BY timestamp DESC
```

**Example Events Returned:**
- access_control_change (A.6.2 - Role assignment)
- key_lifecycle_rotation (A.10.1 - Key management)
- policy_change_creation (A.5 - Policy control)
- certificate_lifecycle (A.10.1 - PKI)

### GDPR/DSGVO (General Data Protection Regulation)

**Audit Query Pattern:**

```sql
SELECT event_type, subject_id, action, timestamp
FROM audit_events
WHERE compliance_tags CONTAINS 'GDPR:*'
  AND (event_type LIKE '%data%' OR event_type LIKE '%deletion%')
ORDER BY timestamp DESC
```

**Example Events Returned:**
- compliance_data_subject_request (Article 15 - Access rights)
- compliance_data_deletion (Article 17 - Right to be forgotten)
- data_filtering_rls (Article 32 - Data protection)

### BSI C5 (Cloud Computing Compliance Criteria)

**Audit Query Pattern:**

```sql
SELECT event_type, resource, control_area, timestamp
FROM audit_events
WHERE compliance_tags CONTAINS 'BSIC5:*'
ORDER BY timestamp DESC
```

**Example Events Returned:**
- key_lifecycle_rotation (C5.1 - Encryption)
- data_filtering_rls (C5.3 - Data isolation)
- threat_detection_injection (C5.4 - Threat detection)

### NIS2 (European Cyber Resilience Act)

**Audit Query Pattern:**

```sql
SELECT event_type, severity, actor, timestamp
FROM audit_events
WHERE compliance_tags CONTAINS 'NIS2'
ORDER BY timestamp DESC
```

**Example Events Returned:**
- threat_detection_injection (Threat detection)
- incident_response_initiated (Incident response)
- provider_failover_initiated (System resilience)

---

## Integration Status

**Security → Audit Mapping:** ✅ Complete  
**Compliance Query Capability:** ✅ Validated  
**Tamper-Evidence Chain:** ✅ Proven under load  
**Forensic Traceability:** ✅ Event correlation demonstrated  

---

## References

- Security Module: `src/security/WAVE_C_CLOSURE_EVIDENCE.md`
- Audit Module: `audit/WAVE_C_AUDIT_EVIDENCE.md`
- Policy Gates: `docs/governance/WAVE_C_POLICY_GATE_EVIDENCE.md`
