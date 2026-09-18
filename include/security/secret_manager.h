/**
 * @file secret_manager.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
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

struct SecretRotationPolicy {
    std::chrono::seconds max_age{86400LL * 90};
    std::chrono::seconds retiring_grace_period{86400LL};  // 1 day
    bool auto_revoke_expired_retiring = true;
    size_t max_secrets = 0;
    size_t max_versions_per_secret = 0;
};

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

    uint32_t storeSecret(const std::string& name,
                         const std::string& value,
                         const std::string& created_by  = "",
                         const std::string& description = "");

    /**
     * @brief Get Secret.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::optional<SecretVersion> getSecret(const std::string& name) const;

    /**
     * @brief Get Secret Version.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    std::optional<SecretVersion> getSecretVersion(const std::string& name,
                                                  uint32_t version) const;

    uint32_t rotateSecret(const std::string& name,
                          const std::string& new_value,
                          const std::string& created_by = "");

    /**
     * @brief Revoke Version.
     * @param[in] name Input parameter.
     * @param[in] version Input parameter.
     * @return True when the operation succeeds.
     */
    bool revokeVersion(const std::string& name, uint32_t version);

    /**
     * @brief Delete Secret.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool deleteSecret(const std::string& name);

    // -----------------------------------------------------------------------
    // Query
    // -----------------------------------------------------------------------

    /**
     * @brief List Versions.
     * @param[in] name Input parameter.
     * @return Return value.
     */
    std::vector<VersionInfo> listVersions(const std::string& name) const;

    /**
     * @brief List Secrets.
     * @return Return value.
     */
    std::vector<std::string> listSecrets() const;

    /**
     * @brief Is Rotation Due.
     * @param[in] name Input parameter.
     * @return True when the operation succeeds.
     */
    bool isRotationDue(const std::string& name) const;

    // -----------------------------------------------------------------------
    // Housekeeping
    // -----------------------------------------------------------------------

    /**
     * @brief Check And Revoke.
     */
    void checkAndRevoke();


    /**
     * @brief Return access control statistics.
     * @return Access control statistics.
     */
    Statistics getStatistics() const;

private:
    struct SecretEntry {
        std::string              name;
        std::vector<SecretVersion> versions;   ///< sorted ascending by version
        uint32_t                 next_version = 1;
        uint64_t                 rotation_count = 0;
    };

    /**
     * @brief Find Version.
     * @param[in,out] entry Input/output parameter.
     * @param[in] version Input parameter.
     * @return Return value.
     */
    SecretVersion& findVersion(SecretEntry& entry, uint32_t version);
    /**
     * @brief Find Version Const.
     * @param[in] entry Input parameter.
     * @param[in] version Input parameter.
     * @return Pointer to the result.
     */
    const SecretVersion* findVersionConst(const SecretEntry& entry,
                                          uint32_t version) const;

    mutable std::mutex mutex_;
    std::unordered_map<std::string, SecretEntry> secrets_;
    RotationPolicy policy_;
    uint64_t total_rotations_ = 0;
};

}  // namespace security
}  // namespace themis
