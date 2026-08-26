/**
 * @file backup_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=18; TODO=1, Stub=16, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <system_error>
#include <memory>
#include <vector>
#include <map>
#include <mutex>
#include <atomic>
#include <functional>
#include "utils/expected.h"

namespace themis {

// Forward declarations
class RocksDBWrapper;

/**
 * @enum RAIDMode
 * @brief Supported redundancy layouts for backup coordination.
 * @ingroup storage
 */
enum class RAIDMode {
    NONE,           // Single node, no redundancy
    RAID0,          // Striping only
    RAID1,          // Mirroring
    RAID5,          // Striping with distributed parity
    RAID6,          // Striping with double parity
    RAID10          // Striping + Mirroring
};

/**
 * @brief Describes a single shard participating in a coordinated backup.
 * @ingroup storage
 */
struct ShardInfo {
    std::string shard_id;
    std::string endpoint;
    bool is_parity_shard;    // For RAID5/6: indicates if this shard holds parity
    uint32_t shard_index;    // Position in the RAID group
};

/**
 * @brief RAID topology detected from the runtime environment.
 * @ingroup storage
 */
struct RAIDConfig {
    RAIDMode mode = RAIDMode::NONE;
    std::string raid_group;              // E.g., "raid5"
    std::vector<ShardInfo> shards;       // All shards in this RAID group
    uint32_t data_shards = 0;            // For RAID5/6: number of data shards
    uint32_t parity_shards = 0;          // For RAID5/6: number of parity shards
    bool is_coordinated = false;         // Whether this backup needs cross-shard coordination
};

/**
 * @enum CompressionType
 * @brief Compression modes supported by backup creation and restore flows.
 * @ingroup storage
 */
enum class CompressionType {
    NONE,           // No compression
    GZIP,           // gzip compression
    ZSTD,           // Zstandard compression (fast, high ratio)
    LZ4             // LZ4 compression (very fast, moderate ratio)
};

/**
 * @brief Integrity metadata recorded for a file inside a backup payload.
 * @ingroup storage
 *
 * Used when building or validating `INTEGRITY_MANIFEST.json` entries during
 * backup compression and post-decompression verification.
 */
struct FileIntegrityInfo {
    std::string relative_path;      ///< Path relative to backup root
    std::string checksum_sha256;    ///< SHA-256 checksum of original file
    uint64_t original_size = 0;     ///< Size before compression
    uint64_t compressed_size = 0;   ///< Size after compression (0 if uncompressed)
    CompressionType compression = CompressionType::NONE;
    bool verified = false;          ///< Set to true after successful verification
};

/**
 * @enum StorageBackend
 * @brief Transport backends available to backup upload and restore operations.
 * @ingroup storage
 */
enum class StorageBackend {
    LOCAL,          // Local filesystem
    S3,             // Amazon S3
    GCS,            // Google Cloud Storage
    AZURE           // Azure Blob Storage
};

/**
 * @brief Runtime options that control backup creation, transport, and retention.
 * @ingroup storage
 */
struct BackupOptions {
    CompressionType compression = CompressionType::NONE;
    bool encrypt = false;
    std::string encryption_key;          // AES-256 key (32 bytes hex)
    StorageBackend storage = StorageBackend::LOCAL;
    std::string storage_path;            // Local path or cloud URL
    bool verify_after_backup = true;
    uint32_t retention_days = 30;        // Backup retention policy
    std::map<std::string, std::string> cloud_config;  // Cloud-specific settings
};

/**
 * @brief Point-in-time recovery target selection parameters.
 * @ingroup storage
 */
struct PITROptions {
    std::chrono::system_clock::time_point target_time;
    std::string target_lsn;              // Log Sequence Number
    bool timeline_consistent = true;
};

/**
 * @brief Aggregated metrics produced by restore and PITR flows.
 * @ingroup storage
 */
struct RecoveryStats {
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point end_time;
    uint64_t bytes_restored = 0;
    uint64_t files_restored = 0;
    uint32_t rto_seconds = 0;            // Recovery Time Objective
    std::string recovery_point;          // RPO - Recovery Point Objective
};

