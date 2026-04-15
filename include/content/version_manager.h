/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            version_manager.h                                  ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:09:48                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     115                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 2a1fb04231  2026-03-03  Merge branch 'develop' into copilot/audit-src-module-docu... ║
    • ff318d29ab  2026-02-28  Implement content versioning with delta storage (Issue #1... ║
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
/// Stores version history with timestamps, metadata, content snapshots, and
/// delta diffs for efficient storage of incremental changes.
class VersionManager {
public:
    struct Version {
        int version_number;
        int64_t timestamp;
        std::string author;
        std::string comment;
        std::string content_hash;  // SHA-256 of content
        size_t size_bytes;
        std::string content;       // Full content snapshot
        std::string delta;         // Delta (diff) from previous version; empty for v1
    };

    /// Create new version for content (metadata-only, caller supplies hash)
    /// Returns version number
    int createVersion(
        const std::string& content_id,
        const std::string& content_hash,
        size_t size_bytes,
        const std::string& author = "",
        const std::string& comment = ""
    );

    /// Create new version with actual content.
    /// Computes SHA-256 hash, stores the full snapshot, and records a
    /// line-level delta against the immediately preceding version.
    /// Returns version number.
    int createVersionWithContent(
        const std::string& content_id,
        const std::string& content,
        const std::string& author = "",
        const std::string& comment = ""
    );

    /// Retrieve the stored content for a specific version.
    /// Returns std::nullopt when the version does not exist or was created
    /// via createVersion() (metadata-only path).
    std::optional<std::string> getContent(
        const std::string& content_id,
        int version_number
    ) const;

    /// Get version history for content
    std::vector<Version> getVersionHistory(const std::string& content_id) const;

    /// Get specific version info
    std::optional<Version> getVersion(const std::string& content_id, int version_number) const;

    /// Get latest version number
    int getLatestVersion(const std::string& content_id) const;

    /// Check if content has versions
    bool hasVersions(const std::string& content_id) const;

    /// Delete a specific version.  Returns true if the version existed and
    /// was removed, false otherwise.
    bool deleteVersion(const std::string& content_id, int version_number);

    // --- Static helpers (exposed for testability) ---

    /// Compute SHA-256 hex digest of the given data.
    static std::string computeHash(const std::string& data);

    /// Compute a simple line-level delta between old_content and new_content.
    /// The returned string encodes added/removed lines in a unified-diff-like
    /// format and can be stored as the Version::delta field.
    static std::string computeDelta(const std::string& old_content,
                                    const std::string& new_content);

private:
    // In-memory storage (simplified - in production would use RocksDB)
    std::unordered_map<std::string, std::vector<Version>> versions_;
};

} // namespace content
} // namespace themis
