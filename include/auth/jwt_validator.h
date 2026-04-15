/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jwt_validator.h                                    ║
  Version:         0.0.43                                             ║
  Last Modified:   2026-04-15 04:09:15                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     261                                            ║
    • Open Issues:     TODOs: 0, Stubs: 0                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 435595de1f  2026-03-22  Changes before error encountered        ║
    • 76eef4d701  2026-03-15  feat(auth): implement JWT scope extraction and role-to-sc... ║
    • c97360e579  2026-03-15  fix(auth,scheduler): JWT scope enforcement, Kerberos role... ║
    • 3071a3bb79  2026-03-12  fix(auth): address JWT JTI reviewer feedback ║
    • 6903f59100  2026-03-12  feat(auth): JWT JTI replay prevention warning (v1.2.0) ║
╠═════════════════════════════════════════════════════════════════════╣
  Status: ✅ Production Ready                                          ║
╚═════════════════════════════════════════════════════════════════════╝
 */

#pragma once

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <chrono>
#include <optional>
#include <memory>
#include <shared_mutex>
#include <atomic>
#include <future>

#include "auth/token_blacklist.h"
#include "auth/auth_worker_thread_pool.h"

namespace themis { namespace utils { class AuditLogger; } }

namespace themis {
namespace auth {

/**
 * @brief JWT token claims
 */
struct JWTClaims {
    std::string sub;                          // Subject (user ID)
    std::string jti;                          // JWT ID – used for per-token revocation
    std::string email;
    std::string tenant_id;                    // Tenant ID from JWT claim
    std::vector<std::string> groups;
    std::vector<std::string> roles;
    /// OAuth2 `scope` / `scp` claim – space-separated or array.
    /// Populated by parseAndValidate() from the `scope` (string) or `scp` (array) claim.
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

/**
 * @brief JWT Validator for Keycloak OIDC tokens
 * 
 * Features:
 * - Parse JWT tokens (header.payload.signature)
 * - Validate signature using JWKS from Keycloak
 * - Check expiration and issuer
 * - Extract claims for access control
 * - Kid revocation/denylist support
 * - Metrics and logging on validation failures
 */

// Input validation limits
constexpr size_t MAX_JWT_TOKEN_SIZE = 16 * 1024;  // 16KB max token size
constexpr size_t MAX_PRINCIPAL_NAME_LENGTH = 256; // 256 chars max for principal names
constexpr int DEFAULT_JWKS_TIMEOUT_SECONDS = 5;   // 5 second timeout for JWKS fetch
constexpr int MAX_JWKS_RETRY_ATTEMPTS = 3;        // Max 3 retry attempts for JWKS

struct JWTValidatorConfig {
    std::string jwks_url;                                    // Keycloak JWKS endpoint
    std::optional<std::string> expected_issuer;              // must be set when require_issuer_validation=true
    std::optional<std::string> expected_audience;            // must be set when require_audience_validation=true
    std::chrono::seconds cache_ttl{600};
    std::chrono::seconds clock_skew{60};
    std::vector<std::string> revoked_kids;                   // Kid denylist for revoked keys
    int jwks_timeout_seconds{DEFAULT_JWKS_TIMEOUT_SECONDS};  // JWKS fetch timeout
    int jwks_max_retries{MAX_JWKS_RETRY_ATTEMPTS};           // JWKS fetch max retries
    bool require_issuer_validation = true;   // throw at construction if expected_issuer is unset
    bool require_audience_validation = true; // throw at construction if expected_audience is unset
    bool require_jti = false;                // when true, reject tokens that are missing the jti claim
};

class JWTValidator {
public:
    /**
     * @brief Initialize with Keycloak JWKS endpoint
     * @param jwks_url URL to Keycloak JWKS endpoint
     *        Example: https://keycloak.vcc.local/realms/vcc/protocol/openid-connect/certs
     */
    explicit JWTValidator(const std::string& jwks_url);

    /** Initialize with full config */
    explicit JWTValidator(const JWTValidatorConfig& cfg);
    
    /**
     * @brief Parse and validate JWT token
     * @param token Bearer token (with or without "Bearer " prefix)
     * @return Parsed claims if valid
     * @throws std::runtime_error if invalid/expired
     */
    JWTClaims parseAndValidate(const std::string& token);