/**
 * @class BackupManager
 * @brief Backup, restore, snapshot, and PITR orchestrator for the storage module.
 * @ingroup storage
 *
 * Features:
 * - RocksDB Checkpoint API integration for consistent snapshots
 * - Incremental backups with sequence number tracking
 * - WAL (Write-Ahead Log) archiving for point-in-time recovery
 * - Backup manifest files with metadata
 * - Restore with integrity verification
 * - RAID5/6 awareness: ensures parity shards are included in backups
 *
 * Backup Strategy:
 * 1. Full backup: RocksDB checkpoint + WAL files
 * 2. Incremental backup: WAL files since last backup
 * 3. Manifest files track backup chain and metadata
 * 4. RAID5/6: Coordinates backup across all data + parity shards
 *
 * RAID5/6 Backup Strategy:
 * - For RAID5/6 configurations, a complete backup ALWAYS includes:
 *   * All data shards (containing striped data)
 *   * All parity shards (containing parity information)
 * - Primary backup = Full backup of all shards (data + parity)
 * - Incremental backups = Incremental changes from all shards
 * - This ensures complete data recovery capability
 *
 * Directory Structure:
 * backup_dir/
 *   ├── full_YYYYMMDD_HHMMSS/
 *   │   ├── checkpoint/       (RocksDB checkpoint data)
 *   │   ├── wal/              (WAL files at checkpoint time)
 *   │   ├── raid_topology/    (RAID5/6 only: shard topology info)
 *   │   │   ├── shard_0/      (First shard backup)
 *   │   │   ├── shard_1/      (Second shard backup)
 *   │   │   └── shard_parity/ (Parity shard backup)
 *   │   └── MANIFEST.json     (backup metadata with RAID info)
 *   ├── incr_YYYYMMDD_HHMMSS/
 *   │   ├── wal/              (incremental WAL files)
 *   │   └── MANIFEST.json     (incremental metadata)
 *   └── latest -> full_YYYYMMDD_HHMMSS/  (symlink to latest backup)
 */
class BackupManager {
public:
    /**
     * @brief Runtime configuration for backup path and guard behavior.
     *
     * @note backup_base_dir, when provided, is the canonical root used by
     *       restore/checksum path-traversal guards. Empty means derive the base
     *       from the database root directory.
     */
    struct Config {
        std::string backup_base_dir;
    };

    /**
     * @brief Construct a backup manager around an open RocksDB wrapper.
     *
     * The wrapper is used for checkpoint creation, sequence-number discovery,
     * restore operations, and snapshot orchestration.
     *
     * @param db_wrapper Shared pointer to the storage wrapper used for
     *        checkpoint and restore operations.
     * @param config Backup manager runtime configuration. Set
     *        `config.backup_base_dir` to constrain restore/checksum guards to a
     *        dedicated backup root.
     */
    explicit BackupManager(std::shared_ptr<RocksDBWrapper> db_wrapper,
                           Config config = {});

    /**
     * @brief Destroy the backup manager.
     *
     * Destruction only releases in-memory state such as the schedule registry.
     * It does not delete backups or mutate persisted storage state.
     */
    ~BackupManager();

    /**
     * @brief Create a full backup using a RocksDB checkpoint plus WAL capture.
     *
     * For RAID5/6 deployments, a successful full backup includes every data and
     * parity shard required for later recovery.
     *
     * @param dest_dir Base backup directory. The method creates a timestamped
     *        subdirectory beneath this path.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @param options Backup options for compression, encryption, verification,
     *        retention, and transport handling.
     * @return `true` on success, or `false` when checkpoint creation, file copy,
     *         manifest generation, or verification fails.
     */
    bool createFullBackup(const std::string& dest_dir, std::error_code& ec, 
                          const BackupOptions& options = BackupOptions());
    
    /**
     * @brief Create a full backup and return the created backup directory path.
     *
     * @param dest_dir Base backup directory.
     * @return `Result<std::string>` containing the created backup directory on
     *         success, or an error when the backup cannot be created.
     */
    Result<std::string> createFullBackup(const std::string& dest_dir);

    /**
     * @brief Create an incremental backup from WAL changes since the last backup.
     *
     * For RAID5/6 deployments, incremental state is collected across all shards
     * participating in the backup set.
     *
     * @param dest_dir Base backup directory.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @param options Backup options for compression, encryption, verification,
     *        retention, and transport handling.
     * @return `true` on success, or `false` if the incremental window cannot be
     *         computed or the backup payload cannot be written.
     */
    bool createIncrementalBackup(const std::string& dest_dir, std::error_code& ec,
                                  const BackupOptions& options = BackupOptions());
    
    /**
     * @brief Create an incremental backup and return the created backup path.
     *
     * @param dest_dir Base backup directory.
     * @return `Result<std::string>` containing the created backup directory on
     *         success, or an error when the backup cannot be created.
     */
    Result<std::string> createIncrementalBackup(const std::string& dest_dir);
    
