/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_signature_manager.h                       ║
  Version:         0.0.23                                             ║
  Last Modified:   2026-02-21 19:42:56                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     82                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 03329d86d  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 31e8b8df0  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 0d722b04c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
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
};

} // namespace storage
} // namespace themis
