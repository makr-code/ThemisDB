/**
 * @file version_manager.cpp
 * @brief Version tracking and content versioning for audit trails and rollback.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 88/100
 * @note Gap Summary: total=4; TODO=0, Stub=0, Unimpl=0, Mock=0, Sim=0, Debt=1, C=0, H=1, M=3, L=0
 * @note Status: Production Ready; Version tracking and compatibility stable; advanced rollback strategies deferred
 * @note This block is auto-generated and will be overwritten.
 */
#include "content/version_manager.h"
#include <algorithm>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <unordered_map>

// SHA-256 via OpenSSL EVP (always available in this build)
#include <openssl/sha.h>

namespace themis {
namespace content {

// ---------------------------------------------------------------------------
// Static helpers
// ---------------------------------------------------------------------------

std::string VersionManager::computeHash(const std::string& data) {
    unsigned char digest[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(data.data()), data.size(), digest);

    std::ostringstream oss = {};
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        oss << std::hex << std::setw(2) << std::setfill('0')
            << static_cast<int>(digest[i]);
    }
    return oss.str();
}

std::string VersionManager::computeDelta(const std::string& old_content,
                                         const std::string& new_content) {
    // Line-level diff: produce a compact unified-diff-like delta.
    // Lines only present in old_content are prefixed with '-';
    // lines only present in new_content are prefixed with '+'.
    auto split_lines = [](const std::string& text) {
        std::vector<std::string> lines;
        std::istringstream ss(text);
        std::string line = {};
        while (std::getline(ss, line)) {
            lines.push_back(line);
        }
        return lines;
    };

    std::vector<std::string> old_lines = split_lines(old_content);
    std::vector<std::string> new_lines = split_lines(new_content);

    std::ostringstream delta = {};

    // Simple O(n*m) LCS-based diff for small files; for large content in
    // production this would be replaced by a proper diff algorithm.
    size_t oi = 0;
    size_t ni = 0;
    while (oi < old_lines.size() && ni < new_lines.size()) {
        if (old_lines[oi] == new_lines[ni]) {
            ++oi;
            ++ni;
        } else {
            // Scan ahead for the next common line (greedy LCS approximation)
            bool found = false;
            for (size_t look = ni + 1; look < new_lines.size() && look < ni + 8; ++look) {
                if (old_lines[oi] == new_lines[look]) {
                    for (size_t k = ni; k < look; ++k) {
                        delta << '+' << new_lines[k] << '\n';
                    }
                    ni = look;
                    found = true;
                    break;
                }
            }
            if (!found) {
                delta << '-' << old_lines[oi] << '\n';
                delta << '+' << new_lines[ni] << '\n';
                ++oi;
                ++ni;
            }
        }
    }
    while (oi < old_lines.size()) {
        delta << '-' << old_lines[oi++] << '\n';
    }
    while (ni < new_lines.size()) {
        delta << '+' << new_lines[ni++] << '\n';
    }

    return delta.str();
}

// ---------------------------------------------------------------------------
// Core API
// ---------------------------------------------------------------------------

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
    // content and delta left empty for metadata-only path

    versions_[content_id].push_back(version);
    return version.version_number;
}

int VersionManager::createVersionWithContent(
    const std::string& content_id,
    const std::string& content,
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
    version.content_hash = computeHash(content);
    version.size_bytes = content.size();
    version.content = content;

    // Compute delta against previous version (if one exists)
    auto it = versions_.find(content_id);
    if (it != versions_.end() && !it->second.empty()) {
        const std::string& prev = it->second.back().content;
        version.delta = computeDelta(prev, content);
    }

    versions_[content_id].push_back(version);
    return version.version_number;
}

std::optional<std::string> VersionManager::getContent(
    const std::string& content_id,
    int version_number
) const {
    auto opt = getVersion(content_id, version_number);
    if (!opt.has_value() || opt->content.empty()) {
        return std::nullopt;
    }
    return opt->content;
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

bool VersionManager::deleteVersion(const std::string& content_id, int version_number) {
    auto it = versions_.find(content_id);
    if (it == versions_.end()) {
        return false;
    }

    auto& vec = it->second;
    auto erase_it = std::find_if(vec.begin(), vec.end(),
        [version_number](const Version& v) {
            return v.version_number == version_number;
        });

    if (erase_it == vec.end()) {
        return false;
    }

    vec.erase(erase_it);
    return true;
}

} // namespace content
} // namespace themis

