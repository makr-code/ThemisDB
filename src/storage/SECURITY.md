> **Sicherheitshinweis:** Security-Angaben gegen aktuelle Build-Flags, Codepfade und Tests validieren.

<!-- Status: current | validated: 2026-05-31 -->
<!-- Links: README.md · ARCHITECTURE.md · ROADMAP.md -->

# Security - Storage Module

> Report vulnerabilities via the project-level [SECURITY.md](../../../SECURITY.md).

## Threat Model

| Threat | Mitigation surface |
|---|---|
| Unauthorized writes/reads via storage API paths | storage engine and wrapper operation validation paths |
| Data tampering in persisted artifacts | storage signature and verification surfaces (`security_signature*`) |
| Operational integrity loss during storage lifecycle events | wrapper lifecycle guards and audit logging (`storage_audit_logger`) |
| Replay/recovery misuse in recovery paths | WAL and PITR flow controls (`wal_storage`, `pitr_manager`) |
| Backend misuse or misconfiguration in blob paths | backend-specific validation and redundancy/encryption wrappers |

## Security Controls

- Signature and verification helpers for integrity-sensitive storage assets.
- Storage audit logger for traceable storage operation events.
- Encrypted blob backend wrapper for backend-agnostic encrypted blob storage paths.
- Recovery and replay surfaces designed with explicit error/result handling.

## Data Handling

- Security-sensitive metadata and signatures are handled in dedicated storage security components.
- Blob and storage artifacts can be wrapped by encryption and redundancy layers before backend persistence.
- Audit and verification surfaces provide traceability for storage-side security events.

## Known Limitations

- Security posture depends on runtime configuration of storage backends, key providers, and deployment hardening.
- Not all operational hardening guarantees can be inferred from this module in isolation; cross-module security enforcement remains required.

## Sourcecode Verification (Module: storage/security)

- Verified files:
	- `src/storage/security_signature.cpp`
	- `src/storage/security_signature_manager.cpp`
	- `src/storage/encrypted_blob_backend.cpp`
	- `src/storage/storage_audit_logger.cpp`
	- `src/storage/wal_storage.cpp`
	- `src/storage/pitr_manager.cpp`
	- `src/storage/rocksdb_wrapper.cpp`
	- `src/storage/storage_engine.cpp`
- Verified controls:
	- integrity/signature behavior and audit surfaces
	- encrypted blob backend integration paths
	- recovery/replay related control paths
