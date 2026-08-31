# ThemisDB Storage Module

<!-- Status: Wave A (Runtime Reliability) | validated: 2026-08-14 -->
<!-- Links: ARCHITECTURE.md · ROADMAP.md · FUTURE_ENHANCEMENTS.md · MODULE_GAPS.md -->

## Module Purpose

The storage module provides persistence, versioned data handling, blob and tiered storage behavior, WAL/durability paths, crash-recovery guarantees, backup-recovery behavior, and storage-level observability for ThemisDB. It forms the foundation for all durable operations and must provide strict ACID guarantees, deterministic recovery, and fail-closed safety.

## Relevant Interfaces

| Interface / File | Role |
|---|---|
| include/storage/storage_module.h | Doxygen module entry point for storage-wide API grouping |
| include/storage/storage_api_contract.h | API contract: error codes, guarantees, determinism model |
| rocksdb_wrapper.cpp | RocksDB integration and durable storage wrapper behavior |
| storage_engine.cpp | storage orchestration and lifecycle behavior |
| mvcc_store.cpp | MVCC version/snapshot behavior |
| wal_storage.cpp | WAL durability path, sync guarantees, replay behavior |
| backup_manager.cpp | backup creation, lifecycle, and restorability behavior |
| pitr_manager.cpp | point-in-time recovery with exact-timestamp determinism |
| tiered_storage.cpp | tiered data placement (hot/warm/cold) with automated migration |
| blob_redundancy_manager.cpp | blob redundancy and integrity behavior |
| encrypted_blob_backend.cpp | encrypted blob backend behavior |
| erasure_coding_backend.cpp | erasure-coded backend behavior |
| compaction_manager.cpp | compaction control and cleanup behavior |
| adaptive_compaction.cpp | adaptive compaction behavior under load |
| index_maintenance.cpp | storage-side index maintenance behavior |
| storage_audit_logger.cpp | storage audit trail generation |
| security_signature_manager.cpp | storage signature/integrity/tamper-detection behavior |

## Scope

**In scope:**
- Durable key-value and blob/tiered storage persistence
- MVCC/WAL/backup/PITR/compaction and lifecycle behavior
- Storage integrity, audit, and maintenance surfaces
- Fail-closed recovery paths and crash-recovery determinism
- ACID guarantee enforcement and verification
- Tiered storage automation and configuration transparency

**Out of scope:**
- Query planning and non-storage API orchestration
- External business-domain logic outside storage boundaries
- Network transport (handled by replication module)
- Encryption key lifecycle (handled by security module)

## Runtime Behavior and Limits

- **Durable write guarantee:** All writes passed to `Storage::write()` are guaranteed durable on return when `sync_mode=SYNC` or `BATCH_SYNC`.
- **MVCC invariant:** Snapshots remain valid for their lifetime; reads are never dirty or inconsistent.
- **WAL durability:** Every durable write is preceded by WAL entry durability; replay is deterministic and idempotent.
- **Crash recovery:** Unclean shutdown recovery is deterministic; no data loss (MVCC-enabled) and no state corruption.
- **Backup guarantee:** Backups are atomic point-in-time copies with full consistency across all layers.
- **PITR accuracy:** Recovery to a specific timestamp ±100ms; no log gaps, no partial transactions.
- **Tiered migration:** Age-based and access-frequency-based tier migration is transparent to readers; no observable latency spike on promotion.
- **Configuration scope:** Behavior is bounded by storage configuration; all limits are enforced with explicit error on overage.
- **Integrity persistence:** `SecuritySignatureManager` requires a persistent RocksDB backend in production and only uses an in-memory store when the caller explicitly opts into the test-only fallback.

## Production Readiness Status

**🟡 Wave A Candidate (Runtime Reliability Focus)**

### Maturity: Release-Candidate (RC)
- ✅ ACID core (persistence, MVCC, WAL, recovery) is proven under stress testing
- ✅ Fail-closed behavior is implemented and chaos-tested for crash recovery
- ✅ Deterministic replay verified under sustained write load
- ✅ Backup/PITR/tiered storage integration is complete
- ⚠️ **Outstanding Wave A Item:** Stress validation under maintenance-overlap scenarios (Q3–Q4 2026)
- ⚠️ **Outstanding Wave A Item:** Benchmark-backed release guardrails for hot paths (Q3–Q4 2026)

### Production Requirements Checklist
- [x] API contract frozen (storage_api_contract.h with explicit error taxonomy)
- [x] Error taxonomy complete (12 error codes, deterministic mapping)
- [x] Fail-closed paths verified (recovery, replay, backup restorability)
- [x] Thread-safety model documented (MVCC isolation, lock ordering)
- [x] Concurrency gates implemented and benchmarked
- [~] Sustained-load chaos evidence collection (Target: Q3 2026)
- [~] Representative-hardware p95/p99 baseline refresh (Target: Q4 2026)

## Thread-Safety and Concurrency Model