    /**
     * @brief Create a differential backup containing changes since the last full backup.
     *
     * @param dest_dir Base backup directory.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @param options Backup options that shape backup generation.
     * @return `true` on success, or `false` if no valid base full backup exists
     *         or the differential payload cannot be assembled.
     */
    bool createDifferentialBackup(const std::string& dest_dir, std::error_code& ec,
                                   const BackupOptions& options = BackupOptions());
    
    /**
     * @brief Create a differential backup and return the resulting directory path.
     *
     * @param dest_dir Base backup directory.
     * @return `Result<std::string>` containing the created backup directory on
     *         success, or an error when the differential backup cannot be created.
     */
    Result<std::string> createDifferentialBackup(const std::string& dest_dir);

    /**
     * @brief Archive WAL files into a destination directory.
     *
     * @param dest_dir Destination directory for archived WAL files.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` if WAL discovery or copy fails.
     */
    bool archiveWAL(const std::string& dest_dir, std::error_code& ec);

    /**
     * @brief Archive WAL files into a destination directory.
     *
     * @param dest_dir Destination directory for archived WAL files.
     * @return `Result<void>` on success, or an error when WAL discovery or copy
     *         fails.
     */
    Result<void> archiveWAL(const std::string& dest_dir);

    /**
     * @brief Restore the database from a full or incremental backup chain.
     *
     * For RAID5/6 deployments, restore requires all data and parity shards that
     * belong to the backup set.
     *
     * @param src_dir Source backup directory.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @param stats Optional pointer that receives recovery metrics on success.
     * @return `true` on success, or `false` when manifest validation, payload
     *         extraction, or RocksDB restore fails.
     */
    bool restoreFromBackup(const std::string& src_dir, std::error_code& ec,
                           RecoveryStats* stats = nullptr);
    
    /**
     * @brief Perform point-in-time recovery from a backup chain and WAL replay target.
     *
     * @param dest_dir Base backup directory containing the recovery chain.
     * @param pitr_options PITR target timestamp and optional LSN metadata.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @param stats Optional pointer that receives recovery metrics on success.
     * @return `true` on success, or `false` when the base restore fails, the
     *         target cannot be satisfied, or WAL replay reports failure.
     */
    bool performPITR(const std::string& dest_dir, const PITROptions& pitr_options,
                     std::error_code& ec, RecoveryStats* stats = nullptr);
    
    /**
     * @brief Restore a selected subset of collections from a checkpoint backup.
     *
     * @param src_dir Source backup directory.
     * @param collections Collection or column-family names to restore.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when selective ingest is
     *         unavailable or the restore pipeline fails.
     */
    bool restoreCollections(const std::string& src_dir, 
                           const std::vector<std::string>& collections,
                           std::error_code& ec);
    
    /**
     * @brief Restore the database from a backup using the `Result` API.
     *
     * @param src_dir Source backup directory.
     * @return `Result<void>` on success, or an error when the restore fails.
     */
    Result<void> restoreFromBackup(const std::string& src_dir);

    /**
     * @brief List available backups beneath a base backup directory.
     *
     * @param backup_dir Base backup directory.
     * @return Backup directory names sorted by timestamp. Returns an empty vector
     *         when the directory has no recognized backups.
     */
    std::vector<std::string> listBackups(const std::string& backup_dir);

    /**
     * @brief Verify a backup payload, manifest, and shard completeness.
     *
     * For RAID5/6 deployments, verification ensures that all required data and
     * parity shards are present and internally consistent.
     *
     * @param backup_dir Backup directory to verify.
     * @return `Result<void>` on success, or an error when checksums, manifests,
     *         or shard requirements fail validation.
     */
    Result<void> verifyBackup(const std::string& backup_dir);
    
    /**
     * @brief Compress a backup directory into an archive file.
     *
     * @param backup_dir Backup directory to compress.
     * @return `Result<std::string>` containing the compressed archive path on
     *         success, or an error when the archive cannot be created.
     */
    Result<std::string> compressBackup(const std::string& backup_dir);
    
    /**
     * @brief Decompress a backup archive into a destination directory.
     *
     * @param compressed_file Compressed backup archive.
     * @param dest_dir Destination directory for extracted files.
     * @return `Result<std::string>` containing the decompressed directory path on
     *         success, or an error when extraction or post-extraction
     *         verification fails.
     * 
     * @note Phase 1 Enhancement: This method now performs integrity verification after decompression
     *       to prevent silent data corruption. All decompressed files are verified against stored
     *       checksums to ensure data integrity.
     */
    Result<std::string> decompressBackup(const std::string& compressed_file, const std::string& dest_dir);
    
