/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_signature_manager.h                       ║
  Version:         0.0.5                                              ║
  Last Modified:   2026-02-21 10:38:19                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     81                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • b00ed23a6  2026-01-29  Add RocksDB options files and test scripts for critical, ... ║
    • ccbfee2af  2025-11-20  Add content policy and hybrid content search systems ║
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
