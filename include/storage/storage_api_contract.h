/*
 * ThemisDB | File: storage_api_contract.h | Version: 1.0.0
 * Author: Copilot | Maturity: 🟢 PRODUCTION-READY | Status: Phase 1 — Frozen Contract
 * Purpose: Frozen storage module API contracts for the active v1.x major line.
 */

/**
 * @file storage_api_contract.h
 * @brief Frozen storage module API contracts for the active v1.x major line.
 *
 * This header defines the normative, binding contract for the ThemisDB storage
 * module covering:
 *   - WAL contract (durability ordering, idempotent replay)
 *   - MVCC contract (read-your-writes, snapshot isolation, dirty-read prevention)
 *   - Recovery contract (crash restart replays WAL from last checkpoint)
 *   - Backup/PITR (consistent snapshot, arbitrary-timestamp restore)
 *   - Tiering (hot→warm→cold transparency to readers)
 *   - Canonical error taxonomy
 *
 * ## Contract Scope
 *
 * These contracts are binding for all v1.x implementations:
 *   - WAL backends (WALStorage)
 *   - MVCC stores (MvccStore, MvccChainPruner)
 *   - RocksDB wrapper (RocksDBWrapper)
 *   - Backup and PITR managers (BackupManager, PitrManager)
 *   - Tiered storage (TieredStorage, BlobStorageManager)
 *   - Compaction (CompactionManager, AdaptiveCompaction)
 *
 * ## Versioning
 *
 * Stable within v1.x.  Breaking changes require v2.0 with migration notes.
 *
 * @see src/storage/ROADMAP.md — Phase 1 frozen contract items
 * @see include/storage/wal_storage.h    — WAL interface
 * @see include/storage/mvcc_store.h     — MVCC interface
 * @see include/storage/pitr_manager.h   — PITR interface
 * @see include/storage/tiered_storage.h — Tiering interface
 */

#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <string>