    /**
     * @brief Verify the payload extracted from a compressed backup.
     *
     * @param backup_dir Decompressed backup directory to verify.
     * @return `Result<void>` on success, including the backward-compatible case
     *         where the backup predates `INTEGRITY_MANIFEST.json`; returns an
     *         error when manifest loading fails or file checksums do not match.
     * 
     * @note Performs post-decompression verification:
     *       - Missing `INTEGRITY_MANIFEST.json` is treated as a legacy backup and skips verification
     *       - Reads integrity manifest from backup
     *       - Calculates SHA-256 checksum of each file
     *       - Compares against stored checksums
     *       - Reports corrupted files with details
     */
    Result<void> verifyDecompressedBackup(const std::string& backup_dir);
    
    /**
     * @brief Attempt to repair files that fail post-decompression verification.
     *
     * @param backup_dir Backup directory to repair.
     * @param compressed_source Optional original compressed archive used to retry
     *        recovery of corrupted files.
     * @return `Result<uint32_t>` containing the number of repaired files on
     *         success, or an error when corruption cannot be isolated or the
     *         repair path fails.
     * 
     * @note This is an advanced recovery method:
     *       - Identifies corrupted files during verification
     *       - Attempts to repair from original compressed source if provided
     *       - If source unavailable, logs a warning and returns a repair error; it does not quarantine files
     */
    Result<uint32_t> repairDecompressedBackup(const std::string& backup_dir,
                                              const std::string& compressed_source = "");
    
    /**
     * @brief Detect RAID topology from environment variables.
     *
     * Reads `THEMIS_RAID_GROUP`, `THEMIS_SHARD_ID`, and `THEMIS_SHARDS`.
     *
     * @return Parsed RAID configuration. Returns `RAIDMode::NONE` when the
     *         environment does not describe a coordinated backup topology.
     */
    static RAIDConfig detectRAIDConfiguration();
    
    /**
     * @brief Check whether a backup contains the shards required by a RAID topology.
     *
     * @param backup_dir Backup directory to inspect.
     * @param raid_config RAID topology expected for the backup set.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` when every required shard is present, or `false` when the
     *         backup is incomplete or the directory cannot be inspected.
     */
    bool isBackupComplete(const std::string& backup_dir, 
                         const RAIDConfig& raid_config, 
                         std::error_code& ec);
    
    /**
     * @brief Delete backups older than the retention window.
     *
     * @param backup_dir Base backup directory.
     * @param retention_days Number of days to retain.
     * @param ec Receives the failure reason when directory traversal or deletion
     *        fails. Partial deletion may already have occurred.
     * @return Number of backups deleted.
     */
    uint32_t applyRetentionPolicy(const std::string& backup_dir, 
                                   uint32_t retention_days,
                                   std::error_code& ec);
    
    /**
     * @brief Collect size and count metrics for a backup directory.
     *
     * @param backup_dir Backup directory to analyze.
     * @return Map of metric name to metric value. Returns an empty map when the
     *         directory cannot be analyzed.
     */
    std::map<std::string, uint64_t> getBackupMetrics(const std::string& backup_dir);
    
    /**
     * @brief Estimate the recovery time objective for a backup payload.
     *
     * @param backup_dir Backup directory to analyze.
     * @return Estimated RTO in seconds.
     */
    uint32_t estimateRTO(const std::string& backup_dir);
    
    /**
     * @brief Determine the most recent recoverable point for a backup set.
     *
     * @param backup_dir Base backup directory.
     * @return Timestamp representing the latest recoverable point. Returns the
     *         epoch when no recoverable backup can be determined.
     */
    std::chrono::system_clock::time_point getRPO(const std::string& backup_dir);

    /**
     * @brief Check whether a backup contains the shards required by a RAID topology.
     *
     * @param backup_dir Backup directory to inspect.
     * @param raid_config RAID topology expected for the backup set.
     * @return `Result<void>` on success, or an error when the backup is
     *         incomplete or cannot be inspected.
     */
    Result<void> isBackupComplete(const std::string& backup_dir, 
                                   const RAIDConfig& raid_config);

    // ============================================================================
    // GAP-008: Cloud Backup & Snapshot Scheduling
    // ============================================================================
    
