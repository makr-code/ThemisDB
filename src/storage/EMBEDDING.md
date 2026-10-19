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

# Storage Embedding and Contract Mapping

## Purpose

This document describes how the storage module embeds the runtime guarantees of ThemisDB into concrete design contracts: WAL durability, MVCC versioning, backup/PITR safety, and production fail-closed configuration.

## Contract embedding model

The module turns abstract database promises into operational behavior in a few concrete places:

| System promise | Embedding in source | Operational meaning |
|---|---|---|
| durable writes | `wal_storage.cpp` | writes are flushed before commit completion is acknowledged |
| snapshot correctness | `mvcc_store.cpp` | readers see a versioned state with logical ordering |
| recovery safety | `backup_manager.cpp`, `pitr_manager.cpp` | restore is manifest-checked and fail-closed |
| production safety | `storage_engine.cpp` | unsafe default implementations are rejected in production |
| integrity visibility | `storage_audit_logger.cpp`, `security_signature_manager.cpp` | auditable state and tamper checks are available |

## Embedding principles

### 1. Durability is a runtime contract, not a comment

The WAL implementation uses explicit fd-level write and fsync logic to ensure local persistence. Unlike a veneer around a memory map, this path actively validates that data reached durable storage before the caller sees success.

### 2. Versioning is a logical data model, not just a cache

Versioned keys form the storage-level identity for each value revision. The code updates a per-key latest timestamp cache for fast reads while preserving time-travel semantics through the versioned key format.

### 3. Recovery is a first-class workflow

Backup creation is treated as a structured, verifiable process because a restore is only useful if the stored snapshot is trustworthy. The manifest-backed remote/local restore flow in `backup_manager.cpp` is a clear example of this design choice.

### 4. Safety is enforced at configuration boundaries

The production mode guardrails in `storage_engine.cpp` are part of the module's trust model. They prevent silent fallback to insecure implementations in the most sensitive runtime path.

## Embedded runtime flows

### Write flow

1. caller submits write to `StorageEngine` or storage API,
2. key is versioned with an HLC timestamp,
3. WAL receives durable record,
4. version cache is updated for latest key lookup,
5. read layer sees consistent versioned state.

### Recovery flow

1. backup or PITR operation loads manifest metadata,
2. payload integrity is validated,
3. codec or crypto prerequisites are checked,
4. restore path either reconstructs a consistent snapshot or fails explicitly.

### Maintenance flow

1. compaction and tiering operate on versioned keys,
2. storage pressure and diagnostics classify the failure,
3. operator-visible recovery instructions are generated through the audit and diagnostics surfaces.

## Risks and limits

The embedded model assumes:

- local persistence is available where WAL is enabled,
- the configured storage backend matches the runtime environment,
- backup artifacts are not silently accepted without integrity validation,
- production deployment includes real dependency implementations rather than default placeholders.

If any of these assumptions fail, the module surfaces explicit error states instead of silently continuing.

## Why this matters

The storage subsystem is the foundation for the rest of ThemisDB. If it is not durable, versioned, and fail-closed, every higher level becomes logically untrustworthy. The contract mapping above makes the module's behavior intelligible to maintainers and operators without requiring them to infer the invariant from scattered code paths.
