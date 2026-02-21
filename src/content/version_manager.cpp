/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            version_manager.cpp                                ║
  Version:         0.0.6                                              ║
  Last Modified:   2026-02-21 11:00:29                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   95.0/100                                       ║
    • Total Lines:     107                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 7f2db8dcb  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 84d1fada6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 2563a40d8  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • f0e1e982c  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • ccbfee2af  2025-11-20  Add content policy and hybrid content search systems ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#include "content/version_manager.h"
#include <chrono>
#include <unordered_map>

namespace themis {
namespace content {

int VersionManager::createVersion(
    const std::string& content_id,
    const std::string& content_hash,
    size_t size_bytes,
    const std::string& author,
    const std::string& comment
) {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        now.time_since_epoch()
    ).count();

    Version version;
    version.version_number = getLatestVersion(content_id) + 1;
    version.timestamp = timestamp;
    version.author = author;
    version.comment = comment;
    version.content_hash = content_hash;
    version.size_bytes = size_bytes;

    versions_[content_id].push_back(version);
    return version.version_number;
}

std::vector<VersionManager::Version> VersionManager::getVersionHistory(
    const std::string& content_id
) const {
    auto it = versions_.find(content_id);
    if (it == versions_.end()) {
        return {};
    }
    return it->second;
}

std::optional<VersionManager::Version> VersionManager::getVersion(
    const std::string& content_id,
    int version_number
) const {
    auto it = versions_.find(content_id);
    if (it == versions_.end()) {
        return std::nullopt;
    }

    for (const auto& v : it->second) {
        if (v.version_number == version_number) {
            return v;
        }
    }

    return std::nullopt;
}

int VersionManager::getLatestVersion(const std::string& content_id) const {
    auto it = versions_.find(content_id);
    if (it == versions_.end() || it->second.empty()) {
        return 0;
    }

    int latest = 0;
    for (const auto& v : it->second) {
        if (v.version_number > latest) {
            latest = v.version_number;
        }
    }

    return latest;
}

bool VersionManager::hasVersions(const std::string& content_id) const {
    auto it = versions_.find(content_id);
    return it != versions_.end() && !it->second.empty();
}

} // namespace content
} // namespace themis