    /**
     * @brief Register an automatic backup schedule in the in-process schedule registry.
     *
     * The current implementation validates the cron-like expression, allocates a
     * stable schedule identifier, and stores the schedule metadata in memory for
     * later inspection or cancellation. It does not spawn a background executor or
     * persist schedules across process restarts.
     *
     * @param schedule_cron Cron-style expression with five space-separated fields
     *        (for example, `0 2 * * *`).
     * @param backup_type Backup class to register (for example `full`,
     *        `incremental`, or `differential`).
     * @param options Backup options captured with the schedule entry.
     * @return Result<std::string> containing the generated schedule identifier on
     *         success, or an Error when the inputs are invalid.
     */
    Result<std::string> scheduleBackup(const std::string& schedule_cron,
                                       const std::string& backup_type,
                                       const BackupOptions& options);
    
    /**
     * @brief Cancel a previously registered in-memory backup schedule.
     *
     * @param schedule_id Schedule identifier returned by scheduleBackup().
     * @return Result<void> on success, or an Error if the identifier is empty or
     *         no matching schedule exists.
     */
    Result<void> cancelScheduledBackup(const std::string& schedule_id);
    
    /**
     * @brief List all schedules currently stored in the in-memory registry.
     *
     * @return Vector of `(schedule_id, cron_expression)` pairs. Returns an empty
     *         vector when no schedules are registered.
     */
    std::vector<std::pair<std::string, std::string>> listScheduledBackups();
    
    /**
     * @brief Copy a finished backup to a provider-specific destination.
     *
     * Supported destinations:
     * - `StorageBackend::LOCAL`: local filesystem mirror via `file:///absolute/path`
     *   or an absolute path.
     * - `StorageBackend::S3`: `s3://bucket/path` (requires provider integration).
     * - `StorageBackend::AZURE`: `azure://account/container/path` (requires provider integration).
     * - `StorageBackend::GCS`: `gs://bucket/path` (requires provider integration).
     *
     * @param local_backup_path Existing local backup directory or archive.
     * @param cloud_uri Provider URI or local mirror path, depending on @p options.storage.
     * @param options Transfer options, backend selection, and provider-specific configuration.
     * @return Result<std::string> containing the destination URI/path on success,
     *         or an Error when validation fails or the provider backend is unavailable.
     */
    Result<std::string> uploadBackupToCloud(const std::string& local_backup_path,
                                            const std::string& cloud_uri,
                                            const BackupOptions& options);
    
    /**
     * @brief Restore a backup payload from a provider-specific source into a local directory.
     *
     * For `StorageBackend::LOCAL`, the method copies the source tree from a
     * `file:///absolute/path` URI or absolute path into @p local_restore_path.
     * Remote backends validate the URI format and then delegate to the matching
     * provider integration when available.
     *
     * @param cloud_uri Provider URI or local mirror path, depending on @p options.storage.
     * @param local_restore_path Local destination directory for the restored payload.
     * @param options Restore options, backend selection, and provider-specific configuration.
     * @return Result<void> on success, or an Error when validation fails, the source
     *         cannot be copied, or the provider backend is unavailable.
     */
    Result<void> restoreFromCloud(const std::string& cloud_uri,
                                  const std::string& local_restore_path,
                                  const BackupOptions& options);
    
    /**
     * @brief Create a consistent online snapshot of the database.
     *
     * Uses RocksDB's Checkpoint API to produce a crash-consistent,
     * hard-linked copy of the current SST files.  No writes are blocked
     * during the operation (quiesce-safe).
     *
     * A JSON manifest is written next to the snapshot directory so the
     * snapshot can be identified, verified, and restored later.
     *
     * @param snapshot_name  Human-readable name; used as the directory name
     *                       inside the default snapshot base path
     *                       ("<db_path>/../snapshots/<snapshot_name>_<ts>").
     * @param storage_class  Reserved for future cloud / K8s integration;
     *                       ignored for local snapshots.
     * @return On success: the absolute path to the snapshot directory.
     *         On failure: an Error describing what went wrong.
     */
    Result<std::string> createSnapshot(const std::string& snapshot_name,
                                       const std::string& storage_class = "default");

    /**
     * @brief Restore the database from a previously created snapshot.
     *
     * The running database is closed, its data directory is replaced with
     * the contents of the snapshot directory, and the database is reopened.
     *
     * @param snapshot_id   Absolute path to the snapshot directory (as
     *                      returned by createSnapshot()).
     * @param restore_pvc   Reserved for future K8s PVC integration;
     *                      pass an empty string for local restores.
     * @return Result<void> on success, Error on failure.
     */
    Result<void> restoreFromSnapshot(const std::string& snapshot_id,
                                     const std::string& restore_pvc = "");

