/**
 * @file version_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
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
        int version_number = 0;
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
