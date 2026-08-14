/**
 * @file manifest_database.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Gap Summary: total=4; TODO=1, Stub=2, Unimpl=0, Mock=1, Sim=0, Debt=0, C=3, H=4, M=6, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/manifest_database.h"
#include <memory>
#include <stdexcept>
#include "utils/logger.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <openssl/rand.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

namespace themis {
namespace updates {

// ============================================================================
// RAII wrapper for temporary file cleanup
// ============================================================================

/**
 * @brief RAII wrapper for temporary files with secure cleanup
 * @see Error Code: 7409 (temporary file cleanup on exception)
 */
class TempFileRaii {
public:
    explicit TempFileRaii(const std::string& path = "") : path_(path) {}
    
    ~TempFileRaii() {
        if (!path_.empty()) {
            try {
                std::error_code ec;
                std::filesystem::remove(path_, ec);
                if (ec) {
                    LOG_WARN("Failed to remove temporary file {}: {}", path_, ec.message());
                }
            } catch (const std::exception& e) {
                LOG_WARN("Exception removing temporary file {}: {}", path_, e.what());
            }
        }
    }
    
    // Non-copyable
    TempFileRaii(const TempFileRaii&) = delete;
    TempFileRaii& operator=(const TempFileRaii&) = delete;
    
    // Movable
    TempFileRaii(TempFileRaii&& other) noexcept : path_(std::move(other.path_)) {
        other.path_.clear();
    }
    TempFileRaii& operator=(TempFileRaii&& other) noexcept {
        if (this != &other) {
            // Cleanup old path if any
            if (!path_.empty()) {
                std::error_code ec;
                std::filesystem::remove(path_, ec);
            }
            path_ = std::move(other.path_);
            other.path_.clear();
        }
        return *this;
    }
    
    const std::string& path() const noexcept { return path_; }
    
    std::string release() noexcept {
        std::string tmp = std::move(path_);
        path_.clear();
        return tmp;
    }
    
private:
    std::string path_;
};

ManifestDatabase::ManifestDatabase(
    std::shared_ptr<RocksDBWrapper> storage,
    std::shared_ptr<acceleration::PluginSecurityVerifier> verifier
)
    : storage_(std::move(storage))
    , verifier_(std::move(verifier)) {
    initializeColumnFamilies();
}

ManifestDatabase::~ManifestDatabase() {
    // Column family handles are managed by RocksDBWrapper
}

void ManifestDatabase::initializeColumnFamilies() {
    auto cf_manifests = storage_->getOrCreateColumnFamily("release_manifests");
    auto cf_files = storage_->getOrCreateColumnFamily("file_registry");
    auto cf_signatures = storage_->getOrCreateColumnFamily("signature_cache");
    auto cf_cache = storage_->getOrCreateColumnFamily("download_cache");
    
    if (cf_manifests && cf_files && cf_signatures && cf_cache) {
        std::lock_guard<std::mutex> lock(cf_mutex_);
        cf_manifests_ = *cf_manifests;
        cf_files_ = *cf_files;
        cf_signatures_ = *cf_signatures;
        cf_cache_ = *cf_cache;
        LOG_INFO("ManifestDatabase column families initialized");
    } else {
        LOG_ERROR("Failed to initialize ManifestDatabase column families:");
        if (!cf_manifests) LOG_ERROR("  - release_manifests: {}", cf_manifests.error().message());
        if (!cf_files) LOG_ERROR("  - file_registry: {}", cf_files.error().message());
        if (!cf_signatures) LOG_ERROR("  - signature_cache: {}", cf_signatures.error().message());
        if (!cf_cache) LOG_ERROR("  - download_cache: {}", cf_cache.error().message());
        
        // Fall back to default CF
        std::lock_guard<std::mutex> lock(cf_mutex_);
        cf_manifests_ = nullptr;
        cf_files_ = nullptr;
        cf_signatures_ = nullptr;
        cf_cache_ = nullptr;
    }
}

bool ManifestDatabase::storeManifest(const ReleaseManifest& manifest) {
    try {
        std::string key = manifest.version;
        std::string value = manifest.toJson().dump();
        
        std::lock_guard<std::mutex> lock(cf_mutex_);
        rocksdb::Status status = storage_->getRawDB()->Put(
            rocksdb::WriteOptions(),
            cf_manifests_ ? cf_manifests_ : storage_->getRawDB()->DefaultColumnFamily(),
            key,
            value
        );
        
        if (!status.ok()) {
            LOG_ERROR("Failed to store manifest {}: {}", manifest.version, status.ToString());
            return false;
        }
        
        // Store individual files
        for (const auto& file : manifest.files) {
            if (!storeFile(file, manifest.version)) {
                LOG_WARN("Failed to store file {} for version {}", file.path, manifest.version);
            }
        }
        
        LOG_INFO("Stored manifest for version {}", manifest.version);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception storing manifest: {}", e.what());
        return false;
    }
}