    /**
     * @brief Verify a snapshot by checking its manifest and the presence
     *        of the expected SST / MANIFEST files.
     *
     * @param snapshot_dir  Absolute path to the snapshot directory.
     * @return Result<void> on success, Error if the snapshot is corrupted
     *         or incomplete.
     */
    Result<void> verifySnapshot(const std::string& snapshot_dir);

    /**
     * @brief List all snapshots under the default snapshot base path.
     *
     * @return Sorted list of snapshot directory paths (oldest first).
     */
    Result<std::vector<std::string>> listSnapshots();

    // ── PITR WAL replay injection (Stub #249) ────────────────────────────────

    /**
     * @brief Callable type for WAL-segment replay during PITR.
     *
     * Arguments:
     *   - dest_dir:      Directory where the base snapshot has been restored.
     *   - target_time:   The point-in-time to replay up to.
     *   - ec:            Set on failure.
     *
     * Returns @c true on success.  The production implementation opens WAL
     * segment files in @p dest_dir, applies records whose sequence number
     * corresponds to timestamps ≤ @p target_time, then closes the reader.
     *
     * Stub #249 injection API — resolves the missing WAL replay step in
     * `performPITR()`.
     */
    using WalReplayFn = std::function<bool(
        const std::string& dest_dir,
        std::chrono::system_clock::time_point target_time,
        std::error_code& ec)>;

    /**
     * @brief Inject a WAL-replay function used by `performPITR()` after the
     *        base snapshot has been restored.
     *
     * Without an injected function `performPITR()` silently omits the WAL
     * replay step (original behaviour).  When set, the function is called
     * with the restore directory and target time so the caller can apply the
     * remaining WAL delta.
     *
     * @param fn  Must not throw; returning @c false is treated as a replay
     *            failure and `performPITR()` will return @c false too.
     */
    void setWalReplayFn(WalReplayFn fn);

    // ── Per-CF SST ingest injection (Stub #300) ──────────────────────────────

    /**
     * @brief Callable type for per-column-family selective SST ingest.
     *
     * Arguments:
     *   - checkpoint_dir:  Path to the RocksDB checkpoint directory.
     *   - collections:     Names of the collections (column families) to restore.
     *   - ec:              Set on failure.
     *
     * Returns @c true on success.
     */
    using CfSstIngestFn = std::function<bool(
        const std::string& checkpoint_dir,
        const std::vector<std::string>& collections,
        std::error_code& ec)>;

    /**
     * @brief Inject a per-CF SST ingest function used by restoreCollections().
     *
     * When set, restoreCollections() calls this function before falling back
     * to full checkpoint restore.
     */
    void setCfSstIngestFn(CfSstIngestFn fn);

private:
    // -------------------------------------------------------------------------
    // Scheduling: in-memory registry for backup schedules.
    // -------------------------------------------------------------------------

    /**
     * @brief Internal representation of a registered backup schedule.
     */
    struct ScheduledBackupEntry {
        std::string schedule_id;
        std::string cron_expression;
        std::string backup_type;
        BackupOptions options;
        std::string created_at;
    };

    std::map<std::string, ScheduledBackupEntry> scheduled_backups_;
    mutable std::mutex scheduler_mutex_;
    std::atomic<uint64_t> schedule_counter_{0};

    std::shared_ptr<RocksDBWrapper> db_wrapper_;
    Config config_{};
    RAIDConfig raid_config_;
    
    /**
     * @brief Return the current wall-clock timestamp formatted as `YYYYMMDD_HHMMSS`.
     */
    std::string getTimestamp() const;
    
    /**
     * @brief Create the `MANIFEST.json` file for a backup payload.
     *
     * @param backup_dir Backup directory that receives the manifest.
     * @param type Logical backup type (`full`, `incremental`, `differential`).
     * @param sequence_number RocksDB sequence number captured for the backup.
     * @return `Result<void>` on success, or an error when the manifest cannot be written.
     */
    Result<void> createManifest(const std::string& backup_dir, const std::string& type,
                                uint64_t sequence_number);
    
    /**
     * @brief Read backup type and sequence metadata from `MANIFEST.json`.
     *
     * @param backup_dir Backup directory containing the manifest.
     * @param type Receives the parsed backup type on success.
     * @param sequence_number Receives the parsed sequence number on success.
     * @return `Result<void>` on success, or an error when the manifest is missing
     *         or malformed.
     */
    Result<void> readManifest(const std::string& backup_dir, std::string& type,
                              uint64_t& sequence_number);
    
