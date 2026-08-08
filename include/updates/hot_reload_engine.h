/**
 * @file hot_reload_engine.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: hot_reload_engine.h | Version: 0.0.47
 * Maturity: 🟢 PRODUCTION-READY | Score: 100/100
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
 */

#pragma once

#include "updates/manifest_database.h"
#include "updates/update_history_logger.h"
#include "utils/update_checker.h"
#include <memory>
#include <mutex>
#include <string>
#include <vector>
#include <functional>

namespace themis {
namespace updates {

/**
 * @brief Result of a hot-reload operation
 */
struct ReloadResult {
    bool success = false;
    std::string error_message;
    std::vector<std::string> files_updated;
    std::string rollback_id;       ///< For rollback capability
    bool health_check_failed = false; ///< true if a post-update health check caused rollback
};

/**
 * @brief Result of manifest verification
 */
struct VerificationResult {
    bool verified = false;
    std::string error_message;
    std::vector<std::string> warnings;
};

/**
 * @brief Engine for hot-reloading ThemisDB from release manifests
 * 
 * Features:
 * - Download and verify releases from GitHub
 * - Atomic file replacement
 * - Automatic backup before update
 * - Rollback capability on failure
 */
class HotReloadEngine {
public:
    /**
     * @brief Result of a download operation
     */
    struct DownloadResult {
        bool success = false;
        std::string error_message;
        std::string download_path;
        ReleaseManifest manifest;
    };

    /**
     * @brief Configuration for hot-reload engine
     */
    struct Config {
        std::string download_directory = "/tmp/themis_updates";
        std::string backup_directory = "/var/lib/themisdb/rollback";
        std::string install_directory = ".";  // Current directory by default
        bool verify_signatures = true;
        bool create_backup = true;
        bool dry_run = false;  // Don't actually apply changes
        /// When true, automatically rollback if the post-update health check fails.
        bool rollback_on_health_check_failure = true;
        /// Path for the persistent update history log.
        /// If empty, history logging is disabled.
        std::string history_log_path;
        /// Actor name written into history entries (e.g. current user).
        std::string history_actor = "system";
    };
    
    /**
     * @brief Get default configuration
     */
    static const Config& defaultConfig() {
        static Config cfg{};
        return cfg;
    }
    
    /**
     * @brief Construct hot-reload engine
     * @param manifest_db Manifest database
     * @param update_checker Update checker for fetching releases
     * @param config Configuration
     */
    HotReloadEngine(
        std::shared_ptr<ManifestDatabase> manifest_db,
        std::shared_ptr<utils::UpdateChecker> update_checker,
        const Config& config = defaultConfig()
    );
    
    virtual ~HotReloadEngine();

    /**
     * @brief Download and verify a release
     * @param version Version to download (e.g., "1.2.3")
     * @return Download result
     */
    DownloadResult downloadRelease(const std::string& version);

    /**
     * @brief Apply hot-reload (atomic operation)
     * @param version Version to apply
     * @param verify_only Dry-run mode (don't actually apply)
     * @return Reload result
     */
    virtual ReloadResult applyHotReload(
        const std::string& version,
        bool verify_only = false
    );

    /**
     * @brief Rollback to previous version
     * @param rollback_id Rollback ID from previous reload
     * @return true if successful
     */
    virtual bool rollback(const std::string& rollback_id);
    
    /**
     * @brief Verify release before applying
     * @param manifest Manifest to verify
     * @return Verification result
     */
    VerificationResult verifyRelease(const ReleaseManifest& manifest);
    
    /**
     * @brief Check if upgrade is compatible
     * @param current_version Current version
     * @param target_version Target version
     * @return true if upgrade is compatible
     */
    bool isCompatibleUpgrade(
        const std::string& current_version,
        const std::string& target_version
    );
    
    /**
     * @brief List available rollback points
     * @return Vector of rollback IDs with timestamps
     */
    std::vector<std::pair<std::string, std::string>> listRollbackPoints() const;
    
    /**
     * @brief Clean old rollback points
     * @param keep_count Number of rollback points to keep
     */
    void cleanRollbackPoints(size_t keep_count = 3);
    
    /**
     * @brief Set progress callback for long operations
     * @param callback Callback function (progress percentage, message)
     */
    void setProgressCallback(
        std::function<void(int, const std::string&)> callback
    );

    /**
     * @brief Callback type for post-update health checks.
     *
     * The function should return @c true when the system is healthy after an
     * update, or @c false to signal that the update should be rolled back.
     */
    using PostUpdateHealthCheck = std::function<bool()>;

    /**
     * @brief Register a post-update health check.
     *
     * When set, the callback is invoked after all files have been atomically
     * replaced.  If it returns @c false and
     * @c Config::rollback_on_health_check_failure is @c true the engine
     * automatically rolls back to the previous version.
     *
     * @param check Health-check callable (may be @c nullptr to clear)
     */
    void setPostUpdateHealthCheck(PostUpdateHealthCheck check);

    /**
     * @brief Access the update history logger.
     * @return Pointer to the logger, or nullptr if history logging is disabled.
     */
    UpdateHistoryLogger* historyLogger();

private:
    std::shared_ptr<ManifestDatabase> manifest_db_;
    std::shared_ptr<utils::UpdateChecker> update_checker_;
    Config config_;
    std::function<void(int, const std::string&)> progress_callback_;
    
    // CRITICAL: Thread synchronization for manifest_db_ (data_race fix)
    mutable std::mutex manifest_db_mutex_;

protected:
    /// Protected (not private) so that test subclasses can access and invoke
    /// the registered callback without requiring a separate accessor.
    PostUpdateHealthCheck post_update_health_check_;

private:
    std::unique_ptr<UpdateHistoryLogger> history_logger_;
    
    /**
     * @brief Download single file with resume support
     * @param file ReleaseFile to download
     * @param dest Destination path
     * @return true if successful
     */
    bool downloadFile(const ReleaseFile& file, const std::string& dest);
    
    /**
     * @brief Verify downloaded file
     * @param file ReleaseFile with expected hash
     * @param path Local file path
     * @return true if hash matches
     */
    bool verifyDownloadedFile(const ReleaseFile& file, const std::string& path);
    
    /**
     * @brief Create backup before update
     * @return Rollback ID for the backup
     */
    std::string createBackup(const std::vector<ReleaseFile>& files);
    
    /**
     * @brief Atomic file replacement
     * @param src Source file path
     * @param dst Destination file path
     * @return true if successful
     */
    bool atomicReplace(const std::string& src, const std::string& dst);
    
    /**
     * @brief Calculate SHA-256 hash of file
     * @param path File path
     * @return Hex-encoded hash
     */
    std::string calculateFileHash(const std::string& path);
    
    /**
     * @brief Generate unique rollback ID
     * @return UUID-based rollback ID
     */
    std::string generateRollbackId();
    
    /**
     * @brief Report progress
     * @param percentage Progress percentage (0-100)
     * @param message Progress message
     */
    void reportProgress(int percentage, const std::string& message);
};

} // namespace updates
} // namespace themis