std::optional<ReleaseManifest> ManifestDatabase::getManifest(const std::string& version) {
    try {
        std::string value;
        {
            std::lock_guard<std::mutex> lock(cf_mutex_);
            rocksdb::Status status = storage_->getRawDB()->Get(
                rocksdb::ReadOptions(),
                cf_manifests_ ? cf_manifests_ : storage_->getRawDB()->DefaultColumnFamily(),
                version,
                &value
            );
            
            if (!status.ok()) {
                if (!status.IsNotFound()) {
                    LOG_ERROR("Failed to get manifest {}: {}", version, status.ToString());
                }
                return std::nullopt;
            }
        }
        
        auto j = json::parse(value);
        return ReleaseManifest::fromJson(j);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception getting manifest: {}", e.what());
        return std::nullopt;
    }
}

std::optional<ReleaseManifest> ManifestDatabase::getLatestManifest() {
    auto versions = listVersions();
    if (versions.empty()) {
        return std::nullopt;
    }
    
    // Return the last version (highest)
    return getManifest(versions.back());
}

std::vector<std::string> ManifestDatabase::listVersions() const {
    std::vector<std::string> versions;
    
    try {
        std::lock_guard<std::mutex> lock(cf_mutex_);
        // Wrap in unique_ptr so iterator is freed on all paths (Phase 8.4 RAII).
        auto it = std::unique_ptr<rocksdb::Iterator>(storage_->getRawDB()->NewIterator(
            rocksdb::ReadOptions(),
            cf_manifests_ ? cf_manifests_ : storage_->getRawDB()->DefaultColumnFamily()
        ));
        
        for (it->SeekToFirst(); it->Valid(); it->Next()) {
            versions.push_back(it->key().ToString());
        }
        // iterator freed automatically by unique_ptr destructor
        
        // Sort versions
        std::sort(versions.begin(), versions.end());
    } catch (const std::exception& e) {
        LOG_ERROR("Exception listing versions: {}", e.what());
    }
    
    return versions;
}

bool ManifestDatabase::verifyManifest(const ReleaseManifest& manifest) {
    // Calculate hash
    std::string calculated_hash = manifest.calculateHash();
    
    // Check if hash matches
    if (calculated_hash != manifest.manifest_hash) {
        LOG_ERROR("Manifest hash mismatch for version {}: expected {}, got {}",
            manifest.version, manifest.manifest_hash, calculated_hash);
        return false;
    }
    
    // Verify signature if verifier is available
    if (verifier_ && !manifest.signature.empty()) {
        // Check cache first
        auto cached = getCachedSignatureVerification(manifest.manifest_hash);
        if (cached) {
            return *cached;
        }
        
        // Create a PluginSignature from manifest data for verification
        acceleration::PluginSignature sig;
        sig.sha256Hash = manifest.manifest_hash;
        sig.signature = manifest.signature;
        sig.signingCertificate = manifest.signing_certificate;
        sig.verified = false;
        
        // Verify signature using the plugin security verifier
        // Note: We're verifying the manifest hash signature, not a file
        // For this, we create a temporary file containing the hash
        try {
            // Get system temporary directory and create cryptographically secure random filename
            auto tempDir = std::filesystem::temp_directory_path();
            
            // Generate cryptographically secure random filename using OpenSSL
            unsigned char randomBytes[16];
            if (RAND_bytes(randomBytes, sizeof(randomBytes)) != 1) {
                LOG_ERROR("Failed to generate secure random bytes using OpenSSL RAND_bytes for temp filename");
                return false;
            }
            
            // Convert random bytes to hex string
            std::ostringstream oss;
            oss << "themis_";
            for (size_t i = 0; i < sizeof(randomBytes); ++i) {
                oss << std::hex << std::setw(2) << std::setfill('0') 
                    << static_cast<int>(randomBytes[i]);
            }
            oss << ".tmp";
            std::string uniqueName = oss.str();
            std::string tempPath = (tempDir / uniqueName).string();
            
            // RAII wrapper ensures cleanup on all paths (Error 7409)
            TempFileRaii temp_guard(tempPath);
            
            // Create temporary file with restricted permissions
            std::ofstream temp(tempPath, std::ios::binary | std::ios::trunc);
            if (!temp) {
                LOG_ERROR("Failed to create temporary file for manifest verification");
                return false;
            }
            temp << manifest.manifest_hash;
            temp.close();
            
            // Set file permissions to owner-only (Unix-like systems)
            #ifndef _WIN32
            std::filesystem::permissions(tempPath, 
                std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
                std::filesystem::perm_options::replace);
            #endif
            
            bool verified = verifier_->verifySignature(tempPath, sig);
            
            // Cleanup via RAII guard happens automatically on scope exit
            
            // Cache the result
            cacheSignatureVerification(manifest.manifest_hash, verified, manifest.signing_certificate);
            
            if (!verified) {
                LOG_ERROR("Manifest signature verification failed for version {}", manifest.version);
                return false;
            }
            
            LOG_INFO("Manifest signature verified for version {}", manifest.version);
        } catch (const std::exception& e) {
            // RAII cleanup ensures temp file is removed even on exception (Error 7409)
            LOG_ERROR("Exception during manifest signature verification: {}", e.what());
            return false;
        }
    }
    
    return true;
}

