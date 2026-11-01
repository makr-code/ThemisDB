---
Author: ThemisDB Contributors
Created: 2026-10-21
Last Updated: 2026-10-31
Status: active
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Operations Guide for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

Use the module's actual runtime surfaces and host evidence; do not infer
provider or scheduler health from the module's health result alone.

## Health and metrics

`checkLevelHealth(level)` reports a configured plain level healthy, or for an
encrypted level checks that its backend exists and that `isMounted()` reports
the configured mount point. It does not retrieve a key, probe Vault/HSM, or
check scheduler state. `checkHealth()` combines those per-level outcomes.

`getMetricsText()` emits:

- `user_storage_mounts_active`
- `user_storage_mount_operations_total{operation="mount"|"unmount"}`
- `user_storage_key_rotations_total`
- `user_storage_container_size_bytes`

The mount and rotation counters are updated in the source. The emitted
container-size gauge is not updated by current implementation code and should
not be used as a live size signal. The rotation metric has no level label.

## Startup and degraded conditions

The plugin reads `multi_level_storage.levels`, validates required fields, and
initializes configured levels during `initialize()`. Encrypted levels depend
on gocryptfs, host FUSE support, valid paths, and an available configured
provider. The standalone KDF API depends on its selected build path and
preserved salt/key material.

Stale-mount cleanup is best-effort and non-fatal. The current health endpoint
does not attest cleanup success. Check host mount state directly if startup
reports cleanup problems. `unmountAll()` currently discards individual
unmount errors and returns success; verify actual mount state independently
after shutdown or maintenance.

## Rotation and recovery

`rotation` fields in configuration are parsed but are not wired to
`KeyRotationScheduler`. Do not assume scheduled rotation is active because
configuration contains `auto_rotate: true`. `KeyRotationScheduler` requires
explicit initialization and schedule registration by its caller.

The scheduler swallows callback exceptions and advances/persists its check
timestamp after the callback attempt. It does not signal those failures
through callback arguments. Inspect the callback's external logs/provider
state and verify resulting data access before declaring recovery.

`MultiLevelEncryptedStorage::rotateKey()` has a separate provider/container
flow and does not provide demonstrated all-or-nothing behavior. Preserve the
old data and key version until deployment-specific recovery checks succeed.

## Operator checks

1. Verify the configured JSON shape and required level fields.
2. Verify gocryptfs/FUSE and process permissions on the host.
3. Verify Vault/HSM availability and key ID/version independently.
4. Verify mount state on the host, especially after `unmountAll()` or
   stale-mount cleanup.
5. Confirm rotation is explicitly wired and verify callback outcome outside
   the scheduler.
6. Preserve salt files and provider key versions required to reopen
   containers.

## References

- [Runbook](RUNBOOK.md)
- [Security](SECURITY.md)
- [API behavior and contract discrepancy](API.md)
- `src/user_storage_encrypted/multi_level_storage.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