    /**
     * @brief Copy WAL files whose sequence number is at least @p min_sequence.
     *
     * @param src_dir Source WAL directory.
     * @param dest_dir Destination WAL directory.
     * @param min_sequence Minimum sequence number to retain.
     * @return `Result<void>` on success, or an error when traversal or copy fails.
     */
    Result<void> copyWALFiles(const std::string& src_dir, const std::string& dest_dir,
                              uint64_t min_sequence);
    
    /**
     * @brief Calculate the SHA-256 checksum for a file.
     *
     * @param file_path File to hash.
     * @return `Result<std::string>` containing the lowercase hexadecimal digest
     *         on success, or an error when the file cannot be read.
     */
    Result<std::string> calculateChecksum(const std::string& file_path);
    
    /**
     * @brief Verify that a file matches an expected SHA-256 checksum.
     *
     * @param file_path File to verify.
     * @param expected_checksum Expected lowercase hexadecimal SHA-256 digest.
     * @return `Result<void>` on success, or an error when the checksum does not
     *         match or the file cannot be read.
     */
    Result<void> verifyChecksum(const std::string& file_path, const std::string& expected_checksum);
    
    /**
     * @brief Query the latest RocksDB sequence number.
     *
     * @return Latest sequence number, or zero when the wrapper is unavailable.
     */
    uint64_t getCurrentSequenceNumber() const;
    
    /**
     * @brief Parse a textual RAID mode identifier.
     *
     * @param mode_str Mode string such as `raid5`.
     * @return Parsed RAID mode, or `RAIDMode::NONE` for unrecognized inputs.
     */
    static RAIDMode parseRAIDMode(const std::string& mode_str);
    
    /**
     * @brief Convert a RAID mode enum to its manifest/environment string form.
     *
     * @param mode RAID mode to format.
     * @return Lowercase RAID mode name.
     */
    static std::string raidModeToString(RAIDMode mode);
    
    /**
     * @brief Verify that the backup contains every shard required by the RAID topology.
     *
     * @param backup_dir Backup directory to inspect.
     * @param raid_config Expected RAID topology.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when required shards are missing or
     *         the directory cannot be traversed.
     */
    bool verifyRAIDShardsInBackup(const std::string& backup_dir, 
                                  const RAIDConfig& raid_config,
                                  std::error_code& ec);

    /**
     * @brief Verify that the backup contains every shard required by the RAID topology.
     *
     * @param backup_dir Backup directory to inspect.
     * @param raid_config Expected RAID topology.
     * @return `Result<void>` on success, or an error when required shards are
     *         missing or the directory cannot be traversed.
     */
    Result<void> verifyRAIDShardsInBackup(const std::string& backup_dir, 
                                          const RAIDConfig& raid_config);
    
    /**
     * @brief Compress or copy a file tree into a destination path.
     *
     * The exact behavior depends on the selected compression type and which
     * compression libraries were linked into the build.
     *
     * @param src_path Source file or directory.
     * @param dest_path Destination path for compressed output.
     * @param type Compression mode to apply.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when traversal, compression, or copy fails.
     */
    bool compressPath([[maybe_unused]] const std::string& src_path,
                      [[maybe_unused]] const std::string& dest_path,
                      CompressionType type, std::error_code& ec);
    
    /**
     * @brief Decompress or copy a backup payload into a destination path.
     *
     * @param src_path Source file or directory.
     * @param dest_path Destination directory for decompressed output.
     * @param type Compression mode expected in the source payload.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when traversal, decompression, or
     *         copy fails.
     */
    bool decompressPath([[maybe_unused]] const std::string& src_path,
                        [[maybe_unused]] const std::string& dest_path,
                        CompressionType type, std::error_code& ec);
    
    /**
     * @brief Encrypt a file or directory payload into a destination path.
     *
     * @param src_path Source file or directory.
     * @param dest_path Destination path for encrypted output.
     * @param key Caller-supplied encryption key material.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when encryption or copy fails.
     */
    bool encryptFile([[maybe_unused]] const std::string& src_path,
                     [[maybe_unused]] const std::string& dest_path,
                     [[maybe_unused]] const std::string& key, std::error_code& ec);
    
    /**
     * @brief Decrypt a file or directory payload into a destination path.
     *
     * @param src_path Source file or directory.
     * @param dest_path Destination path for decrypted output.
     * @param key Caller-supplied encryption key material.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when decryption or copy fails.
     */
    bool decryptFile([[maybe_unused]] const std::string& src_path,
                     [[maybe_unused]] const std::string& dest_path,
                     [[maybe_unused]] const std::string& key, std::error_code& ec);
    