bool ManifestDatabase::verifyFile(const std::string& path, const std::string& version) {
    auto file = getFile(path, version);
    if (!file) {
        LOG_ERROR("File not found in registry: {} (version {})", path, version);
        return false;
    }
    
    // Check if file exists on filesystem
    if (!std::filesystem::exists(path)) {
        LOG_ERROR("File does not exist on filesystem: {}", path);
        return false;
    }
    
    // Verify file hash if available
    if (!file->sha256_hash.empty() && verifier_) {
        std::string actualHash = verifier_->calculateFileHash(path);
        if (actualHash.empty()) {
            LOG_ERROR("Failed to calculate hash for file: {}", path);
            return false;
        }
        
        if (actualHash != file->sha256_hash) {
            LOG_ERROR("File hash mismatch for {}: expected {}, got {}", 
                     path, file->sha256_hash, actualHash);
            return false;
        }
        
        LOG_DEBUG("File hash verified for {}: {}", path, actualHash);
    }
    
    // Verify individual file signature if available
    if (!file->file_signature.empty() && verifier_) {
        auto manifest = getManifest(version);
        if (!manifest) {
            LOG_ERROR("Manifest not found for version {}", version);
            return false;
        }
        
        acceleration::PluginSignature sig;
        sig.sha256Hash = file->sha256_hash;
        sig.signature = file->file_signature;
        sig.signingCertificate = manifest->signing_certificate;
        sig.verified = false;
        
        bool verified = verifier_->verifySignature(path, sig);
        if (!verified) {
            LOG_ERROR("File signature verification failed for {}", path);
            return false;
        }
        
        LOG_INFO("File signature verified for {}", path);
    }
    
    return true;
}

std::optional<ReleaseFile> ManifestDatabase::getFile(
    const std::string& path,
    const std::string& version
) {
    try {
        std::string key = path + ":" + version;
        std::string value;
        
        {
            std::lock_guard<std::mutex> lock(cf_mutex_);
            rocksdb::Status status = storage_->getRawDB()->Get(
                rocksdb::ReadOptions(),
                cf_files_ ? cf_files_ : storage_->getRawDB()->DefaultColumnFamily(),
                key,
                &value
            );
            
            if (!status.ok()) {
                return std::nullopt;
            }
        }
        
        auto j = json::parse(value);
        return ReleaseFile::fromJson(j);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception getting file: {}", e.what());
        return std::nullopt;
    }
}

bool ManifestDatabase::storeFile(const ReleaseFile& file, const std::string& version) {
    try {
        std::string key = file.path + ":" + version;
        std::string value = file.toJson().dump();
        
        std::lock_guard<std::mutex> lock(cf_mutex_);
        rocksdb::Status status = storage_->getRawDB()->Put(
            rocksdb::WriteOptions(),
            cf_files_ ? cf_files_ : storage_->getRawDB()->DefaultColumnFamily(),
            key,
            value
        );
        
        return status.ok();
    } catch (const std::exception& e) {
        LOG_ERROR("Exception storing file: {}", e.what());
        return false;
    }
}

void ManifestDatabase::cacheSignatureVerification(
    const std::string& hash,
    bool verified,
    const std::string& certificate
) {
    try {
        json j;
        j["verified"] = verified;
        j["certificate"] = certificate;
        j["timestamp"] = std::chrono::system_clock::now().time_since_epoch().count();
        
        std::string value = j.dump();
        
        std::lock_guard<std::mutex> lock(cf_mutex_);
        storage_->getRawDB()->Put(
            rocksdb::WriteOptions(),
            cf_signatures_ ? cf_signatures_ : storage_->getRawDB()->DefaultColumnFamily(),
            hash,
            value
        );
    } catch (const std::exception& e) {
        LOG_ERROR("Exception caching signature verification: {}", e.what());
    }
}

