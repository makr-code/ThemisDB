/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            version_manager.h                                  ║
  Version:         0.0.7                                              ║
  Last Modified:   2026-02-21 11:48:21                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     80                                             ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • bdb82d096  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <cstdint>
#include <unordered_map>

namespace themis {
namespace content {

/// Content Version Manager
/// Provides simple versioning for content objects
/// Stores version history with timestamps and metadata
class VersionManager {
public:
    struct Version {
        int version_number;
        int64_t timestamp;
        std::string author;
        std::string comment;
        std::string content_hash;  // SHA-256 of content
        size_t size_bytes;
    };

    /// Create new version for content
    /// Returns version number
    int createVersion(
        const std::string& content_id,
        const std::string& content_hash,
        size_t size_bytes,
        const std::string& author = "",
        const std::string& comment = ""
    );

    /// Get version history for content
    std::vector<Version> getVersionHistory(const std::string& content_id) const;

    /// Get specific version info
    std::optional<Version> getVersion(const std::string& content_id, int version_number) const;

    /// Get latest version number
    int getLatestVersion(const std::string& content_id) const;

    /// Check if content has versions
    bool hasVersions(const std::string& content_id) const;

private:
    // In-memory storage (simplified - in production would use RocksDB)
    std::unordered_map<std::string, std::vector<Version>> versions_;
};

} // namespace content
} // namespace themis