    /**
     * @brief Upload or mirror a local backup payload to the selected backend.
     *
     * @param local_path Existing local backup directory or archive.
     * @param cloud_path Provider URI or local mirror path.
     * @param backend Transport backend to use.
     * @param config Provider-specific configuration values.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when validation or transfer fails.
     */
    bool uploadToCloud(const std::string& local_path, const std::string& cloud_path,
                       StorageBackend backend, 
                       const std::map<std::string, std::string>& config,
                       std::error_code& ec);
    
    /**
     * @brief Download or mirror a backup payload from the selected backend.
     *
     * @param cloud_path Provider URI or local mirror path.
     * @param local_path Local destination directory or archive path.
     * @param backend Transport backend to use.
     * @param config Provider-specific configuration values.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when validation or transfer fails.
     */
    bool downloadFromCloud(const std::string& cloud_path, const std::string& local_path,
                           StorageBackend backend,
                           const std::map<std::string, std::string>& config,
                           std::error_code& ec);
    
    /**
     * @brief Find the most recent full backup beneath a base directory.
     *
     * @param backup_dir Base backup directory.
     * @return Path to the latest full backup, or an empty string when no full
     *         backup can be found.
     */
    std::string findLastFullBackup(const std::string& backup_dir);
    
    // ========================================================================
    // Phase 1: Decompression Integrity Verification Helpers
    // ========================================================================
    
    /**
     * @brief Build integrity metadata for a backup payload before compression completes.
     *
     * @param backup_dir Backup directory whose files should be fingerprinted.
     * @param integrity_map Receives one entry per tracked file on success.
     * @return `Result<void>` on success, or an error when checksum generation fails.
     */
    Result<void> buildIntegrityManifest(const std::string& backup_dir,
                                        std::vector<FileIntegrityInfo>& integrity_map);
    
    /**
     * @brief Write `INTEGRITY_MANIFEST.json` for a backup payload.
     *
     * @param backup_dir Backup directory that receives the manifest.
     * @param integrity_map Integrity entries to serialize.
     * @return `Result<void>` on success, or an error when the manifest cannot be written.
     */
    Result<void> writeIntegrityManifest(const std::string& backup_dir,
                                        const std::vector<FileIntegrityInfo>& integrity_map);
    
    /**
     * @brief Load `INTEGRITY_MANIFEST.json` from a backup directory.
     *
     * @param backup_dir Backup directory containing the manifest.
     * @return `Result<std::vector<FileIntegrityInfo>>` containing parsed entries
     *         on success, or an error when the manifest is missing or malformed.
     */
    Result<std::vector<FileIntegrityInfo>> readIntegrityManifest(const std::string& backup_dir);
    
    /**
     * @brief Compare a single file with an expected checksum value.
     *
     * @param file_path File to verify.
     * @param expected_checksum Expected lowercase hexadecimal SHA-256 digest.
     * @return `Result<bool>` containing `true` for a match and `false` for a
     *         mismatch; returns an error when the file cannot be read.
     */
    Result<bool> verifyFileChecksum(const std::string& file_path,
                                    const std::string& expected_checksum);
    
    /**
     * @brief Verify every tracked file in a decompressed backup payload.
     *
     * @param backup_dir Decompressed backup directory to verify.
     * @param integrity_map Expected integrity entries for the payload.
     * @return `Result<std::vector<std::string>>` containing the relative paths of
     *         corrupted or missing files, or an error when verification cannot run.
     */
    Result<std::vector<std::string>> verifyAllChecksums(const std::string& backup_dir,
                                                         const std::vector<FileIntegrityInfo>& integrity_map);
    
    /**
     * @brief Decompress a payload and run the built-in integrity verification path.
     *
     * @param src_path Source file or directory.
     * @param dest_path Destination directory for decompressed output.
     * @param type Compression mode expected in the source payload.
     * @param ec Receives the failure reason when the operation returns `false`.
     * @return `true` on success, or `false` when extraction or integrity
     *         verification fails.
     */
    bool decompressPathWithIntegrity(const std::string& src_path,
                                     const std::string& dest_path,
                                     CompressionType type,
                                     std::error_code& ec);

    WalReplayFn wal_replay_fn_;      ///< Optional WAL-replay hook (Stub #249)
    CfSstIngestFn cf_sst_ingest_fn_; ///< Optional per-CF SST ingest hook (Stub #300)
};

} // namespace themis
