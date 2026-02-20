#include "auth/jwt_key_rotation_manager.h"
#include "utils/logger.h"

namespace themis {
namespace auth {

JWTKeyRotationManager::JWTKeyRotationManager(
    JWTValidator& validator,
    TokenBlacklist* blacklist,
    const Config& config)
    : validator_(validator)
    , blacklist_(blacklist)
    , config_(config)
{}

void JWTKeyRotationManager::rotateActiveKey(
    const std::string& new_kid,
    std::optional<std::chrono::seconds> max_age)
{
    std::lock_guard<std::mutex> lock(mutex_);

    // Demote any currently ACTIVE key to PASSIVE
    for (auto& [kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE) {
            info.status     = JWKKeyInfo::Status::PASSIVE;
            info.demoted_at = std::chrono::system_clock::now();
            THEMIS_INFO("JWTKeyRotation: key '{}' demoted to PASSIVE", kid);
        }
    }

    // Register and activate the new key
    JWKKeyInfo new_info;
    new_info.kid          = new_kid;
    new_info.status       = JWKKeyInfo::Status::ACTIVE;
    new_info.activated_at = std::chrono::system_clock::now();
    new_info.max_age      = max_age.value_or(config_.max_key_age);
    keys_[new_kid]        = new_info;

    rotation_count_++;
    THEMIS_INFO("JWTKeyRotation: key '{}' is now ACTIVE (rotation #{})",
                new_kid, rotation_count_);
}

bool JWTKeyRotationManager::revokeKey(const std::string& kid) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = keys_.find(kid);
    if (it == keys_.end()) {
        THEMIS_WARN("JWTKeyRotation: revokeKey – unknown kid '{}'", kid);
        return false;
    }

    if (it->second.status == JWKKeyInfo::Status::REVOKED) {
        return true;  // Already revoked
    }

    it->second.status = JWKKeyInfo::Status::REVOKED;
    revocation_count_++;

    // Add to JWTValidator denylist so tokens signed with this kid are rejected
    validator_.revokeKid(kid);

    THEMIS_WARN("JWTKeyRotation: key '{}' REVOKED (revocation #{})",
                kid, revocation_count_);
    return true;
}

bool JWTKeyRotationManager::reactivateKey(const std::string& kid) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = keys_.find(kid);
    if (it == keys_.end()) return false;
    if (it->second.status == JWKKeyInfo::Status::REVOKED) {
        THEMIS_WARN("JWTKeyRotation: cannot reactivate REVOKED key '{}'", kid);
        return false;
    }

    // Demote any currently ACTIVE key to PASSIVE first
    for (auto& [k, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE && k != kid) {
            info.status     = JWKKeyInfo::Status::PASSIVE;
            info.demoted_at = std::chrono::system_clock::now();
        }
    }

    it->second.status       = JWKKeyInfo::Status::ACTIVE;
    it->second.activated_at = std::chrono::system_clock::now();
    THEMIS_INFO("JWTKeyRotation: key '{}' reactivated", kid);
    return true;
}

bool JWTKeyRotationManager::isRotationDue() const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto& [kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE) {
            return info.isExpired();
        }
    }
    // No active key → rotation is needed
    return true;
}

void JWTKeyRotationManager::checkAndRotate() {
    if (!config_.auto_revoke_expired_passive) return;

    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_revoke;

    for (const auto& [kid, info] : keys_) {
        if (info.status != JWKKeyInfo::Status::PASSIVE) continue;
        auto age = std::chrono::duration_cast<std::chrono::seconds>(
            now - info.demoted_at);
        if (age > config_.passive_grace_period) {
            to_revoke.push_back(kid);
        }
    }

    // Revoke outside the range-for (modifies the map via revokeKey)
    // Note: we already hold the lock, so we call the validator directly
    for (const auto& kid : to_revoke) {
        keys_[kid].status = JWKKeyInfo::Status::REVOKED;
        revocation_count_++;
        validator_.revokeKid(kid);
        THEMIS_WARN("JWTKeyRotation: passive key '{}' auto-revoked after grace period",
                    kid);
    }
}

std::string JWTKeyRotationManager::activeKeyId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto& [kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE) return kid;
    }
    return {};
}

std::vector<std::string> JWTKeyRotationManager::passiveKeyIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::PASSIVE) result.push_back(kid);
    }
    return result;
}

std::vector<std::string> JWTKeyRotationManager::revokedKeyIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto& [kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::REVOKED) result.push_back(kid);
    }
    return result;
}

std::optional<JWKKeyInfo> JWTKeyRotationManager::getKeyInfo(
    const std::string& kid) const
{
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = keys_.find(kid);
    if (it == keys_.end()) return std::nullopt;
    return it->second;
}

JWTKeyRotationManager::Statistics JWTKeyRotationManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Statistics s;
    s.total_keys         = keys_.size();
    s.total_rotations    = rotation_count_;
    s.total_revocations  = revocation_count_;
    for (const auto& [kid, info] : keys_) {
        switch (info.status) {
            case JWKKeyInfo::Status::ACTIVE:  s.active_keys++;  break;
            case JWKKeyInfo::Status::PASSIVE: s.passive_keys++; break;
            case JWKKeyInfo::Status::REVOKED: s.revoked_keys++; break;
        }
    }
    return s;
}

} // namespace auth
} // namespace themis
