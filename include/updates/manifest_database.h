/**
 * @file manifest_database.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "updates/release_manifest.h"
#include "storage/rocksdb_wrapper.h"
#include "acceleration/plugin_security.h"
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include <mutex>

namespace themis {
namespace updates {

/**
 * @brief RocksDB-backed database for release manifests
 * 
 * Column Families:
 * - release_manifests: version -> ReleaseManifest (JSON)
 * - file_registry: path:version -> ReleaseFile (JSON)
 * - signature_cache: hash -> Signature verification result
 * - download_cache: version:file -> local path
 */
class ManifestDatabase {
public:
    /**
     * @brief Construct manifest database with storage backend
     * @param storage RocksDB wrapper
     * @param verifier Plugin security verifier for signature verification
     */
    ManifestDatabase(
        std::shared_ptr<RocksDBWrapper> storage,
        std::shared_ptr<acceleration::PluginSecurityVerifier> verifier
    );
    
    ~ManifestDatabase();
    
    /**
     * @brief Store manifest in database
     * @param manifest Release manifest to store
     * @return true if successful
     */
    bool storeManifest(const ReleaseManifest& manifest);
    
    /**
     * @brief Retrieve manifest by version
     * @param version Version string (e.g., "1.2.3")
     * @return Manifest if found
     */
    std::optional<ReleaseManifest> getManifest(const std::string& version);
    
    /**
     * @brief Get latest manifest (by version number)
     * @return Latest manifest if any exist
     */
    std::optional<ReleaseManifest> getLatestManifest();
    
    /**
     * @brief List all available versions
     * @return Vector of version strings, sorted ascending
     */
    std::vector<std::string> listVersions() const;
    
    /**
     * @brief Verify manifest integrity and authenticity before an update is trusted.
     * @param manifest Manifest whose calculated hash and detached signature are checked.
     *        A signing certificate and a configured verifier are required.
     * @return True only when the hash matches, the certificate chains to the system
     *         trust store, its issuer is allowed by the verifier policy, and the
     *         signature is valid. Returns false for missing trust material,
     *         untrusted or expired certificates, malformed signatures, or any
     *         verification error. Cached results are not used to authorize manifests.
     */
    bool verifyManifest(const ReleaseManifest& manifest);
    
    /**
     * @brief Check if file exists and is valid for a version
     * @param path File path
     * @param version Version string
     * @return true if file exists and hash matches
     */
    bool verifyFile(const std::string& path, const std::string& version);
    
    /**
     * @brief Get file from registry
     * @param path File path
     * @param version Version string
     * @return ReleaseFile if found
     */
    std::optional<ReleaseFile> getFile(
        const std::string& path,
        const std::string& version
    );
    
    /**
     * @brief Store file in registry
     * @param file ReleaseFile to store
     * @param version Version string
     * @return true if successful
     */
    bool storeFile(const ReleaseFile& file, const std::string& version);
    
    /**
     * @brief Store a signature-verification cache record for diagnostics or callers.
     * @param hash SHA-256 hash used as the cache key.
     * @param verified Verification result to store.
     * @param certificate Certificate associated with the record.
     * @note This cache is not consulted by verifyManifest() and cannot authorize an
     *       update, because a cached result is not bound to the current signature
     *       and verifier policy.
     */
    void cacheSignatureVerification(
        const std::string& hash,
        bool verified,
        const std::string& certificate
    );
    
    /**
     * @brief Read a signature-verification cache record.
     * @param hash SHA-256 hash used as the cache key.
     * @return The stored result if present; this result is not an authorization
     *         decision and is never used by verifyManifest().
     */
    std::optional<bool> getCachedSignatureVerification(const std::string& hash);
    
    /**
     * @brief Store download cache entry
     * @param version Version string
     * @param filename Filename
     * @param local_path Local file path
     */
    void cacheDownload(
        const std::string& version,
        const std::string& filename,
        const std::string& local_path
    );
    
    /**
     * @brief Get cached download path
     * @param version Version string
     * @param filename Filename
     * @return Local path if cached
     */
    std::optional<std::string> getCachedDownload(
        const std::string& version,
        const std::string& filename
    );
    
    /**
     * @brief Delete manifest and associated files
     * @param version Version to delete
     * @return true if successful
     */
    bool deleteManifest(const std::string& version);
    
private:
    std::shared_ptr<RocksDBWrapper> storage_;
    std::shared_ptr<acceleration::PluginSecurityVerifier> verifier_;
    
    // Thread synchronization for column family handle access (CRITICAL: data_race fix)
    mutable std::mutex cf_mutex_;
    
    // Column family handles
    rocksdb::ColumnFamilyHandle* cf_manifests_ = nullptr;
    rocksdb::ColumnFamilyHandle* cf_files_ = nullptr;
    rocksdb::ColumnFamilyHandle* cf_signatures_ = nullptr;
    rocksdb::ColumnFamilyHandle* cf_cache_ = nullptr;
    
    /**
     * @brief Initialize column families
     */
    void initializeColumnFamilies();
};

} // namespace updates
} // namespace themis