namespace themis {
namespace storage {

// ============================================================================
// § 1  WAL Durability Contract
//
// The Write-Ahead Log is the primary durability primitive.  All implementations
// MUST satisfy:
//
//   a) WRITE-BEFORE-ACK: Every committed mutation is fully persisted in the WAL
//      before the operation acknowledgement is returned to the caller.  The ACK
//      must not be sent after an in-memory write only.
//
//   b) IDEMPOTENT REPLAY: Replaying the WAL from any valid checkpoint position
//      produces exactly the same committed state as the original sequence.
//      Duplicate application of a WAL entry (identified by sequence number) is
//      a no-op.
//
//   c) MONOTONIC SEQUENCE: WAL entries are assigned monotonically increasing
//      sequence numbers.  No entry may be assigned a number lower than any
//      previously written entry; gaps are not permitted.
//
//   d) CHECKPOINT ADVANCE: A checkpoint record may only advance the checkpoint
//      position to a sequence number that has already been fully flushed to
//      the primary store.
// ============================================================================

/// Magic marker at the start of every WAL entry (little-endian 0xDBAB1234).
inline constexpr std::uint32_t kWalMagic = 0xDBAB1234U;

/// Maximum single WAL entry payload size.
inline constexpr std::size_t kWalMaxEntryBytes = 256 * 1024 * 1024;  ///< 256 MiB.

/// Maximum number of WAL entries between forced checkpoints.
/// Implementations MUST trigger a checkpoint when this limit is reached.
inline constexpr std::uint64_t kWalCheckpointInterval = 100'000;

/// Hard timeout for a WAL write operation.  Writes that exceed this return
/// WAL_WRITE_FAILED rather than blocking indefinitely.
inline constexpr std::chrono::milliseconds kWalWriteHardTimeout{500};

// ============================================================================
// § 2  MVCC Contract
//
// The MVCC layer provides:
//
//   a) READ-YOUR-WRITES: Within a transaction, a read of a key that was written
//      by the same transaction MUST return the value written in that transaction
//      (not a snapshot from before the transaction began).
//
//   b) SNAPSHOT ISOLATION: Reads observe the committed state at transaction
//      start time.  Concurrent uncommitted writes are never visible.
//
//   c) DIRTY-READ PREVENTION: No transaction may observe uncommitted changes
//      from any other transaction.  This invariant holds across crash-restart
//      after WAL replay.
//
//   d) VERSION CHAIN BOUNDED: The MVCC version chain for any single key is
//      bounded by kMvccMaxVersionsPerKey.  Compaction MUST prune expired
//      versions before this limit is reached.
// ============================================================================

/// Maximum concurrent read-write transactions per storage engine instance.
inline constexpr int kMvccMaxConcurrentTransactions = 65536;

/// Maximum number of MVCC versions retained per key before forced compaction.
inline constexpr int kMvccMaxVersionsPerKey = 1024;

/// Maximum transaction lifetime.  Transactions open beyond this duration are
/// automatically rolled back with TRANSACTION_TIMEOUT.
inline constexpr std::chrono::minutes kMvccMaxTransactionLifetime{30};

// ============================================================================
// § 3  Recovery Contract
//
// On restart after a crash or clean shutdown:
//
//   a) The storage engine locates the last valid checkpoint in the WAL.
//   b) All WAL entries after the checkpoint are replayed in sequence-number order.
//   c) Entries with sequence numbers ≤ the checkpoint are skipped (idempotency).
//   d) If the WAL tail is partially written (torn write), the partial entry is
//      discarded and a RECOVERY_PARTIAL_ENTRY diagnostic event is emitted.
//   e) Recovery is complete when the first entry after the WAL tail is reached.
//
// Partial recovery (§ 3.d) MUST surface a non-fatal diagnostic; it MUST NOT
// silently discard entries that appear structurally complete.
// ============================================================================

/// Maximum time the recovery procedure may run before RECOVERY_TIMEOUT is raised.
inline constexpr std::chrono::minutes kRecoveryHardTimeout{10};

// ============================================================================
// § 4  Backup / PITR Contract
//
// Backup and Point-in-Time Recovery semantics:
//
//   a) A backup captures a fully consistent snapshot; no in-flight transaction
//      can produce a partial state in the backup artifact.
//
//   b) PITR restore targets any committed timestamp within the retention window.
//      Requesting a future timestamp returns PITR_INVALID_TIMESTAMP.
//
//   c) A concurrent write during backup must not corrupt the backup artifact;
//      writes after the backup start timestamp are captured in WAL only and
//      not reflected in the snapshot artifact.
//
//   d) Restore from backup is idempotent; restoring the same backup twice
//      produces the same committed state.
// ============================================================================

/// Minimum backup retention window guaranteed by the engine.
inline constexpr std::chrono::hours kBackupMinRetention{24 * 7};  ///< 7 days.

/// Maximum number of concurrent backup operations.
inline constexpr int kMaxConcurrentBackups = 2;

// ============================================================================
// § 5  Tiering Contract
//
// The hot→warm→cold tiering transition is transparent to readers:
//
//   a) A read issued while a value is being migrated between tiers MUST return
//      the correct value (possibly from the source tier during migration).
//
//   b) The tier of a value MUST NOT influence the read result; only latency may
//      differ across tiers.
//
//   c) A tiering migration MUST NOT lose the value.  If migration fails,
//      the value remains in the source tier and TIERING_MIGRATION_FAILED is
//      emitted to the diagnostic channel.
// ============================================================================

/// Maximum time a tier-migration operation may run before it is abandoned.
inline constexpr std::chrono::minutes kTieringMigrationTimeout{60};

// ============================================================================
// § 6  Error Taxonomy
//
// Codes 1–99: WAL; 100–199: MVCC/transaction; 200–299: recovery;
// 300–399: backup/PITR; 400–499: tiering/compaction; 9xxx: internal.
// ============================================================================

/**
 * @brief Canonical error codes for the ThemisDB storage module.
 *
 * All storage operation failures MUST map to one of these codes before being
 * returned to callers or emitted in metrics/audit events.
 */
enum class StorageErrorCode : int {
    // ── WAL ───────────────────────────────────────────────────────────────────
    /// WAL write failed (I/O error, timeout, or disk full).
    WAL_WRITE_FAILED            = 1,
    /// WAL sequence number is non-monotonic (implementation bug).
    WAL_SEQUENCE_ERROR          = 2,
    /// WAL file is corrupted beyond recovery.
    WAL_CORRUPTED               = 3,

