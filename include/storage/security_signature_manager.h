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
    explicit SecuritySignatureManager(std::shared_ptr<RocksDBWrapper> db);
    
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
        std::vector<std::string> failed_resource_ids; ///< resource_ids that failed verification
        bool success() const { return failed == 0; }
    };

    /// Verify all stored signatures by iterating over all document keys and
    /// checking each file's SHA256 hash against its stored signature.
    /// Uses RocksDBWrapper::iterateRange under the hood when RocksDB is available.
    VerifyAllResult verifyAll();
    
    /// Compute SHA256 hash of a file
    static std::string computeFileHash(const std::string& file_path);
    
    /// Normalize resource identifier (resolve relative paths, symlinks)
    static std::string normalizeResourceId(const std::string& path);
    
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