std::optional<bool> ManifestDatabase::getCachedSignatureVerification(const std::string& hash) {
    try {
        std::string value;
        {
            std::lock_guard<std::mutex> lock(cf_mutex_);
            rocksdb::Status status = storage_->getRawDB()->Get(
                rocksdb::ReadOptions(),
                cf_signatures_ ? cf_signatures_ : storage_->getRawDB()->DefaultColumnFamily(),
                hash,
                &value
            );
            
            if (!status.ok()) {
                return std::nullopt;
            }
        }
        
        auto j = json::parse(value);
        return j.value("verified", false);
    } catch (...) {
        return std::nullopt;
    }
}

void ManifestDatabase::cacheDownload(
    const std::string& version,
    const std::string& filename,
    const std::string& local_path
) {
    try {
        std::string key = version + ":" + filename;
        
        std::lock_guard<std::mutex> lock(cf_mutex_);
        storage_->getRawDB()->Put(
            rocksdb::WriteOptions(),
            cf_cache_ ? cf_cache_ : storage_->getRawDB()->DefaultColumnFamily(),
            key,
            local_path
        );
    } catch (const std::exception& e) {
        LOG_ERROR("Exception caching download: {}", e.what());
    }
}

std::optional<std::string> ManifestDatabase::getCachedDownload(
    const std::string& version,
    const std::string& filename
) {
    try {
        std::string key = version + ":" + filename;
        std::string value;
        
        {
            std::lock_guard<std::mutex> lock(cf_mutex_);
            rocksdb::Status status = storage_->getRawDB()->Get(
                rocksdb::ReadOptions(),
                cf_cache_ ? cf_cache_ : storage_->getRawDB()->DefaultColumnFamily(),
                key,
                &value
            );
            
            if (!status.ok()) {
                return std::nullopt;
            }
        }
        
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

bool ManifestDatabase::deleteManifest(const std::string& version) {
    try {
        // Retrieve manifest before deletion to obtain the list of associated files.
        // Also acts as an existence check: if the manifest is absent, abort early.
        auto manifest_opt = getManifest(version);
        if (!manifest_opt) {
            LOG_ERROR("Cannot delete manifest {}: not found", version);
            return false;
        }

        std::lock_guard<std::mutex> lock(cf_mutex_);
        auto* manifests_cf = cf_manifests_ ? cf_manifests_
                                           : storage_->getRawDB()->DefaultColumnFamily();
        auto* files_cf = cf_files_ ? cf_files_
                                   : storage_->getRawDB()->DefaultColumnFamily();
        auto* cache_cf = cf_cache_ ? cf_cache_
                                   : storage_->getRawDB()->DefaultColumnFamily();

        // Write tombstone key before the deletion window to guard against races.
        // This signals that deletion of version is in progress; on restart any
        // pending tombstones can be detected and the cleanup retried.
        const std::string tombstone_key = "__tombstone__:" + version;
        storage_->getRawDB()->Put(
            rocksdb::WriteOptions(),
            manifests_cf,
            tombstone_key,
            "deleting"
        );

        // Delete manifest record from RocksDB (committed before touching files)
        rocksdb::Status status = storage_->getRawDB()->Delete(
            rocksdb::WriteOptions(),
            manifests_cf,
            version
        );

        if (!status.ok()) {
            LOG_ERROR("Failed to delete manifest {}: {}", version, status.ToString());
            // Remove tombstone so the version is not stuck in a deleting state
            storage_->getRawDB()->Delete(rocksdb::WriteOptions(), manifests_cf, tombstone_key);
            return false;
        }

        // Delete associated files only after the RocksDB manifest entry is committed
        for (const auto& file : manifest_opt->files) {
            // Remove file_registry entry from RocksDB
            const std::string file_key = file.path + ":" + version;
            rocksdb::Status file_status = storage_->getRawDB()->Delete(
                rocksdb::WriteOptions(),
                files_cf,
                file_key
            );
            if (!file_status.ok() && !file_status.IsNotFound()) {
                LOG_WARN("Failed to delete file registry entry for {}: {}",
                         file.path, file_status.ToString());
            }

            // Look up the cached local path and remove the file from the filesystem
            auto cached_path = getCachedDownload(version, file.path);
            if (cached_path) {
                std::error_code ec;
                if (std::filesystem::remove(*cached_path, ec)) {
                    LOG_DEBUG("Deleted cached file: {}", *cached_path);
                } else if (ec) {
                    LOG_WARN("Failed to delete cached file {}: {}",
                             *cached_path, ec.message());
                }

                // Remove download cache entry from RocksDB
                const std::string cache_key = version + ":" + file.path;
                storage_->getRawDB()->Delete(
                    rocksdb::WriteOptions(),
                    cache_cf,
                    cache_key
                );
            }
        }

        // Deletion window complete; remove tombstone
        storage_->getRawDB()->Delete(rocksdb::WriteOptions(), manifests_cf, tombstone_key);

        LOG_INFO("Deleted manifest for version {}", version);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception deleting manifest: {}", e.what());
        return false;
    }
}

} // namespace updates
} // namespace themis


