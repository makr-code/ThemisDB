/**
 * @file manifest_database.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 81/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "updates/manifest_database.h"
#include "updates/batch5_safety_helpers.h"
#include <memory>
#include <stdexcept>
#include "utils/logger.h"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <limits>
#include <openssl/bio.h>
#include <openssl/pem.h>
#include <openssl/rand.h>
#include <openssl/x509v3.h>
#include <rocksdb/db.h>
#include <rocksdb/options.h>
#include <rocksdb/utilities/transaction_db.h>

#define LOG_ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define LOG_INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define LOG_WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define LOG_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)

namespace themis {
namespace updates {

namespace {

std::string normalizeDistinguishedName(const std::string& distinguished_name) {
    std::string normalized;
    normalized.reserve(distinguished_name.size());
    for (const unsigned char character : distinguished_name) {
        if (!std::isspace(character)) {
            normalized.push_back(static_cast<char>(character));
        }
    }
    return normalized;
}

bool isTrustedManifestSigner(
    acceleration::PluginSecurityVerifier& verifier,
    const std::string& certificate_pem
) {
    const auto& trusted_issuers = verifier.getPolicy().trustedIssuers;
    if (certificate_pem.empty()
        || certificate_pem.size() > static_cast<size_t>(std::numeric_limits<int>::max())
        || trusted_issuers.empty()
        || !verifier.verifyCertificateChain(certificate_pem)) {
        return false;
    }

    std::unique_ptr<BIO, decltype(&BIO_free)> certificate_bio(
        BIO_new_mem_buf(certificate_pem.data(), static_cast<int>(certificate_pem.size())),
        BIO_free);
    if (!certificate_bio) {
        return false;
    }

    std::unique_ptr<X509, decltype(&X509_free)> certificate(
        PEM_read_bio_X509(certificate_bio.get(), nullptr, nullptr, nullptr),
        X509_free);
    if (!certificate) {
        return false;
    }

    std::unique_ptr<BIO, decltype(&BIO_free)> issuer_bio(BIO_new(BIO_s_mem()), BIO_free);
    if (!issuer_bio
        || X509_NAME_print_ex(issuer_bio.get(), X509_get_issuer_name(certificate.get()), 0, XN_FLAG_RFC2253) < 0) {
        return false;
    }

    char* issuer_data = nullptr;
    const long issuer_size = BIO_get_mem_data(issuer_bio.get(), &issuer_data);
    if (issuer_size <= 0 || issuer_data == nullptr) {
        return false;
    }

    const std::string actual_issuer(issuer_data, static_cast<size_t>(issuer_size));
    const std::string normalized_actual = normalizeDistinguishedName(actual_issuer);
    return std::any_of(trusted_issuers.begin(), trusted_issuers.end(), [&](const std::string& trusted_issuer) {
        return !trusted_issuer.empty()
            && normalizeDistinguishedName(trusted_issuer) == normalized_actual;
    });
}

} // namespace

// ============================================================================
// RAII wrapper for temporary file cleanup
// ============================================================================

class TempFileRaii {
public:
    explicit TempFileRaii(const std::string& path = "") : path_(path) {}
    
    ~TempFileRaii() {
        if (!path_.empty()) {
            try {
                std::error_code ec = {};
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
                std::error_code ec = {};
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

/**
 * @brief Initialize Column Families.
 * @details Calls: getOrCreateColumnFamily(), lock(), LOG_INFO(), LOG_ERROR(), error(), message().
 */
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

/**
 * @brief Store Manifest.
 * @param[in] manifest Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: toJson(), dump(), lock(), getRawDB(), Put(), rocksdb::WriteOptions(), DefaultColumnFamily(), ok().
 */
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

/**
 * @brief Get Manifest.
 * @param[in] version Input parameter.
 * @return Return value.
 * @details Calls: lock(), getRawDB(), Get(), rocksdb::ReadOptions(), DefaultColumnFamily(), ok(), IsNotFound(), LOG_ERROR().
 */
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

/**
 * @brief Get Latest Manifest.
 * @return Return value.
 * @details Calls: listVersions(), empty(), getManifest(), back().
 */
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
        /**
         * @brief Lock.
         * @param[in] cf_mutex_ Input parameter.
         * @return Return value.
         */
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

/**
 * @brief Verify Manifest.
 * @param[in] manifest Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: calculateHash(), LOG_ERROR(), empty(), verifyCertificateChain(), std::filesystem::temp_directory_path(), RAND_bytes(), std::setw(), std::setfill().
 */
