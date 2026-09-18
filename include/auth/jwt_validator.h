/**
 * @file jwt_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */


#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
#include <shared_mutex>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <future>

#include "auth/token_blacklist.h"
#include "auth/auth_worker_thread_pool.h"

namespace themis { namespace utils { class AuditLogger; } }

namespace themis {
namespace auth {

struct JWTClaims {
    std::string sub;                          // Subject (user ID)
    std::string jti;                          // JWT ID – used for per-token revocation
    std::string email;
    std::string tenant_id;                    // Tenant ID from JWT claim
    std::vector<std::string> groups;
    std::vector<std::string> roles;
    std::vector<std::string> scopes;
    std::string issuer;
    std::chrono::system_clock::time_point expiration;
    std::optional<std::chrono::system_clock::time_point> not_before;
    std::optional<std::chrono::system_clock::time_point> issued_at;
    std::vector<std::string> audience;
    
    bool isExpired() const {
        return std::chrono::system_clock::now() > expiration;
    }
};


// Input validation limits
constexpr size_t MAX_JWT_TOKEN_SIZE = 16 * 1024;  // 16KB max token size
constexpr size_t MAX_PRINCIPAL_NAME_LENGTH = 256; // 256 chars max for principal names
constexpr int DEFAULT_JWKS_TIMEOUT_SECONDS = 5;   // 5 second timeout for JWKS fetch
constexpr int MAX_JWKS_RETRY_ATTEMPTS = 3;        // Max 3 retry attempts for JWKS

struct JWTValidatorConfig {
    std::string jwks_url;                                    // Keycloak JWKS endpoint
    std::optional<std::string> expected_issuer;              // must be set when require_issuer_validation=true
    std::optional<std::string> expected_audience;            // must be set when require_audience_validation=true
    std::chrono::milliseconds cache_ttl{600000};
    std::chrono::milliseconds clock_skew{60000};
    std::vector<std::string> revoked_kids;                   // Kid denylist for revoked keys
    int jwks_timeout_seconds{DEFAULT_JWKS_TIMEOUT_SECONDS};  // JWKS fetch timeout
    int jwks_max_retries{MAX_JWKS_RETRY_ATTEMPTS};           // JWKS fetch max retries
    bool require_issuer_validation = true;   // throw at construction if expected_issuer is unset
    bool require_audience_validation = true; // throw at construction if expected_audience is unset
    bool require_jti = false;                // when true, reject tokens that are missing the jti claim
    std::chrono::milliseconds refresh_wait_timeout{15'000};
};

class JWTValidator {
public:
    /**
     * @brief JWTValidator.
     * @param[in] jwks_url Input parameter.
     * @return Return value.
     */
    explicit JWTValidator(const std::string& jwks_url);

    /**
     * @brief JWTValidator.
     * @param[in] cfg Input parameter.
     * @return Return value.
     */
    explicit JWTValidator(const JWTValidatorConfig& cfg);
    
    /**
     * @brief Parse And Validate.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    JWTClaims parseAndValidate(const std::string& token);

    /**
     * @brief Validate Async.
     * @param[in] token Input parameter.
     * @return Return value.
     */
    std::future<JWTClaims> validateAsync(const std::string& token);
    
    /**
     * @brief Derive User Key.
     * @param[in] dek Input parameter.
     * @param[in] claims Input parameter.
     * @param[in] field_name Name of the field.
     * @return Return value.
     */
    static std::vector<uint8_t> deriveUserKey(
        const std::vector<uint8_t>& dek,
        const JWTClaims& claims,
        const std::string& field_name
    );
    
    /**
     * @brief Has Access.
     * @param[in] claims Input parameter.
     * @param[in] encryption_context Input parameter.
     * @return True when the operation succeeds.
     */
    static bool hasAccess(const JWTClaims& claims, const std::string& encryption_context);
    
    /**
     * @brief Set Token Blacklist.
     * @param[in,out] bl Input/output parameter.
     */
    void setTokenBlacklist(TokenBlacklist* bl);

    /**
     * @brief Set Audit Logger.
     * @param[in,out] logger Input/output parameter.
     * @details Implements setAuditLogger without additional internal calls.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Revoke Kid.
     * @param[in] kid Input parameter.
     */
    void revokeKid(const std::string& kid);
    
    /**
     * @brief Is Kid Revoked.
     * @param[in] kid Input parameter.
     * @return True when the operation succeeds.
     */
    bool isKidRevoked(const std::string& kid) const;

private:
    /**
     * @brief Decode Base64 Url.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::vector<uint8_t> decodeBase64Url(const std::string& input);

    /**
     * @brief Decode Base64 Url To String.
     * @param[in] input Input parameter.
     * @return Return value.
     */
    std::string decodeBase64UrlToString(const std::string& input);

    /**
     * @brief Fetch JWKS.
     * @return Return value.
     */
    nlohmann::json fetchJWKS();

    /**
     * @brief Find Jwk For Kid.
     * @param[in] jwks Input parameter.
     * @param[in] kid Input parameter.
     * @return Pointer to the result.
     */
    const nlohmann::json* findJwkForKid(const nlohmann::json& jwks, const std::string& kid) const;

    /**
     * @brief Verify Signature RS256.
     * @param[in] header_payload Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] jwk Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignatureRS256(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);
    /**
     * @brief Verify Signature RSA.
     * @param[in] header_payload Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] jwk Input parameter.
     * @param[in] alg Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignatureRSA(const std::string& header_payload,
                            const std::vector<uint8_t>& signature,
                            const nlohmann::json& jwk,
                            const std::string& alg);
    /**
     * @brief Verify Signature ES256.
     * @param[in] header_payload Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] jwk Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignatureES256(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);
    /**
     * @brief Verify Signature EC.
     * @param[in] header_payload Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] jwk Input parameter.
     * @param[in] alg Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignatureEC(const std::string& header_payload,
                           const std::vector<uint8_t>& signature,
                           const nlohmann::json& jwk,
                           const std::string& alg);
    /**
     * @brief Verify Signature Ed DSA.
     * @param[in] header_payload Input parameter.
     * @param[in] signature Input parameter.
     * @param[in] jwk Input parameter.
     * @return True when the operation succeeds.
     */
    bool verifySignatureEdDSA(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);

    /**
     * @brief Check Audience.
     * @param[in] payload Input parameter.
     * @return True when the operation succeeds.
     */
    bool checkAudience(const nlohmann::json& payload) const;
    
    // testing helper
public:
    void setJWKSForTesting(const nlohmann::json& jwks,
                           std::chrono::system_clock::time_point t = std::chrono::system_clock::now());
private:
    JWTValidatorConfig cfg_;
    std::string jwks_url_;
    mutable std::shared_mutex jwks_cache_mutex_;
    nlohmann::json jwks_cache_;
    std::chrono::system_clock::time_point jwks_cache_time_;
    std::vector<std::string> revoked_kids_runtime_;  // Runtime revocation list
    TokenBlacklist* token_blacklist_ = nullptr;      // Optional JTI-based revocation
    utils::AuditLogger* audit_logger_ = nullptr;     // Optional audit logger (non-owning)
    mutable std::atomic<bool> warned_blacklist_no_jti_{false};  // Warn once when blacklist set but token has no jti

    mutable std::mutex jwks_refresh_mutex_;
    mutable std::condition_variable jwks_refresh_cv_;
    mutable bool jwks_refreshing_{false};

    std::unique_ptr<AuthWorkerThreadPool> worker_pool_;
};

} // namespace auth
} // namespace themis
