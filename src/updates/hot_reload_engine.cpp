/**
 * @file hot_reload_engine.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=2, H=3, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/hot_reload_engine.h"
#include "updates/batch5_safety_helpers.h"
#include "utils/logger.h"

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <openssl/evp.h>
#include <memory>

#ifdef THEMIS_ENABLE_CURL
#include <curl/curl.h>
#endif

namespace themis {
namespace updates {

namespace fs = std::filesystem;

// ============================================================================
// RAII Wrapper for EVP_MD_CTX (Error Code: 7441-7443)
// ============================================================================

/**
 * @brief RAII wrapper for EVP_MD_CTX to ensure cleanup in all execution paths.
 * 
 * Guarantees exception-safe resource cleanup of OpenSSL EVP context.
 * Prevents resource leaks even during early returns or exceptions.
 * 
 * @error_code 7441 EVP_MD_CTX resource leak in exception path
 */
class EvpMdCtxRaii {
public:
    explicit EvpMdCtxRaii(EVP_MD_CTX* ctx = nullptr) : ctx_(ctx) {}
    
    ~EvpMdCtxRaii() {
        if (ctx_) {
            EVP_MD_CTX_free(ctx_);
        }
    }
    
    // Non-copyable
    EvpMdCtxRaii(const EvpMdCtxRaii&) = delete;
    EvpMdCtxRaii& operator=(const EvpMdCtxRaii&) = delete;
    
    // Movable
    EvpMdCtxRaii(EvpMdCtxRaii&& other) noexcept : ctx_(other.release()) {}
    EvpMdCtxRaii& operator=(EvpMdCtxRaii&& other) noexcept {
        if (this != &other) {
            if (ctx_) EVP_MD_CTX_free(ctx_);
            ctx_ = other.release();
        }
        return *this;
    }
    
    EVP_MD_CTX* get() const noexcept { return ctx_; }
    EVP_MD_CTX* release() noexcept {
        EVP_MD_CTX* tmp = ctx_;
        ctx_ = nullptr;
        return tmp;
    }
    
private:
    EVP_MD_CTX* ctx_ = nullptr;
};

HotReloadEngine::HotReloadEngine(
    std::shared_ptr<ManifestDatabase> manifest_db,
    std::shared_ptr<utils::UpdateChecker> update_checker,
    const Config& config
)
    : manifest_db_(std::move(manifest_db))
    , update_checker_(std::move(update_checker))
    , config_(config) {
    
    // Create directories
    fs::create_directories(config_.download_directory);
    fs::create_directories(config_.backup_directory);

    // Initialise history logger if a path is configured
    if (!config_.history_log_path.empty()) {
        history_logger_ = std::make_unique<UpdateHistoryLogger>(config_.history_log_path);
    }

    LOG_INFO("HotReloadEngine initialized");
}

HotReloadEngine::~HotReloadEngine() = default;

HotReloadEngine::DownloadResult HotReloadEngine::downloadRelease(const std::string& version) {
    HotReloadEngine::DownloadResult result;
    result.success = false;
    
    reportProgress(0, "Fetching manifest for version " + version);
    
    // Get manifest from database first
    // CRITICAL: data_race fix - manifest_db_ is accessed by multiple methods
    std::shared_ptr<ManifestDatabase> manifest_db;
    {
        std::lock_guard<std::mutex> lock(manifest_db_mutex_);
        manifest_db = manifest_db_;
    }
    
    if (!manifest_db) {
        result.error_message = "Manifest database is not available";
        LOG_ERROR("{}", result.error_message);
        return result;
    }
    
    auto manifest = manifest_db->getManifest(version);
    if (!manifest) {
        result.error_message = "Manifest not found for version: " + version;
        LOG_ERROR("{}", result.error_message);
        return result;
    }
    
    result.manifest = *manifest;
    
    // Verify manifest
    reportProgress(10, "Verifying manifest");
    if (config_.verify_signatures && !manifest_db->verifyManifest(*manifest)) {
        result.error_message = "Manifest verification failed";
        LOG_ERROR("{}", result.error_message);
        return result;
    }
    
    // Create version-specific download directory
    std::string version_dir = config_.download_directory + "/" + version;
    fs::create_directories(version_dir);
    result.download_path = version_dir;
    
    // Download files
    size_t file_count = manifest->files.size();
    size_t current_file = 0;
    
    for (const auto& file : manifest->files) {
        current_file++;
        int progress = 10 + (static_cast<int>(current_file) * 80 / static_cast<int>(file_count));
        reportProgress(progress, "Downloading " + file.path);
        
        std::string dest_path = version_dir + "/" + file.path;
        
        // Check cache first
        auto cached_path = manifest_db->getCachedDownload(version, file.path);
        if (cached_path && fs::exists(*cached_path)) {
            // Verify cached file
            if (verifyDownloadedFile(file, *cached_path)) {
                LOG_DEBUG("Using cached file: {}", *cached_path);
                continue;
            }
        }
        
        // Create parent directories
        fs::create_directories(fs::path(dest_path).parent_path());
        
        // Download file
        if (!downloadFile(file, dest_path)) {
            result.error_message = "Failed to download file: " + file.path;
            LOG_ERROR("{}", result.error_message);
            return result;
        }
        
        // Verify downloaded file
        if (!verifyDownloadedFile(file, dest_path)) {
            result.error_message = "Verification failed for file: " + file.path;
            LOG_ERROR("{}", result.error_message);
            return result;
        }
        
        // Cache download
        manifest_db->cacheDownload(version, file.path, dest_path);
    }
    
    reportProgress(100, "Download complete");
    result.success = true;
    return result;
}

