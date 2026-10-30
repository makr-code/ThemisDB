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

# Security - Storage Module

<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md · TRUST.md -->

Report vulnerabilities privately via [project SECURITY.md](../../SECURITY.md). This page does not certify all stored data or backups as encrypted.

## Security Scope

Security in the storage module focuses on deterministic durability and recovery behavior, storage integrity/audit signals, bounded blob/tiering behavior, and explicit error signaling in maintenance/replay paths.

## Threat Model

| Threat | Current Mitigation Surface |
|---|---|
| silent durability regression | explicit WAL/replay and storage lifecycle outcomes |
| integrity tampering in storage path | signature manager and audit logging surfaces |
| hidden backup/recovery failure | explicit backup/PITR failure signaling |
| opaque blob/tiering degradation | bounded backend behavior with diagnosable outcomes |

## Implemented Security Controls

- storage and replay operations expose explicit result states.
- encrypted blob storage uses AES-256-GCM and verifies the authentication tag; it protects only data routed through that backend.
- the default no-op field-encryption and key-provider implementations are rejected only when production mode is detected (`THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT=production`).
- backup/PITR and maintenance faults remain observable.
- blob/redundancy/tiering failures surface deterministic outcomes.

## Security Follow-ups

- broaden fault-injection coverage for replay/recovery corruption scenarios.
- expand stress coverage for concurrent write + maintenance contention.
- tighten diagnostics taxonomy across storage recovery incident classes.
- The default storage audit logger writes log records to files without encryption; protect the log directory and access path as sensitive data.
- Backup encryption is an explicit option, not an implication of field or blob encryption. Protect backup, WAL, manifest, and recovery files according to deployment policy.
- Configure and validate production-mode flags and inject real encryption/key providers; otherwise development defaults may return plaintext or dummy key material.

## Permission and Incident Handling

Restrict database, WAL, backup, audit-log, and key-provider access to the service identity and authorized operators. If integrity or confidentiality is in doubt, pause writes, preserve WAL and backup evidence, validate a known-good backup and its integrity before restoring, and verify recovery on an isolated copy. Follow [SOP-05](../../SOP.md) and [SOP-08](../../SOP.md).

## Sourcecode Verification (Module: storage/security)

- Verified files:
  - src/storage/wal_storage.cpp
  - src/storage/backup_manager.cpp
  - src/storage/pitr_manager.cpp
  - src/storage/security_signature.cpp
  - src/storage/security_signature_manager.cpp
  - src/storage/storage_audit_logger.cpp
  - src/storage/encrypted_blob_backend.cpp
- Verified controls:
  - explicit durability/recovery error signaling
  - encrypted-blob authentication and storage integrity surfaces
  - bounded encrypted blob behavior

- Additional source references:
  - `src/storage/storage_engine.cpp`
  - `src/storage/encrypted_blob_backend.cpp`
  - `src/storage/storage_audit_logger.cpp`
  - [Trust and risk model](TRUST.md)