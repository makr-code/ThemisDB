/**
 * @file api_key_authenticator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.15
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include "auth/auth_error.h"

#include <string>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <optional>

namespace themis {
namespace utils { class AuditLogger; }
namespace auth {

struct ApiKeyClaims {
    std::string key_id;       ///< The API key identifier that was authenticated
    std::string principal;    ///< Subject/user principal associated with this key
    std::string tenant_id;    ///< Optional tenant identifier
    std::vector<std::string> scopes; ///< Authorised scopes for this key
    std::vector<std::string> roles;  ///< Optional roles associated with this key
    std::chrono::system_clock::time_point expires_at; ///< Zero means no expiry

    bool isExpired() const {
        static const std::chrono::system_clock::time_point epoch{};
        if (expires_at == epoch) {
            return false;
        }
        return std::chrono::system_clock::now() > expires_at;
    }

    bool hasScope(const std::string& scope) const {
        for (const auto& s : scopes) {
            if (s == scope) {
              return true;
            }
        }
        return false;
    }
};

struct ApiKeyCredential {
    std::string key_id;          ///< Public key identifier (e.g., "sk_live_abc123")
    std::string secret_hash;     ///< SHA-256(secret) as lowercase hex
    std::string principal;       ///< Subject/user principal
    std::string tenant_id;       ///< Optional tenant identifier
    std::vector<std::string> scopes; ///< Authorised scopes
    std::vector<std::string> roles;  ///< Optional roles
    std::chrono::system_clock::time_point expires_at; ///< Zero = no expiry
    bool active{true};           ///< False means the key has been revoked
};

class ApiKeyAuthenticator {
public:
    struct Config {
        bool check_expiry{true};        ///< Reject keys whose expiry has passed
        size_t max_key_id_length{128};  ///< Maximum allowed key_id length
        size_t max_secret_length{512};  ///< Maximum allowed secret length
        /**
         * @brief Defaults.
         * @return Return value.
         * @details Implements defaults without additional internal calls.
         */
        static Config defaults() { return {}; }
    };

    explicit ApiKeyAuthenticator(const Config& config = Config::defaults());

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    // -------------------------------------------------------------------------
    // Credential management
    // -------------------------------------------------------------------------

    /**
     * @brief Add Credential.
     * @param[in] credential Input parameter.
     */
    void addCredential(const ApiKeyCredential& credential);

    /**
     * @brief Remove Credential.
     * @param[in] key_id Identifier of the key.
     */
    void removeCredential(const std::string& key_id);

    /**
     * @brief Credential Count.
     * @return Return value.
     */
    size_t credentialCount() const;

    // -------------------------------------------------------------------------
    // Authentication
    // -------------------------------------------------------------------------

    /**
     * @brief Authenticate.
     * @param[in] key_id Identifier of the key.
     * @param[in] secret Input parameter.
     * @return Authentication result.
     */
    ApiKeyClaims authenticate(const std::string& key_id,
                               const std::string& secret);

    /**
     * @brief Authenticate Combined.
     * @param[in] combined Input parameter.
     * @return Return value.
     */
    ApiKeyClaims authenticateCombined(const std::string& combined);

    // -------------------------------------------------------------------------
    // Static helpers
    // -------------------------------------------------------------------------

    /**
     * @brief Hash Secret.
     * @param[in] secret Input parameter.
     * @return Return value.
     */
    static std::string hashSecret(const std::string& secret);

    static ApiKeyCredential createCredential(
        const std::string& key_id,
        const std::string& secret,
        const std::string& principal,
        const std::vector<std::string>& scopes = {},
        const std::vector<std::string>& roles = {},
        const std::string& tenant_id = "",
        std::chrono::system_clock::time_point expires_at =
            std::chrono::system_clock::time_point{}
    );

private:
    Config config_;
    mutable std::mutex mutex_;
    std::unordered_map<std::string, ApiKeyCredential> credentials_;
    utils::AuditLogger* audit_logger_{nullptr};  ///< Non-owning; may be nullptr.

    /**
     * @brief Constant Time Equal.
     * @param[in] a Input parameter.
     * @param[in] b Input parameter.
     * @return True when the operation succeeds.
     */
    static bool constantTimeEqual(const std::string& a, const std::string& b);
    /**
     * @brief Hex Encode.
     * @param[in] data Input parameter.
     * @param[in] len Input parameter.
     * @return Return value.
     */
    static std::string hexEncode(const unsigned char* data, size_t len);

    /**
     * @brief Claims From Credential.
     * @param[in] cred Input parameter.
     * @return Return value.
     */
    ApiKeyClaims claimsFromCredential(const ApiKeyCredential& cred) const;
};

} // namespace auth
} // namespace themis
