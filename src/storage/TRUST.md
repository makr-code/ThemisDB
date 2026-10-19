---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "MAI-Code-1.1-Flash"
AI_Contributor: "MAI-Code-1.1-Flash"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

AI-assisted drafting for this document was performed with MAI-Code-1.1-Flash in the current task session.

# Storage Trust and Security Boundary

## Trust boundary

The storage module sits at the trust boundary between raw durable state and every higher-level database function. It owns the persistence substrate, the versioned state model, and the recovery path. That means any weakness in the storage outputs propagates upward into query correctness, system availability, and integrity-sensitive workflows.

## Key risks

### 1. Silent durability degradation

Risk: a write path appears successful while the durable state is not actually safe. This is mitigated by the WAL semantics and explicit fsync-defined behavior in `wal_storage.cpp`.

### 2. Unsafe default configuration

Risk: production runs with default no-op encryption, key providers, or index managers. The source explicitly rejects these defaults in `storage_engine.cpp` rather than allowing them to proceed.

### 3. Recovery path corruption

Risk: a backup or PITR operation reconstructs from partial or invalid data. The backup manager and recovery logic reject unsupported codec or crypto conditions and verify manifests before trusting restore data.

### 4. Hidden integrity loss

Risk: tampering or corruption is not surfaced clearly. The module includes security and audit surfaces that make integrity and diagnostic events explicit via storage audit and signature management code.

## Security controls

- fail-closed production configuration checks,
- explicit WAL durability requirements,
- manifest-driven backup validation,
- integrity and audit surfaces for storage state,
- failure classification and explicit diagnostics for recovery and pressure events.

## Trust assumptions

The storage module assumes:

- the configured filesystem and backend are reliable enough for the durability contract,
- production deployments do not use insecure default providers,
- recovery paths are validated before restoration,
- maintenance and compaction processes remain observable and bounded.

Any violation of these assumptions should be treated as a real integrity or availability event.

## Incident handling

If storage integrity or replay correctness is in doubt:

1. stop write traffic to limit the blast radius,
2. preserve the current WAL and backup state for forensic review,
3. validate the last known-good backup or snapshot,
4. classify the failure as durability, recovery, or configuration-related,
5. restore only from a validated state after the failing dependency has been corrected.

## Bottom line

The trust posture of ThemisDB depends on storage remaining explicit, deterministic, and fail-closed. The module's design is intentionally conservative: it treats silent degradation as unacceptable and keeps recovery and integrity checking as first-class operational concerns.
