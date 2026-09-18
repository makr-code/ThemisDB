/**
 * @file jwt_key_rotation_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/jwt_validator.h"
#include "auth/token_blacklist.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <mutex>
#include <chrono>
#include <functional>
#include <memory>
#include <optional>

namespace themis {
// Forward declaration to avoid pulling the full AuditLogger header here
namespace utils { class AuditLogger; }
namespace auth {

struct JWKKeyInfo {
    std::string kid = {};

    enum class Status {
        ACTIVE,    ///< Current signing key – issued tokens have this kid
        PASSIVE,   ///< Still valid for verification; no new tokens issued with it
        REVOKED,   ///< No longer trusted – tokens signed with this kid are rejected
    };

    Status status = Status::ACTIVE;

    std::chrono::system_clock::time_point activated_at;

    std::chrono::system_clock::time_point demoted_at;

    std::chrono::seconds max_age{86400 * 30};  // default: 30 days

    bool isExpired() const {
        if (status != Status::ACTIVE) {
          return false;
        }
        auto age = std::chrono::system_clock::now() - activated_at;
        return age > max_age;
    }
};

class JWTKeyRotationManager {
public:
    struct Config {
        std::chrono::seconds passive_grace_period{86400};  // 24 h

        std::chrono::seconds max_key_age{86400 * 30};      // 30 days

        bool auto_revoke_expired_passive = false;

        size_t max_keys = 0;
    };

    explicit JWTKeyRotationManager(
        JWTValidator& validator,
        TokenBlacklist* blacklist = nullptr);
    JWTKeyRotationManager(
        JWTValidator& validator,
        TokenBlacklist* blacklist,
        const Config& config);

    // Non-copyable
    JWTKeyRotationManager(const JWTKeyRotationManager&) = delete;
    JWTKeyRotationManager& operator=(const JWTKeyRotationManager&) = delete;

    ~JWTKeyRotationManager();

    // ---------------------------------------------------------------------------
    // Key lifecycle
    // ---------------------------------------------------------------------------

    void rotateActiveKey(const std::string& new_kid,
                         std::optional<std::chrono::seconds> max_age = std::nullopt);

    /**
     * @brief Revoke Key.
     * @param[in] kid Input parameter.
     * @return True when the operation succeeds.
     */
    bool revokeKey(const std::string& kid);

    /**
     * @brief Reactivate Key.
     * @param[in] kid Input parameter.
     * @return True when the operation succeeds.
     */
    bool reactivateKey(const std::string& kid);

    // ---------------------------------------------------------------------------
    // Rotation policy
    // ---------------------------------------------------------------------------

    /**
     * @brief Is Rotation Due.
     * @return True when the operation succeeds.
     */
    bool isRotationDue() const;

    /**
     * @brief Check And Rotate.
     */
    void checkAndRotate();

    // ---------------------------------------------------------------------------
    // Queries
    // ---------------------------------------------------------------------------

    /**
     * @brief Active Key Id.
     * @return Return value.
     */
    std::string activeKeyId() const;

    /**
     * @brief Passive Key Ids.
     * @return Return value.
     */
    std::vector<std::string> passiveKeyIds() const;

    /**
     * @brief Revoked Key Ids.
     * @return Return value.
     */
    std::vector<std::string> revokedKeyIds() const;

    /**
     * @brief Get Key Info.
     * @param[in] kid Input parameter.
     * @return Return value.
     */
    std::optional<JWKKeyInfo> getKeyInfo(const std::string& kid) const;

    // ---------------------------------------------------------------------------
    // Statistics
    // ---------------------------------------------------------------------------

    struct Statistics {
        size_t total_keys     = 0;
        size_t active_keys    = 0;  // Should always be ≤ 1
        size_t passive_keys   = 0;
        size_t revoked_keys   = 0;
        uint64_t total_rotations = 0;
        uint64_t total_revocations = 0;
    };

    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

private:
    JWTValidator&    validator_;
    TokenBlacklist*  blacklist_;
    Config           config_;
    utils::AuditLogger* audit_logger_ = nullptr;  // non-owning, optional

    mutable std::mutex mutex_;
    std::unordered_map<std::string, JWKKeyInfo> keys_;
    uint64_t rotation_count_   = 0;
    uint64_t revocation_count_ = 0;
};

} // namespace auth
} // namespace themis