ReloadResult HotReloadEngine::applyHotReload(
    const std::string& version,
    bool verify_only
) {
    ReloadResult result;
    result.success = false;

    reportProgress(0, "Starting hot-reload for version " + version);

    // Capture current version early for history recording
    auto current_version = update_checker_->getConfig().current_version;

    // Helper: record a history entry (no-op when logger is not configured)
    auto recordHistory = [this, &current_version, &version](bool success,
                              const std::string& error_msg,
                              const std::string& event_type) {
        if (!history_logger_) return;
        UpdateHistoryEntry entry;
        entry.who           = config_.history_actor;
        entry.timestamp_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()).count();
        entry.from_version  = current_version;
        entry.to_version    = version;
        entry.event_type    = event_type;
        entry.success       = success;
        entry.error_message = error_msg;
        history_logger_->record(entry);
    };

    // Get manifest
    auto manifest = manifest_db_->getManifest(version);
    if (!manifest) {
        result.error_message = "Manifest not found";
        recordHistory(false, result.error_message, "update");
        return result;
    }

    // Verify release
    reportProgress(10, "Verifying release");
    auto verify_result = verifyRelease(*manifest);
    if (!verify_result.verified) {
        result.error_message = verify_result.error_message;
        recordHistory(false, result.error_message, "update");
        return result;
    }

    if (verify_only) {
        result.success = true;
        result.error_message = "Verification passed (dry-run mode)";
        return result;
    }

    // Check compatibility
    reportProgress(20, "Checking compatibility");
    if (!isCompatibleUpgrade(current_version, version)) {
        result.error_message = "Incompatible upgrade from " + current_version + " to " + version;
        recordHistory(false, result.error_message, "update");
        return result;
    }

    // Create backup
    std::string rollback_id;
    if (config_.create_backup) {
        reportProgress(30, "Creating backup");
        rollback_id = createBackup(manifest->files);
        result.rollback_id = rollback_id;
    }

    // Apply updates
    reportProgress(50, "Applying updates");
    size_t file_count = manifest->files.size();
    size_t current_file = 0;

    std::string version_dir = config_.download_directory + "/" + version;

    for (const auto& file : manifest->files) {
        current_file++;
        int progress = 50 + (static_cast<int>(current_file) * 40 / static_cast<int>(file_count));
        reportProgress(progress, "Updating " + file.path);

        std::string src_path = version_dir + "/" + file.path;
        std::string dst_path = config_.install_directory + "/" + file.path;

        if (!fs::exists(src_path)) {
            result.error_message = "Source file not found: " + src_path;
            LOG_ERROR("{}", result.error_message);

            // Rollback if backup was created
            if (!rollback_id.empty()) {
                rollback(rollback_id);
            }
            recordHistory(false, result.error_message, "update");
            return result;
        }

        // Atomic replace
        if (!atomicReplace(src_path, dst_path)) {
            result.error_message = "Failed to replace file: " + file.path;
            LOG_ERROR("{}", result.error_message);

            // Rollback
            if (!rollback_id.empty()) {
                rollback(rollback_id);
            }
            recordHistory(false, result.error_message, "update");
            return result;
        }

        result.files_updated.push_back(file.path);
    }

    // Run post-update health check if registered
    if (post_update_health_check_) {
        reportProgress(95, "Running post-update health check");
        bool healthy = post_update_health_check_();
        if (!healthy) {
            result.health_check_failed = true;
            if (config_.rollback_on_health_check_failure && !rollback_id.empty()) {
                LOG_WARN("Post-update health check failed for version {}; rolling back to {}", version, rollback_id);
                result.error_message = "Post-update health check failed; rolled back to previous version";
                rollback(rollback_id);
            } else {
                LOG_WARN("Post-update health check failed for version {}; rollback skipped", version);
                result.error_message = "Post-update health check failed";
            }
            result.success = false;
            return result;
        }
        LOG_INFO("Post-update health check passed for version {}", version);
    }

    reportProgress(100, "Hot-reload complete");
    result.success = true;

    recordHistory(true, "", "update");
    LOG_INFO("Hot-reload applied successfully: {} -> {}", current_version, version);
    return result;
}

