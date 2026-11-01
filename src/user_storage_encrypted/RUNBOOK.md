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

# Runbook for user_storage_encrypted

Exact model backend not verifiable from final document text alone.

Operational methods commonly return `Result<T>` error strings. Diagnostic
codes in `error_codes.hpp` are a separate type from the frozen error enum in
`user_storage_encrypted_api_contract.h`; do not assume every method returns a
specific contract code.

## Backend unavailable or mount fails

1. Check that the configured gocryptfs binary is available to the service
   account.
2. On Linux, check `/dev/fuse`, FUSE support, and mount permissions.
3. Check `encrypted_dir`, `mount_point`, and the `encryption` settings in
   `multi_level_storage.levels`.
4. Inspect the returned `Result` message and module/host logs before retrying.
5. Verify the resulting mount state directly on the host. Do not treat
   `unmountAll()` success as proof that all levels unmounted.

Do not silently redirect an encrypted level to the `OFFEN` path; `OFFEN` is
an explicitly unencrypted configuration.

## Stale mount after startup/restart

The plugin attempts best-effort stale mount cleanup during initialization.
On Linux it scans `/proc/mounts` and tries `fusermount -u`, then `umount` in
the cleanup paths. Cleanup failure is non-fatal and does not prove that the
level is healthy.

1. Inspect `/proc/mounts` and compare the affected mount to configured paths.
2. Confirm no active process still owns the mount.
3. Check service-account permissions and availability of unmount utilities.
4. Perform any manual cleanup only under the deployment's mount-recovery
   procedure; recheck the mount state before enabling access.

## KDF or salt issue

The multi-level plugin does not configure a KDF service. For a directly
constructed KDF-enabled backend:

1. Confirm which build selected the KDF path: CMake defines
   `THEMIS_HAS_ARGON2` when libargon2 is found; otherwise PBKDF2-HMAC-SHA256
   is used.
2. Preserve and inspect `<encrypted_dir>/.themis_kdf_salt`; do not replace or
   regenerate a salt for existing data without an approved migration plan.
3. Verify the matching key/provider version and validate recovery on a copy
   before changing production state.

The source does not establish that an old key derivation scheme is compatible
with either current build mode. Confirm compatibility with the data owner.

## Rotation issue

`rotation` configuration is parsed but is not wired to
`KeyRotationScheduler`. First confirm the application explicitly initialized
the scheduler and registered a schedule/callback. The scheduler calls
callbacks with `(level, true, "")`; it swallows exceptions and advances its
check timestamp after an attempt. Its callback arguments therefore do not
prove rotation succeeded.

For `MultiLevelEncryptedStorage::rotateKey()`, inspect provider version, both
container paths, mount state, and preserved data before retrying. The source
does not establish fully atomic recovery; do not delete the old container or
key version until an independent recovery check passes.

## Health and metrics

`checkLevelHealth()` checks backend presence and mounted state, not provider
or scheduler health. `getMetricsText()` emits mount/rotation counters and a
container-size gauge; the source does not update that gauge. Use provider and
host-level checks for those boundaries.

## Escalation

Escalate to the module/host owner if mount state is ambiguous, cleanup fails,
key/salt compatibility is unknown, the scheduler may have advanced after a
failed callback, or rotation left multiple container versions. Preserve
configuration (without secrets), logs, provider key-version metadata, and
mount state for diagnosis.

## References

- [Operations](OPERATIONS.md)
- [Security](SECURITY.md)
- [API behavior and contract discrepancy](API.md)
- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/multi_level_storage.cpp`