    /**
     * @brief Non-blocking variant of parseAndValidate().
     *
     * Dispatches token validation (including any JWKS refresh) to the internal
     * AuthWorkerThreadPool so the calling thread is never stalled.
     *
     * Performance target: JWKS refresh never blocks the validation hot path
     * for more than 1 ms (visible to callers).
     *
     * @param token Bearer token (with or without "Bearer " prefix)
     * @return std::future<JWTClaims> — becomes ready when validation completes
     * @throws std::runtime_error if the internal thread pool is not running
     */
    std::future<JWTClaims> validateAsync(const std::string& token);
    
    /**
     * @brief Derive user-specific encryption key from DEK
     * @param dek Base data encryption key
     * @param claims JWT claims for user context
     * @param field_name Field identifier for HKDF context
     * @return User-specific field key
     */
    static std::vector<uint8_t> deriveUserKey(
        const std::vector<uint8_t>& dek,
        const JWTClaims& claims,
        const std::string& field_name
    );
    
    /**
     * @brief Check if user has access to group-encrypted data
     * @param claims User's JWT claims
     * @param encryption_context Context used for encryption (user_id or group name)
     */
    static bool hasAccess(const JWTClaims& claims, const std::string& encryption_context);
    
    /**
     * @brief Attach a TokenBlacklist for per-token (JTI) revocation checks.
     *
     * When set, parseAndValidate() will extract the "jti" claim from the
     * token payload and reject any token whose JTI appears in the blacklist.
     * The validator does NOT take ownership; the caller must ensure the
     * blacklist outlives the validator.
     *
     * @param bl Pointer to the TokenBlacklist (nullptr to detach).
     */
    void setTokenBlacklist(TokenBlacklist* bl);

    /**
     * @brief Attach an AuditLogger to receive LOGIN_SUCCESS / LOGIN_FAILED events.
     * Pass nullptr to detach.  The validator does NOT take ownership.
     */
    void setAuditLogger(utils::AuditLogger* logger) { audit_logger_ = logger; }

    /**
     * @brief Add a key ID to the revocation list
     * @param kid Key ID to revoke
     */
    void revokeKid(const std::string& kid);
    
    /**
     * @brief Check if a key ID is revoked
     * @param kid Key ID to check
     * @return true if the kid is revoked
     */
    bool isKidRevoked(const std::string& kid) const;

private:
    std::vector<uint8_t> decodeBase64Url(const std::string& input);
    std::string decodeBase64UrlToString(const std::string& input);
    nlohmann::json fetchJWKS();
    const nlohmann::json* findJwkForKid(const nlohmann::json& jwks, const std::string& kid) const;
    bool verifySignatureRS256(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);
    /**
     * @brief Verify an RSA JWT signature for the given algorithm (RS256/RS384/RS512).
     *
     * Supports SHA-256 (RS256), SHA-384 (RS384), and SHA-512 (RS512) digest algorithms.
     */
    bool verifySignatureRSA(const std::string& header_payload,
                            const std::vector<uint8_t>& signature,
                            const nlohmann::json& jwk,
                            const std::string& alg);
    bool verifySignatureES256(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);
    /**
     * @brief Verify an ECDSA JWT signature for the given algorithm (ES256/ES384/ES512).
     *
     * Dispatches to the appropriate curve (P-256, P-384, P-521) and hash (SHA-256,
     * SHA-384, SHA-512) based on the algorithm label.
     */
    bool verifySignatureEC(const std::string& header_payload,
                           const std::vector<uint8_t>& signature,
                           const nlohmann::json& jwk,
                           const std::string& alg);
    /**
     * @brief Verify an EdDSA (Ed25519) JWT signature.
     *
     * Expects a JWK of type "OKP" with crv="Ed25519" and a 32-byte base64url-
     * encoded public key in the "x" field.  Requires OpenSSL ≥ 1.1.1.
     */
    bool verifySignatureEdDSA(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);
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

    /// Prevents concurrent JWKS refresh stampedes: only one thread performs the
    /// HTTP fetch at a time; others wait on jwks_refresh_cv_ for it to finish.
    mutable std::mutex jwks_refresh_mutex_;
    mutable std::condition_variable jwks_refresh_cv_;
    mutable bool jwks_refreshing_{false};

    /// Worker thread pool for validateAsync().
    /// LIFETIME NOTE: worker_pool_ MUST be the last data member declared.
    /// C++ destroys members in reverse-declaration order, so worker_pool_ is
    /// destroyed first — its shutdown() joins all in-flight tasks before any
    /// other member (cache, config, etc.) is released.  This guarantees that
    /// tasks capturing 'this' never access a dangling member.
    std::unique_ptr<AuthWorkerThreadPool> worker_pool_;
};

} // namespace auth
} // namespace themis
