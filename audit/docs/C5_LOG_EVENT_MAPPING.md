# BSI C5 2026 — Log-Event Mapping

**Author:** ThemisDB Contributors
**Created:** 2026-09-21
**Status:** active
**Closes:** C5-03 (Logging & Monitoring — Kein Mapping von Log-Events auf C5-Kontrollfamilien)

## Scope

This document maps ThemisDB audit log event types to BSI C5 2026 control families.
Source: `include/utils/audit_logger.h` — `SecurityEventType` enum and `logSecurityEvent()`.

## C5 Control Family Mapping

| C5 Control Family | C5 Control IDs | Mapped Event Types | Source Reference |
|---|---|---|---|
| **OPS — Betrieb** | OPS-01, OPS-04 | `CONFIGURATION_CHANGE`, `SERVICE_START`, `SERVICE_STOP`, `HEALTH_CHECK` | `audit_logger.h:34` |
| **SEC — Sicherheit** | SEC-01, SEC-03, SEC-06 | `AUTHENTICATION_SUCCESS`, `AUTHENTICATION_FAILURE`, `AUTHORIZATION_GRANTED`, `AUTHORIZATION_DENIED`, `POLICY_VIOLATION`, `ANOMALY_DETECTED` | `audit_logger.h:34-80` |
| **IAM — Identitäts- und Zugriffsverwaltung** | IAM-01, IAM-02, IAM-04 | `ACCESS_GRANTED`, `ACCESS_DENIED`, `PRIVILEGE_ESCALATION`, `SESSION_START`, `SESSION_END` | `audit_logger.h:34-80` |
| **KRY — Kryptographie** | KRY-01, KRY-02 | `KEY_CREATED`, `KEY_ROTATED`, `KEY_DELETED`, `KEY_REVOKED` | `audit_logger.h:62-65`, `lek_manager.h:46-47` |
| **BCM — Business Continuity** | BCM-01, BCM-03 | `BACKUP_CREATED`, `BACKUP_VERIFIED`, `RECOVERY_INITIATED`, `RECOVERY_COMPLETE`, `FAILOVER_TRIGGERED` | `audit_logger.h:34-80` |
| **LGM — Protokollierung** | LGM-01, LGM-03 | All `SecurityEventType` values — CEF/Syslog format via `formatAsCef()` / `formatAsSyslog()` | `audit_logger.h:540-541` |
| **SCM — Supply-Chain-Management** | SCM-01 | `DEPLOYMENT_STARTED`, `DEPLOYMENT_COMPLETED`, `DEPENDENCY_CHANGE`, `SBOM_GENERATED` | CI workflow events |

## AuditLogger Integration

All events are emitted via:
```cpp
logger->logSecurityEvent(
    SecurityEventType event_type,
    std::string_view component,
    std::string_view resource,
    nlohmann::json metadata
);
```

CEF and Syslog output formats are automatically applied. See `include/utils/audit_logger.h:313-317`.

## Per-Module Coverage

| Module | Primary C5 Controls | Log Call Sites | Status |
|---|---|---|---|
| `auth` | IAM, KRY | `src/auth/jwt_key_rotation_manager.cpp:103,156` | ✅ active |
| `governance` | SEC, OPS | `include/governance/policy_version_history.h:168` | ✅ active |
| `security` | SEC, KRY | `src/security/security_evidence_collector.cpp`, `hsm_key_provider_adapter.cpp` | ✅ active |
| `user_storage_encrypted` | KRY, BCM | `src/user_storage_encrypted/key_rotation_scheduler.cpp` | ✅ active |
| `timeseries` | KRY, BCM | `src/timeseries/ts_encrypted_key_rotation.cpp` | ✅ active |
| `server` | OPS, SEC | `src/server/http_server.cpp` | ✅ active |

## Gap Status

- All 7 C5 control families have at least one mapped event type in ThemisDB's `SecurityEventType` enum.
- Full per-event CI trace evidence (live run logs → C5 control ID) is part of the quarterly C5 evidence review cycle.
- Next quarterly review: Q4 2026.
