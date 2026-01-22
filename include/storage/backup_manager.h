#pragma once

#include <string>
#include <system_error>
#include <memory>
#include <vector>
#include <map>

namespace themis {

// Forward declarations
class RocksDBWrapper;

/**
 * RAID Mode Types
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
 * Shard Information for RAID configurations
 */
struct ShardInfo {
    std::string shard_id;
    std::string endpoint;
    bool is_parity_shard;    // For RAID5/6: indicates if this shard holds parity
    uint32_t shard_index;    // Position in the RAID group
};

/**
 * RAID Configuration detected from environment
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
 * Backup Compression Type
 */
enum class CompressionType {
    NONE,           // No compression
    GZIP,           // gzip compression
    ZSTD,           // Zstandard compression (fast, high ratio)
    LZ4             // LZ4 compression (very fast, moderate ratio)
};

/**
 * Backup Storage Backend
 */
enum class StorageBackend {
    LOCAL,          // Local filesystem
    S3,             // Amazon S3
    GCS,            // Google Cloud Storage
    AZURE           // Azure Blob Storage
};

/**
 * Backup Options
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
 * Point-in-Time Recovery Options
 */
struct PITROptions {
    std::chrono::system_clock::time_point target_time;
    std::string target_lsn;              // Log Sequence Number
    bool timeline_consistent = true;
};

/**
 * Recovery Statistics
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
 * Production-ready BackupManager for incremental backups and WAL archiving
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
     * @param db_wrapper: Shared pointer to RocksDBWrapper for checkpoint operations
     */
    explicit BackupManager(std::shared_ptr<RocksDBWrapper> db_wrapper);
    ~BackupManager();

    /**
     * Create a full backup using RocksDB checkpoint
     * For RAID5/6: Backs up ALL shards (data + parity) to ensure complete recovery
     * @param dest_dir: Base backup directory (will create timestamped subdirectory)
     * @param ec: Error code on failure
     * @param options: Backup options (compression, encryption, storage)
     * @return true on success, false otherwise
     */
    bool createFullBackup(const std::string& dest_dir, std::error_code& ec, 
                          const BackupOptions& options = BackupOptions());

    /**
     * Create an incremental backup (WAL files since last backup)
     * For RAID5/6: Backs up incremental changes from ALL shards
     * @param dest_dir: Base backup directory
     * @param ec: Error code on failure
     * @param options: Backup options (compression, encryption, storage)
     * @return true on success, false otherwise
     */
    bool createIncrementalBackup(const std::string& dest_dir, std::error_code& ec,
                                  const BackupOptions& options = BackupOptions());
    
    /**
     * Create a differential backup (changes since last full backup)
     * @param dest_dir: Base backup directory
     * @param ec: Error code on failure
     * @param options: Backup options
     * @return true on success, false otherwise
     */
    bool createDifferentialBackup(const std::string& dest_dir, std::error_code& ec,
                                   const BackupOptions& options = BackupOptions());

    /**
     * Archive WAL files to destination directory
     * @param dest_dir: Destination for WAL files
     * @param ec: Error code on failure
     * @return true on success, false otherwise
     */
    bool archiveWAL(const std::string& dest_dir, std::error_code& ec);

    /**
     * Restore database from backup directory
     * For RAID5/6: Restores from all shards (data + parity) to reconstruct complete data
     * @param src_dir: Source backup directory (full or incremental chain)
     * @param ec: Error code on failure
     * @param stats: Optional recovery statistics output
     * @return true on success, false otherwise
     */
    bool restoreFromBackup(const std::string& src_dir, std::error_code& ec,
                           RecoveryStats* stats = nullptr);
    
    /**
     * Perform point-in-time recovery (PITR)
     * @param dest_dir: Base backup directory containing backup chain
     * @param pitr_options: PITR target time/LSN
     * @param ec: Error code on failure
     * @param stats: Optional recovery statistics output
     * @return true on success, false otherwise
     */
    bool performPITR(const std::string& dest_dir, const PITROptions& pitr_options,
                     std::error_code& ec, RecoveryStats* stats = nullptr);
    
    /**
     * Restore specific collections (partial recovery)
     * @param src_dir: Source backup directory
     * @param collections: List of collection names to restore
     * @param ec: Error code on failure
     * @return true on success, false otherwise
     */
    bool restoreCollections(const std::string& src_dir, 
                           const std::vector<std::string>& collections,
                           std::error_code& ec);

    /**
     * List available backups in directory
     * @param backup_dir: Base backup directory
     * @return Vector of backup directory names sorted by timestamp
     */
    std::vector<std::string> listBackups(const std::string& backup_dir);