    // ── MVCC / Transaction ────────────────────────────────────────────────────
    /// Write-write conflict detected; transaction must be retried.
    TRANSACTION_CONFLICT        = 100,
    /// Transaction exceeded kMvccMaxTransactionLifetime.
    TRANSACTION_TIMEOUT         = 101,
    /// MVCC version chain has reached kMvccMaxVersionsPerKey.
    VERSION_CHAIN_FULL          = 102,
    /// Read attempted on a key that does not exist (not found).
    KEY_NOT_FOUND               = 103,

    // ── Recovery ──────────────────────────────────────────────────────────────
    /// Recovery procedure did not complete within kRecoveryHardTimeout.
    RECOVERY_TIMEOUT            = 200,
    /// Recovery completed but some WAL entries were partially written and discarded.
    RECOVERY_INCOMPLETE         = 201,
    /// Checkpoint position in WAL is invalid or points past the WAL tail.
    CHECKPOINT_FAILED           = 202,

    // ── Backup / PITR ─────────────────────────────────────────────────────────
    /// Requested PITR timestamp is in the future or outside the retention window.
    PITR_INVALID_TIMESTAMP      = 300,
    /// Backup snapshot is corrupted or incomplete.
    BACKUP_CORRUPTED            = 301,
    /// Maximum concurrent backup limit kMaxConcurrentBackups reached.
    BACKUP_LIMIT_EXCEEDED       = 302,
    /// Restore operation failed due to I/O or consistency error.
    RESTORE_FAILED              = 303,

    // ── Tiering / Compaction ──────────────────────────────────────────────────
    /// Tier-migration operation failed; value remains in source tier.
    TIERING_MIGRATION_FAILED    = 400,
    /// Compaction was aborted (e.g. due to I/O pressure or shutdown signal).
    COMPACTION_ABORTED          = 401,
    /// Storage space has been exhausted.
    STORAGE_EXHAUSTED           = 402,

    // ── Generic ───────────────────────────────────────────────────────────────
    /// Operation succeeded.
    OK                          = 0,
    /// Unclassified internal storage error; always fail-safe (no data loss).
    INTERNAL_ERROR              = 9999,
};

// ============================================================================
// § 7  Fail-Safe Classification Helpers
// ============================================================================

/**
 * @brief Returns true when @p code represents a durability-threatening failure.
 *
 * Durability-threatening codes mandate immediate operator alert and MUST NOT
 * be silently retried: WAL_WRITE_FAILED, WAL_CORRUPTED, STORAGE_EXHAUSTED.
 */
[[nodiscard]] inline constexpr bool isDurabilityThreat(StorageErrorCode code) noexcept {
    return code == StorageErrorCode::WAL_WRITE_FAILED
        || code == StorageErrorCode::WAL_CORRUPTED
        || code == StorageErrorCode::STORAGE_EXHAUSTED
        || code == StorageErrorCode::BACKUP_CORRUPTED;
}

/**
 * @brief Returns true when @p code is a transaction conflict that the caller
 *        SHOULD retry with exponential backoff.
 */
[[nodiscard]] inline constexpr bool isRetryableConflict(StorageErrorCode code) noexcept {
    return code == StorageErrorCode::TRANSACTION_CONFLICT;
}

// ============================================================================
// § 8  Contract Conformance Notes
//
// All storage module implementations MUST:
//   1. Persist to WAL before returning ACK on any committed write.
//   2. Ensure MVCC reads never expose uncommitted data from other transactions.
//   3. Emit RECOVERY_INCOMPLETE (not silently drop) for torn WAL entries.
//   4. Refuse PITR restore to future timestamps with PITR_INVALID_TIMESTAMP.
//   5. Keep tiering migrations transparent to concurrent readers.
// ============================================================================

}  // namespace storage
}  // namespace themis
