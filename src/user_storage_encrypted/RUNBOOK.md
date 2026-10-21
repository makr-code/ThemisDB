---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-21
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: unknown
Human_Review: true
Human_Author: ThemisDB Contributors
---

# Runbook for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This runbook is grounded in the actual source paths and runtime behavior present in the module: `gocryptfs_backend.cpp`, `key_derivation_service.cpp`, `key_rotation_scheduler.cpp`, and `multi_level_storage.cpp`.

## Symptom: gocryptfs is missing or backend not available

### Signals

- `checkAvailability()` returns an error
- mount attempts fail before any storage operation executes
- logs record `BACKEND_NOT_AVAILABLE`

### Response

1. Confirm the `gocryptfs` binary is installed and on PATH.
2. Confirm host FUSE support is available for the runtime environment.
3. Re-run backend validation before retrying the storage path.
4. If this is a deployment issue, fix the host environment rather than bypassing the backend check.

### Recovery

- do not continue with plaintext fallback
- treat this as a deployment or host configuration problem
- escalate to environment or infrastructure ownership if the host lacks required support

## Symptom: stale mount remains after a restart

### Signals

- startup logs show stale mount cleanup work
- `/proc/mounts` contains a mount at a previous ephemeral path
- initial level initialization is slower or blocked by orphaned state

### Response

1. Inspect the base path and configured mount points.
2. Confirm the stale mount is not part of the currently active configuration.
3. Allow `reconcileStaleMounts()` to cleanup orphaned entries during initialization.
4. If cleanup fails, verify host permissions and `fusermount` / `umount` availability.

### Recovery

- if cleanup fails, keep the storage path in a degraded but explicit state
- do not proceed with a silent re-mount that hides the stale mount condition

## Symptom: key derivation fails

### Signals

- empty master key or salt
- `argon2id_hash_raw` or PBKDF2 path fails
- `kKeyDerivationFailed` is surfaced in the error contract

### Response

1. Confirm the input key material is not empty and is of valid length.
2. Check whether the environment supports Argon2 by inspecting the build definition or logs for `THEMIS_HAS_ARGON2` and the compile-time branch in `key_derivation_service.cpp`.
3. If `THEMIS_HAS_ARGON2` is not set, confirm the PBKDF2 fallback is expected for this build and record the active path in the environment's deployment notes.
4. Confirm the salt file is valid and persisted correctly when `loadOrCreateSalt()` is used.

### Recovery

- reject the operation and do not continue with an incompatible key path
- rotate to a valid key source or restore the previously known-good salt and key material

## Symptom: rotation scheduling or callback failure

### Signals

- `scheduleRotation()` is present but no rotation callback fires
- schedule state is missing or corrupted in persisted JSON
- callback exceptions are swallowed and operator visibility is low

### Response

1. Inspect rotation state under the configured store.
2. Confirm the level is included in the schedule and the interval is valid.
3. Verify the callback is not re-entering the scheduler or backend unexpectedly.
4. Check the persisted JSON for invalid `last_check_ms` or interval values.

### Recovery

- reset the schedule state only after confirming the correct rotation policy
- keep the old key active if rotation cannot be validated
- escalate if callback or store logic is causing repeated failure

## Symptom: mount or unmount operation fails

### Signals

- `kMountFailed` or `kUnmountFailed`
- logs show shell invocation or mount process issues
- a mount remains half-open after a failed operation

### Response

1. Check that the mount path and encrypted directory are valid.
2. Confirm the host has permission to mount/unmount the target path.
3. Verify the backend is available and the command validator accepts the target parameters.
4. Inspect whether a stale path or stale mount state is blocking the operation.

### Recovery

- attempt cleanup in a controlled way
- treat partial success as evidence of a failed operation, not a resolved one

## Escalation policy

Escalate to the owning subsystem when:

- `gocryptfs` or FUSE is unavailable for more than a low-level maintenance window
- key derivation fails repeatedly across multiple operators or containers
- rotation state cannot be recovered without changing policy assumptions
- mount cleanup or unmount failures threaten persistent production storage

## Rollback guidance

Rollback should be narrow and evidence-backed. The module's source treats invalid backend state and key derivation failure as fail-closed events, so rollback should generally mean:

- restore a previous valid configuration
- re-establish the correct key provider state
- re-run availability and rotation validation before re-enabling the level

Do not attempt a broad revert that re-enables an old plaintext or unvalidated backend path without verifying the host environment and level contract.
