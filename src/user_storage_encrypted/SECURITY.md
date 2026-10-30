---
Author: "ThemisDB Contributors"
Created: "2026-10-30"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Security - User Storage Encrypted Module

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · TRUST.md -->

Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md). This page describes module controls, not a guarantee of host or key-provider security.

## Security Scope

Security in the user_storage_encrypted module focuses on controlled encrypted mount handling, bounded key-material processing, explicit key-rotation behavior, and diagnosable failure signaling around host-backed encrypted storage operations.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| unsafe subprocess or mount invocation behavior | module-local backend lifecycle and availability controls |
| key-handling regressions during encrypted storage operations | dedicated key derivation and rotation code paths |
| opaque scheduler failure | explicit rotation scheduling surfaces and audit follow-up |
| host-environment induced mount faults | observable backend and orchestration error signaling |

## Implemented Security Controls

- encrypted mount lifecycle behavior is isolated to module-owned backend paths.
- key derivation logic is separated from orchestration and backend control paths.
- key rotation uses dedicated scheduler behavior per security level.
- storage-tier orchestration keeps failure impact bounded to configured levels.

## Security Follow-ups

- expand deterministic validation for scheduler exception and recovery behavior.
- harden path and host-environment validation around encrypted container operations.
- deepen coverage for failure handling across mount, unmount, and rotation error paths.
- Key material is delivered to gocryptfs through standard input rather than command arguments, but remains sensitive in process memory and at the host boundary.
- The built-in KDF uses Argon2id when available and PBKDF2-HMAC-SHA256 otherwise. Preserve per-container salts and key-provider/version state; older iterative-SHA-256 derived keys are not compatible with the current fallback.
- A missing key, invalid path, failed KDF, or failed mount must not be treated as successful access. Restrict mount/config/salt paths and the host account that can invoke gocryptfs.

## Incident Handling

On suspected key exposure or mount compromise, stop access to the affected level, unmount it if safe, preserve scheduler and backend diagnostics, revoke/rotate the affected key through its provider, and validate data access with the retained salt and approved key version before resuming. Follow [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Sourcecode Verification (Module: user_storage_encrypted/security)

- Verified files:
  - src/user_storage_encrypted/gocryptfs_backend.cpp
  - src/user_storage_encrypted/key_derivation_service.cpp
  - src/user_storage_encrypted/key_rotation_scheduler.cpp
  - src/user_storage_encrypted/multi_level_storage.cpp
- Verified controls:
  - isolated encrypted mount lifecycle behavior
  - dedicated key derivation and rotation surfaces
  - bounded failure propagation across tier orchestration

- Additional source references:
  - `src/user_storage_encrypted/gocryptfs_backend.cpp`
  - `src/user_storage_encrypted/key_derivation_service.cpp`
  - `src/user_storage_encrypted/key_rotation_scheduler.cpp`
  - `src/user_storage_encrypted/multi_level_storage.cpp`
  - [Trust and risk model](TRUST.md)