/**
 * @file jwt_key_rotation_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
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

/**
 * @brief Key metadata tracked per JWK key ID (kid)
 */
struct JWKKeyInfo {
    std::string kid = {};

    enum class Status {
        ACTIVE,    ///< Current signing key – issued tokens have this kid
        PASSIVE,   ///< Still valid for verification; no new tokens issued with it
        REVOKED,   ///< No longer trusted – tokens signed with this kid are rejected
    };

    Status status = Status::ACTIVE;

    /// When this key was promoted to ACTIVE
    std::chrono::system_clock::time_point activated_at;

    /// When this key was demoted to PASSIVE (zero if never demoted)
    std::chrono::system_clock::time_point demoted_at;

    /// Maximum lifetime before rotation is mandatory
    std::chrono::seconds max_age{86400 * 30};  // default: 30 days

    bool isExpired() const {
        if (status != Status::ACTIVE) {
          return false;
        }
        auto age = std::chrono::system_clock::now() - activated_at;
        return age > max_age;
    }
};

/**
 * @brief JWT Key Rotation Manager
 *
 * Tracks the lifecycle of JWK key IDs used for JWT signing and verification:
 *
 *   ACTIVE   → issued tokens carry this kid; signature verification passes
 *   PASSIVE  → new tokens MUST NOT use this kid; existing tokens still verified
 *   REVOKED  → tokens signed with this kid are outright rejected
 *
 * Rotation workflow:
 *   1. Call rotateActiveKey(new_kid) – marks old active key as PASSIVE,
 *      new key becomes ACTIVE.
 *   2. After the grace period, call revokePassiveKey(old_kid) – moves it to
 *      REVOKED and adds it to the JWTValidator denylist.
 *   3. Optionally, associate a TokenBlacklist to also mass-revoke any issued
 *      JTIs that belong to the old key (call blacklistTokensForKid).
 *
 * Thread-safety: all public methods are thread-safe.
 */
class JWTKeyRotationManager {
public:
    struct Config {
        /// Duration a PASSIVE key remains valid for verification before auto-revocation.
        std::chrono::seconds passive_grace_period{86400};  // 24 h

        /// Maximum key age before isRotationDue() returns true.
        std::chrono::seconds max_key_age{86400 * 30};      // 30 days

        /// If true, revokePassiveKey() is also called automatically when a
        /// passive key's grace period expires on the next checkAndRotate() call.
        bool auto_revoke_expired_passive = false;

        /// Maximum total number of tracked keys (ACTIVE + PASSIVE + REVOKED).
        /// rotateActiveKey() throws std::length_error when reached.
        /// 0 means unlimited (default).
        size_t max_keys = 0;
    };

    /**
     * @param validator  JWTValidator whose kid denylist is updated on revocation.
     * @param blacklist  Optional TokenBlacklist for mass-revoking JTIs.
     */
    explicit JWTKeyRotationManager(
        JWTValidator& validator,
        TokenBlacklist* blacklist = nullptr);
    /**
     * @param validator  JWTValidator whose kid denylist is updated on revocation.
     * @param blacklist  Optional TokenBlacklist for mass-revoking JTIs.
     * @param config     Rotation policy configuration.
     */
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

    /**
     * @brief Register a new key as the active signing key.
     *
     * Any previously ACTIVE key is demoted to PASSIVE.
     * The caller is responsible for actually updating the signing service to
     * use the new kid – this class only tracks state.
     *
     * @param new_kid  The key ID of the new signing key.
     * @param max_age  Optional override for this key's rotation schedule.
     */
    void rotateActiveKey(const std::string& new_kid,
                         std::optional<std::chrono::seconds> max_age = std::nullopt);

    /**
     * @brief Explicitly revoke a key (moves ACTIVE or PASSIVE → REVOKED).
     *
     * Adds the kid to JWTValidator's runtime denylist so all future tokens
     * signed with it are rejected immediately.
     *
     * @param kid  Key ID to revoke.
     * @return true if the key was found and revoked; false if unknown.
     */
    bool revokeKey(const std::string& kid);

    /**
     * @brief Re-activate a key that was previously set to PASSIVE.
     *
     * Intended for emergency rollback scenarios where the new key must be
     * abandoned.  Returns false if the key is REVOKED or unknown.
     *
     * @param kid  Key ID to re-activate.
     * @return true on success.
     */
    bool reactivateKey(const std::string& kid);

    // ---------------------------------------------------------------------------
    // Rotation policy
    // ---------------------------------------------------------------------------

    /**
     * @brief Check whether the current active key is due for rotation.
     *
     * Returns true if:
     *   - No active key is registered (rotation needed to establish one), OR
     *   - The active key's age exceeds config_.max_key_age.
     */
    bool isRotationDue() const;

    /**
     * @brief Perform deferred housekeeping (auto-revoke expired passive keys).
     *
     * Call this periodically (e.g., from a background thread) to enforce the
     * passive grace period automatically.
     */
    void checkAndRotate();

    // ---------------------------------------------------------------------------
    // Queries
    // ---------------------------------------------------------------------------

    /** @brief Return the kid of the currently ACTIVE key, or empty string. */
    std::string activeKeyId() const;

    /** @brief Return all key IDs with PASSIVE status. */
    std::vector<std::string> passiveKeyIds() const;

    /** @brief Return all key IDs with REVOKED status. */
    std::vector<std::string> revokedKeyIds() const;

    /** @brief Return the full key info for a specific kid, or nullopt. */
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

    Statistics getStatistics() const;

    /**
     * @brief Attach an AuditLogger that receives KEY_ROTATED / KEY_DELETED events.
     * Pass nullptr to detach. The manager does NOT take ownership.
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
