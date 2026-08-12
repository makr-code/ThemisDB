/**
 * @file secret_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <chrono>
#include <mutex>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace themis {
namespace security {

/**
 * @brief Rotation policy for the SecretManager.
 *
 * Declared separately so it can be used as a default function argument
 * without triggering "default member initializer not yet parsed" errors.
 */
struct SecretRotationPolicy {
    /// How long until isRotationDue() returns true (default 90 days)
    std::chrono::seconds max_age{86400LL * 90};
    /// How long a RETIRING version stays accessible before auto-revoke
    std::chrono::seconds retiring_grace_period{86400LL};  // 1 day
    /// When true, checkAndRevoke() automatically revokes expired RETIRING versions
    bool auto_revoke_expired_retiring = true;
    /// Maximum number of named secrets that can be stored. 0 means unlimited.
    size_t max_secrets = 0;
    /// Maximum number of versions kept per secret (including REVOKED). 0 means unlimited.
    size_t max_versions_per_secret = 0;
};

/**
 * @brief Versioned application-secret storage with rotation support
 *
 * SecretManager stores named secrets (API keys, database passwords, service
 * credentials, etc.) with full version history and a simple lifecycle:
 *
 *   ACTIVE   – The current version returned by getSecret().
 *   RETIRING – A previously-active version kept accessible during the grace
 *              period so consumers can migrate before the secret is revoked.
 *   REVOKED  – No longer returned by getSecret(); kept only for audit history.
 *
 * Rotation workflow
 * -----------------
 * 1. Call storeSecret() once to create version 1 (ACTIVE).
 * 2. When the secret needs to change, call rotateSecret() which atomically
 *    - creates a new ACTIVE version with the new value
 *    - moves the old ACTIVE version to RETIRING
 * 3. After the grace period, call revokeVersion() or let checkAndRevoke()
 *    automatically transition RETIRING → REVOKED.
 *
 * Secret values are stored in-memory only.  Attach an external store or
 * extend this class if persistence is needed.
 *
 * Thread-safety: all public methods are thread-safe.
 */
class SecretManager {
public:
    using RotationPolicy = SecretRotationPolicy;

    enum class SecretStatus {
        ACTIVE,    ///< Current version; returned by getSecret()
        RETIRING,  ///< Old version; accessible during grace period
        REVOKED,   ///< No longer accessible; kept for audit history
    };

    struct SecretVersion {
        uint32_t    version    = 0;
        std::string value;             ///< Actual secret value
        SecretStatus status            = SecretStatus::ACTIVE;
        std::chrono::system_clock::time_point created_at;
        std::chrono::system_clock::time_point expires_at;  ///< epoch-zero means never
        std::string created_by;
        std::string description;
    };

    /// Lightweight per-version information returned by listVersions() –
    /// deliberately omits the secret value for safe enumeration.
    struct VersionInfo {
        uint32_t     version    = 0;
        SecretStatus status     = SecretStatus::ACTIVE;
        std::chrono::system_clock::time_point created_at;
        std::string  created_by;
        std::string  description;
    };

    struct Statistics {
        size_t   total_secrets       = 0;
        size_t   active_versions     = 0;
        size_t   retiring_versions   = 0;
        size_t   revoked_versions    = 0;
        uint64_t total_rotations     = 0;
    };

    explicit SecretManager(RotationPolicy policy = RotationPolicy{});

    // -----------------------------------------------------------------------
    // Lifecycle
    // -----------------------------------------------------------------------

    /**
     * @brief Store a brand-new secret, creating version 1.
     *
     * @return Version number (1 on first call).
     * @throws std::invalid_argument if a secret with this name already exists.
     */
    uint32_t storeSecret(const std::string& name,
                         const std::string& value,
                         const std::string& created_by  = "",
                         const std::string& description = "");

    /**
     * @brief Get the current ACTIVE version of a secret.
     *
     * @return nullopt if no secret with this name exists or no active version.
     */
    std::optional<SecretVersion> getSecret(const std::string& name) const;

    /**
     * @brief Get a specific version regardless of its status.
     *
     * Useful for audit and transition: callers may still read RETIRING values
     * so they can migrate to the new version before it is revoked.
     *
     * @return nullopt if the name or version is unknown.
     */
    std::optional<SecretVersion> getSecretVersion(const std::string& name,
                                                  uint32_t version) const;

    /**
     * @brief Rotate a secret: store a new ACTIVE version, move the previous
     *        ACTIVE version to RETIRING.
     *
     * @return The new version number.
     * @throws std::invalid_argument if no secret with this name exists.
     */
    uint32_t rotateSecret(const std::string& name,
                          const std::string& new_value,
                          const std::string& created_by = "");

    /**
     * @brief Explicitly revoke a specific version (ACTIVE or RETIRING → REVOKED).
     *
     * @return true if found and revoked; false if already REVOKED or unknown.
     */
    bool revokeVersion(const std::string& name, uint32_t version);

    /**
     * @brief Delete a secret and all its versions permanently.
     *
     * @return true if the secret existed.
     */
    bool deleteSecret(const std::string& name);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /** @brief List all version metadata for a secret (values not included). */
    std::vector<VersionInfo> listVersions(const std::string& name) const;

    /** @brief List all secret names. */
    std::vector<std::string> listSecrets() const;

    /**
     * @brief Return true if the active version's age exceeds policy.max_age.
     *
     * @return false if the secret does not exist.
     */
    bool isRotationDue(const std::string& name) const;

    // -----------------------------------------------------------------------
    // Housekeeping
    // -----------------------------------------------------------------------

    /**
     * @brief Housekeeping: auto-revoke RETIRING versions whose grace period
     *        has expired (only runs when policy.auto_revoke_expired_retiring).
     */
    void checkAndRevoke();

    // -----------------------------------------------------------------------
    // Statistics
    // -----------------------------------------------------------------------

    Statistics getStatistics() const;

private:
    struct SecretEntry {
        std::string              name;
        std::vector<SecretVersion> versions;   ///< sorted ascending by version
        uint32_t                 next_version = 1;
        uint64_t                 rotation_count = 0;
    };

    SecretVersion& findVersion(SecretEntry& entry, uint32_t version);
    const SecretVersion* findVersionConst(const SecretEntry& entry,
                                          uint32_t version) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, SecretEntry> secrets_;
    RotationPolicy policy_;
    uint64_t total_rotations_ = 0;
};

}  // namespace security
}  // namespace themis