bool HotReloadEngine::rollback(const std::string& rollback_id) {
    try {
        std::string backup_dir = config_.backup_directory + "/" + rollback_id;
        
        if (!fs::exists(backup_dir)) {
            LOG_ERROR("Rollback directory not found: {}", backup_dir);
            if (history_logger_) {
                UpdateHistoryEntry entry;
                entry.who          = config_.history_actor;
                entry.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()).count();
                entry.event_type   = "rollback";
                entry.success      = false;
                entry.error_message = "Rollback directory not found: " + rollback_id;
                history_logger_->record(entry);
            }
            return false;
        }
        
        // Read rollback metadata
        std::string metadata_path = backup_dir + "/rollback.json";
        if (!fs::exists(metadata_path)) {
            LOG_ERROR("Rollback metadata not found");
            return false;
        }
        
        // IMPORTANT: File I/O with implicit timeout consideration (Error Code: 7482)
        // Note: std::ifstream is synchronous. Filesystem-level timeouts should be
        // implemented at a higher level if needed for non-local filesystems.
        std::ifstream metadata_file(metadata_path);
        if (!metadata_file.is_open()) {
            LOG_ERROR("Failed to open rollback metadata: {}", metadata_path);
            return false;
        }
        
        try {
            json metadata_json;
            metadata_file >> metadata_json;
            
            // Restore files
            for (const auto& file_json : metadata_json["files"]) {
                std::string file_path = file_json["path"];
                std::string backup_file = backup_dir + "/" + file_path;
                std::string dest_file = config_.install_directory + "/" + file_path;
                
                if (fs::exists(backup_file)) {
                    fs::copy_file(backup_file, dest_file, fs::copy_options::overwrite_existing);
                    LOG_DEBUG("Restored: {}", file_path);
                }
            }
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to restore files from rollback: {}", e.what());
            return false;
        }

        if (history_logger_) {
            UpdateHistoryEntry entry;
            entry.who           = config_.history_actor;
            entry.timestamp_ms  = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            entry.from_version  = update_checker_->getConfig().current_version;
            entry.event_type    = "rollback";
            entry.success       = true;
            history_logger_->record(entry);
        }

        LOG_INFO("Rollback completed: {}", rollback_id);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Rollback failed: {}", e.what());
        return false;
    }
}

VerificationResult HotReloadEngine::verifyRelease(const ReleaseManifest& manifest) {
    VerificationResult result;
    result.verified = false;
    
    // Verify manifest
    if (!manifest_db_->verifyManifest(manifest)) {
        result.error_message = "Manifest verification failed";
        return result;
    }
    
    // Check if files are downloaded
    std::string version_dir = config_.download_directory + "/" + manifest.version;
    for (const auto& file : manifest.files) {
        std::string file_path = version_dir + "/" + file.path;
        if (!fs::exists(file_path)) {
            result.warnings.push_back("File not downloaded: " + file.path);
        } else {
            // Verify hash
            if (!verifyDownloadedFile(file, file_path)) {
                result.error_message = "Hash mismatch for file: " + file.path;
                return result;
            }
        }
    }
    
    result.verified = true;
    return result;
}