bool ManifestDatabase::verifyManifest(const ReleaseManifest& manifest) {
    // Calculate hash
    std::string calculated_hash = manifest.calculateHash();
    
    // Check if hash matches
    if (calculated_hash.empty() || manifest.manifest_hash.empty() || calculated_hash != manifest.manifest_hash) {
        LOG_ERROR("Manifest hash mismatch for version {}: expected {}, got {}",
            manifest.version, manifest.manifest_hash, calculated_hash);
        return false;
    }

    if (!verifier_) {
        LOG_ERROR("Manifest verification rejected for version {}: no signature verifier is configured",
            manifest.version);
        return false;
    }
    if (manifest.signature.empty() || manifest.signing_certificate.empty()) {
        LOG_ERROR("Manifest verification rejected for version {}: signature or signing certificate is missing",
            manifest.version);
        return false;
    }
    if (!isTrustedManifestSigner(*verifier_, manifest.signing_certificate)) {
        LOG_ERROR("Manifest verification rejected for version {}: signing certificate is not trusted",
            manifest.version);
        return false;
    }

    acceleration::PluginSignature sig;
    sig.sha256Hash = manifest.manifest_hash;
    sig.signature = manifest.signature;
    sig.signingCertificate = manifest.signing_certificate;
    sig.verified = false;

    try {
        auto temp_dir = std::filesystem::temp_directory_path();
        unsigned char random_bytes[16];
        if (RAND_bytes(random_bytes, sizeof(random_bytes)) != 1) {
            LOG_ERROR("Failed to generate secure random bytes using OpenSSL RAND_bytes for temp filename");
            return false;
        }

        std::ostringstream name_stream;
        name_stream << "themis_";
        for (const unsigned char byte : random_bytes) {
            name_stream << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(byte);
        }
        name_stream << ".tmp";
        const std::string temp_path = (temp_dir / name_stream.str()).string();
        TempFileRaii temp_guard(temp_path);

        std::ofstream temp(temp_path, std::ios::binary | std::ios::trunc);
        if (!temp) {
            LOG_ERROR("Failed to create temporary file for manifest verification");
            return false;
        }
        temp << manifest.manifest_hash;
        temp.close();
        if (!temp) {
            LOG_ERROR("Failed to write manifest hash to temporary verification file");
            return false;
        }

#ifndef _WIN32
        std::filesystem::permissions(temp_path,
            std::filesystem::perms::owner_read | std::filesystem::perms::owner_write,
            std::filesystem::perm_options::replace);
#endif

        if (!verifier_->verifySignature(temp_path, sig)) {
            LOG_ERROR("Manifest signature verification failed for version {}", manifest.version);
            return false;
        }

        LOG_INFO("Manifest signature verified for version {}", manifest.version);
    } catch (const std::exception& e) {
        LOG_ERROR("Exception during manifest signature verification: {}", e.what());
        return false;
    }
    
    return true;
}

/**
 * @brief Verify File.
 * @param[in] path Input parameter.
 * @param[in] version Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: getFile(), LOG_ERROR(), std::filesystem::exists(), empty(), calculateFileHash(), LOG_DEBUG(), getManifest(), verifySignature().
 */
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

/**
 * @brief Get File.
 * @param[in] path Input parameter.
 * @param[in] version Input parameter.
 * @return Return value.
 * @details Calls: lock(), getRawDB(), Get(), rocksdb::ReadOptions(), DefaultColumnFamily(), ok(), json::parse(), ReleaseFile::fromJson().
 */
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

/**
 * @brief Store File.
 * @param[in] file Input parameter.
 * @param[in] version Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: toJson(), dump(), lock(), getRawDB(), Put(), rocksdb::WriteOptions(), DefaultColumnFamily(), ok().
 */
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

/**
 * @brief Cache Signature Verification.
 * @param[in] hash Input parameter.
 * @param[in] verified Input parameter.
 * @param[in] certificate Input parameter.
 * @details Calls: std::chrono::system_clock::now(), time_since_epoch(), count(), dump(), lock(), getRawDB(), Put(), rocksdb::WriteOptions().
 */
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

/**
 * @brief Get Cached Signature Verification.
 * @param[in] hash Input parameter.
 * @return Return value.
 * @details Calls: lock(), getRawDB(), Get(), rocksdb::ReadOptions(), DefaultColumnFamily(), ok(), json::parse(), value().
 */
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

/**
 * @brief Cache Download.
 * @param[in] version Input parameter.
 * @param[in] filename Input parameter.
 * @param[in] local_path Path to the local.
 * @details Calls: lock(), getRawDB(), Put(), rocksdb::WriteOptions(), DefaultColumnFamily(), LOG_ERROR(), what().
 */
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

/**
 * @brief Get Cached Download.
 * @param[in] version Input parameter.
 * @param[in] filename Input parameter.
 * @return Return value.
 * @details Calls: lock(), getRawDB(), Get(), rocksdb::ReadOptions(), DefaultColumnFamily(), ok().
 */
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

/**
 * @brief Delete Manifest.
 * @param[in] version Input parameter.
 * @return True when the operation succeeds.
 * @details Calls: getManifest(), LOG_ERROR(), lock(), getRawDB(), DefaultColumnFamily(), Put(), rocksdb::WriteOptions(), Delete().
 */
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
                std::error_code ec = {};
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
