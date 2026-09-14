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