### Isolation Guarantee
- **MVCC Isolation:** Snapshots created via `getSnapshot()` are isolated; concurrent writes do not affect ongoing reads.
- **Lock Ordering:** Internal lock hierarchy is strictly enforced: `mvcc_lock` → `wal_lock` → `compaction_lock`. No circular waits.
- **Write Serialization:** Writes are serialized through `write_coordinator` with deterministic ordering.
- **WAL Thread-Safety:** WAL writer is single-threaded; concurrent write calls are queued and processed serially.
- **Compaction Safety:** Compaction runs with a reader-writer lock; snapshot references prevent compaction of live versions.

### Concurrency Model
```
Reads (many)        Writes (1 at a time)      Maintenance
  │                       │                         │
  ├─ snapshot ────────────├─ write ────────────────┤
  │  (MVCC)               │  (sync to WAL)         │ (compaction,
  │  no lock              │  update MVCC            │  backup,
  │  on read              │  → release WAL         │  PITR)
  │                       │                         │
  └─────────────────────────────────────────────────┘
         Isolation via MVCC versioning
```

### Contention Hot-Spots and Mitigations
1. **MVCC map access:** Protected by fine-grained RW lock; read-heavy optimization
2. **WAL buffer filling:** Ring buffer with lock-free enqueue for low-contention writes
3. **Compaction frequency:** Adaptive compaction throttles based on load; avoids write stall
4. **Tiered storage promotion:** Background task; never blocks foreground I/O

## Fail-Closed Safety and Recovery

### Crash Recovery Determinism
1. **WAL Replay:** On startup, all WAL entries are replayed in order; order is idempotent (no double-apply risk).
2. **MVCC Consistency:** Partial transactions are rolled back; orphaned snapshots are cleaned up.
3. **Blob Integrity:** Checksums verified on recovery; corrupted blobs trigger recovery failure (explicit error).
4. **Backup Verification:** Backups are validated on restore; corrupt backup rejected with explicit error.
5. **State Durability:** In-flight compaction is atomic or rolled back; no partial compactions.

### Fail-Closed Behavior
- **On WAL write failure:** Transaction fails immediately; caller never observes partial state.
- **On storage exhaustion:** Write is rejected with `STORAGE_EXHAUSTED` error; state unmodified.
- **On recovery failure:** Startup fails fast with diagnostic context; no silent corruption.
- **On backup failure:** Backup is rolled back; incomplete backup is not used.
- **On PITR timestamp mismatch:** Recovery fails with explicit error; no approximate recovery.

### Recovery Path Validation
- ✅ Unclean shutdown: WAL replay is deterministic (tested via chaos injection)
- ✅ Partial writes: MVCC rolls back orphaned snapshots (tested)
- ✅ Compaction crash: Atomic or rolled back, no state corruption (tested)
- ✅ Backup corruption: Validated on restore, rejected with error (tested)
- ✅ Storage pressure: Tiered migration and cleanup enforce limits (tested)

## Sourcecode Verification (Module: storage/readme)

- Verified core files (15+):
  - `src/storage/rocksdb_wrapper.cpp` — RocksDB wrapper and durability enforcement
  - `src/storage/storage_engine.cpp` — orchestration and lifecycle
  - `src/storage/mvcc_store.cpp` — MVCC versioning and snapshot lifecycle
  - `src/storage/wal_storage.cpp` — WAL durability and replay guarantee
  - `src/storage/backup_manager.cpp` — backup atomicity and restorability
  - `src/storage/pitr_manager.cpp` — point-in-time recovery with exact determinism
  - `src/storage/tiered_storage.cpp` — tiered migration automation
  - `src/storage/blob_redundancy_manager.cpp` — blob redundancy enforcement
  - `src/storage/encrypted_blob_backend.cpp` — encrypted blob storage
  - `src/storage/erasure_coding_backend.cpp` — erasure-coded storage
  - `src/storage/compaction_manager.cpp` — compaction lifecycle
  - `src/storage/adaptive_compaction.cpp` — adaptive compaction
  - `src/storage/index_maintenance.cpp` — index maintenance
  - `src/storage/storage_audit_logger.cpp` — audit trail generation
  - `src/storage/security_signature_manager.cpp` — integrity verification

- Verified behavior surfaces:
  - ACID persistence, MVCC isolation, WAL durability, crash recovery, backup/PITR, tiering, audit

- Documentation structure:
  - API contract in `include/storage/storage_api_contract.h` (error codes, guarantees, determinism)
  - Production requirements in `src/storage/PRODUCTION_REQUIREMENTS.md` (checklist, gates)
  - Forward planning tracked in `ROADMAP.md` (phases, Wave A/B/C/D alignment)
  - Historical entries in `CHANGELOG.md`

---

**Next Step:** See `ROADMAP.md` for Batch 4 Wave A (Runtime Reliability) closure plan and Q3–Q4 2026 timeline.