bool HotReloadEngine::isCompatibleUpgrade(
    const std::string& current_version,
    const std::string& target_version
) {
    // Parse versions
    auto current = utils::Version::parse(current_version);
    auto target = utils::Version::parse(target_version);
    
    if (!current || !target) {
        return false;
    }
    
    // Can't downgrade
    if (*target < *current) {
        LOG_WARN("Cannot downgrade from {} to {}", current_version, target_version);
        return false;
    }
    
    // Check manifest's minimum upgrade version
    auto manifest = manifest_db_->getManifest(target_version);
    if (manifest && !manifest->min_upgrade_from.empty()) {
        auto min_version = utils::Version::parse(manifest->min_upgrade_from);
        if (min_version && *current < *min_version) {
            LOG_WARN("Current version {} is below minimum required {}", 
                current_version, manifest->min_upgrade_from);
            return false;
        }
    }
    
    return true;
}

std::vector<std::pair<std::string, std::string>> HotReloadEngine::listRollbackPoints() const {
    std::vector<std::pair<std::string, std::string>> rollback_points;
    
    try {
        // Store iterator to ensure valid lifetime across loop (Error Code: 7443)
        auto it = fs::directory_iterator(config_.backup_directory);
        for (const auto& entry : it) {
            if (entry.is_directory()) {
                std::string rollback_id = entry.path().filename().string();
                std::string metadata_path = entry.path().string() + "/rollback.json";
                
                if (fs::exists(metadata_path)) {
                    // IMPORTANT: File I/O with implicit timeout consideration (Error Code: 7481)
                    // Note: std::ifstream is synchronous. In production, filesystem operations
                    // should be protected by filesystem-level timeouts or async mechanisms.
                    // For now, we assume the filesystem is responsive (typical for local storage).
                    std::ifstream metadata_file(metadata_path);
                    if (!metadata_file.is_open()) {
                        LOG_WARN("HotReloadEngine: failed to open rollback metadata: {}", metadata_path);
                        continue;
                    }
                    
                    try {
                        json metadata_json;
                        metadata_file >> metadata_json;
                        
                        std::string timestamp = metadata_json.value("timestamp", "unknown");
                        rollback_points.emplace_back(rollback_id, timestamp);
                    } catch (const std::exception& e) {
                        LOG_WARN("HotReloadEngine: failed to parse rollback metadata: {}", e.what());
                        continue;
                    }
                }
            }
        }
        
        // Sort by timestamp (newest first)
        std::sort(rollback_points.begin(), rollback_points.end(),
            [](const auto& a, const auto& b) { return a.second > b.second; });
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to list rollback points: {}", e.what());
    }
    
    return rollback_points;
}

void HotReloadEngine::cleanRollbackPoints([[maybe_unused]] size_t keep_count) {
    auto rollback_points = listRollbackPoints();
    
    if (rollback_points.size() <= keep_count) {
        return;
    }
    
    // Delete old rollback points
    for (size_t i = keep_count; i < rollback_points.size(); i++) {
        std::string rollback_dir = config_.backup_directory + "/" + rollback_points[i].first;
        try {
            fs::remove_all(rollback_dir);
            LOG_INFO("Cleaned old rollback point: {}", rollback_points[i].first);
        } catch (const std::exception& e) {
            LOG_ERROR("Failed to clean rollback point {}: {}", rollback_points[i].first, e.what());
        }
    }
}

void HotReloadEngine::setProgressCallback(
    std::function<void(int, const std::string&)> callback
) {
    progress_callback_ = std::move([[maybe_unused]] callback);
}

void HotReloadEngine::setPostUpdateHealthCheck(PostUpdateHealthCheck check) {
    post_update_health_check_ = std::move(check);
}

#ifdef THEMIS_ENABLE_CURL
static size_t writeFileCallback(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}
#endif

bool HotReloadEngine::downloadFile(const ReleaseFile& file, const std::string& dest) {
#ifdef THEMIS_ENABLE_CURL
    if (file.download_url.empty()) {
        LOG_ERROR("No download URL for file: {}", file.path);
        return false;
    }
    
    CURL* curl = curl_easy_init();
    if (!curl) {
        LOG_ERROR("Failed to initialize CURL");
        return false;
    }
    
    FILE* fp = fopen(dest.c_str(), "wb");
    if (!fp) {
        LOG_ERROR("Failed to open file for writing: {}", dest);
        curl_easy_cleanup(curl);
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, file.download_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writeFileCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);  // 5 minutes
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ThemisDB-HotReload/1.0");
    
    CURLcode res = curl_easy_perform(curl);
    
    fclose(fp);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        LOG_ERROR("CURL error downloading {}: {}", file.path, curl_easy_strerror(res));
        fs::remove(dest);  // Clean up partial download
        return false;
    }
    
    return true;
