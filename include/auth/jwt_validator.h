/*
╔═════════════════════════════════════════════════════════════════════╗
║ ThemisDB - Hybrid Database System                                   ║
╠═════════════════════════════════════════════════════════════════════╣
  File:            jwt_validator.h                                    ║
  Version:         0.0.20                                             ║
  Last Modified:   2026-02-21 19:14:23                                ║
  Author:          unknown                                            ║
╠═════════════════════════════════════════════════════════════════════╣
  Quality Metrics:                                                    ║
    • Maturity Level:  🟢 PRODUCTION-READY                             ║
    • Quality Score:   100.0/100                                      ║
    • Total Lines:     192                                            ║
    • Open Issues:     TODOs: 0, Stubs: 1                             ║
╠═════════════════════════════════════════════════════════════════════╣
  Revision History:                                                   ║
    • 468bda607  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 189cdf5b1  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • a5676b06f  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • 56752fde6  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
    • c3f305f42  2026-02-21  🤖 Auto-update: Code maturity analysis & versioning [skip ci] ║
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

#include "auth/token_blacklist.h"

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
    std::string jwks_url;               // Keycloak JWKS endpoint
    std::string expected_issuer;        // optional: exact match required if set
    std::string expected_audience;      // optional: must be contained in aud if set
    std::chrono::seconds cache_ttl{600};
    std::chrono::seconds clock_skew{60};
    std::vector<std::string> revoked_kids;  // Kid denylist for revoked keys
    int jwks_timeout_seconds{DEFAULT_JWKS_TIMEOUT_SECONDS};  // JWKS fetch timeout
    int jwks_max_retries{MAX_JWKS_RETRY_ATTEMPTS};            // JWKS fetch max retries
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
    bool verifySignatureES256(const std::string& header_payload,
                              const std::vector<uint8_t>& signature,
                              const nlohmann::json& jwk);
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
    nlohmann::json jwks_cache_;
    std::chrono::system_clock::time_point jwks_cache_time_;
    std::vector<std::string> revoked_kids_runtime_;  // Runtime revocation list
    TokenBlacklist* token_blacklist_ = nullptr;      // Optional JTI-based revocation
};

} // namespace auth
} // namespace themis
