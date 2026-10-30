---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-30"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "GitHub Copilot"
Human_Review: false
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Storage Trust and Security Boundary

## Trust boundary

The storage module sits at the trust boundary between raw durable state and every higher-level database function. It owns the persistence substrate, the versioned state model, and the recovery path. That means any weakness in the storage outputs propagates upward into query correctness, system availability, and integrity-sensitive workflows.

## Key risks

### 1. Silent durability degradation

Risk: a write path appears successful while the durable state is not actually safe. This is mitigated by the WAL semantics and explicit fsync-defined behavior in `wal_storage.cpp`.

### 2. Unsafe default configuration

Risk: a deployment uses default no-op encryption, key providers, or index managers without production mode being detected. `storage_engine.cpp` rejects some defaults only when `THEMIS_PRODUCTION_MODE` or `THEMIS_ENVIRONMENT=production` is set; deployments must set the intended mode and inject real providers.

### 3. Recovery path corruption

Risk: a backup or PITR operation reconstructs from partial or invalid data. The backup manager and recovery logic reject unsupported codec or crypto conditions and verify manifests before trusting restore data.

### 4. Hidden integrity loss

Risk: tampering or corruption is not surfaced clearly. The module includes security and audit surfaces that make integrity and diagnostic events explicit via storage audit and signature management code.

## Security controls

- fail-closed production configuration checks,
- explicit WAL durability requirements,
- manifest-driven backup validation,
- AES-256-GCM blob encryption when the encrypted blob backend is configured,
- integrity and audit surfaces for storage state,
- failure classification and explicit diagnostics for recovery and pressure events.

## Trust assumptions

The storage module assumes:

- the configured filesystem and backend are reliable enough for the durability contract,
- production deployments do not use insecure default providers,
- only the encrypted-blob path encrypts its wrapped payload; this does not establish that WAL, backup, manifest, or audit files are encrypted,
- storage audit records are protected by host/filesystem access controls because the logger writes them as files,
- recovery paths are validated before restoration,
- maintenance and compaction processes remain observable and bounded.

Any violation of these assumptions should be treated as a real integrity or availability event.

## Bottom line

Storage is a confidentiality and integrity boundary, but this page is not a certification that every persistence path is encrypted or recoverable. Verify provider configuration, file permissions, backup encryption, and restore evidence for the actual deployment.

## Incident handling

When storage integrity or replay correctness is in doubt, stop write traffic, preserve the current WAL and backup state, classify the issue as durability, recovery, key-provider, or configuration related, and test restoration from a validated backup before replacing production state. Do not report recovery as complete until integrity checks pass. Use [project SECURITY.md](../../SECURITY.md) for private vulnerability reporting and [SOP-08](../../SOP.md) for operational response.
