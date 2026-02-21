/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            security_signature.h                               ║
  Version:         0.0.4                                              ║
  Last Modified:   2026-02-21 08:35:28                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     54                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ccbfee2af  2025-11-20  Add content policy and hybrid content search systems ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <nlohmann/json.hpp>

namespace themis {
namespace storage {

/// Security signature for critical resources (config files, schemas, etc.)
/// Stored externally in RocksDB to avoid self-reference paradox
struct SecuritySignature {
    std::string resource_id;     ///< Normalized resource identifier (e.g., "config/mime_types.yaml")
    std::string hash;             ///< SHA256 hash (64 hex chars)
    std::string algorithm;        ///< Hash algorithm (currently "sha256")
    uint64_t created_at;          ///< Unix timestamp (seconds since epoch)
    std::string created_by;       ///< User or system identifier (optional)
    std::string comment;          ///< Human-readable description (optional)
    
    /// Serialize to JSON
    nlohmann::json toJson() const;
    
    /// Deserialize from JSON
    static std::optional<SecuritySignature> fromJson(const nlohmann::json& j);
    
    /// Serialize to binary (for RocksDB storage)
    std::string serialize() const;
    
    /// Deserialize from binary
    static std::optional<SecuritySignature> deserialize(const std::string& data);
};

} // namespace storage
} // namespace themis
