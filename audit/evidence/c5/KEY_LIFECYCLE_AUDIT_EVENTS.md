---
Author: "ThemisDB Contributors"
Created: "2026-09-14"
Last Updated: "2026-11-10"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Team"
---

> Exact model backend not verifiable from final document text alone.

# BSI C5 Key Lifecycle Audit Events

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-11-10
**Status:** active

## Scope

Canonical event classes for cryptographic key lifecycle audit coverage.

## Required event classes

| Event Class | Required Metadata |
|---|---|
| key_created | key_id, algorithm, actor, source, timestamp |
| key_rotated | old_key_id, new_key_id, actor, reason, timestamp |
| key_revoked | key_id, actor, reason, timestamp |
| key_activated | key_id, actor, scope, timestamp |
| key_deactivated | key_id, actor, reason, timestamp |
| break_glass_access | key_id, actor, approval_ref, duration, timestamp |

## Coverage rule

Quarterly C5 evidence must map each class above to one source-verified test, CI run, or production audit trace reference.

## Source Mapping (verified 2026-11-10)

`SecurityEventType` declares the key-management values `KEY_CREATED`,
`KEY_ROTATED`, `KEY_ROTATION_FAILED`, `KEY_DELETED`,
`KEY_REVOCATION_FAILED`, and `KEY_ACCESS`
(`include/utils/audit_logger.h:61-67`). These declarations do not establish
production emission. Among the requested successful lifecycle classes,
`KEY_CREATED` and `KEY_ROTATED` have enum values; no `KEY_REVOKED`,
`KEY_ACTIVATED`, `KEY_DEACTIVATED`, or break-glass event value is declared.
The successful-operation emitters found are JWT rotation (`KEY_ROTATED`),
JWT revocation (`KEY_DELETED`), and LEK expired-key rotation (`KEY_ROTATED`);
LEK `revokeKey()` itself does not emit an event.

| Event Class | Source Reference | Implementation | Audit Path |
|---|---|---|---|
| key_created | `include/utils/audit_logger.h:62` defines `KEY_CREATED`; no production key operation calls `logSecurityEvent(KEY_CREATED, ...)` | Enum/string support only; `LEKManager::ensureLEKExists()` generates LEKs without emitting this event | No production audit path verified |
| key_rotated | `include/utils/audit_logger.h:63`; `src/auth/jwt_key_rotation_manager.cpp:52-105`; `src/utils/lek_manager.cpp:544-562` | `JWTKeyRotationManager::rotateActiveKey()` emits `KEY_ROTATED` when a logger is attached; LEK auto-rotation emits it for expired keys. The JWT event lacks canonical `old_key_id`, `new_key_id`, actor, and reason fields (`new_kid` is logged instead) | `AuditLogger::logSecurityEvent()` → `logEvent()` → JSONL append in `src/utils/audit_logger.cpp`; focused real-operation test added |
| key_revoked | `src/auth/jwt_key_rotation_manager.cpp:114-159`; `src/utils/lek_manager.cpp:331-351` | JWT revocation logs `KEY_DELETED` (not `KEY_REVOKED`); LEK `revokeKey()` logs no audit event. `include/security/security_api_contract.h` has a separate `KEY_REVOKED` status code, not an `AuditLogger` event | No required `key_revoked` audit event verified |
| key_activated | `src/auth/jwt_key_rotation_manager.cpp:84-105,168-191` | Rotation/reactivation updates key status but emits no distinct activation event; rotation only emits `KEY_ROTATED` | No `key_activated` audit event verified |
| key_deactivated | `src/auth/jwt_key_rotation_manager.cpp:75-81` | Rotation demotes the prior active key to passive without a distinct deactivation event | No `key_deactivated` audit event verified |
| break_glass_access | `include/utils/audit_logger.h`; production sources under `src/` | No required event class or production break-glass operation emitting it was identified | No production audit path verified |

The `key_rotated` API path is exercised through `JWTKeyRotationManager::rotateActiveKey()` and its persisted JSONL record is decoded and asserted by `tests/auth/test_c5_key_lifecycle_audit_evidence.cpp`. No in-repository application runtime construction/logger attachment for this manager was found. CI is configured to retain JUnit plus the JSONL record; no successful CI run or uploaded artifact is claimed until that workflow executes. This establishes only the tested API path, not runtime deployment, incident, or certification evidence.

## v2.5.0 Release-Candidate Traceability (2026-11-09)

The [candidate manifest](v2.5.0-rc-candidate/manifest.json) and its [schema validation report](v2.5.0-rc-candidate/manifest_validation.json) link the available CI evidence. The latest available compliance run was [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), commit `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`. It failed in SOC 2 evidence-container setup; the SBOM job succeeded, but the C5 packaging job was skipped. This run did not execute the focused key-lifecycle tests.

| Event Class | Source Reference | Focused Test Reference | CI Run and Commit | Persisted Evidence | Status / Remediation |
|---|---|---|---|---|---|
| key_created | Enum only: `include/utils/audit_logger.h:62`; no emitting production path found | No successful create-event test can be added against a current production emitter | Not run | None; CI artifact cannot establish an event the production flow does not emit | Open; production event emission is missing |
| key_rotated | `src/auth/jwt_key_rotation_manager.cpp:52-105` | `tests/auth/test_c5_key_lifecycle_audit_evidence.cpp::C5KeyLifecycleAuditEvidence.ProductionRotationPersistsAuditRecord` exercises the production API operation and parses the persisted `KEY_ROTATED` record | No CI run claimed; the release-critical PR gate and mainline CTest workflows are wired to run it | CI output: `c5-key-lifecycle-audit/key-rotated.jsonl`; JUnit is retained with the CI artifact | API event/persistence path is tested; canonical metadata and application-runtime wiring are unverified, and a successful CI trace remains pending |
| key_revoked | JWT uses `KEY_DELETED` at `src/auth/jwt_key_rotation_manager.cpp:156`; LEK `revokeKey()` has no logger call | Existing revocation tests assert state, not the required event | Not run | None for required event | Open; no production `key_revoked` emission |
| key_activated | `src/auth/jwt_key_rotation_manager.cpp:84-105,168-191` changes state but emits no distinct event | No event assertion possible against current production behavior | Not run | None | Open; no production `key_activated` emission |
| key_deactivated | `src/auth/jwt_key_rotation_manager.cpp:75-81` demotes prior active key without a distinct event | No event assertion possible against current production behavior | Not run | None | Open; no production `key_deactivated` emission |
| break_glass_access | No required event producer or break-glass operation identified in `src/` | No production-path test identified | Not run | None | Open; no production event/operation identified |

## Gap Status

- Only `key_rotated` has a production API emitter, a test of that real operation, and a CI artifact path configured. No application runtime wiring was found. The tested JWT event lacks canonical `old_key_id`, `new_key_id`, actor, and reason metadata; a successful CI run is still pending.
- The other five required event classes have no production emitter matching the required event name. `KEY_DELETED` on JWT revoke is not relabeled as `key_revoked`; LEK revocation emits no audit event.
- The current workflow run is a failed develop compliance run, not successful key-lifecycle test evidence.
- Five event-to-test-to-run-to-record traces remain open for the Q4 2026 review; configured CI artifacts are not uploaded-run evidence.
- Scheduled-rotation coverage: `src/user_storage_encrypted/key_rotation_scheduler.cpp`, `src/timeseries/ts_encrypted_key_rotation.cpp`.
