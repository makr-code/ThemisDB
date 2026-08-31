/**
 * @file security_signature_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "storage/security_signature.h"
#include "storage/rocksdb_wrapper.h"
#include <string>
#include <vector>
#include <optional>
#include <memory>
#include <unordered_map>

namespace themis {
namespace storage {

/// Manager for security signatures stored in RocksDB
/// Provides CRUD operations and file verification capabilities
class SecuritySignatureManager {
public:
    /**
     * @brief Construction-time options for storage-backed signature verification.
     */
    struct Options {
        /**
         * @brief Allow an in-memory fallback store when RocksDB is unavailable.
         *
         * This is intended for focused tests or explicitly ephemeral workflows.
         * Production callers should keep the default fail-closed behavior.
         */
        bool allow_in_memory_fallback = false;
    };

    /**
     * @brief Create a signature manager backed by RocksDB.
     *
     * @param db Persistent RocksDB wrapper. When null, the manager stays
     *        unavailable and all mutating operations fail closed.
     */
    explicit SecuritySignatureManager(std::shared_ptr<RocksDBWrapper> db);
    /**
     * @brief Create a signature manager backed by RocksDB with explicit fallback policy.
     *
     * @param db      Persistent RocksDB wrapper. When null and
     *                `options.allow_in_memory_fallback` is `false`, the manager stays
     *                unavailable and all mutating operations fail closed.
     * @param options Construction-time fallback policy.
     */
    explicit SecuritySignatureManager(std::shared_ptr<RocksDBWrapper> db,
                                      Options options);
    
    // CRUD Operations
    
    /// Store or update a security signature
    bool storeSignature(const SecuritySignature& sig);
    
    /// Retrieve a signature by resource_id
    std::optional<SecuritySignature> getSignature(const std::string& resource_id);
    
    /// Delete a signature by resource_id
    bool deleteSignature(const std::string& resource_id);
    
    /// List all stored signatures
    std::vector<SecuritySignature> listAllSignatures();
    
    // Verification Operations
    
    /// Verify a file against stored signature
    /// Returns true if hash matches, false if mismatch or signature missing
    bool verifyFile(const std::string& file_path, const std::string& resource_id);

    /// Result returned by verifyAll()
    struct VerifyAllResult {
        int total = 0;           ///< Total signatures scanned
        int verified = 0;        ///< Signatures whose files matched their stored hashes
        int failed = 0;          ///< Signatures with hash mismatches or missing files
        bool backend_available = true; ///< False when no persistent backend exists and fallback was not enabled
        bool used_fallback_memory_store = false; ///< True when verification ran against the explicit in-memory fallback
        std::string error_message; ///< Operator-facing reason when verification cannot execute
        std::vector<std::string> failed_resource_ids; ///< resource_ids that failed verification
        bool success() const {
            return backend_available && error_message.empty() && failed == 0;
        }
    };

    /// Verify all stored signatures by iterating over all document keys and
    /// checking each file's SHA256 hash against its stored signature.
    /// Uses RocksDBWrapper::iterateRange under the hood when RocksDB is available.
    VerifyAllResult verifyAll();
    
    /// Compute SHA256 hash of a file
    static std::string computeFileHash(const std::string& file_path);
    
    /// Normalize resource identifier (resolve relative paths, symlinks)
    static std::string normalizeResourceId(const std::string& path);

    /// Return whether the manager is currently using the explicit in-memory fallback store.
    [[nodiscard]] bool isUsingFallbackMemoryStore() const noexcept {
        return use_fallback_memory_store_;
    }

    /// Return whether a persistent RocksDB backend is available.
    [[nodiscard]] bool hasPersistentBackend() const noexcept {
        return static_cast<bool>(db_);
    }
    
private:
    std::shared_ptr<RocksDBWrapper> db_;
    bool use_fallback_memory_store_ = false;                 // In-memory fallback when RocksDB is unavailable
    std::unordered_map<std::string, std::string> mem_store_; // Simple map for tests/in-memory mode
    static constexpr const char* KEY_PREFIX = "security_sig:";
    
    std::string makeKey(const std::string& resource_id) const;

    /// Returns the [start_key, end_key) range that covers all keys with KEY_PREFIX.
    /// end_key is KEY_PREFIX with the last byte incremented (e.g. "security_sig;" when
    /// KEY_PREFIX == "security_sig:").
    static std::pair<std::string, std::string> makePrefixRange();
};

} // namespace storage
} // namespace themis