#else
    static_cast<void>(file);
    static_cast<void>(dest);
    LOG_ERROR("CURL support not enabled - cannot download files");
    return false;
#endif
}

bool HotReloadEngine::verifyDownloadedFile(const ReleaseFile& file, const std::string& path) {
    std::string actual_hash = calculateFileHash(path);
    
    if (actual_hash != file.sha256_hash) {
        LOG_ERROR("Hash mismatch for {}: expected {}, got {}",
            file.path, file.sha256_hash, actual_hash);
        return false;
    }
    
    // Verify size
    auto file_size = fs::file_size(path);
    if (file_size != file.size_bytes) {
        LOG_ERROR("Size mismatch for {}: expected {}, got {}",
            file.path, file.size_bytes, file_size);
        return false;
    }
    
    return true;
}

std::string HotReloadEngine::createBackup(const std::vector<ReleaseFile>& files) {
    std::string rollback_id = generateRollbackId();
    std::string backup_dir = config_.backup_directory + "/" + rollback_id;
    
    try {
        fs::create_directories(backup_dir);
        
        // Backup files
        for (const auto& file : files) {
            std::string src = config_.install_directory + "/" + file.path;
            std::string dst = backup_dir + "/" + file.path;
            
            if (fs::exists(src)) {
                fs::create_directories(fs::path(dst).parent_path());
                fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
            }
        }
        
        // Save metadata
        json metadata;
        metadata["rollback_id"] = rollback_id;
        metadata["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        metadata["files"] = json::array();
        for (const auto& file : files) {
            metadata["files"].push_back({{"path", file.path}});
        }
        
        std::ofstream metadata_file(backup_dir + "/rollback.json");
        metadata_file << metadata.dump(2);
        
        LOG_INFO("Backup created: {}", rollback_id);
        return rollback_id;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create backup: {}", e.what());
        return "";
    }
}

bool HotReloadEngine::atomicReplace(const std::string& src, const std::string& dst) {
    try {
        // Create temp file
        std::string temp = dst + ".tmp";
        
        // Copy to temp
        fs::copy_file(src, temp, fs::copy_options::overwrite_existing);
        
        // Atomic rename
        fs::rename(temp, dst);
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Atomic replace failed for {}: {}", dst, e.what());
        return false;
    }
}

std::string HotReloadEngine::calculateFileHash(const std::string& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return "";
    }
    
    // Use RAII wrapper for EVP_MD_CTX (Error Code: 7442)
    EvpMdCtxRaii mdctx(EVP_MD_CTX_new());
    if (!mdctx.get()) {
        return "";
    }
    
    if (EVP_DigestInit_ex(mdctx.get(), EVP_sha256(), nullptr) != 1) {
        return "";
    }
    
    const size_t bufferSize = 32768;
    std::vector<char> buffer(bufferSize);
    
    while (file.read(buffer.data(), bufferSize) || file.gcount() > 0) {
        if (EVP_DigestUpdate(mdctx.get(), buffer.data(), file.gcount()) != 1) {
            return "";
        }
    }
    
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int hashLen = 0;
    if (EVP_DigestFinal_ex(mdctx.get(), hash, &hashLen) != 1) {
        return "";
    }
    
    std::ostringstream ss;
    for (unsigned int i = 0; i < hashLen; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    
    return ss.str();
}

std::string HotReloadEngine::generateRollbackId() {
    // Simple timestamp-based ID
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()
    ).count();
    
    return "rollback_" + std::to_string(timestamp);
}

void HotReloadEngine::reportProgress(int percentage, const std::string& message) {
    LOG_DEBUG("Progress: {}% - {}", percentage, message);
    
    if ([[maybe_unused]] progress_callback_) {
        progress_callback_(percentage, message);
    }
}

UpdateHistoryLogger* HotReloadEngine::historyLogger() {
    return history_logger_.get();
}

} // namespace updates
} // namespace themis

