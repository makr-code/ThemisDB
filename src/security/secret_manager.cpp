/**
 * @file secret_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=1, M=5, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "security/secret_manager.h"

#include <stdexcept>

namespace themis {
namespace security {

// ─────────────────────────────────────────────────────────────────────────────
// Constructor
// ─────────────────────────────────────────────────────────────────────────────

SecretManager::SecretManager(RotationPolicy policy)
    : policy_(policy)
{}

// ─────────────────────────────────────────────────────────────────────────────
// Private helpers
// ─────────────────────────────────────────────────────────────────────────────

SecretManager::SecretVersion&
SecretManager::findVersion(SecretEntry& entry, uint32_t version) {
    for (auto& v : entry.versions) {
        if (v.version == version) {
          return v;
        }
    }
    throw std::out_of_range("version not found");
}

const SecretManager::SecretVersion*
SecretManager::findVersionConst(const SecretEntry& entry,
                                uint32_t version) const {
    for (const auto& v : entry.versions) {
        if (v.version == version) {
          return &v;
        }
    }
    return nullptr;
}

/// Returns true if (now - created_at) >= threshold.
static bool isAgeExceeded(std::chrono::system_clock::time_point created_at,
                           std::chrono::seconds threshold) {
    return (std::chrono::system_clock::now() - created_at) >= threshold;
}

// ─────────────────────────────────────────────────────────────────────────────
// storeSecret
// ─────────────────────────────────────────────────────────────────────────────

uint32_t SecretManager::storeSecret(const std::string& name,
                                    const std::string& value,
                                    const std::string& created_by,
                                    const std::string& description) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (secrets_.count(name)) {
        throw std::invalid_argument(
            "SecretManager: secret '" + name + "' already exists; "
            "use rotateSecret() to change its value");
    }

    if (policy_.max_secrets > 0 && static_cast<int>(secrets_.size()) >= policy_.max_secrets) {
        throw std::length_error(
            "SecretManager: max_secrets limit (" +
            std::to_string(policy_.max_secrets) + ") reached");
    }

    SecretEntry entry;
    entry.name = name;
    entry.next_version = 1;

    SecretVersion ver;
    ver.version    = entry.next_version++;
    ver.value      = value;
    ver.status     = SecretStatus::ACTIVE;
    ver.created_at = std::chrono::system_clock::now();
    ver.expires_at = std::chrono::system_clock::time_point{};  // never
    ver.created_by = created_by;
    ver.description = description;
    entry.versions.push_back(std::move(ver));

    uint32_t ver_num = entry.versions.back().version;
    secrets_.emplace(name, std::move(entry));
    return ver_num;
}

// ─────────────────────────────────────────────────────────────────────────────
// getSecret
// ─────────────────────────────────────────────────────────────────────────────

std::optional<SecretManager::SecretVersion>
SecretManager::getSecret(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = secrets_.find(name);
    if (it == secrets_.end()) {
      return std::nullopt;
    }

    for (auto rit = it->second.versions.rbegin();
         rit != it->second.versions.rend(); ++rit) {
        if (rit->status == SecretStatus::ACTIVE) {
            return *rit;
        }
    }
    return std::nullopt;
}

// ─────────────────────────────────────────────────────────────────────────────
// getSecretVersion
// ─────────────────────────────────────────────────────────────────────────────

std::optional<SecretManager::SecretVersion>
SecretManager::getSecretVersion(const std::string& name,
                                uint32_t version) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = secrets_.find(name);
    if (it == secrets_.end()) {
      return std::nullopt;
    }

    const auto* ver = findVersionConst(it->second, version);
    if (!ver) {
      return std::nullopt;
    }
    return *ver;
}

// ─────────────────────────────────────────────────────────────────────────────
// rotateSecret
// ─────────────────────────────────────────────────────────────────────────────

uint32_t SecretManager::rotateSecret(const std::string& name,
                                     const std::string& new_value,
                                     const std::string& created_by) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = secrets_.find(name);
    if (it == secrets_.end()) {
        throw std::invalid_argument(
            "SecretManager: secret '" + name + "' not found; "
            "use storeSecret() to create it first");
    }

    SecretEntry& entry = it->second;

    if (policy_.max_versions_per_secret > 0 &&
            entry.versions.size() >= policy_.max_versions_per_secret) {
        throw std::length_error(
            "SecretManager: max_versions_per_secret limit (" +
            std::to_string(policy_.max_versions_per_secret) +
            ") reached for secret '" + name + "'");
    }

    // Demote current ACTIVE → RETIRING
    auto now = std::chrono::system_clock::now();
    for (auto& v : entry.versions) {
        if (v.status == SecretStatus::ACTIVE) {
            v.status = SecretStatus::RETIRING;
        }
    }

    // Create new ACTIVE version
    SecretVersion ver;
    ver.version    = entry.next_version++;
    ver.value      = new_value;
    ver.status     = SecretStatus::ACTIVE;
    ver.created_at = now;
    ver.expires_at = std::chrono::system_clock::time_point{};
    ver.created_by = created_by;
    entry.versions.push_back(std::move(ver));

    ++entry.rotation_count;
    ++total_rotations_;
    return entry.versions.back().version;
}

// ─────────────────────────────────────────────────────────────────────────────
// revokeVersion
// ─────────────────────────────────────────────────────────────────────────────

bool SecretManager::revokeVersion(const std::string& name, uint32_t version) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = secrets_.find(name);
    if (it == secrets_.end()) {
      return false;
    }

    try {
        SecretVersion& v = findVersion(it->second, version);
        if (v.status == SecretStatus::REVOKED) {
          return false;
        }
        v.status = SecretStatus::REVOKED;
        return true;
    } catch (const std::out_of_range&) {
        return false;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// deleteSecret
// ─────────────────────────────────────────────────────────────────────────────

bool SecretManager::deleteSecret(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex_);
    return secrets_.erase(name) > 0;
}

// ─────────────────────────────────────────────────────────────────────────────
// listVersions
// ─────────────────────────────────────────────────────────────────────────────

std::vector<SecretManager::VersionInfo>
SecretManager::listVersions(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = secrets_.find(name);
    if (it == secrets_.end()) return {};

    std::vector<VersionInfo> result = {};

    result.reserve(it-> static_cast<int>(second.versions.size()));
    for (const auto& v : it->second.versions) {
        VersionInfo info;
        info.version    = v.version;
        info.status     = v.status;
        info.created_at = v.created_at;
        info.created_by = v.created_by;
        info.description = v.description;
        result.push_back(info);
    }
    return result;
}

// ─────────────────────────────────────────────────────────────────────────────
// listSecrets
// ─────────────────────────────────────────────────────────────────────────────

std::vector<std::string> SecretManager::listSecrets() const {
    std::lock_guard<std::mutex> lock(mutex_);

    std::vector<std::string> names = {};

    names.reserve(secrets_.size());
    for (const auto& kv : secrets_) {
        names.push_back(kv.first);
    }
    return names;
}

// ─────────────────────────────────────────────────────────────────────────────
// isRotationDue
// ─────────────────────────────────────────────────────────────────────────────

bool SecretManager::isRotationDue(const std::string& name) const {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = secrets_.find(name);
    if (it == secrets_.end()) {
      return false;
    }

    for (auto rit = it->second.versions.rbegin();
         rit != it->second.versions.rend(); ++rit) {
        if (rit->status == SecretStatus::ACTIVE) {
            return isAgeExceeded(rit->created_at, policy_.max_age);
        }
    }
    return false;  // no active version found
}

// ─────────────────────────────────────────────────────────────────────────────
// checkAndRevoke
// ─────────────────────────────────────────────────────────────────────────────

void SecretManager::checkAndRevoke() {
    if (!policy_.auto_revoke_expired_retiring) {
      return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    for (auto& kv : secrets_) {
        for (auto& v : kv.second.versions) {
            if (v.status != SecretStatus::RETIRING) {
              continue;
            }
            if (isAgeExceeded(v.created_at, policy_.retiring_grace_period)) {
                v.status = SecretStatus::REVOKED;
            }
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
// getStatistics
// ─────────────────────────────────────────────────────────────────────────────

SecretManager::Statistics SecretManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);

    Statistics s;
    s.total_secrets   = secrets_.size();
    s.total_rotations = total_rotations_;

    for (const auto& kv : secrets_) {
        for (const auto& v : kv.second.versions) {
            switch (v.status) {
                case SecretStatus::ACTIVE:   ++s.active_versions;   break;
                case SecretStatus::RETIRING: ++s.retiring_versions; break;
                case SecretStatus::REVOKED:  ++s.revoked_versions;  break;
            }
        }
    }
    return s;
}

}  // namespace security
}  // namespace themis
