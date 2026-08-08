# Security - User Storage Encrypted Module

<!-- Status: current | validated: 2026-08-08 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

Report vulnerabilities via project-level SECURITY.md.

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