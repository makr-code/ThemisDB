---
Author: "ThemisDB Contributors"
Created: "2026-09-21"
Last Updated: "2026-11-09"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# BSI C5 2026 — Log-Event Mapping

**Author:** ThemisDB Contributors
**Created:** 2026-09-21
**Last Updated:** 2026-11-09
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
- Source mapping is distinct from run-time evidence. No current release-specific per-event run/artifact trace is linked for the families below.
- Next quarterly review: Q4 2026.

## v2.5.0 Release-Candidate Run/Artifact Trace (2026-11-09)

The [candidate manifest](../evidence/c5/v2.5.0-rc-candidate/manifest.json) and [validation report](../evidence/c5/v2.5.0-rc-candidate/manifest_validation.json) are linked from the Wave C closure package. The latest available compliance run was [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686) on commit `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`; it failed overall, and the C5 packaging job was skipped. Its successful SBOM job produced artifacts but no key-event, incident-drill, or log-event traces. Each missing trace remains assigned to ThemisDB maintainers under [#7817](https://github.com/makr-code/ThemisDB/issues/7817).

| Control Family | Current Run / Artifact Trace | Gap and Remediation |
|---|---|---|
| OPS | None for the mapped operational events | No successful event-specific CI run or persisted record linked; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| SEC | None for the mapped security events | No successful event-specific CI run or persisted record linked; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| IAM | None for the mapped identity/access events | No successful event-specific CI run or persisted record linked; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| KRY | Source/test references are listed in [Key Lifecycle Audit Events](../evidence/c5/KEY_LIFECYCLE_AUDIT_EVENTS.md); no successful event-specific run or persisted record | Open; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| BCM | [Incident drill index](../evidence/c5/INCIDENT_DRILL_EVIDENCE_INDEX.md) has historical test paths only | No current drill run/artifact; BCM-03 remains partial; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| LGM | None for persisted per-event output | No successful event-specific CI run or persisted record linked; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| SCM | [Run 42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686) produced SBOM artifacts `13553885640`, `13553855703`, and `13553162780`; signatures/provenance and the C5 package were not produced | Partial; this develop run is not target-release provenance; owner: ThemisDB maintainers, [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |

The linked manifest validator checks schema and run-reference consistency only. It does not verify CI test results, artifact contents, signatures, or independent certification.