    /**
     * Verify backup integrity
     * For RAID5/6: Verifies ALL shards are present and consistent
     * @param backup_dir: Backup directory to verify
     * @param ec: Error code on failure
     * @return true if backup is valid, false otherwise
     */
    bool verifyBackup(const std::string& backup_dir, std::error_code& ec);
    
    /**
     * Detect RAID configuration from environment variables
     * Reads: THEMIS_RAID_GROUP, THEMIS_SHARD_ID, THEMIS_SHARDS
     * @return RAIDConfig structure with detected configuration
     */
    static RAIDConfig detectRAIDConfiguration();
    
    /**
     * Check if backups are complete for RAID5/6 configurations
     * Verifies that all required shards (data + parity) are backed up
     * @param backup_dir: Backup directory to check
     * @param raid_config: RAID configuration
     * @param ec: Error code on failure
     * @return true if backup is complete, false otherwise
     */
    bool isBackupComplete(const std::string& backup_dir, 
                         const RAIDConfig& raid_config, 
                         std::error_code& ec);
    
    /**
     * Apply backup retention policy (delete old backups)
     * @param backup_dir: Base backup directory
     * @param retention_days: Keep backups younger than this many days
     * @param ec: Error code on failure
     * @return Number of backups deleted
     */
    uint32_t applyRetentionPolicy(const std::string& backup_dir, 
                                   uint32_t retention_days,
                                   std::error_code& ec);
    
    /**
     * Get backup metrics
     * @param backup_dir: Backup directory to analyze
     * @return Map of metric name to value
     */
    std::map<std::string, uint64_t> getBackupMetrics(const std::string& backup_dir);
    
    /**
     * Calculate Recovery Time Objective (RTO) estimate
     * @param backup_dir: Backup to analyze
     * @return Estimated RTO in seconds
     */
    uint32_t estimateRTO(const std::string& backup_dir);
    
    /**
     * Get Recovery Point Objective (RPO) information
     * @param backup_dir: Base backup directory
     * @return Timestamp of most recent recoverable point
     */
    std::chrono::system_clock::time_point getRPO(const std::string& backup_dir);

private:
    std::shared_ptr<RocksDBWrapper> db_wrapper_;
    RAIDConfig raid_config_;
    
    // Helper: Get current timestamp string (YYYYMMDD_HHMMSS)
    std::string getTimestamp() const;
    
    // Helper: Create backup manifest file (with RAID info)
    bool createManifest(const std::string& backup_dir, const std::string& type,
                        uint64_t sequence_number, std::error_code& ec);
    
    // Helper: Read backup manifest (including RAID info)
    bool readManifest(const std::string& backup_dir, std::string& type,
                      uint64_t& sequence_number, std::error_code& ec);
    
    // Helper: Copy WAL files with sequence number filtering
    bool copyWALFiles(const std::string& src_dir, const std::string& dest_dir,
                      uint64_t min_sequence, std::error_code& ec);
    
    // Helper: Get current WAL sequence number from RocksDB
    uint64_t getCurrentSequenceNumber() const;
    
    // Helper: Parse RAID mode from string
    static RAIDMode parseRAIDMode(const std::string& mode_str);
    
    // Helper: Convert RAID mode to string
    static std::string raidModeToString(RAIDMode mode);
    
    // Helper: Verify all RAID5/6 shards are present in backup
    bool verifyRAIDShardsInBackup(const std::string& backup_dir, 
                                  const RAIDConfig& raid_config,
                                  std::error_code& ec);
    
    // Helper: Compress file/directory
    bool compressPath(const std::string& src_path, const std::string& dest_path,
                      CompressionType type, std::error_code& ec);
    
    // Helper: Decompress file
    bool decompressPath(const std::string& src_path, const std::string& dest_path,
                        CompressionType type, std::error_code& ec);
    
    // Helper: Encrypt file
    bool encryptFile(const std::string& src_path, const std::string& dest_path,
                     const std::string& key, std::error_code& ec);
    
    // Helper: Decrypt file
    bool decryptFile(const std::string& src_path, const std::string& dest_path,
                     const std::string& key, std::error_code& ec);
    
    // Helper: Upload to cloud storage
    bool uploadToCloud(const std::string& local_path, const std::string& cloud_path,
                       StorageBackend backend, 
                       const std::map<std::string, std::string>& config,
                       std::error_code& ec);
    
    // Helper: Download from cloud storage
    bool downloadFromCloud(const std::string& cloud_path, const std::string& local_path,
                           StorageBackend backend,
                           const std::map<std::string, std::string>& config,
                           std::error_code& ec);
    
    // Helper: Find last full backup for differential
    std::string findLastFullBackup(const std::string& backup_dir);
};

} // namespace themis
