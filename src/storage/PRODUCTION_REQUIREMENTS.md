---
Author: "ThemisDB Contributors"
Created: "2026-06-01"
Last Updated: "2026-10-31"
Status: "review"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot coding agent"
Human_Review: false
Human_Author: "ThemisDB Contributors"
Source_Level: "level1"
SOT_Domain: "module-behavior"
Canonical_References:
  - "../../include/storage/storage_engine.h"
  - "../../include/storage/wal_storage.h"
  - "../../include/storage/backup_manager.h"
  - "../../include/storage/security_signature_manager.h"
  - "../../include/storage/encrypted_blob_backend.h"
  - "storage_engine.cpp"
  - "wal_storage.cpp"
  - "backup_manager.cpp"
  - "../../tests/storage/test_storage_engine_prod.cpp"
Related_Milestone: "DOC-WEEKLY-2026-W44"
---

Exact model backend not verifiable from final document text alone.

# Production configuration checks

This is an implementation-based checklist, not a normative deployment
certification. Production requirements depend on which storage components are
used and on application wiring; the code does not establish a universal
module-wide requirement to enable every component.

## Checks supported by current source

- **StorageEngine dependencies:** provide real encryption and key-provider
  implementations. The default implementations are no-op/dummy and reject
  production mode. A null index manager is rejected in production mode. The
  mode is detected from `THEMIS_PRODUCTION_MODE` values `1`, `true`, or
  `production`, or `THEMIS_ENVIRONMENT=production`.
- **RocksDB WAL:** `StorageEngine::open()` sets RocksDB's `enable_wal` option
  to true. This says nothing about the standalone `WALStorage` component.
- **Standalone WAL:** when used, configure its directory and select
  `fsync_on_write` deliberately; the option defaults to true but is
  configurable. Check every returned `Result`.
- **Backup:** choose backup destination, compression/encryption options, and
  provider credentials explicitly for the deployment. Backup encryption is
  optional in `BackupOptions`; this API does not imply all backups are
  encrypted.
- **Encrypted blobs:** use `EncryptedBlobBackend` only for data routed through
  that decorator and supply its required backend and key provider. It does not
  encrypt RocksDB, standalone WAL, backup files, or audit logs.

## Not established by this checklist

The source reviewed here does not establish a module-wide WAL scheduling
requirement, backup schedule/retention default, universal audit-logging
requirement, mandatory signature verification on every read, cloud credential
policy, exact PITR accuracy, or a specific RPO/RTO. Configure and verify such
controls at the application/deployment layer.

TODO: add a deployment-specific profile and cite its bootstrap configuration,
runbook, and executed restore/security tests before converting this guidance
into binding operational requirements.
