/**
 * @file cloud_backup.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <optional>
#include <chrono>
#include <functional>

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
 *
 * ## Cloud SDK Integration (Callback Injection)
 * 
 * This coordinator uses a callback-based dependency injection pattern to support
 * cloud SDKs without hard build-time dependencies:
 * 
 * **Production Usage**:
 * @code
 * // Initialize cloud SDKs (e.g., AWS SDK for S3)
 * // ...
 * 
 * // Set callbacks before creating coordinator
 * setS3UploadFn([](const std::string& bucket, const std::string& local_path,
 *                   const std::string& remote_path,
 *                   const std::map<std::string, std::string>& metadata) {
 *     // Use AWS SDK to upload
 *     // return success status
 * });
 * // Set all 5 callbacks: upload, download, delete, list, exists
 * 
 * // Create coordinator - will verify all callbacks are set
 * CloudBackupCoordinator coordinator(agent, manager, config);
 * 
 * // Operations now use real cloud SDKs via callbacks
 * coordinator.createBackup("backup-1", {"shard1", "shard2"});
 * @endcode
 *
 * **Error Behavior**:
 * - If any required callback is not set, createBackup() fails immediately with
 *   THEMIS_ERROR logging and returns false (fail-closed behavior)
 * - All operations are deterministic: either succeed with real cloud operations
 *   or fail with clear error messages
 * 
 * @see setS3UploadFn, setS3DownloadFn, setS3DeleteFn, setS3ListFn, setS3ExistsFn
 * @see setAzureUploadFn, setAzureDownloadFn, setAzureDeleteFn, setAzureListFn, setAzureExistsFn
 * @see setGCSUploadFn, setGCSDownloadFn, setGCSDeleteFn, setGCSListFn, setGCSExistsFn
 * @see cloud_sdk_integration.h for real SDK implementations (S3, Azure, GCS)
 * @see src/sharding/cloud_backup.cpp for detailed implementation documentation
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
        * @return false when the cloud provider is not configured, the backup is
        *         unknown, the shard list is empty, any shard id is empty, any
        *         requested shard is not part of the catalogued backup, the backup
        *         id is empty, or no local BackupManager is available to apply the
        *         downloaded artifact
     */
    bool restoreBackup(const std::string& backup_id,
                      const std::vector<std::string>& shard_ids);
    
    /**
     * Delete a backup from cloud storage
     * 
     * @param backup_id Backup identifier to delete
     * @return true if deletion was successful
     * @return false when the backup_id is empty or the backup is not found
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
     * @return false when datacenter_id is empty or replication target not found
     */
    bool enableContinuousReplication(const std::string& datacenter_id);
    
    /**
     * Disable continuous replication to a datacenter
     * 
     * @param datacenter_id Target datacenter identifier
     * @return true if replication was disabled
     * @return false when datacenter_id is empty or replication target not found
     */
    bool disableContinuousReplication(const std::string& datacenter_id);
    
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

using S3DownloadFn = std::function<bool(const std::string& bucket,
                                        const std::string& remote_path,
                                        const std::string& local_path)>;
using S3UploadFn = std::function<bool(const std::string& bucket,
                                      const std::string& local_path,
                                      const std::string& remote_path,
                                      const std::map<std::string, std::string>& metadata)>;
using S3DeleteFn = std::function<bool(const std::string& bucket,
                                      const std::string& remote_path)>;
using S3ListFn = std::function<std::vector<std::string>(const std::string& bucket,
                                                        const std::string& prefix)>;
using S3ExistsFn = std::function<bool(const std::string& bucket,
                                      const std::string& remote_path)>;
using AzureUploadFn = std::function<bool(const std::string& account,
                                         const std::string& container,
                                         const std::string& local_path,
                                         const std::string& remote_path,
                                         const std::map<std::string, std::string>& metadata)>;
using AzureDownloadFn = std::function<bool(const std::string& account,
                                           const std::string& container,
                                           const std::string& remote_path,
                                           const std::string& local_path)>;
using AzureDeleteFn = std::function<bool(const std::string& account,
                                         const std::string& container,
                                         const std::string& remote_path)>;
using AzureListFn = std::function<std::vector<std::string>(const std::string& account,
                                                           const std::string& container,
                                                           const std::string& prefix)>;
using AzureExistsFn = std::function<bool(const std::string& account,
                                         const std::string& container,
                                         const std::string& remote_path)>;
using GCSUploadFn = std::function<bool(const std::string& bucket,
                                       const std::string& local_path,
                                       const std::string& remote_path,
                                       const std::map<std::string, std::string>& metadata)>;
using GCSDownloadFn = std::function<bool(const std::string& bucket,
                                         const std::string& remote_path,
                                         const std::string& local_path)>;
using GCSDeleteFn = std::function<bool(const std::string& bucket,
                                       const std::string& remote_path)>;
using GCSListFn = std::function<std::vector<std::string>(const std::string& bucket,
                                                         const std::string& prefix)>;
using GCSExistsFn = std::function<bool(const std::string& bucket,
                                       const std::string& remote_path)>;

void setS3DownloadFn(S3DownloadFn fn);
void setS3UploadFn(S3UploadFn fn);
void setS3DeleteFn(S3DeleteFn fn);
void setS3ListFn(S3ListFn fn);
void setS3ExistsFn(S3ExistsFn fn);
void setAzureUploadFn(AzureUploadFn fn);
void setAzureDownloadFn(AzureDownloadFn fn);
void setAzureDeleteFn(AzureDeleteFn fn);
void setAzureListFn(AzureListFn fn);
void setAzureExistsFn(AzureExistsFn fn);
void setGCSUploadFn(GCSUploadFn fn);
void setGCSDownloadFn(GCSDownloadFn fn);
void setGCSDeleteFn(GCSDeleteFn fn);
void setGCSListFn(GCSListFn fn);
void setGCSExistsFn(GCSExistsFn fn);

} // namespace sharding
} // namespace themis
