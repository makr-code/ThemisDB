/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            cloud_backup.h                                     ║
  Version:         0.0.12                                             ║
  Last Modified:   2026-04-15 05:38:01                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     222                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • 252b3f2e9c  2026-02-07  Implement production GPU backend, cloud backup infrastruc... ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <chrono>

namespace themis {

// Forward declarations
class BackupManager;

namespace sharding {

// Forward declarations
class CloudAgent;

/**
 * Cloud backup configuration
 */
struct CloudBackupConfig {
    // Cloud storage provider: "s3", "azure", "gcs"
    std::string provider;
    
    // S3 configuration
    std::string s3_bucket;
    std::string s3_region;
    std::string s3_endpoint;  // For S3-compatible storage like MinIO
    
    // Azure configuration
    std::string azure_account;
    std::string azure_container;
    
    // GCS configuration
    std::string gcs_project_id;
    std::string gcs_bucket;
    
    // Common settings
    std::string backup_prefix;  // Prefix for backup objects in cloud storage
    std::string local_backup_dir;  // Local staging directory
    
    // Retention policy
    int max_backups = 10;
    std::chrono::hours retention_period{24 * 7};  // 7 days default
    
    // Compression and encryption
    // NOTE: These flags are currently not applied by the coordinator/provider implementations
    // and are reserved for future use (v1.4.0). Callers MUST NOT assume backups are compressed
    // or encrypted unless they verify the underlying implementation enforces these settings.
    // When v1.4.0 is released with real SDK integration, these features will be implemented.
    bool enable_compression = false;
    bool enable_encryption = false;
    std::string encryption_key;
};

/**
 * Backup status
 */
enum class BackupStatus {
    PENDING,
    IN_PROGRESS,
    COMPLETED,
    FAILED,
    DELETED
};

/**
 * Backup information
 */
struct BackupInfo {
    std::string backup_id;
    std::chrono::system_clock::time_point timestamp;
    std::vector<std::string> shard_ids;
    BackupStatus status;
    std::string storage_provider;
    size_t total_size_bytes = 0;
    std::string error_message;
};

/**
 * Multi-datacenter replication target
 */
struct ReplicationTarget {
    std::string datacenter_id;
    std::vector<std::string> shard_endpoints;
    bool enabled = false;
    std::chrono::system_clock::time_point last_sync;
};

/**
 * Cloud Backup Coordinator
 * 
 * Provides cloud backup and multi-datacenter replication capabilities.
 * Supports S3, Azure Blob Storage, and Google Cloud Storage.
 * 
 * Features:
 * - Automatic backup to cloud storage
 * - Multi-datacenter replication
 * - Backup retention and lifecycle management
 * - Point-in-time recovery from cloud backups
 * - Incremental and full backup support
 */
class CloudBackupCoordinator {
public:
    /**
     * Constructor
     * 
     * @param cloud_agent Cloud agent for shard coordination
     * @param backup_manager Local backup manager
     * @param config Cloud backup configuration
     */
    CloudBackupCoordinator(std::shared_ptr<CloudAgent> cloud_agent,
                          std::shared_ptr<BackupManager> backup_manager,
                          const CloudBackupConfig& config);
    
    ~CloudBackupCoordinator();
    
    // Disable copy
    CloudBackupCoordinator(const CloudBackupCoordinator&) = delete;
    CloudBackupCoordinator& operator=(const CloudBackupCoordinator&) = delete;
    
    /**
     * Create a new backup and upload to cloud storage
     * 
     * @param backup_id Unique backup identifier
     * @param shard_ids List of shard IDs to backup
     * @return true if backup was successful
     */
    bool createBackup(const std::string& backup_id,
                     const std::vector<std::string>& shard_ids);
    
    /**
     * Restore a backup from cloud storage
     * 
     * @param backup_id Backup identifier to restore
     * @param shard_ids List of shard IDs to restore
     * @return true if restore was successful
     */
    bool restoreBackup(const std::string& backup_id,
                      const std::vector<std::string>& shard_ids);
    
    /**
     * Delete a backup from cloud storage
     * 
     * @param backup_id Backup identifier to delete
     * @return true if deletion was successful
     */
    bool deleteBackup(const std::string& backup_id);
    
    /**
     * List all available backups
     * 
     * @return Vector of backup information
     */
    std::vector<BackupInfo> listBackups() const;
    
    /**
     * Get information about a specific backup
     * 
     * @param backup_id Backup identifier
     * @return Backup info if found, nullopt otherwise
     */
    std::optional<BackupInfo> getBackupInfo(const std::string& backup_id) const;
    
    /**
     * Configure a replication target datacenter
     * 
     * @param datacenter_id Unique datacenter identifier
     * @param shard_endpoints List of shard endpoints in target datacenter
     * @return true if configuration was successful
     */
    bool setReplicationTarget(const std::string& datacenter_id,
                             const std::vector<std::string>& shard_endpoints);
    
    /**
     * Enable continuous replication to a datacenter
     * 
     * @param datacenter_id Target datacenter identifier
     * @return true if replication was enabled
     */
    bool enableContinuousReplication(const std::string& datacenter_id);
    
    /**
     * Disable continuous replication to a datacenter
     * 
     * @param datacenter_id Target datacenter identifier
     * @return true if replication was disabled
     */
    bool disableContinuousReplication(const std::string& datacenter_id);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace sharding
} // namespace themis
