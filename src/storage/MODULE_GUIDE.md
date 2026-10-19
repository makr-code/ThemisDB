---
Author: "ThemisDB Contributors"
Created: "2026-10-19"
Last Updated: "2026-10-19"
Status: "active"
AI_Used: true
AI_Models: "unknown"
AI_Contributor: "unknown"
Human_Review: true
Human_Author: "ThemisDB Contributors"
---

Exact model backend not verifiable from final document text alone.

# Storage Module Guide

## What this module owns

The storage module owns the durable data plane for ThemisDB. It is responsible for persistence, versioning, WAL durability, recovery, tiering, and integrity-visible maintenance behavior. It is not responsible for query planning, request routing, or application-layer business logic.

## Primary references

- [README.md](./README.md)
- [ARCHITECTURE.md](./ARCHITECTURE.md)
- [ROADMAP.md](./ROADMAP.md)
- [FUTURE_ENHANCEMENTS.md](./FUTURE_ENHANCEMENTS.md)
- [SECURITY.md](./SECURITY.md)
- [PRODUCTION_REQUIREMENTS.md](./PRODUCTION_REQUIREMENTS.md)

## Module flow

1. Reads and writes are directed through the storage engine lifecycle.
2. Versioned keys and timestamps preserve snapshot correctness.
3. WAL durability ensures recoverable writes after crashes or interruptions.
4. Compaction, tiering, and maintenance run as bounded maintenance tasks.
5. Backup and PITR reconstruct state only when manifests and integrity checks pass.

## Main implementation surfaces

### Major files

| File | Responsibility |
|---|---|
| `src/storage/storage_engine.cpp` | lifecycle, dependency validation, production fail-closed checks |
| `src/storage/mvcc_store.cpp` | versioned data model and latest-version reads |
| `src/storage/wal_storage.cpp` | durable WAL record writing and fsync semantics |
| `src/storage/backup_manager.cpp` | backup creation, manifesting, and restore safety |
| `src/storage/pitr_manager.cpp` | point-in-time recovery behavior |
| `src/storage/tiered_storage.cpp` | hot/warm/cold placement and access-driven migration |
| `src/storage/security_signature_manager.cpp` | integrity verification and tamper detection |
| `src/storage/storage_audit_logger.cpp` | audit events and structured diagnostics |

## Operational expectations

- configuration errors should be visible and explicit,
- production environments should not accept default insecure fallbacks,
- recovery failures should block unsafe restore paths,
- maintenance and compaction must leave the system in a consistent state,
- integrity and audit surfaces should remain available for operator review.

## Common maintainer tasks

### When adding a storage dependency

Ensure the dependency is truly production-safe and not a no-op placeholder. If it is not production-safe, the code path should fail closed rather than silently proceed.

### When changing WAL behavior

Validate the exact write and fsync semantics. A change to the WAL contract must be accompanied by explicit recovery reasoning and a focused durability test.

### When modifying backup or PITR logic

Treat the backup manifest as security-sensitive. Do not accept partial or invalid manifests. Recovery should reject missing or invalid state before reconstructing anything.

### When modifying tiering or compaction

Keep the movement and validation semantics bounded. Tier promotion/demotion must remain transparent to readers and must not create state inconsistency.

## Maintainer checklist

- [ ] production safety checks stay strict,
- [ ] version ordering and tombstones remain correct,
- [ ] WAL durability semantics remain explicit,
- [ ] backup/PITR restoration remains fail-closed,
- [ ] tiering and compaction continue to be observable,
- [ ] diagnostics and audit surfaces remain usable.

## Practical guidance

The storage module is easiest to work on if the maintainer always asks: "What is the failure mode if this step fails?" The implementation has been built to expose those failure modes clearly rather than hide them in silent fallback logic.
