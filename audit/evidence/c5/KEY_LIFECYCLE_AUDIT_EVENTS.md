---
Author: "ThemisDB Contributors"
Created: "2026-09-14"
Last Updated: "2026-11-09"
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
**Last Updated:** 2026-11-09
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

## Source Mapping (baseline reviewed 2026-09-21)

| Event Class | Source Reference | Implementation | Audit Path |
|---|---|---|---|
| key_created | `include/utils/audit_logger.h:62` | `SecurityEventType::KEY_CREATED` via `logSecurityEvent()` | `src/auth/jwt_key_rotation_manager.cpp:68` |
| key_rotated | `include/utils/audit_logger.h:63` | `SecurityEventType::KEY_ROTATED` — JWT rotation + LEK rotation | `src/auth/jwt_key_rotation_manager.cpp:103`, `include/utils/lek_manager.h:46` |
| key_revoked | `include/utils/audit_logger.h` | `SecurityEventType::KEY_DELETED` maps to revocation; `KEY_REVOKED` via `include/utils/lek_manager.h:47` | `src/auth/jwt_key_rotation_manager.cpp:156` |
| key_activated | `include/utils/audit_logger.h` | Emitted during key loading/provisioning flows via `logSecurityEvent()` | `src/security/hsm_key_provider_adapter.cpp` |
| key_deactivated | `include/utils/audit_logger.h` | Emitted on key expiry and rotation-complete; `KEY_ROTATED` + metadata includes old key deactivation | `include/utils/lek_manager.h:258` |
| break_glass_access | `include/utils/audit_logger.h` | `SecurityEventType` extended for privileged access; HSM break-glass via `src/security/hsm_key_provider_adapter.cpp` | `src/security/security_evidence_collector.cpp` |

These paths establish source-level coverage only. They do not establish that the event was exercised, persisted, or included in a release artifact.

## v2.5.0 Release-Candidate Traceability (2026-11-09)

The [candidate manifest](v2.5.0-rc-candidate/manifest.json) and its [schema validation report](v2.5.0-rc-candidate/manifest_validation.json) link the available CI evidence. The latest available compliance run was [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), commit `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`. It failed in SOC 2 evidence-container setup; the SBOM job succeeded, but the C5 packaging job was skipped. This run did not execute the focused key-lifecycle tests.

| Event Class | Source Reference | Focused Test Reference | CI Run and Commit | Persisted Release Evidence | Status / Remediation |
|---|---|---|---|---|---|
| key_created | `include/utils/audit_logger.h:62`; `src/auth/jwt_key_rotation_manager.cpp:68` | `tests/security/test_hsm_key_provider_adapter.cpp::HSMKeyProviderAdapterTest.CreateKeyFromBytes` (key creation/status test; no audit-event assertion) | No event-specific successful test run linked in this evidence index; [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`, is not test evidence | None linked | Open; add event assertion, successful CI trace, and persisted record under [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| key_rotated | `include/utils/audit_logger.h:63`; `src/auth/jwt_key_rotation_manager.cpp:103`; `include/utils/lek_manager.h:46` | `tests/security/test_key_rotation_manager.cpp::KeyRotationManagerTest.KEYROT_01_StartRotation` (rotation state test; no audit-event assertion) | No event-specific successful test run linked in this evidence index; [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`, is not test evidence | None linked | Open; add event assertion, successful CI trace, and persisted record under [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| key_revoked | `include/utils/audit_logger.h`; `include/utils/lek_manager.h:47`; `src/auth/jwt_key_rotation_manager.cpp:156` | `tests/utils/test_utils_lek_rotation_atomic.cpp::LEKRotationTest.RevokeAndCheckKeyState` (revocation-state test; no audit-event assertion) | No event-specific successful test run linked in this evidence index; [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`, is not test evidence | None linked | Open; add event assertion, successful CI trace, and persisted record under [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| key_activated | `include/utils/audit_logger.h`; `src/security/hsm_key_provider_adapter.cpp` | `tests/security/test_hsm_key_provider_adapter.cpp::HSMKeyProviderAdapterTest.CreateKeyFromBytes` exercises active metadata, not an activation audit event | No event-specific successful test run linked in this evidence index; [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`, is not test evidence | None linked | Open; identify/add activation event assertion and trace under [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| key_deactivated | `include/utils/audit_logger.h`; `include/utils/lek_manager.h:258` | `tests/security/test_hsm_key_provider_adapter.cpp::HSMKeyProviderAdapterTest.RotateKey` exercises deprecated metadata, not a deactivation audit event | No event-specific successful test run linked in this evidence index; [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`, is not test evidence | None linked | Open; identify/add deactivation event assertion and trace under [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |
| break_glass_access | `include/utils/audit_logger.h`; `src/security/hsm_key_provider_adapter.cpp`; `src/security/security_evidence_collector.cpp` | No dedicated focused regression test identified | No event-specific successful test run linked in this evidence index; [42321944686](https://github.com/makr-code/ThemisDB/actions/runs/42321944686), `5f8b44bacd78cfb940f5c1bf40456d6617d0284a`, is not test evidence | None linked | Open; add an authorized break-glass event test and trace under [#7817](https://github.com/makr-code/ThemisDB/issues/7817) |

## Gap Status

- The source mapping exists for all six event classes, but current release-specific test/CI and persisted-audit traces are not evidenced.
- The current workflow run is a failed develop compliance run, not successful key-lifecycle test evidence.
- Six event-to-test-to-run-to-record traces remain open for the Q4 2026 review; source mappings must not be treated as runtime evidence.
- Scheduled-rotation coverage: `src/user_storage_encrypted/key_rotation_scheduler.cpp`, `src/timeseries/ts_encrypted_key_rotation.cpp`.
