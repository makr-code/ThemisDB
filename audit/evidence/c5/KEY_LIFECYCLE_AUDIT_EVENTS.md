# BSI C5 Key Lifecycle Audit Events

**Author:** ThemisDB Contributors  
**Created:** 2026-09-14  
**Last Updated:** 2026-09-14  
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

## Source-Verified Coverage (v2.4.0 — 2026-09-21)

| Event Class | Source Reference | Implementation | Audit Path |
|---|---|---|---|
| key_created | `include/utils/audit_logger.h:62` | `SecurityEventType::KEY_CREATED` via `logSecurityEvent()` | `src/auth/jwt_key_rotation_manager.cpp:68` |
| key_rotated | `include/utils/audit_logger.h:63` | `SecurityEventType::KEY_ROTATED` — JWT rotation + LEK rotation | `src/auth/jwt_key_rotation_manager.cpp:103`, `include/utils/lek_manager.h:46` |
| key_revoked | `include/utils/audit_logger.h` | `SecurityEventType::KEY_DELETED` maps to revocation; `KEY_REVOKED` via `include/utils/lek_manager.h:47` | `src/auth/jwt_key_rotation_manager.cpp:156` |
| key_activated | `include/utils/audit_logger.h` | Emitted during key loading/provisioning flows via `logSecurityEvent()` | `src/security/hsm_key_provider_adapter.cpp` |
| key_deactivated | `include/utils/audit_logger.h` | Emitted on key expiry and rotation-complete; `KEY_ROTATED` + metadata includes old key deactivation | `include/utils/lek_manager.h:258` |
| break_glass_access | `include/utils/audit_logger.h` | `SecurityEventType` extended for privileged access; HSM break-glass via `src/security/hsm_key_provider_adapter.cpp` | `src/security/security_evidence_collector.cpp` |

## Gap Status

- All 6 required event classes have production source coverage via `SecurityEventType` in `include/utils/audit_logger.h`.
- Full per-event test mapping (CI run trace → event class) is in progress; target: Q4 2026 quarterly review.
- Scheduled-rotation coverage: `src/user_storage_encrypted/key_rotation_scheduler.cpp`, `src/timeseries/ts_encrypted_key_rotation.cpp`.
