/**
 * @file jwt_validator.h
 * @brief Canonical Doxygen file header for ThemisDB-generated maturity metadata.
 * @version 0.0.47
 * @note Maturity: 🟢 PRODUCTION-READY
 * @note Score: 86/100
 * @note Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * @note Status: Production Ready
 * @note This block is auto-generated and will be overwritten.
 */

/*
 * ThemisDB | File: jwt_validator.h | Version: 0.0.47 | Last Modified: 2026-05-31 12:17:24
 * Author: makr-code | Maturity: 🟢 PRODUCTION-READY | Score: 100/100 | Lines: 246
 * Gap Summary: total=3; TODO=1, Stub=1, Unimpl=0, Mock=1, Sim=0, Debt=0, C=n/a, H=n/a, M=n/a, L=n/a
 * PR History (last 5): #4781 Harden JWTValidator JWKS si... (2026-04-22) | #4386 [WIP] Update documentation ... (2026-03-22) | #4279 feat(auth): JWT scope extra... (2026-03-16) | #4119 feat(auth): JWT JTI replay ... (2026-03-12) | #4113 feat(auth): Async / Non-Blo... (2026-03-12)
 * Status: Production Ready
 * (Automatisch generiert, Änderungen werden überschrieben)
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
    
    /**
     * @brief Check whether the token is already expired at call time.
     * @return true when current system time is greater than expiration.
     */
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
    std::chrono::milliseconds cache_ttl{600000};
    std::chrono::milliseconds clock_skew{60000};
    std::vector<std::string> revoked_kids;                   // Kid denylist for revoked keys
    int jwks_timeout_seconds{DEFAULT_JWKS_TIMEOUT_SECONDS};  // JWKS fetch timeout
    int jwks_max_retries{MAX_JWKS_RETRY_ATTEMPTS};           // JWKS fetch max retries
    bool require_issuer_validation = true;   // throw at construction if expected_issuer is unset
    bool require_audience_validation = true; // throw at construction if expected_audience is unset
    bool require_jti = false;                // when true, reject tokens that are missing the jti claim
    /// Maximum time to wait for a concurrent JWKS refresh to complete.
    /// If the refresher thread does not finish within this window, validation
    /// proceeds with the stale cache (or returns an empty set if no cache exists).
    std::chrono::milliseconds refresh_wait_timeout{15'000};
};

class JWTValidator {
public:
    /**
     * @brief Initialize with Keycloak JWKS endpoint
     * @param jwks_url URL to Keycloak JWKS endpoint
     *        Example: https://keycloak.vcc.local/realms/vcc/protocol/openid-connect/certs
     */
    explicit JWTValidator(const std::string& jwks_url);

    /**
     * @brief Initialize validator with full runtime configuration.
     * @param cfg Validator config including JWKS endpoint, cache policy, and validation rules.
     * @throws std::runtime_error when required issuer/audience validation is enabled but not configured.
     */
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
    /**
     * @brief Decode a Base64URL-encoded string into raw bytes.
     * @param input Base64URL input without padding requirements.
     * @return Decoded bytes, or empty vector when decoding fails.
     */
    std::vector<uint8_t> decodeBase64Url(const std::string& input);

    /**
     * @brief Decode a Base64URL string and return UTF-8 text payload.
     * @param input Base64URL input.
     * @return Decoded text string (may be empty on decode failure).
     */
    std::string decodeBase64UrlToString(const std::string& input);

    /**
     * @brief Fetch and validate JWKS with cache and single-flight refresh semantics.
     * @return Parsed JWKS JSON object.
     * @throws std::runtime_error when HTTP fetch or JWKS validation fails.
     */
    nlohmann::json fetchJWKS();

    /**
     * @brief Find matching JWK entry by key id.
     * @param jwks JWKS JSON document containing a keys array.
     * @param kid Key id to search for.
     * @return Pointer to matching JWK object, or nullptr if not found.
     */
    const nlohmann::json* findJwkForKid(const nlohmann::json& jwks, const std::string& kid) const;

    /**
     * @brief Verify RS256 signature using an RSA JWK.
     * @param header_payload JWT signing input (header.payload).
     * @param signature Decoded signature bytes.
     * @param jwk RSA JSON Web Key.
     * @return true when signature verification succeeds.
     */
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

    /**
     * @brief Validate configured audience against token payload.
     * @param payload Parsed JWT payload JSON.
     * @return true when audience requirements are satisfied.
     */
    bool checkAudience(const nlohmann::json& payload) const;
    
    // testing helper
public:
    /**
     * @brief Inject JWKS cache content for deterministic tests.
     * @param jwks JWKS document to store as current cache.
     * @param t Cache timestamp associated with the injected JWKS.
     */
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
