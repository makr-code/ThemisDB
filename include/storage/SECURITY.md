<!-- Status: current | validated: 2026-04-06 -->

# include/storage/ — Security

> Security scope covers **public header interfaces** of the storage module.
> Runtime security implementation is in [`../../src/storage/`](../../src/storage/).

---

## Scope

This document covers the security properties exposed through the public storage
headers, including:
- Content integrity (`security_signature.h`, `security_signature_manager.h`)
- Audit logging (`storage_audit_logger.h`)
- WAL durability and tamper evidence (`wal_storage.h`)
- MVCC snapshot isolation (`mvcc_store.h`, `raft_mvcc_bridge.h`)
- Blob confidentiality (planned: `encrypted_blob_backend.h`)
- GPU buffer management for compression (`gpu_compression.h`)

---

## Threat Model

| Threat | Impact | Mitigation |
|--------|--------|-----------|
| Data corruption at rest | Silent data loss | `security_signature.h` signs all stored objects; signatures verified on read |
| Tampered WAL entries | Incorrect state replay | WAL entries include CRC32C checksums; `wal_storage.h` verifies on apply |
| Unauthorised blob access | Data exfiltration | `storage_audit_logger.h` records all blob read/write/delete operations |
| MVCC phantom reads | Dirty or non-repeatable reads | `mvcc_store.h` enforces snapshot isolation; timestamp-bound reads |
| Stale Raft log application | Data inconsistency after failover | `raft_mvcc_bridge.h` checks log index monotonicity before applying |
| Disk exhaustion denial-of-service | Write failures / data loss | `disk_space_monitor.h` exposes threshold callbacks; watermark alerts |
| Compaction starvation | Unbounded space amplification | `compaction_manager.h` exposes priority API; admin can force compaction |
| GPU memory leakage | Sensitive data in GPU DRAM | `gpu_compression.h` zeroes device buffers post-compression |
| Backup exfiltration | Complete data exposure | `backup_manager.h` requires caller-supplied credentials; no built-in storage |
| Schema migration data exposure | Partial plaintext during migration | `online_schema_migration.h` performs atomic cut-overs; no dual-write window |
| Audit log tampering | Compliance violation | `storage_audit_logger.h` writes to append-only log; rotation is atomic |
| Blob metadata leakage via NLP | PII in extracted metadata | `nlp_metadata_extractor.h` — callers must sanitise output before persistence |

---

## Security Controls

### Content Integrity
- `security_signature.h` provides Ed25519 / HMAC-SHA256 content signatures.
- `security_signature_manager.h` manages key lifecycle including rotation.
- Signatures are verified automatically by `storage_engine.h` on read when
  configured; callers can also verify explicitly.

### Audit Logging
- `storage_audit_logger.h` records `(timestamp, caller-identity, operation,
  key, result)` tuples to an append-only audit log.
- Log entries are flushed to disk synchronously before acknowledging writes
  when `AuditSync::SYNC` mode is configured.
- Audit log files are rotated atomically; partial files are discarded.

### WAL Durability & Integrity
- `wal_storage.h` uses group commit with fsync before acknowledging writes.
- Each WAL entry includes a CRC32C checksum and monotonic sequence number.
- Replay (`raft_mvcc_bridge.h`) validates sequence continuity; gaps trigger
  a recovery alert rather than silent skip.

### MVCC Snapshot Isolation
- `mvcc_store.h` ensures readers always see a consistent snapshot.
- Write-write conflicts are detected and surfaced as `WRITE_CONFLICT` errors;
  callers use `transaction_retry_manager.h` for automatic retry.

### Backup Security
- `backup_manager.h` does not store credentials; callers inject them.
- `pitr_manager.h` verifies backup chain integrity before beginning restore.
- Backups are checksummed at file and manifest levels.

### GPU Buffer Management
- `gpu_compression.h` guarantees device buffer zeroing after each
  compression/decompression operation on CUDA ≥ 12.0.
- Callers on older CUDA must synchronise the stream before buffer reuse.

---

## Known Limitations

1. **Blob encryption not yet in public headers** — field-level blob encryption
   is planned via `encrypted_blob_backend.h` (Target: Q2 2026).  Until then,
   encryption at rest must be provided by the underlying storage medium (disk
   encryption, cloud-provider CMK).
2. **`nlp_metadata_extractor.h` PII risk** — the NLP extractor may surface PII
   from blob content.  Callers are responsible for sanitising extracted metadata
   before persisting it to the metadata store.
3. **Audit log not cryptographically chained** — log entries are individually
   signed but not forward-chained.  A sophisticated attacker with write access
   to the audit log directory could delete individual records without detection.
   Cryptographic chaining is tracked for Q3 2026.
4. **`gpu_compression.h` CUDA < 12.0 buffer zeroing** — see GPU Buffer
   Management above.  Non-CUDA builds are unaffected.

---

## Reporting Vulnerabilities

See [`../../SECURITY.md`](../../SECURITY.md) for the project-wide responsible
disclosure policy.
