---
Author: ThemisDB Contributors
Created: 2026-06-01
Last Updated: 2026-10-31
Status: review
AI_Used: true
AI_Models: unknown
AI_Contributor: GitHub Copilot
Human_Review: false
Human_Author: ThemisDB Contributors
---

# Production Requirements - user_storage_encrypted

Exact model backend not verifiable from final document text alone.

This page separates source-verifiable implementation requirements from
deployment policy. It is not a production-readiness certification. For
implementation details see [README.md](README.md), [ROADMAP.md](ROADMAP.md),
and [FUTURE_ENHANCEMENTS.md](FUTURE_ENHANCEMENTS.md).

## Requirements visible in the implementation

- For an encrypted level, provide `encrypted_dir`, `mount_point`, and
  `encryption.key_id`; the current validator rejects missing values.
- Configure a working `gocryptfs` executable and host FUSE support. On Linux,
  the backend checks `gocryptfs` in PATH and `/dev/fuse`.
- For the Vault provider, configure `encryption.vault_addr` and make
  `VAULT_TOKEN` available in the process environment. For HSM, configure
  `encryption.hsm_library`. Provider availability and permissions remain
  deployment dependencies.
- Keep the salt file and the matching key/provider version needed to reopen a
  container when a KDF-enabled backend is used.
- Treat operational failures as failures; do not assume plaintext fallback.

The multi-level plugin currently creates `GocryptfsBackend` without a KDF
service and does not connect parsed `rotation` settings to
`KeyRotationScheduler`. Direct users of the backend and scheduler must manage
those APIs separately.

## Configuration and algorithm caveats

The KDF implementations evidenced in source are Argon2id when CMake finds
libargon2 and PBKDF2-HMAC-SHA256 otherwise; HKDF is not implemented or selected
by this module. The implementation also does not enforce a 256-bit master-key
minimum or establish that both build modes are compatible with existing
containers. No migration guarantee for prior key-derivation schemes is
established here.

`MultiLevelEncryptedStorage::validateConfiguration()` checks required paths
and key IDs; it does not validate all security, authorization, resource-limit,
audit-logging, timeout, retry, or deployment-mode policies listed in older
versions of this document. Those controls must be reviewed and supplied by
the deployment/host owner where applicable.

The current source does not establish requirements for
`THEMIS_PRODUCTION_MODE` / `THEMIS_ENVIRONMENT`, Apache Ranger, audit logging,
file permissions such as `0700`, key `mlock`, or external retry policy. Do not
represent these as module-enforced controls.

## Contract and implementation mismatch

`include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
defines normative atomic-mount-cleanup, key-zeroing, transactional-rotation,
error, and threading guarantees. These are not uniformly demonstrated by the
operational implementation: mount failure has no cleanup attempt, selected
key buffers only are cleared, rotation has ignored unmount/rollback outcomes,
and scheduler callbacks can run from either its polling thread or a manual
caller-thread `triggerRotation()`. That callback only invokes the registered
function; it does not itself call `MultiLevelEncryptedStorage::rotateKey()`.
See the [source-by-source comparison](API.md); reconcile these guarantees
before adopting the header comments as production evidence.

## Deployment review checklist

- [ ] Confirm encrypted-level paths, provider settings, service-account
      permissions, FUSE support, and recovery access to salts/keys.
- [ ] Confirm the selected KDF build mode and validate compatibility with
      data created by that same mode.
- [ ] Decide how key rotation is initiated and monitored; the plugin does not
      wire its parsed rotation options to the scheduler.
- [ ] Review partial mount/unmount and key-rotation failure behavior for the
      deployment; do not infer atomic recovery from contract comments alone.
- [ ] Verify which health/metrics signals are consumed and account for the
      unpopulated container-size metric.
- [ ] Independently verify host-level authorization, audit, backup, recovery,
      resource, and incident-response requirements.

## Canonical implementation references

- `include/user_storage_encrypted/multi_level_storage.hpp`
- `include/user_storage_encrypted/key_derivation_service.hpp`
- `include/user_storage_encrypted/key_rotation_scheduler.hpp`
- `include/user_storage_encrypted/user_storage_encrypted_api_contract.h`
- `src/user_storage_encrypted/multi_level_storage.cpp`
- `src/user_storage_encrypted/gocryptfs_backend.cpp`
- `src/user_storage_encrypted/key_derivation_service.cpp`
- `src/user_storage_encrypted/key_rotation_scheduler.cpp`
- `src/user_storage_encrypted/CMakeLists.txt`
