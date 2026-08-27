/**
 * @file jwt_key_rotation_manager.cpp
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 85/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=0, H=0, M=3, L=0
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#include "auth/jwt_key_rotation_manager.h"

#include <openssl/crypto.h>

#include "utils/audit_logger.h"
#include "utils/logger.h"
#include "auth/auth_redaction.h"

namespace themis {
namespace auth {

JWTKeyRotationManager::JWTKeyRotationManager(JWTValidator &validator, TokenBlacklist *blacklist)
    : JWTKeyRotationManager(validator, blacklist, Config{}) {}

JWTKeyRotationManager::JWTKeyRotationManager(JWTValidator &validator, TokenBlacklist *blacklist, const Config &config)
    : validator_(validator), blacklist_(blacklist), config_(config) {}

JWTKeyRotationManager::~JWTKeyRotationManager() {
    // Zero all key identifier strings before the map is destroyed.
    // Kid strings may carry information about key type/algorithm (e.g.
    // "rsa-2048-2024-03") which would be sensitive if leaked in a core dump.
    // Note: this zeroes the *current* allocation of each std::string; if the
    // unordered_map ever rehashed and triggered internal string moves, prior
    // memory locations would not be covered. For full lifecycle protection,
    // migrate JWKKeyInfo::kid to SecureString in a future refactor.
    for (auto &[kid_str, info] : keys_) {
        if (!info.kid.empty()) {
            char *buf = &info.kid[0];
            OPENSSL_cleanse(buf, info.kid.size());
        }
    }
}

void JWTKeyRotationManager::rotateActiveKey(const std::string &new_kid, std::optional<std::chrono::seconds> max_age) {
    utils::AuditLogger *logger = nullptr;
    uint64_t rotation_num      = 0;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        // Enforce max_keys resource limit (new key will be added)
        if (config_.max_keys > 0 && keys_.size() >= config_.max_keys && keys_.find(new_kid) == keys_.end()) {
            if (audit_logger_) {
                nlohmann::json meta;
                meta["new_kid"]   = new_kid;
                meta["max_keys"]  = config_.max_keys;
                meta["reason"]    = "max_keys_limit_reached";
                audit_logger_->logSecurityEvent(utils::SecurityEventType::KEY_ROTATION_FAILED,
                                                "jwt_key_rotation_manager",
                                                "jwt_key/" + new_kid, meta);
            }
            throw std::length_error("JWTKeyRotationManager: max_keys limit (" + std::to_string(config_.max_keys)
                                    + ") reached");
        }

        // Demote any currently ACTIVE key to PASSIVE
        for (auto &[kid, info] : keys_) {
            if (info.status == JWKKeyInfo::Status::ACTIVE) {
                info.status     = JWKKeyInfo::Status::PASSIVE;
                info.demoted_at = std::chrono::system_clock::now();
                THEMIS_INFO("JWTKeyRotation: key '{}' demoted to PASSIVE", redact(kid));
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
        rotation_num = rotation_count_;
        logger       = audit_logger_;
        THEMIS_INFO("JWTKeyRotation: key '{}' is now ACTIVE (rotation #{})", redact(new_kid), rotation_num);
    }

    if (logger) {
        nlohmann::json meta;
        meta["new_kid"]  = new_kid;
        meta["rotation"] = rotation_num;
        logger->logSecurityEvent(utils::SecurityEventType::KEY_ROTATED, "jwt_key_rotation_manager",
                                 "jwt_key/" + new_kid, meta);
    }
}

bool JWTKeyRotationManager::revokeKey(const std::string &kid) {
    utils::AuditLogger *logger = nullptr;
    uint64_t revocation_num    = 0;
    bool revoked               = false;
    {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = keys_.find(kid);
        if (it == keys_.end()) {
            THEMIS_WARN("JWTKeyRotation: revokeKey – unknown kid '{}'", redact(kid));
            if (audit_logger_) {
                nlohmann::json meta;
                meta["kid"]    = kid;
                meta["reason"] = "unknown_kid";
                audit_logger_->logSecurityEvent(utils::SecurityEventType::KEY_REVOCATION_FAILED,
                                                "jwt_key_rotation_manager",
                                                "jwt_key/" + kid, meta);
            }
            return false;
        }

        if (it->second.status == JWKKeyInfo::Status::REVOKED) {
            return true; // Already revoked
        }

        it->second.status = JWKKeyInfo::Status::REVOKED;
        revocation_count_++;
        revocation_num = revocation_count_;
        revoked        = true;

        // Add to JWTValidator denylist so tokens signed with this kid are rejected
        validator_.revokeKid(kid);
        logger = audit_logger_;

        THEMIS_WARN("JWTKeyRotation: key '{}' REVOKED (revocation #{})", redact(kid), revocation_num);
    }

    if (revoked && logger) {
        nlohmann::json meta;
        meta["kid"]        = kid;
        meta["revocation"] = revocation_num;
        logger->logSecurityEvent(utils::SecurityEventType::KEY_DELETED, "jwt_key_rotation_manager", "jwt_key/" + kid,
                                 meta);
    }
    return revoked;
}

bool JWTKeyRotationManager::reactivateKey(const std::string &kid) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = keys_.find(kid);
    if (it == keys_.end()) {
        return false;
    }
    if (it->second.status == JWKKeyInfo::Status::REVOKED) {
        THEMIS_WARN("JWTKeyRotation: cannot reactivate REVOKED key '{}'", redact(kid));
        return false;
    }

    // Demote any currently ACTIVE key to PASSIVE first
    for (auto &[k, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE && k != kid) {
            info.status     = JWKKeyInfo::Status::PASSIVE;
            info.demoted_at = std::chrono::system_clock::now();
        }
    }

    it->second.status       = JWKKeyInfo::Status::ACTIVE;
    it->second.activated_at = std::chrono::system_clock::now();
    THEMIS_INFO("JWTKeyRotation: key '{}' reactivated", redact(kid));
    return true;
}

bool JWTKeyRotationManager::isRotationDue() const {
    std::lock_guard<std::mutex> lock(mutex_);

    for (const auto &[kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE) {
            return info.isExpired();
        }
    }
    // No active key → rotation is needed
    return true;
}

void JWTKeyRotationManager::checkAndRotate() {
    if (!config_.auto_revoke_expired_passive) {
        return;
    }

    std::lock_guard<std::mutex> lock(mutex_);

    auto now = std::chrono::system_clock::now();
    std::vector<std::string> to_revoke;

    for (const auto &[kid, info] : keys_) {
        if (info.status != JWKKeyInfo::Status::PASSIVE) {
            continue;
        }
        auto age = std::chrono::duration_cast<std::chrono::seconds>(now - info.demoted_at);
        if (age > config_.passive_grace_period) {
            to_revoke.push_back(kid);
        }
    }

    // Revoke outside the range-for (modifies the map via revokeKey)
    // Note: we already hold the lock, so we call the validator directly
    for (const auto &kid : to_revoke) {
        keys_[kid].status = JWKKeyInfo::Status::REVOKED;
        revocation_count_++;
        validator_.revokeKid(kid);
        THEMIS_WARN("JWTKeyRotation: passive key '{}' auto-revoked after grace period", redact(kid));
    }
}

std::string JWTKeyRotationManager::activeKeyId() const {
    std::lock_guard<std::mutex> lock(mutex_);
    for (const auto &[kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::ACTIVE) {
            return kid;
        }
    }
    return {};
}

std::vector<std::string> JWTKeyRotationManager::passiveKeyIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto &[kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::PASSIVE) {
            result.push_back(kid);
        }
    }
    return result;
}

std::vector<std::string> JWTKeyRotationManager::revokedKeyIds() const {
    std::lock_guard<std::mutex> lock(mutex_);
    std::vector<std::string> result;
    for (const auto &[kid, info] : keys_) {
        if (info.status == JWKKeyInfo::Status::REVOKED) {
            result.push_back(kid);
        }
    }
    return result;
}

std::optional<JWKKeyInfo> JWTKeyRotationManager::getKeyInfo(const std::string &kid) const {
    std::lock_guard<std::mutex> lock(mutex_);
    auto it = keys_.find(kid);
    if (it == keys_.end()) {
        return std::nullopt;
    }
    return it->second;
}

JWTKeyRotationManager::Statistics JWTKeyRotationManager::getStatistics() const {
    std::lock_guard<std::mutex> lock(mutex_);
    Statistics s;
    s.total_keys        = keys_.size();
    s.total_rotations   = rotation_count_;
    s.total_revocations = revocation_count_;
    for (const auto &[kid, info] : keys_) {
        switch (info.status) {
            case JWKKeyInfo::Status::ACTIVE:
                s.active_keys++;
                break;
            case JWKKeyInfo::Status::PASSIVE:
                s.passive_keys++;
                break;
            case JWKKeyInfo::Status::REVOKED:
                s.revoked_keys++;
                break;
        }
    }
    return s;
}

} // namespace auth
} // namespace themis
