#pragma once

#include <string>
#include <system_error>
#include <memory>
#include <vector>
#include <map>
#include "utils/expected.h"

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
     * @return Result<std::string> containing backup directory path on success, Error on failure
     */
    Result<std::string> createFullBackup(const std::string& dest_dir);

    /**
     * Create an incremental backup (WAL files since last backup)
     * For RAID5/6: Backs up incremental changes from ALL shards
     * @param dest_dir: Base backup directory
     * @return Result<std::string> containing backup directory path on success, Error on failure
     */
    Result<std::string> createIncrementalBackup(const std::string& dest_dir);
    
    /**
     * Create a differential backup (changes since last full backup)
     * @param dest_dir: Base backup directory
     * @return Result<std::string> containing backup directory path on success, Error on failure
     */
    Result<std::string> createDifferentialBackup(const std::string& dest_dir);

    /**
     * Archive WAL files to destination directory
     * @param dest_dir: Destination for WAL files
     * @return Result<void> on success, Error on failure
     */
    Result<void> archiveWAL(const std::string& dest_dir);

    /**
     * Restore database from backup directory
     * For RAID5/6: Restores from all shards (data + parity) to reconstruct complete data
     * @param src_dir: Source backup directory (full or incremental chain)
     * @return Result<void> on success, Error on failure
     */
    Result<void> restoreFromBackup(const std::string& src_dir);

    /**
     * List available backups in directory
     * @param backup_dir: Base backup directory
     * @return Vector of backup directory names sorted by timestamp
     */
    std::vector<std::string> listBackups(const std::string& backup_dir);

    /**
     * Verify backup integrity with checksum validation
     * For RAID5/6: Verifies ALL shards are present and consistent
     * @param backup_dir: Backup directory to verify
     * @return Result<void> on success, Error on failure
     */
    Result<void> verifyBackup(const std::string& backup_dir);
    
    /**
     * Compress a backup directory
     * @param backup_dir: Backup directory to compress
     * @return Result<std::string> containing compressed file path on success, Error on failure
     */
    Result<std::string> compressBackup(const std::string& backup_dir);
    
    /**
     * Decompress a backup file
     * @param compressed_file: Compressed backup file
     * @param dest_dir: Destination directory
     * @return Result<std::string> containing decompressed directory path on success, Error on failure
     */
    Result<std::string> decompressBackup(const std::string& compressed_file, const std::string& dest_dir);
    
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
     * @return Result<void> on success, Error on failure
     */
    Result<void> isBackupComplete(const std::string& backup_dir, 
                                   const RAIDConfig& raid_config);

private:
    std::shared_ptr<RocksDBWrapper> db_wrapper_;
    RAIDConfig raid_config_;
    
    // Helper: Get current timestamp string (YYYYMMDD_HHMMSS)
    std::string getTimestamp() const;
    
    // Helper: Create backup manifest file (with RAID info)
    Result<void> createManifest(const std::string& backup_dir, const std::string& type,
                                uint64_t sequence_number);
    
    // Helper: Read backup manifest (including RAID info)
    Result<void> readManifest(const std::string& backup_dir, std::string& type,
                              uint64_t& sequence_number);
    
    // Helper: Copy WAL files with sequence number filtering
    Result<void> copyWALFiles(const std::string& src_dir, const std::string& dest_dir,
                              uint64_t min_sequence);
    
    // Helper: Calculate checksum for a file
    Result<std::string> calculateChecksum(const std::string& file_path);
    
    // Helper: Verify checksum of a file
    Result<void> verifyChecksum(const std::string& file_path, const std::string& expected_checksum);
    
    // Helper: Get current WAL sequence number from RocksDB
    uint64_t getCurrentSequenceNumber() const;
    
    // Helper: Parse RAID mode from string
    static RAIDMode parseRAIDMode(const std::string& mode_str);
    
    // Helper: Convert RAID mode to string
    static std::string raidModeToString(RAIDMode mode);
    
    // Helper: Verify all RAID5/6 shards are present in backup
    Result<void> verifyRAIDShardsInBackup(const std::string& backup_dir, 
                                          const RAIDConfig& raid_config);
};

} // namespace themis
